import { createRoot, hydrateRoot } from "react-dom/client";
import { App } from "@/App";
import "@/globals.css";

const container = document.getElementById("root");
if (!container) throw new Error("index.html must contain a #root element");

// Production builds ship prerendered markup (scripts/prerender.mjs), so hydrate
// it; the dev server serves an empty shell and gets a normal client render.
if (container.firstChild) hydrateRoot(container, <App />);
else createRoot(container).render(<App />);
