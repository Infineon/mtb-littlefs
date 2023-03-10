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

## Quick start

The [mtb-example-psoc6-filesystem-littlefs-freertos](https://github.com/Infineon/mtb-example-psoc6-filesystem-littlefs-freertos)
code example describes the implementation of the littlefs file system on SD card and QSPI NOR flash.

1. Create an empty application using the Project Creator tool in the ModusToolbox™ software.
2. Add the *mtb-littlefs* and *retarget-io* libraries using the Library Manager.
3. Add the below code to *main.c*.
4. Open a serial terminal. Set the serial port parameters to 8N1 and 115200 baud.
5. Build the application and program the kit.

   ```cpp
   #include "cybsp.h"
   #include "cy_retarget_io.h"
   #include "lfs.h"
   #include "lfs_spi_flash_bd.h"
   
   void check_status(char *message, uint32_t status)
   {
      if (0u != status)
      {
          printf("\n================================================================================\n");
          printf("\nFAIL: %s\n", message);
          printf("Error Code: 0x%08"PRIx32"\n", status);
          printf("\n================================================================================\n");
          
          while(true);
      }
   }
   
   int main(void)
   {
      cy_rslt_t result;
      uint32_t boot_count = 0;
   
      /* variables used by the filesystem */
      lfs_t lfs;
      struct lfs_config lfs_cfg;
      lfs_file_t file;
      lfs_spi_flash_bd_config_t bd_cfg;
   
      /* Initialize the device and board peripherals */
      result = cybsp_init() ;
      CY_ASSERT (result == CY_RSLT_SUCCESS);
   
      /* Enable global interrupts */
      __enable_irq();
   
      /* Initialize retarget-io to use the debug UART port */
      cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
   
      printf("\nIncrementing the boot count on SPI flash\n\n");
   
      /* Get the default configuration for the SPI flash block device. */
      lfs_spi_flash_bd_get_default_config(&bd_cfg);
   
      /* Initialize the pointers in lfs_cfg to NULL. */
      memset(&lfs_cfg, 0, sizeof(lfs_cfg));
   
      /* Create the SPI flash block device. */
      result = lfs_spi_flash_bd_create(&lfs_cfg, &bd_cfg);
      check_status("Creating SPI flash block device failed.", result);
   
      printf("Number of blocks: %"PRIu32"\n", lfs_cfg.block_count);
      printf("Erase block size: %"PRIu32" bytes\n", lfs_cfg.block_size);
      printf("Prog size: %"PRIu32" bytes\n", lfs_cfg.prog_size);
   
      /* Mount the filesystem */
      int err = lfs_mount(&lfs, &lfs_cfg);
   
      /* Reformat if we cannot mount the filesystem.
       * This should only happen on the first boot.
       */
      if (err) {
          lfs_format(&lfs, &lfs_cfg);
          lfs_mount(&lfs, &lfs_cfg);
      }
   
      /* Read the current boot count. */
      lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
      lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));
   
      /* Update the boot count. */
      boot_count += 1;
      lfs_file_rewind(&lfs, &file);
      lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));
   
      /* The storage is not updated until the file is closed successfully. */
      lfs_file_close(&lfs, &file);
   
      /* Release any resources we were using. */
      lfs_unmount(&lfs);
   
      /* Print the boot count. */
      printf("\nboot_count: %"PRIu32"\n", boot_count);
   
      /* Free the resources associated with the block device. */
      lfs_spi_flash_bd_destroy(&lfs_cfg);
   
      for (;;) {}
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

- PSoC™ 6 MCUs

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
© 2021-2023 Cypress Semiconductor Corporation, an Infineon Technologies Company.