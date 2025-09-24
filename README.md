# Block Device Drivers for littlefs

## Overview

Provides a set of block device drivers for the
[littlefs](https://github.com/littlefs-project/littlefs) file system.
The drivers' interface is very similar to
[lfs_rambd.h](https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.h)
provided with littlefs. See the API reference manual for driver-specific information.

## Features

- Supports SPI flash and SD card (Card mode) block devices
- Implements the thread safety for multi-threaded RTOS environments using the
  [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library
- Built on top of existing drivers such as
  [serial-memory](https://github.com/Infineon/serial-memory) and HAL
- Supports Serial Flash Discoverable Parameter (SFDP) mode for SPI flash memories

### Devices and supported features:
|   Device  | SPI flash | SD card       |
|:----------|----------:|--------------:|
| PSE84     | Supported | Supported     |

## Supported Devices

- PSoC™ PSE84 MCUs

## Quick Start

The [Quick Start section of the mtb-littlefs Middleware API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html#section_quick_start_guide)
describes step-by-step instructions to set up a mtb-littlefs application.

## More Information

- <a href="https://github.com/Infineon/mtb-littlefs/blob/master/RELEASE.md">RELEASE.md</a>
- [API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html)
- [Littlefs filesystem](https://github.com/littlefs-project/littlefs)
- [Infineon GitHub](https://github.com/Infineon)
- [ModusToolbox™ software GitHub](https://github.com/Infineon/modustoolbox-software)
- [ModusToolbox™ Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)

---
© 2021-2025 Cypress Semiconductor Corporation, an Infineon Technologies Company.
