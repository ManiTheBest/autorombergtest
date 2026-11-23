# AutoRomberg

Capture Romberg balance tests from an ESP32, **review cleaned accelerometer CSV in the Next.js web app**, then optionally save to **Neon** via **Spring Boot + Hibernate**.

```
ESP32 (romberg_capture.ino)
        │  POST /api/ingest  { durationMs, sampleCount, csvContent }
        ▼
Next.js (frontend/)
        │  cleans to ax,ay,az · queues for review
        │  user clicks "Send cleaned CSV to database"
        ▼
Spring Boot (backend/)  ──Hibernate──►  Neon Postgres
```

Gyro columns (`gx,gy,gz`) are not stored for this project.

## Prerequisites

- Java 21+ (JDK 25 works)
- Node.js 20+
- A [Neon](https://neon.tech) project (Postgres)

## 1. Neon database

```bash
cp backend/.env.example backend/.env
```

Edit `backend/.env` with your Neon JDBC URL, user, and password.

## 2. Spring backend (saves approved CSVs)

```bash
cd backend
set -a && source .env && set +a
./mvnw spring-boot:run
```

Runs on **http://localhost:8080**.

## 3. Next.js frontend (device ingest + review UI)

```bash
cd frontend
cp .env.local.example .env.local
npm run dev
```

Open **http://localhost:3000**.

| Method | Path | Purpose |
|--------|------|---------|
| `POST` | `/api/ingest` | ESP32 upload → clean → pending queue |
| `GET` | `/api/pending` | List pending captures |
| `POST` | `/api/pending/{id}/commit` | Save cleaned CSV to Spring/Neon |
| `DELETE` | `/api/pending/{id}` | Discard pending capture |

## 4. ESP32 sketch

In `romberg_capture.ino` set Wi‑Fi and:

```cpp
const char *API_BASE = "http://192.168.1.163:3000"; // your Mac LAN IP + Next.js
```

Flash, open Serial Monitor at 115200, send any character to run a test.  
On success you should see HTTP **202** and the capture appears under **Incoming device captures**. Click **Send cleaned CSV to database** to write it to Neon.

## Manual smoke test (without ESP32)

With Next.js running:

```bash
curl -s -X POST http://localhost:3000/api/ingest \
  -H 'Content-Type: application/json' \
  -d '{"durationMs":30000,"sampleCount":2,"csvContent":"ax,ay,az,gx,gy,gz\n0.1,0.2,0.3,0.4,0.5,0.6\n0.2,0.1,0.3,0.4,0.5,0.6\n"}'
```

Then open the site, approve the pending item (Spring must be running for commit).

## Repo layout

- `romberg_capture.ino` — ESP32 capture → Next.js ingest
- `backend/` — Spring Boot, JPA/Hibernate, Neon
- `frontend/` — review UI + session library
