"use client";

import { animate, useReducedMotion } from "motion/react";
import { useEffect, useRef, type ReactNode } from "react";

export function RevealProvider({ children }: { children: ReactNode }) {
  const root = useRef<HTMLDivElement>(null);
  const reduced = useReducedMotion();
  useEffect(() => {
    const elements = Array.from(root.current?.querySelectorAll<HTMLElement>(".reveal") || []);
    if (reduced) { elements.forEach(element => { element.style.opacity = "1"; element.style.transform = "none"; }); return; }
    const animations: ReturnType<typeof animate>[] = [];
    const observer = new IntersectionObserver(entries => {
      for (const entry of entries) {
        if (!entry.isIntersecting) continue;
        animations.push(animate(entry.target as HTMLElement, { opacity: [0, 1], transform: ["translateY(14px)", "translateY(0)"] }, { duration: .35, ease: "easeOut" }));
        observer.unobserve(entry.target);
      }
    }, { threshold: .08 });
    elements.forEach(element => observer.observe(element));
    return () => { observer.disconnect(); animations.forEach(animation => animation.stop()); };
  }, [reduced]);
  return <div ref={root}>{children}</div>;
}
