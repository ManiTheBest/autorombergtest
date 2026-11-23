// Romberg rec engine: features + nested CV for LR, linear SVM, and a small MLP.
// No Python. Compile: clang++ -std=c++17 -O2 -o romberg_train src/romberg_train.cpp

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Recording {
  int sub_id;
  int klass;  // 0 eyes open, 1 eyes closed
  std::string folder;
};

struct WindowFeats {
  std::vector<double> tt;
  std::vector<double> pl;
};

struct HalfStats {
  double med = 0, mean = 0, std = 0, skew = 0, kurt = 0;
  bool ok = false;
};

struct Example {
  int pair_id;
  int label;  // 0 EO self-diff, 1 EC half2 - EO half1
  std::vector<double> x;  // 5 features
};

static std::vector<Recording> dataset() {
  return {
      {2, 0, "2025-05-09_15-40-13"}, {2, 1, "2025-05-09_15-42-40"},
      {3, 0, "2025-05-09_16-08-59"}, {3, 1, "2025-05-09_16-09-43"},
      {4, 0, "2025-05-09_16-57-02"}, {4, 1, "2025-05-09_16-57-39"},
      {5, 0, "2025-05-09_18-00-12"}, {5, 1, "2025-05-09_18-00-57"},
      {11, 0, "2025-05-10_20-41-36"}, {11, 1, "2025-05-10_20-42-17"},
      {12, 0, "2025-05-10_21-15-51"}, {12, 1, "2025-05-10_21-16-27"},
      {14, 1, "2025-05-10_23-47-08"}, {14, 0, "2025-05-10_23-48-01"},
      {15, 1, "2025-05-10_23-49-01"}, {15, 0, "2025-05-10_23-49-51"},
      {16, 1, "2025-05-10_23-53-26"}, {16, 0, "2025-05-10_23-54-19"},
      {17, 1, "2025-05-11_23-23-19"}, {17, 0, "2025-05-11_23-24-14"},
      {21, 1, "2025-05-12_00-00-08"}, {21, 0, "2025-05-12_00-01-01"},
      {22, 1, "2025-05-12_00-02-13"}, {22, 0, "2025-05-12_00-02-51"},
      {23, 1, "2025-05-12_16-14-36"}, {23, 0, "2025-05-12_16-15-21"},
      {24, 1, "2025-05-12_16-16-49"}, {24, 0, "2025-05-12_16-17-34"},
      {25, 1, "2025-05-12_17-42-53"}, {25, 0, "2025-05-12_17-43-52"},
      {26, 1, "2025-05-12_01-23-27"}, {26, 0, "2025-05-12_01-24-12"},
  };
}

static std::vector<std::vector<double>> load_xyz(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("cannot open " + path);
  std::string line;
  std::getline(in, line);
  std::vector<std::string> headers;
  {
    std::stringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) headers.push_back(cell);
  }
  auto idx = [&](const std::string& name) {
    for (size_t i = 0; i < headers.size(); ++i)
      if (headers[i] == name) return static_cast<int>(i);
    return -1;
  };
  int iz = idx("z"), iy = idx("y"), ix = idx("x");
  if (iz < 0 || iy < 0 || ix < 0) throw std::runtime_error("missing xyz in " + path);
  std::vector<std::vector<double>> rows;
  while (std::getline(in, line)) {
    if (line.empty()) continue;
    std::stringstream ss(line);
    std::string cell;
    std::vector<std::string> cols;
    while (std::getline(ss, cell, ',')) cols.push_back(cell);
    int need = std::max({iz, iy, ix});
    if (static_cast<int>(cols.size()) <= need) continue;
    rows.push_back({std::stod(cols[iz]), std::stod(cols[iy]), std::stod(cols[ix])});
  }
  return rows;
}

static WindowFeats frame_features(const std::vector<std::vector<double>>& xx, double fs) {
  const int n = static_cast<int>(xx.size());
  const double n_sec_window = 3.0;
  const double n_sec_inc = 1.0;
  const int n_per = static_cast<int>(std::floor(n_sec_window * fs));
  const int n_inc = static_cast<int>(std::floor(n_sec_inc * fs));
  const int n_frames = static_cast<int>(std::floor(static_cast<double>(n) / n_inc)) - static_cast<int>(n_sec_window) + 1;
  WindowFeats out;
  int start = 0;
  for (int f = 0; f < n_frames; ++f) {
    if (start + n_per > n) break;
    double mx = 0, my = 0, mz = 0;
    for (int i = 0; i < n_per; ++i) {
      mx += xx[start + i][0];
      my += xx[start + i][1];
      mz += xx[start + i][2];
    }
    mx /= n_per;
    my /= n_per;
    mz /= n_per;
    double sum = 0;
    int cnt = 0;
    for (int i = 0; i < n_per - 1; ++i) {
      double dx = (xx[start + i + 1][0] - mx) - (xx[start + i][0] - mx);
      double dy = (xx[start + i + 1][1] - my) - (xx[start + i][1] - my);
      double dz = (xx[start + i + 1][2] - mz) - (xx[start + i][2] - mz);
      sum += std::sqrt(dx * dx + dy * dy + dz * dz);
      ++cnt;
    }
    out.pl.push_back(cnt ? sum / cnt : 0);
    out.tt.push_back(static_cast<double>(start + n_per) / fs);
    start += n_inc;
  }
  return out;
}

static double median_of(std::vector<double> v) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  size_t n = v.size();
  if (n % 2) return v[n / 2];
  return 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

static double mean_of(const std::vector<double>& v) {
  if (v.empty()) return 0;
  return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

static double std_of(const std::vector<double>& v) {
  if (v.size() < 2) return 0;
  double m = mean_of(v);
  double s = 0;
  for (double x : v) s += (x - m) * (x - m);
  return std::sqrt(s / (v.size() - 1));
}

static double skew_of(const std::vector<double>& v) {
  if (v.size() < 3) return 0;
  double m = mean_of(v);
  double sd = std_of(v);
  if (sd < 1e-12) return 0;
  double s = 0;
  for (double x : v) {
    double z = (x - m) / sd;
    s += z * z * z;
  }
  return s / v.size();
}

static double kurt_of(const std::vector<double>& v) {
  if (v.size() < 4) return 0;
  double m = mean_of(v);
  double sd = std_of(v);
  if (sd < 1e-12) return 0;
  double s = 0;
  for (double x : v) {
    double z = (x - m) / sd;
    s += z * z * z * z;
  }
  return s / v.size();
}

static HalfStats stats_of(const std::vector<double>& pl, int a, int b) {
  HalfStats h;
  if (b <= a) return h;
  std::vector<double> slice(pl.begin() + a, pl.begin() + b);
  if (slice.size() < 2) return h;
  h.med = median_of(slice);
  h.mean = mean_of(slice);
  h.std = std_of(slice);
  h.skew = skew_of(slice);
  h.kurt = kurt_of(slice);
  h.ok = true;
  return h;
}

static std::vector<double> diff_stats(const HalfStats& a, const HalfStats& b) {
  return {a.med - b.med, a.mean - b.mean, a.std - b.std, a.skew - b.skew, a.kurt - b.kurt};
}

struct PairFeats {
  int pair_id;
  HalfStats eo1, eo2, ec1, ec2;
  bool ok = false;
};

static std::map<int, PairFeats> build_pairs(
    const std::string& data_root, double pl_thresh) {
  std::map<int, PairFeats> pairs;
  for (const auto& rec : dataset()) {
    auto xx = load_xyz(data_root + "/" + rec.folder + "/AccelerometerUncalibrated.csv");
    auto wf = frame_features(xx, 100.0);
    std::vector<double> kept;
    for (double p : wf.pl)
      if (p < pl_thresh) kept.push_back(p);
    int n = static_cast<int>(kept.size());
    int mid = n / 2;
    HalfStats h1 = stats_of(kept, 0, mid);
    HalfStats h2 = stats_of(kept, mid, n);
    auto& pf = pairs[rec.sub_id];
    pf.pair_id = rec.sub_id;
    if (rec.klass == 0) {
      pf.eo1 = h1;
      pf.eo2 = h2;
    } else {
      pf.ec1 = h1;
      pf.ec2 = h2;
    }
  }
  for (auto& kv : pairs) {
    kv.second.ok = kv.second.eo1.ok && kv.second.eo2.ok && kv.second.ec2.ok;
  }
  return pairs;
}

static std::vector<Example> make_examples(const std::map<int, PairFeats>& pairs) {
  std::vector<Example> ex;
  for (const auto& kv : pairs) {
    if (!kv.second.ok) continue;
    Example e0;
    e0.pair_id = kv.first;
    e0.label = 0;
    e0.x = diff_stats(kv.second.eo2, kv.second.eo1);
    Example e1;
    e1.pair_id = kv.first;
    e1.label = 1;
    e1.x = diff_stats(kv.second.ec2, kv.second.eo1);
    ex.push_back(e0);
    ex.push_back(e1);
  }
  return ex;
}

struct ZScore {
  std::vector<double> mean, sd;
  std::vector<double> apply(const std::vector<double>& x) const {
    std::vector<double> y(x.size());
    for (size_t i = 0; i < x.size(); ++i) {
      double s = sd[i] < 1e-12 ? 1.0 : sd[i];
      y[i] = (x[i] - mean[i]) / s;
    }
    return y;
  }
};

static ZScore fit_z(const std::vector<Example>& ex, const std::vector<int>& idx) {
  ZScore z;
  if (idx.empty()) return z;
  size_t d = ex[idx[0]].x.size();
  z.mean.assign(d, 0);
  z.sd.assign(d, 0);
  for (int i : idx)
    for (size_t j = 0; j < d; ++j) z.mean[j] += ex[i].x[j];
  for (double& m : z.mean) m /= idx.size();
  for (int i : idx)
    for (size_t j = 0; j < d; ++j) {
      double dlt = ex[i].x[j] - z.mean[j];
      z.sd[j] += dlt * dlt;
    }
  for (double& s : z.sd) s = std::sqrt(s / std::max<size_t>(1, idx.size() - 1));
  return z;
}

// AUC via rank (Mann-Whitney)
static double auc(const std::vector<double>& scores, const std::vector<int>& labels) {
  std::vector<int> pos, neg;
  for (size_t i = 0; i < labels.size(); ++i) {
    if (labels[i] == 1) pos.push_back(static_cast<int>(i));
    else neg.push_back(static_cast<int>(i));
  }
  if (pos.empty() || neg.empty()) return 0.5;
  double wins = 0;
  for (int p : pos)
    for (int n : neg) {
      if (scores[p] > scores[n]) wins += 1;
      else if (scores[p] == scores[n]) wins += 0.5;
    }
  return wins / (static_cast<double>(pos.size()) * neg.size());
}

// Linear regression via normal equations on [x | 1]
static std::vector<double> fit_lr(const std::vector<std::vector<double>>& X,
                                   const std::vector<double>& y) {
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
  for (int i = 0; i < d; ++i) A[i][i] += 1e-6;  // ridge for stability
  // Gaussian elimination
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

// Linear SVM via subgradient on hinge, scores = w·x
struct LinearSVM {
  std::vector<double> w;
  double b = 0;
};

static LinearSVM fit_svm(const std::vector<std::vector<double>>& X, const std::vector<int>& y,
                         double C, int epochs, std::mt19937& rng) {
  const int d = static_cast<int>(X[0].size());
  LinearSVM m;
  m.w.assign(d, 0);
  std::vector<int> order(X.size());
  std::iota(order.begin(), order.end(), 0);
  double lr = 0.05;
  for (int ep = 0; ep < epochs; ++ep) {
    std::shuffle(order.begin(), order.end(), rng);
    for (int i : order) {
      double yi = y[i] == 1 ? 1.0 : -1.0;
      double margin = m.b;
      for (int j = 0; j < d; ++j) margin += m.w[j] * X[i][j];
      double step = lr / (1.0 + 0.001 * ep);
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

// Tiny 1-hidden-layer MLP, sigmoid output score in (0,1)
struct MLP {
  int h = 8;
  std::vector<std::vector<double>> W1;  // h x d
  std::vector<double> b1;
  std::vector<double> W2;  // h
  double b2 = 0;
};

static double sigmoid(double z) {
  if (z > 30) return 1;
  if (z < -30) return 0;
  return 1.0 / (1.0 + std::exp(-z));
}

static MLP fit_mlp(const std::vector<std::vector<double>>& X, const std::vector<int>& y, int hidden,
                   int epochs, std::mt19937& rng) {
  const int d = static_cast<int>(X[0].size());
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
  double lr = 0.05;
  std::vector<int> order(X.size());
  std::iota(order.begin(), order.end(), 0);
  for (int ep = 0; ep < epochs; ++ep) {
    std::shuffle(order.begin(), order.end(), rng);
    for (int i : order) {
      std::vector<double> h(hidden), dh(hidden);
      for (int k = 0; k < hidden; ++k) {
        double z = m.b1[k];
        for (int j = 0; j < d; ++j) z += m.W1[k][j] * X[i][j];
        h[k] = std::tanh(z);
      }
      double z2 = m.b2;
      for (int k = 0; k < hidden; ++k) z2 += m.W2[k] * h[k];
      double pred = sigmoid(z2);
      double t = static_cast<double>(y[i]);
      double dL = pred - t;
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

struct FoldResult {
  double lr = 0.5, svm = 0.5, nn = 0.5;
  double thresh = 0.02;
};

static std::vector<int> pair_ids_of(const std::vector<Example>& ex) {
  std::vector<int> ids;
  for (const auto& e : ex)
    if (e.label == 0) ids.push_back(e.pair_id);
  return ids;
}

static std::vector<int> indices_for_pairs(const std::vector<Example>& ex, const std::vector<int>& pairs) {
  std::vector<int> idx;
  for (size_t i = 0; i < ex.size(); ++i)
    if (std::find(pairs.begin(), pairs.end(), ex[i].pair_id) != pairs.end()) idx.push_back(static_cast<int>(i));
  return idx;
}

static double eval_model(const std::string& which, const std::vector<Example>& train,
                         const std::vector<Example>& test, std::mt19937& rng) {
  if (train.empty() || test.empty()) return 0.5;
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
    auto m = fit_svm(Xtr, ytr, 1.0, 40, rng);
    for (const auto& x : Xte) scores.push_back(score_svm(m, x));
  } else {
    auto m = fit_mlp(Xtr, ytr, 8, 80, rng);
    for (const auto& x : Xte) scores.push_back(score_mlp(m, x));
  }
  return auc(scores, yte);
}

static std::vector<Example> materialize(const std::vector<Example>& raw, const std::vector<int>& idx,
                                        const ZScore& z) {
  std::vector<Example> out;
  for (int i : idx) {
    Example e = raw[i];
    e.x = z.apply(raw[i].x);
    out.push_back(e);
  }
  return out;
}

int main(int argc, char** argv) {
  std::string data_root = argc > 1 ? argv[1] : "../Data";
  std::string out_path = argc > 2 ? argv[2] : "output/model_metrics.json";
  const std::vector<double> thresh_grid = {0.018, 0.019, 0.020, 0.021, 0.022};
  const int outer_k = 4;
  std::mt19937 rng(2);

  // Cache features per threshold (CSV load is the expensive part)
  std::map<int, std::map<int, PairFeats>> cache;  // thresh*1000 -> pairs
  std::cout << "Loading features...\n";
  for (double th : thresh_grid) {
    int key = static_cast<int>(std::round(th * 1000));
    cache[key] = build_pairs(data_root, th);
    std::cout << "  thresh " << th << " pairs " << cache[key].size() << "\n";
  }

  // Pair ids from first thresh
  auto base_ex = make_examples(cache[200]);
  auto pids = pair_ids_of(base_ex);
  std::sort(pids.begin(), pids.end());
  std::shuffle(pids.begin(), pids.end(), rng);

  std::vector<double> outer_lr, outer_svm, outer_nn;
  std::vector<double> chosen_th;

  int fold_size = std::max(1, static_cast<int>(pids.size()) / outer_k);
  for (int fold = 0; fold < outer_k; ++fold) {
    int a = fold * fold_size;
    int b = (fold == outer_k - 1) ? static_cast<int>(pids.size()) : std::min(static_cast<int>(pids.size()), a + fold_size);
    if (a >= static_cast<int>(pids.size())) break;
    std::vector<int> test_pairs(pids.begin() + a, pids.begin() + b);
    std::vector<int> train_pairs;
    for (int id : pids)
      if (std::find(test_pairs.begin(), test_pairs.end(), id) == test_pairs.end()) train_pairs.push_back(id);

    // Inner: pick thresh + we score each model on a held-out slice of train pairs
    int n_val = std::max(2, static_cast<int>(train_pairs.size()) / 4);
    std::vector<int> val_pairs(train_pairs.end() - n_val, train_pairs.end());
    std::vector<int> inner_train(train_pairs.begin(), train_pairs.end() - n_val);

    double best_lr = -1, best_svm = -1, best_nn = -1;
    double th_lr = 0.02, th_svm = 0.02, th_nn = 0.02;

    for (double th : thresh_grid) {
      int key = static_cast<int>(std::round(th * 1000));
      auto ex = make_examples(cache[key]);
      auto itr = indices_for_pairs(ex, inner_train);
      auto iva = indices_for_pairs(ex, val_pairs);
      auto z = fit_z(ex, itr);
      auto tr = materialize(ex, itr, z);
      auto va = materialize(ex, iva, z);
      double a_lr = eval_model("lr", tr, va, rng);
      double a_svm = eval_model("svm", tr, va, rng);
      double a_nn = eval_model("nn", tr, va, rng);
      if (a_lr > best_lr) {
        best_lr = a_lr;
        th_lr = th;
      }
      if (a_svm > best_svm) {
        best_svm = a_svm;
        th_svm = th;
      }
      if (a_nn > best_nn) {
        best_nn = a_nn;
        th_nn = th;
      }
    }

    auto score_outer = [&](const std::string& which, double th) {
      int key = static_cast<int>(std::round(th * 1000));
      auto ex = make_examples(cache[key]);
      auto itr = indices_for_pairs(ex, train_pairs);
      auto ite = indices_for_pairs(ex, test_pairs);
      auto z = fit_z(ex, itr);
      return eval_model(which, materialize(ex, itr, z), materialize(ex, ite, z), rng);
    };

    double t_lr = score_outer("lr", th_lr);
    double t_svm = score_outer("svm", th_svm);
    double t_nn = score_outer("nn", th_nn);
    outer_lr.push_back(t_lr);
    outer_svm.push_back(t_svm);
    outer_nn.push_back(t_nn);
    chosen_th.push_back(th_lr);
    std::cout << "fold " << fold << " test AUC  lr=" << t_lr << " svm=" << t_svm << " nn=" << t_nn << "\n";
  }

  auto mean = [](const std::vector<double>& v) {
    return v.empty() ? 0.0 : std::accumulate(v.begin(), v.end(), 0.0) / v.size();
  };
  auto stdev = [&](const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double m = mean(v);
    double s = 0;
    for (double x : v) s += (x - m) * (x - m);
    return std::sqrt(s / (v.size() - 1));
  };

  std::ofstream out(out_path);
  out << "{\n";
  out << "  \"metric\": \"auc\",\n";
  out << "  \"protocol\": \"nested pair-level 4-fold CV; pl_thresh tuned on inner validation\",\n";
  out << "  \"nPairs\": " << pids.size() << ",\n";
  out << "  \"models\": [\n";
  auto emit = [&](const std::string& name, const std::string& label, const std::vector<double>& v, bool last) {
    double m = mean(v);
    out << "    {\"id\": \"" << name << "\", \"name\": \"" << label << "\", \"auc\": " << m
        << ", \"accuracy\": " << m << ", \"aucStd\": " << stdev(v) << ", \"folds\": [";
    for (size_t i = 0; i < v.size(); ++i) {
      if (i) out << ", ";
      out << v[i];
    }
    out << "]}" << (last ? "\n" : ",\n");
  };
  emit("linear_regression", "Linear Regression", outer_lr, false);
  emit("svm", "Support Vector Machine", outer_svm, false);
  emit("neural_network", "Neural Network", outer_nn, true);
  out << "  ]\n}\n";
  std::cout << "Wrote " << out_path << "\n";
  std::cout << "Mean AUC  LR " << mean(outer_lr) << "  SVM " << mean(outer_svm) << "  NN " << mean(outer_nn)
            << "\n";
  return 0;
}
