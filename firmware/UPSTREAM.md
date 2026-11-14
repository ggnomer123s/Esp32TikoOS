# Upstream provenance

System base: https://github.com/Sor3nt/Flipper-Zero-ESP32-Port
Pinned commit: e6ec11972a8e025d7ab686a6ef3016cf52360834

Furi/FreeRTOS, GUI/Input, board HAL, Storage, Loader/Desktop, BT and Wi-Fi services
are reused from that revision. Existing author/copyright notices are retained.
TikoOS apps and integration changes are maintained separately in components/tiko.
The upstream does not include a repository-wide LICENSE at this revision;
component-specific notices remain in their files. No ownership over upstream
code is claimed. This is a source integration, not an official Sor3nt release.

TikoOS target selects core/system apps, connectivity, archive and study/tools.
Raw Wi-Fi driver patching, upstream OTA, attack apps and third-party game bundles
are not activated by the TikoOS build. The original repository is the reference
for its complete catalogue. No runtime update from its release server is enabled.
