"use client";

import { createContext, useContext, useEffect, useState, type ReactNode } from "react";
import { zh, type MessageKey } from "@/locales/zh";
import { en } from "@/locales/en";

export type Locale = "zh" | "en";
const Context = createContext<{ locale: Locale; setLocale: (value: Locale) => void; t: (key: MessageKey) => string } | null>(null);

export function I18nProvider({ children }: { children: ReactNode }) {
  const [locale, updateLocale] = useState<Locale>("zh");
  useEffect(() => {
    try { if (localStorage.getItem("smm-website-locale") === "en") updateLocale("en"); }
    catch (error) { if (!(error instanceof DOMException)) console.warn("Could not read language preference", error); }
  }, []);
  function setLocale(value: Locale) {
    updateLocale(value);
    try { localStorage.setItem("smm-website-locale", value); }
    catch (error) { if (!(error instanceof DOMException)) console.warn("Could not save language preference", error); }
  }
  useEffect(() => {
    document.documentElement.lang = locale === "zh" ? "zh-CN" : "en";
    document.title = locale === "zh" ? "Sekiro Mod Manager · 重返苇名，由你执掌" : "Sekiro Mod Manager · Return to Ashina. On your terms.";
  }, [locale]);
  return <Context.Provider value={{ locale, setLocale, t: key => (locale === "zh" ? zh : en)[key] }}>{children}</Context.Provider>;
}

export function useI18n() {
  const context = useContext(Context);
  if (!context) throw new Error("useI18n requires I18nProvider");
  return context;
}
