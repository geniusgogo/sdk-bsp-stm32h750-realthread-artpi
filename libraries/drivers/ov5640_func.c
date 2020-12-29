/*
 * Copyright (c) 2015-2020, xieyangrun
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-29     xieyangrun   Change Copyright
 */

#include <rtdevice.h>
#include <rthw.h>
#include "ov5640_func.h"
#include "ov5640_regtable.h"

// #define OV5640_SCCB_SLAVE_ADDR 0x78
#define OV5640_SCCB_SLAVE_ADDR 0x3C

static void _ov5640_sccb_write(struct rt_i2c_bus_device *i2c_bus, uint16_t reg, uint8_t val)
{
    struct rt_i2c_msg msg;
    uint8_t sccb_stream[3];

    sccb_stream[0] = reg >> 8;
    sccb_stream[1] = reg & 0xFFu;
    sccb_stream[2] = val;
    msg.addr = OV5640_SCCB_SLAVE_ADDR;
    msg.flags = RT_I2C_WR | RT_I2C_IGNORE_NACK; // the ninth bit is don't care
    msg.buf = sccb_stream;
    msg.len = 3;

    rt_i2c_transfer(i2c_bus, &msg, 1);
}

static void _ov5640_sccb_read(struct rt_i2c_bus_device *i2c_bus, uint16_t reg, uint8_t *pval)
{
    struct rt_i2c_msg msg[2];
    uint8_t sccb_stream[2];

    sccb_stream[0] = reg >> 8;
    sccb_stream[1] = reg & 0xFFu;

    msg[0].addr = OV5640_SCCB_SLAVE_ADDR;
    msg[0].flags = RT_I2C_WR | RT_I2C_IGNORE_NACK; // the ninth bit is don't care
    msg[0].buf = sccb_stream;
    msg[0].len = 2;

    msg[1].addr = OV5640_SCCB_SLAVE_ADDR;
    msg[1].flags = RT_I2C_RD | RT_I2C_IGNORE_NACK; // the ninth bit is don't care
    msg[1].buf = pval;
    msg[1].len = 1;

    rt_i2c_transfer(i2c_bus, msg, 2);
}

void ov5640_softreset(struct rt_i2c_bus_device *i2c_bus)
{
    /*
        0x3008 SYSTEM CTROL0 0x02 RW
        System Control
        Bit[7]: Software reset
        Bit[6]: Software power down
        Bit[5:0]: Debug mode
    */
    _ov5640_sccb_write(i2c_bus, 0x3008, 0x82);
    // The reset pulse width should be greater than or equal to 1 ms.
    rt_hw_us_delay(1100);
}

void ov5640_softpowerdown(struct rt_i2c_bus_device *i2c_bus)
{
    /*
        0x3008 SYSTEM CTROL0 0x02 RW
        System Control
        Bit[7]: Software reset
        Bit[6]: Software power down
        Bit[5:0]: Debug mode
    */
    _ov5640_sccb_write(i2c_bus, 0x3008, 0x42);
}

uint16_t ov5640_id_read(struct rt_i2c_bus_device *i2c_bus)
{
    uint16_t ov5640_id;
    uint8_t id[2];

    /*
        0x300A CHIP ID HIGH BYTE 0x56 R Chip ID High Byte
        0x300B CHIP ID LOW BYTE 0x40 R Chip ID Low Byte
    */
    _ov5640_sccb_read(i2c_bus, 0x300Au, &id[0]);
    _ov5640_sccb_read(i2c_bus, 0x300Bu, &id[1]);

    ov5640_id = id[0];
    ov5640_id <<= 8;
    ov5640_id |= id[1];

    return ov5640_id;
}

void ov5640_reg_init(struct rt_i2c_bus_device *i2c_bus)
{
    int i;

    for (i = 0; i < sizeof(ov5640_init_reg_tbl) / sizeof(ov5640_init_reg_tbl[0]); i++)
    {
        _ov5640_sccb_write(i2c_bus, ov5640_init_reg_tbl[i][0], ov5640_init_reg_tbl[i][1]);
    }
}

//闪光灯控制
//mode:0,关闭
//     1,打开
void ov5640_flash_strobe(struct rt_i2c_bus_device *i2c_bus, uint8_t on)
{
    uint8_t regv;

    _ov5640_sccb_read(i2c_bus, 0x3016u, &regv);
    if (!(regv & 0x02))
    {
        regv |= 0X02;
        _ov5640_sccb_write(i2c_bus, 0x3016u, regv);
    }

    _ov5640_sccb_read(i2c_bus, 0x301Cu, &regv);
    if (!(regv & 0x02))
    {
        regv |= 0X02;
        _ov5640_sccb_write(i2c_bus, 0x301Cu, regv);
    }

    if (on)
    {
        _ov5640_sccb_read(i2c_bus, 0x3019u, &regv);
        if (!(regv & 0x02))
        {
            regv |= 0X02;
            _ov5640_sccb_write(i2c_bus, 0x3019u, regv);
        }
    }
    else
    {
        regv &= ~0x02u;
        _ov5640_sccb_write(i2c_bus, 0x3019u, regv);
    }
}

//OV5640切换为JPEG模式
void ov5640_jpeg_mode(struct rt_i2c_bus_device *i2c_bus)
{
    int i;

    //设置:输出JPEG数据
    for (i = 0; i < sizeof(ov5640_jpeg_reg_tbl) / sizeof(ov5640_jpeg_reg_tbl[0]); i++)
    {
        _ov5640_sccb_write(i2c_bus, ov5640_jpeg_reg_tbl[i][0], ov5640_jpeg_reg_tbl[i][1]);
    }
}

//OV5640切换为RGB565模式
void ov5640_rgb565_mode(struct rt_i2c_bus_device *i2c_bus)
{
    int i;

    //设置:RGB565输出
    for (i = 0; i < sizeof(ov5640_rgb565_reg_tbl) / sizeof(ov5640_rgb565_reg_tbl[0]); i++)
    {
        _ov5640_sccb_write(i2c_bus, ov5640_rgb565_reg_tbl[i][0], ov5640_rgb565_reg_tbl[i][1]);
    }
}

void ov5640_sxga_init(struct rt_i2c_bus_device *i2c_bus)
{
    int i;

    //初始化 OV5640,采用SXGA分辨率(1600*1200)
    for(i = 0; i < sizeof(ov5640_uxga_init_reg_tbl) / sizeof(ov5640_uxga_init_reg_tbl[0]); i++)
    {
         _ov5640_sccb_write(i2c_bus, ov5640_uxga_init_reg_tbl[i][0], ov5640_uxga_init_reg_tbl[i][1]);
    }
}

//初始化自动对焦
//返回值:0,成功;1,失败.
uint8_t ov5640_focus_init(struct rt_i2c_bus_device *i2c_bus)
{
    uint16_t i;
    uint16_t addr = 0x8000;
    uint8_t state = 0x8F;

    _ov5640_sccb_write(i2c_bus, 0x3000, 0x20);     // reset MCU
    for (i = 0; i < sizeof(ov5640_af_config); i++) // 发送配置数组
    {
        _ov5640_sccb_write(i2c_bus, addr, ov5640_af_config[i]);
        addr++;
    }

    _ov5640_sccb_write(i2c_bus, 0x3022, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3023, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3024, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3025, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3026, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3027, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3028, 0x00);
    _ov5640_sccb_write(i2c_bus, 0x3029, 0x7f);
    _ov5640_sccb_write(i2c_bus, 0x3000, 0x00);

    i = 0;
    do
    {
        _ov5640_sccb_read(i2c_bus, 0x3029, &state);
        rt_thread_mdelay(5);
        i++;
        if (i > 1000)
            return 1;
    } while (state != 0x70);

    return 0;
}

//执行一次自动对焦
//返回值:0,成功;1,失败.
uint8_t ov5640_focus_single(struct rt_i2c_bus_device *i2c_bus)
{
    uint8_t temp;
    uint16_t retry = 0;

    _ov5640_sccb_write(i2c_bus, 0x3022, 0x03); //触发一次自动对焦
    while (1)
    {
        retry++;
        _ov5640_sccb_read(i2c_bus, 0x3029, &temp); //检查对焦完成状态
        if (temp == 0x10)
            break; // focus completed
        rt_thread_mdelay(5);
        if (retry > 1000)
            return 1;
    }

    return 0;
}

//持续自动对焦,当失焦后,会自动继续对焦
//返回值:0,成功;其他,失败.
uint8_t ov5640_focus_constant(struct rt_i2c_bus_device *i2c_bus)
{
    uint8_t temp = 0;
    uint16_t retry = 0;

    _ov5640_sccb_write(i2c_bus, 0x3023, 0x01);
    _ov5640_sccb_write(i2c_bus, 0x3022, 0x08); //发送IDLE指令
    do
    {
        _ov5640_sccb_read(i2c_bus, 0x3023, &temp);
        retry++;
        if (retry > 1000)
            return 2;
        rt_thread_mdelay(5);
    } while (temp != 0x00);

    _ov5640_sccb_write(i2c_bus, 0x3023, 0x01);
    _ov5640_sccb_write(i2c_bus, 0x3022, 0x04); //发送持续对焦指令
    retry = 0;
    do
    {
        _ov5640_sccb_read(i2c_bus, 0x3023, &temp);
        retry++;
        if (retry > 1000)
            return 2;
        rt_thread_mdelay(5);
    } while (temp != 0x00); //0,对焦完成;1:正在对焦

    return 0;
}

//EV曝光补偿
//exposure:0~6,代表补偿-3~3.
void ov5640_exposure(struct rt_i2c_bus_device *i2c_bus, uint8_t exposure)
{
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    _ov5640_sccb_write(i2c_bus, 0x3a0f, OV5640_EXPOSURE_TBL[exposure][0]);
    _ov5640_sccb_write(i2c_bus, 0x3a10, OV5640_EXPOSURE_TBL[exposure][1]);
    _ov5640_sccb_write(i2c_bus, 0x3a1b, OV5640_EXPOSURE_TBL[exposure][2]);
    _ov5640_sccb_write(i2c_bus, 0x3a1e, OV5640_EXPOSURE_TBL[exposure][3]);
    _ov5640_sccb_write(i2c_bus, 0x3a11, OV5640_EXPOSURE_TBL[exposure][4]);
    _ov5640_sccb_write(i2c_bus, 0x3a1f, OV5640_EXPOSURE_TBL[exposure][5]);
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//白平衡设置
//0:自动
//1:日光sunny
//2,办公室office
//3,阴天cloudy
//4,家里home
void ov5640_light_mode(struct rt_i2c_bus_device *i2c_bus, uint8_t mode)
{
    uint8_t i;

    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    for (i = 0; i < 7; i++)
    {
        _ov5640_sccb_write(i2c_bus, 0x3400 + i, OV5640_LIGHTMODE_TBL[mode][i]); //设置饱和度
    }
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//色度设置
//sat:0~6,代表饱和度-3~3.
void ov5640_color_saturation(struct rt_i2c_bus_device *i2c_bus, uint8_t sat)
{
    uint8_t i;

    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    _ov5640_sccb_write(i2c_bus, 0x5381, 0x1c);
    _ov5640_sccb_write(i2c_bus, 0x5382, 0x5a);
    _ov5640_sccb_write(i2c_bus, 0x5383, 0x06);
    for (i = 0; i < 6; i++)
    {
        _ov5640_sccb_write(i2c_bus, 0x5384 + i, OV5640_SATURATION_TBL[sat][i]); //设置饱和度
    }
    _ov5640_sccb_write(i2c_bus, 0x538b, 0x98);
    _ov5640_sccb_write(i2c_bus, 0x538a, 0x01);
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//亮度设置
//bright:0~8,代表亮度-4~4.
void ov5640_brightness(struct rt_i2c_bus_device *i2c_bus, uint8_t bright)
{
    uint8_t brtval;

    if (bright < 4)
    {
        brtval = 4 - bright;
    }
    else
    {
        brtval = bright - 4;
    }
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    _ov5640_sccb_write(i2c_bus, 0x5587, brtval << 4);
    if (bright < 4)
    {
        _ov5640_sccb_write(i2c_bus, 0x5588, 0x09);
    }
    else
    {
        _ov5640_sccb_write(i2c_bus, 0x5588, 0x01);
    }
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//对比度设置
//contrast:0~6,代表亮度-3~3.
void ov5640_contrast(struct rt_i2c_bus_device *i2c_bus, uint8_t contrast)
{
    uint8_t reg0val = 0X00; //contrast=3,默认对比度
    uint8_t reg1val = 0X20;

    switch (contrast)
    {
    case 0: //-3
        reg1val = reg0val = 0X14;
        break;
    case 1: //-2
        reg1val = reg0val = 0X18;
        break;
    case 2: //-1
        reg1val = reg0val = 0X1C;
        break;
    case 4: //1
        reg0val = 0X10;
        reg1val = 0X24;
        break;
    case 5: //2
        reg0val = 0X18;
        reg1val = 0X28;
        break;
    case 6: //3
        reg0val = 0X1C;
        reg1val = 0X2C;
        break;
    }
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    _ov5640_sccb_write(i2c_bus, 0x5585, reg0val);
    _ov5640_sccb_write(i2c_bus, 0x5586, reg1val);
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//锐度设置
//sharp:0~33,0,关闭;33,auto;其他值,锐度范围.
void ov5640_sharpness(struct rt_i2c_bus_device *i2c_bus, uint8_t sharp)
{
    if (sharp < 33) //设置锐度值
    {
        _ov5640_sccb_write(i2c_bus, 0x5308, 0x65);
        _ov5640_sccb_write(i2c_bus, 0x5302, sharp);
    }
    else //自动锐度
    {
        _ov5640_sccb_write(i2c_bus, 0x5308, 0x25);
        _ov5640_sccb_write(i2c_bus, 0x5300, 0x08);
        _ov5640_sccb_write(i2c_bus, 0x5301, 0x30);
        _ov5640_sccb_write(i2c_bus, 0x5302, 0x10);
        _ov5640_sccb_write(i2c_bus, 0x5303, 0x00);
        _ov5640_sccb_write(i2c_bus, 0x5309, 0x08);
        _ov5640_sccb_write(i2c_bus, 0x530a, 0x30);
        _ov5640_sccb_write(i2c_bus, 0x530b, 0x04);
        _ov5640_sccb_write(i2c_bus, 0x530c, 0x06);
    }
}

//特效设置
//0:正常
//1,冷色
//2,暖色
//3,黑白
//4,偏黄
//5,反色
//6,偏绿
void ov5640_special_effects(struct rt_i2c_bus_device *i2c_bus, uint8_t eft)
{
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x03); //start group 3
    _ov5640_sccb_write(i2c_bus, 0x5580, OV5640_EFFECTS_TBL[eft][0]);
    _ov5640_sccb_write(i2c_bus, 0x5583, OV5640_EFFECTS_TBL[eft][1]); // sat U
    _ov5640_sccb_write(i2c_bus, 0x5584, OV5640_EFFECTS_TBL[eft][2]); // sat V
    _ov5640_sccb_write(i2c_bus, 0x5003, 0x08);
    _ov5640_sccb_write(i2c_bus, 0x3212, 0x13); //end group 3
    _ov5640_sccb_write(i2c_bus, 0x3212, 0xa3); //launch group 3
}

//设置图像输出大小
//OV5640输出图像的大小(分辨率),完全由该函数确定
//offx,offy,为输出图像在OV5640_ImageWin_Set设定窗口(假设长宽为xsize和ysize)上的偏移
//由于开启了scale功能,用于输出的图像窗口为:xsize-2*offx,ysize-2*offy
//width,height:实际输出图像的宽度和高度
//实际输出(width,height),是在xsize-2*offx,ysize-2*offy的基础上进行缩放处理.
//一般设置offx和offy的值为16和4,更小也是可以,不过默认是16和4
//返回值:0,设置成功
//    其他,设置失败
uint8_t ov5640_outsize_set(struct rt_i2c_bus_device *i2c_bus, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    _ov5640_sccb_write(i2c_bus, 0X3212, 0X03); //开始组3
    //以下设置决定实际输出尺寸(带缩放)
    _ov5640_sccb_write(i2c_bus, 0x3808, width >> 8);    //设置实际输出宽度高字节
    _ov5640_sccb_write(i2c_bus, 0x3809, width & 0xff);  //设置实际输出宽度低字节
    _ov5640_sccb_write(i2c_bus, 0x380a, height >> 8);   //设置实际输出高度高字节
    _ov5640_sccb_write(i2c_bus, 0x380b, height & 0xff); //设置实际输出高度低字节
                                                        //以下设置决定输出尺寸在ISP上面的取图范围
                                                        //范围:xsize-2*offx,ysize-2*offy
    _ov5640_sccb_write(i2c_bus, 0x3810, offx >> 8);     //设置X offset高字节
    _ov5640_sccb_write(i2c_bus, 0x3811, offx & 0xff);   //设置X offset低字节

    _ov5640_sccb_write(i2c_bus, 0x3812, offy >> 8);   //设置Y offset高字节
    _ov5640_sccb_write(i2c_bus, 0x3813, offy & 0xff); //设置Y offset低字节

    _ov5640_sccb_write(i2c_bus, 0X3212, 0X13); //结束组3
    _ov5640_sccb_write(i2c_bus, 0X3212, 0Xa3); //启用组3设置

    return 0;
}

//设置图像开窗大小(ISP大小),非必要,一般无需调用此函数
//在整个传感器上面开窗(最大2592*1944),用于OV5640_OutSize_Set的输出
//注意:本函数的宽度和高度,必须大于等于OV5640_OutSize_Set函数的宽度和高度
//     OV5640_OutSize_Set设置的宽度和高度,根据本函数设置的宽度和高度,由DSP
//     自动计算缩放比例,输出给外部设备.
//width,height:宽度(对应:horizontal)和高度(对应:vertical)
//返回值:0,设置成功
//    其他,设置失败
uint8_t ov5640_imagewin_set(struct rt_i2c_bus_device *i2c_bus, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height)
{
    uint16_t xst, yst, xend, yend;

    xst = offx;
    yst = offy;
    xend = offx + width - 1;
    yend = offy + height - 1;
    _ov5640_sccb_write(i2c_bus, 0X3212, 0X03); //开始组3
    _ov5640_sccb_write(i2c_bus, 0X3800, xst >> 8);
    _ov5640_sccb_write(i2c_bus, 0X3801, xst & 0XFF);
    _ov5640_sccb_write(i2c_bus, 0X3802, yst >> 8);
    _ov5640_sccb_write(i2c_bus, 0X3803, yst & 0XFF);
    _ov5640_sccb_write(i2c_bus, 0X3804, xend >> 8);
    _ov5640_sccb_write(i2c_bus, 0X3805, xend & 0XFF);
    _ov5640_sccb_write(i2c_bus, 0X3806, yend >> 8);
    _ov5640_sccb_write(i2c_bus, 0X3807, yend & 0XFF);
    _ov5640_sccb_write(i2c_bus, 0X3212, 0X13); //结束组3
    _ov5640_sccb_write(i2c_bus, 0X3212, 0Xa3); //启用组3设置
    return 0;
}
