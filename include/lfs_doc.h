/***************************************************************************//**
 * \file lfs_doc.h
 *
 * \brief
 * General documentation for mtb-littlefs
 *
 *******************************************************************************
 * \copyright
 * (c) (2025), Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *
 *******************************************************************************/

/**
 * \mainpage
 *
 *
********************************************************************************
* \section section_mainpage_overview Overview
********************************************************************************
* This library provides implemented drivers for the
* [littlefs](https://github.com/littlefs-project/littlefs) file system. Typically,
* drivers are based on the HAL driver or other high-level middleware such as
* [serial-memory](https://github.com/Infineon/serial-memory).
*
********************************************************************************
* \section section_general_desc General Description
********************************************************************************
*
* This middleware:
* - Supports two types drivers: SPI NOR flash and SD card (Card mode).
* - Implements the thread safety for multi-threaded RTOS environments using the
* [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library.
* - Built on top of existing drivers such as
* [serial-memory](https://github.com/Infineon/serial-memory) and HAL
* - Supports Serial Flash Discoverable Parameter (SFDP) mode for SPI flash memories
*
********************************************************************************
* \section section_quick_start_guide Quick Start Guide
********************************************************************************
* The Quick Start Guide section has simple examples for the SPI flash and SD Card
* drivers.
* Examples are used to initialize the file system on the target memory,
* demonstrate the creation/opening file, and update its content.
*
********************************************************************************
* \subsection section_qsg_project_creation Project creation
********************************************************************************
*
* 1. Create an empty application using the Project Creator tool in the
* ModusToolbox&trade; software.
* 2. Add the [mtb-littlefs](https://github.com/Infineon/mtb-littlefs) and
* [retarget-io](https://github.com/Infineon/retarget-io) libraries using the
* Library Manager.
*
* 3. To run an example in the RTOS-environment, add the
* [FreeRTOS](https://github.com/Infineon/freertos) library using the Library
* Manager. Then, add *RTOS_AWARE* and *FREERTOS* to the Makefile COMPONENTS
* variable.
* \code COMPONENTS += RTOS_AWARE FREERTOS \endcode
* \note In the RTOS environment, select Active mode for System Idle Power Mode
* in Power personality (System Tab).
*
* *The next steps are applicable only for the SPI flash driver*
*
* 4. To use the same storage for code execution and File System,
* add the ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH macro to the DEFINES variable in
* the project Makefile.
* \code DEFINES += ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH \endcode
* \note For some devices, the same storage for code execution and File
* System is used as the default flow. So, adding
* ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH is mandatory.
*
********************************************************************************
* \subsection section_qsg_project_dev_conf HW Configuration in Device Configurator
********************************************************************************
*
* All HW resources used in this QSG must be configured in the Device Configurator:
* - UART for logging
* - One pin to check the button status
* - SD Card HW - SDHC
* - SPI flash HW - SMIF
*
* If a different alias name is selected, instead of expected resources name in
* the Device Configurator, the code snippets must be updated.
*
* <table class="doxtable">
*   <tr><th>Resource</th><th>Name</th></tr>
*   <tr>
*     <td>Quad Serial Memory Interface</td>
*     <td>SMIF_LITTLEFS</td>
*   </tr>
*   <tr>
*     <td>SD Host Controller</td>
*     <td>SDHC_LITTLEFS</td>
*   </tr>
*   <tr>
*     <td>Serial Communication Block (for debug UART)</td>
*     <td>DEBUG_UART</td>
*   </tr>
*   <tr>
*     <td>Pin</td>
*     <td>CYBSP_USER_BTN</td>
*   </tr>
* </table>
*
* *Recommended SD Card configuration*
* \image html littlefs_sd_card_cat1d.png
*
* *Recommended SMIF configuration*
* \image html littlefs_smif_cat1d.png
*
* *Recommended Debug UART*
* \image html littlefs_debug_uart_part1_cat1d.png
* \image html littlefs_debug_uart_part2_cat1d.png
*
* *Recommended Button Pin configuration*
* \image html littlefs_user_btn_cat1d.png
*
********************************************************************************
* \subsection section_qsg_main Add mtb-littlefs logic to main.c
********************************************************************************
* <b>Specific steps for SPI flash driver configuration</b>
* 1. Include the required headers.
*    \snippet main.c LITTLEFS_NOR_INCLUDE
* 2. If XIP support is enabled, limit the available space for littlefs
* on the memory device.
*    \snippet main.c LITTLEFS_NOR_LIMITS_MACRO_CAT1D
* \note The addresses and sizes depend on the memory device and project
* memory layout.
* 3. Add global variables.
*    \snippet main.c LITTLEFS_NOR_OBJ
* 4. Add the function for the NOR HW initialization.
*    \snippet main.c LITTLEFS_NOR_HW_INIT
* 5. Add the deinit function.
*    \snippet main.c LITTLEFS_NOR_HW_DEINIT
*
* <b>Specific steps for the SD Card driver configuration</b>
* 1. Include the required headers.
*    \snippet main.c LITTLEFS_SD_CARD_INCLUDE
* 2. Add global variables.
*    \snippet main.c LITTLEFS_SD_CARD_OBJ
* 3. Add the interrupt handler for SD Card HW.
*    \snippet main.c LITTLEFS_SD_CARD_ISR
* 4. Add the Cy_SD_Host_IsCardConnected() function
*    \snippet main.c LITTLEFS_SD_CARD_IS_CON
* 5. Add the function for the SD Card HW initialization.
*    \snippet main.c LITTLEFS_SD_CARD_HW_INIT
* 6. Add the deinit function.
*    \snippet main.c LITTLEFS_SD_CARD_HW_DEINIT
*
* <b>Common code</b>
* 1. Include the required headers.
*    \snippet main.c LITTLEFS_INCLUDE
*    If the FREERTOS is used, include the next headers.
*    \snippet main.c LITTLEFS_FREERTOS_INCLUDE
* 2. Add the common define to the project.
*    \snippet main.c LITTLEFS_GENERAL_MACRO
* 3. Add common global variables.
*    \snippet main.c LITTLEFS_OBJ
* 4. Add the function prototypes.
*    \snippet main.c LITTLEFS_GENERAL_FUNC_PROT
* 5. Add the boot count function.
*    \snippet main.c LITTLEFS_BOOT_COUNT
* 6. Add the check status function.
*    \snippet main.c LITTLEFS_CHECK_STATUS
* 7. Add the callback function for the user button.
*    \snippet main.c LITTLEFS_USER_BUTTON_CALLBACK
* 8. Add the interrupt handler for the user button.
*    \snippet main.c LITTLEFS_USER_BUTTON_ISR
* 9. Add the function for the initialization user button.
*    \snippet main.c LITTLEFS_USER_BUTTON_INIT
* 10. Update main.c.
*    \snippet main.c LITTLEFS_MAIN_C
*
* <b>retarget-io configuration</b>
* 1. Add the retarget-io configuration function.
*    \snippet retarget_io_config.c LITTLEFS_RETARGET_IO
* 2. Call this function in main.c before any other littlefs functions
*    \snippet main.c LITTLEFS_RETARGET_CALL
*
********************************************************************************
* \subsection section_qsg_project_execution Project execution
********************************************************************************
* 1. Open a serial terminal. Set the serial port parameters to 8N1 and 115200 baud.
* 2. Build and program the project.
* 3. If the previous steps were correct, the logs will appear in the
* serial terminal.
* 4. This example initializes the littlefs file system on the target memory device.
*    Create a file with a boot count and a value updated at every button reset
*    is pressed. Also if press the user button, The boot count value must reset to 0.
*
********************************************************************************
* \section section_conf_cons Configuration Considerations
********************************************************************************
*
* The driver configuration details are described in the relevant section:
* - \ref group_lfs_spi_flash_bd
* - \ref group_lfs_sd_bd
*
* \note The source files under *\<littlefs_path\>/bd* are ignored from
* auto-discovery. Therefore, they will be excluded from compilation because some
* of the files (e.g., *lfs_filebd.c*) use POSIX file APIs such as 'open()' and
* 'close()'. POSIX APIs are not supported by the ModusToolbox&trade;. If you implement POSIX
* APIs, you can include those files for compilation by adding them to the `SOURCES`
* and `INCLUDES` variables in the Makefile.
*
********************************************************************************
* \subsection section_rtos Usage in an RTOS environment:
********************************************************************************
*
* 1. Add [FreeRTOS](https://github.com/Infineon/freertos) using the
*    Library Manager if FreeRTOS is your choice of RTOS.
* 2. If you use a RTOS other than FreeRTOS, add
*    [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) using the
*    Library Manager tool.
*    *abstraction-rtos* is added automatically when you add FreeRTOS using the
*    Library Manager.
* 3. Add `FREERTOS` to the components list when using FreeRTOS.
* 4. Add `DEFINES=LFS_THREADSAFE` to enable the thread-safety for the littlefs APIs.
* 5. Add `COMPONENTS=RTOS_AWARE` in the Makefile to enable RTOS-friendly features,
*    such as waiting on a semaphore until read completion is indicated through an
*    interrupt or a callback.
*
********************************************************************************
* \section section_dependencies Dependencies
********************************************************************************
*
* The dependencies except *abstraction-rtos* are automatically pulled in when you
* run the 'make getlibs' command in the ModusToolbox&trade;.
*
* - [littlefs](https://github.com/littlefs-project/littlefs)
* - [serial-memory](https://github.com/Infineon/serial-memory)
* - [abstraction-rtos](https://github.com/Infineon/abstraction-rtos) library if RTOS support is required.
*
* \note *abstraction-rtos* is automatically pulled in only when you add FreeRTOS
* using the Library Manager; otherwise, add it manually.
*
********************************************************************************
* \section section_changelog Changelog
********************************************************************************
*
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for Change</th></tr>
*   <tr>
*     <td rowspan="1">3.0.0</td>
*     <td>Migrate mtb-littlefs Middleware to the HAL Next flow</td>
*     <td></td>
*   </tr>
* </table>
*
********************************************************************************
* \section section_more_info More Information
********************************************************************************
*
* For more information, refer to the links in the
* [README.md](https://github.com/Infineon/mtb-littlefs/blob/master/README.md#more-information)
*
*/

