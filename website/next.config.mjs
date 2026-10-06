/** @type {import("next").NextConfig} */
const nextConfig = {
  output: "export",
  turbopack: { root: import.meta.dirname },
  images: { unoptimized: true },
  basePath: process.env.NEXT_PUBLIC_BASE_PATH || "",
  trailingSlash: true,
};
export default nextConfig;
