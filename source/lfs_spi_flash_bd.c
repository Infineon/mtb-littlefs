/***************************************************************************//**
 * \file lfs_spi_flash_bd.c
 *
 * \brief
 * Implements the block device driver functions for SPI NOR flash for use with
 * littlefs API. Makes use of the
 * <a href="https://github.com/cypresssemiconductorco/serial-flash">serial-flash</a>
 * library.
 *
 *******************************************************************************
 * \copyright
 * Copyright 2021, Cypress Semiconductor Corporation (an Infineon company) or
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
 * Based upon lfs_rambd.c from littlefs repo 
 * [https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.c]
 * Copyright (c) 2017, Arm Limited. All rights reserved
 *******************************************************************************/

#include "lfs_spi_flash_bd.h"
#include "lfs_util.h"
#include "cyhal_gpio.h"
#include "cy_serial_flash_qspi.h"
#include "lfs_qspi_memslot.h"
#include "cycfg_pins.h"

#if defined(COMPONENT_RTOS_AWARE) || defined(LFS_THREADSAFE)
#include "cyabs_rtos.h"
#endif /* #if defined(COMPONENT_RTOS_AWARE) || defined(LFS_THREADSAFE) */

#ifdef CY_IP_MXSMIF

#if defined(__cplusplus)
extern "C"
{
#endif

#define RESULT_OK                                   (0)
#define RESULT_ERROR                                (-1)
#define GET_INT_RETURN_VALUE(result)                ((CY_RSLT_SUCCESS == (result)) ? RESULT_OK : RESULT_ERROR)

#define DEFAULT_QSPI_FREQUENCY_HZ                   (50000000UL)
#define QSPI_MIN_READ_SIZE                          (1UL)

/* Recommended value as per the comment in lfs.c is in the range of 100-1000. */
#define LFS_CFG_DEFAULT_BLOCK_CYCLES                (512)
#define LFS_CFG_LOOKAHEAD_SIZE_MIN                  (64UL) /* Must be a multiple of 8. */

#if defined(COMPONENT_RTOS_AWARE)
#define QSPI_READ_SEMA_MAX_COUNT                    (1UL)
#define QSPI_READ_SEMA_INIT_COUNT                   (0UL)

#ifndef LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS
#define LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS      (500UL) /* in milliseconds */
#endif /* #ifndef LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS */

/* Semaphore used while waiting for the QSPI read operation to complete. */
static cy_semaphore_t qspi_read_sema;

void qspi_read_complete_callback(cy_rslt_t status, void *arg)
{
    cy_rslt_t result;

    *((cy_rslt_t *) arg) = status;
    result = cy_rtos_set_semaphore(&qspi_read_sema, true);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
    CY_UNUSED_PARAMETER(result); /* To avoid compiler warning in Release mode. */
}
#endif /* #if defined(COMPONENT_RTOS_AWARE) */

#if defined(LFS_THREADSAFE)

#ifndef LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS
#define LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS      (500UL)
#endif /* #ifndef LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS */

static cy_mutex_t _spi_flash_bd_mutex;
#endif /* #if defined(LFS_THREADSAFE) */

void lfs_spi_flash_bd_get_default_config(lfs_spi_flash_bd_config_t *bd_cfg)
{
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_get_default_config(%p)", (void*)bd_cfg);
    LFS_ASSERT(NULL != bd_cfg);

    bd_cfg->mem_config = &LFS_SFDP_SlaveSlot_0;

    bd_cfg->io0 = NC;
    bd_cfg->io1 = NC;
    bd_cfg->io2 = NC;
    bd_cfg->io3 = NC;
    bd_cfg->io4 = NC;
    bd_cfg->io5 = NC;
    bd_cfg->io6 = NC;
    bd_cfg->io7 = NC;
    bd_cfg->sclk = NC;
    bd_cfg->ssel = NC;
    bd_cfg->freq_hz = DEFAULT_QSPI_FREQUENCY_HZ;

#ifdef CYBSP_QSPI_D0
    bd_cfg->io0 = CYBSP_QSPI_D0;
#endif

#ifdef CYBSP_QSPI_D1
    bd_cfg->io1 = CYBSP_QSPI_D1;
#endif

#ifdef CYBSP_QSPI_D2
    bd_cfg->io2 = CYBSP_QSPI_D2;
#endif

#ifdef CYBSP_QSPI_D3
    bd_cfg->io3 = CYBSP_QSPI_D3;
#endif

#ifdef CYBSP_QSPI_SCK
    bd_cfg->sclk = CYBSP_QSPI_SCK;
#endif

#ifdef CYBSP_QSPI_SS
    bd_cfg->ssel = CYBSP_QSPI_SS;
#endif

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_get_default_config -> %d", 0);
}

cy_rslt_t lfs_spi_flash_bd_create(struct lfs_config *lfs_cfg, const lfs_spi_flash_bd_config_t *bd_cfg)
{
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_create(%p, %p)", (void*)lfs_cfg, (void*)bd_cfg);

    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(NULL != bd_cfg);

    /* Initialize the custom Card Detect pin. */
    cy_rslt_t result = cy_serial_flash_qspi_init(
                                bd_cfg->mem_config,
                                bd_cfg->io0, bd_cfg->io1, bd_cfg->io2, bd_cfg->io3,
                                bd_cfg->io4, bd_cfg->io5, bd_cfg->io6, bd_cfg->io7,
                                bd_cfg->sclk, bd_cfg->ssel, bd_cfg->freq_hz);

#if defined(LFS_THREADSAFE)
    if(CY_RSLT_SUCCESS == result)
    {
        /* Initialize the mutex. */
        result = cy_rtos_init_mutex(&_spi_flash_bd_mutex);
    }
#endif /* #if defined(LFS_THREADSAFE) */

    if(CY_RSLT_SUCCESS == result)
    {
        /* Block device operations */
        lfs_cfg->read        = lfs_spi_flash_bd_read;
        lfs_cfg->prog        = lfs_spi_flash_bd_prog;
        lfs_cfg->erase       = lfs_spi_flash_bd_erase;
        lfs_cfg->sync        = lfs_spi_flash_bd_sync;

#if defined(LFS_THREADSAFE)
        lfs_cfg->lock        = lfs_spi_flash_bd_lock;
        lfs_cfg->unlock      = lfs_spi_flash_bd_unlock;
#endif /* #if defined(LFS_THREADSAFE) */

        /* Block device configuration */
        lfs_cfg->read_size   = QSPI_MIN_READ_SIZE;
        lfs_cfg->prog_size   = cy_serial_flash_qspi_get_prog_size(0);
        lfs_cfg->block_size  = cy_serial_flash_qspi_get_erase_size(0);
        lfs_cfg->block_count = cy_serial_flash_qspi_get_size() / lfs_cfg->block_size;

        /* Refer to lfs.h for the description of the following parameters. */

        /* Number of erase cycles before the data is moved to a new block.
         * A larger value results in more efficient filesystem performance, but
         * causes less even-wear distribution.
         *
         * Setting this to -1 disables dynamic wear leveling.
         */
        lfs_cfg->block_cycles = LFS_CFG_DEFAULT_BLOCK_CYCLES;

        /* cache_size must be a multiple of prog & read sizes.
         * i.e., cache_size % prog_size = 0 and cache_size % read_size = 0
         * block_size must be a multiple of cache_size. i.e., block_size % cache_size = 0.
         *
         * littlefs allocates 1 cache for each file and 2 caches for
         * internal operations.
         * The higher the cache size, the better the performance is, but the
         * RAM consumption is also higher.
         */
        lfs_cfg->cache_size = lfs_cfg->prog_size;

        /* A larger Lookahead size reduces the number of scans performed by
         * the block allocation algorithm thus increasing the filesystem
         * performance, but results in higher RAM consumption.
         *
         * Must be a multiple of 8.
         */
        lfs_cfg->lookahead_size = lfs_min((lfs_size_t) LFS_CFG_LOOKAHEAD_SIZE_MIN, 8UL * ((lfs_cfg->block_count + 63UL)/64UL) );

#if defined(COMPONENT_RTOS_AWARE)
        result = cy_rtos_init_semaphore(&qspi_read_sema, QSPI_READ_SEMA_MAX_COUNT, QSPI_READ_SEMA_INIT_COUNT);
#endif /* #if defined(COMPONENT_RTOS_AWARE) */
    }

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_create -> %"PRIu32"", result);
    return result;
}

void lfs_spi_flash_bd_destroy(const struct lfs_config *lfs_cfg)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_destroy(%p)", (void*)lfs_cfg);

    CY_UNUSED_PARAMETER(lfs_cfg);
    cy_serial_flash_qspi_deinit();

#if defined(COMPONENT_RTOS_AWARE)
    result = cy_rtos_deinit_semaphore(&qspi_read_sema);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
#endif /* #if defined(COMPONENT_RTOS_AWARE) */

#if defined(LFS_THREADSAFE)
    result = cy_rtos_deinit_mutex(&_spi_flash_bd_mutex);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
#endif /* #if defined(LFS_THREADSAFE) */

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_destroy -> %d", 0);
    CY_UNUSED_PARAMETER(result); /* To avoid compiler warning in Release mode. */
}

int lfs_spi_flash_bd_read(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    cy_rslt_t result;

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_read(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT((off % lfs_cfg->read_size) == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT((size % lfs_cfg->read_size) == 0);

#if defined(COMPONENT_RTOS_AWARE)
    cy_rslt_t qspi_read_status = CY_RSLT_SUCCESS;

    /* Disable interrupts to ensure interrupt occurs only when we are ready to
     * get the semaphore.
     */
    uint32_t saved_intr_status = cyhal_system_critical_section_enter();
    result = cy_serial_flash_qspi_read_async((block * lfs_cfg->block_size) + off, size, buffer, qspi_read_complete_callback, (void *)&qspi_read_status);
    cyhal_system_critical_section_exit(saved_intr_status);

    if(CY_RSLT_SUCCESS == result)
    {
        /* Wait until the read semaphore is set. */
        result = cy_rtos_get_semaphore(&qspi_read_sema, LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS, false);

        if(CY_RSLT_SUCCESS == result)
        {
            result = qspi_read_status;
        }
    }
#else
    result = cy_serial_flash_qspi_read((block * lfs_cfg->block_size) + off, size, buffer);
#endif /* #if defined(COMPONENT_RTOS_AWARE) */

    int res = GET_INT_RETURN_VALUE(result);

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_read -> %d", res);
    return res;
}

int lfs_spi_flash_bd_prog(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_prog(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT(off  % lfs_cfg->prog_size == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT(size % lfs_cfg->prog_size == 0);

    cy_rslt_t result = cy_serial_flash_qspi_write((block * lfs_cfg->block_size) + off, size, buffer);
    int res = GET_INT_RETURN_VALUE(result);

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_prg -> %d", res);
    return res;
}

int lfs_spi_flash_bd_erase(const struct lfs_config *lfs_cfg, lfs_block_t block)
{
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_erase(%p, 0x%"PRIx32")", (void*)lfs_cfg, block);

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);

    uint32_t addr = block * lfs_cfg->block_size;
    cy_rslt_t result = cy_serial_flash_qspi_erase(addr, lfs_cfg->block_size);
    int res = GET_INT_RETURN_VALUE(result);

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_erase -> %d", res);
    return res;
}

/* Simply return zero because the QSPI block does not have any write cache in MMIO
 * mode.
 */
int lfs_spi_flash_bd_sync(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);

    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_sync(%p)", (void*)lfs_cfg);
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_sync -> %d", 0);
    return 0;
}

#if defined(LFS_THREADSAFE)
int lfs_spi_flash_bd_lock(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);
    return GET_INT_RETURN_VALUE(cy_rtos_get_mutex(&_spi_flash_bd_mutex, LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS));
}

int lfs_spi_flash_bd_unlock(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);
    return GET_INT_RETURN_VALUE(cy_rtos_set_mutex(&_spi_flash_bd_mutex));
}
#endif /* #if defined(LFS_THREADSAFE) */


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // CY_IP_MXSMIF
