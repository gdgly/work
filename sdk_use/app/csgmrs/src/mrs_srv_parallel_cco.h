//*****************************************************************************
//
//                  版权所有 (C), 1998-2011, 华为技术有限公司
//
//*****************************************************************************
//  文 件 名   : mrs_srv_parallel_cco.h
//  版 本 号   : V1.0 
//  作    者   : liuxipeng/KF54842
//  生成日期   : 2012-12-22
//  功能描述   : 并发读表模块函数及结构声明
//               
//  函数列表   :
//  修改历史   : 
//  1.日    期 : 2012-12-22
//    作    者 : liuxipeng/KF54842
//    修改内容 : 创建文件 
//
//*****************************************************************************

//*****************************************************************************
// PROJECT   : 
// SUBSYSTEM :
// MODULE    :  
// OWNER     :  
//*****************************************************************************

#ifndef __MRS_SRV_PARALLEL_CCO_H__
#define __MRS_SRV_PARALLEL_CCO_H__

HI_START_HEADER
#if defined(PRODUCT_CFG_PRODUCT_TYPE_CCO)

// 并发读表初始化(必须在读表管理模块初始化后执行)
HI_PUBLIC HI_U32 mrsParallelReadMeterInit(HI_VOID);

#if defined(PRODUCT_CFG_PRODUCT_TYPE_CSG_GD)
// 广东电网并发数据处理
HI_PUBLIC HI_U32 mrsParallelReadMeterProcCsg(HI_U8 *pucFrame, HI_U16 usLength, HI_U8 *pucAddr, HI_PVOID pvParam);
#else
// 集中器下行并发数据处理
HI_PUBLIC HI_U32 mrsParallelReadMeterProc(HI_IN HI_U8, HI_IN HI_U8, HI_IN HI_U8 *, HI_IN HI_U16);
#endif

#endif
HI_END_HEADER

#endif //__MRS_SRV_PARALLEL_H__
