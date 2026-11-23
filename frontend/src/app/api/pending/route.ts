import { NextResponse } from "next/server";
import { listPending } from "@/lib/pending-store";

export async function GET() {
  const items = await listPending();
  return NextResponse.json(items);
}
