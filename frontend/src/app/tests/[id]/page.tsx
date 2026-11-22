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
      <div className="space-y-4">
        <Link href="/" className="text-sm text-accent hover:underline">
          ← Back to tests
        </Link>
        <div className="rounded-xl border border-red-200 bg-red-50 px-4 py-3 text-sm text-red-800">
          {error ?? "Test not found"}
        </div>
      </div>
    );
  }

  const previewLines = test.csvContent.split("\n").slice(0, 40).join("\n");
  const truncated = test.csvContent.split("\n").length > 40;

  return (
    <div className="space-y-8">
      <div className="space-y-3">
        <Link href="/" className="text-sm text-accent hover:underline">
          ← Back to tests
        </Link>
        <h1 className="text-3xl font-semibold tracking-tight">Test #{test.id}</h1>
        <p className="text-muted">Captured {formatWhen(test.createdAt)}</p>
      </div>

      <dl className="grid gap-4 sm:grid-cols-3">
        <div className="rounded-xl border border-line bg-panel px-4 py-3">
          <dt className="text-xs uppercase tracking-wide text-muted">Duration</dt>
          <dd className="mt-1 text-xl font-semibold">{(test.durationMs / 1000).toFixed(0)}s</dd>
        </div>
        <div className="rounded-xl border border-line bg-panel px-4 py-3">
          <dt className="text-xs uppercase tracking-wide text-muted">Samples</dt>
          <dd className="mt-1 text-xl font-semibold">{test.sampleCount}</dd>
        </div>
        <div className="rounded-xl border border-line bg-panel px-4 py-3">
          <dt className="text-xs uppercase tracking-wide text-muted">CSV</dt>
          <dd className="mt-1">
            <a
              href={csvDownloadUrl(test.id)}
              className="text-xl font-semibold text-accent hover:underline"
            >
              Download
            </a>
          </dd>
        </div>
      </dl>

      <section className="space-y-3">
        <h2 className="text-lg font-semibold">CSV preview</h2>
        <pre className="overflow-x-auto rounded-xl border border-line bg-[#0f1714] p-4 font-mono text-xs leading-relaxed text-[#d7ebe3]">
          {previewLines}
          {truncated ? "\n…" : ""}
        </pre>
      </section>
    </div>
  );
}
