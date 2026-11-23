import Link from "next/link";
import { csvDownloadUrl, getTest } from "@/lib/api";

export const dynamic = "force-dynamic";

type PageProps = {
  params: Promise<{ id: string }>;
};

function formatWhen(iso: string) {
  return new Intl.DateTimeFormat(undefined, {
    dateStyle: "medium",
    timeStyle: "medium",
  }).format(new Date(iso));
}

export default async function TestDetailPage({ params }: PageProps) {
  const { id } = await params;

  let error: string | null = null;
  let test: Awaited<ReturnType<typeof getTest>> | null = null;

  try {
    test = await getTest(id);
  } catch (err) {
    error = err instanceof Error ? err.message : "Failed to load test";
  }

  if (error || !test) {
    return (
      <div className="mx-auto w-full max-w-6xl px-5 py-12 sm:px-8">
        <Link
          href="/#sessions"
          className="text-sm font-medium text-accent-deep underline-offset-4 hover:underline"
        >
          ← Back to sessions
        </Link>
        <div className="mt-6 border border-danger/20 bg-danger-soft px-5 py-4 text-sm text-danger">
          {error ?? "Session not found"}
        </div>
      </div>
    );
  }

  const lines = test.csvContent.split("\n").filter((line) => line.trim().length > 0);
  const previewLines = lines.slice(0, 48).join("\n");
  const truncated = lines.length > 48;
  const hz = test.durationMs > 0 ? Math.round(test.sampleCount / (test.durationMs / 1000)) : 0;

  return (
    <div className="mx-auto w-full max-w-6xl px-5 py-10 sm:px-8 sm:py-14">
      <Link
        href="/#sessions"
        className="text-sm font-medium text-accent-deep underline-offset-4 hover:underline"
      >
        ← Back to sessions
      </Link>

      <div className="mt-8 grid gap-10 lg:grid-cols-[1.1fr_0.9fr] lg:items-end">
        <div>
          <p className="font-mono text-[11px] uppercase tracking-[0.22em] text-muted">
            Session record
          </p>
          <h1 className="mt-3 font-display text-4xl tracking-tight text-ink sm:text-5xl">
            Test #{test.id}
          </h1>
          <p className="mt-4 max-w-xl text-lg text-muted">
            Captured {formatWhen(test.createdAt)}. Stored accelerometer CSV
            (ax,&nbsp;ay,&nbsp;az) in Neon.
          </p>
        </div>

        <div className="flex flex-wrap gap-3 lg:justify-end">
          <a
            href={csvDownloadUrl(test.id)}
            className="cta-primary inline-flex items-center rounded-md bg-accent px-5 py-3 text-sm font-semibold text-white"
          >
            Download CSV
          </a>
          <Link
            href="/#sessions"
            className="cta-secondary inline-flex items-center rounded-md border border-line bg-panel px-5 py-3 text-sm font-medium text-foreground"
          >
            All sessions
          </Link>
        </div>
      </div>

      <dl className="mt-12 grid gap-px overflow-hidden border border-line bg-line sm:grid-cols-3">
        <div className="bg-panel px-5 py-6">
          <dt className="font-mono text-[11px] uppercase tracking-[0.18em] text-muted">Duration</dt>
          <dd className="mt-2 font-display text-3xl tracking-tight text-ink">
            {(test.durationMs / 1000).toFixed(0)}
            <span className="ml-1 text-lg text-muted">sec</span>
          </dd>
        </div>
        <div className="bg-panel px-5 py-6">
          <dt className="font-mono text-[11px] uppercase tracking-[0.18em] text-muted">Samples</dt>
          <dd className="mt-2 font-display text-3xl tracking-tight text-ink">
            {test.sampleCount.toLocaleString()}
          </dd>
        </div>
        <div className="bg-panel px-5 py-6">
          <dt className="font-mono text-[11px] uppercase tracking-[0.18em] text-muted">
            Effective rate
          </dt>
          <dd className="mt-2 font-display text-3xl tracking-tight text-ink">
            {hz}
            <span className="ml-1 text-lg text-muted">Hz</span>
          </dd>
        </div>
      </dl>

      <section className="mt-12">
        <div className="mb-4 flex flex-col gap-1 sm:flex-row sm:items-end sm:justify-between">
          <div>
            <h2 className="font-display text-2xl tracking-tight text-ink">Signal preview</h2>
            <p className="mt-1 text-sm text-muted">
              Columns: ax, ay, az · showing first {Math.min(48, lines.length)} lines
            </p>
          </div>
          <p className="font-mono text-xs uppercase tracking-[0.16em] text-muted">
            {lines.length.toLocaleString()} lines total
          </p>
        </div>
        <pre className="overflow-x-auto border border-line bg-ink p-5 font-mono text-[12px] leading-6 text-[#c9e8eb] shadow-[inset_0_1px_0_rgba(255,255,255,0.04)]">
          {previewLines}
          {truncated ? "\n…" : ""}
        </pre>
      </section>
    </div>
  );
}
