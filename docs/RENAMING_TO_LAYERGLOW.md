# LayerGlow naming and OTA migration

The public project and interface name is **LayerGlow**. The repository is [TomaszK00/LayerGlow](https://github.com/TomaszK00/LayerGlow). The current test candidate is **v2.36**; `Software/update.json` points to one image, `LayerGlow_v2_36_OTA.bin`. Original older K2 binaries remain in the archive, while duplicate K2-named aliases for v2.27–v2.31 have been removed.

The previous repository name was `TomaszK00/Creality-K2-Status-Light`. Firmware already installed on a device may contain that old URL or expect a `K2_Status_Light_..._OTA.bin` filename. In that case, use the web panel to upload the v2.36 OTA image once; subsequent updates use the LayerGlow URL and filename. Whether a particular old installation follows GitHub redirects must be checked on the device.

The historical v2.15 firmware contained these hard-coded endpoints:

- `https://raw.githubusercontent.com/TomaszK00/Creality-K2-Status-Light/main/Software/update.json`
- `https://api.github.com/repos/TomaszK00/Creality-K2-Status-Light/contents/Software?ref=main`
- `https://raw.githubusercontent.com/TomaszK00/Creality-K2-Status-Light/main/Software/` + OTA filename

Repository page redirects do not establish that every API and raw-content request will work for all deployed firmware. The new manifest and compiled image have been checked by the publishing workflow; physical OTA and printer behaviour remain to be tested.

Keep **LayerGlow** as the application name in every language and name future OTA files `LayerGlow_v<version>_OTA.bin`. Preserve original historical artifacts and their licensing. Rebranding does not revoke rights granted in earlier MIT-licensed copies. New covered material follows the repository's Personal Home Use License, subject to third-party licenses and prior grants.
