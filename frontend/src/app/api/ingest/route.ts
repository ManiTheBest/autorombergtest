import { NextResponse } from "next/server";
import { cleanAccelerometerCsv } from "@/lib/csv";
import { addPending } from "@/lib/pending-store";

type IngestBody = {
  durationMs?: number;
  sampleCount?: number;
  csvContent?: string;
};

export async function POST(request: Request) {
  let body: IngestBody;
  try {
    body = (await request.json()) as IngestBody;
  } catch {
    return NextResponse.json({ error: "Invalid JSON body" }, { status: 400 });
  }

  const csvContent = typeof body.csvContent === "string" ? body.csvContent : "";
  if (!csvContent.trim()) {
    return NextResponse.json({ error: "csvContent is required" }, { status: 400 });
  }

  const durationMs =
    typeof body.durationMs === "number" && body.durationMs > 0 ? body.durationMs : 30000;
  const originalSampleCount =
    typeof body.sampleCount === "number" && body.sampleCount > 0 ? body.sampleCount : 0;

  const { cleanedCsv, sampleCount } = cleanAccelerometerCsv(csvContent);
  if (sampleCount === 0) {
    return NextResponse.json(
      { error: "No accelerometer rows found after cleaning" },
      { status: 400 },
    );
  }

  const pending = await addPending({
    durationMs,
    originalSampleCount: originalSampleCount || sampleCount,
    cleanedCsv,
    sampleCount,
  });

  return NextResponse.json(
    {
      id: pending.id,
      status: "pending_review",
      message: "CSV cleaned to ax,ay,az and queued for review in the web app",
      sampleCount: pending.sampleCount,
      receivedAt: pending.receivedAt,
    },
    { status: 202 },
  );
}
