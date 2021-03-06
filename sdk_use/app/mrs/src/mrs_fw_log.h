//*****************************************************************************
//
//                  版权所有 (C), 2001-2011, 华为技术有限公司
//
//*****************************************************************************
//  文 件 名   : mrs_fw_log.h
//  版 本 号   : V1.0 
//  作    者   : 聂松松/KF62735
//  生成日期   : 2012-06-13
//  功能描述   : 打印日志
//  函数列表   : NA
//  修改历史   : 
//  1.日    期 : 2012-06-13
//    作    者 : 聂松松/KF62735
//    修改内容 : 创建文件 
//*****************************************************************************
#ifndef __MRS_FW_LOG_H__
#define __MRS_FW_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(SAL_HAVE_OS_VX_VER)
#ifndef HAVE_PCLINT_CHECK
#include <stdio.h>
#endif
#endif/*SAL_HAVE_OS_VX_VER*/

//是否支持变参
#define MRS_SUPPROT_VARIADIC_MACROS     1
//打印16进制（通道）
#define MRS_SRV_CHL_PRINT_HEX_ON        1

//MOD define
#define MRS_MOD_CHL   HI_MOD_ID_APP_MRS_CHL
#define MRS_MOD_PROTO HI_MOD_ID_APP_MRS_PROTO
#define MRS_MOD_CCO   HI_MOD_ID_APP_MRS_CCO
#define MRS_MOD_STA   HI_MOD_ID_APP_MRS_STA
#define MRS_MOD_DFX   HI_MOD_ID_APP_DFX
#define MRS_MOD_MAP   HI_MOD_ID_APP_MRS_MAP
#define MRS_MOD_RM    HI_MOD_ID_APP_MRS_RM

#define MRS_SRV_PROTO_LOG_ON            1
#define MRS_SRV_STA_LOG_ON              1
#define MRS_SRV_CCO_LOG_ON              1
#define MRS_SRV_MAP_LOG_ON              1
#define MRS_SRV_SESSION_LOG_ON          1
#define MRS_SRV_TOOLS_LOG_ON            1
#define MRS_SRV_RM_LOG_ON               1
#define MRS_SRV_SM_LOG_ON               1
#define MRS_SRV_UPG_LOG_ON              1

#define MRS_SRV_II_COLLETOR_LOG_ON      1

#define  MRS_LOG_ERROR      HI_DIAG_PRINT_LEVEL_ERROR
#define  MRS_LOG_WARNING    HI_DIAG_PRINT_LEVEL_WARNING
#define  MRS_LOG_INFO       HI_DIAG_PRINT_LEVEL_INFO
#define  MRS_LOG_NORMAL     HI_DIAG_PRINT_LEVEL_NORMAL

typedef enum
{
    MRS_FILE_LOG_FLAG_001 = 1,
    MRS_FILE_LOG_FLAG_002,
    MRS_FILE_LOG_FLAG_003,
    MRS_FILE_LOG_FLAG_004,
    MRS_FILE_LOG_FLAG_005,
    MRS_FILE_LOG_FLAG_006,
    MRS_FILE_LOG_FLAG_007,
    MRS_FILE_LOG_FLAG_008,
    MRS_FILE_LOG_FLAG_009,
    MRS_FILE_LOG_FLAG_010,
    MRS_FILE_LOG_FLAG_011,
    MRS_FILE_LOG_FLAG_012,
    MRS_FILE_LOG_FLAG_013,
    MRS_FILE_LOG_FLAG_014,
    MRS_FILE_LOG_FLAG_015,
    MRS_FILE_LOG_FLAG_016,
    MRS_FILE_LOG_FLAG_017,
    MRS_FILE_LOG_FLAG_018,
    MRS_FILE_LOG_FLAG_019,
    MRS_FILE_LOG_FLAG_020,
    MRS_FILE_LOG_FLAG_021,
    MRS_FILE_LOG_FLAG_022,
    MRS_FILE_LOG_FLAG_023,
    MRS_FILE_LOG_FLAG_024,
    MRS_FILE_LOG_FLAG_025,
    MRS_FILE_LOG_FLAG_026,
    MRS_FILE_LOG_FLAG_027,
    MRS_FILE_LOG_FLAG_028,
    MRS_FILE_LOG_FLAG_029,
    MRS_FILE_LOG_FLAG_030,
    MRS_FILE_LOG_FLAG_031,
    MRS_FILE_LOG_FLAG_032,
    MRS_FILE_LOG_FLAG_033,
    MRS_FILE_LOG_FLAG_034,
    MRS_FILE_LOG_FLAG_035,
    MRS_FILE_LOG_FLAG_036,
    MRS_FILE_LOG_FLAG_037,
    MRS_FILE_LOG_FLAG_038,
    MRS_FILE_LOG_FLAG_039,
    MRS_FILE_LOG_FLAG_040,
    MRS_FILE_LOG_FLAG_041,
    MRS_FILE_LOG_FLAG_042,
    MRS_FILE_LOG_FLAG_043,
    MRS_FILE_LOG_FLAG_044,
    MRS_FILE_LOG_FLAG_045,
    MRS_FILE_LOG_FLAG_046,
    MRS_FILE_LOG_FLAG_047,
    MRS_FILE_LOG_FLAG_048,
    MRS_FILE_LOG_FLAG_049,
    MRS_FILE_LOG_FLAG_050,
    MRS_FILE_LOG_FLAG_051,
    MRS_FILE_LOG_FLAG_052,
    MRS_FILE_LOG_FLAG_053,
    MRS_FILE_LOG_FLAG_054,
    MRS_FILE_LOG_FLAG_055,
    MRS_FILE_LOG_FLAG_056,
    MRS_FILE_LOG_FLAG_057,
    MRS_FILE_LOG_FLAG_058,
    MRS_FILE_LOG_FLAG_059,
    MRS_FILE_LOG_FLAG_060,
    MRS_FILE_LOG_FLAG_061,
    MRS_FILE_LOG_FLAG_062,
    MRS_FILE_LOG_FLAG_063,
    MRS_FILE_LOG_FLAG_064,
    MRS_FILE_LOG_FLAG_065,
    MRS_FILE_LOG_FLAG_066,
    MRS_FILE_LOG_FLAG_067,
    MRS_FILE_LOG_FLAG_068,
    MRS_FILE_LOG_FLAG_069,
    MRS_FILE_LOG_FLAG_070,
    MRS_FILE_LOG_FLAG_071,
    MRS_FILE_LOG_FLAG_072,
    MRS_FILE_LOG_FLAG_073,
    MRS_FILE_LOG_FLAG_074,
    MRS_FILE_LOG_FLAG_075,
    MRS_FILE_LOG_FLAG_076,
    MRS_FILE_LOG_FLAG_077,
    MRS_FILE_LOG_FLAG_078,
    MRS_FILE_LOG_FLAG_079,
    MRS_FILE_LOG_FLAG_080,
    MRS_FILE_LOG_FLAG_081,
    MRS_FILE_LOG_FLAG_082,
    MRS_FILE_LOG_FLAG_083,
    MRS_FILE_LOG_FLAG_084,
    MRS_FILE_LOG_FLAG_085,
    MRS_FILE_LOG_FLAG_086,
    MRS_FILE_LOG_FLAG_087,
    MRS_FILE_LOG_FLAG_088,
    MRS_FILE_LOG_FLAG_089,
    MRS_FILE_LOG_FLAG_090,
    MRS_FILE_LOG_FLAG_091,
    MRS_FILE_LOG_FLAG_092,
    MRS_FILE_LOG_FLAG_093,
    MRS_FILE_LOG_FLAG_094,
    MRS_FILE_LOG_FLAG_095,
    MRS_FILE_LOG_FLAG_096,
    MRS_FILE_LOG_FLAG_097,
    MRS_FILE_LOG_FLAG_098,
    MRS_FILE_LOG_FLAG_099,
    }MRS_FILE_LOG_FLAG_ENUM;

typedef enum
{
    MRS_FILE_LOG_BUF_1000 = 1000,
    MRS_FILE_LOG_BUF_1001,
    MRS_FILE_LOG_BUF_1002,
    MRS_FILE_LOG_BUF_1003,
    MRS_FILE_LOG_BUF_1004,
    MRS_FILE_LOG_BUF_1005,
    MRS_FILE_LOG_BUF_1006,
    MRS_FILE_LOG_BUF_1007,
    MRS_FILE_LOG_BUF_1008,
    MRS_FILE_LOG_BUF_1009,
    MRS_FILE_LOG_BUF_1010,
    MRS_FILE_LOG_BUF_1011,
    MRS_FILE_LOG_BUF_1012,
    MRS_FILE_LOG_BUF_1013,
    MRS_FILE_LOG_BUF_1014,
    MRS_FILE_LOG_BUF_1015,
    MRS_FILE_LOG_BUF_1016,
    MRS_FILE_LOG_BUF_1017,
    MRS_FILE_LOG_BUF_1018,
    MRS_FILE_LOG_BUF_1019,
    MRS_FILE_LOG_BUF_1020,
    MRS_FILE_LOG_BUF_1021,
    MRS_FILE_LOG_BUF_1022,
}MRS_FILE_LOG_BUF_ENUM;


#ifdef __cplusplus
}
#endif


#endif
