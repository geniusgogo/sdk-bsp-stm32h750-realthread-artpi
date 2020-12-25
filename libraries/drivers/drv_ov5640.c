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

struct ov5640_dev_t
{
    struct rt_device parent;
    // sccb/i2c
    struct rt_i2c_bus_device *i2c_bus;
    rt_device_t dcmi_dev;
};

static rt_err_t _ov5640_init(rt_device_t dev)
{
    struct ov5640_dev_t *ov5640_dev = (struct ov5650_dev_t *)dev;
    uint16_t ov5640_id;

    RT_ASSERT(dev != RT_NULL);

    // 1: init powerup reset
    rt_pin_mode(OV5640_RST_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(OV5640_RST_PIN, PIN_LOW);
    rt_thread_mdelay(6); // delay >= 5 + 1ms
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
    struct ov5640_dev_t *ov5640_dev = (struct ov5650_dev_t *)dev;

    RT_ASSERT(dev != RT_NULL);

    LOG_D("ov5640 open start tick:%u", rt_tick_get());

    ov5640_softreset(ov5640_dev->i2c_bus);
    ov5640_reg_init(ov5640_dev->i2c_bus);
    ov5640_flash_strobe(ov5640_dev->i2c_bus, 1);
    rt_thread_mdelay(50);
    ov5640_flash_strobe(ov5640_dev->i2c_bus, 0);

    ov5640_rgb565_mode(ov5640_dev->i2c_bus); //RGB565模式
    if (ov5640_focus_init(ov5640_dev->i2c_bus) != 0)
    {
        LOG_W("init auto focus failure");
    }

    ov5640_light_mode(ov5640_dev->i2c_bus, 0);          //自动模式
    ov5640_color_saturation(ov5640_dev->i2c_bus, 3);    //色彩饱和度0
    ov5640_brightness(ov5640_dev->i2c_bus, 4);          //亮度0
    ov5640_contrast(ov5640_dev->i2c_bus, 3);            //对比度0
    ov5640_sharpness(ov5640_dev->i2c_bus, 33);          //自动锐度
    if (ov5640_focus_constant(ov5640_dev->i2c_bus) != 0)//启动持续对焦
    {
        LGO_W("enable focus constant failure");
    }

    LOG_D("ov5640 config end tick:%u", rt_tick_get());
    /*
    My_DCMI_Init();            //DCMI配置
    DCMI_DMA_Init((u32)&LCD->LCD_RAM,1,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Disable);//DCMI DMA配置
     OV5640_OutSize_Set(4,0,lcddev.width,lcddev.height);
    DCMI_Start();         //启动传输
    */
    ov5640_outsize_set(ov5640_dev->i2c_bus, (1280 - 800) / 2, (800 - 480) / 2, 800, 480);
    return rt_device_open(ov5640_dev->dcmi_dev, RT_DEVICE_FLAG_RDONLY);
}

static rt_err_t _ov5640_close(rt_device_t dev)
{
    struct ov5640_dev_t *ov5640_dev = (struct ov5650_dev_t *)dev;

    RT_ASSERT(dev != RT_NULL);

    ov5640_softpowerdown(ov5640_dev->i2c_bus);
    return RT_EOK;
}

static rt_size_t _ov5640_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size);
static rt_size_t _ov5640_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size);
static rt_err_t _ov5640_control(rt_device_t dev, int cmd, void *args);
