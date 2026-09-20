import React, { useId } from 'react';

export interface SekiroLogoProps {
  size?: number | string;
  className?: string;
  variant?: 'full' | 'seal' | 'crest';
  title?: string;
}

export const SekiroLogo: React.FC<SekiroLogoProps> = ({
  size = 38,
  className = '',
  title = '隻狼: Shadows Die Twice',
}) => {
  const rawId = useId();
  const id = rawId.replace(/[^a-zA-Z0-9]/g, '');

  const sealGradId = `sekiro-seal-grad-${id}`;
  const goldGradId = `sekiro-gold-grad-${id}`;
  const inkShadowId = `sekiro-ink-shadow-${id}`;

  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      viewBox="0 0 100 100"
      width={size}
      height={size}
      fill="none"
      className={`select-none flex-shrink-0 ${className}`}
      role="img"
      aria-label={title}
    >
      <title>{title}</title>
      <defs>
        {/* Sengoku Cinnabar Lacquer Seal Gradient */}
        <radialGradient id={sealGradId} cx="38%" cy="32%" r="75%">
          <stop offset="0%" stopColor="#ef4444" />
          <stop offset="35%" stopColor="#dc2626" />
          <stop offset="70%" stopColor="#991b1b" />
          <stop offset="100%" stopColor="#581010" />
        </radialGradient>

        {/* Ashina Antique Gold Rim Gradient */}
        <linearGradient id={goldGradId} x1="0%" y1="0%" x2="100%" y2="100%">
          <stop offset="0%" stopColor="#fef08a" />
          <stop offset="35%" stopColor="#eab308" />
          <stop offset="75%" stopColor="#b45309" />
          <stop offset="100%" stopColor="#78350f" />
        </linearGradient>

        {/* Deep Ink Drop Shadow */}
        <filter id={inkShadowId} x="-20%" y="-20%" width="140%" height="140%">
          <feDropShadow dx="0" dy="1.2" stdDeviation="1.2" floodColor="#000000" floodOpacity="0.45" />
        </filter>
      </defs>

      {/* Outer Lacquered Cinnabar Square Seal (Sengoku Hanko Style) */}
      <rect
        x="6"
        y="6"
        width="88"
        height="88"
        rx="22"
        fill={`url(#${sealGradId})`}
        stroke={`url(#${goldGradId})`}
        strokeWidth="2.8"
        filter={`url(#${inkShadowId})`}
      />

      {/* Inner Fine Antique Gold Border */}
      <rect
        x="12"
        y="12"
        width="76"
        height="76"
        rx="16"
        fill="none"
        stroke={`url(#${goldGradId})`}
        strokeWidth="1"
        strokeOpacity="0.6"
      />

      {/* Four Corner Accents */}
      <circle cx="16" cy="16" r="1.5" fill="#fef08a" fillOpacity="0.8" />
      <circle cx="84" cy="16" r="1.5" fill="#fef08a" fillOpacity="0.8" />
      <circle cx="16" cy="84" r="1.5" fill="#fef08a" fillOpacity="0.8" />
      <circle cx="84" cy="84" r="1.5" fill="#fef08a" fillOpacity="0.8" />

      {/* Vertical Japanese Brush Calligraphy: 隻狼 */}
      <g filter={`url(#${inkShadowId})`}>
        {/* Top Kanji: 隻 */}
        <text
          x="50"
          y="46"
          textAnchor="middle"
          fill="#ffffff"
          fontSize="36"
          fontWeight="900"
          fontFamily='"Shippori Mincho", "Klee One", "Noto Serif JP", "Yu Mincho", "MS Mincho", "STSong", serif'
        >
          隻
        </text>

        {/* Bottom Kanji: 狼 */}
        <text
          x="50"
          y="83"
          textAnchor="middle"
          fill="#ffffff"
          fontSize="36"
          fontWeight="900"
          fontFamily='"Shippori Mincho", "Klee One", "Noto Serif JP", "Yu Mincho", "MS Mincho", "STSong", serif'
        >
          狼
        </text>
      </g>

      {/* Subtle Brush Streak Accent Across Top Edge */}
      <path
        d="M24 16 Q 50 13 76 16"
        stroke="#ffffff"
        strokeWidth="0.8"
        strokeOpacity="0.4"
        strokeLinecap="round"
      />
    </svg>
  );
};
