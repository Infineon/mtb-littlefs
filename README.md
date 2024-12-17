# Block Device Drivers for littlefs

## Overview

Provides a set of block device drivers for the
[littlefs](https://github.com/littlefs-project/littlefs) file system.
The drivers' interface is very similar to
[lfs_rambd.h](https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.h)
provided with littlefs. See the API reference manual for driver-specific information.

## Features

- Supports SPI flash and SD card (Card mode) block devices
- Each driver provides the function to fetch the default configuration of the block device
- Implements the thread safety for multi-threaded RTOS environments using the
  [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library
- Built on top of existing drivers such as
  [serial-flash](https://github.com/Infineon/serial-flash) and HAL

- Supports Serial Flash Discoverable Parameter (SFDP) mode for SPI flash memories

### Devices and supported features:
| Device | SPI flash | SD card       |
|:-------|----------:|--------------:|
| CAT1A  | Supported | Supported     |
| CAT1B  | Supported | Not supported |

## Quick start

The [mtb-example-psoc6-filesystem-littlefs-freertos](https://github.com/Infineon/mtb-example-psoc6-filesystem-littlefs-freertos)
code example describes the implementation of the littlefs file system on SD card and QSPI NOR flash.

1. Create an empty application using the Project Creator tool in the ModusToolbox™ software.
2. Add the *mtb-littlefs* and *retarget-io* libraries using the Library Manager.
3. Add the below code to *main.c*.
4. For PSoC™ Edge E84 MCUs, add macro ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH to the DEFINES variable in the project Makefile.
5. Open a serial terminal. Set the serial port parameters to 8N1 and 115200 baud.
6. Build the application and program the kit.

   ```cpp
   #include "cybsp.h"
   #include "cy_retarget_io.h"
   #include "lfs.h"
   #include "lfs_spi_flash_bd.h"
   
   #if defined (COMPONENT_CAT1B)
   #define FLASH_LFS_ADDRESS_START                 (0x80000U)
   #define FLASH_LFS_SIZE                          (0x80000U)
   #endif /* #if defined (COMPONENT_CAT1B) */
   
   void check_status(const char *message, uint32_t status); /* Functions must have interface definition before declarations to satisfy MISRA C-2012 */
   
   void check_status(const char *message, uint32_t status)
   {
       if (0u != status)
       {
           (void)printf("\n================================================================================\n");
           (void)printf("\nFAIL: %s\n", message);
           (void)printf("Error Code: 0x%08"PRIx32"\n", status);
           (void)printf("\n================================================================================\n");
   
           while(true)
           {
           }
       }
   }
   
   int main(void)
   {
       cy_rslt_t result;
       uint32_t boot_count = 0;
   
       /* variables used by the filesystem */
       lfs_t lfs_inst;
       struct lfs_config lfs_cfg;
       lfs_file_t file;
       lfs_spi_flash_bd_config_t bd_cfg;
   
       /* Initialize the device and board peripherals */
       result = cybsp_init();
       CY_ASSERT (result == CY_RSLT_SUCCESS);
   
       /* Enable global interrupts */
       __enable_irq();
   
       /* Initialize retarget-io to use the debug UART port */
       (void)cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
   
       (void)printf("\nIncrementing the boot count on SPI flash\n\n");
   
       /* Get the default configuration for the SPI flash block device. */
       lfs_spi_flash_bd_get_default_config(&bd_cfg);
   
       /* Initialize the pointers in lfs_cfg to NULL. */
       (void)memset(&lfs_cfg, 0, sizeof(lfs_cfg));
   
   #if defined (COMPONENT_CAT1B)
       lfs_spi_flash_bd_configure_memory(&lfs_cfg, FLASH_LFS_ADDRESS_START, FLASH_LFS_SIZE);
   #endif /* #if defined (COMPONENT_CAT1B) */
   
       /* Create the SPI flash block device. */
       result = lfs_spi_flash_bd_create(&lfs_cfg, &bd_cfg);
       check_status("Creating SPI flash block device failed.", result);
   
       (void)printf("Number of blocks: %"PRIu32"\n", lfs_cfg.block_count);
       (void)printf("Erase block size: %"PRIu32" bytes\n", lfs_cfg.block_size);
       (void)printf("Prog size: %"PRIu32" bytes\n", lfs_cfg.prog_size);
   
       /* Mount the filesystem */
       int32_t err = lfs_mount(&lfs_inst, &lfs_cfg);
   
       /* Reformat if we cannot mount the filesystem.
       * This should only happen on the first boot.
       */
       if (err != 0) {
           (void)lfs_format(&lfs_inst, &lfs_cfg);
           (void)lfs_mount(&lfs_inst, &lfs_cfg);
       }
   
       /* Read the current boot count. */
       (void)lfs_file_open(&lfs_inst, &file, "boot_count", (int32_t)((uint32_t)LFS_O_RDWR | (uint32_t)LFS_O_CREAT)); /* The argument of arithmetic operation must be an unsigned integer but the argument lfs_file_open is defined by a third party and has type signed integer. For MISRA C-2012 */
       (void)lfs_file_read(&lfs_inst, &file, &boot_count, sizeof(boot_count));
   
       /* Update the boot count. */
       boot_count += 1u;
       (void)lfs_file_rewind(&lfs_inst, &file);
       (void)lfs_file_write(&lfs_inst, &file, &boot_count, sizeof(boot_count));
   
       /* The storage is not updated until the file is closed successfully. */
       (void)lfs_file_close(&lfs_inst, &file);
   
       /* Release any resources we were using. */
       (void)lfs_unmount(&lfs_inst);
   
       /* Print the boot count. */
       (void)printf("\nboot_count: %"PRIu32"\n", boot_count);
   
       /* Free the resources associated with the block device. */
       lfs_spi_flash_bd_destroy(&lfs_cfg);
   
       for (;;) 
       {
       }
   }
   ```


## Usage instructions

### For the usage in an RTOS environment:

1. Add [FreeRTOS](https://github.com/Infineon/freertos) using the
   Library Manager if FreeRTOS is your choice of RTOS.
2. Add [abstraction-rtos](https://github.com/Infineon/abstraction-rtos)
   using the Library Manager tool if you are using an RTOS other than FreeRTOS.
   *abstraction-rtos* is added automatically when you add FreeRTOS using the Library Manager.
3. Add `FREERTOS` to the components list when using FreeRTOS.
4. Add `DEFINES=LFS_THREADSAFE` to enable the thread-safety for the littlefs APIs.
5. Add `COMPONENTS=RTOS_AWARE` in the Makefile to enable RTOS-friendly features,
   such as waiting on a semaphore until read completion is indicated through an interrupt or a callback.

**Note:** The source files under *\<littlefs_path\>/bd* are ignored from
auto-discovery. Therefore, they will be excluded from compilation because some
of the files (e.g., *lfs_filebd.c*) use POSIX file APIs such as `open()` and
`close()`. POSIX APIs are not supported by the ModusToolbox™. If you implement POSIX
APIs, you can include those files for compilation by adding them to the `SOURCES`
and `INCLUDES` variables in the Makefile.

## Dependencies

The dependencies except *abstraction-rtos* are automatically pulled in when you
run the `make getlibs` command in the ModusToolbox™.

- [littlefs](https://github.com/littlefs-project/littlefs)
- [serial-flash](https://github.com/Infineon/serial-flash)
- [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library if RTOS support is required.

    **Note:** *abstraction-rtos* is automatically pulled in only when you add FreeRTOS using the Library Manager; otherwise, add it manually.

## Supported Devices

- PSoC™ 6 MCUs        (CAT1A)
- AIROC™ CYW20829     (CAT1B)

## More Information

- <a href="https://github.com/Infineon/mtb-littlefs/blob/master/RELEASE.md">RELEASE.md</a>
- [API Reference Guide](https://infineon.github.io/mtb-littlefs/api_reference_manual/html/index.html)
- [PSoC™ 6 MCU: littlefs Filesystem](https://github.com/Infineon/mtb-example-psoc6-filesystem-littlefs-freertos) code example
- [Littlefs filesystem](https://github.com/littlefs-project/littlefs)
- [Infineon GitHub](https://github.com/Infineon)
- [ModusToolbox™ software GitHub](https://github.com/Infineon/modustoolbox-software)
- [ModusToolbox™ Software](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
- [PSoC™ 6 MCU Code Examples using ModusToolbox™](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software)
- [How to Design with PSoC™ 6 MCU - KBA223067](https://community.infineon.com/t5/Knowledge-Base-Articles/How-to-Design-with-PSoC-6-MCU-KBA223067/ta-p/248857)

---
© 2021-2024 Cypress Semiconductor Corporation, an Infineon Technologies Company.