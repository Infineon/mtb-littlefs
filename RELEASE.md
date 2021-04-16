# Block Device Drivers for littlefs Release Notes

Provides a set of block device drivers for use with the [littlefs](https://github.com/littlefs-project/littlefs) file system. The interface of the drivers is very similar to [lfs_rambd.h](https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.h) provided with littlefs. See the API reference manual for driver-specific information.

## What's Included?

- Supports SPI flash and SD card (card mode) block devices

- Implements thread safety for use with multi-threaded RTOS environments using the [abstraction-rtos](https://github.com/cypresssemiconductorco/abstraction-rtos) library

- Built on top of existing drivers such as [serial-flash](https://github.com/cypresssemiconductorco/serial-flash) and HAL

- Supports Serial Flash Discoverable Parameter (SFDP) mode for SPI flash memories

## Known Issues and Limitations

- Simultaneous use of the SD block device through the SDHC1 hardware instance and SPI flash block device is not supported. However, you can use them simultaneously if you use SDHC0 hardware instance when available. The SD block device driver picks an instance based on the data and clock pins provided by the user. See the [PSoC® 6 MCU Datasheets](https://www.cypress.com/search/all?f[0]=meta_type%3Atechnical_documents&f[1]=resource_meta_type%3A575&f[2]=field_related_products%3A114026) for information on the pin assignment. This will be addressed in a future release.

- For the SD block device driver, thread-safety is implemented using only one mutex instance. This means that a thread will get blocked while trying to perform an operation regardless of which SDHC hardware instance it is accessing until another thread completes that operation.

## Supported Software and Tools

This version was validated for compatibility with the following software and tools:

| Software and Tools                        | Version |
| :---                                      | :----  |
| ModusToolbox® Software Environment        | 2.2     |
| GCC Compiler                              | 9.3     |
| IAR Compiler                              | 8.4     |
| Arm® Compiler 6                           | 6.13    |

Minimum required ModusToolbox Software Environment: v2.2

## More information

- [mtb-example-psoc6-filesystem-littlefs-freertos](https://github.com/cypresssemiconductorco/mtb-example-psoc6-filesystem-littlefs-freertos) code example

- [API Reference Guide](https://cypresssemiconductorco.github.io/mtb-littlefs/html/index.html)

- [README.md](./README.md)

- [Littlefs filesystem](https://github.com/littlefs-project/littlefs)

- [Cypress Semiconductor GitHub](https://github.com/cypresssemiconductorco)

- [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)

- [PSoC 6 MCU Code Examples using ModusToolbox](https://github.com/cypresssemiconductorco/Code-Examples-for-ModusToolbox-Software)

- [PSoC 6 MCU Middleware](https://github.com/cypresssemiconductorco/psoc6-middleware)

- [PSoC 6 MCU Resources - KBA223067](https://community.cypress.com/docs/DOC-14644)


---
© 2021 Cypress Semiconductor Corporation, an Infineon Technologies Company.
