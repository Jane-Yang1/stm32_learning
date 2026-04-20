/**
 ****************************************************************************************************
 * @file        key.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-20
 * @brief       按键输入 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32F103开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20200420
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./BSP/KEY/key.h"
#include "./SYSTEM/delay/delay.h"		/* 按键消抖要用到延时函数 */


/**
 * @brief       按键初始化函数
 * @param       无
 * @retval      无
 */
 
void key_init(void)
{
    GPIO_InitTypeDef gpio_init_struct;
    KEY0_GPIO_CLK_ENABLE();                                     /* KEY0时钟使能 */
    KEY1_GPIO_CLK_ENABLE();                                     /* KEY1时钟使能 */
    WKUP_GPIO_CLK_ENABLE();                                     /* WKUP时钟使能 */

    gpio_init_struct.Pin = KEY0_GPIO_PIN;                       /* KEY0引脚 */
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                    /* 输入 */
    gpio_init_struct.Pull = GPIO_PULLUP;                        /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;              /* 高速 */
    HAL_GPIO_Init(KEY0_GPIO_PORT, &gpio_init_struct);           /* KEY0引脚模式设置,上拉输入 */

    gpio_init_struct.Pin = KEY1_GPIO_PIN;                       /* KEY1引脚 */
    HAL_GPIO_Init(KEY1_GPIO_PORT, &gpio_init_struct);           /* KEY1引脚模式设置,上拉输入 */

    gpio_init_struct.Pin = WKUP_GPIO_PIN;                       /* WKUP引脚 */
    gpio_init_struct.Pull = GPIO_PULLDOWN;                      /* 下拉 */
    HAL_GPIO_Init(WKUP_GPIO_PORT, &gpio_init_struct);           /* WKUP引脚模式设置,下拉输入 */

}

/**
 * @brief       按键扫描函数
 * @note        该函数有响应优先级(同时按下多个按键): WK_UP > KEY1 > KEY0!!
 * @param       mode:0 / 1, 具体含义如下:
 *   @arg       0,  不支持连续按(当按键按下不放时, 只有第一次调用会返回键值,
 *                  必须松开以后, 再次按下才会返回其他键值)
 *   @arg       1,  支持连续按(当按键按下不放时, 每次调用该函数都会返回键值)
 * @retval      键值, 定义如下:
 *              KEY0_PRES, 1, KEY0按下
 *              KEY1_PRES, 2, KEY1按下
 *              WKUP_PRES, 3, WKUP按下
 */
/** 这是一段非常经典的单片机按键扫描函数,它的核心作用是检测按键状态、消除按键抖动，并支持“长按连发”功能。 
		核心设计思想是利用一个静态变量key_up来记录按键的“松/紧”状态，从而巧妙地实现了“单次触发”和“连续触发”两种模式。**/

uint8_t key_scan(uint8_t mode)	/*参数 mode 控制连按模式，返回值是按下按键的编号（0表示无按键）*/
{
    static uint8_t key_up = 1;  /* 按键按松开标志 初始化为1：所有按键松开，可以再次检测、按下；0：按键按住不放*/
    uint8_t keyval = 0;					/* 返回值，按键值为0，没有一个按键被按下 */
    if (mode) key_up = 1;       /* 支持连按 这一行代码决定了你希望按键是“按一次动一下”还是“按住一直动”*/

    if (key_up && (KEY0 == 0 || KEY1 == 0 || WK_UP == 1))  /* 按键松开标志为1, 且有任意一个按键按下了 */
    {
        delay_ms(10);           /* 去抖动 只要检测到按键状态发生变化，都进行消抖，以确保读取到的电平是稳定的。*/
        key_up = 0;

        if (KEY0 == 0)  keyval = KEY0_PRES;

        if (KEY1 == 0)  keyval = KEY1_PRES;

        if (WK_UP == 1) keyval = WKUP_PRES;
    }
    else if (KEY0 == 1 && KEY1 == 1 && WK_UP == 0) /* 没有任何按键按下, 标记按键松开 */
    {
        key_up = 1;
    }

    return keyval;              /* 返回键值 */
}
/* 	1、输入参数 mode：（功能开关：0/1）
		mode 的值不是函数自己算出来的，而是由调用这个函数的地方（通常是 main.c 中的 while(1) 循环里）决定的。程序员在写代码时，根据需求直接把 0 或 1 传进去。
		(1)mode = 0：不支持连按。按下一次只返回一次键值，必须松开后才能再次触发（防止误触）。
			效果：如果你按住按键不放，程序只会响应一次，必须松手后才能再次响应。这适合做开关灯（按一下开，再按一下关）
		(2)mode = 1：支持连按。如果一直按着不松手，函数会持续返回键值（适合做长按加速或连续调节）。按住就是一直触发。
			效果：无论你上次按键状态如何，程序都“假装”你已经松手了。这样只要你还按着，下一次循环进来依然会判定为“按下”，从而实现长按连续触发的效果（类似键盘长按删除）。
			
		2、返回值：
			返回 0：表示没有按键按下。
			返回 1、2、3 ：表示具体的按键编号。 */
/* 通过连续三个if覆盖赋值，后面的if会覆盖前面的，所以最后判断的WK_UP优先级最高。 */



















