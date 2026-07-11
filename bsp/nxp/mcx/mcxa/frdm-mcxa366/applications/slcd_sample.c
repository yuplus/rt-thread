/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FRDM-MCXA366 onboard SLCD sample.
 *
 * MSH commands:
 *   slcd_demo                 - show 000000~999999 then all segments
 *   slcd_show <text>          - e.g. slcd_show 123456 / slcd_show 12:30
 *   slcd_time <hh> <mm>       - e.g. slcd_time 12 30
 *   slcd_clear                - clear display
 */

#include <rtthread.h>

#ifdef BSP_USING_SLCD

#include <stdlib.h>
#include "drv_slcd.h"

static void slcd_demo(int argc, char **argv)
{
    uint8_t num;
    char buf[7];

    (void)argc;
    (void)argv;

    if (rt_hw_slcd_init() != RT_EOK)
    {
        rt_kprintf("SLCD init failed\n");
        return;
    }

    rt_kprintf("SLCD demo start\n");

    for (num = 0; num < 10; num++)
    {
        rt_snprintf(buf, sizeof(buf), "%u%u%u%u%u%u",
                    num, num, num, num, num, num);
        rt_hw_slcd_display_string(buf);
        rt_thread_mdelay(400);
    }

    rt_hw_slcd_display_time(12, 30);
    rt_thread_mdelay(1000);

    rt_hw_slcd_display_all();
    rt_thread_mdelay(800);
    rt_hw_slcd_clear();
    rt_hw_slcd_display_string("888888");

    rt_kprintf("SLCD demo done, showing 888888\n");
}
MSH_CMD_EXPORT(slcd_demo, SLCD digit and segment demo);

static void slcd_show(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: slcd_show <text>\n");
        rt_kprintf("  slcd_show 123456\n");
        rt_kprintf("  slcd_show 12:30\n");
        return;
    }

    if (rt_hw_slcd_init() != RT_EOK)
    {
        rt_kprintf("SLCD init failed\n");
        return;
    }

    rt_hw_slcd_clear();
    rt_hw_slcd_display_string(argv[1]);
    rt_kprintf("SLCD show: %s\n", argv[1]);
}
MSH_CMD_EXPORT(slcd_show, Show text on SLCD. eg: slcd_show 12:30);

static void slcd_time(int argc, char **argv)
{
    int hour;
    int minute;

    if (argc < 3)
    {
        rt_kprintf("Usage: slcd_time <hh> <mm>\n");
        return;
    }

    if (rt_hw_slcd_init() != RT_EOK)
    {
        rt_kprintf("SLCD init failed\n");
        return;
    }

    hour = atoi(argv[1]);
    minute = atoi(argv[2]);
    if ((hour < 0) || (hour > 99) || (minute < 0) || (minute > 99))
    {
        rt_kprintf("Invalid time\n");
        return;
    }

    rt_hw_slcd_display_time((uint8_t)hour, (uint8_t)minute);
    rt_kprintf("SLCD time: %02d:%02d\n", hour, minute);
}
MSH_CMD_EXPORT(slcd_time, Show HH:MM on SLCD. eg: slcd_time 12 30);

static void slcd_clear(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (rt_hw_slcd_init() != RT_EOK)
    {
        rt_kprintf("SLCD init failed\n");
        return;
    }

    rt_hw_slcd_clear();
    rt_kprintf("SLCD cleared\n");
}
MSH_CMD_EXPORT(slcd_clear, Clear SLCD display);

static int slcd_sample_init(void)
{
    if (rt_hw_slcd_init() != RT_EOK)
    {
        return -RT_ERROR;
    }

    rt_hw_slcd_display_time(12, 30);
    rt_kprintf("SLCD ready. Try: slcd_show 12:30 / slcd_time 12 30\n");
    return RT_EOK;
}
INIT_APP_EXPORT(slcd_sample_init);

#endif /* BSP_USING_SLCD */
