export function WaveVisual() {
  return (
    <div className="relative h-full min-h-[280px] w-full overflow-hidden" aria-hidden="true">
      <div className="absolute inset-0 bg-gradient-to-tr from-ink via-[#0c2430] to-[#134650]" />
      <div className="site-grid absolute inset-0 opacity-35" />
      <div className="absolute -right-16 top-10 h-56 w-56 rounded-full bg-signal/20 blur-3xl" />
      <div className="absolute -left-10 bottom-0 h-40 w-40 rounded-full bg-accent/25 blur-3xl" />

      <svg
        className="absolute inset-0 h-full w-full"
        viewBox="0 0 640 360"
        preserveAspectRatio="none"
      >
        <path
          className="wave-path"
          d="M0 210 C40 190 70 250 110 220 C150 190 180 140 220 165 C260 190 290 240 330 210 C370 180 400 120 440 150 C480 180 510 230 550 200 C580 180 610 170 640 185"
          fill="none"
          stroke="rgba(125, 211, 217, 0.9)"
          strokeWidth="2.5"
        />
        <path
          d="M0 240 C50 255 90 225 130 235 C180 248 210 270 260 250 C310 230 340 200 390 215 C440 230 480 260 530 245 C580 230 610 220 640 230"
          fill="none"
          stroke="rgba(255, 255, 255, 0.22)"
          strokeWidth="1.5"
        />
      </svg>
    </div>
  );
}
