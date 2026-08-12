# E-Ink Assistant

A macOS menu bar app for colour e-ink displays. Mark which of your displays are
e-ink panels, then give each one a saturation boost and an optional shadow-lift
mode for video.

Public APIs only. No permissions required.

```
./build.sh
open "E-Ink Assistant.app"
```

Ad-hoc signed rather than notarized, so the first launch may need
right-click > Open.

## What it does

### Saturation

Colour e-ink puts a colour filter array over a monochrome panel, which costs a
lot of gamut — everything looks washed out. This rewrites the display's ICC
profile so macOS sends more vivid signals to compensate.

It is applied by the system colour pipeline, so it covers the whole desktop on
that display — apps, video, fullscreen, every Space — and leaves other displays
alone.

**It persists.** The setting lives in the display profile, exactly like a Colour
Profile choice made in System Settings, so macOS re-applies it at login whether
or not this app is running.

Presets at 100% / 130% / 150% / 200%, or a slider up to 300%. The 100%
button removes the override and puts the factory profile back.

Un-marking a display as e-ink resets it fully: the tone curve is cleared and the
factory colour profile is restored, leaving the panel exactly as it was.

### Video Enhance

Colour e-ink has a very low contrast ratio, so dark detail collapses into an
undifferentiated mush. Video Enhance brightens **only the darkest part** of the
image and leaves mid-tones and highlights exactly as they were:

```
w(x)   = 1 - smoothstep(0, knee, x)
out(x) = (1 - w(x))*x + w(x)*x^gamma
```

Below the knee the curve lifts; at and above it, output is *exactly* identity.
Both endpoints are pinned, so pure black stays pure black and white stays white.

| level | knee | γ | lift at 0.05 | lift at 0.10 |
|---|---|---|---|---|
| Subtle | 0.25 | 0.75 | 2.0× | 1.5× |
| Medium | 0.35 | 0.60 | 3.2× | 2.2× |
| Strong | 0.45 | 0.45 | 5.1× | 3.2× |

**The trade-off, stated plainly:** this is a global tone curve. It cannot tell
dark video from dark text, so anything dark gets lighter — including body text.
Measured contrast cost:

| level | light-mode body text | dark mode (all text) |
|---|---|---|
| Subtle | −11% | −13% |
| Medium | −32% | −34% |
| Strong | −55% | −56% |

Dark mode suffers most because the *background* lifts too, which drags down
contrast for every font regardless of colour. At Medium, dark-mode secondary
text falls to 3.9:1 — below the WCAG AA threshold of 4.5:1.

Dark coloured text also desaturates, because the curve runs per channel: dark
channels get lifted while bright ones do not, narrowing the gap between them. A
navy heading drops from 0.69 saturation to 0.39 at Medium. Fully saturated
colours are unaffected.

So: **turn it on for video and photos, off for reading.** The app warns in the
UI whenever a level is active.

**It does not persist.** Video Enhance drives the display's gamma table, which
macOS clears on sleep and on display changes. The app re-applies it on wake and
reconfiguration, so it needs to keep running — unlike saturation.

## ToneLab

A tuning tool, if you want to find your own curve rather than use the presets.

```
open ToneLab.app
```

Live knee and γ sliders driving the gamma table directly, a curve plot, a
numeric read-out of the lift at each level, and dark step-wedge and ramp
patterns for judging tone separation and banding by eye.

Worth knowing: colour e-ink has few grey levels per channel, so an aggressive
curve can reveal posterization. The ramp patterns are there to catch that.

## Layout

```
Sources/Shared/           ICC profile generation, tone curve, display enumeration
Sources/EinkAssistant/    the menu bar app
Sources/ToneLab/          the curve tuning tool
build.sh                  builds both
```

## Related

The ICC saturation mechanism, and the investigation behind it — including what
does *not* work on modern macOS — is documented in
[mac-saturation](https://github.com/kiteretsu903/mac-saturation), which also has
a CLI for exporting profiles. The relevant source files are vendored here so
this repo stands alone.
