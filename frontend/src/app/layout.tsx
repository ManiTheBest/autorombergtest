import type { Metadata } from "next";
import { Fraunces, IBM_Plex_Mono, Source_Sans_3 } from "next/font/google";
import Link from "next/link";
import "./globals.css";

const display = Fraunces({
  variable: "--font-display",
  subsets: ["latin"],
});

const body = Source_Sans_3({
  variable: "--font-body",
  subsets: ["latin"],
});

const mono = IBM_Plex_Mono({
  variable: "--font-mono",
  subsets: ["latin"],
  weight: ["400", "500"],
});

export const metadata: Metadata = {
  title: "AutoRomberg",
  description: "Instrument-grade Romberg balance capture and review",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html
      lang="en"
      className={`${display.variable} ${body.variable} ${mono.variable} h-full antialiased`}
    >
      <body className="flex min-h-full flex-col">
        <header className="sticky top-0 z-40 border-b border-line/70 bg-[#f4f7fa]/85 backdrop-blur-md">
          <div className="mx-auto flex w-full max-w-6xl items-center justify-between px-5 py-4 sm:px-8">
            <Link href="/" className="group flex items-center gap-3">
              <span className="relative flex h-9 w-9 items-center justify-center rounded-md bg-ink text-panel">
                <span className="absolute inset-x-2 top-[11px] h-px bg-signal/80" />
                <span className="absolute inset-x-3 top-[17px] h-px bg-white/35" />
                <span className="absolute inset-x-2.5 top-[23px] h-px bg-white/20" />
              </span>
              <span className="font-display text-xl tracking-tight text-ink transition-colors group-hover:text-accent-deep">
                AutoRomberg
              </span>
            </Link>
            <nav className="flex items-center gap-6 text-sm text-muted">
              <a href="#sessions" className="hidden hover:text-foreground sm:inline">
                Sessions
              </a>
              <span className="hidden font-mono text-[11px] uppercase tracking-[0.16em] text-muted/80 md:inline">
                Balance capture lab
              </span>
            </nav>
          </div>
        </header>

        <div className="flex-1">{children}</div>

        <footer className="mt-auto border-t border-line/80 bg-panel/60">
          <div className="mx-auto flex w-full max-w-6xl flex-col gap-2 px-5 py-8 text-sm text-muted sm:flex-row sm:items-center sm:justify-between sm:px-8">
            <p className="font-display text-base text-foreground">AutoRomberg</p>
            <p>ESP32 capture · Spring ingest · Neon storage</p>
          </div>
        </footer>
      </body>
    </html>
  );
}
