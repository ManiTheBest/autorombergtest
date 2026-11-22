import Link from "next/link";
import { WaveVisual } from "@/components/WaveVisual";
import { listTests, type TestSummary } from "@/lib/api";

export const dynamic = "force-dynamic";

function formatWhen(iso: string) {
  return new Intl.DateTimeFormat(undefined, {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(new Date(iso));
}

function formatRate(sampleCount: number, durationMs: number) {
  if (durationMs <= 0) return "—";
  return `${Math.round(sampleCount / (durationMs / 1000))} Hz`;
}

export default async function HomePage() {
  let tests: TestSummary[] = [];
  let error: string | null = null;

  try {
    tests = await listTests();
  } catch (err) {
    error = err instanceof Error ? err.message : "Failed to load tests";
  }

  return (
    <>
      <section className="relative isolate min-h-[78vh] overflow-hidden bg-ink text-panel">
        <div className="absolute inset-0">
          <WaveVisual />
        </div>
        <div className="absolute inset-0 bg-gradient-to-r from-ink via-ink/88 to-ink/25" />

        <div className="relative mx-auto flex min-h-[78vh] w-full max-w-6xl flex-col justify-end px-5 pb-14 pt-24 sm:px-8 sm:pb-20">
          <p className="anim-rise font-mono text-[11px] uppercase tracking-[0.28em] text-signal">
            Instrumented balance assessment
          </p>
          <h1 className="anim-rise-delay-1 mt-4 max-w-3xl font-display text-5xl leading-[1.05] tracking-tight text-white sm:text-6xl md:text-7xl">
            AutoRomberg
          </h1>
          <p className="anim-rise-delay-2 mt-5 max-w-xl text-lg leading-relaxed text-white/72 sm:text-xl">
            Capture standing-balance IMU streams from the ESP32 and review stored sessions with
            clinical clarity.
          </p>
          <div className="anim-rise-delay-2 mt-9 flex flex-wrap gap-3">
            <a
              href="#sessions"
              className="cta-primary inline-flex items-center rounded-md bg-signal px-5 py-3 text-sm font-semibold text-ink"
            >
              Open session library
            </a>
            <Link
              href={tests[0] ? `/tests/${tests[0].id}` : "#sessions"}
              className="cta-secondary inline-flex items-center rounded-md border border-white/25 px-5 py-3 text-sm font-medium text-white"
            >
              {tests[0] ? "Latest session" : "Awaiting first capture"}
            </Link>
          </div>
        </div>
      </section>

      <section id="sessions" className="mx-auto w-full max-w-6xl px-5 py-16 sm:px-8 sm:py-20">
        <div className="mb-10 flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">
          <div>
            <p className="font-mono text-[11px] uppercase tracking-[0.22em] text-muted">
              Session library
            </p>
            <h2 className="mt-2 font-display text-3xl tracking-tight text-ink sm:text-4xl">
              Recorded Romberg tests
            </h2>
            <p className="mt-3 max-w-2xl text-muted">
              Each row is a 30-second capture posted by the device into Neon through the Spring API.
            </p>
          </div>
          <p className="font-mono text-sm text-muted">
            {error ? "API offline" : `${tests.length} session${tests.length === 1 ? "" : "s"}`}
          </p>
        </div>

        {error ? (
          <div className="border border-danger/20 bg-danger-soft px-5 py-4 text-sm text-danger">
            <p className="font-semibold">Could not reach the ingest API</p>
            <p className="mt-1 text-danger/80">
              Start the Spring backend on port 8080, then refresh this page.
            </p>
            <p className="mt-2 font-mono text-xs opacity-80">{error}</p>
          </div>
        ) : null}

        {!error && tests.length === 0 ? (
          <div className="border border-dashed border-line bg-panel/70 px-6 py-14 text-center">
            <p className="font-display text-2xl text-ink">No sessions yet</p>
            <p className="mx-auto mt-3 max-w-lg text-muted">
              Flash <span className="font-mono text-foreground">romberg_capture.ino</span>, connect
              over Serial, and run a capture. Successful uploads appear here automatically.
            </p>
          </div>
        ) : null}

        {tests.length > 0 ? (
          <div className="overflow-hidden border border-line bg-panel/90 shadow-[0_24px_60px_rgba(12,26,36,0.06)]">
            <div className="overflow-x-auto">
              <table className="w-full min-w-[640px] text-left text-sm">
                <thead>
                  <tr className="border-b border-line bg-[#edf3f5] font-mono text-[11px] uppercase tracking-[0.16em] text-muted">
                    <th className="px-5 py-4 font-medium">Session</th>
                    <th className="px-5 py-4 font-medium">Captured</th>
                    <th className="px-5 py-4 font-medium">Duration</th>
                    <th className="px-5 py-4 font-medium">Samples</th>
                    <th className="px-5 py-4 font-medium">Rate</th>
                    <th className="px-5 py-4 font-medium" />
                  </tr>
                </thead>
                <tbody>
                  {tests.map((test) => (
                    <tr key={test.id} className="row-link border-b border-line last:border-b-0">
                      <td className="px-5 py-4 font-mono text-foreground">#{test.id}</td>
                      <td className="px-5 py-4 text-foreground">{formatWhen(test.createdAt)}</td>
                      <td className="px-5 py-4">{(test.durationMs / 1000).toFixed(0)}s</td>
                      <td className="px-5 py-4">{test.sampleCount.toLocaleString()}</td>
                      <td className="px-5 py-4 font-mono text-muted">
                        {formatRate(test.sampleCount, test.durationMs)}
                      </td>
                      <td className="px-5 py-4 text-right">
                        <Link
                          href={`/tests/${test.id}`}
                          className="font-semibold text-accent-deep underline-offset-4 hover:underline"
                        >
                          Review
                        </Link>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        ) : null}
      </section>
    </>
  );
}
