# Release and OTA checklist

The active OTA manifest is at [`Software/update.json`](../Software/update.json) and currently points to **v2.15**. Archived **v2.16** is *not yet promoted*. **Keep the manifest format and path unchanged unless the updater firmware is updated too.**

## Preparing a new version

1. Build and test firmware on the supported ESP32 hardware.
2. Export the OTA-compatible `.bin` using the **same compatible partition and firmware layout** as existing deployed devices.
3. Name it consistently, e.g. `K2_Status_Light_v2_17_OTA.bin`.
4. Upload the binary to `Software/` *before* updating the manifest.
5. Compute the actual binary byte size and SHA-256 checksum.
6. Update `Software/update.json` with the new version, exact filename, byte size and lowercase SHA-256.
7. Verify the binary URL, checksum, update detection and actual OTA installation before announcing the release.
8. Update [CHANGELOG.md](../CHANGELOG.md); optionally create a GitHub Release with notes and checksums.

## Manifest format

```json
{
  "version": "2.15",
  "file": "K2_Status_Light_v2_15_OTA.bin",
  "size": 1668576,
  "sha256": "9af27684f46872ca9051091aac6c4dd59a6d650b49f8d904e88271d77eebd9b5"
}
```

This is the manifest **as currently published**, not a proof that the binary was validated. Storing a higher version in `Software/archive/` does not activate it. Before publishing a new version to `Software/`, review the associated source code and test OTA on your hardware. **Do not change the manifest to a version that has not been uploaded and tested.** Keep older known-good firmware files available for recovery where appropriate.
