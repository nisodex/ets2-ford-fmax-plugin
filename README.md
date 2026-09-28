# ETS2 Ford Trucks F-MAX Brand Bridge Plugin (x64)

[![Build and Release Plugin](https://github.com/nisodex/ets2-ford-fmax-plugin/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/nisodex/ets2-ford-fmax-plugin/actions/workflows/build-and-release.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()
[![ETS2 Compatibility](https://img.shields.io/badge/ETS2-1.50%20--%201.61%2B-green.svg)]()

Native 64-bit SCS Telemetry SDK plugin for **Euro Truck Simulator 2** that dynamically unlocks and preserves the **Ford brand filter button** in the Quick Jobs (*Trabajos Rápidos*) menu.

---

## 🇪🇸 Descripción en Español

En Euro Truck Simulator 2, el motor del juego ejecuta una rutina interna (`verify_dealer_brands` en RVA `0x88F77A`) al cargar cualquier mapa (`/map/europe.mbd`). Esta función escanea los concesionarios físicos del mapa y, si una marca no tiene un concesionario físico en los sectores cargados, ejecuta `vector::erase` eliminando la marca del vector de memoria activo (`[main_obj + 0x4038]`).

Como Ford es una marca personalizada y el mapa base no contiene concesionarios de Ford, el juego elimina a Ford en cada arranque, haciendo que el botón de la marca nunca se dibuje en la barra lateral izquierda de *Trabajos Rápidos*.

**Este plugin resuelve el problema de forma permanente:**
1. Se carga de forma nativa al arrancar el juego mediante la API oficial de plugins de SCS Software (`bin/win_x64/plugins/`).
2. Utiliza escaneo de firmas de memoria (*pattern scanning*) para localizar la rutina de poda y neutraliza la instrucción de borrado dinámicamente en RAM.
3. Mantiene un guardián en segundo plano que asegura que el token de la marca (`0xC5A86 = 'ford'`) permanezca activo durante todo el juego.
4. Permite mantener el mod principal (`ford_fmax.scs`) 100% limpio y compatible con **Steam Workshop**, sin necesidad de modificar sectores de mapa ni alterar archivos de ciudades.

### Instalación
1. Descarga `ford_fmax_plugin.dll` desde la sección [Releases](https://github.com/nisodex/ets2-ford-fmax-plugin/releases).
2. Copia `ford_fmax_plugin.dll` en la carpeta de plugins de tu instalación de ETS2:
   ```text
   C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\
   ```
   *(Si la carpeta `plugins` no existe, créala).*
3. Inicia el juego desde Steam. Al arrancar aparecerá el aviso oficial de SCS informando del uso de características SDK. Pulsa **OK**.
4. ¡Listo! El botón con el óvalo oficial de Ford estará activo permanentemente en Trabajos Rápidos.

---

## 🇬🇧 English Description

In Euro Truck Simulator 2, the engine runs an internal pruning routine (`verify_dealer_brands` at RVA `0x88F77A`) every time a map is loaded. It scans the map sectors for physical dealerships; if a brand has no physical dealership on the active map, the engine invokes `vector::erase` to delete that brand from the active brand vector (`[main_obj + 0x4038]`).

Because Ford is a standalone modded brand without vanilla map dealerships, the engine deletes Ford on every startup, preventing the Ford filter button from appearing on the left sidebar of the Quick Jobs market.

**This plugin permanently solves the issue:**
1. Loads natively on game startup via SCS Software's official plugin system (`bin/win_x64/plugins/`).
2. Scans memory patterns to locate the pruning check and patches the conditional jump in RAM (`0x88F797`), bypassing the deletion loop.
3. Runs a lightweight guardian thread ensuring the brand token (`0xC5A86 = 'ford'`) remains active across profile reload cycles.
4. Keeps the main truck mod (`ford_fmax.scs`) 100% clean and compliant with **Steam Workshop** rules (no map sector edits required).

### Installation
1. Download `ford_fmax_plugin.dll` from [Releases](https://github.com/nisodex/ets2-ford-fmax-plugin/releases).
2. Place `ford_fmax_plugin.dll` in your ETS2 binary plugins directory:
   ```text
   C:\Program Files (x86)\Steam\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\
   ```
   *(Create the `plugins` folder if it doesn't already exist).*
3. Launch ETS2. When the standard SCS SDK confirmation dialog appears, click **OK**.
4. The Ford brand filter button will now remain permanently available in Quick Jobs.

---

## 🛠️ Compilación / Building from Source

### Requisitos / Requirements
- Windows 10/11 x64
- Visual Studio 2022 / 2026 con herramientas C++ (MSVC x64) o CMake 3.20+

### Con PowerShell (Recomendado)
```powershell
.\scripts\build.ps1 -Install
```
El parámetro `-Install` compila la DLL e instala el archivo directamente en tu directorio de ETS2.

### Con CMake
```bash
mkdir build && cd build
cmake .. -A x64
cmake --build . --config Release
```

---

## 📄 Licencia / License

Distribuido bajo la licencia MIT. Consulta [LICENSE](LICENSE) para más detalles.
