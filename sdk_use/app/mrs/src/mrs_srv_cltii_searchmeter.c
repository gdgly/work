//*****************************************************************************
//
//                  版权所有 (C), 1998-2013, 华为技术有限公司
//
//*****************************************************************************
//  文 件 名   : mrs_srv_cltii_searchmeter.c
//  版 本 号   : V1.0 
//  作    者   : cuiate/00233580
//  生成日期   : 2014-08-13
//  功能描述   : II采搜表函数及接口实现
//               
//  函数列表   : TODO: ...
//  修改历史   : 
//  1.日    期 : 2014-08-13
//    作    者 : cuiate/00233580
//    修改内容 : 创建文件 
//
//*****************************************************************************

#include "mrs_common.h"
#include "mrs_cmd_msg.h"
#include "mrs_srv_common.h"
#include "mrs_fw_tools.h"
#include "mrs_fw_log.h"
#include "mrs_srv_sta_queue.h"
#include "mrs_srv_parallel_sta.h"
#include "mrs_srv_sta_event.h"
#include "mrs_srv_sta.h"
#include "mrs_srv_sta_searchmeter.h"
#include "mrs_srv_cltii_searchmeter.h"
#include "mrs_srv_cltii_searchmeter_n.h"
#include "mrs_srv_collector.h"
#include "mrs_srv_baudrate_manage.h"
#include "mrs_dfx_clt.h"
#include "equip_dut_proc.h"
#include "mrs_fw_proto698_45.h"
#include "mrs_srv_cltii_power_on_off.h"


#if defined(PRODUCT_CFG_PRODUCT_TYPE_STA)


HI_U32 mrsCltiiSearchStart(MRS_STA_SEARCH_CTX *pstCtx, HI_BOOL bTryFirstFlag)
{
    HI_BOOL bTryFlag = bTryFirstFlag;

    mrsStaSearchSetFirstFlag(HI_FALSE);
    mrsStaSearchSetStatus(MRS_SEARCH_STATUS_IN_PROGRESS);
    mrsStaSearchSetEndReason(MRS_SEARCH_END_REASON_DEFAULT);

    MRS_StopTimer(MRS_STA_TIMER_SM_PERIOD_NO_METER);
    
    if (mrsStaGetSupportVM())
    {
        return mrsCltiiVMSearch();
    }

    if ((HI_ERR_SUCCESS != mrsCltiiSearchCtxInit(pstCtx)) 
        || (MRS_SEARCH_STATUS_POWER_OFF_RM == mrsStaSearchGetStatus()))
    {
        return HI_ERR_FAILURE;
    }

    mrsCollectorSetRunLedTime(MRS_COLLECTOR_RUN_LED_SEARCHMETER);
    HI_MDM_LED_SetSearchMeterStatus(LED_SEARCH_METER_BEGIN);

    mrsStaSearchClearMeterList(pstCtx);

    mrsStaSearchStartNotify();

    do
    {
        if (!bTryFlag)
        {
            break;
        }

        if (HI_ERR_SUCCESS != mrsCltiiCheckNvMeterParam(pstCtx))
        {
            break;
        }

        if (HI_ERR_SUCCESS != mrsCltiiInitFirstFrame(pstCtx))
        {
            break;
        }

        mrsStaSearchSetFirstFlag(HI_TRUE);
        mrsCltiiSearchResetRunning(pstCtx);

        return mrsCltiiSearchSendFrame(pstCtx);   
    } while (0);

    return mrsCltiiSearchStartPortal(pstCtx);
}


HI_U32 mrsCltiiSearchStop(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_StopTimer(MRS_STA_TIMER_SM_FRAME);
    MRS_StopTimer(MRS_STA_TIMER_SM_BYTE);
    MRS_StopTimer(MRS_STA_TIMER_SM_VM);

    mrsCollectorSetRunLedTime(MRS_COLLECTOR_RUN_LED_NORMAL);
    HI_MDM_LED_SetSearchMeterStatus(LED_SEARCH_METER_END);

    mrsStaSearchSetStatus(MRS_SEARCH_STATUS_IDLE);

    pstCtx->stRunning.unClt.stCltII.bFrameTimerFlag = HI_FALSE;
    pstCtx->stRunning.unClt.stCltII.bByteTimerFlag = HI_FALSE;

    // 当前已搜到的表插入NV
    mrsStaRefreshSearchResultNv(pstCtx, HI_FALSE);

    mrsStaSearchFinishNotify(MRS_SEARCH_FINISH_ABNORMAL);

    // 从未搜到过表，起一个10分钟定时器，到时间时开始搜表
    if (HI_TRUE != pstCtx->stMeterList.bMeterFlag)
    {
    	mrsAssetNumberAsMeterInsert();
        MRS_StartTimer(MRS_STA_TIMER_SM_PERIOD_NO_METER, MRS_STA_TIME_SM_PERIOD_NO_METER, HI_SYS_TIMER_ONESHOT);
    }

    {
        MRS_STA_SRV_CTX_STRU *pstSta = mrsStaGetContext();
        MRS_STA_ITEM  *pstItem = HI_NULL;

        pstItem = mrsSrvQueueTop(&(pstSta->stQueModule.stMrQueue));
        if (pstItem && (MRS_STA_ITEM_ID_SEARCH_METER_CLT_II == pstItem->id))
        {
            mrsStaTryDeQueue(&pstSta->stQueModule, mrsStaQueueFree);
        }
    }

    mrsSrvEmptyQueue(&pstCtx->stRunning.unClt.stCltII.stQueue, mrsStaQueueFree);
    
    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiSearchStartPortal(MRS_STA_SEARCH_CTX* pstCtx)
{
    mrsStaSearchLoadNvResult(&pstCtx->stNvMeterList);
    
    if (mrsCltiiSearchIsNeedDetect(pstCtx))
    {
        return mrsCltiiSearchDetectStart(pstCtx);
    }

    return mrsCltiiSearchLoopStart(pstCtx);
}


HI_VOID  mrsCltiiSearchResetRunning(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    pstRunning->ucRetry = 0;
    pstRunning->bFrameTimerFlag = HI_FALSE;
    pstRunning->bByteTimerFlag = HI_FALSE;
    pstRunning->ucValidFrame = 0;
    pstRunning->ucInvalidFrame = 0;
}


HI_VOID mrsCltiiSearchSetState(HI_U8 ucState)
{
    MRS_STA_SEARCH_CTX * pstCtx = mrsStaSearchCtx();
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    pstRunning->ucSearchState = ucState;
}


HI_U8 mrsCltiiSearchGetState(HI_VOID)
{
    MRS_STA_SEARCH_CTX * pstCtx = mrsStaSearchCtx();
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    return (pstRunning->ucSearchState);
}


HI_BOOL mrsCltiiSearchIsNeedDetect(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_NV_METERLIST_STRU* pstNvList = &(pstCtx->stNvMeterList);

    return (pstNvList->ucTotal > 0);
}


HI_BOOL mrsCltiiSearchIsAllDetected(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_NV_METERLIST_STRU* pstNvList = &(pstCtx->stNvMeterList);

    return (pstNvList->ucDetectCount >= pstNvList->ucTotal);
}


HI_U32 mrsCltiiSearchDetectStart(MRS_STA_SEARCH_CTX* pstCtx)
{    
    mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_DETECT);
    return mrsCltiiSearchDetectProc(pstCtx);
}


HI_U32 mrsCltiiSearchDetectProc(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_NV_METERLIST_STRU* pstNvList = &(pstCtx->stNvMeterList);
    NV_APP_MRS_SM_NODE* pstNode = &pstNvList->astNvMeter[pstNvList->ucDetectCount];
    HI_U32 ulDI = mrsCltiiSearchGetNodeDI(pstCtx, pstNode);;

    pstNvList->ucDetectProtocol = mrsStaSearchGetNodeProtocol(pstNode);
    pstNvList->ucBaudrateIndex = mrsCltiiSearchGetNodeBaudRateIndex(pstNode->ucParam);
    pstNvList->ucDetectCount++;
    
    mrsCltiiInitSearchFrame(pstCtx, ulDI, pstNvList->ucDetectProtocol);
    mrsCltiiCreateSearchFrame(pstCtx, pstNode->aucAddr, pstNvList->ucDetectProtocol);
    
    mrsCltiiSearchResetRunning(pstCtx);
    
    return mrsCltiiSearchSendFrame(pstCtx);
}


HI_U16 mrsCltiiGetCurLoopBaudrate(MRS_SRV_QUEUE *pQueue)
{
    MRS_SEARCH_NODE *pstNode = (MRS_SEARCH_NODE *)mrsSrvQueueTop(pQueue);
    if (pstNode)
    {
        return pstNode->stItem.usBaudrate;
    }

    return MRS_SRV_BAUD_RATE_DEFAULT;
}


HI_U16 mrsCltiiGetSearchBaudRate(MRS_STA_SEARCH_CTX *pstCtx)
{
    HI_U8 ucState = mrsCltiiSearchGetState();
    HI_U16 usBaudRate = MRS_SRV_BAUD_RATE_DEFAULT;

    switch (ucState)
    {
    case MRS_CLTII_SEARCH_STATE_TRYFISRT:
        (HI_VOID)mrsSrvIndex2BaudRate(pstCtx->stSearchCfg.ucBaudRateIndex, &usBaudRate);
        break;

    case MRS_CLTII_SEARCH_STATE_DETECT:
        (HI_VOID)mrsSrvIndex2BaudRate(pstCtx->stNvMeterList.ucBaudrateIndex, &usBaudRate);
        break;

    default:
        usBaudRate = mrsCltiiGetCurLoopBaudrate(&(pstCtx->stRunning.unClt.stCltII.stQueue));
        break;        
    }

    return usBaudRate;
}


HI_U32 mrsCltiiSend2Meter(HI_PBYTE pstBuf, HI_U16 usLen)
{
    MRS_STA_SEARCH_CTX *pstCtx = mrsStaSearchCtx();
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_U32 ulBaudrate = 0;
    HI_U32 ulTimeout = 0;
    HI_U32 ulRet = HI_ERR_SUCCESS;

    if ((HI_NULL == pstBuf) || (0 == usLen))
    {
        return HI_ERR_INVALID_PARAMETER;
    }

    if (mrsStaSearchGetStatus() != MRS_SEARCH_STATUS_IN_PROGRESS)
    {
        MRS_STA_SRV_CTX_STRU *pstSta = mrsStaGetContext();
        mrsStaTryDeQueue(&pstSta->stQueModule, mrsStaQueueFree);
        return HI_ERR_INVALID_PERIOD;
    }

    // 1. 清空Rx Buf 
    (hi_void)memset_s(&(pstCtx->stRx), sizeof(pstCtx->stRx), 0, sizeof(pstCtx->stRx));

    // 2. 复位搜表上下文状态参数
    pstRunning->bFrameTimerFlag = HI_FALSE;
    pstRunning->bByteTimerFlag = HI_FALSE;

    ulTimeout = pstCtx->stSearchCfg.usFrameTimeout;
    pstRunning->bFrameTimerFlag = HI_TRUE;
    
    // 3. 切换串口波特率
    ulBaudrate = (HI_U32)mrsCltiiGetSearchBaudRate(pstCtx);
    MRS_ChangeBaudRate(ulBaudrate, MRS_SRV_UART_PARITY_EVEN);

    ulTimeout = pstCtx->stSearchCfg.usFrameTimeout;
    ulTimeout += MRS_GET_UART_SEND_TIME(ulBaudrate, usLen);

    // 4. 发送数据给电表
    ulRet = MRS_SendMrData(pstBuf, usLen, HI_DMS_CHL_UART_PORT_APP);
    mrsDfxCltiiSetLocalTxStats(mrsCltiiSearchTxAddr(pstCtx), mrsCltiiSearchTxProtocol(pstCtx), 
                               mrsCltiiSearchTxDI(pstCtx), ulBaudrate);

    MRS_StartTimer(MRS_STA_TIMER_SM_FRAME, ulTimeout, HI_SYS_TIMER_ONESHOT);
    
    return ulRet;
}


HI_U32 mrsCltiiSearchRx(HI_DMS_CHL_RX_S* pstRx)
{
    MRS_STA_SEARCH_CTX *pstCtx = mrsStaSearchCtx();
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    if (!pstRx)
    {
        return HI_ERR_INVALID_PARAMETER;
    }

    if ((!pstRunning->bFrameTimerFlag) && (!pstRunning->bByteTimerFlag))
    {
        return HI_ERR_FAILURE;
    }
    
    mrsDfxCltiiSetFrameTimeStats();

    if ((pstCtx->stRx.ulSize + pstRx->usPayloadLen) > MRS_STA_FRAME645_BUFSIZE_MAX)
    {
        return HI_ERR_INVALID_PARAMETER;
    }

    MRS_StopTimer(MRS_STA_TIMER_SM_BYTE);
    pstRunning->bByteTimerFlag = HI_FALSE;

    (hi_void)memcpy_s(pstCtx->stRx.aucBuf + pstCtx->stRx.ulSize, 
        sizeof(pstCtx->stRx.aucBuf) - pstCtx->stRx.ulSize,
        pstRx->pPayload, pstRx->usPayloadLen);
    pstCtx->stRx.ulSize += pstRx->usPayloadLen;

    MRS_StartTimer(MRS_STA_TIMER_SM_BYTE, pstCtx->stSearchCfg.ucByteTimeout, HI_SYS_TIMER_ONESHOT);
    pstRunning->bByteTimerFlag = HI_TRUE;

    return HI_ERR_SUCCESS;
}

HI_U32 mrsCltiiSearchDetectRxProc(HI_U8 *pData, HI_U16 usLen)
{
    MRS_STA_SEARCH_CTX *pstCtx = mrsStaSearchCtx();
    
    if (!pData || (usLen > MRS_STA_FRAME645_BUFSIZE_MAX))
    {
        return HI_ERR_INVALID_PARAMETER;
    }
    
    (hi_void)memcpy_s(pstCtx->stRx.aucBuf, sizeof(pstCtx->stRx.aucBuf), pData, usLen);
    pstCtx->stRx.ulSize = usLen;

    return mrsCltiiSearchParseRx(pstCtx);
}

HI_U32 mrsCltiiSearchByteTimeoutProc(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    pstRunning->bByteTimerFlag = HI_FALSE;
    if (HI_TRUE == pstRunning->bFrameTimerFlag)
    {
        return HI_ERR_SUCCESS;
    }

    return mrsCltiiSearchParseRx(pstCtx);
}


HI_U32 mrsCltiiSearchFrameTimeoutProc(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    pstRunning->bFrameTimerFlag = HI_FALSE;

    if (HI_TRUE == pstRunning->bByteTimerFlag)
    {
        return HI_ERR_SUCCESS;
    }

    return mrsCltiiSearchParseRx(pstCtx);
}


HI_U32 mrsCltiiSearchVMTimeoutProc(HI_VOID)
{
    if (mrsStaGetSupportVM())
    {
        mrsStaSearchSetStatus(MRS_SEARCH_STATUS_IDLE);
    }

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiVMSearch(HI_VOID)
{
    HI_U8 ucMac[HI_PLC_MAC_ADDR_LEN] = { 0 };
    HI_U32 ulTimeout = 0;
    HI_U32 ulHi = 0;
    HI_U32 ulLo = 0;

    MRS_StopTimer(MRS_STA_TIMER_SM_VM);

    mrsToolsGetLocalMac(ucMac);
    ulTimeout = (ucMac[5] % 100);
    if (ulTimeout > 0)
    {
        ulTimeout *= 1000;
        MRS_StartTimer(MRS_STA_TIMER_SM_VM, ulTimeout, HI_SYS_TIMER_ONESHOT);
    }
    else
    {
        mrsStaSearchSetStatus(MRS_SEARCH_STATUS_IDLE);
    }

    ulHi = ((HI_U32)ucMac[2] * 1000) + ucMac[3];
    ulLo = ((HI_U32)ucMac[4] * 1000) + ucMac[5];

    mrsIntToBcd(ulHi, ucMac + 3, 3);
    mrsIntToBcd(ulLo, ucMac, 3);

    mrsStaVMAddr2MeterList(ucMac, METER_PROTO_645_2007);

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiSearchCtxInit(MRS_STA_SEARCH_CTX * pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_CFG_STRU *pstCfg = &(pstCtx->stSearchCfg);
    HI_U32 i = 0;

    pstRunning->bEnableBcSearch = pstCfg->bEnableBcSearch97;

    mrsSrvInitQueue(&pstRunning->stQueue);

    (HI_VOID)mrsCltiiInsertSearchItem(&pstRunning->stQueue, pstCfg->ulDI07, METER_PROTO_645_2007);
    (HI_VOID)mrsCltiiInsertSearchItem(&pstRunning->stQueue, 0, METER_PROTO_698_45);
    for (i = 0; i < MRS_TOOLS_ALEN(pstCfg->ausDI97); i++)
    {
        mrsCltiiInsertSearchItem(&pstRunning->stQueue, pstCfg->ausDI97[i], METER_PROTO_645_1997);
    }

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiInsertSearchItem(MRS_SRV_QUEUE *pstQueue, HI_U32 ulDI, HI_U8 ucProtocol)
{
    MRS_SRV_BAUDRATE_ID_E eId = MRS_SRV_ID_BAUDRATE_SM_07;
    MRS_SRV_BAUDRATE_CFG_S *pstCfg = HI_NULL;
    HI_U16 usBaudrateDefault = MRS_SRV_BAUD_RATE_2400;
    HI_U16 *pusBrList = &usBaudrateDefault;
    HI_U8 ucBrCnt = 1;
    HI_U8 i = 0;

    if (METER_PROTO_645_1997 == ucProtocol)
    {
        if (0 == ulDI)
        {
            return HI_ERR_SKIP;
        }

        eId = MRS_SRV_ID_BAUDRATE_SM_97;
        usBaudrateDefault = MRS_SRV_BAUD_RATE_1200;
    }
    else if (METER_PROTO_698_45 == ucProtocol)
    {
        eId = MRS_SRV_ID_BAUDRATE_SM_698;
        usBaudrateDefault = MRS_SRV_BAUD_RATE_9600;
    }

    pstCfg = mrsSrvGetBaudRateCfg(eId);
    if (pstCfg)
    {
        pusBrList = pstCfg->usBaudRateList;
        ucBrCnt = pstCfg->ucValidNum;
    }

    for (i = 0; i < ucBrCnt; i++)
    {
        MRS_SEARCH_NODE *pstNode = mrsToolsMalloc(sizeof(MRS_SEARCH_NODE));

        if (!pstNode)
        {
            return HI_ERR_MALLOC_FAILUE;
        }

        (hi_void)memset_s(pstNode, sizeof(MRS_SEARCH_NODE), 0, sizeof(MRS_SEARCH_NODE));
        pstNode->stItem.ulDI = ulDI;
        pstNode->stItem.ucProtocol = ucProtocol;
        pstNode->stItem.usBaudrate = pusBrList[i];

        (HI_VOID)mrsSrvEnQueue(pstQueue, &pstNode->stLink);
    }

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiCheckNvMeterParam(MRS_STA_SEARCH_CTX * pstCtx)
{
    HI_U8 aucAddr[HI_METER_ADDR_LEN] = {0};

    mrsMeterAddrPreProc(aucAddr, pstCtx->stSearchCfg.aucNvMeter, pstCtx->stSearchCfg.ucNvProtocol);

    if (!mrsToolsNormalAddr(aucAddr))
    {
        return HI_ERR_FAILURE;
    }

    if ((METER_PROTO_645_2007 != pstCtx->stSearchCfg.ucNvProtocol) 
        && (METER_PROTO_645_1997 != pstCtx->stSearchCfg.ucNvProtocol)
        && (METER_PROTO_698_45 != pstCtx->stSearchCfg.ucNvProtocol))
    {
        return HI_ERR_FAILURE;
    }

    return HI_ERR_SUCCESS;
}


HI_BOOL mrsCltiiSearchItemVisit(HI_VOID *pItem, HI_VOID *pParam)
{
    MRS_SEARCH_NODE *pNode = (MRS_SEARCH_NODE *)pItem;
    MRS_SEARCH_ITEM_EXTRA *pExtra = (MRS_SEARCH_ITEM_EXTRA *)pParam;

    if ((METER_PROTO_698_45 == pExtra->ucProtocol) || (METER_PROTO_645_2007 == pExtra->ucProtocol))
    {
        if (pExtra->ucProtocol == pNode->stItem.ucProtocol)
        {
            pExtra->ulDiOut = pNode->stItem.ulDI;
            pExtra->bResult = HI_TRUE;
            return HI_TRUE;
        }
    }
    else
    {
        if ((pExtra->ucProtocol == pNode->stItem.ucProtocol)
            && (pExtra->ulDiIn == (HI_U16)pNode->stItem.ulDI))
        {
            pExtra->ulDiOut = pNode->stItem.ulDI;
            pExtra->bResult = HI_TRUE;
            return HI_TRUE;
        }
    }

    return HI_FALSE;
}

HI_U32 mrsCltiiInitFirstFrame(MRS_STA_SEARCH_CTX * pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_CFG_STRU *pstCfg = &(pstCtx->stSearchCfg);
    MRS_SEARCH_ITEM_EXTRA stExtra = {0};
    HI_U32 ulDi = 0;

    stExtra.ulDiIn = 0;
    stExtra.ucProtocol = pstCfg->ucNvProtocol;
    stExtra.bResult = HI_FALSE;
    stExtra.ulDiOut = 0;

    if ((METER_PROTO_698_45 == pstCfg->ucNvProtocol) || (METER_PROTO_645_2007 == pstCfg->ucNvProtocol))
    {
        mrsSrvTraverseQueue(&pstRunning->stQueue, mrsCltiiSearchItemVisit, &stExtra);
        if (!stExtra.bResult)
        {
            return HI_ERR_NOT_FOUND;
        }

        ulDi = stExtra.ulDiOut;
    }
    else
    {
        stExtra.ulDiIn = pstCfg->usNvDI97;
        mrsSrvTraverseQueue(&pstRunning->stQueue, mrsCltiiSearchItemVisit, &stExtra);

        ulDi = pstRunning->usDefaultDi97;
        if (stExtra.bResult)
        {
            ulDi = stExtra.ulDiOut;
        }

        if (ulDi == 0)
        {
            return HI_ERR_NOT_FOUND;
        }
    }

    mrsCltiiInitSearchFrame(pstCtx, ulDi, pstCfg->ucNvProtocol);
    mrsCltiiCreateSearchFrame(pstCtx, pstCfg->aucNvMeter, pstCfg->ucNvProtocol);

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiInitSearchFrame(MRS_STA_SEARCH_CTX * pstCtx, HI_U32 ulDI, HI_U8 ucProtocol)
{
    if (METER_PROTO_698_45 == ucProtocol)
    {
        return mrsCltiiInitSearchFrame698(pstCtx, ulDI, ucProtocol);
    }
    else
    {
        return mrsCltiiInitSearchFrame645(pstCtx, ulDI, ucProtocol);
    }
}


HI_U32 mrsCltiiInitSearchFrame645(MRS_STA_SEARCH_CTX * pstCtx, HI_U32 ulDI, HI_U8 ucProtocol)
{
    HI_U32 i = 0;
    HI_U8 * pucBuf = pstCtx->stTx.aucBuf;
    HI_U8 ucLen = 0;
    HI_U32 ulDiSize = 0;
    HI_U32 ulDiTemp = ulDI;
    HI_U8 ucCS = 0;

    (hi_void)memset_s(pstCtx->stTx.aucBuf, sizeof(pstCtx->stTx.aucBuf), 0, sizeof(pstCtx->stTx.aucBuf));
    (hi_void)memset_s(pucBuf + ucLen, MRS_STA_FRAME645_BUFSIZE_MAX, 0xFE, MRS_SM_POS_H);
    ucLen += MRS_SM_POS_H;

    pucBuf[ucLen++] = MRS_645_FRAME_START_FLG;
    (hi_void)memset_s(pstCtx->stRunning.unClt.stCltII.aucTxAddr, HI_METER_ADDR_LEN, 0xAA, HI_METER_ADDR_LEN);
    (hi_void)memcpy_s(pucBuf + ucLen, MRS_STA_FRAME645_BUFSIZE_MAX - MRS_SM_POS_H - 1, pstCtx->stRunning.unClt.stCltII.aucTxAddr, HI_METER_ADDR_LEN);
    ucLen += HI_METER_ADDR_LEN;
    pucBuf[ucLen++] = MRS_645_FRAME_START_FLG;

    if (METER_PROTO_645_1997 == ucProtocol)
    {
        pucBuf[ucLen++] = 0x01;
        ulDiSize = sizeof(HI_U16);
    }
    else
    {
        pucBuf[ucLen++] = 0x11;
        ulDiSize = sizeof(HI_U32);
    }

    pucBuf[ucLen++] = (HI_U8)ulDiSize;
    for (i = 0; i < ulDiSize; i++)
    {
        pucBuf[ucLen++] = ((HI_U8)(ulDiTemp >> (i*8))) + MRS_645_FRAME_HEX33;
    }

    ucCS = mrsToolsCalcCheckSum(pucBuf + MRS_SM_POS_H, (HI_U16)(ucLen - MRS_SM_POS_H));
    pucBuf[ucLen++] = ucCS;
    pucBuf[ucLen++] = MRS_645_FRAME_END_FLG;

    pstCtx->stTx.ulSize = (HI_U32)ucLen;
    
    pstCtx->stRunning.unClt.stCltII.ulTxDI = ulDI;
    pstCtx->stRunning.unClt.stCltII.ucTxProtocol = ucProtocol;

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiCreateSearchFrame(MRS_STA_SEARCH_CTX * pstCtx, HI_U8 *pucMeter, HI_U8 ucProtocol)
{
    if (METER_PROTO_698_45 == ucProtocol)
    {
        return mrsCltiiCreateSearchFrame698(pstCtx, pucMeter);
    }
    else
    {
        return mrsCltiiCreateSearchFrame645(pstCtx, pucMeter);
    }
}


HI_U32 mrsCltiiCreateSearchFrame645(MRS_STA_SEARCH_CTX * pstCtx, HI_U8 *pucMeter)
{
    HI_U32 ulPosH = MRS_SM_POS_H;
    
    if (pstCtx->stTx.ulSize > MRS_STA_FRAME645_BUFSIZE_MAX)
    {
        return HI_ERR_FAILURE;
    }
    
    (hi_void)memcpy_s(pstCtx->stRunning.unClt.stCltII.aucTxAddr, HI_METER_ADDR_LEN, pucMeter, HI_METER_ADDR_LEN);
    (hi_void)memcpy_s(pstCtx->stTx.aucBuf + MRS_SM_POS_H + MRS_SM_POS_A, 
        sizeof(pstCtx->stTx.aucBuf) - MRS_SM_POS_H - MRS_SM_POS_A, pucMeter, HI_METER_ADDR_LEN);
    *(pstCtx->stTx.aucBuf + pstCtx->stTx.ulSize - 2) = mrsToolsCalcCheckSum(pstCtx->stTx.aucBuf + ulPosH, (HI_U16)(pstCtx->stTx.ulSize - ulPosH - 2));
    
    return HI_ERR_SUCCESS;
}
HI_U32 mrsCltiiInitSearchFrame698(MRS_STA_SEARCH_CTX * pstCtx, HI_U32 ulDI, HI_U8 ucProtocol)
{
    HI_U16 usLength = 0;
    HI_U8 *pucFrame = HI_NULL;

    (hi_void)memset_s(pstCtx->stTx.aucBuf, sizeof(pstCtx->stTx.aucBuf), 0, sizeof(pstCtx->stTx.aucBuf));
    pucFrame = mrsGetSendAddrFrame(MRS_SRV_PROTO_698_45, &usLength);
    (hi_void)memcpy_s(pstCtx->stTx.aucBuf, MRS_STA_FRAME645_BUFSIZE_MAX, pucFrame, usLength);

    pstCtx->stTx.ulSize = (HI_U32)usLength;

    pstCtx->stRunning.unClt.stCltII.ulTxDI = ulDI;
    pstCtx->stRunning.unClt.stCltII.ucTxProtocol = ucProtocol;
    (hi_void)memset_s(pstCtx->stRunning.unClt.stCltII.aucTxAddr, HI_METER_ADDR_LEN, 0xAA, HI_METER_ADDR_LEN);

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiCreateSearchFrame698(MRS_STA_SEARCH_CTX * pstCtx, HI_U8 *pucMeter)
{
    MRS_698_HEAD_STRU *pHead = (MRS_698_HEAD_STRU *)(pstCtx->stTx.aucBuf + MRS_SM_POS_H);
    HI_U16 usFCS = 0;

    (hi_void)memcpy_s(pHead->aucAddr, HI_METER_ADDR_LEN, pucMeter, HI_METER_ADDR_LEN);
    pHead->usHCS = mrs698Crc16(MRS_698_CRC_INIT, pstCtx->stTx.aucBuf + MRS_SM_POS_H + 1, (HI_S32)(sizeof(MRS_698_HEAD_STRU) - 3));
    usFCS = mrs698Crc16(MRS_698_CRC_INIT, pstCtx->stTx.aucBuf + MRS_SM_POS_H + 1, (HI_S32)(pstCtx->stTx.ulSize - MRS_SM_POS_H - 4));
    (hi_void)memcpy_s(pstCtx->stTx.aucBuf + pstCtx->stTx.ulSize - 3, sizeof(HI_U16), &usFCS, sizeof(usFCS));
    (hi_void)memcpy_s(pstCtx->stRunning.unClt.stCltII.aucTxAddr, HI_METER_ADDR_LEN, pucMeter, HI_METER_ADDR_LEN);

    HI_DIAG_LOG_BUF(MRS_FILE_LOG_BUF_1004, HI_DIAG_MT("Create TX Frame"), pstCtx->stTx.aucBuf, pstCtx->stTx.ulSize);

    return HI_ERR_SUCCESS;
}


HI_U32 mrsCltiiSearchSendFrame(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_STA_SRV_CTX_STRU *pstSta = mrsStaGetContext();
    MRS_STA_ITEM * pstItem = HI_NULL;
    HI_U16 usPayloadLen = 0;
    HI_U8 *pucPayload = HI_NULL;

    usPayloadLen = (HI_U16)pstCtx->stTx.ulSize;
    pucPayload = pstCtx->stTx.aucBuf;

    pstItem = (MRS_STA_ITEM *)mrsToolsMalloc(sizeof(MRS_STA_ITEM) + usPayloadLen);
    if (!pstItem)
    {
        return HI_ERR_NOT_ENOUGH_MEMORY;
    }

    (hi_void)memset_s(pstItem, sizeof(MRS_STA_ITEM) + usPayloadLen, 0, sizeof(MRS_STA_ITEM) + usPayloadLen);

    pstItem->id = MRS_STA_ITEM_ID_SEARCH_METER_CLT_II;
    pstItem->buf = (HI_U8 *)pstItem + sizeof(MRS_STA_ITEM);
    pstItem->len = usPayloadLen;
    pstItem->from = MRS_STA_QUEUE_FROM_STA;
    (hi_void)memcpy_s(pstItem->buf, usPayloadLen, pucPayload, usPayloadLen);

    if (!mrsStaTryEnQueue(&pstSta->stQueModule, pstItem))
    {
        mrsToolsFree(pstItem);
        return HI_ERR_FAILURE;
    }

    mrsStaNotifyQueue();

    return HI_ERR_SUCCESS;
}


// 开始通配符搜表循环
HI_U32 mrsCltiiSearchLoopStart(MRS_STA_SEARCH_CTX *pstCtx)
{
    return mrsCltiiSearchLoopStartProc(pstCtx);
}


HI_U32 mrsCltiiSearchLoopStartProc(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_NODE *pstNode = (MRS_SEARCH_NODE *)mrsSrvQueueTop(&pstRunning->stQueue);

    if (!pstNode)
    {
        return HI_ERR_FAILURE;
    }

    mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_LOOP);

    mrsCltiiSearchClearVerifyParams(pstCtx);
    mrsCltiiSearchClearExploreParams(pstCtx);
    mrsCltiiSearchClearLoopParams(pstCtx);

    mrsCltiiInitSearchFrame(pstCtx, pstNode->stItem.ulDI, pstNode->stItem.ucProtocol);
    mrsCltiiSearchResetRunning(pstCtx);

    return mrsCltiiSearchSendFrame(pstCtx);
}


/* BEGIN: PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/12 */
HI_U32 mrsCltiiSearchLoopRxMeter(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 *pucMeter, HI_U8 ucProtocol)
{
    if (mrsStaFindMeterOfSearchResult(pucMeter, ucProtocol, HI_FALSE, 0))
    {
        return HI_ERR_EXIST;
    }

    return mrsCltiiSearchSaveVerifyParams(pstCtx, pucMeter, ucProtocol);
}


HI_U32 mrsCltiiSearchLoopRxMeterProc(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 *pucMeter, HI_U8 ucProtocol)
{
    mrsCltiiSearchLoopRxMeter(pstCtx, pucMeter, ucProtocol);

    if ((ucProtocol == METER_PROTO_645_1997)
        && (pucMeter[HI_METER_ADDR_LEN - 1] == MRS_645_METER_ADDR_WILDCARDS))
    {
        HI_U8 aucMeterAddr[HI_METER_ADDR_LEN] = {0};

        mrsMeterAddrPreProc(aucMeterAddr, pucMeter, ucProtocol);
        mrsCltiiSearchLoopRxMeter(pstCtx, aucMeterAddr, ucProtocol);
    }

    return HI_ERR_SUCCESS;
}
/* END:   PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/12 */


HI_U32 mrsCltiiSearchParseRx(MRS_STA_SEARCH_CTX *pstCtx)
{
    HI_U32 ulRxLen = pstCtx->stRx.ulSize;

    MRS_WaitSem(EN_APP_SEM_SRV_UART, HI_SYS_WAIT_FOREVER);
    (HI_VOID)mrsCltiiSearchParseRxProc(pstCtx);
    MRS_SignalSem(EN_APP_SEM_SRV_UART);
    if (ulRxLen > 0)
    {
        mrsDfxCltiiRxFrameCount();
    }

    return mrsCltiiSearchCheck(pstCtx);
}


HI_U32 mrsCltiiSearchParseRxProc(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_U32 ulRet = HI_ERR_SUCCESS;
    HI_U16 usPos = 0;
    HI_U16 usFrmLen = 0;
    HI_U8 ucBadFrm = 0;
    HI_U8 *pucBuf = pstCtx->stRx.aucBuf;
    HI_U16 usLen = (HI_U16)pstCtx->stRx.ulSize;
    HI_U16 usSearchDi = 0;
    HI_U8 ucProtocol = mrsCltiiSearchTxProtocol(pstCtx);
    MRS_STA_ITEM * pstStaItem;
    MRS_QUE_MODULE *pstQueue;

    // 放最前面，保证搜表满32个的情况下也不漏存报文
    if (usLen > 0)
    {
        mrsDfxCltiiAppendCurrentFrame(mrsCltiiSearchTxAddr(pstCtx), 
                                      ucProtocol,
                                      mrsCltiiSearchTxDI(pstCtx), 
                                      (HI_U16)mrsGetCurBaudRate());
    }

    while (usLen > 0)
    {
        HI_U16 i = 0;
        HI_U16 usTmpLen = 0;
        HI_U8  ucCtrl = 0;
        HI_BOOL bValidFlag = HI_TRUE;

        if (METER_PROTO_698_45 == ucProtocol)
        {
            ulRet = mrsFind698Frame(pucBuf, usLen, &usPos, &usFrmLen);
        }
        else
        {
            ulRet = mrsFind645Frame(pucBuf, (HI_S16)usLen, &usPos, &usFrmLen);
        }

        if (HI_ERR_SUCCESS != ulRet)
        {
            if (mrsCltiiSearchCheckInvalidFrame(pstCtx, usLen, ucProtocol))
            {
                ucBadFrm++;
                mrsDfxCltiiSetFrameStatsInvalid();
            }
            break;
        }

        mrsDfxCltiiSetLocalRxValid645Frame();

        if (HI_ERR_SUCCESS == mrsJudgeEqiupMode(pucBuf + usPos, usFrmLen))
        {
            return ulRet;
        }

        usTmpLen = usPos;
        while ((usTmpLen > 0) && (i++ < 4))
        {
            if (pucBuf[usTmpLen - 1] != 0xFE)
            {
                break;
            }
            
            usTmpLen--;
        }

        if (mrsCltiiSearchCheckInvalidFrame(pstCtx, usTmpLen, ucProtocol))
        {
            ucBadFrm++;
            mrsDfxCltiiSetFrameStatsInvalid();
        }

        if (METER_PROTO_698_45 == ucProtocol)
        {
            ucCtrl = pucBuf[usPos + MRS_698_FRAME_CTRL_OFFSET];
            HI_DIAG_LOG_MSG_E1(MRS_FILE_LOG_FLAG_001, HI_DIAG_MT("698.45 frame"), ucCtrl);
            if (0 == (ucCtrl & MRS_698_FRAME_DIR_MASK))
            {
                mrsDfxCltiiSetSmMultiCltiiConnect();
            }
        }
        else
        {
            ucCtrl = pucBuf[usPos + MRS_645_FRAME_CTRL_OFFSET];

            if (0 == (ucCtrl & MRS_645_FRAME_CONTROL_DIR_UP))
            {
                mrsDfxCltiiSetSmMultiCltiiConnect();
            }

            if (MRS_645_FRAME_CONTROL_DENY_MASK == (ucCtrl & MRS_645_FRAME_CONTROL_DENY_MASK))
            {
                mrsDfxCltiiSetFrameStatsDeny();

                if (METER_PROTO_645_2007 == ucProtocol)
                {
                    bValidFlag = HI_FALSE;
                }
            }
        }

        if (bValidFlag)
        {
            HI_U8 aucTempAddr[HI_METER_ADDR_LEN] = {0};
            HI_U8 aucMeter[HI_METER_ADDR_LEN] = {0};

            if (METER_PROTO_698_45 == ucProtocol)
            {
                (HI_VOID)mrsFind698MeterAddr(pucBuf + usPos, usFrmLen, aucMeter);
                (hi_void)memcpy_s(aucTempAddr, HI_METER_ADDR_LEN, aucMeter, HI_METER_ADDR_LEN);
            }
            else
            {
                mrsCltiiGetMeterAddr(pucBuf + usPos, usFrmLen, ucProtocol, aucMeter);
                mrsMeterAddrULPreProc(aucTempAddr, aucMeter, ucProtocol, ucCtrl);
            }

            if (mrsToolsNormalAddr(aucTempAddr))
            {
                mrsDfxCltiiSetFrameStatsValid();
                usSearchDi = mrsCltiiSearchGetDI97(pstCtx, ucProtocol);
                
                pstQueue = &(mrsStaGetContext()->stQueModule);
                pstStaItem = mrsSrvQueueTop(&pstQueue->stMrQueue);
                if (pstStaItem && !mrsStaIsMrFrameMatch(pstStaItem->buf, pstStaItem->len, pucBuf + usPos, usFrmLen, pstStaItem->ucProtocol))
                {
                    break;
                }
                
                switch(mrsCltiiSearchGetState())
                {
                case MRS_CLTII_SEARCH_STATE_LOOP:
                case MRS_CLTII_SEARCH_STATE_EXPLORE:
                    mrsCltiiSearchLoopRxMeterProc(pstCtx, aucMeter, ucProtocol);
                    break;

                default:
                    if (mrsMeterAddrMatch(mrsCltiiSearchTxAddr(pstCtx), aucMeter))
                    {
                        MRS_METER_ITEM_S stMeter = {0};

                        stMeter.ucValidFlag = HI_TRUE;
                        stMeter.ucProtocol = ucProtocol;
                        stMeter.usOption = usSearchDi;
                        stMeter.usBaudRate = mrsCltiiGetSearchBaudRate(pstCtx);
                        (hi_void)memcpy_s(stMeter.ucMeterAddr, HI_METER_ADDR_LEN, aucMeter, HI_METER_ADDR_LEN);

                        /* Add meter to local meter list. CNcomment: 添加电表信息到本地列表。CNend */
                        mrsStaAddMeter2LocalList(&stMeter);
                        if (MRS_CLTII_SEARCH_STATE_DETECT == mrsCltiiSearchGetState())
                        {
                            mrsCltiiSearchDetectNvPowerProc(HI_TRUE, (HI_U8)(pstCtx->stNvMeterList.ucDetectCount - 1), 
                                                            &pstCtx->stNvMeterList.astNvMeter[pstCtx->stNvMeterList.ucDetectCount - 1]);
                        }

                        ulRet = mrsStaSearchMeterInsert(pstCtx, &stMeter);
                        if (HI_ERR_FULL == ulRet)
                        {
                            return ulRet;
                        }
                    }
                    break;
                }
                
                pstRunning->ucValidFrame++;
            }
            else
            {
                mrsDfxCltiiSetFrameStatsDiscard();
            }
        }

        mrsUARTRTxLedPro();

        pucBuf += usPos + usFrmLen;
        usLen -= usPos + usFrmLen;
    }

    pstRunning->ucInvalidFrame += ucBadFrm;

    pstCtx->stRx.ulSize = 0;
    (hi_void)memset_s(pstCtx->stRx.aucBuf, sizeof(pstCtx->stRx.aucBuf), 0, sizeof(pstCtx->stRx.aucBuf));

    return HI_ERR_SUCCESS;
}


/* BEGIN: PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/11 */
HI_U32 mrsCltiiSearchSaveVerifyParams(MRS_STA_SEARCH_CTX* pstCtx, HI_U8* pucMeter, HI_U8 ucProtocol)
{
    MRS_SEARCH_RUNNING_CLT_II* pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_VERIFY_STRU* pstVerify = &(pstRunning->stVerify);
    MRS_SEARCH_VERIFY_ITEM* pstItem = HI_NULL;
    HI_U8 ucIndex = 0;

    HI_UNREF_PARAM(ucProtocol);

    if (pstVerify->ucTotal >= MRS_SEARCH_MAX_METERNUM_IN_RX)
    {
        return HI_ERR_FULL;
    }

    for (ucIndex = 0; ucIndex < pstVerify->ucTotal; ucIndex++)
    {
        pstItem = &pstVerify->astVerify[ucIndex];
        if (mrsMeterAddrMatch(pucMeter, pstItem->aucAddr))
        {
            return HI_ERR_EXIST;
        }        
    }

    pstItem = &pstVerify->astVerify[pstVerify->ucTotal];
    (hi_void)memcpy_s(pstItem->aucAddr, HI_METER_ADDR_LEN, pucMeter, HI_METER_ADDR_LEN);
    pstVerify->ucTotal++;

    return HI_ERR_SUCCESS;
}
/* END:   PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/11 */


HI_BOOL mrsCltiiSearchIsNeedVerify(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II* pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    switch(mrsCltiiSearchGetState())
    {
    case MRS_CLTII_SEARCH_STATE_LOOP:
    case MRS_CLTII_SEARCH_STATE_EXPLORE:
        return (pstRunning->stVerify.ucTotal > 0);
        
    default:
        return HI_FALSE;
    }    
}

HI_BOOL mrsCltiiSearchIsEnable(HI_VOID)
{
    MRS_STA_SEARCH_CTX *pstSmCtx = mrsStaSearchCtx();
    return (HI_BOOL)(mrsToolsIsIICollector() && (pstSmCtx->stSearchCfg.bCltiiSmEnable));
}

HI_BOOL mrsCltiiSearchIsAllVerified(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II* pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    return (pstRunning->stVerify.ucVerifyCount >= pstRunning->stVerify.ucTotal);
}


HI_VOID mrsCltiiSearchSaveLoopParams(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II* pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    mrsCltiiSearchGetTxAddr(pstCtx, pstRunning->stParams.aucLoopAddr);
    pstRunning->stParams.ucLoopInvalidFrame = pstRunning->ucInvalidFrame;
    pstRunning->stParams.ucLoopValidFrame = pstRunning->ucValidFrame;
    pstRunning->stParams.ucLoopRetry = pstRunning->ucRetry;
    pstRunning->stParams.usLoopRxSize = (HI_U16)(pstCtx->stRx.ulSize);
}


HI_VOID mrsCltiiSearchRestoreLoopParams(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    // 恢复发送缓存stTx.aucBuf的报文，报文中的通配符地址也恢复了
    mrsCltiiCreateSearchFrame(pstCtx, pstRunning->stParams.aucLoopAddr, pstRunning->ucTxProtocol);
    
    pstRunning->ucInvalidFrame = pstRunning->stParams.ucLoopInvalidFrame;
    pstRunning->ucValidFrame = pstRunning->stParams.ucLoopValidFrame;
    pstRunning->ucRetry = pstRunning->stParams.ucLoopRetry;
    pstCtx->stRx.ulSize = pstRunning->stParams.usLoopRxSize;
}


HI_VOID mrsCltiiSearchClearLoopParams(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    (hi_void)memset_s(&pstRunning->stParams, sizeof(pstRunning->stParams), 0, sizeof(pstRunning->stParams));
}


// 开始抄读验证某个通配符地址问回来的合法表号(列表)
HI_U32 mrsCltiiSearchVerifyStart(MRS_STA_SEARCH_CTX *pstCtx)
{
    // 先存参数，再改变状态
    mrsCltiiSearchSaveLoopParams(pstCtx);
    
    if (MRS_CLTII_SEARCH_STATE_EXPLORE == mrsCltiiSearchGetState())
    {
        mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_EXPLORE_VERIFY);
    }
    else
    {
        mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_VERIFY);
    }

    return mrsCltiiSearchVerifyProc(pstCtx);
}


HI_U32 mrsCltiiSearchVerifyProc(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_VERIFY_STRU* pstVerify = &(pstRunning->stVerify);
    MRS_SEARCH_VERIFY_ITEM* pstItem = &(pstVerify->astVerify[pstVerify->ucVerifyCount]);

    mrsCltiiCreateSearchFrame(pstCtx, pstItem->aucAddr, pstRunning->ucTxProtocol);
    mrsCltiiSearchResetRunning(pstCtx);

    pstVerify->ucVerifyCount++;
    
    return mrsCltiiSearchSendFrame(pstCtx);
}


HI_U32 mrsCltiiSearchVerifyFinish(MRS_STA_SEARCH_CTX *pstCtx)
{
    mrsCltiiSearchClearVerifyParams(pstCtx);  // 清空抄读验证结构体
    mrsCltiiSearchRestoreLoopParams(pstCtx);  // 恢复抄读验证之前的通配符循环参数
    mrsCltiiSearchClearLoopParams(pstCtx);    // 清空保存的通配符循环参数
    
    if (MRS_CLTII_SEARCH_STATE_EXPLORE_VERIFY == mrsCltiiSearchGetState())
    {
        mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_EXPLORE);
    }
    else
    {
        mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_LOOP);
    }
    
    return mrsCltiiSearchCheckNormal(pstCtx);
}


HI_VOID mrsCltiiSearchClearVerifyParams(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    
    (hi_void)memset_s(&pstRunning->stVerify, sizeof(pstRunning->stVerify), 0, sizeof(pstRunning->stVerify));
}


HI_BOOL mrsCltiiSearchAllowExplore(MRS_STA_SEARCH_CTX* pstCtx)
{
    HI_U8 ucUpperBoundFromNv = pstCtx->stSearchCfg.ucExploreByteCountMax;
    HI_U8 ucNumOfBytesAvailable = mrsCltiiSearchGetAddrAvailMaskByteNum(mrsCltiiSearchTxAddr(pstCtx),
                                                                        mrsCltiiSearchTxProtocol(pstCtx));
    
    if (mrsCltiiSearchIsMaskMeterAddr(mrsCltiiSearchTxAddr(pstCtx)))
    {
        return HI_FALSE;
    }

    if (0 == ucUpperBoundFromNv || 0 == ucNumOfBytesAvailable)
    {
        return HI_FALSE;
    }

    return HI_TRUE;
}


// 开始多缩N字节通配符缩位搜表
HI_U32 mrsCltiiSearchExploreStart(MRS_STA_SEARCH_CTX* pstCtx)
{
    mrsCltiiSearchSaveExploreParams(pstCtx);

    mrsCltiiSearchExploreDepthIncrease(pstCtx, mrsCltiiSearchTxAddr(pstCtx), mrsCltiiSearchTxProtocol(pstCtx));
    mrsCltiiCreateSearchFrame(pstCtx, mrsCltiiSearchTxAddr(pstCtx), mrsCltiiSearchTxProtocol(pstCtx));
    mrsCltiiSearchResetRunning(pstCtx);

    mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_EXPLORE);
    return mrsCltiiSearchSendFrame(pstCtx);
}


HI_VOID mrsCltiiSearchSaveExploreParams(MRS_STA_SEARCH_CTX* pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_U8 ucUpperBoundFromNv = pstCtx->stSearchCfg.ucExploreByteCountMax;
    HI_U8 ucNumOfBytesAvailable = mrsCltiiSearchGetAddrAvailMaskByteNum(mrsCltiiSearchTxAddr(pstCtx),
                                                                        mrsCltiiSearchTxProtocol(pstCtx));
    
    pstRunning->ucExploreByteCountMax = MRS_MIN(ucNumOfBytesAvailable, ucUpperBoundFromNv);
    pstRunning->ucExploreByteCount = 0;
}


// 计算通配符地址中可用于缩位的掩码字节个数，A1~A5，A0除外
HI_U8 mrsCltiiSearchGetAddrAvailMaskByteNum(HI_U8* pucAddr, HI_U8 ucProtocol)
{
    return (HI_U8)(HI_METER_ADDR_LEN - mrsCltiiSearchGetAddrDepth(pucAddr, ucProtocol));
}


HI_VOID mrsCltiiSearchExploreDepthIncrease(MRS_STA_SEARCH_CTX *pstCtx, HI_U8* pucAddr, HI_U8 ucProtocol)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    
    mrsCltiiSearchAddrDepthIncrease(pucAddr, ucProtocol);
    pstRunning->ucExploreByteCount++;
}


HI_VOID mrsCltiiSearchExploreFinish(MRS_STA_SEARCH_CTX *pstCtx)
{
    mrsCltiiSearchClearExploreParams(pstCtx);
    mrsCltiiSearchSetState(MRS_CLTII_SEARCH_STATE_LOOP);
}


HI_VOID mrsCltiiSearchClearExploreParams(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    pstRunning->ucExploreByteCountMax = 0;
    pstRunning->ucExploreByteCount = 0;
}


HI_BOOL mrsCltiiSearchCheckInvalidFrame(MRS_STA_SEARCH_CTX *pstCtx, HI_U16 usLen, HI_U8 ucProtocol)
{
    HI_U16 usInvalidMin = 0xFFFF;

    switch (ucProtocol)
    {
        case METER_PROTO_698_45:
        case METER_PROTO_645_2007:
            usInvalidMin = pstCtx->stSearchCfg.ucInvalidFrmLen07;
            break;

        case METER_PROTO_645_1997:
            usInvalidMin = pstCtx->stSearchCfg.ucInvalidFrmLen97;
            break;

        default:
            break;
    }

    mrsDfxCltiiSetLocalRxInvalidStats(usLen);

    return (HI_BOOL)(usLen >= usInvalidMin);
}


HI_PRV HI_U32 mrsCltiiSearchFinish(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_STA_SRV_CTX_STRU * pstSta = mrsStaGetContext();
    MRS_METER_LIST_S * pstMeterList = &(pstSta->stMeterList);
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_METERLIST_STRU *pstSmList = &(pstCtx->stMeterList);
    HI_BOOL bChanged = HI_FALSE;

    MRS_StopTimer(MRS_STA_TIMER_SM_FRAME);
    MRS_StopTimer(MRS_STA_TIMER_SM_BYTE);

    pstRunning->bFrameTimerFlag = HI_FALSE;
    pstRunning->bByteTimerFlag = HI_FALSE;

    // 开机15秒内，搜表结束未搜到表，继续尝试搜表
    if ((0 == pstSmList->ucMeterNum) && (HI_MDM_GetSeconds() < MRS_SM_STARTUP_TIMESTAMP))
    {
        return mrsCltiiSearchLoopStart(pstCtx);
    }

    // 从未搜到过表，起一个10分钟定时器，到时间时开始搜表
    if (HI_TRUE != pstSmList->bMeterFlag)
    {
    	mrsAssetNumberAsMeterInsert();
        MRS_StartTimer(MRS_STA_TIMER_SM_PERIOD_NO_METER, MRS_STA_TIME_SM_PERIOD_NO_METER, HI_SYS_TIMER_ONESHOT);
    }

    bChanged = mrsCltiiSearchCheckChange(pstMeterList, pstSmList);
    if (bChanged)
    {
        MRS_SEARCH_EVENT_ITEM stEventItem;

        stEventItem.ulTime = HI_MDM_GetMilliSeconds();
        stEventItem.usEvent = MRS_SEARCH_EVENT_ADDR_CHANGED;
        stEventItem.usReserved = 0;

        mrsCltiiSearchEventInsert(pstCtx, &stEventItem);
    }

    mrsStaSearchCheckFirstMeterAddr(pstCtx);
    mrsStaRefreshLocalMeterList(pstMeterList, pstSmList);
    mrsStaRefreshSearchResultNv(pstCtx, HI_TRUE);
    mrsStaSearchFinishNotify(MRS_SEARCH_FINISH_NORMAL);

    if (pstSmList->ucMeterNum > 0)
    {
        mrsCollectorSetRunLedTime(MRS_COLLECTOR_RUN_LED_NORMAL);
    }
    else
    {
        mrsCollectorSetRunLedTime(MRS_COLLECTOR_RUN_LED_NO_METER);
    }

    HI_MDM_LED_SetSearchMeterStatus(LED_SEARCH_METER_END);

    pstCtx->stRunning.bFirstOver = HI_TRUE;
    mrsStaSearchSetStatus(MRS_SEARCH_STATUS_IDLE);
    mrsStaSearchSetEndReason(MRS_SEARCH_END_REASON_FINISH);

    mrsStaSetMeterListToMac(pstSmList, HI_TRUE);

    mrsSrvEmptyQueue(&pstCtx->stRunning.unClt.stCltII.stQueue, mrsStaQueueFree);

    return HI_ERR_SUCCESS;
}


HI_PRV HI_U32 mrsCltiiSearchCheck(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_STA_SRV_CTX_STRU * pstSta = mrsStaGetContext();
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    MRS_StopTimer(MRS_STA_TIMER_SM_FRAME);
    MRS_StopTimer(MRS_STA_TIMER_SM_BYTE);

    pstRunning->bFrameTimerFlag = HI_FALSE;
    pstRunning->bByteTimerFlag = HI_FALSE;

    mrsStaTryDeQueue(&pstSta->stQueModule, mrsStaQueueFree);

    if (mrsCltiiSearchWaitMeterStartup())
    {
        pstRunning->ucRetry++;
        return mrsCltiiSearchSendFrame(pstCtx);
    }

    switch(mrsCltiiSearchGetState())
    {
    case MRS_CLTII_SEARCH_STATE_TRYFISRT:
        return mrsCltiiSearchCheckTryFirst(pstCtx);

    case MRS_CLTII_SEARCH_STATE_DETECT:
        return mrsCltiiSearchCheckDetect(pstCtx);

    case MRS_CLTII_SEARCH_STATE_VERIFY:
    case MRS_CLTII_SEARCH_STATE_EXPLORE_VERIFY:
        return mrsCltiiSearchCheckVerify(pstCtx);

    default:
        return mrsCltiiSearchCheckNormal(pstCtx);
    }
}


HI_U32 mrsCltiiSearchCheckTryFirst(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    if (((pstRunning->ucValidFrame + pstRunning->ucInvalidFrame) > 0)
        || (pstRunning->ucRetry >= MRS_SM_FIRST_METER_MAXRETRY))
    {
        mrsStaSearchSetFirstFlag(HI_FALSE);
        return mrsCltiiSearchStartPortal(pstCtx);
    }

    pstRunning->ucRetry++;
    return mrsCltiiSearchSendFrame(pstCtx);
}


HI_U32 mrsCltiiSearchCheckDetect(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    if (pstRunning->ucValidFrame == 0) {
        if (pstRunning->ucRetry < pstCtx->stSearchCfg.ucSearchRetry) {
            pstRunning->ucRetry++;
            return mrsCltiiSearchSendFrame(pstCtx);
        } else {
            mrsCltiiSearchDetectNvPowerProc(HI_FALSE, (HI_U8)(pstCtx->stNvMeterList.ucDetectCount - 1), 
                                            &pstCtx->stNvMeterList.astNvMeter[pstCtx->stNvMeterList.ucDetectCount - 1]);
        }
    }

    if (mrsCltiiSearchIsAllDetected(pstCtx)) {
        mrsCltiiSearchPowerOnProc(pstCtx->stNvMeterList.astNvMeter);
        return mrsCltiiSearchLoopStart(pstCtx);
    }

    return mrsCltiiSearchDetectProc(pstCtx);
}


HI_U32 mrsCltiiSearchCheckVerify(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    if ((pstRunning->ucValidFrame == 0) && (pstRunning->ucRetry < pstCtx->stSearchCfg.ucSearchRetry))
    {
        pstRunning->ucRetry++;
        return mrsCltiiSearchSendFrame(pstCtx);
    }

    if (mrsCltiiSearchIsAllVerified(pstCtx))
    {
        return mrsCltiiSearchVerifyFinish(pstCtx);
    }

    return mrsCltiiSearchVerifyProc(pstCtx);
}


HI_U32 mrsCltiiSearchCheckNormal(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);

    if (mrsCltiiSearchIsNeedVerify(pstCtx))
    {
        return mrsCltiiSearchVerifyStart(pstCtx);
    }

    if (pstRunning->ucInvalidFrame > 0)
    {
        // 多缩N字节过程中有乱码帧，结束多缩N字节过程，清空参数，回到LOOP状态
        if (MRS_CLTII_SEARCH_STATE_EXPLORE == mrsCltiiSearchGetState())
        {
            mrsCltiiSearchExploreFinish(pstCtx);
        }
        
        return mrsCltiiSearchNext(pstCtx);
    }

    if (pstRunning->ucRetry < pstCtx->stSearchCfg.ucSearchRetry)
    {        
        pstRunning->ucRetry++;
        return mrsCltiiSearchSendFrame(pstCtx);
    }

    // 仅有合法应答帧，且允许多缩N字节，开始多缩N字节
    if ((pstRunning->ucValidFrame > 0)
        && (MRS_CLTII_SEARCH_STATE_LOOP == mrsCltiiSearchGetState())
        && mrsCltiiSearchAllowExplore(pstCtx))
    {
        return mrsCltiiSearchExploreStart(pstCtx);
    }

    if ((pstCtx->stRx.ulSize == 0) && ((pstRunning->ucValidFrame + pstRunning->ucInvalidFrame) == 0))
    {
        if (mrsCltiiSearchIsMaskMeterAddr(mrsCltiiSearchTxAddr(pstCtx))
            && !mrsCltiiSearchIsBcSearchEnable(pstCtx))
        {
            return mrsCltiiSearchNextLoop(pstCtx);
        }
    }

    return mrsCltiiSearchNext(pstCtx);
}


HI_BOOL mrsCltiiSearchCheckChange(MRS_METER_LIST_S * pstMeterList, MRS_SEARCH_METERLIST_STRU *pstSmList)
{
    HI_U32 i, j;

    if (pstMeterList->ucMeterNum != pstSmList->ucMeterNum)
    {
        return HI_TRUE;
    }

    for (i = 0; i < PRODUCT_CFG_MRS_MAX_METER_IN_MAC_NUM; i++)
    {
        if (!pstMeterList->stMeterItem[i].ucValidFlag)
        {
            continue;
        }

        for (j = 0; j < (HI_U32)pstSmList->ucMeterNum; j++)
        {
            if (mrsToolsMemEq(pstMeterList->stMeterItem[i].ucMeterAddr, pstSmList->astMeter[j].ucMeterAddr, HI_METER_ADDR_LEN))
            {
                break;
            }
        }

        if (j == (HI_U32)pstSmList->ucMeterNum)
        {
            return HI_TRUE;
        }
    }

    return HI_FALSE;
}


HI_U32 mrsCltiiSearchEventInsert(MRS_STA_SEARCH_CTX *pstCtx, MRS_SEARCH_EVENT_ITEM *pstEventItem)
{
    MRS_SEARCH_EVENT_STRU *pstEvent = &(pstCtx->stEvent);
    MRS_SEARCH_EVENT_ITEM *pstItem = pstEvent->astItem;

    if (pstEvent->ucNum < MRS_SEARCH_EVENT_MAX)
    {
        (hi_void)memcpy_s(pstItem + pstEvent->ucNum, sizeof(MRS_SEARCH_EVENT_ITEM), pstEventItem, sizeof(MRS_SEARCH_EVENT_ITEM));
        pstEvent->ucNum++;
        return HI_ERR_SUCCESS;
    }

    (hi_void)memmove_s(pstItem, sizeof(MRS_SEARCH_EVENT_ITEM) * MRS_SEARCH_EVENT_MAX, 
        pstItem + 1, sizeof(MRS_SEARCH_EVENT_ITEM) * (MRS_SEARCH_EVENT_MAX - 1));
    (hi_void)memcpy_s(pstItem + MRS_SEARCH_EVENT_MAX - 1, sizeof(MRS_SEARCH_EVENT_ITEM), pstEventItem, sizeof(MRS_SEARCH_EVENT_ITEM));

    return HI_ERR_SUCCESS;
}


HI_BOOL mrsCltiiSearchWaitMeterStartup(HI_VOID)
{
    return (HI_BOOL)(HI_MDM_GetSeconds() < MRS_SM_STARTUP_WAIT_METER);
}


HI_U32 mrsCltiiSearchNext(MRS_STA_SEARCH_CTX * pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_U8 aucAddr[HI_METER_ADDR_LEN];

    do
    {
        HI_U8 ucProtocol = mrsCltiiSearchTxProtocol(pstCtx);

        mrsCltiiSearchGetTxAddr(pstCtx, aucAddr);

        if (mrsCltiiSearchIsMaskMeterAddr(aucAddr)
            && (pstRunning->ucInvalidFrame + pstRunning->ucValidFrame > 0))
        {
            aucAddr[0] = (METER_PROTO_698_45 == ucProtocol) ? 0xA0 : 0x00;
            break;
        }

        if (mrsCltiiSearchIsBcMeterAddr(aucAddr))
        {
            return mrsCltiiSearchNextLoop(pstCtx);
        }

        mrsCltiiSearchAddrIncrease(pstCtx, aucAddr, ucProtocol);

        if (MRS_CLTII_SEARCH_STATE_EXPLORE == mrsCltiiSearchGetState())
        {
            if ((0 == pstRunning->ucExploreByteCount) || mrsCltiiSearchIsBcMeterAddr(aucAddr))
            {
                mrsCltiiSearchExploreFinish(pstCtx);
            }
        }

        if (mrsCltiiSearchIsMaskMeterAddr(aucAddr) || mrsCltiiSearchIsBcMeterAddr(aucAddr))
        {            
            if (mrsCltiiSearchIsBcSearchEnable(pstCtx))
            {
                (hi_void)memcpy_s(aucAddr, HI_METER_ADDR_LEN, MRS_SM_ADDR_BROADCAST, HI_METER_ADDR_LEN);
                break;
            }

            return mrsCltiiSearchNextLoop(pstCtx);
        }
    } while (0);

    mrsCltiiCreateSearchFrame(pstCtx, aucAddr, mrsCltiiSearchTxProtocol(pstCtx));

    pstRunning->ucRetry = 0;
    pstRunning->ucValidFrame = 0;
    pstRunning->ucInvalidFrame = 0;

    return mrsCltiiSearchSendFrame(pstCtx);
}


HI_VOID mrsCltiiSearchAddrIncrease(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 *pucAddr, HI_U8 ucProtocol)
{
    if (METER_PROTO_698_45 == ucProtocol)
    {
        mrsCltiiSearchAddrIncrease698(pstCtx, pucAddr);
    }
    else
    {
        mrsCltiiSearchAddrIncrease645(pstCtx, pucAddr);
    }
}


HI_VOID mrsCltiiSearchAddrIncrease645(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 *pucAddr)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_U8 ucProtocol = mrsCltiiSearchTxProtocol(pstCtx);
    HI_U8 ucMask = MRS_SM_MASK_BYTE_GDW;
    HI_U8 ucMaskIndexMax = MRS_METER_ADDR_INDEX_5;
    HI_S32 i;

    if (pucAddr[ucMaskIndexMax] == ucMask)
    {
        if (pstRunning->ucInvalidFrame > 0)
        {
            mrsCltiiSearchAddrDepthIncrease(pucAddr, ucProtocol);
            return;
        }

        // 多缩N字节状态下,仅有合法帧且可缩，则缩1字节
        if ((MRS_CLTII_SEARCH_STATE_EXPLORE == mrsCltiiSearchGetState())
            && (pstRunning->ucValidFrame > 0)
            && (pstRunning->ucExploreByteCount < pstRunning->ucExploreByteCountMax))
        {
            mrsCltiiSearchExploreDepthIncrease(pstCtx, pucAddr, ucProtocol);
            return;
        }
    }

    for (i = ucMaskIndexMax; i >= 0; i--)
    {
        if (pucAddr[i] == ucMask)
        {
            continue;
        }

        pucAddr[i] = mrsCltiiSearchAddrByteIncrease(pucAddr[i], ucProtocol);
        if (pucAddr[i] != ucMask)
        {            
            break; 
        }

        if ((MRS_CLTII_SEARCH_STATE_EXPLORE == mrsCltiiSearchGetState())
            && (pstRunning->ucExploreByteCount > 0))
        {
            pstRunning->ucExploreByteCount--;
        }
    }
}


HI_VOID mrsCltiiSearchAddrIncrease698(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 *pucAddr)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    HI_S8 i;
    HI_S8 sAddrLength = HI_METER_ADDR_LEN * 2;

    if ((pucAddr[HI_METER_ADDR_LEN - 1] & 0xF0) == 0xA0)
    {
        if (pstRunning->ucInvalidFrame > 0)
        {
            for (i = 0; i < sAddrLength; i++)
            {
                if (((pucAddr[i/2] >> (4 * (hi_u8)(i % 2))) & 0xF) == 0xA)
                {
                    pucAddr[i/2] &= ~(HI_U8)(0xF << (4 * (hi_u8)(i % 2)));
                    break; 
                }
            }

            return;
        }
    }

    for (i = sAddrLength - 1; i >= 0; i--)
    {
        if (((pucAddr[i/2] >> (4 * (hi_u8)(i % 2))) & 0xF) == 0xA)
        {
            continue;
        }

        pucAddr[i/2] += (HI_U8)(1 << (4 * (hi_u8)(i % 2)));
        if (((pucAddr[i/2] >> (4 * (hi_u8)(i % 2))) & 0xF) != 0xA)
        {
            break;
        }
    }
}


HI_VOID mrsCltiiSearchAddrDepthIncrease(HI_U8* pucAddr, HI_U8 ucProtocol)
{
    HI_U8 ucSize = HI_METER_ADDR_LEN;
    HI_S32 i;

    for (i = 0; i < ucSize; i++)
    {
        if (pucAddr[i] == mrsCltiiSearchMaskByte(ucProtocol))
        {
            pucAddr[i] = 0x00;
            return;
        }
    }
}


HI_U8 mrsCltiiSearchAddrByteIncrease(HI_U8 ucByte, HI_U8 ucProtocol)
{
    HI_U8 ucMask = mrsCltiiSearchMaskByte(ucProtocol);
    HI_U8 ucTemp =  (HI_U8)HI_SYS_BCDHEX2DEC(ucByte) + 1;

    return (HI_U8)((ucTemp <= MRS_BCD_BYTE_DEC_MAX_VALUE) ? (HI_U8)HI_SYS_DEC2BCDHEX(ucTemp) : ucMask);
}


HI_U32 mrsCltiiSearchNextLoop(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SRV_QUEUE *pstQueue = &(pstCtx->stRunning.unClt.stCltII.stQueue);
    MRS_SEARCH_NODE *pstNode = (MRS_SEARCH_NODE *)mrsSrvDeQueue(pstQueue);

    if (pstNode)
    {
        mrsToolsFree(pstNode);
        pstNode = (MRS_SEARCH_NODE *)mrsSrvQueueTop(pstQueue);
    }

    if (!pstNode)
    {
        return mrsCltiiSearchFinish(pstCtx);
    }

    return mrsCltiiSearchLoopStartProc(pstCtx);
}


HI_BOOL mrsCltiiSearchIsMaskMeterAddr(HI_U8 *pucAddr)
{
    return mrsToolsMemEq(pucAddr, MRS_SM_ADDR_DEFAULT, HI_METER_ADDR_LEN);
}


HI_BOOL mrsCltiiSearchIsBcMeterAddr(HI_U8 *pucAddr)
{
    return mrsToolsMemEq(pucAddr, MRS_SM_ADDR_BROADCAST, HI_METER_ADDR_LEN);
}


HI_BOOL mrsCltiiSearchIsBcSearchEnable(MRS_STA_SEARCH_CTX *pstCtx)
{
    MRS_SEARCH_RUNNING_CLT_II *pstRunning = &(pstCtx->stRunning.unClt.stCltII);
    MRS_SEARCH_NODE *pstNode = (MRS_SEARCH_NODE *)mrsSrvQueueTop(&pstRunning->stQueue);

    if (!pstNode || !pstRunning->bEnableBcSearch)
    {
        return HI_FALSE;
    }
    
    return (HI_BOOL)(METER_PROTO_645_1997 == pstNode->stItem.ucProtocol);
}


HI_U32 mrsCltiiSearchGetNodeDI(MRS_STA_SEARCH_CTX* pstCtx, NV_APP_MRS_SM_NODE* pstNode)
{
    MRS_SEARCH_CFG_STRU *pstCfg = &(pstCtx->stSearchCfg);

    switch(mrsStaSearchGetNodeProtocol(pstNode))
    {
    case METER_PROTO_645_1997:
        return mrsCltiiSearchGetNodeDI97(pstCtx, pstNode);
        
    default:
        return (pstCfg->ulDI07);
    }   
}


HI_U8 mrsCltiiSearchGetNodeBaudRateIndex(HI_U8 ucParam)
{
    MRS_NV_SEARCH_PARAM_S stParam;
    (hi_void)memcpy_s(&stParam, sizeof(stParam), &ucParam, sizeof(ucParam));
    return (HI_U8)((((HI_U8)stParam.ucBrIdxLo) & 0x3) | ((((HI_U8)stParam.ucBrIdxHi) << 2) & 0x4));
}


HI_BOOL mrsCltiiSearchItemVisit97(HI_VOID *pItem, HI_VOID *pParam)
{
    MRS_SEARCH_NODE *pstNode = (MRS_SEARCH_NODE *)pItem;
    MRS_SEARCH_ITEM_EXTRA *pstExtra = (MRS_SEARCH_ITEM_EXTRA *)pParam;

    if ((pstExtra->ucProtocol == pstNode->stItem.ucProtocol)
        && ((HI_U8)pstExtra->ulDiIn == (HI_U8)pstNode->stItem.ulDI))
    {
        pstExtra->ulDiOut = pstNode->stItem.ulDI;
        pstExtra->bResult = HI_TRUE;
        return HI_TRUE;
    }

    return HI_FALSE;
}


HI_U32 mrsCltiiSearchGetNodeDI97(MRS_STA_SEARCH_CTX* pstCtx, NV_APP_MRS_SM_NODE* pstNode)
{
    MRS_SRV_QUEUE* pstQueue = &(pstCtx->stRunning.unClt.stCltII.stQueue);
    MRS_SEARCH_ITEM_EXTRA stExtra;

    stExtra.ulDiIn = pstNode->ucOption;
    stExtra.ucProtocol = METER_PROTO_645_1997;
    stExtra.bResult = HI_FALSE;
    stExtra.ulDiOut = 0;
    mrsSrvTraverseQueue(pstQueue, mrsCltiiSearchItemVisit97, &stExtra);

    if (stExtra.bResult)
    {
        return stExtra.ulDiOut;
    }

    return 0;
}


HI_BOOL mrsCltiiSearchIsNvNodeDI97Valid(HI_U8 ucOption)
{
    MRS_STA_SEARCH_CTX* pstCtx = mrsStaSearchCtx();
    MRS_SRV_QUEUE* pstQueue = &(pstCtx->stRunning.unClt.stCltII.stQueue);
    MRS_SEARCH_ITEM_EXTRA stExtra;
    
    stExtra.ulDiIn = ucOption;
    stExtra.ucProtocol = METER_PROTO_645_1997;
    stExtra.bResult = HI_FALSE;
    stExtra.ulDiOut = 0;
    mrsSrvTraverseQueue(pstQueue, mrsCltiiSearchItemVisit97, &stExtra);

    return stExtra.bResult;
}


HI_U16 mrsCltiiSearchGetDI97(MRS_STA_SEARCH_CTX *pstCtx, HI_U8 ucProtocol)
{
    if (METER_PROTO_645_1997 == ucProtocol)
    {
        return (HI_U16)mrsCltiiSearchTxDI(pstCtx);
    }

    return 0;
}


HI_U8* mrsCltiiSearchTxAddr(MRS_STA_SEARCH_CTX* pstCtx)
{
    return pstCtx->stRunning.unClt.stCltII.aucTxAddr;
}


HI_VOID mrsCltiiSearchGetTxAddr(MRS_STA_SEARCH_CTX* pstCtx, HI_OUT HI_U8* pucAddr)
{
    (hi_void)memcpy_s(pucAddr, HI_METER_ADDR_LEN, mrsCltiiSearchTxAddr(pstCtx), HI_METER_ADDR_LEN);
}


HI_U8 mrsCltiiSearchTxProtocol(MRS_STA_SEARCH_CTX* pstCtx)
{
    return (pstCtx->stRunning.unClt.stCltII.ucTxProtocol);
}


HI_U32 mrsCltiiSearchTxDI(MRS_STA_SEARCH_CTX* pstCtx)
{
    return (pstCtx->stRunning.unClt.stCltII.ulTxDI);
}


HI_U8 mrsCltiiSearchGetAddrDepth(HI_U8* pucAddr, HI_U8 ucProtocol)
{
    HI_U8 ucMask = mrsCltiiSearchMaskByte(ucProtocol);
    HI_U8 ucIndex = 0;

    for (ucIndex = 0; ucIndex < HI_METER_ADDR_LEN; ucIndex++)
    {
        if (pucAddr[ucIndex] == ucMask)
        {
            return ucIndex;
        }
    }

    return HI_METER_ADDR_LEN;
}


HI_U8 mrsCltiiSearchMaskByte(HI_U8 ucProtocol)
{
    HI_UNREF_PARAM(ucProtocol);
    return MRS_SM_MASK_BYTE_GDW;
}


/* BEGIN: PN: DTS2015042907633 MODIFY\ADD\DEL by cuiate/00233580 at 2015/4/29 */
/* BEGIN: PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/11 */
HI_U32 mrsCltiiGetMeterAddr(HI_U8 *pFrame, HI_U16 usFrameLen, HI_U8 ucProtocol, HI_U8 *pMeter)
{
    (hi_void)memcpy_s(pMeter, HI_METER_ADDR_LEN, pFrame + MRS_SM_POS_A, HI_METER_ADDR_LEN);

    // 07表，取地址域
    // 97表: 地址域为BCD码，取地址域
    if ((ucProtocol != METER_PROTO_645_1997) || mrsToolsNormalAddr(pMeter))
    {
        return HI_ERR_SUCCESS;
    }

    if ((MRS_645_FRAME_DATA_DI_SIZE_V97 + MRS_645_FRAME_LENGTH_MIN + HI_METER_ADDR_LEN) == usFrameLen)
    {
        HI_U8 *aucDIList[3] = {(HI_U8 *)MRS_645_DI_METER_ID, (HI_U8 *)MRS_645_DI_USER_ID, (HI_U8 *)MRS_645_DI_DEVICE_ID};
        HI_U8 aucMeterAddr[HI_METER_ADDR_LEN] = {0};
        HI_U8 i = 0;
        HI_U8 ucListCnt = (HI_U8)MRS_TOOLS_ALEN(aucDIList);

        // 查询DI是否是读"表号"、"用户号"、"设备号"
        for (i = 0; i < ucListCnt; i++)
        {
            if (mrsToolsMemEq(pFrame + MRS_SM_POS_DI, aucDIList[i], MRS_645_FRAME_DATA_DI_SIZE_V97))
            {
                break;
            }
        }

        // DI不是读"表号"、"用户号"、"设备号"，取地址域
        if (i >= ucListCnt)
        {
            return HI_ERR_SUCCESS;
        }

        (hi_void)memcpy_s(aucMeterAddr, HI_METER_ADDR_LEN, pFrame + MRS_SM_POS_DI + MRS_645_FRAME_DATA_DI_SIZE_V97, HI_METER_ADDR_LEN);
        mrs645DataDecode(aucMeterAddr, HI_METER_ADDR_LEN);

        if (mrsMeterAddrMatch(pMeter, aucMeterAddr))
        {
            (hi_void)memcpy_s(pMeter, HI_METER_ADDR_LEN, aucMeterAddr, HI_METER_ADDR_LEN);
        }
    }

    return HI_ERR_SUCCESS;
}
/* END:   PN: DTS2015051601564 MODIFY\ADD\DEL by cuiate/00233580 at 2015/5/11 */
/* END:   PN: DTS2015042907633 MODIFY\ADD\DEL by cuiate/00233580 at 2015/4/29 */




#endif //defined(PRODUCT_CFG_PRODUCT_TYPE_STA)

