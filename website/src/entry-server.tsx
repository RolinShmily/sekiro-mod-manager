import { renderToString } from "react-dom/server";
import { App } from "@/App";

/** Build-time prerender entry: keeps the first paint readable without JavaScript. */
export function render(): string {
  return renderToString(<App />);
}
