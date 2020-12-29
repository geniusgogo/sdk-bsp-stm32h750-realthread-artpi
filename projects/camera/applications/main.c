/*
 * Copyright (c) 2015-2020, xieyangrun
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-29     xieyangrun   Change Copyright
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "drv_common.h"

#define LED_PIN GET_PIN(I, 8)

static struct rt_semaphore frame_sem;
volatile uint32_t lcd_line_pos = 0;
struct rt_device_graphic_info info;

void camera_frame_data_cb(void *dcmi_buffer)
{
    // rt_kprintf("frame: %u, line_pos:%u\n", rt_tick_get(), lcd_line_pos);
    // if (lcd_line_pos != 11)
    // {
    //     DMA2_Stream1->CR&=~(1<<0);
    //    memcpy(info.framebuffer+(lcd_line_pos * 32768), dcmi_buffer, DMA2_Stream1->NDTR);
    //     DMA2_Stream1->CR|= (1<<0);
    // }
    // lcd_line_pos = 0;
    // rt_sem_release(&frame_sem);
}

void camera_line_data_cb(void *dcmi_buffer)
{
    // rt_mb_send(&dcmi_mb, (uint32_t)dcmi_buffer);

    memcpy(info.framebuffer+(lcd_line_pos * 800 * 2), (void*)dcmi_buffer, 800 * 2);
    lcd_line_pos++;
    if (lcd_line_pos == 446)
    {
        lcd_line_pos = 0;
        rt_sem_release(&frame_sem);
        DCMI_Stop();
    }
}

int main(void)
{
    struct rt_device * lcd;
    rt_device_t camera_dev;

    rt_sem_init(&frame_sem, "framesem", 0, RT_IPC_FLAG_FIFO);

    lcd = rt_device_find("lcd");
    if (lcd != RT_NULL)
    {
        rt_device_open(lcd, RT_DEVICE_OFLAG_RDWR);
        rt_device_control(lcd, RTGRAPHIC_CTRL_GET_INFO, &info);
    }
    else
    {
        rt_kprintf("lcd device not found\n");
    }

    camera_dev = rt_device_find("ov5640");
    if (camera_dev != RT_NULL)
    {
        rt_device_open(camera_dev, RT_DEVICE_OFLAG_RDONLY);
    }
    else
    {
        rt_kprintf("camera device not found\n");
    }

    while (1)
    {
        rt_sem_take(&frame_sem, RT_WAITING_FOREVER);
        // rt_device_control(lcd, RTGRAPHIC_CTRL_SET_FRAMEBUFFER, (void*)dcmi_buffer);
        rt_device_control(lcd, RTGRAPHIC_CTRL_RECT_UPDATE, 0);
        DCMI_Start();
        // rt_kprintf("update tick: %u\n", rt_tick_get());
        // rt_kprintf("buffer: %u, lcdupdate tick %u, done tick %u\n", dcmi_buffer, starttick, rt_tick_get());
    }

    return RT_EOK;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


