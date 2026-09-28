# ETS2 Ford Trucks F-MAX Brand Bridge Plugin (x64)

[![Build and Release Plugin](https://github.com/nisodex/ets2-ford-fmax-plugin/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/nisodex/ets2-ford-fmax-plugin/actions/workflows/build-and-release.yml)
[![Steam Workshop](https://img.shields.io/badge/Steam%20Workshop-3459583210-blue.svg)](https://steamcommunity.com/sharedfiles/filedetails/?id=3459583210)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()
[![ETS2 Compatibility](https://img.shields.io/badge/ETS2-1.50%20--%201.61%2B-orange.svg)]()

A native 64-bit SCS Telemetry SDK plugin for **Euro Truck Simulator 2** that dynamically unlocks and permanently preserves the **Ford brand filter button** in the Quick Jobs (*Trabajos Rápidos*) market menu.

Companion plugin for the [Ford Trucks F-MAX Steam Workshop Mod](https://steamcommunity.com/sharedfiles/filedetails/?id=3459583210).

---

## The Engine Problem

In Euro Truck Simulator 2, the game engine runs an internal dealership verification routine (`verify_dealer_brands` at RVA `0x88F77A`) whenever a map is loaded (`/map/europe.mbd`). 

1. **Map Dealer Scan:** The engine scans all physical map items (`.base` sector files) for dealership prefabs.
2. **Prune Routine (`0x88F81B`):** If a brand registered in `/def/vehicle/truck_dealer/` does not possess at least one physical dealership placed in the loaded map sectors, the engine explicitly invokes `prism::array_t::erase` to delete that brand from the active memory vector (`[main_obj + 0x4038]`).
3. **Quick Jobs UI Impact (`0x1412DE720`):** When the player opens the Quick Jobs market, the UI loops strictly over `[main_obj + 0x4038]` to generate the brand filter buttons on the left sidebar. Because Ford has no physical dealership in the base Europe map, the engine deletes the Ford token (`0xC5A86`) on every startup, preventing the button from ever appearing.

Modifying map sector files directly inside a truck mod is strictly forbidden by the **SCS Workshop Uploader** (which rejects `.base` and `.aux` files), and doing so in standalone mods risks major conflicts with map expansions (ProMods, RoExtended, etc.).

---

## How This Plugin Works

This plugin resolves the issue dynamically from memory without altering any game map files:

1. **Native SCS Plugin Integration:** Automatically loaded by `eurotrucks2.exe` at game startup via the official SCS Telemetry plugin loader (`bin/win_x64/plugins/`).
2. **Dynamic Byte-Level Patching:** Uses pattern scanning to find the `verify_dealer_brands` routine in `.text` and dynamically converts the conditional jump instruction at `0x88F797` (`0F 84` -> `E9 ... 90`) into an unconditional jump to the cleanup routine (`0x88F842`), completely bypassing the `vector::erase` loop in RAM.
3. **Active Memory Guardian:** A lightweight background thread continuously monitors the economy manager (`[base + 0x36AE6D8] + 0x4038`), ensuring the Ford brand token (`0xC5A86`) remains persistently active across savegame loads and fast-travel transitions.
4. **Diagnostic Logging:** Logs all runtime events and patch verification to `ford_fmax_plugin.log`.

---

## Installation

1. Download `ford_fmax_plugin.dll` from the latest [GitHub Release](https://github.com/nisodex/ets2-ford-fmax-plugin/releases/latest).
2. Copy `ford_fmax_plugin.dll` into your ETS2 plugins directory:
   ```text
   C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\
   ```
   *(If the `plugins` folder does not exist, create it).*
3. Launch Euro Truck Simulator 2 from Steam.
4. When the standard official SCS Software SDK notification appears (*"Request to use advanced SDK features detected"*), click **OK**.
5. Open **Quick Jobs** in-game: the official Ford oval brand filter button is now permanently visible on the left sidebar and filters jobs exclusively for the Ford F-MAX.

---

## Building from Source

### Requirements
* Windows 10 / 11 (64-bit)
* Visual Studio 2022 / 2026 with C++ desktop tools (MSVC x64) or CMake 3.20+

### Option A: 1-Click PowerShell Build (Recommended)
```powershell
.\scripts\build.ps1 -Install
```
The `-Install` switch automatically compiles the 64-bit DLL and copies it directly into your ETS2 installation directory.

### Option B: CMake Build
```bash
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

---

## Compatibility

* **Game:** Euro Truck Simulator 2 (64-bit Windows)
* **Game Versions:** Tested and fully compatible with **1.50 through 1.61+**
* **Truck Mod:** Compatible with [Ford Trucks F-MAX](https://steamcommunity.com/sharedfiles/filedetails/?id=3459583210) (and any other standalone mod utilizing the `ford` dealer token)
* **Map Mods:** 100% compatible with all map mods (Vanilla, ProMods, TR Extended, RoExtended) since it does not touch map geometry.

---

## Related Projects

* **Steam Workshop Mod:** [Ford Trucks F-MAX (Standalone)](https://steamcommunity.com/sharedfiles/filedetails/?id=3459583210)
* **Official Website & Templates:** [SimülasyonTÜRK F-MAX Portal](https://fmax.simulasyonturk.com/?lang=en)

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
