# AutoRomberg

Phase 1: capture Romberg balance tests from an ESP32, store CSV in **Neon (Postgres)** via a **Java Spring Boot + Hibernate** API, and browse them in a **Next.js + Tailwind** frontend.

```
ESP32 (romberg_capture.ino)
        │  POST /api/tests  { durationMs, sampleCount, csvContent }
        ▼
Spring Boot (backend/)  ──Hibernate──►  Neon Postgres
        ▲
Next.js (frontend/)  GET /api/tests, /api/tests/{id}
```

## Prerequisites

- Java 21+ (JDK 25 works)
- Node.js 20+
- A [Neon](https://neon.tech) project (Postgres)

## 1. Neon database

1. Create a Neon project and copy the **JDBC** connection string.
2. In `backend/`, create a shell env file from the example:

```bash
cp backend/.env.example backend/.env
```

Edit `backend/.env`:

```bash
DATABASE_URL=jdbc:postgresql://ep-xxxx.region.aws.neon.tech/neondb?sslmode=require
DATABASE_USER=neondb_owner
DATABASE_PASSWORD=your_password
```

Hibernate creates the `romberg_tests` table on first run (`ddl-auto=update`).

## 2. Spring backend

```bash
cd backend
set -a && source .env && set +a   # loads DATABASE_* into the shell
./mvnw spring-boot:run
```

API (port **8080**):

| Method | Path | Purpose |
|--------|------|---------|
| `POST` | `/api/tests` | Ingest CSV (ESP32) |
| `GET` | `/api/tests` | List tests |
| `GET` | `/api/tests/{id}` | Test detail + CSV |
| `GET` | `/api/tests/{id}/csv` | Download CSV file |

ESP32 body shape (already built in `romberg_capture.ino`):

```json
{
  "durationMs": 30000,
  "sampleCount": 1500,
  "csvContent": "ax,ay,az,gx,gy,gz\n0.1,0.2,..."
}
```

Set `API_BASE` in the sketch to your computer’s LAN IP, e.g. `http://192.168.1.20:8080` (not `localhost` from the ESP32).

## 3. Next.js frontend

```bash
cd frontend
cp .env.local.example .env.local
npm run dev
```

Open [http://localhost:3000](http://localhost:3000).

## Manual smoke test

With the backend running against Neon:

```bash
curl -s -X POST http://localhost:8080/api/tests \
  -H 'Content-Type: application/json' \
  -d '{"durationMs":30000,"sampleCount":2,"csvContent":"ax,ay,az,gx,gy,gz\n0.1,0.2,0.3,0.4,0.5,0.6\n0.2,0.1,0.3,0.4,0.5,0.6\n"}'
```

Then refresh the frontend list.

## Repo layout

- `romberg_capture.ino` — ESP32 capture + Wi‑Fi POST
- `backend/` — Spring Boot, JPA/Hibernate, Neon
- `frontend/` — Next.js App Router + Tailwind
