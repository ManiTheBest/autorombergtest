import Link from "next/link";
import { listTests, type TestSummary } from "@/lib/api";

export const dynamic = "force-dynamic";

function formatWhen(iso: string) {
  return new Intl.DateTimeFormat(undefined, {
    dateStyle: "medium",
    timeStyle: "short",
  }).format(new Date(iso));
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
    <div className="space-y-8">
      <section className="space-y-2">
        <h1 className="text-3xl font-semibold tracking-tight">Recorded tests</h1>
        <p className="max-w-2xl text-muted">
          CSV captures posted by the ESP32 Romberg sketch are stored in Neon via the Spring API.
        </p>
      </section>

      {error ? (
        <div className="rounded-xl border border-red-200 bg-red-50 px-4 py-3 text-sm text-red-800">
          Could not reach the API. Is the Spring backend running on port 8080?
          <div className="mt-1 font-mono text-xs opacity-80">{error}</div>
        </div>
      ) : null}

      {!error && tests.length === 0 ? (
        <div className="rounded-xl border border-line bg-panel px-5 py-8 text-muted">
          No tests yet. Flash{" "}
          <code className="font-mono text-foreground">romberg_capture.ino</code> and run a
          capture to POST into <code className="font-mono text-foreground">/api/tests</code>.
        </div>
      ) : null}

      {tests.length > 0 ? (
        <div className="overflow-hidden rounded-xl border border-line bg-panel">
          <table className="w-full text-left text-sm">
            <thead className="bg-accent-soft/60 text-muted">
              <tr>
                <th className="px-4 py-3 font-medium">ID</th>
                <th className="px-4 py-3 font-medium">Recorded</th>
                <th className="px-4 py-3 font-medium">Duration</th>
                <th className="px-4 py-3 font-medium">Samples</th>
                <th className="px-4 py-3 font-medium" />
              </tr>
            </thead>
            <tbody>
              {tests.map((test) => (
                <tr key={test.id} className="border-t border-line">
                  <td className="px-4 py-3 font-mono">#{test.id}</td>
                  <td className="px-4 py-3">{formatWhen(test.createdAt)}</td>
                  <td className="px-4 py-3">{(test.durationMs / 1000).toFixed(0)}s</td>
                  <td className="px-4 py-3">{test.sampleCount}</td>
                  <td className="px-4 py-3 text-right">
                    <Link
                      href={`/tests/${test.id}`}
                      className="font-medium text-accent hover:underline"
                    >
                      View
                    </Link>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      ) : null}
    </div>
  );
}
