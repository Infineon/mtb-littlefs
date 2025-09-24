/***************************************************************************//**
 * \file lfs_spi_flash_bd.c
 *
 * \brief
 * Implements the block device driver functions for SPI NOR flash for use with
 * littlefs API. Makes use of the
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
 * Based upon lfs_rambd.c from littlefs repo
 * [https://github.com/littlefs-project/littlefs/blob/master/bd/lfs_rambd.c]
 * Copyright (c) 2017, Arm Limited. All rights reserved
 *******************************************************************************/

#include "lfs_spi_flash_bd.h"
#include "lfs_util.h"
#include "mtb_serial_memory.h"

#if defined(COMPONENT_RTOS_AWARE) || defined(LFS_THREADSAFE)
#include "cyabs_rtos.h"
#endif /* #if defined(COMPONENT_RTOS_AWARE) || defined(LFS_THREADSAFE) */

/* Define if the asynchronous transfer is enabled */
#define ASYNC_TRANSFER_IS_ENABLED               (0U) /* Serial memory doesn't support asynchronous transfer. Must be 0 */

#ifdef CY_IP_MXSMIF

#if defined(LFS_THREADSAFE) /* This block of code ignores violations of Directive 4.6 MISRA. Functions lfs_spi_flash_bd_unlock and lfs_spi_flash_bd_lock don't reproduce violations if LFS_THREADSAFE not defined. */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',6,\
'The third-party defines the function interface with basic numeral type')
#else
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Directive 4.6',4,\
'The third-party defines the function interface with basic numeral type')
#endif /* #if defined(LFS_THREADSAFE) */

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

#if (ASYNC_TRANSFER_IS_ENABLED) == 1U
#define QSPI_READ_SEMA_MAX_COUNT                    (1UL)
#define QSPI_READ_SEMA_INIT_COUNT                   (0UL)

#ifndef LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS
#define LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS      (500UL) /* in milliseconds */
#endif /* #ifndef LFS_SPI_FLASH_BD_ASYNC_READ_TIMEOUT_MS */

void qspi_read_complete_callback(cy_rslt_t status, void *arg);

/* Semaphore used while waiting for the QSPI read operation to complete. */
static cy_semaphore_t qspi_read_sema;

void qspi_read_complete_callback(cy_rslt_t status, void *arg)
{
    cy_rslt_t result;

CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The void* pointer arg is cast to cy_rslt_t* to store the callback status. It is guaranteed that arg points to a memory location of type cy_rslt_t.');
    *((cy_rslt_t *) arg) = status;
    result = cy_rtos_set_semaphore(&qspi_read_sema, true);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
    CY_UNUSED_PARAMETER(result); /* To avoid compiler warning in Release mode. */
}
#endif /* #if (ASYNC_TRANSFER_IS_ENABLED) == 1U */

#if defined(LFS_THREADSAFE)

#ifndef LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS
#define LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS      (500UL)
#endif /* #ifndef LFS_SPI_FLASH_BD_GET_MUTEX_TIMEOUT_MS */

static cy_mutex_t _spi_flash_bd_mutex;
#endif /* #if defined(LFS_THREADSAFE) */

/* The static variable to safe the memory configuration */
static uint32_t lfs_spi_flash_address_start = 0U;
static uint32_t lfs_spi_flash_region_size = 0U;
static bool lfs_spi_flash_en_custom_config = false;


void lfs_spi_flash_bd_configure_memory(const struct lfs_config *lfs_cfg, uint32_t address, uint32_t region_size)
{
    CY_UNUSED_PARAMETER(lfs_cfg);
    lfs_spi_flash_en_custom_config = true;

    /* Save the address and region size to use during configuration
     * in lfs_spi_flash_bd_create */
    lfs_spi_flash_address_start = address;
    lfs_spi_flash_region_size = region_size;
}

cy_rslt_t lfs_spi_flash_bd_create(struct lfs_config *lfs_cfg, mtb_serial_memory_t *serial_memory_obj)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_create(%p, %p)", (void*)lfs_cfg, (void*)serial_memory_obj);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(NULL != serial_memory_obj);

    cy_rslt_t result = CY_RSLT_SUCCESS;

    lfs_cfg->context     = (mtb_serial_memory_t *)serial_memory_obj;

#if defined(LFS_THREADSAFE)
    if(CY_RSLT_SUCCESS == result)
    {
        /* Initialize the mutex. */
        result = cy_rtos_init_mutex(&_spi_flash_bd_mutex);
    }
#endif /* #if defined(LFS_THREADSAFE) */

    /* Block device operations */
    lfs_cfg->read        = lfs_spi_flash_bd_read;
    lfs_cfg->prog        = lfs_spi_flash_bd_prog;
    lfs_cfg->erase       = lfs_spi_flash_bd_erase;
    lfs_cfg->sync        = lfs_spi_flash_bd_sync;

#if defined(LFS_THREADSAFE)
    lfs_cfg->lock        = lfs_spi_flash_bd_lock;
    lfs_cfg->unlock      = lfs_spi_flash_bd_unlock;
#endif /* #if defined(LFS_THREADSAFE) */

    /* Block the device configuration.
        * All the blocks of the flash module are expected to be the same size.
        * As a result, we can find the program size and block size by the first
        * block. Also, if the hybrid memory is used, these parameters are
        * found for the first provided block (configured by lfs_spi_flash_bd_configure_memory()).
        */
    lfs_cfg->read_size   = QSPI_MIN_READ_SIZE;
    lfs_cfg->prog_size   = mtb_serial_memory_get_prog_size(serial_memory_obj, lfs_spi_flash_en_custom_config ?
                                                                lfs_spi_flash_address_start : 0U);
    lfs_cfg->block_size  = mtb_serial_memory_get_erase_size(serial_memory_obj, lfs_spi_flash_en_custom_config ?
                                                                lfs_spi_flash_address_start : 0U);
    lfs_cfg->block_count = (lfs_spi_flash_en_custom_config ? lfs_spi_flash_region_size :
                            mtb_serial_memory_get_size(serial_memory_obj)) / lfs_cfg->block_size;

    /* Refer to lfs.h for the description of the following parameters: */

    /* The number of erase cycles before data is moved to a new block.
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

#if (ASYNC_TRANSFER_IS_ENABLED) == 1U
    result = cy_rtos_init_semaphore(&qspi_read_sema, QSPI_READ_SEMA_MAX_COUNT, QSPI_READ_SEMA_INIT_COUNT);
#endif /* #if (ASYNC_TRANSFER_IS_ENABLED) == 1U */

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_create -> %"PRIu32"", result);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    return result;
}

void lfs_spi_flash_bd_destroy(const struct lfs_config *lfs_cfg)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_destroy(%p)", (void*)lfs_cfg);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    CY_UNUSED_PARAMETER(lfs_cfg);
    /* For some reason, the 20829 requires the deinit function in XIP mode
     * while the PSE84 device requires detach function
     */

    /* Forget the settings of the custom configuration of the memory module */
    lfs_spi_flash_en_custom_config = false;

#if (ASYNC_TRANSFER_IS_ENABLED) == 1U
    result = cy_rtos_deinit_semaphore(&qspi_read_sema);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
#endif /* #if (ASYNC_TRANSFER_IS_ENABLED) == 1U */

#if defined(LFS_THREADSAFE)
    result = cy_rtos_deinit_mutex(&_spi_flash_bd_mutex);
    LFS_ASSERT(CY_RSLT_SUCCESS == result);
#endif /* #if defined(LFS_THREADSAFE) */

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_destroy -> %d", 0);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    CY_UNUSED_PARAMETER(result); /* To avoid compiler warning in Release mode. */
}

int lfs_spi_flash_bd_read(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    cy_rslt_t result;

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_read(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT((off % lfs_cfg->read_size) == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT((size % lfs_cfg->read_size) == 0);

CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The pointer lfs_cfg->context is cast to mtb_serial_memory_t*. It is guaranteed that lfs_cfg->context points to a valid mtb_serial_memory_t instance.');
    mtb_serial_memory_t *serial_memory_obj = (mtb_serial_memory_t *)(lfs_cfg->context);

#if (ASYNC_TRANSFER_IS_ENABLED) == 1U
    cy_rslt_t qspi_read_status = CY_RSLT_SUCCESS;

    /* Disable interrupts to ensure interrupt occurs only when we are ready to
     * get the semaphore.
     */
    uint32_t saved_intr_status = mtb_hal_system_critical_section_enter();
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The void* pointer buffer is cast to uint8_t* for byte-level access. It is guaranteed that buffer points to a memory region containing uint8_t data.');
    result = mtb_serial_memory_read_async(serial_memory_obj, lfs_spi_flash_address_start + (block * lfs_cfg->block_size) + off, size, (uint8_t*)buffer, qspi_read_complete_callback, (void *)&qspi_read_status);
    mtb_hal_system_critical_section_exit(saved_intr_status);

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
    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the function interface');
    result = mtb_serial_memory_read(serial_memory_obj, lfs_spi_flash_address_start + (block * lfs_cfg->block_size) + off, size, buffer);
#endif /* #if (ASYNC_TRANSFER_IS_ENABLED) == 1U */

    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_read -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')
    return res;
}

int lfs_spi_flash_bd_prog(const struct lfs_config *lfs_cfg, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
     'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_prog(%p, "
                    "0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
                (void*)lfs_cfg, block, off, buffer, size);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);
    LFS_ASSERT(off  % lfs_cfg->prog_size == 0);
    LFS_ASSERT(NULL != buffer);
    LFS_ASSERT(size % lfs_cfg->prog_size == 0);

CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The pointer lfs_cfg->context is cast to mtb_serial_memory_t*. It is guaranteed that lfs_cfg->context points to a valid mtb_serial_memory_t instance.');
    mtb_serial_memory_t *serial_memory_obj = (mtb_serial_memory_t *)(lfs_cfg->context);

    CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5','The third-party defines the function interface');
    cy_rslt_t result = mtb_serial_memory_write(serial_memory_obj, lfs_spi_flash_address_start + (block * lfs_cfg->block_size) + off, size, buffer);
    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_prg -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    return res;
}

int lfs_spi_flash_bd_erase(const struct lfs_config *lfs_cfg, lfs_block_t block)
{
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_erase(%p, 0x%"PRIx32")", (void*)lfs_cfg, block);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    /* Check if parameters are valid. */
    LFS_ASSERT(NULL != lfs_cfg);
    LFS_ASSERT(block < lfs_cfg->block_count);

CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.5', 'The pointer lfs_cfg->context is cast to mtb_serial_memory_t*. It is guaranteed that lfs_cfg->context points to a valid mtb_serial_memory_t instance.');
    mtb_serial_memory_t *serial_memory_obj = (mtb_serial_memory_t *)(lfs_cfg->context);

    uint32_t addr = block * lfs_cfg->block_size;
    cy_rslt_t result = mtb_serial_memory_erase(serial_memory_obj, lfs_spi_flash_address_start + addr, lfs_cfg->block_size);
    int32_t res = GET_INT_RETURN_VALUE(result);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',1,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 21.6','Using the safe wrapper of printf');
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_erase -> %d", (int)res);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

    return res;
}

/* Simply return zero because the QSPI block does not have any write cache in MMIO
 * mode.
 */

int lfs_spi_flash_bd_sync(const struct lfs_config *lfs_cfg)
{
    CY_UNUSED_PARAMETER(lfs_cfg);

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.7',2,\
    'Impossible to cast due-to the macros wrapper of printf')
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 21.6',2,\
    'Using the safe wrapper of printf')
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_sync(%p)", (void*)lfs_cfg);
    LFS_SPI_FLASH_BD_TRACE("lfs_spi_flash_bd_sync -> %d", 0);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 21.6')
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.7')

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

CY_MISRA_BLOCK_END('MISRA C-2012 Directive 4.6')

#endif // CY_IP_MXSMIF
