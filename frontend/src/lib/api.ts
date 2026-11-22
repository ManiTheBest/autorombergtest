const API_BASE = process.env.NEXT_PUBLIC_API_URL ?? "http://localhost:8080";

export type TestSummary = {
  id: number;
  createdAt: string;
  durationMs: number;
  sampleCount: number;
};

export type TestDetail = TestSummary & {
  csvContent: string;
};

async function parseJson<T>(res: Response): Promise<T> {
  if (!res.ok) {
    const text = await res.text();
    throw new Error(text || `Request failed (${res.status})`);
  }
  return res.json() as Promise<T>;
}

export async function listTests(): Promise<TestSummary[]> {
  const res = await fetch(`${API_BASE}/api/tests`, { cache: "no-store" });
  return parseJson<TestSummary[]>(res);
}

export async function getTest(id: string | number): Promise<TestDetail> {
  const res = await fetch(`${API_BASE}/api/tests/${id}`, { cache: "no-store" });
  return parseJson<TestDetail>(res);
}

export function csvDownloadUrl(id: string | number): string {
  return `${API_BASE}/api/tests/${id}/csv`;
}
