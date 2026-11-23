import fs from "fs";
import path from "path";

type ModelScore = {
  id: string;
  name: string;
  plThresh: number;
  accuracy: number;
  validationAccuracy: number;
  auc: number;
  validationAuc: number;
};

type Report = {
  task: string;
  protocol: string;
  nPairs: number;
  models: ModelScore[];
};

function percent(value: number) {
  return `${Math.round(value * 100)}%`;
}

export function ModelAccuracy() {
  const file = path.join(process.cwd(), "public/ml/model_metrics.json");
  if (!fs.existsSync(file)) {
    return (
      <section id="models" className="mx-auto w-full max-w-6xl px-5 py-16 sm:px-8">
        <h2 className="font-display text-3xl tracking-tight text-ink">Eyes open vs eyes closed</h2>
        <p className="mt-3 text-muted">Model accuracies have not been computed yet.</p>
      </section>
    );
  }

  const report = JSON.parse(fs.readFileSync(file, "utf8")) as Report;

  return (
    <section id="models" className="mx-auto w-full max-w-6xl px-5 py-16 sm:px-8 sm:py-20">
      <div className="mb-10 max-w-2xl">
        <p className="font-mono text-[11px] uppercase tracking-[0.22em] text-muted">
          10 train / 3 validation / 3 test
        </p>
        <h2 className="mt-2 font-display text-3xl tracking-tight text-ink sm:text-4xl">
          Eyes open vs eyes closed
        </h2>
        <p className="mt-3 text-muted">
          Spring builds path-length features for thresholds from 0.018 to 0.022. The C++ engine
          trains on 10 subject pairs, picks each model&apos;s threshold by validation AUC, and the
          number below is accuracy on the 3 held-out test pairs.
        </p>
      </div>
      <div className="grid gap-px overflow-hidden border border-line bg-line sm:grid-cols-3">
        {report.models.map((model) => (
          <article key={model.id} className="bg-panel px-5 py-6">
            <p className="font-mono text-[11px] uppercase tracking-[0.18em] text-muted">{model.name}</p>
            <p className="mt-3 font-display text-5xl tracking-tight text-ink">{percent(model.accuracy)}</p>
            <p className="mt-2 text-sm text-muted">Test accuracy</p>
            <p className="mt-4 text-sm text-foreground">
              Validation {percent(model.validationAccuracy)}
            </p>
            <p className="text-sm text-muted">
              Test AUC {model.auc.toFixed(2)} · threshold {model.plThresh.toFixed(4)}
            </p>
          </article>
        ))}
      </div>
    </section>
  );
}
