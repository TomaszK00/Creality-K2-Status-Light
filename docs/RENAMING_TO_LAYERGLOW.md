# Renaming Creality K2 Status Light to LayerGlow

**Decision:** The public project brand is **LayerGlow**. The existing Creality K2 integration is preserved and remains the only documented firmware integration at present. Multi-printer support is a future goal, not an already released capability.

## Current state (23 October 2026)

- Repository: `TomaszK00/Creality-K2-Status-Light` (unchanged for OTA compatibility).
- Published OTA version: **2.15**.
- Archived, not promoted: **2.16**.
- Update manifest: `Software/update.json` (do not change until a firmware transition is tested).
- Existing `.ino` and `.bin` names are historical release artifacts and are intentionally retained.

## Why a direct GitHub rename is risky

The deployed v2.15 firmware contains hard-coded URLs:

- `https://raw.githubusercontent.com/TomaszK00/Creality-K2-Status-Light/main/Software/update.json`
- `https://api.github.com/repos/TomaszK00/Creality-K2-Status-Light/contents/Software?ref=main`
- `https://raw.githubusercontent.com/TomaszK00/Creality-K2-Status-Light/main/Software/` + OTA filename

GitHub redirects for repository web pages do **not** guarantee equivalent behaviour from the raw content host or every REST API request. Firmware may fail to detect or install updates after renaming. **Do not rely on redirects without checking with an already installed ESP32.**

## Safe migration checklist

1. Keep the repository and current OTA manifest in place while transitioning.
2. Update public documentation and branding to LayerGlow without changing previously shipped firmware binaries.
3. Build a **new firmware version**, changing the project display name and introducing a robust update-discovery strategy with tested old/new endpoint handling.
4. Confirm version checks, manifest parsing, TLS/HTTP redirects, SHA-256 checks and an actual OTA update on a real ESP32-C3. Ensure the firmware can still recover on network errors.
5. Offer a tested transitional update **at the old repository address** and confirm deployed v2.15 installations can install it successfully.
6. Only then rename the GitHub repository via **Settings → General → Repository name → Rename**, confirming `LayerGlow` is available.
7. Re-test `raw.githubusercontent.com`, GitHub API and the OTA binary URLs after the rename. If any old-firmware endpoint fails, retain a separate compatibility repository under the original name with the correct `Software/update.json` and binary. Be aware that creating a new repository with an old name overrides GitHub's usual redirects.
8. Update repository description, badges, README links, future file naming, GitHub Release notes and any published external links.
9. Keep historical source archives, release binaries, version numbers, checksums, and previously granted licenses unchanged.

## Naming guidelines

Use **LayerGlow** for the project and interface branding in future releases. Use **Creality K2 integration** for the currently documented printer protocol. Do not rename old firmware files purely for cosmetic reasons because the update manifest references exact names.

## Non-commercial license

Rebranding does not revoke rights granted in earlier MIT-licensed copies. New covered material follows the repository's Personal Home Use License, subject to third-party licenses and prior grants.
