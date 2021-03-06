/*----------------------------------------------------------------------------
 * Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei LiteOS of U.S. and the country in which you are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#include "los_sys.ph"
#include "los_tick.h"
#include "los_task.ph"
#include "los_task.h"
#include "los_config.h"
#include "los_printf.h"
#include "los_swtmr.h"
#include "los_swtmr.ph"
#include "los_timeslice.ph"
#include "los_memory.ph"
#include "hisoc/uart.h"
//#include "los_mac.h"

#include "los_sem.ph"
#include "los_mux.ph"
#include "los_queue.ph"

#ifdef LOSCFG_LIB_LIBC
#include "string.h"
#include "stdio.h"
#endif
#ifdef LOSCFG_COMPAT_LINUX
#include "linux/workqueue.h"
#endif
#ifdef LOSCFG_DRIVERS_UART
#include "console.h"
#endif
#ifdef LOSCFG_KERNEL_TICKLESS
#include "los_tickless.ph"
#endif

extern SWTMR_PROC_FUNC update_sched_clock(void);
LITE_OS_SEC_DATA_MINOR UINT16 g_swtmr_id;

user_get_timebase_us  g_func_get_cur_us;
user_get_timebase_ms  g_func_get_cur_ms;
user_get_timebase_s   g_func_get_cur_s;

LITE_OS_SEC_TEXT_INIT UINT32 los_reg_timebase_hook (user_get_timebase_us pfunc_us,
                                                            user_get_timebase_ms pfunc_ms,
                                                            user_get_timebase_s  pfunc_s)
{
    UINT32 uwIntSave;

    uwIntSave = LOS_IntLock();
    g_func_get_cur_us = pfunc_us;
    g_func_get_cur_ms = pfunc_ms;
    g_func_get_cur_s = pfunc_s;
    LOS_IntRestore(uwIntSave);
    return 0;
}

LITE_OS_SEC_TEXT_INIT VOID sched_clock_swtmr(VOID)
{
    UINT32 uwRet;
    uwRet = LOS_SwtmrCreate(100, LOS_SWTMR_MODE_PERIOD, (SWTMR_PROC_FUNC)update_sched_clock, &g_swtmr_id, 0);
    if(uwRet != LOS_OK)
    {
        PRINT_ERR("LOS_SwtmrCreate error %d\n", uwRet);
        return;
    }
    uwRet = LOS_SwtmrStart(g_swtmr_id);
    if(uwRet != LOS_OK)
    {
        PRINT_ERR("LOS_SwtmrStart error %d\n", uwRet);
        return;
    }
}

LITE_OS_SEC_TEXT_INIT VOID osRegister(VOID)
{
    g_uwSysClock = OS_SYS_CLOCK;
    g_uwTickPerSecond =  LOSCFG_BASE_CORE_TICK_PER_SECOND;

    return;
}

LITE_OS_SEC_TEXT_INIT VOID osStart(void)
{
 #if (LOSCFG_BASE_CORE_TICK_HW_TIME == YES)
    extern VOID osTickStart(VOID);
    osTickStart();
//    sched_clock_swtmr();
 #endif
    osStartToRun();
}
extern VOID osSecPageInit(VOID);
LITE_OS_SEC_TEXT_INIT int osMain(void)
{
    UINT32 uwRet;
    osSecPageInit();

    osRegister();

#if OS_SYS_NOCACHEMEM_SIZE
    uwRet = osNocacheMemSystemInit();
    if (uwRet != LOS_OK)
    {
        PRINT_ERR("osNocacheMemSystemInit error %d\n", uwRet);
        return uwRet;
    }
#endif
    #if (LOSCFG_PLATFORM_HWI == YES)
    extern VOID osHwiInit(void);
        osHwiInit();
    #endif
    extern UINT32 osTickInit(UINT32 uwSystemClock, UINT32 uwTickPerSecond);
    uwRet = osTickInit(g_uwSysClock, LOSCFG_BASE_CORE_TICK_PER_SECOND);

    if (uwRet != LOS_OK)
    {
        return uwRet;
    }
#ifdef LOSCFG_PLATFORM_UART_WITHOUT_VFS
#ifdef LOSCFG_SHELL
    uart_hwiCreate();
#endif //LOSCFG_SHELL
#endif
    uwRet = osTaskInit();
    if (uwRet != LOS_OK)
    {
        PRINT_ERR("osTaskInit error\n");
        return uwRet;
    }
    #if (LOSCFG_BASE_CORE_TSK_MONITOR == YES)
        osTaskMonInit();
    #endif

    #ifdef LOSCFG_KERNEL_CPUP
    {
        extern UINT32 osCpupInit(VOID);
        uwRet = osCpupInit();
        if (uwRet != LOS_OK)
        {
            PRINT_ERR("osCpupInit error\n");
            return uwRet;
        }
    }
    #endif

    #if (LOSCFG_BASE_IPC_SEM == YES)
    {
        uwRet = osSemInit();
        if (uwRet != LOS_OK)
        {
            return uwRet;
        }
    }
    #endif

	#if (LOSCFG_BASE_IPC_MUX == YES)
    {
        uwRet = osMuxInit();
        if (uwRet != LOS_OK)
        {
            return uwRet;
        }
    }
    #endif

    #if (LOSCFG_BASE_IPC_QUEUE == YES)
    {
        uwRet = osQueueInit();
        if (uwRet != LOS_OK)
        {
            /* PRINT_ERR("osQueueInit error\n"); */
            return uwRet;
        }
    }
    #endif

    #if (LOSCFG_BASE_CORE_SWTMR == YES)
    {
        uwRet = osSwTmrInit();
        if (uwRet != LOS_OK)
        {
            /* PRINT_ERR("osSwTmrInit error\n"); */
            return uwRet;
        }
    }
    #endif

#if(LOSCFG_BASE_CORE_TIMESLICE == YES)
    osTimesliceInit();
#endif

#ifdef LOSCFG_KERNEL_TICKLESS
    g_bTicklessFlag =1;
    g_uwMinSleepTicks = LOSCFG_MIN_SLEEP_TICKS;
#endif
    uwRet = osIdleTaskCreate();
    if (uwRet != LOS_OK)
    {
        return uwRet;
    }

#ifdef LOSCFG_TEST
    extern void Watchdog_Disable(void);
    Watchdog_Disable();
    extern UINT32 los_TestInit(VOID);
    uwRet = los_TestInit();
#else /* LOSCFG_PLATFORM_OSAPPINIT */
    UINT32 osAppInit(VOID);
    uwRet = osAppInit();
#endif
    if (uwRet != LOS_OK) {
        return uwRet;
    }
    return LOS_OK;
}

LITE_OS_SEC_TEXT_INIT int main(void)
{
    UINT32 uwRet;

    extern char __bss_end;
    uwRet = osMemSystemInit((UINT32)&__bss_end);
    if (uwRet != LOS_OK)
    {
        return uwRet;
    }

#ifdef LOSCFG_PLATFORM_UART_WITHOUT_VFS
    uart_init();
#endif
    PRINTK("********hello Huawei LiteOS ARM926********\n");
    PRINTK("\nversion : %s\nopen-version : %s\n", VER, OPEN_VER);
    PRINTK("build date : %s %s\n\n",__DATE__,__TIME__);
    PRINTK("**********************************\n");
    uwRet = osMain();
    if (uwRet != LOS_OK)
    {
        return LOS_NOK;
    }

    osStart();

    for (;;);
}

extern void app_init(void);
UINT32 osAppInit(VOID)
{
#ifdef LOSCFG_PLATFORM_OSAPPINIT
    UINT32 ipctaskid;
    TSK_INIT_PARAM_S stappTask;
    UINT32 uwRet;
#ifdef LOSCFG_FS_VFS
    extern void los_vfs_init(void);
    los_vfs_init();
#endif
#ifdef LOSCFG_COMPAT_LINUX
    (void)init_default_workqueue(g_cfg_uwWorkMaxNum);
#endif
    memset(&stappTask, 0, sizeof(TSK_INIT_PARAM_S));
    stappTask.pfnTaskEntry = (TSK_ENTRY_FUNC)app_init;
    stappTask.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    stappTask.pcName = "app_Task";
    stappTask.usTaskPrio = LOSCFG_BASE_CORE_TSK_DEFAULT_PRIO;
    stappTask.uwResved   = LOS_TASK_STATUS_DETACHED;
    uwRet = LOS_TaskCreate(&ipctaskid, &stappTask);
    PRINTK("**************osAppInit**************\n");
    if (LOS_OK != uwRet) {
        return uwRet;
    }
#endif /* LOSCFG_PLATFORM_OSAPPINIT */
    return 0;

}

