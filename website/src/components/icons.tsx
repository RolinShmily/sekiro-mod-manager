import React from "react";

export function BrandLogo({ className = "w-8 h-8" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 48 48"
      fill="none"
      xmlns="http://www.w3.org/2000/svg"
      aria-hidden="true"
    >
      <rect
        x="3"
        y="3"
        width="42"
        height="42"
        rx="6"
        fill="#9d3429"
        stroke="#d98564"
        strokeWidth="1.5"
      />
      {/* Refined crest geometry */}
      <path
        d="M14 15h20M14 24h20M14 33h20M24 11v26M17 11l-3 4m20-4 3 4M17 33l-3 4m20-4 3 4"
        stroke="#fff4e8"
        strokeWidth="2.2"
        strokeLinecap="round"
        strokeLinejoin="round"
      />
    </svg>
  );
}

export function GithubIcon({ className = "w-5 h-5" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path
        fillRule="evenodd"
        clipRule="evenodd"
        d="M12 2C6.477 2 2 6.484 2 12.017c0 4.425 2.865 8.18 6.839 9.504.5.092.682-.217.682-.483 0-.237-.008-.868-.013-1.703-2.782.605-3.369-1.343-3.369-1.343-.454-1.158-1.11-1.466-1.11-1.466-.908-.62.069-.608.069-.608 1.003.07 1.53 1.032 1.53 1.032.892 1.53 2.341 1.088 2.91.832.092-.647.35-1.088.636-1.338-2.22-.253-4.555-1.113-4.555-4.951 0-1.093.39-1.988 1.029-2.688-.103-.253-.446-1.272.098-2.65 0 0 .84-.27 2.75 1.026A9.564 9.564 0 0112 6.844c.85.004 1.705.115 2.504.337 1.909-1.296 2.747-1.027 2.747-1.027.546 1.379.202 2.398.1 2.651.64.7 1.028 1.595 1.028 2.688 0 3.848-2.339 4.695-4.566 4.943.359.309.678.92.678 1.855 0 1.338-.012 2.419-.012 2.747 0 .268.18.58.688.482A10.019 10.019 0 0022 12.017C22 6.484 17.522 2 12 2z"
      />
    </svg>
  );
}

export function WindowsIcon({ className = "w-5 h-5" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M0 3.449L9.75 2.1v9.451H0m10.949-9.602L24 0v11.4H10.949M0 12.6h9.75v9.451L0 20.699M10.949 12.6H24V24l-12.951-1.799" />
    </svg>
  );
}

export function NextjsIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 180 180"
      fill="currentColor"
      aria-hidden="true"
    >
      <circle cx="90" cy="90" r="90" fill="currentColor" fillOpacity="0.1" />
      <path
        d="M149.508 157.438L69.1419 54H54V125.969H66.5273V69.8315L139.923 164.78C143.239 162.518 146.438 160.065 149.508 157.438Z"
        fill="currentColor"
      />
      <rect x="115" y="54" width="12" height="72" fill="currentColor" />
    </svg>
  );
}

export function ReactIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="-11.5 -10.23174 23 20.46348"
      fill="none"
      stroke="currentColor"
      strokeWidth="1"
      aria-hidden="true"
    >
      <circle cx="0" cy="0" r="2.05" fill="currentColor" stroke="none" />
      <ellipse rx="11" ry="4.2" />
      <ellipse rx="11" ry="4.2" transform="rotate(60)" />
      <ellipse rx="11" ry="4.2" transform="rotate(120)" />
    </svg>
  );
}

export function TypescriptIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M1.5 0h21A1.5 1.5 0 0 1 24 1.5v21a1.5 1.5 0 0 1-1.5 1.5h-21A1.5 1.5 0 0 1 0 22.5v-21A1.5 1.5 0 0 1 1.5 0zm10.74 13.83h-3.4v6.62H6.46v-6.62H3.08v-2.31h9.16v2.31zm9.36 2.5c0 1.25-.46 2.22-1.37 2.9-.91.68-2.15 1.02-3.72 1.02-1.18 0-2.25-.19-3.2-.56v-2.36c.99.53 2.02.8 3.09.8.76 0 1.34-.14 1.74-.42.4-.28.6-.66.6-1.14 0-.37-.14-.68-.42-.92-.28-.24-.76-.48-1.44-.72-1.18-.4-2.02-.91-2.52-1.53-.5-.62-.75-1.39-.75-2.31 0-1.18.44-2.12 1.32-2.82.88-.7 2.05-1.05 3.51-1.05 1.09 0 2.06.18 2.91.54v2.32c-.89-.48-1.83-.72-2.82-.72-.68 0-1.21.13-1.59.39-.38.26-.57.61-.57 1.05 0 .34.13.63.39.87.26.24.75.49 1.47.75 1.24.44 2.11.96 2.61 1.56.5.6.75 1.39.75 2.36z" />
    </svg>
  );
}

export function TailwindIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M12.001,4.8c-3.2,0-5.2,1.6-6,4.8c1.2-1.6,2.6-2.2,4.2-1.8c0.913,0.228,1.565,0.89,2.288,1.624 C13.666,10.618,15.027,12,18.001,12c3.2,0,5.2-1.6,6-4.8c-1.2,1.6-2.6,2.2-4.2,1.8c-0.913-0.228-1.565-0.89-2.288-1.624 C16.337,6.182,14.976,4.8,12.001,4.8z M6.001,12c-3.2,0-5.2,1.6-6,4.8c1.2-1.6,2.6-2.2,4.2-1.8c0.913,0.228,1.565,0.89,2.288,1.624 c1.177,1.194,2.538,2.576,5.512,2.576c3.2,0,5.2-1.6,6-4.8c-1.2,1.6-2.6,2.2-4.2,1.8c-0.913-0.228-1.565-0.89-2.288-1.624 C10.337,13.382,8.976,12,6.001,12z" />
    </svg>
  );
}

export function CloudflareIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M16.545 15.75c.196-.64.088-1.246-.26-1.687-.33-.42-.857-.655-1.528-.682l-5.617-.208c-.13-.005-.246-.075-.308-.189-.06-.114-.055-.251.014-.36.07-.11.189-.175.318-.175l5.807-.023c1.17 0 2.21-.692 2.656-1.765.315-.758.286-1.58-.08-2.316a3.633 3.633 0 0 0-2.073-1.748C14.773 4.417 12.87 3 10.5 3 7.828 3 5.602 4.982 5.295 7.625 3.987 7.973 3 9.176 3 10.625c0 1.864 1.514 3.375 3.375 3.375h9.75c.15 0 .285.08.353.208.067.128.06.284-.019.405l-.914 1.387zM18.75 9.75c-.244 0-.482.025-.713.072.338.647.458 1.402.32 2.164-.287 1.58-1.6 2.764-3.207 2.764h-.4c-.114 0-.222.046-.301.127a.428.428 0 0 0-.11.319c.07.784-.213 1.536-.777 2.062-.572.533-1.343.804-2.172.742-1.393-.104-2.502-1.188-2.64-2.58l-.02-.2c-.015-.152-.104-.282-.243-.342a.428.428 0 0 0-.404.053c-.347.26-.774.401-1.218.401-1.103 0-2-.897-2-2 0-.256.05-.506.148-.737C3.393 13.044 2 14.743 2 16.75 2 19.097 3.903 21 6.25 21h12.5c2.899 0 5.25-2.351 5.25-5.25S21.649 9.75 18.75 9.75z" />
    </svg>
  );
}

export function RadixIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 25 25"
      fill="currentColor"
      aria-hidden="true"
    >
      <path
        d="M12 25C5.37258 25 0 19.6274 0 13C0 6.37258 5.37258 1 12 1C18.6274 1 24 6.37258 24 13C24 19.6274 18.6274 25 12 25ZM12 3C6.47715 3 2 7.47715 2 13C2 18.5228 6.47715 23 12 23C17.5228 23 22 18.5228 22 13C22 7.47715 17.5228 3 12 3Z"
        fillRule="evenodd"
        clipRule="evenodd"
      />
      <circle cx="12" cy="13" r="5" />
    </svg>
  );
}

export function MotionIcon({ className = "w-4 h-4" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="currentColor"
      aria-hidden="true"
    >
      <path d="M4 4h16v8l-8 8-8-8V4zm16 8l-8-8v8h8z" />
    </svg>
  );
}

export function ExeBadgeIcon({ className = "w-5 h-5" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.75"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <rect x="3" y="4" width="18" height="16" rx="2" />
      <path d="M7 8h10M7 12h5m-5 4h8" />
    </svg>
  );
}

export function ZipBadgeIcon({ className = "w-5 h-5" }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.75"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <path d="M10 2v2m4-2v2m-4 2v2m4-2v2m-4 2v2m4-2v2" />
      <rect x="4" y="4" width="16" height="16" rx="2" />
      <circle cx="12" cy="15" r="2" />
      <path d="M12 17v3" />
    </svg>
  );
}
