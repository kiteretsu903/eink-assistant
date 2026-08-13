# Technical Notes

Background for anyone modifying this app, or investigating the same problem.
The [README](README.md) covers using it; this covers why it works the way it
does. Every number here was measured on a Bigme B251 Pro under macOS 27.

## Why three mechanisms

| feature | mechanism | persists |
|---|---|---|
| Saturation | ICC display profile | yes, until removed |
| Tone curves | display gamma table | no |
| Reduce Shaking | I/O Registry property | yes (Apple Silicon only) |

This split is deliberate, not an accident of history.

**Saturation cannot use the gamma table.** That table is a per-channel 1D
lookup: `out_R = f(in_R)`. Saturation is cross-channel, `out_R = a*R + b*G +
c*B`, so it needs the 3x3 matrix an ICC profile carries. No 1D LUT can express
it at any resolution.

**Tone curves could move into the profile, but are not.** A profile TRC only
affects content being converted into display space, so anything that bypasses
colour management would miss it, and HDR content and clipping boundaries behave
differently. The gamma table applies unconditionally at scanout, which is the
more predictable behaviour for a tone adjustment.

The cost of the split is that the gamma table is volatile: macOS clears it on
sleep, on display reconfiguration, and as a side effect of writing a colour
profile. That is why saturation is applied before curves, why curves are
re-asserted shortly after a profile write, and why the app has to keep running.

### Saturation

Colour e-ink puts a colour filter array over a monochrome panel, which costs a
lot of gamut, so everything looks washed out. This rewrites the display's ICC
profile so macOS sends more vivid signals to compensate.

It is applied by the system colour pipeline, so it covers the whole desktop on
that display: apps, video, fullscreen, every Space. Other displays are left
alone.

**Quitting resets it.** Every setting in the app is app-managed: quitting (or
logging out) returns each display to its original state, and launching puts
your stored settings back. So the app needs to be running for anything here to
apply. Launch at Login is the intended setup.

The quit-time cleanup only removes profiles this app installed. A calibration
profile you set yourself is recognised as foreign and left untouched.

Presets at 100% / 130% / 150% / 200%, or a slider up to 300%. The 100%
button removes the override and puts the factory profile back.

Un-marking a display as e-ink resets it fully: the tone curve is cleared and the
factory colour profile is restored, leaving the panel exactly as it was.

### Text Contrast

The mirror of Video Enhance: γ above 1 darkens the low end while pinning white,
pushing text toward the panel's floor. For reading.

| level | knee | γ | black point | body | secondary | tertiary |
|---|---|---|---|---|---|---|
| Medium | 0.55 | 1.70 | 0 | 19.1:1 | 5.1:1 | 2.8:1 |
| Strong | 0.65 | 2.10 | 0 | 19.9:1 | 6.0:1 | 2.9:1 |
| Sharp | 0.90 | 3.00 | 0 | 20.5:1 | 9.6:1 | 4.0:1 |
| Solid | 0.90 | 3.00 | 0.16 | 21.0:1 | 15.5:1 | 6.2:1 |

(contrast against white, unadjusted is 15.1 / 4.8 / 2.8)

Body text saturates quickly. The real gains are in **secondary and faint
text**, which is where a low-contrast panel struggles. `Solid` additionally
crushes antialiased glyph edges to black, making stems solid at the cost of
harder edges.

ReadingLab also exposes a **white point**, the counterpart to the black point.
In practice it turned out **not to be worth promoting to a level**: with macOS
font smoothing doing stem darkening, glyph fringe pixels sit mostly on the dark
side of the edge, which the black point already handles, so there is little
light halo left to crush. It only helps with grey text on a dark background,
which is a layout to avoid on a reflective panel anyway. Kept in the lab for
completeness.

That is roughly the ceiling for this approach. A display transform only ever
sees one pixel at a time, so real sharpening or outlining (which need
neighbouring pixels) would require capturing and re-rendering the screen.

White stays exactly 1.000 at every level, so the page never greys.

**Advanced** replaces both preset pickers with direct control of the curve:
knee, gamma, black point and white point, applied live and stored per display.
Gamma below 1 lifts shadows, above 1 darkens them, so one set of sliders covers
both directions.

Advanced also has **five slots** for saving curves. Click an empty slot to
store the current curve, a saved one to apply it; right-click to rename,
overwrite or clear. Slots are global rather than per display, so a curve tuned
on one panel can be applied to another.

**Text Contrast and Video Enhance are mutually exclusive.** They drive the same
hardware gamma table and pull in opposite directions, so turning one on turns
the other off.

### Reduce Shaking

macOS dithers display output to smooth gradients. An LCD refreshes the pattern
away; a colour e-ink panel holds each pixel, so the dither shows up as a
constant shimmer. Turning it off makes the image sit still.

This works by setting `enableDither` on the display's `IOMobileFramebufferAP`
I/O Registry entry. No root or special permissions needed, and the toggle is
hidden on hardware where no framebuffer can be matched rather than sitting there
doing nothing.

**Per display**, unlike Stillcolor, which applies to all displays or to
embedded/external as a group. Each framebuffer is matched to a
`CGDirectDisplayID` by product id and vendor (decoding the EISA manufacturer
code, so `CPO` and `3599` are recognised as the same vendor), which lets an
e-ink panel run without dithering while the built-in screen keeps it.

Dithering is hardware state that outlives the process, so it is re-asserted
after display changes and turned back on when the app quits.

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
dark video from dark text, so anything dark gets lighter, including body text.
Measured contrast cost:

| level | light-mode body text | dark mode (all text) |
|---|---|---|
| Subtle | −11% | −13% |
| Medium | −32% | −34% |
| Strong | −55% | −56% |

Dark mode suffers most because the *background* lifts too, which drags down
contrast for every font regardless of colour. At Medium, dark-mode secondary
text falls to 3.9:1, below the WCAG AA threshold of 4.5:1.

Dark coloured text also desaturates, because the curve runs per channel: dark
channels get lifted while bright ones do not, narrowing the gap between them. A
navy heading drops from 0.69 saturation to 0.39 at Medium. Fully saturated
colours are unaffected.

So: **turn it on for video and photos, off for reading.** The app warns in the
UI whenever a level is active.

Related: prefer **Light mode** on colour e-ink generally. The panel is
reflective, so white is its natural resting state; dark backgrounds cost
contrast and are where Video Enhance does the most damage to text.

**It does not persist.** Video Enhance drives the display's gamma table, which
macOS clears on sleep and on display changes. The app re-applies it on wake and
reconfiguration, so it needs to keep running.

Quitting the app clears the tone curve from every display, and so does logging
out or shutting down, as it does for saturation.

## Tuning labs

Two tools for finding your own curve rather than using the presets. Both drive
the gamma table live, so nothing persists: quitting restores the display.

**ToneLab** (`open ToneLab.app`): shadow lift, for video. Knee and γ sliders,
a curve plot, the lift factor at each level, and dark step-wedge and ramp
patterns for judging tone separation and banding by eye.

**ReadingLab** (`open ReadingLab.app`): text contrast, the mirror image. Uses
γ **above** 1 to darken the low end while pinning white, pushing text toward the
panel's floor. Shows live WCAG contrast ratios before and after, and renders real
text specimens at the greys macOS actually uses (body, secondary, tertiary,
headings, regular and bold) on a white page.

Reading mode is **tone-only**: it does not touch saturation, so whatever the
colour profile is doing stays as it is.

| preset | knee | γ | body text | secondary |
|---|---|---|---|---|
| off | 0.55 | 1.00 | 15.1:1 | 4.8:1 |
| light | 0.45 | 1.35 | 17.7:1 | 4.8:1 |
| medium | 0.55 | 1.70 | 19.1:1 | 5.1:1 |
| strong | 0.65 | 2.10 | 19.9:1 | 6.0:1 |

These are *signal* contrast ratios. A colour e-ink panel's physical ceiling is
far lower, so the perceived gain is bounded by the panel, not by the curve.

Worth knowing: colour e-ink has few grey levels per channel, so an aggressive
curve can reveal posterization. The ramp patterns are there to catch that.

## Layout

```
Sources/Shared/           ICC profile generation, tone curve, display enumeration
Sources/EinkAssistant/    the menu bar app
Sources/ToneLab/          the curve tuning tool
build.sh                  builds both
```
