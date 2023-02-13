/***************************************************************************//**
 * \file lfs_sd_bd.h
 *
 * \brief
 * Implements the block device driver functions for SD card (card mode) for use
 * with littlefs API. Makes use of the Secure Digital Host Controller (SDHC) HAL
 * driver.
 *
 *******************************************************************************
 * \copyright
 * (c) (2021-2022), Cypress Semiconductor Corporation (an Infineon company) or
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
 * \addtogroup group_lfs_sd_bd SD Card Block Device Driver
 * \{
 * * Implements the block device driver functions for the SD card (card mode) for use
 * with littlefs API.
 * * Makes use of the Secure Digital Host Controller (SDHC) HAL
 * driver.
 * * Provides \ref lfs_sd_bd_lock() and \ref lfs_sd_bd_unlock() functions for
 * use with lfs_config structure when LFS_THREADSAFE macro is defined.
 * * Thread safety is implemented using only one mutex instance. This means that
 * a thread will get blocked while trying to perform an operation regardless of
 * which SDHC hardware instance it is accessing until another thread completes
 * that operation.
 *
 * <b>Note:</b>
 * * Add DEFINE=LFS_THREADSAFE in the Makefile when thread-safety is required.
 * * Add COMPONENTS=RTOS_AWARE for enabling the RTOS-friendly features of the
 * underlying SDHC HAL driver. An example of such feature is that the read
 * function waits on a semaphore until read completion is indicated through an
 * interrupt.
 */

#pragma once

#include "lfs.h"
#include "lfs_util.h"
#include "cy_result.h"
#include "cyhal_sdhc.h"
#include "cyhal_gpio.h"
#include <stdbool.h>

#ifdef CY_IP_MXSDHC

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Enable trace for this driver by defining this macro. You must also define the
 * global trace enable macro LFS_YES_TRACE.
 */
#ifdef LFS_SD_BD_YES_TRACE
#define LFS_SD_BD_TRACE(...) LFS_TRACE(__VA_ARGS__)
#else
#define LFS_SD_BD_TRACE(...)
#endif

/**
 * Configuration structure for the SD block device. The members of this
 * structure is passed to the SDHC HAL driver. See the device-specific HAL
 * driver API reference manual for details.
 */
typedef struct
{
    cyhal_sdhc_t sdhc_obj; /**< Object for use with the SDHC HAL driver. */
    cyhal_sdhc_config_t sdhc_config; /**< Card configuration structure to be passed to the HAL driver. */
    cyhal_gpio_t cmd;   /**< The pin connected to the command signal. */
    cyhal_gpio_t clk;   /**< The pin connected to the clock signal. */
    cyhal_gpio_t data0; /**< The pin connected to the data0 signal. */
    cyhal_gpio_t data1; /**< The pin connected to the data1 signal. */
    cyhal_gpio_t data2; /**< The pin connected to the data2 signal. */
    cyhal_gpio_t data3; /**< The pin connected to the data3 signal. */
    cyhal_gpio_t data4; /**< The pin connected to the data4 signal; pass NC when unused. */
    cyhal_gpio_t data5; /**< The pin connected to the data5 signal; pass NC when unused. */
    cyhal_gpio_t data6; /**< The pin connected to the data6 signal; pass NC when unused. */
    cyhal_gpio_t data7; /**< The pin connected to the data7 signal; pass NC when unused. */
    cyhal_gpio_t card_detect; /**< The pin connected to the card detect signal. */
    cyhal_gpio_t io_volt_sel; /**< The pin connected to the voltage select signal. */
    cyhal_gpio_t card_if_pwr_en; /**< The pin connected to the card interface power enable signal. */
    cyhal_gpio_t card_mech_write_prot; /**< The pin connected to the write protect signal. */
    cyhal_gpio_t led_ctrl; /**< The pin connected to the LED control signal. */
    cyhal_gpio_t card_emmc_reset; /**< The pin connected to the eMMC card reset signal. */
    cyhal_clock_t * block_clk; /**< The clock to use can be shared, if not provided a new clock will be allocated. */
} lfs_sd_bd_config_t;

/**
 * \brief Fetches the default configuration for the block device for use with
 * the \ref lfs_sd_bd_create() function.
 * Default configuration: SD card (card mode) with 3.3V signalling.
 * \param bd_cfg Pointer to the block device configuration structure.
 */
void lfs_sd_bd_get_default_config(lfs_sd_bd_config_t *bd_cfg);

/**
 * \brief Initializes the SD card interface and populates the lfs_config
 * structure with the default values.
 * \param lfs_cfg Pointer to the lfs_config structure that will be
          initialized with the default values.
 * \param bd_cfg Pointer to the block device configuration structure.
 * \returns CY_RSLT_SUCCESS if the initialization was successful; an error code
 *          otherwise.
 */
cy_rslt_t lfs_sd_bd_create(struct lfs_config *lfs_cfg, const lfs_sd_bd_config_t *bd_cfg);

/**
 * \brief De-initializes the SD interface and frees the resources.
 * \param lfs_cfg Pointer to the lfs_config structure.
 */
void lfs_sd_bd_destroy(const struct lfs_config *lfs_cfg);

/**
 * \brief Reads data starting from a given block and offset.
 * This is a blocking function.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \param block Block number from which read should begin.
 * \param off Offset in the block from which read should begin.
 * \param buffer Pointer to the buffer to store the data read from the memory.
 * \param size Number of bytes to read.
 * \returns 0 if the read was successful; -1 otherwise.
 */
int lfs_sd_bd_read(const struct lfs_config *lfs_cfg, lfs_block_t block,
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
int lfs_sd_bd_prog(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size);

/**
 * \brief Erases a given block. SD/MMC cards do not require explicit erase
 * before programming since the card controllers perform erase internally during
 * programming. Therefore, this API is not to be used with the erase() of
 * lfs_config structure. However, user can directly call this API to erase
 * specific blocks of a memory card.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \param block Block number to be erased.
 * \returns 0 if the erase was successful; -1 otherwise.
 */
int lfs_sd_bd_erase(const struct lfs_config *lfs_cfg, lfs_block_t block);

/**
 * \brief Flushes the write cache when present. Simply returns zero
 * because the SDHC block does not have any write cache.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \returns Always 0.
 */
int lfs_sd_bd_sync(const struct lfs_config *lfs_cfg);

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
int lfs_sd_bd_lock(const struct lfs_config *lfs_cfg);

/**
 * \brief Unlocks or sets the mutex associated with this block device.
 * This function is internally called by the littlefs APIs when
 * LFS_THREADSAFE is defined. User should call this function directly only if
 * the other block device functions are directly called and thread-safety is
 * required in that case.
 * \param lfs_cfg Pointer to the lfs_config structure.
 * \returns 0 if unlocking was successful; -1 otherwise.
 */
int lfs_sd_bd_unlock(const struct lfs_config *lfs_cfg);
#endif /* #if defined(LFS_THREADSAFE) */


#if defined(__cplusplus)
}
#endif

#endif /* CY_IP_MXSDHC */

/** \} group_lfs_sd_bd */
