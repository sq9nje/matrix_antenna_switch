# Interface Module Hardware

This directory contains all hardware design files for the 6x2 Matrix Antenna Switch Interface Module, designed in KiCad.

## Overview

The interface module is the control hub for the 6x2 matrix antenna switch system, designed by SQ9NJE (Quirky Solutions). The main component of the interface module is an ESP32 DevKit v1 development board. The module is also equipped with a half-duplex RS485 interface with automatic transmission direction detection and a 5V voltage regulator.

Next to the control signal connector for connecting the relay module, there is an additional SAO (Simple Add-On) connector. This connector provides access to I2C bus signals, two GPIO pins, and 3.3V power supply with ground. In the future, it may be used to extend the hardware capabilities of the switch, such as adding a display, band decoder, etc.

**📋 [View Schematic (PDF)](interface_module.pdf)**

**🔧 [Interactive BOM](https://htmlpreview.github.io/?https://github.com/sq9nje/matrix_antenna_switch/blob/master/interface_module/hardware/bom/ibom.html){:target="_blank"}**
