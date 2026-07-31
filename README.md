# BallisticCalc
Ballistic Calc for CardputerADV

A standalone ballistic trajectory calculator running on Cardputer ADV. Computes real-time bullet drop, wind drift, velocity, and energy at range.


# Changelog

## 2.0.1
- Added several new weapons.
- Rewrote the menu system.
- Replaced the core calculation formula with a new one (based on a new reference model).
- Integrated atmospheric density into the calculations.

## 2.0.2
- Since we are building a ballistic calculator, accuracy is the top priority.
  - The ESP32's FPU only supports single-precision floating-point operations. Our current solution is to increase the computational bit width.
  - Consequently, the following aspects have been significantly improved:
    - Atmospheric model precision
    - Speed of sound calculation
    - Gravity calculation
    - Core algorithm for position, velocity, and acceleration
    - Kinetic energy and penetration calculations
  - Effective precision has been increased from 7 to 15 significant digits.
  - **Drawback:** The range table, which previously required almost no waiting time, now takes a bit longer to generate.

- Rewrote the range table display logic.

- Future versions may add multi-language support. For now, the option is visible but not yet functional.
