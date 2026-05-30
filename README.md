# BallisticCalc
Ballistic Calc for CardputerADV

A standalone ballistic trajectory calculator running on Cardputer ADV. Computes real-time bullet drop, wind drift, velocity, and energy at range using G1/G7 drag models.


## Features


- **19 Built-in Cartridges** — organized by caliber (up to 12 caliber groups), browse and select with a two-level caliber → load interface.
- **Full Trajectory Solver** — iterative numerical integration computing drop, windage, velocity, energy, and time-of-flight at each range step.
- **Range Card View** — tabular display of trajectory data with selectable step intervals (10 / 25 / 50 / 100 m), including MOA and MIL correction values.
- **Trajectory Graph** — visual drop/windage curve with scrolling offset, auto-scaled axes, and grid overlay.
- **Persistent Cartridge Profiles** — pre-loaded load data stored in flash (PROGMEM) for zero-NVAM startup.


## Notes

In parameter edit menu, you should use **fn+.** to input demical **.**

Arror keys always used as arror keys to avoid logic confusion.
