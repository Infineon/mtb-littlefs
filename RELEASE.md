# Block Device Drivers for littlefs v2.1.0

## What's Included?

For details, refer to [README.md](./README.md) and the
[API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html).

New in the release:

* Adding support of AIROC™ CYW20829 (CAT1B) device

## Known issues and limitations

* Not supported the simultaneous use of the SD block device through the SDHC1 hardware instance
and SPI flash block device. But, if you use SDHC0 hardware instance when available,
you can use them simultaneously. The SD block device driver picks an instance
based on the data and clock pins provided by the user. See the
[PSoC™ 6 MCU](https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-6-32-bit-arm-cortex-m4-mcu/)
for information on the pin assignment. This will be addressed in a future release.

* For the SD block device driver, the thread-safety is implemented using only one
mutex instance. This means that while trying to perform an operation, a thread
will be blocked until another thread completes that operation,
whichever SDHC hardware instance the thread is accessing.

* The SD block device driver is not supported on CM55 core for PSoC™ Edge E84 MCUs. This will be addressed in a future release.

## Supported software and tools

This version was validated for compatibility with the following software and tools:

| Software and Tools                        | Version |
| :---------------------------------------- | :------ |
| ModusToolbox™ Software Environment        | 3.3     |
| mtb-hal-cat1                              | 2.6.1   |
| serial-flash                              | 1.4.2   |
| abstraction-rtos                          | 1.9.0   |
| Littlefs                                  | 2.4.0   |
| GCC Compiler                              | 11.3.1  |
| IAR Compiler                              | 9.50.2  |
| Arm® Compiler 6                           | 6.22    |

Minimum required ModusToolbox™ Software Environment: ModusToolbox™ v3.0 and
HAL Cat1 2.0

## More information

* [README.md](./README.md)
* [API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html)
* [PSoC™ 6 MCU: littlefs Filesystem](https://github.com/Infineon/mtb-example-psoc6-filesystem-littlefs-freertos) code example
* [Littlefs filesystem](https://github.com/littlefs-project/littlefs)
* [Infineon GitHub](https://github.com/Infineon)
* [ModusToolbox™ software GitHub](https://github.com/Infineon/modustoolbox-software)
* [ModusToolbox™ Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
* [PSoC™ 6 MCU Code Examples using ModusToolbox](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software)
* [How to Design with PSoC™ 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857)

---
© 2021-2024 Cypress Semiconductor Corporation, an Infineon Technologies Company.
