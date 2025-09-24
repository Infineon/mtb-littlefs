/***************************************************************************//**
 * \file lfs_sd_bd.c
 *
 * \brief
 * Implements the block device driver functions for SD card (card mode) for use
 * with littlefs API. Makes use of the Secure Digital Host Controller (SDHC) HAL
 * driver.
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
 * Based upon lfs_rambd.c from littlefs repo
 * [https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.c]
 * Copyright (c) 2017, Arm Limited. All rights reserved
 *******************************************************************************/

#include "lfs_sd_bd.h"
#include "lfs_util.h"
#include "mtb_hal_sdhc.h"

#if defined(LFS_THREADSAFE)
#include "cyabs_rtos.h"
#endif /* #if defined(LFS_THREADSAFE) */

#ifdef CY_IP_MXSDHC

#if defined(LFS_THREADSAFE) /* This block of code ignores violations of Directive 4.6 MISRA. Functions lfs_spi_flash_bd_unlock and lfs_spi_flash_bd_lock don't reproduce violations if LFS_THREADSAFE not defined. */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',7,\
'The third-party defines the function interface with basic numeral type')
#else
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',5,\
'The third-party defines the function interface with basic numeral type')
#endif /* #if defined(LFS_THREADSAFE) */

#if defined(__cplusplus)
extern "C"
{
#endif

#define RESULT_OK                           (0)
#define RESULT_ERROR                        (-1)
#define GET_INT_RETURN_VALUE(result)        ((CY_RSLT_SUCCESS == (result)) ? RESULT_OK : RESULT_ERROR)

#define LFS_CFG_LOOKAHEAD_SIZE_MIN          (64UL)    /* Must be a multiple of 8 */
#define SDHC_BLOCK_SIZE                     (512UL)
#define ONE_BLOCK                           (1U)

#if defined(LFS_THREADSAFE)
#ifndef LFS_SD_BD_GET_MUTEX_TIMEOUT_MS
#define LFS_SD_BD_GET_MUTEX_TIMEOUT_MS      (500UL)
#endif /* #ifndef LFS_SD_BD_GET_MUTEX_TIMEOUT_MS */

static cy_mutex_t _sd_bd_mutex;
#endif /* #if defined(LFS_THREADSAFE) */

static int _erase(const struct lfs_config *lfs_cfg, lfs_block_t block);

cy_rslt_t lfs_sd_bd_create(struct lfs_config *lfs_cfg, const mtb_hal_sdhc_t *sdhc_obj)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_create(%p, %p)", (void*)lfs_cfg, (void*)sdhc_obj);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(NULL != sdhc_obj);

#if defined(LFS_THREADSAFE)
    /* Initialize the mutex. */
    result = cy_rtos_init_mutex(&_sd_bd_mutex);
#endif /* #if defined(LFS_THREADSAFE) */

#if defined(LFS_THREADSAFE)
    if(CY_RSLT_SUCCESS == result)
    {
#endif /* #if defined(LFS_THREADSAFE) */
        lfs_cfg->context     = (mtb_hal_sdhc_t *)sdhc_obj;

        /* Block device operations */
        lfs_cfg->read        = lfs_sd_bd_read;
        lfs_cfg->prog        = lfs_sd_bd_prog;

        /* SD/MMC cards do not require explicit erase. The card controllers
         * perform erase internally during the write. Therefore, a function that
         * simply returns zero is used for erase().
         */
        lfs_cfg->erase       = _erase;
        lfs_cfg->sync        = lfs_sd_bd_sync;

#if defined(LFS_THREADSAFE)
        lfs_cfg->lock        = lfs_sd_bd_lock;
        lfs_cfg->unlock      = lfs_sd_bd_unlock;
#endif /* #if defined(LFS_THREADSAFE) */

        /* Block device configuration */
        lfs_cfg->read_size   = SDHC_BLOCK_SIZE;
        lfs_cfg->prog_size   = SDHC_BLOCK_SIZE;
        lfs_cfg->block_size  = SDHC_BLOCK_SIZE;

        result = mtb_hal_sdhc_get_block_count((mtb_hal_sdhc_t *) sdhc_obj, (uint32_t *) &lfs_cfg->block_count);
        if(CY_RSLT_SUCCESS == result)
        {
            /* Refer to lfs.h for the description of the following parameters. */

            /* Set to -1 to disable wear leveling as the controller in the
             * SD card may be handling wear leveling.
             */
            lfs_cfg->block_cycles = -1;

            /* cache_size must be a multiple of prog & read sizes.
             * i.e., cache_size % prog_size = 0 and cache_size % read_size = 0
             * block_size must be a multiple of cache_size. i.e., block_size % cache_size = 0.
             *
             * littlefs allocates 1 cache for each file and 2 caches for
             * internal operations.
             * The higher the cache size, the better the performance is, but the
             * RAM consumption is also higher.
             */
            lfs_cfg->cache_size = SDHC_BLOCK_SIZE;

            /* A larger Lookahead size reduces the number of scans performed by
             * the block allocation algorithm thus increasing the filesystem
             * performance but results in higher RAM consumption.
             *
             * Must be a multiple of 8.
             */
            lfs_cfg->lookahead_size = lfs_min((lfs_size_t) LFS_CFG_LOOKAHEAD_SIZE_MIN, 8UL * ((lfs_cfg->block_count + 63UL)/64UL) );
        }
#if defined(LFS_THREADSAFE)
    }
#endif /* #if defined(LFS_THREADSAFE) */

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_create -> %"PRIu32"", result);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return result;
}

void lfs_sd_bd_destroy(const struct lfs_config *lfs_cfg)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_destroy(%p)", (void*)lfs_cfg);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    LFS_ASSERT(NULL != lfs_cfg);

    (void) lfs_cfg;

#if defined(LFS_THREADSAFE)
    cy_rslt_t result = CY_RSLT_SUCCESS;
    result = cy_rtos_deinit_mutex(&_sd_bd_mutex);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
    CY_UNUSED_PARAMETER(result); /* To avoid compiler warning in Release mode. */
#endif /* #if defined(LFS_THREADSAFE) */

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_destroy -> %d", 0);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
}

int lfs_sd_bd_read(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_read(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT((off % lfs_cfg->read_size) == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT((size % lfs_cfg->read_size) == 0);

    size_t block_count =  size / lfs_cfg->block_size;
    uint32_t addr = block + (off / lfs_cfg->block_size);
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the typr of "context"');
    mtb_hal_sdhc_t *sdhc_obj = (mtb_hal_sdhc_t *)(lfs_cfg->context);

    /* addr represents the block number at which read should begin */
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the function interface');
    
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The void* pointer buffer is cast to uint8_t* for byte-level access. It is guaranteed that buffer points to a memory region containing uint8_t data.');
    cy_rslt_t result = mtb_hal_sdhc_read_async(sdhc_obj, addr, (uint8_t*)buffer, &block_count);
    if(CY_RSLT_SUCCESS == result)
    {
        /* Waits on a semaphore until the transfer completes, when RTOS_AWARE component is defined. */
        result = mtb_hal_sdhc_wait_transfer_complete(sdhc_obj);
    }

    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_read -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return res;
}

int lfs_sd_bd_prog(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
        'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_prog(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT(off % lfs_cfg->prog_size == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT(size % lfs_cfg->prog_size == 0);

    size_t block_count =  size / lfs_cfg->block_size;
    uint32_t addr = block + (off / lfs_cfg->block_size);
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the typr of "context"');
    mtb_hal_sdhc_t *sdhc_obj = (mtb_hal_sdhc_t *)(lfs_cfg->context);

    /* Addr represents the block number at which write should begin */
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the function interface');

CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The void* pointer buffer is cast to const uint8_t* for byte-level access. It is guaranteed that buffer points to a memory region containing const uint8_t data.');
    cy_rslt_t result = mtb_hal_sdhc_write_async(sdhc_obj, addr, (const uint8_t*)buffer, &block_count);

    if(CY_RSLT_SUCCESS == result)
    {
        /* Waits on a semaphore until the transfer completes, when RTOS_AWARE component is defined. */
        result = mtb_hal_sdhc_wait_transfer_complete(sdhc_obj);
    }

    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_prg -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return res;
}

/* SD/MMC cards do not require explicit erase. The card controllers perform
 * erase internally during the write. Therefore, this function simply returns
 * zero and is used with the erase() of lfs_config structure.
 */

static int _erase(const struct lfs_config *lfs_cfg, lfs_block_t block)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_erase(%p, 0x%"PRIx32")", (void*)lfs_cfg, block);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    CY_UNUSED_PARAMETER(lfs_cfg);
    CY_UNUSED_PARAMETER(block);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_erase -> %d", 0);

CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return 0;
}

int lfs_sd_bd_erase(const struct lfs_config *lfs_cfg, lfs_block_t block)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_erase(%p, 0x%"PRIx32")", (void*)lfs_cfg, block);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);

    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the typr of "context"');
    mtb_hal_sdhc_t *sdhc_obj = (mtb_hal_sdhc_t *)(lfs_cfg->context);

    cy_rslt_t result = mtb_hal_sdhc_erase(sdhc_obj, block, ONE_BLOCK, 0U);
    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SD_BD_TRACE("lfs_sd_bd_erase -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return res;
}

/* Simply return zero because the SDHC block does not have any write cache. */

int lfs_sd_bd_sync(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',2,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 21.6',2,\
    'Using the safe wrapper of printf')
    LFS_SD_BD_TRACE("lfs_sd_bd_sync(%p)", (void*)lfs_cfg);
    LFS_SD_BD_TRACE("lfs_sd_bd_sync -> %d", 0);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 21.6')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return 0;
}

#if defined(LFS_THREADSAFE)

int lfs_sd_bd_lock(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);
    return GET_INT_RETURN_VALUE(cy_rtos_get_mutex(&_sd_bd_mutex, LFS_SD_BD_GET_MUTEX_TIMEOUT_MS));
}

int lfs_sd_bd_unlock(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);
    return GET_INT_RETURN_VALUE(cy_rtos_set_mutex(&_sd_bd_mutex));
}
#endif /* #if defined(LFS_THREADSAFE) */


#ifdef __cplusplus
} /* extern "C" */
#endif

CY_MISRA_BLOCK_END('MISRA C-2012 Directive 4.6')

#endif /* CY_IP_MXSDHC */