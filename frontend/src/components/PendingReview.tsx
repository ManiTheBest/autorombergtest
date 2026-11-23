"use client";

import { useCallback, useEffect, useState, useTransition } from "react";

export type PendingItem = {
  id: string;
  receivedAt: string;
  durationMs: number;
  sampleCount: number;
  originalSampleCount: number;
  cleanedCsv: string;
  preview: string;
};

function formatWhen(iso: string) {
  return new Intl.DateTimeFormat(undefined, {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(new Date(iso));
}

export function PendingReview() {
  const [items, setItems] = useState<PendingItem[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [busyId, setBusyId] = useState<string | null>(null);
  const [message, setMessage] = useState<string | null>(null);
  const [isPending, startTransition] = useTransition();

  const load = useCallback(async () => {
    try {
      const res = await fetch("/api/pending", { cache: "no-store" });
      if (!res.ok) throw new Error(await res.text());
      const data = (await res.json()) as PendingItem[];
      setItems(data);
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to load pending captures");
    }
  }, []);

  useEffect(() => {
    startTransition(() => {
      void load();
    });
    const timer = setInterval(() => {
      void load();
    }, 4000);
    return () => clearInterval(timer);
  }, [load]);

  async function commit(id: string) {
    setBusyId(id);
    setMessage(null);
    try {
      const res = await fetch(`/api/pending/${id}/commit`, { method: "POST" });
      const body = await res.json();
      if (!res.ok) {
        throw new Error(body.error || body.detail || "Commit failed");
      }
      setMessage("Cleaned CSV saved to Neon via Spring.");
      await load();
      // Refresh saved sessions list
      window.location.hash = "sessions";
      window.location.reload();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Commit failed");
    } finally {
      setBusyId(null);
    }
  }

  async function discard(id: string) {
    setBusyId(id);
    setMessage(null);
    try {
      const res = await fetch(`/api/pending/${id}`, { method: "DELETE" });
      if (!res.ok) {
        const body = await res.json().catch(() => ({}));
        throw new Error(body.error || "Discard failed");
      }
      setMessage("Pending capture discarded.");
      await load();
    } catch (err) {
      setError(err instanceof Error ? err.message : "Discard failed");
    } finally {
      setBusyId(null);
    }
  }

  return (
    <section id="pending" className="mx-auto w-full max-w-6xl px-5 py-16 sm:px-8 sm:py-20">
      <div className="mb-10 flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
        <div>
          <p className="font-mono text-[11px] uppercase tracking-[0.22em] text-muted">
            Review queue
          </p>
          <h2 className="mt-2 font-display text-3xl tracking-tight text-ink sm:text-4xl">
            Incoming device captures
          </h2>
          <p className="mt-3 max-w-2xl text-muted">
            Device uploads land here first. Gyro columns are removed automatically — only{" "}
            <span className="font-mono text-foreground">ax, ay, az</span> remain. Choose whether to
            save each cleaned CSV to the database.
          </p>
        </div>
        <p className="font-mono text-sm text-muted">
          {isPending && items.length === 0 ? "Loading…" : `${items.length} pending`}
        </p>
      </div>

      {error ? (
        <div className="mb-6 border border-danger/20 bg-danger-soft px-5 py-4 text-sm text-danger">
          {error}
        </div>
      ) : null}

      {message ? (
        <div className="mb-6 border border-accent/20 bg-accent-soft px-5 py-4 text-sm text-accent-deep">
          {message}
        </div>
      ) : null}

      {items.length === 0 ? (
        <div className="border border-dashed border-line bg-panel/70 px-6 py-12 text-center">
          <p className="font-display text-2xl text-ink">No pending uploads</p>
          <p className="mx-auto mt-3 max-w-lg text-muted">
            When the ESP32 finishes a test it posts to{" "}
            <span className="font-mono text-foreground">/api/ingest</span>. Cleaned captures will
            show up here for approval.
          </p>
        </div>
      ) : (
        <div className="space-y-6">
          {items.map((item) => (
            <article
              key={item.id}
              className="border border-line bg-panel/90 shadow-[0_18px_40px_rgba(12,26,36,0.05)]"
            >
              <div className="flex flex-col gap-4 border-b border-line px-5 py-5 sm:flex-row sm:items-center sm:justify-between">
                <div>
                  <p className="font-mono text-[11px] uppercase tracking-[0.18em] text-muted">
                    Received {formatWhen(item.receivedAt)}
                  </p>
                  <h3 className="mt-1 font-display text-2xl text-ink">
                    Cleaned capture · {item.sampleCount.toLocaleString()} samples
                  </h3>
                  <p className="mt-1 text-sm text-muted">
                    {(item.durationMs / 1000).toFixed(0)}s · original rows{" "}
                    {item.originalSampleCount.toLocaleString()} · columns reduced to ax, ay, az
                  </p>
                </div>
                <div className="flex flex-wrap gap-3">
                  <button
                    type="button"
                    disabled={busyId === item.id}
                    onClick={() => void commit(item.id)}
                    className="cta-primary rounded-md bg-accent px-4 py-2.5 text-sm font-semibold text-white disabled:opacity-60"
                  >
                    {busyId === item.id ? "Saving…" : "Send cleaned CSV to database"}
                  </button>
                  <button
                    type="button"
                    disabled={busyId === item.id}
                    onClick={() => void discard(item.id)}
                    className="cta-secondary rounded-md border border-line bg-white px-4 py-2.5 text-sm font-medium text-foreground disabled:opacity-60"
                  >
                    Discard
                  </button>
                </div>
              </div>
              <pre className="overflow-x-auto bg-ink p-5 font-mono text-[12px] leading-6 text-[#c9e8eb]">
                {item.preview}
                {"\n…"}
              </pre>
            </article>
          ))}
        </div>
      )}
    </section>
  );
}
