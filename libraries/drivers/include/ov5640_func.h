#ifndef _OV5640_FUNC_H_
#define _OV5640_FUNC_H_

#define OV5640_DEVICE_ID 0x5640u

void ov5640_softreset(struct rt_i2c_bus_device *i2c_bus);
void ov5640_softpowerdown(struct rt_i2c_bus_device *i2c_bus);
uint16_t ov5640_id_read(struct rt_i2c_bus_device *i2c_bus);
void ov5640_reg_init(struct rt_i2c_bus_device *i2c_bus);

//闪光灯控制
//mode:0,关闭
//     1,打开
void ov5640_flash_strobe(struct rt_i2c_bus_device *i2c_bus, uint8_t on);

//OV5640切换为JPEG模式
void ov5640_jpeg_mode(struct rt_i2c_bus_device *i2c_bus);

//OV5640切换为RGB565模式
void ov5640_rgb565_mode(struct rt_i2c_bus_device *i2c_bus);

void ov5640_sxga_init(struct rt_i2c_bus_device *i2c_bus);

//初始化自动对焦
//返回值:0,成功;1,失败.
uint8_t ov5640_focus_init(struct rt_i2c_bus_device *i2c_bus);

//执行一次自动对焦
//返回值:0,成功;1,失败.
uint8_t ov5640_focus_single(struct rt_i2c_bus_device *i2c_bus);

//持续自动对焦,当失焦后,会自动继续对焦
//返回值:0,成功;其他,失败.
uint8_t ov5640_focus_constant(struct rt_i2c_bus_device *i2c_bus);

//EV曝光补偿
//exposure:0~6,代表补偿-3~3.
void ov5640_exposure(struct rt_i2c_bus_device *i2c_bus, uint8_t exposure);

//白平衡设置
//0:自动
//1:日光sunny
//2,办公室office
//3,阴天cloudy
//4,家里home
void ov5640_light_mode(struct rt_i2c_bus_device *i2c_bus, uint8_t mode);

//色度设置
//sat:0~6,代表饱和度-3~3.
void ov5640_color_saturation(struct rt_i2c_bus_device *i2c_bus, uint8_t sat);

//亮度设置
//bright:0~8,代表亮度-4~4.
void ov5640_brightness(struct rt_i2c_bus_device *i2c_bus, uint8_t bright);

//对比度设置
//contrast:0~6,代表亮度-3~3.
void ov5640_contrast(struct rt_i2c_bus_device *i2c_bus, uint8_t contrast);

//锐度设置
//sharp:0~33,0,关闭;33,auto;其他值,锐度范围.
void ov5640_sharpness(struct rt_i2c_bus_device *i2c_bus, uint8_t sharp);

//特效设置
//0:正常
//1,冷色
//2,暖色
//3,黑白
//4,偏黄
//5,反色
//6,偏绿
void ov5640_special_effects(struct rt_i2c_bus_device *i2c_bus, uint8_t eft);

//设置图像输出大小
//OV5640输出图像的大小(分辨率),完全由该函数确定
//offx,offy,为输出图像在OV5640_ImageWin_Set设定窗口(假设长宽为xsize和ysize)上的偏移
//由于开启了scale功能,用于输出的图像窗口为:xsize-2*offx,ysize-2*offy
//width,height:实际输出图像的宽度和高度
//实际输出(width,height),是在xsize-2*offx,ysize-2*offy的基础上进行缩放处理.
//一般设置offx和offy的值为16和4,更小也是可以,不过默认是16和4
//返回值:0,设置成功
//    其他,设置失败
uint8_t ov5640_outsize_set(struct rt_i2c_bus_device *i2c_bus, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);

//设置图像开窗大小(ISP大小),非必要,一般无需调用此函数
//在整个传感器上面开窗(最大2592*1944),用于OV5640_OutSize_Set的输出
//注意:本函数的宽度和高度,必须大于等于OV5640_OutSize_Set函数的宽度和高度
//     OV5640_OutSize_Set设置的宽度和高度,根据本函数设置的宽度和高度,由DSP
//     自动计算缩放比例,输出给外部设备.
//width,height:宽度(对应:horizontal)和高度(对应:vertical)
//返回值:0,设置成功
//    其他,设置失败
uint8_t ov5640_imagewin_set(struct rt_i2c_bus_device *i2c_bus, uint16_t offx, uint16_t offy, uint16_t width, uint16_t height);

#endif
