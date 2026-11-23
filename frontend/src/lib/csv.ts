/**
 * Keep only accelerometer columns (ax, ay, az).
 * Accepts headers with or without time_ms / gyro columns.
 */
export function cleanAccelerometerCsv(raw: string): {
  cleanedCsv: string;
  sampleCount: number;
} {
  const lines = raw
    .replace(/^\uFEFF/, "")
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0);

  if (lines.length === 0) {
    return { cleanedCsv: "ax,ay,az\n", sampleCount: 0 };
  }

  const headerParts = lines[0].split(",").map((p) => p.trim().toLowerCase());
  const looksLikeHeader = headerParts.some((p) => p === "ax" || p === "ay" || p === "az");

  let axIdx = headerParts.indexOf("ax");
  let ayIdx = headerParts.indexOf("ay");
  let azIdx = headerParts.indexOf("az");

  // Fallback layouts:
  // ax,ay,az,gx,gy,gz  → 0,1,2
  // time_ms,ax,ay,az,gx,gy,gz → 1,2,3
  if (axIdx < 0 || ayIdx < 0 || azIdx < 0) {
    if (headerParts[0] === "time_ms" || headerParts.length >= 7) {
      axIdx = 1;
      ayIdx = 2;
      azIdx = 3;
    } else {
      axIdx = 0;
      ayIdx = 1;
      azIdx = 2;
    }
  }

  const dataLines = looksLikeHeader ? lines.slice(1) : lines;
  const rows: string[] = [];

  for (const line of dataLines) {
    const cols = line.split(",").map((c) => c.trim());
    if (cols.length <= Math.max(axIdx, ayIdx, azIdx)) continue;
    const ax = cols[axIdx];
    const ay = cols[ayIdx];
    const az = cols[azIdx];
    if (!ax || !ay || !az) continue;
    if ([ax, ay, az].some((v) => Number.isNaN(Number(v)))) continue;
    rows.push(`${ax},${ay},${az}`);
  }

  return {
    cleanedCsv: `ax,ay,az\n${rows.join("\n")}${rows.length ? "\n" : ""}`,
    sampleCount: rows.length,
  };
}
