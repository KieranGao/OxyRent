---
name: OxyRent
description: Vehicle rental management system — dark luxury with warm gold accents
colors:
  bg-primary: "#050505"
  bg-secondary: "#0a0a0a"
  bg-surface: "#111111"
  bg-elevated: "#1a1a1a"
  bg-hover: "#222222"
  accent: "#c8a96e"
  accent-light: "#d4b87a"
  accent-dark: "#a88b5a"
  success: "#4ade80"
  warning: "#fbbf24"
  danger: "#f87171"
  info: "#60a5fa"
  text-primary: "#f0f0f0"
  text-secondary: "#8a8a8a"
  text-tertiary: "#555555"
  border: "rgba(255, 255, 255, 0.06)"
  border-hover: "rgba(255, 255, 255, 0.12)"
typography:
  display:
    fontFamily: "Cormorant Garamond, Georgia, serif"
    fontWeight: 400
    lineHeight: 1.2
    letterSpacing: "-0.02em"
  body:
    fontFamily: "Outfit, -apple-system, BlinkMacSystemFont, sans-serif"
    fontWeight: 400
    lineHeight: 1.6
    fontSize: "14px"
  label:
    fontFamily: "Outfit, sans-serif"
    fontWeight: 500
    fontSize: "11px"
    letterSpacing: "0.08em"
    textTransform: "uppercase"
rounded:
  sm: "6px"
  md: "10px"
  lg: "14px"
  xl: "20px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "16px"
  lg: "24px"
  xl: "32px"
components:
  button-primary:
    backgroundColor: "linear-gradient(135deg, #c8a96e, #a88b5a)"
    textColor: "#050505"
    rounded: "{rounded.md}"
    padding: "10px 24px"
    fontWeight: 600
  button-ghost:
    backgroundColor: "{colors.bg-elevated}"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.md}"
    padding: "10px 24px"
  input:
    backgroundColor: "#0d0d0d"
    textColor: "{colors.text-primary}"
    rounded: "{rounded.md}"
    border: "1px solid {colors.border}"
  card:
    backgroundColor: "{colors.bg-surface}"
    rounded: "{rounded.lg}"
    border: "1px solid {colors.border}"
  tag:
    rounded: "20px"
    fontSize: "11px"
    padding: "2px 10px"
---

# Design System: OxyRent

## 1. Overview

**Creative North Star: "The Night Office"**

A fleet manager's desk after hours — the glow of a single warm lamp on dark wood, organized stacks of paperwork, a quiet hum of machines. The interface should feel like that desk: precise, warm, and silent. Every element earns its place through function, not decoration.

This system rejects the reflexive dark-mode SaaS dashboard (neon accents, glass-everything, gradient hero cards). It rejects the crypto terminal (glowing borders, futuristic type). It rejects the over-designed admin panel (heavy branding, decorative illustrations, animated page transitions). Instead it leans into earned familiarity — patterns that a user fluent in Linear or Stripe would recognize and trust immediately.

**Key Characteristics:**
- Near-black surfaces (#050505–#111111) with a single warm gold accent (#c8a96e) used sparingly for actions and active states
- Serif display headings (Cormorant Garamond) soften what would otherwise be a cold dark UI
- Solid content surfaces — no decorative glass on content
- 150–250ms transitions on state changes, nothing decorative
- Dense data tables, spacious dashboards, comfortable forms — spacing serves content

## 2. Colors: The Noir Palette

A near-black canvas with warm gold punctuation. The palette is intentionally restrained — one accent, used for primary actions, active selections, and key indicators only.

### Primary
- **Warm Gold** (#c8a96e): The sole accent. Used on primary buttons, active nav items, selected states, and key data indicators. Never decorative — its rarity is the point.

### Neutral
- **Void** (#050505): The deepest background. Page canvas, sidebar base.
- **Carbon** (#111111): Content surfaces. Cards, tables, form panels.
- **Slate** (#1a1a1a): Elevated surfaces. Dropdowns, popovers, hover states.
- **Smoke** (#222222): Hover feedback on interactive surfaces.
- **Ink** (#f0f0f0): Primary text. High contrast on dark surfaces.
- **Iron** (#8a8a8a): Secondary text. Labels, descriptions, metadata. ≥4.5:1 on carbon.
- **Ash** (#555555): Tertiary text. Timestamps, placeholder content.
- **Line** (rgba(255,255,255,0.06)): Default borders. Structural, not decorative.
- **Line-hover** (rgba(255,255,255,0.12)): Hover borders. Subtle lift feedback.

### Semantic
- **Spring** (#4ade80): Success states, active/available indicators.
- **Amber** (#fbbf24): Warnings, pending states, attention needed.
- **Coral** (#f87171): Errors, danger states, overdue indicators.
- **Sky** (#60a5fa): Information, links, neutral highlights.

### Named Rules

**The One Accent Rule.** Warm gold appears on ≤10% of any given screen. Its scarcity is what makes it meaningful. If everything is gold, nothing is.

**The Solid Surface Rule.** Content surfaces (cards, tables, forms) are solid (#111111). Glass is reserved for the sidebar shell and modal overlays only. No decorative blur on content.

## 3. Typography

**Display Font:** Cormorant Garamond (with Georgia fallback)
**Body Font:** Outfit (with system-ui fallback)

**Character:** The serif display font brings warmth and quiet authority to headings. Outfit is clean and highly legible at small sizes — ideal for data-dense interfaces. The pairing works on a contrast axis: ornamental vs. functional.

### Hierarchy
- **Display** (400, 28–36px, line-height 1.2): Page titles, section headings. Only in `.page-header h2` and dashboard hero elements.
- **Title** (500, 18–20px, line-height 1.3): Card headers, dialog titles, subsection headings.
- **Body** (400, 14px, line-height 1.6): All paragraph text, descriptions, content. Max 65–75ch line length.
- **Label** (500, 11px, uppercase, letter-spacing 0.08em): Table headers, form labels, stat labels, section markers.
- **Caption** (400, 12px, line-height 1.4): Helper text, timestamps, metadata.

### Named Rules

**The Body-Only Rule.** The display font (Cormorant Garamond) is used ONLY for page-level headings and the brand mark. Never on labels, buttons, form fields, data values, or inline text. All UI chrome uses Outfit.

**The Balance Rule.`h1`–`h3` elements use `text-wrap: balance` for even line lengths. Long prose uses `text-wrap: pretty` to reduce orphans.

## 4. Elevation

Flat by default. This system uses tonal layering (slightly lighter backgrounds for elevated surfaces) rather than shadows for depth. Shadows appear only as hover feedback on interactive elements.

### Shadow Vocabulary
- **Lift** (`0 4px 12px rgba(0,0,0,0.4)`): Hover state on cards and interactive surfaces. Indicates interactivity.
- **Drop** (`0 8px 32px rgba(0,0,0,0.5)`): Floating panels — sidebar, dropdowns, modals.
- **Gold glow** (`0 4px 24px rgba(200,169,110,0.15)`): Primary button hover. Warm, not neon.

### Named Rules

**The Flat-By-Default Rule.** Surfaces are flat at rest. Shadows appear only as a response to state (hover, elevation, focus). No ambient shadows on static content.

## 5. Components

### Buttons
- **Shape:** Gently curved (10px radius)
- **Primary:** Gold gradient (#c8a96e → #a88b5a), dark text (#050505), 600 weight, 10px 24px padding. Hover: subtle lift (-1px translateY) + gold glow shadow.
- **Ghost:** Elevated background (#1a1a1a), light border, white text. Hover: border tints gold, text turns gold.
- **Danger:** Coral background (#f87171), dark text. Used for destructive actions only.

### Cards / Containers
- **Shape:** Softly rounded (14px radius)
- **Background:** Solid carbon (#111111)
- **Border:** 1px line (rgba(255,255,255,0.06)), brightens on hover
- **Header:** Separated by border-bottom, padding 20px 24px
- **Body:** Padding 24px (or flush for tables)

### Inputs / Fields
- **Style:** Dark fill (#0d0d0d), 1px line border, 10px radius
- **Focus:** Border shifts to warm gold
- **Placeholder:** Ash (#555555) — must meet 4.5:1 contrast

### Tags / Chips
- **Shape:** Pill (20px radius), 11px font, 2px 10px padding
- **Variants:** Dark style with semantic color tinting (success-bg, warning-bg, danger-bg, info-bg)

### Navigation (Top Bar)
- **Primary bar (56px):** Horizontal nav pills (Dashboard, Vehicles, Rentals) centered in a full-width bar. Brand mark + name on left, user pill on right.
- **Secondary strip (38px, conditional):** Admin/staff items as smaller horizontal pills. Only renders for admin/staff roles.
- **Active state:** Gold text + muted gold background tint
- **Background:** Solid #0a0a0a with 1px bottom border

### Bottom Dock
- **Shape:** Floating pill (16px radius), fixed at bottom center
- **Background:** Elevated (#1a1a1a) with 1px border + drop shadow
- **Buttons:** Icon + expandable label on hover (max-width transition)
- **Purpose:** Quick-access actions (new rental, add vehicle, balance, profile)

### Tables
- **Header:** Uppercase label style (11px, 500 weight, ash color), subtle bg tint
- **Rows:** Transparent bg, hover lifts to rgba(255,255,255,0.03)
- **Borders:** Line color (rgba(255,255,255,0.06))

## 6. Do's and Don'ts

### Do:
- **Do** use warm gold (#c8a96e) only on primary actions, active selections, and key indicators — never decoration.
- **Do** keep content surfaces solid (#111111). Glass is for the sidebar shell and modal overlays only.
- **Do** use Cormorant Garamond only for page-level headings and the brand mark. All other text uses Outfit.
- **Do** verify text contrast: body text ≥4.5:1, large text ≥3:1 against its background.
- **Do** use `text-wrap: balance` on h1–h3 headings.
- **Do** include `prefers-reduced-motion: reduce` media query for all animations.
- **Do** use 150–250ms transitions on state changes. No decorative motion.
- **Do** use standard patterns (sidebar nav, data tables, form dialogs) — earned familiarity, not novelty.

### Don't:
- **Don't** use glassmorphism as a default surface treatment. Per PRODUCT.md: "Glass is reserved for the sidebar and modal overlays only — content surfaces are solid."
- **Don't** put the display font on labels, buttons, data values, or form fields. Per PRODUCT.md: "Display fonts on every label" is an anti-reference.
- **Don't** use neon accents, glowing borders, or futuristic type. Per PRODUCT.md: "This is a rental fleet tool, not a trading platform."
- **Don't** use gradient text (`background-clip: text`). Use solid color with weight/size for emphasis.
- **Don't** use colored side-stripe borders (border-left > 1px). Use full borders, background tints, or nothing.
- **Don't** animate layout properties (width, height, margin, padding). Animate transform and opacity only.
- **Don't** use the hero-metric template (big number + small label + gradient accent). It's a SaaS cliché.
- **Don't** gate content visibility on animation classes. Content must be visible by default; animations enhance, never reveal.
