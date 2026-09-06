# macOS localization E2E — 2026-09-06

**PASS for the exercised live localization workflows.** Tested with Computer Use
against the production app at `E-Ink Assistant.app` on `dev/multi-lingual`, using
real connected displays and active controls. This was not the fictional-data
preview. No source changes were needed during this E2E run.

The initially running process exposed the previous four-language picker. It was
quit normally and the current on-disk build was reopened; its native picker
exposed all 80 locales plus System.

| Workflow | Observed result |
|---|---|
| All 80 language choices | Selected every locale through the actual native menu. Each selected autonym appeared in the picker and the Quit label refreshed to its translation. No switching failures. |
| Tuning values during switching | Saturation stayed at 200% and RGB stayed at 100%/100%/100% across all 80 switches. |
| RTL | Arabic live controls and explanatory content mirrored correctly; numeric values and the curve retained their intended direction. Hebrew, Persian and Urdu also passed switching. |
| Expanded explanation | Opened English help, switched to German and Arabic, and verified the expanded prose translated without a restart. |
| RGB disclosure | Expanded the German RGB controls; translated Red/Green/Blue labels and 100% values appeared. |
| Options menu | Opened German Options and verified translated settings/helper entries, then dismissed it. No helper reinstall was requested. |
| Live saturation control | Decremented the real control from 200% to 185%, then incremented to 200%. Both changes appeared in the live UI. Restored the original Anime preset selection afterward. |
| Restart persistence | Quit with Arabic selected and reopened the app. Arabic labels, selected language, and 200% saturation persisted. |
| System language restoration | Selected System and verified the English interface returned. |
| Settings restoration | Restored the initially displayed transparency/motion Off state; automatic following remained On. All saved app preference values matched the pre-test backup after restoring the Anime preset. |

The app's existing automatic-follow behavior changed transparency/motion from
Off to On at launch. It was returned to Off through the app, and the completed
helper state was verified. The original selected display, other display
selection states, RGB values, contrast/video/advanced modes, and login setting
were preserved. Help/RGB disclosures returned to their original collapsed state
after reopening. The current multilingual build remains running.

Evidence includes the Computer Use accessibility observations and screenshots in
the task transcript. Before/after preference snapshots are retained locally in
ignored `artifacts/localization-e2e/`; their parsed dictionaries compare equal.
Computer Use briefly timed out during process exit/launch transitions; retrying
resolved the app and verified the resulting UI state.

## Coverage limits

This run verifies live localization switching, representative menus/disclosures,
restart persistence, and a reversible tuning-control interaction on this Mac.
It does not measure physical display color output or test every hardware-control
combination, admin-password operations, helper installation, every menu in every
language, other macOS versions, or Windows. Native-speaker review is not required.

## Full-width selector follow-up

After the user's layout report, replaced wrapping option rows with single-line
rows that distribute spare width across every button. The panel measures the
widest localized row with bold labels and padding: English stays at 540 points,
Russian grows to 707, and the largest measured locale (isiZulu) needs 850.
All 80 catalogs were measured. Bubble resize handling now repositions the window
and its arrow within the screen when the language changes.

The rebuilt production app was checked through Computer Use in English, Russian,
isiZulu, and Arabic. Screenshots confirm that saturation, text contrast, and video
selectors fill the available row, remain on one line, and stay visible. Growing
from English to isiZulu and shrinking back also preserved correct screen placement.
System language and the original transparency/motion Off setting were restored.
The final production build passed without compiler warnings.
