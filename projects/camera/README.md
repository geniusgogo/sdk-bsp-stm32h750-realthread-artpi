# ART-Pi上开发OV5640摄像头

## 硬件平台

- ART-Pi主板

- ART-Pi媒体扩展板

- 原子哥的RGB 800x480 4.3寸LCD屏
- 原子哥的OV5640摄像头

## 软件平台

- MDK5
- ENV（RT-Thread命令行环境）
- 源码包(修改版github：https://github.com/geniusgogo/sdk-bsp-stm32h750-realthread-artpi.git)

## 所需知识储备

- RT-Thread操作系统开发
- RT-Thread设备驱动框架
- STM32H750芯片、DCMI外设、LTDC外设

## 代码分析

### DCMI驱动

> DCMI是STM32H750芯片上用于连接摄像头的接口外设。其信号线主要有VSYNC（帧同步信号）、HSYNC/HREF（行同步信号）、PCLK(像素时钟)、D0~Dn(数据线).
在通讯过程中STM32H750的DCMI充当从机被动接收各种信号。
在与OV5640进行连接时，还需要额外的控制引脚来控制摄像头的复位等。
因为DCMI速度非常高，所以**DCMI通常与DMA搭配使用**。
在开发过程中，ART-Pi源码包里自带了一份DCMI的驱动(`drv_dcmi.c`)，相比在我的github中的代码也是基于此修改而来。
修改的主要是：
> 1. 同步方式采用硬件信号同步
> 2. 修改像素时钟极性、行同步信号、帧同步信号的极性
> 3. 采用RGB565模式，所以关闭JPEG模式
> 
OV5640除了与MCU的DCMI接口相连，还有必要的命令通讯接口SCCB（变种的I2C接口），由H750控制OV5640的寄存器配置。

### LTDC驱动

> LTDC是STM32H750芯片上用于连接LCD-TFT屏的接口外设。支持常用的RGB888、RGB565等数据格式。类似于DCMI的信号线，LTDC也有相似的信号线：行同步、帧同步、时钟、数据（RGB并口）等。但是LTDC在通讯过程中充当主机的角色，用于控制外面的LCD。所以与DCMI恰好反过来。
> 本例中使用的LCD还具备触摸功能，并且也是通过I2C接口将触摸芯片与MCU连接起来。
> 在开发过程中，ART-Pi源码包里自带一份LTDC(`drv_lcd.c`)驱动，相比在我的github中代码做了如下修改：
>
> 1. 修改分辨率（宽，高）
> 2. 像素格式RGB565，16位
> 

### OV5640驱动

> OV5640驱动主要是实现上电、复位时序，寄存器配置功能。这份驱动参考原子哥的demo代码，porting过来的。在`drv_ov5640.c`中按照RT-Thread的设备驱动框架封装并注册到系统。在应用中只要通过设备查找获取出设备操作实例。
驱动讲解：
> 1. 驱动中将SCCB(i2c)、DCMI接口、DMA配置串联起来
> 2. 驱动中主要实现了init和open函数
> > init实现上电复位、OV5640的ID校验
> > open函数实现摄像头的功能配置集、DMA配置，DCMI的两个回调函数封装（DMA中断、帧中断）
> 3. 当open成功后，应用层捕捉封装的DMA中断和帧中断，然后在DMA中断中COPY出当前接收到的摄像头RGB565数据流。帧中断回调里可以做帧同步。
> 4. 由于DMA和帧同步信号是独立的，所以会存在最后一帧没有接收完的问题，需要做特殊处理。

### 应用实现

简单的在`main.c`里实现OV5640驱动里封装的DMA回调、帧同步回调，从而拿到摄像头传给DMA双buffer里的RGB565数据。再通过将其copy到LCD的framebuffer中，即可显示出来。
由于DMA的最大传输尺寸限制，所以我们需要对摄像头的一帧数据（800x480X2）做多次串联。

### 源码下载

github：https://github.com/geniusgogo/sdk-bsp-stm32h750-realthread-artpi.git
相关源码文件：
工程目录：sdk-bsp-stm32h750-realthread-artpi/projects/camera
驱动源码：sdk-bsp-stm32h750-realthread-artpi/libraries/drivers

> `drv_dcmi.c`、`drv_lcd.c`、`drv_ov5640.c`、`ov5640_func.c`