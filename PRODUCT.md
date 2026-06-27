# Product

## Register

product

## Users

Three roles use the system equally:

- **Admin** (fleet owner/manager): full access — manages fleet, finances, users, and reports. Primary tasks: reviewing revenue, approving operations, overseeing fleet health.
- **Staff** (office/yard workers): operational access — handles maintenance records, customer top-ups, day-to-day rental processing. Primary tasks: quick lookups, status updates, customer service.
- **Customer** (renters): self-service — views available vehicles, checks rental history, manages account balance. Primary tasks: browse, rent, check status.

All three log in regularly through the same frontend. The UI must serve power users (admin doing bulk operations) and casual users (customers checking balance) without favoring either.

## Product Purpose

OxyRent is a vehicle rental management system that handles the full lifecycle: fleet inventory, rental bookings, vehicle maintenance, billing and payments, and customer account management. It exists to replace manual tracking with a single operational dashboard where all roles see what they need.

Success means: an admin can glance at the dashboard and know fleet health; a staff member can process a rental in under 30 seconds; a customer can check their balance and history without calling anyone.

## Brand Personality

**Professional, restrained, efficient.**

The existing "Noir Elegance" direction is the foundation — near-black backgrounds with warm gold accent, serif display headings paired with a clean sans-serif body. The personality is quiet confidence, not flash. Think: a well-organized office at night, not a nightclub.

Three words: **precise, warm, silent.**

- Precise: every element earns its space. No decoration for decoration's sake.
- Warm: the gold accent and serif typeface soften what would otherwise be a cold dark UI.
- Silent: the interface disappears into the task. Users shouldn't notice the design — they should notice their work getting done.

## Anti-references

- **SaaS dashboard clichés**: cream/beige backgrounds, gradient hero sections, identical stat-card grids with icon + number + label. The "Landing page SaaS starter kit" aesthetic.
- **Over-designed admin panels**: heavy sidebar branding, animated page transitions, display fonts on every label, decorative illustrations.
- **Glassmorphism as default**: translucent blurred cards everywhere. Glass is reserved for the sidebar and modal overlays only — content surfaces are solid.
- **Crypto/fintech dark mode**: neon accents, glowing borders, futuristic type. This is a rental fleet tool, not a trading platform.

## Design Principles

1. **Earned familiarity.** Use standard patterns (sidebar nav, data tables, form dialogs) and make them precise, not weird. A user fluent in Linear or Stripe should feel at home immediately.
2. **One accent, one job.** Warm gold (#c8a96e) is for primary actions, active selections, and key indicators only — never decoration. Content surfaces stay neutral.
3. **Dense when it matters, spacious when it helps.** Data tables pack information tightly. Dashboards breathe. Forms are comfortable. Spacing serves the content, not a grid system.
4. **Motion for state, not show.** 150–250ms transitions on state changes, loading feedback, and micro-interactions. No page-load choreography, no decorative reveals.
5. **The interface is the last resort.** Every pixel should help the user complete a task or understand a situation. If it doesn't do either, remove it.

## Accessibility & Inclusion

- WCAG 2.1 AA compliance target for all interactive elements.
- All text must meet 4.5:1 contrast ratio against its background; large text 3:1.
- `prefers-reduced-motion` media query required on all animations.
- Semantic HTML: proper heading hierarchy, form labels, table headers, ARIA attributes where needed.
- Keyboard navigation for all interactive elements (sidebar, dropdowns, modals, tables).
