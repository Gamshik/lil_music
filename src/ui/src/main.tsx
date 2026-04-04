import { StrictMode } from "react";
import { createRoot } from "react-dom/client";
import { App } from "@app/App";
import "@styles/globals.scss";

const rootElement = document.getElementById("root");

if (!rootElement) {
  throw new Error("LilMusic root element was not found.");
}

createRoot(rootElement).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
