// Nested (double) cross-validation for LR, linear SVM, and a small MLP.
// Input rows from Spring:
//   block pairId label tilt_mean tilt_std tilt_rms
// Features are gravity-tilt half differences relative to the eyes-open
// first-half baseline. Outer folds hold out the test pairs. The remaining
// pairs are split into train and validation. The SVM picks C, training
// passes, and its decision cutoff on validation AUC.
// Usage: ./romberg_cv features.txt model_metrics.json

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Example {
  double thresh = 0;
  int pair_id = 0;
  int label = 0;
  std::vector<double> x;
};

struct ZScore {
  std::vector<double> mean, sd;
  std::vector<double> apply(const std::vector<double>& v) const {
    std::vector<double> y(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
      double s = sd[i] < 1e-12 ? 1.0 : sd[i];
      y[i] = (v[i] - mean[i]) / s;
    }
    return y;
  }
};

static std::vector<Example> load_features(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path);
  std::vector<Example> rows;
  std::string line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::stringstream ss(line);
    Example e;
    ss >> e.thresh >> e.pair_id >> e.label;
    double v;
    while (ss >> v) e.x.push_back(v);
    if (e.x.size() == 3) rows.push_back(e);
  }
  return rows;
}

static double auc(const std::vector<double>& scores, const std::vector<int>& labels) {
  std::vector<int> pos, neg;
  for (size_t i = 0; i < labels.size(); ++i) (labels[i] == 1 ? pos : neg).push_back(static_cast<int>(i));
  if (pos.empty() || neg.empty()) return 0.5;
  double wins = 0;
  for (int p : pos)
    for (int n : neg) {
      if (scores[p] > scores[n]) wins += 1;
      else if (scores[p] == scores[n]) wins += 0.5;
    }
  return wins / (static_cast<double>(pos.size()) * neg.size());
}

static std::vector<double> fit_lr(const std::vector<std::vector<double>>& X, const std::vector<double>& y) {
  const int n = static_cast<int>(X.size());
  const int d = static_cast<int>(X[0].size()) + 1;
  std::vector<std::vector<double>> A(d, std::vector<double>(d, 0));
  std::vector<double> b(d, 0);
  for (int i = 0; i < n; ++i) {
    std::vector<double> row = X[i];
    row.push_back(1.0);
    for (int r = 0; r < d; ++r) {
      b[r] += row[r] * y[i];
      for (int c = 0; c < d; ++c) A[r][c] += row[r] * row[c];
    }
  }
  for (int i = 0; i < d; ++i) A[i][i] += 1e-6;
  for (int col = 0; col < d; ++col) {
    int piv = col;
    for (int r = col + 1; r < d; ++r)
      if (std::fabs(A[r][col]) > std::fabs(A[piv][col])) piv = r;
    std::swap(A[col], A[piv]);
    std::swap(b[col], b[piv]);
    double div = A[col][col];
    if (std::fabs(div) < 1e-12) continue;
    for (int c = col; c < d; ++c) A[col][c] /= div;
    b[col] /= div;
    for (int r = 0; r < d; ++r) {
      if (r == col) continue;
      double f = A[r][col];
      for (int c = col; c < d; ++c) A[r][c] -= f * A[col][c];
      b[r] -= f * b[col];
    }
  }
  return b;
}

static double predict_lr(const std::vector<double>& w, const std::vector<double>& x) {
  double s = w.back();
  for (size_t i = 0; i < x.size(); ++i) s += w[i] * x[i];
  return s;
}

struct LinearSVM {
  std::vector<double> w;
  double b = 0;
};

static LinearSVM fit_svm(const std::vector<std::vector<double>>& X, const std::vector<int>& y, std::mt19937& rng,
                        double C, int epochs) {
  const int d = static_cast<int>(X[0].size());
  LinearSVM m;
  m.w.assign(d, 0);
  std::vector<int> order(X.size());
  std::iota(order.begin(), order.end(), 0);
  for (int ep = 0; ep < epochs; ++ep) {
    std::shuffle(order.begin(), order.end(), rng);
    double step = 0.05 / (1.0 + 0.001 * ep);
    for (int i : order) {
      double yi = y[i] == 1 ? 1.0 : -1.0;
      double margin = m.b;
      for (int j = 0; j < d; ++j) margin += m.w[j] * X[i][j];
      for (int j = 0; j < d; ++j) m.w[j] *= (1.0 - step * 0.001);
      if (yi * margin < 1.0) {
        for (int j = 0; j < d; ++j) m.w[j] += step * C * yi * X[i][j];
        m.b += step * C * yi;
      }
    }
  }
  return m;
}

static double score_svm(const LinearSVM& m, const std::vector<double>& x) {
  double s = m.b;
  for (size_t i = 0; i < x.size(); ++i) s += m.w[i] * x[i];
  return s;
}

static double sigmoid(double z) {
  if (z > 30) return 1;
  if (z < -30) return 0;
  return 1.0 / (1.0 + std::exp(-z));
}

struct MLP {
  int h = 8;
  std::vector<std::vector<double>> W1;
  std::vector<double> b1, W2;
  double b2 = 0;
};

static MLP fit_mlp(const std::vector<std::vector<double>>& X, const std::vector<int>& y, std::mt19937& rng) {
  const int d = static_cast<int>(X[0].size());
  const int hidden = 8;
  MLP m;
  m.h = hidden;
  std::normal_distribution<double> nd(0, 0.2);
  m.W1.assign(hidden, std::vector<double>(d));
  m.b1.assign(hidden, 0);
  m.W2.assign(hidden, 0);
  for (int i = 0; i < hidden; ++i) {
    for (int j = 0; j < d; ++j) m.W1[i][j] = nd(rng);
    m.W2[i] = nd(rng);
  }
  std::vector<int> order(X.size());
  std::iota(order.begin(), order.end(), 0);
  const double lr = 0.05;
  for (int ep = 0; ep < 120; ++ep) {
    std::shuffle(order.begin(), order.end(), rng);
    for (int i : order) {
      std::vector<double> h(hidden);
      for (int k = 0; k < hidden; ++k) {
        double z = m.b1[k];
        for (int j = 0; j < d; ++j) z += m.W1[k][j] * X[i][j];
        h[k] = std::tanh(z);
      }
      double z2 = m.b2;
      for (int k = 0; k < hidden; ++k) z2 += m.W2[k] * h[k];
      double pred = sigmoid(z2);
      double dL = pred - y[i];
      for (int k = 0; k < hidden; ++k) {
        double dz = dL * m.W2[k] * (1.0 - h[k] * h[k]);
        for (int j = 0; j < d; ++j) m.W1[k][j] -= lr * dz * X[i][j];
        m.b1[k] -= lr * dz;
        m.W2[k] -= lr * dL * h[k];
      }
      m.b2 -= lr * dL;
    }
  }
  return m;
}

static double score_mlp(const MLP& m, const std::vector<double>& x) {
  double z2 = m.b2;
  for (int k = 0; k < m.h; ++k) {
    double z = m.b1[k];
    for (size_t j = 0; j < x.size(); ++j) z += m.W1[k][j] * x[j];
    z2 += m.W2[k] * std::tanh(z);
  }
  return sigmoid(z2);
}

struct Metrics {
  double accuracy = 0;
  double auc = 0.5;
};

static Metrics evaluate(const std::string& which, const std::vector<Example>& train,
                        const std::vector<Example>& test, std::mt19937& rng) {
  Metrics metrics;
  if (train.empty() || test.empty()) return metrics;
  std::vector<std::vector<double>> Xtr, Xte;
  std::vector<double> ytr_d;
  std::vector<int> ytr, yte;
  for (const auto& e : train) {
    Xtr.push_back(e.x);
    ytr.push_back(e.label);
    ytr_d.push_back(e.label);
  }
  for (const auto& e : test) {
    Xte.push_back(e.x);
    yte.push_back(e.label);
  }
  std::vector<double> scores;
  if (which == "lr") {
    auto w = fit_lr(Xtr, ytr_d);
    for (const auto& x : Xte) scores.push_back(predict_lr(w, x));
  } else if (which == "svm") {
    auto model = fit_svm(Xtr, ytr, rng, 1.0, 60);
    for (const auto& x : Xte) scores.push_back(score_svm(model, x));
  } else {
    auto model = fit_mlp(Xtr, ytr, rng);
    for (const auto& x : Xte) scores.push_back(score_mlp(model, x));
  }
  int correct = 0;
  for (size_t i = 0; i < scores.size(); ++i) {
    int pred = which == "svm" ? (scores[i] >= 0 ? 1 : 0) : (scores[i] >= 0.5 ? 1 : 0);
    if (pred == yte[i]) correct++;
  }
  metrics.accuracy = static_cast<double>(correct) / scores.size();
  metrics.auc = auc(scores, yte);
  return metrics;
}

static ZScore fit_z(const std::vector<Example>& rows) {
  ZScore z;
  if (rows.empty()) return z;
  size_t d = rows[0].x.size();
  z.mean.assign(d, 0);
  z.sd.assign(d, 0);
  for (const auto& e : rows)
    for (size_t j = 0; j < d; ++j) z.mean[j] += e.x[j];
  for (double& m : z.mean) m /= rows.size();
  for (const auto& e : rows)
    for (size_t j = 0; j < d; ++j) {
      double dlt = e.x[j] - z.mean[j];
      z.sd[j] += dlt * dlt;
    }
  for (double& s : z.sd) s = std::sqrt(s / std::max<size_t>(1, rows.size() - 1));
  return z;
}

static std::vector<Example> subset(const std::vector<Example>& all, const std::vector<int>& pairs, const ZScore& z) {
  std::vector<Example> out;
  for (const auto& e : all) {
    if (std::find(pairs.begin(), pairs.end(), e.pair_id) == pairs.end()) continue;
    Example copy = e;
    copy.x = z.apply(e.x);
    out.push_back(copy);
  }
  return out;
}

static double mean(const std::vector<double>& v) {
  return v.empty() ? 0 : std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

static std::vector<Example> for_thresh(const std::vector<Example>& all, double thresh) {
  std::vector<Example> out;
  for (const auto& e : all)
    if (std::fabs(e.thresh - thresh) < 1e-6) out.push_back(e);
  return out;
}

static double accuracy_at(const std::vector<double>& scores, const std::vector<int>& labels, double cutoff) {
  if (scores.empty()) return 0;
  int correct = 0;
  for (size_t i = 0; i < scores.size(); ++i) {
    int pred = scores[i] >= cutoff ? 1 : 0;
    if (pred == labels[i]) correct++;
  }
  return static_cast<double>(correct) / scores.size();
}

static double best_cutoff(const std::vector<double>& scores, const std::vector<int>& labels) {
  if (scores.empty()) return 0;
  std::vector<double> cuts = scores;
  std::sort(cuts.begin(), cuts.end());
  cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
  std::vector<double> candidates;
  candidates.push_back(cuts.front() - 1.0);
  for (double score : cuts) candidates.push_back(score);
  candidates.push_back(cuts.back() + 1.0);
  double best_cut = 0;
  double best_acc = -1;
  for (double cut : candidates) {
    double acc = accuracy_at(scores, labels, cut);
    if (acc > best_acc || (acc == best_acc && std::fabs(cut) < std::fabs(best_cut))) {
      best_acc = acc;
      best_cut = cut;
    }
  }
  return best_cut;
}

struct FoldScore {
  double thresh = 0;
  double test_accuracy = 0;
  double val_accuracy = 0;
  double test_auc = 0;
  double val_auc = 0;
};

static double mode_thresh(const std::vector<double>& values) {
  double best = values.empty() ? 0.02 : values[0];
  int best_count = 0;
  for (double candidate : values) {
    int count = 0;
    for (double value : values)
      if (std::fabs(candidate - value) < 1e-8) count++;
    if (count > best_count) {
      best = candidate;
      best_count = count;
    }
  }
  return best;
}

int main(int argc, char** argv) {
  std::string in_path = argc > 1 ? argv[1] : "output/features.txt";
  std::string out_path = argc > 2 ? argv[2] : "output/model_metrics.json";
  auto examples = load_features(in_path);
  const std::vector<double> grid = {0.02};
  auto pair_ids = [&](double fraction) {
    std::vector<int> ids;
    for (const auto& e : examples)
      if (e.label == 0 && std::fabs(e.thresh - fraction) < 1e-6) ids.push_back(e.pair_id);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
  };
  std::vector<int> pairs = pair_ids(grid[0]);
  for (size_t i = 1; i < grid.size(); ++i) {
    auto ids = pair_ids(grid[i]);
    std::vector<int> both;
    for (int id : pairs)
      if (std::find(ids.begin(), ids.end(), id) != ids.end()) both.push_back(id);
    pairs.swap(both);
  }
  if (pairs.size() < 16) {
    std::cerr << "Need 16 pairs, got " << pairs.size() << "\n";
    return 1;
  }

  std::mt19937 split_rng(2);
  std::shuffle(pairs.begin(), pairs.end(), split_rng);
  const int outer_k = 4;
  const int test_count = static_cast<int>(pairs.size()) / outer_k;

  auto run_model = [&](const std::string& which) {
    std::vector<FoldScore> folds;
    for (int fold = 0; fold < outer_k; ++fold) {
      int test_begin = fold * test_count;
      int test_end = (fold == outer_k - 1) ? static_cast<int>(pairs.size()) : test_begin + test_count;
      std::vector<int> test_pairs(pairs.begin() + test_begin, pairs.begin() + test_end);
      std::vector<int> rest;
      for (int id : pairs)
        if (std::find(test_pairs.begin(), test_pairs.end(), id) == test_pairs.end()) rest.push_back(id);
      int n_val = std::max(2, static_cast<int>(rest.size()) / 4);
      std::vector<int> val_pairs(rest.end() - n_val, rest.end());
      std::vector<int> train_pairs(rest.begin(), rest.end() - n_val);

      FoldScore best;
      best.val_auc = -1;
      for (double thresh : grid) {
        auto rows = for_thresh(examples, thresh);
        std::vector<Example> train_raw;
        for (const auto& e : rows)
          if (std::find(train_pairs.begin(), train_pairs.end(), e.pair_id) != train_pairs.end())
            train_raw.push_back(e);
        auto z = fit_z(train_raw);
        auto train = subset(rows, train_pairs, z);
        auto val = subset(rows, val_pairs, z);
        auto test = subset(rows, test_pairs, z);
        std::mt19937 rng(2);
        auto val_metrics = evaluate(which, train, val, rng);
        if (val_metrics.auc > best.val_auc) {
          std::mt19937 test_rng(2);
          auto test_metrics = evaluate(which, train, test, test_rng);
          best.thresh = thresh;
          best.val_accuracy = val_metrics.accuracy;
          best.val_auc = val_metrics.auc;
          best.test_accuracy = test_metrics.accuracy;
          best.test_auc = test_metrics.auc;
        }
      }
      folds.push_back(best);
      std::cout << which << " fold " << fold << " thresh " << best.thresh
                << " val acc " << best.val_accuracy << " test acc " << best.test_accuracy << "\n";
    }
    return folds;
  };

  auto run_svm = [&]() {
    const double penalties[] = {0.01, 0.1, 1.0, 10.0};
    const int passes[] = {40, 80, 160};
    std::vector<FoldScore> folds;
    for (int fold = 0; fold < outer_k; ++fold) {
      int test_begin = fold * test_count;
      int test_end = (fold == outer_k - 1) ? static_cast<int>(pairs.size()) : test_begin + test_count;
      std::vector<int> test_pairs(pairs.begin() + test_begin, pairs.begin() + test_end);
      std::vector<int> rest;
      for (int id : pairs)
        if (std::find(test_pairs.begin(), test_pairs.end(), id) == test_pairs.end()) rest.push_back(id);
      int n_val = std::max(2, static_cast<int>(rest.size()) / 4);
      std::vector<int> val_pairs(rest.end() - n_val, rest.end());
      std::vector<int> train_pairs(rest.begin(), rest.end() - n_val);

      FoldScore best;
      best.val_auc = -1;
      double best_c = 1;
      int best_passes = 60;
      for (double fraction : grid) {
        auto rows = for_thresh(examples, fraction);
        std::vector<Example> train_raw;
        for (const auto& e : rows)
          if (std::find(train_pairs.begin(), train_pairs.end(), e.pair_id) != train_pairs.end())
            train_raw.push_back(e);
        auto z = fit_z(train_raw);
        auto train = subset(rows, train_pairs, z);
        auto val = subset(rows, val_pairs, z);
        auto test = subset(rows, test_pairs, z);
        if (train.empty() || val.empty() || test.empty()) continue;
        std::vector<std::vector<double>> Xtr;
        std::vector<int> ytr, yval, ytest;
        for (const auto& e : train) {
          Xtr.push_back(e.x);
          ytr.push_back(e.label);
        }
        for (const auto& e : val) yval.push_back(e.label);
        for (const auto& e : test) ytest.push_back(e.label);
        for (double C : penalties) {
          for (int epochs : passes) {
            std::mt19937 rng(2);
            auto model = fit_svm(Xtr, ytr, rng, C, epochs);
            std::vector<double> val_scores, test_scores;
            for (const auto& e : val) val_scores.push_back(score_svm(model, e.x));
            for (const auto& e : test) test_scores.push_back(score_svm(model, e.x));
            double val_auc = auc(val_scores, yval);
            if (val_auc > best.val_auc) {
              double cutoff = best_cutoff(val_scores, yval);
              best.thresh = fraction;
              best.val_auc = val_auc;
              best.val_accuracy = accuracy_at(val_scores, yval, cutoff);
              best.test_accuracy = accuracy_at(test_scores, ytest, cutoff);
              best.test_auc = auc(test_scores, ytest);
              best_c = C;
              best_passes = epochs;
            }
          }
        }
      }
      folds.push_back(best);
      std::cout << "svm fold " << fold << " thresh " << best.thresh << " C " << best_c
                << " passes " << best_passes << " val acc " << best.val_accuracy
                << " test acc " << best.test_accuracy << "\n";
    }
    return folds;
  };

  auto lr = run_model("lr");
  auto svm = run_svm();
  auto nn = run_model("nn");

  auto acc = [](const std::vector<FoldScore>& folds, auto member) {
    std::vector<double> values;
    for (const auto& fold : folds) values.push_back(member(fold));
    return mean(values);
  };
  auto thresh_of = [](const std::vector<FoldScore>& folds) {
    std::vector<double> values;
    for (const auto& fold : folds) values.push_back(fold.thresh);
    return mode_thresh(values);
  };

  std::ofstream out(out_path);
  out << "{\n";
  out << "  \"task\": \"eyes open vs eyes closed\",\n";
  out << "  \"protocol\": \"Nested cross-validation on within-person gravity-tilt changes (mean, std, RMS). Eyes open is the open second half minus the open first half. Eyes closed is the full closed recording minus the full open recording. The SVM chooses its penalty, training length, and decision cutoff on validation, then the test pairs are scored once.\",\n";
  out << "  \"nPairs\": " << pairs.size() << ",\n";
  out << "  \"models\": [\n";
  auto emit = [&](const std::string& id, const std::string& name, const std::vector<FoldScore>& folds, bool last) {
    out << "    {\"id\": \"" << id << "\", \"name\": \"" << name
        << "\", \"plThresh\": " << thresh_of(folds)
        << ", \"accuracy\": " << acc(folds, [](const FoldScore& f) { return f.test_accuracy; })
        << ", \"validationAccuracy\": " << acc(folds, [](const FoldScore& f) { return f.val_accuracy; })
        << ", \"auc\": " << acc(folds, [](const FoldScore& f) { return f.test_auc; })
        << ", \"validationAuc\": " << acc(folds, [](const FoldScore& f) { return f.val_auc; })
        << "}" << (last ? "\n" : ",\n");
  };
  emit("linear_regression", "Linear Regression", lr, false);
  emit("svm", "Support Vector Machine", svm, false);
  emit("neural_network", "Neural Network", nn, true);
  out << "  ]\n}\n";
  std::cout << "Wrote " << out_path << "\n";
  return 0;
}
