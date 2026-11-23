import { NextResponse } from "next/server";
import { getPending, removePending } from "@/lib/pending-store";

type RouteContext = {
  params: Promise<{ id: string }>;
};

export async function POST(_request: Request, context: RouteContext) {
  const { id } = await context.params;
  const pending = await getPending(id);
  if (!pending) {
    return NextResponse.json({ error: "Pending capture not found" }, { status: 404 });
  }

  const apiBase = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:8080";
  const res = await fetch(`${apiBase}/api/tests`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({
      durationMs: pending.durationMs,
      sampleCount: pending.sampleCount,
      csvContent: pending.cleanedCsv,
    }),
  });

  const text = await res.text();
  if (!res.ok) {
    return NextResponse.json(
      {
        error: "Failed to save cleaned CSV to Spring/Neon",
        status: res.status,
        detail: text,
      },
      { status: 502 },
    );
  }

  await removePending(id);

  let saved: unknown = null;
  try {
    saved = JSON.parse(text);
  } catch {
    saved = text;
  }

  return NextResponse.json({ ok: true, saved });
}
