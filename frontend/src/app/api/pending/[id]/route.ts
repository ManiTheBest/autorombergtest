import { NextResponse } from "next/server";
import { removePending } from "@/lib/pending-store";

type RouteContext = {
  params: Promise<{ id: string }>;
};

export async function DELETE(_request: Request, context: RouteContext) {
  const { id } = await context.params;
  const removed = await removePending(id);
  if (!removed) {
    return NextResponse.json({ error: "Pending capture not found" }, { status: 404 });
  }
  return NextResponse.json({ ok: true });
}
