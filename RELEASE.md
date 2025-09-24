# Block Device Drivers for littlefs

## What's Included?

For details, refer to [README.md](./README.md) and the
[API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html).

New in the release:

* Migrate mtb-littlefs Middleware to the HAL Next flow

## Known issues and limitations

* For the SD block device driver, the thread-safety is implemented using only one
mutex instance. This means that while trying to perform an operation, a thread
will be blocked until another thread completes that operation,
whichever SDHC hardware instance the thread is accessing.

* To avoid the compiler's warnings in the mtb-littlefs project, you should add DEFINES+=LFS_NO_ASSERT in the Makefile.

* Serial-memory does not support asynchronous transfers. As a result, asynchronous transfer is disabled by default in mtb-littlefs.

## Supported software and tools

This version was validated for compatibility with the following software and tools:

| Software and Tools                        | Version  |
| :---------------------------------------- | :------- |
| ModusToolbox™ Software Environment        | 3.6      |
| MTB Device Support package for PSE84      | 1.0.0    |
| serial-memory                             | 3.0.0    |
| abstraction-rtos                          | 1.11.0   |
| Littlefs                                  | 2.4.0    |
| GCC Compiler                              | 14.2.1   |
| IAR Compiler                              | 9.50.2   |
| Arm® Compiler 6                           | 6.22     |
| LLVM Compiler                             | 19.1.5   |

## More information

* [README.md](./README.md)
* [API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html)
* [Littlefs filesystem](https://github.com/littlefs-project/littlefs)
* [Infineon GitHub](https://github.com/Infineon)
* [ModusToolbox™ software GitHub](https://github.com/Infineon/modustoolbox-software)
* [ModusToolbox™ Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
---
© 2021-2025 Cypress Semiconductor Corporation, an Infineon Technologies Company.
