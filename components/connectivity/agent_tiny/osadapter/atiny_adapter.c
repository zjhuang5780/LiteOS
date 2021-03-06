/*----------------------------------------------------------------------------
 * Copyright (c) <2016-2018>, <Huawei Technologies Co., Ltd>
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

#include "atiny_adapter.h"
#include "stm32f4xx.h"
#include "stm32f4xx_hal_rng.h"
#include "dwt.h"
#include "los_memory.h"
#include "los_sys.ph"
#include "los_sem.ph"
#include "los_tick.ph"

extern RNG_HandleTypeDef hrng;

#define ATINY_CNT_MAX_WAITTIME 0xFFFFFFFF
#define LOG_BUF_SIZE (256)

static uint64_t osKernelGetTickCount (void)
{
    uint64_t ticks;
    UINTPTR uvIntSave;

    if(OS_INT_ACTIVE)
    {
        ticks = 0U;
    }
    else
    {
        uvIntSave = LOS_IntLock();
        ticks = g_ullTickCount;
        LOS_IntRestore(uvIntSave);
    }

    return ticks;
}

uint64_t atiny_gettime_ms(void)
{
    return osKernelGetTickCount() * (OS_SYS_MS_PER_SECOND / LOSCFG_BASE_CORE_TICK_PER_SECOND);
}

void atiny_usleep(unsigned long usec)
{
    delayus((uint32_t)usec);
}

int atiny_random(unsigned char* output, size_t len)
{
    size_t i;
    uint32_t random_number;

    for (i = 0; i < len; i += sizeof(uint32_t))
    {
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random_number) != HAL_OK)
        {
            return -1;
        }
        memcpy(output + i, &random_number,
               sizeof(uint32_t) > len - i ? len - i : sizeof(uint32_t));
    }

    return 0;
}

void* atiny_malloc(size_t size)
{
    return LOS_MemAlloc(m_aucSysMem0, size);
}

void atiny_free(void* ptr)
{
    (void)LOS_MemFree(m_aucSysMem0, ptr);
}

int atiny_snprintf(char* buf, unsigned int size, const char* format, ...)
{
    int     ret;
    va_list args;

    va_start(args, format);
    ret = vsnprintf(buf, size, format, args);
    va_end(args);

    return ret;
}

int atiny_printf(const char* format, ...)
{
    int ret;
    char str_buf[LOG_BUF_SIZE] = {0};
    va_list list;

    memset(str_buf, 0, LOG_BUF_SIZE);
    va_start(list, format);
    ret = vsnprintf(str_buf, LOG_BUF_SIZE, format, list);
    va_end(list);

    printf("%s", str_buf);

    return ret;
}

#if (LOSCFG_BASE_IPC_SEM == YES)

void* atiny_mutex_create(void)
{
    uint32_t uwRet;
    uint32_t uwSemId;

    if (OS_INT_ACTIVE)
    {
        return NULL;
    }

    uwRet = LOS_BinarySemCreate(1, (UINT32*)&uwSemId);

    if (uwRet == LOS_OK)
    {
        return (void*)(GET_SEM(uwSemId));
    }
    else
    {
        return NULL;
    }
}

void atiny_mutex_destroy(void* mutex)
{
    if (OS_INT_ACTIVE)
    {
        return;
    }

    if (mutex == NULL)
    {
        return;
    }

    (void)LOS_SemDelete(((SEM_CB_S*)mutex)->usSemID);
}

void atiny_mutex_lock(void* mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    if (OS_INT_ACTIVE)
    {
        return;
    }

    (void)LOS_SemPend(((SEM_CB_S*)mutex)->usSemID, ATINY_CNT_MAX_WAITTIME);
}

void atiny_mutex_unlock(void* mutex)
{
    if (mutex == NULL)
    {
        return;
    }

    (void)LOS_SemPost(((SEM_CB_S*)mutex)->usSemID);
}

#else

void* atiny_mutex_create(void) { return NULL; }

void atiny_mutex_destroy(void* mutex) { ((void)mutex); }

void atiny_mutex_lock(void* mutex) { ((void)mutex); }

void atiny_mutex_unlock(void* mutex) { ((void)mutex); }

#endif /* LOSCFG_BASE_IPC_SEM == YES */

