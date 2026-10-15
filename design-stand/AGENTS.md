# Prototype Instructions

Run the local server yourself and open the preview in the browser available to this environment. Do not give the user server-start instructions when you can run it.

Before making substantial visual changes, use the Product Design plugin's `get-context` skill when the visual source is unclear or no longer matches the current goal. When the user gives durable prototype-specific design feedback, preferences, or decisions, record them in `AGENTS.md`.

When implementing from a selected generated mock, treat that image as the source of truth for layout, component anatomy, density, spacing, color, typography, visible content, and hierarchy.

Build app UI in `src/`. Keep `.openai/hosting.json`, `worker/index.js`, `scripts/prepare-sites-build.mjs`, and `tests/sites-worker.test.mjs` intact so the same local prototype can be handed to Sites. Before a Sites handoff, run `npm run build` and `npm run test:sites`; the build must leave `dist/client/index.html`, `dist/server/index.js`, and `dist/.openai/hosting.json`.

# Silver-K decisions

This is an isolated UI-only stand, not the native plugin. Do not add DSP, ROM loading, audio/MIDI, or modify native sources while refining this stand.

Use `public/reference.png` as the original layout reference, with the user's latest browser comments taking precedence. This is a UI-only stand; native integration is a separate task.

The three oscillators share one outer panel, separated by internal horizontal lines. The two main envelopes also share one outer panel with a separator and one shared Extra. FM Env has no Extra. All actual outer modules must have a 6 px gutter and must not intersect. Use a 1122 px frame and a 1026 px body: OSC/Mixer height 730 px, right main section 790 px; lower master/Program height 290 px, Effects height 230 px. Bottom columns: 428 / 362 / 612 px. Effects must match Filter width exactly.

All primary knobs have an 80 px logical diameter. Mixer first/last knobs align with OSC 1/3 Pitch Bend and all five have equal spacing. OSC waveform/octave menus and three PWM LEDs remain on the face; no PW Position/LFO2 PWM/Env PWM duplicates in Extra. Sync/FM and the internal lines sit halfway between the preceding knob readouts and the following oscillator header (21 px above the row boundary), outside inert faces, visible and clickable with every Extra open. Extra faces leave 39 px clearance below OSC 1/2 and 18 px above OSC 2/3.

KS: LFO selection LEDs beside the heading, waveform at bottom left, Sync Rate at bottom right. K: two side-by-side LFO columns, each with Rate above Delay and Wave/Sync below; no selector. Expose only model-supported controls. Delay Sync and rare LFO modes belong in Extra.

Lower left is one Volume / Arp / Glide module: Glide and Tempo on the upper row, Volume and Gate Length on the lower row. Volume at the bottom-left has a blue header-style gradient behind its knob and readout, all full-size knobs. Sync, Pattern, Octaves, KS Rhythm and LED modes sit beside the knobs. Rhythm is a menu with Off / 1–32 only for KS. Remaining Arp/Glide settings stay in Extra. Voice options belong in the Program Options button, never in Volume Extra. Program retains demo patch labels and functional Previous/Next buttons below LCD.

Effects uses six physical knob positions, the same grid as Filter. Chorus exposes Centre and Mod Wheel plus Type / Sync / Sync Start menus evenly spaced on the lower row, so it has no remaining Extra. Do not invent DSP controls for effects with fewer parameters: unused knob positions stay dimmed.

All additional settings must fit without scrolling/clipping. Arp/Options continuous settings use 28 px trim knobs with readouts on the right; other Extra panels use horizontal sliders with readouts. Keep menus for discrete choices. Small knob artwork: `public/assets/mini-knob.png`, a finger-turnable black ribbed cap with a white pointer, Volca/Eurorack style; do not use the previous blue screwdriver trim-pot look.

KS lower panels are light; K lower panels are dark. Toolbar scale/grid/reference overlay stay outside the skin. No audio/ROM/MIDI, no native-source changes during stand refinement.

# Compact stand study — 2026-10-14

The user requests size reduction on the stand only. Do not apply this compact draft to the native app/plugin. Preserve the preceding 1440 × 1122 layout as the Original option. The default Compact option is 1280 × 1000, about 21% less canvas area; this is actual re-layout, not only a transform. Primary knobs are uniformly 72 px; labels stay 11–12 px and outer module gutters remain 6 px. Compact columns: 552 / 136 / 280 / 268 px; bottom columns: 380 / 308 / 554 px. Filter and Effects retain the same six-column width. Compact OSC/Mixer height: 644 px, oscillator rows 224 / 224 / 194 px, mixer endpoints aligned at 84 and 532 px. Keep the agreed panel topology, shared frames, Sync/FM seams, vertical K LFO pairs and full Chorus controls. Arp/Program height is 254 px, Effects 198 px. All Extra settings fit without scroll or overflow; compact OSC modulation labels can omit the redundant Oscillator word because the containing panel already identifies the oscillator. Preserve full labels in tooltips. The fader cap position must adapt to the track height. Switching the layout variant must retain local control values.

# Performance mixer draft — 2026-10-14

Use the stand's Perf tab for the user's requested preview before native integration. Performance is a full page in the existing skin with four equal vertical channel strips, all visible together. Each strip exposes Part On/Off, a demo preset selector with Previous/Next, MIDI channel, output routing, low/high keyboard notes with a range indicator, full-size Transpose/Fine knobs, velocity curve (raw 0–15) and a long Volume fader. Keep the actual KS parameter ranges from `src/catalog.json`; do not invent pan, meters or additional DSP controls. Six-pixel outer gutters, aligned rows, original finishes and existing control assets remain consistent with Main. Edit sound selects that part and opens Main. Part values are shared with Main but independently keyed by model and part. In K-Station, four-part controls are unavailable. Performance fit mode fits the entire frame to the viewport; explicit zoom remains available. This draft has local demo state only and must not update native plugin sources.

# Keyboard-range MIDI Learn — 2026-10-14

The user explicitly authorizes MIDI note learning on the stand, overriding the earlier blanket no-MIDI constraint only for read-only MIDI input. Low/High Note open a small editor inside the channel with manual note selection and an explicit Learn MIDI button. Learn captures the next positive-velocity Note On from any input/channel once, then stops; Note Off, zero-velocity Note On, CC/pedal and clock do not change notes. Only one target may learn at once. Cancel on Escape, outside click, manual selection, page/model change, hidden browser tab and unmount. Request Web MIDI only after Learn is clicked, without SysEx, audio generation or MIDI output. Show waiting, cancellation, learned-note and permission/unsupported errors, retaining manual selection. Use exact raw notes 0–127 with C4 = 60 labels; if the chosen edge crosses the other one, move the opposite edge to that same note rather than replacing the played note. Keep native app/plugin untouched.

# Backlit button study — 2026-10-15

The user requests backlit physical buttons in place of separate round selector lamps, inspired by original Novation K-Station and KS Rack panels. Preview on the stand first. Use the low, horizontally oval charcoal cap from those panels with fixed upper-left lighting; the selected cap itself glows ruby red. Assets: `public/assets/novation-button-off.png` and `novation-button-on.png`, generated matching off/on states. Preserve adjacent readable labels, existing selection/toggle behavior, six-pixel panel grid and compact geometry. Apply the shared cap to PWM, filter type/slope, LFO selection, Sync/FM, Arp modes and Performance/part power. Existing rectangular navigation/Extra buttons are outside this study. Do not change native sources as part of this preview.

Approved 2026-10-15: FM is a rectangular Extra-style button, not an oval illuminated selector. Performance strips have plain Part 1–4 headings without numbered selection squares. Select a part by clicking anywhere on its strip, including its controls; keep the upper global 1–4 buttons. Keyboard Enter/Space on a focused strip also selects it.
