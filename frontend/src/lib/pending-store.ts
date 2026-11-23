import { randomUUID } from "crypto";
import { promises as fs } from "fs";
import path from "path";

export type PendingCapture = {
  id: string;
  receivedAt: string;
  durationMs: number;
  sampleCount: number;
  originalSampleCount: number;
  cleanedCsv: string;
  preview: string;
};

type StoreFile = {
  items: PendingCapture[];
};

const DATA_DIR = path.join(process.cwd(), ".data");
const STORE_PATH = path.join(DATA_DIR, "pending.json");

async function readStore(): Promise<StoreFile> {
  try {
    const raw = await fs.readFile(STORE_PATH, "utf8");
    const parsed = JSON.parse(raw) as StoreFile;
    return { items: Array.isArray(parsed.items) ? parsed.items : [] };
  } catch {
    return { items: [] };
  }
}

async function writeStore(store: StoreFile): Promise<void> {
  await fs.mkdir(DATA_DIR, { recursive: true });
  await fs.writeFile(STORE_PATH, JSON.stringify(store, null, 2), "utf8");
}

function previewCsv(csv: string, maxLines = 12): string {
  return csv.split(/\r?\n/).slice(0, maxLines).join("\n");
}

export async function listPending(): Promise<PendingCapture[]> {
  const store = await readStore();
  return [...store.items].sort((a, b) => b.receivedAt.localeCompare(a.receivedAt));
}

export async function getPending(id: string): Promise<PendingCapture | null> {
  const store = await readStore();
  return store.items.find((item) => item.id === id) ?? null;
}

export async function addPending(input: {
  durationMs: number;
  originalSampleCount: number;
  cleanedCsv: string;
  sampleCount: number;
}): Promise<PendingCapture> {
  const store = await readStore();
  const item: PendingCapture = {
    id: randomUUID(),
    receivedAt: new Date().toISOString(),
    durationMs: input.durationMs,
    sampleCount: input.sampleCount,
    originalSampleCount: input.originalSampleCount,
    cleanedCsv: input.cleanedCsv,
    preview: previewCsv(input.cleanedCsv),
  };
  store.items.unshift(item);
  await writeStore(store);
  return item;
}

export async function removePending(id: string): Promise<boolean> {
  const store = await readStore();
  const next = store.items.filter((item) => item.id !== id);
  if (next.length === store.items.length) return false;
  await writeStore({ items: next });
  return true;
}
