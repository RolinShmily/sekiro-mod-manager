"use client";

import { createContext, useContext, useEffect, useState, type ReactNode } from "react";
import { zh } from "@/locales/zh";
import { en } from "@/locales/en";

export type Locale = "zh" | "en";
type Dictionary = typeof zh | typeof en;

interface I18nContextValue {
  locale: Locale;
  setLocale: (value: Locale) => void;
  toggleLocale: () => void;
  t: Dictionary;
}

const Context = createContext<I18nContextValue | null>(null);

export function I18nProvider({ children }: { children: ReactNode }) {
  const [locale, updateLocale] = useState<Locale>("zh");

  useEffect(() => {
    try {
      const saved = localStorage.getItem("smm-website-locale");
      if (saved === "en" || saved === "zh") updateLocale(saved);
    } catch (error) {
      if (!(error instanceof DOMException)) console.warn("Could not read language preference", error);
    }
  }, []);

  function setLocale(value: Locale) {
    updateLocale(value);
    try {
      localStorage.setItem("smm-website-locale", value);
    } catch (error) {
      if (!(error instanceof DOMException)) console.warn("Could not save language preference", error);
    }
  }

  function toggleLocale() {
    setLocale(locale === "zh" ? "en" : "zh");
  }

  const t: Dictionary = locale === "zh" ? zh : en;

  useEffect(() => {
    document.documentElement.lang = locale === "zh" ? "zh-CN" : "en";
    document.title = t.meta.title;
  }, [locale, t.meta.title]);

  return (
    <Context.Provider value={{ locale, setLocale, toggleLocale, t }}>
      {children}
    </Context.Provider>
  );
}

export function useI18n() {
  const context = useContext(Context);
  if (!context) throw new Error("useI18n requires I18nProvider");
  return context;
}
