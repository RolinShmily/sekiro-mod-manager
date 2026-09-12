/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        canvas: '#ffffff',
        'surface-soft': '#f1f4f7',
        primary: {
          DEFAULT: '#0064e0',
          deep: '#0457cb',
          soft: '#0091ff',
          'on-primary': '#ffffff',
        },
        'primary-deep': '#0457cb',
        'primary-soft': '#0091ff',
        'on-primary': '#ffffff',
        'ink-button': '#000000',
        'on-ink-button': '#ffffff',
        'ink-deep': '#0a1317',
        ink: '#1c1e21',
        charcoal: '#444950',
        slate: '#4b4c4f',
        steel: '#5d6c7b',
        stone: '#8595a4',
        hairline: '#ced0d4',
        'hairline-soft': '#dee3e9',
        'disabled-text': '#bcc0c4',
        'fb-blue': '#1876f2',
        'meta-link': '#385898',
        'oculus-purple': '#a121ce',
        success: {
          DEFAULT: '#31a24c',
          bg: '#24e400',
        },
        warning: {
          DEFAULT: '#f7b928',
          bg: '#ffe200',
        },
        attention: '#f2a918',
        critical: {
          DEFAULT: '#e41e3f',
          strong: '#f0284a',
        },
        'critical-strong': '#f0284a',
      },
      borderRadius: {
        xl: '16px',
        xxl: '24px',
        xxxl: '32px',
        feature: '40px',
        full: '100px',
        circle: '9999px',
      },
      boxShadow: {
        subtle: '0 1px 2px 0 rgba(0, 0, 0, 0.05)',
        card: '0 1px 3px 0 rgba(0, 0, 0, 0.04), 0 1px 2px -1px rgba(0, 0, 0, 0.04)',
        panel: 'rgba(20, 22, 26, 0.06) 0px 1px 4px 0px',
        sticky: 'rgba(20, 22, 26, 0.18) 0px 1px 4px 0px',
        float: '0 12px 30px -10px rgba(0, 0, 0, 0.12), 0 4px 12px -4px rgba(0, 0, 0, 0.06)',
        pill: 'rgba(0, 0, 0, 0.15) 1px 1px 0px 0px',
      },
      fontFamily: {
        sans: ['"Maple Mono NF CN"', '"Maple Mono SC NF"', '"Maple Mono"', 'monospace', 'sans-serif'],
        mono: ['"Maple Mono NF CN"', '"Maple Mono SC NF"', '"Maple Mono"', 'monospace'],
        serif: ['"Cinzel"', '"Maple Mono NF CN"', '"Noto Serif SC"', 'serif'],
      },
    },
  },
  plugins: [],
}
