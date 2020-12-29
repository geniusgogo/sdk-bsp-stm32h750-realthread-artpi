/*
 * Copyright (c) 2015-2020, xieyangrun
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-29     xieyangrun   Change Copyright
 */

#include "board.h"
#include <rtdevice.h>
#include <drv_ov5640.h>
#include <drv_dcmi.h>
#include <drv_gpio.h>
#include "ov5640_func.h"

#define DRV_DEBUG
//#define CAMERA_DUMP
#define LOG_TAG "drv.ov5640"
#include <drv_log.h>

#define OV5640_SCCB_DEVICE_NAME "i2c2"

#define OV5640_RST_PIN GET_PIN(A, 3)

#define FRAME_SIZE (800 * 480 * 2)
#define LINE_SIZE (800 * 2)

struct ov5640_dev_t
{
    struct rt_device parent;
    // sccb/i2c
    struct rt_i2c_bus_device *i2c_bus;
    rt_device_t dcmi_dev;

    void *frame_dma_buffer1, *frame_dma_buffer2;
};

static uint32_t framebuffer1[LINE_SIZE];
static uint32_t framebuffer2[LINE_SIZE];

static struct ov5640_dev_t _ov5640_dev;

static rt_err_t _ov5640_init(rt_device_t dev)
{
    struct ov5640_dev_t *ov5640_dev = (struct ov5640_dev_t *)dev;
    uint16_t ov5640_id;

    RT_ASSERT(dev != RT_NULL);

    // 1: init powerup reset
    rt_pin_mode(OV5640_RST_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(OV5640_RST_PIN, PIN_LOW);
    rt_thread_mdelay(21); // delay >= 5 + 1ms
    rt_pin_write(OV5640_RST_PIN, PIN_HIGH);
    rt_thread_mdelay(21); // delay >= 20ms

    // 2: init sccb/i2c
    if (!ov5640_dev->i2c_bus)
    {
        LOG_E("i2c bus no configuration");
        return -RT_EINVAL;
    }

    // 3: read ov5640 id
    ov5640_id = ov5640_id_read(ov5640_dev->i2c_bus);
    if (ov5640_id != OV5640_DEVICE_ID)
    {
        LOG_E("device id read mismatch: (%04x != %04x)", ov5640_id, OV5640_DEVICE_ID);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t _ov5640_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct ov5640_dev_t *ov5640_dev = (struct ov5640_dev_t *)dev;

    RT_ASSERT(dev != RT_NULL);

    LOG_D("ov5640 open start tick:%u", rt_tick_get());

    ov5640_softreset(ov5640_dev->i2c_bus);
    rt_thread_mdelay(30);

    LOG_D("ov5640 reg init tick:%u", rt_tick_get());
    ov5640_reg_init(ov5640_dev->i2c_bus);

    LOG_D("ov5640 flash strobe tick:%u", rt_tick_get());
    ov5640_flash_strobe(ov5640_dev->i2c_bus, 1);
    rt_thread_mdelay(50);
    ov5640_flash_strobe(ov5640_dev->i2c_bus, 0);

    ov5640_rgb565_mode(ov5640_dev->i2c_bus); //RGB565模式
#if 0
    if (ov5640_focus_init(ov5640_dev->i2c_bus) != 0)
    {
        LOG_W("init auto focus failure");
    }
#endif
    ov5640_light_mode(ov5640_dev->i2c_bus, 0);          //自动模式
    ov5640_color_saturation(ov5640_dev->i2c_bus, 3);    //色彩饱和度0
    ov5640_brightness(ov5640_dev->i2c_bus, 4);          //亮度0
    ov5640_contrast(ov5640_dev->i2c_bus, 3);            //对比度0
    ov5640_sharpness(ov5640_dev->i2c_bus, 33);          //自动锐度
#if 0
    if (ov5640_focus_constant(ov5640_dev->i2c_bus) != 0)//启动持续对焦
    {
        LOG_W("enable focus constant failure");
    }
#endif
    LOG_D("ov5640 config end tick:%u", rt_tick_get());

    ov5640_outsize_set(ov5640_dev->i2c_bus, 4, 0, 800, 480);
    if (rt_device_open(ov5640_dev->dcmi_dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        LOG_E("dcmi device open failure");
        return -RT_ERROR;
    }

    ov5640_dev->frame_dma_buffer1 = framebuffer1;//rt_calloc(LINE_SIZE, 1);
    if (!ov5640_dev->frame_dma_buffer1)
    {
        LOG_E("camera frame dma buffer1 alloc failure");
        return -RT_ENOMEM;
    }
    ov5640_dev->frame_dma_buffer2 = framebuffer2;//rt_calloc(LINE_SIZE, 1);
    if (!ov5640_dev->frame_dma_buffer2)
    {
        LOG_E("camera frame dma buffer2 alloc failure");
        return -RT_ENOMEM;
    }
    rt_hw_dcmi_dma_config((uint32_t)ov5640_dev->frame_dma_buffer1, (uint32_t)ov5640_dev->frame_dma_buffer2, LINE_SIZE / 4);

    DCMI_Start();

    LOG_D("camera open successful");

    return RT_EOK;
}

static rt_err_t _ov5640_close(rt_device_t dev)
{
    struct ov5640_dev_t *ov5640_dev = (struct ov5640_dev_t *)dev;

    RT_ASSERT(dev != RT_NULL);

    ov5640_softpowerdown(ov5640_dev->i2c_bus);
    return RT_EOK;
}

static rt_size_t _ov5640_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    return 0;
}

static rt_size_t _ov5640_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    return 0;
}

static rt_err_t _ov5640_control(rt_device_t dev, int cmd, void *args)
{

    return -RT_ENOSYS;
}

void camera_dma_data_process(void)
{
    // rt_kprintf("dma cb tick:%u\n", rt_tick_get());
    void camera_line_data_cb(void *dcmi_buffer);
    if (DMA2_Stream1->CR & (1<<19))
    {
        camera_line_data_cb(_ov5640_dev.frame_dma_buffer1);
    }
    else
    {
        camera_line_data_cb(_ov5640_dev.frame_dma_buffer2);
    }
}

void camera_frame_data_process(void)
{
    void camera_frame_data_cb(void *);
    // rt_kprintf("frame cb tick:%u\n", rt_tick_get());

    // if (DMA2_Stream1->CR & (1<<19))
    // {
    //     camera_frame_data_cb(_ov5640_dev.frame_dma_buffer1);
    // }
    // else
    // {
    //     camera_frame_data_cb(_ov5640_dev.frame_dma_buffer2);
    // }
    camera_frame_data_cb(0);
}

#ifdef RT_USING_DEVICE_OPS
static const struct rt_device_ops _ov5640_dev_ops = {
    _ov5640_init,
    _ov5640_open,
    _ov5640_close,
    _ov5640_read,
    _ov5640_write,
    _ov5640_control
};

#endif

int rt_hw_camera_ov5640_init(void)
{
    _ov5640_dev.dcmi_dev = rt_device_find("dcmi");
    if (!_ov5640_dev.dcmi_dev)
    {
        LOG_E("camera dcmi device not found");
        return -RT_ENOSYS;
    }

    _ov5640_dev.i2c_bus = (struct rt_i2c_bus_device *)rt_device_find(OV5640_SCCB_DEVICE_NAME);
    if (!_ov5640_dev.i2c_bus)
    {
        LOG_E("camera sccb device not found");
        return -RT_ENOSYS;
    }

    _ov5640_dev.parent.type = RT_Device_Class_Miscellaneous;
#ifdef RT_USING_DEVICE_OPS
    _ov5640_dev.parent.ops = &_ov5640_dev_ops;
#else
    _ov5640_dev.parent.init = _ov5640_init;
    _ov5640_dev.parent.open = _ov5640_open;
    _ov5640_dev.parent.close = _ov5640_close;
    _ov5640_dev.parent.read = _ov5640_read;
    _ov5640_dev.parent.write = _ov5640_write;
    _ov5640_dev.parent.control = _ov5640_control;
#endif

    rt_device_register(&_ov5640_dev.parent, "ov5640", RT_DEVICE_FLAG_RDONLY | RT_DEVICE_FLAG_REMOVABLE | RT_DEVICE_FLAG_STANDALONE);

    LOG_I("camera init success!");
    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_camera_ov5640_init);
