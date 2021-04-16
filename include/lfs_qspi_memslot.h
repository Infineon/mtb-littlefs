/*******************************************************************************
* File Name: lfs_qspi_memslot.h
*
* Description:
* Provides declarations of the SMIF-driver memory configuration.
* This file was automatically generated and should not be modified.
* QSPI Configurator 2.20.0.3018
*
********************************************************************************
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
 ******************************************************************************/

#ifndef LFS_QSPI_MEMSLOT_H
#define LFS_QSPI_MEMSLOT_H
#include "cy_smif_memslot.h"

#define CY_SMIF_CFG_TOOL_VERSION           (220)

/* Supported QSPI Driver version */
#define CY_SMIF_DRV_VERSION_REQUIRED       (100)

#if !defined(CY_SMIF_DRV_VERSION)
    #define CY_SMIF_DRV_VERSION            (100)
#endif

/* Check the used driver version */
#if (CY_SMIF_DRV_VERSION_REQUIRED > CY_SMIF_DRV_VERSION)
   #error The QSPI Configurator requires a newer version of the PDL. Update the PDL in your project.
#endif

#define CY_SMIF_DEVICE_NUM 1

extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_readCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_writeEnCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_writeDisCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_eraseCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_chipEraseCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_programCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_readStsRegQeCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_readStsRegWipCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_writeStsRegQeCmd;
extern cy_stc_smif_mem_cmd_t LFS_SFDP_SlaveSlot_0_readSfdpCmd;

extern cy_stc_smif_mem_device_cfg_t LFS_DeviceCfg_SFDP_SlaveSlot_0;

extern cy_stc_smif_mem_config_t LFS_SFDP_SlaveSlot_0;
extern const cy_stc_smif_mem_config_t* const LFS_SmifMemConfigsSfdp[CY_SMIF_DEVICE_NUM];


#endif /* LFS_QSPI_MEMSLOT_H */
