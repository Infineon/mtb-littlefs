/***************************************************************************//**
 * \file lfs_spi_flash_bd.h
 *
 * \brief
 * Implements the block device driver functions for SPI NOR flash for use with
 * the littlefs API. Makes use of the
 * <a href="https://github.com/Infineon/serial-flash">serial-flash</a>
 * library.
 *
 *******************************************************************************
 * \copyright
 * (c) (2021-2025), Cypress Semiconductor Corporation (an Infineon company) or
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
 * Based upon lfs_rambd.h from littlefs repo
 * [https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.h]
 * Copyright (c) 2017, Arm Limited. All rights reserved
 *******************************************************************************/

/**
 * \addtogroup group_lfs_spi_flash_bd SPI Flash Block Device Driver
 * \{
 * * Implements the block device driver functions for SPI NOR flash for use with the
 * littlefs API.
 * * Makes use of the <a href="https://github.com/Infineon/serial-flash">serial-flash</a>
 * library.
 * * Uses the SFDP protocol to automatically discover the flash parameters.
 * * Provides \ref lfs_spi_flash_bd_lock() and \ref lfs_spi_flash_bd_unlock()
 * functions for use with lfs_config structure when LFS_THREADSAFE macro is
 * defined.
 *
* <b>Note:</b>
 * * Add DEFINE=LFS_THREADSAFE in the Makefile when thread-safety is required.
 * * Add COMPONENTS=RTOS_AWARE for enabling the RTOS-friendly features such as
 * waiting on a semaphore until the read completion is notified through a
 * callback by serial-flash.
 * * XIP support in CAT1B/CAT1D devices:
 * When the same memory is used for code execution (XIP) and storing data by
 * Littlefs, macro ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH must be added to
 * DEFINES variable in Makefile. When macro ENABLE_XIP_LITTLEFS_ON_SAME_NOR_FLASH
 * is defined, the Littlefs does not use features of serial-flash not supported
 * for this case, for example, asynchronous transfer. Also, the size of available
 * for Littlefs memory must be restricted by the
 * \ref lfs_spi_flash_bd_configure_memory function.
 * \note Disabling of the asynchronous transfer can impact the performance of
 * RTOS environment.
 * * Littlefs can use the memory with blocks of the same size. For hybrid
 * memory, it is compulsory to limit the size available for littlefs by the
 * \ref lfs_spi_flash_bd_configure_memory function to use only same-size blocks.
 */

#ifndef LFS_SPI_FLASH_BD_H            /* Guard against multiple inclusion */
#define LFS_SPI_FLASH_BD_H

#include "lfs.h"
#include "lfs_util.h"
#include "cy_result.h"
#include "cy_smif_memslot.h"
#include "mtb_serial_memory.h"

#ifdef CY_IP_MXSMIF

/**
 * \cond DO_NOT_DOCUMENT
 * This block of code ignores violations of Directive 4.6 MISRA.
 * Functions lfs_spi_flash_bd_unlock and lfs_spi_flash_bd_lock don't reproduce violations if LFS_THREADSAFE not defined.
 */

#if defined(LFS_THREADSAFE)
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',6,\
'The third-party defines the function interface with basic numeral type')
#else
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',4,\
'The third-party defines the function interface with basic numeral type')
#endif /* #if defined(LFS_THREADSAFE) */

/**
 *\endcond
 */

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Enable trace for this driver by defining this macro. You must also define the
 * global trace enable macro LFS_YES_TRACE.
 */
#ifdef LFS_SPI_FLASH_BD_YES_TRACE
#define LFS_SPI_FLASH_BD_TRACE(...) LFS_TRACE(__VA_ARGS__)
#else
#define LFS_SPI_FLASH_BD_TRACE(...)
#endif


/**
 * \brief Configures the memory region used by littlefs. If this function
 * is not called, the littlefs will use the whole size of the memory module.
 * The function must be called before lfs_spi_flash_bd_create(). After
 * de-initialization of littlefs, the settings configured by this function
 * are lost.
 * \param lfs_cfg The pointer to the block device configuration structure
 * \param address The start of a memory region available for littlefs.
 * \param region_size The size of a memory region available for littlefs.
 */
void lfs_spi_flash_bd_configure_memory(const struct lfs_config *lfs_cfg, uint32_t address, uint32_t region_size);

/**
 * \brief Initializes the SPI flash and populates the lfs_config structure with
 * the default values.
 * \param lfs_cfg Pointer to the lfs_config structure that will be
          initialized with the default values.
 * \param serial_memory_obj Pointer to the serial memory object.
 * \returns CY_RSLT_SUCCESS if the initialization was successful; an error code
 *          otherwise.
 */
cy_rslt_t lfs_spi_flash_bd_create(struct lfs_config *lfs_cfg, mtb_serial_memory_t *serial_memory_obj);

/**
 * \brief De-initializes the SPI flash and frees the resources.
 * \param lfs_cfg Pointer to the lfs_config structure.
 */
void lfs_spi_flash_bd_destroy(const struct lfs_config *lfs_cfg);

/**
 * \brief Reads the data starting from a given block and offset.
 * Calls the non-blocking read API from serial-flash and waits on a semaphore
 * until read completes if COMPONENTS=RTOS_AWARE is added in the makefile.
 * Otherwise, calls the blocking read function.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \param block Block number from which read should begin.
 * \param off Offset in the block from which read should begin.
 * \param buffer Pointer to the buffer to store the data read from the memory.
 * \param size Number of bytes to read.
 * \returns 0 if the read was successful; -1 otherwise.
 */
int lfs_spi_flash_bd_read(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size);

/**
 * \brief Programs or writes the data starting from a given block and offset.
 * The block must have been previously erased. This is a blocking function.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \param block Block number from which write should begin.
 * \param off Offset in the block from which write should begin.
 * \param buffer Pointer to the buffer that contains the data to be written.
 * \param size Number of bytes to write.
 * \returns 0 if the write was successful; -1 otherwise.
 */
int lfs_spi_flash_bd_prog(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

/**
 * \brief Erases a given block. A block must be erased before being programmed.
 * This is a blocking function.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \param block Block number to be erased.
 * \returns 0 if the write was successful; -1 otherwise.
 */
int lfs_spi_flash_bd_erase(const struct lfs_config *lfs_cfg, lfs_block_t block);

/**
 * \brief Flushes the write cache when present. Simply returns zero
 * because QSPI block does not have any write cache in MMIO mode.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \returns Always 0.
 */
int lfs_spi_flash_bd_sync(const struct lfs_config *lfs_cfg);

#if defined(LFS_THREADSAFE)
/**
 * \brief Locks or gets the mutex associated with this block device.
 * This function is internally called by the littlefs APIs when
 * LFS_THREADSAFE is defined. User should call this function directly only if
 * the other block device functions are directly called and thread-safety is
 * required in that case.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \returns 0 if locking was successful; -1 otherwise.
 */
int lfs_spi_flash_bd_lock(const struct lfs_config *lfs_cfg);

/**
 * \brief Unlocks or sets the mutex associated with this block device.
 * This function is internally called by the littlefs APIs when
 * LFS_THREADSAFE is defined. User should call this function directly only if
 * the other block device functions are directly called and thread-safety is
 * required in that case.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \returns 0 if unlocking was successful; -1 otherwise.
 */
int lfs_spi_flash_bd_unlock(const struct lfs_config *lfs_cfg);

#endif /* #if defined(LFS_THREADSAFE) */

#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSMIF */

CY_MISRA_BLOCK_END('MISRA C-2012 Directive 4.6')

#endif                      /* Avoid multiple inclusion */

/** \} group_lfs_spi_flash_bd */
