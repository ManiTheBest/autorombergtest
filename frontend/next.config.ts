import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // Live API viewer — keep pages uncached by default
  cacheComponents: false,
  turbopack: {
    rules: {
      "*.css": {
        loaders: ["@tailwindcss/turbopack"],
        as: "*.css",
      },
    },
  },
};

export default nextConfig;
