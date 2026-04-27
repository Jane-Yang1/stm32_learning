/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-19
 * @brief       按键输入 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 STM32开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/BEEP/beep.h"
#include "./BSP/KEY/key.h"

/* 模式定义 */
typedef enum
{
    MODE_NORMAL = 0,    /* 正常跑马灯模式 */
    MODE_SYNC,          /* 同步闪烁模式 (KEY_UP) */
    MODE_PRIORITY0,     /* LED0闪两次、LED1闪一次 (KEY0) */
    MODE_PRIORITY1      /* LED1闪两次、LED0闪一次 (KEY1) */
} Mode_t;

/* 全局变量 */
static Mode_t current_mode = MODE_NORMAL;		/* 当前身份，记录现在正在跑哪种灯效 */
static Mode_t next_mode = MODE_NORMAL;			/* 目标身份。按下按键，程序不会马上切换，而是先记下“想去 MODE_SYNC”，等当前的“灭灯+蜂鸣”特效做完，才真正赋值给 current_mode */
static uint32_t last_tick = 0;							/*时间锚点，这是非阻塞的核心。它记录了上一次动作发生时的系统时间，通过对比“现在时间”和“锚点时间”，判断是否该做下一个动作，而不需要让程序停下来等 */
static uint8_t sub_state = 0;          			/* 子状态：0=灭灯等待，1=蜂鸣等待，2=闪烁运行（不在切换流程里） */
static uint8_t blink_phase = 0;        			/* 闪烁相位 动作节拍，用于复杂的闪烁（如闪两下）。记录当前是“亮、灭、亮、灭”这四步里的哪一步*/

/* 函数声明 */
static void enter_new_mode(Mode_t mode);
static void run_normal_mode(void);
static void run_sync_mode(void);
static void run_priority0_mode(void);
static void run_priority1_mode(void);
static void process_switch_sequence(void);

/* 直接调 HAL_GetTick() 也行，但封装成 get_tick() 增加了代码的可移植性 */
static uint32_t get_tick(void)
{
    return HAL_GetTick();
}

/* 这个函数只负责“发起”切换，不负责“执行”切换。它设置好路标（变量），然后立刻返回，让主循环去处理具体的延时，这样不会卡死程序 */
static void enter_new_mode(Mode_t mode)
{
    next_mode = mode;
    sub_state = 0;                     /* 开始灭灯阶段 */
    LED0(1); LED1(1);                  /* 关闭所有LED */
    last_tick = get_tick();
}

/* 通过快速切换执行不同状态机的片段，可以模拟出多个任务并发执行的效果，非阻塞，系统极其灵敏
	 处理切换序列（灭灯300ms -> 蜂鸣100ms -> 真正进入新模式）*/
static void process_switch_sequence(void)
{
    if (sub_state == 0)   /* 灭灯300ms阶段 */
    {
				/* 非阻塞判断：如果当前时间 - 上次记录时间 >= 300ms */
        if (get_tick() - last_tick >= 300)
        {
            BEEP(1);                     /* 开启蜂鸣器 */
            last_tick = get_tick();			 /* 更新计时起点 */
				    sub_state = 1;							 /* 进度条进入下一阶段：蜂鸣 */
        }
    }
    else if (sub_state == 1)  /* 蜂鸣100ms阶段 */
    {
        if (get_tick() - last_tick >= 100)
        {
            BEEP(0);                     /* 关闭蜂鸣器 */
            current_mode = next_mode;    /* 关键点，正式切换模式 */
            sub_state = 2;               /* 进入运行状态 */
            /* 根据新模式初始化闪烁参数，确保新模式变量归零 */
            if (current_mode == MODE_NORMAL)
            {
                /* 跑马灯模式不需要额外初始化 */
            }
            else if (current_mode == MODE_SYNC)
            {
                /* 同步闪烁：初始让两个LED都亮 */
                LED0(0); LED1(0);
                last_tick = get_tick();
                blink_phase = 0;
            }
            else if (current_mode == MODE_PRIORITY0)
            {
                /* LED0闪两次、LED1闪一次 */
                blink_phase = 0;
                last_tick = get_tick();
                LED0(1); LED1(1);  /* 初始全灭 */
            }
            else if (current_mode == MODE_PRIORITY1)
            {
                blink_phase = 0;
                last_tick = get_tick();
                LED0(1); LED1(1);
            }
        }
    }
}

/* 正常跑马灯模式（类似实验一）*/
static void run_normal_mode(void)
{
    static uint32_t last_flip = 0;
    static uint8_t step = 0;   /* 0: LED0亮LED1灭, 1: LED0灭LED1亮 */
    uint32_t now = get_tick();

    if (now - last_flip >= 500)
    {
        if (step == 0)
        {
            LED0(0); LED1(1);
        }
        else
        {
            LED0(1); LED1(0);
        }
        step = !step;
        last_flip = now;
    }
}

/* 同步闪烁模式：两个LED同时闪烁，间隔500ms */
static void run_sync_mode(void)
{
    static uint32_t last_toggle = 0;
    static uint8_t state = 0;
    uint32_t now = get_tick();

    if (now - last_toggle >= 500)
    {
        if (state == 0)
        {
            LED0(0); LED1(0);
        }
        else
        {
            LED0(1); LED1(1);
        }
        state = !state;
        last_toggle = now;
    }
}

/* 优先级0模式：LED0闪两次，LED1闪一次，交替，间隔500ms */
static void run_priority0_mode(void)
{
    static uint32_t last_action = 0;
    uint32_t now = get_tick();

    if (now - last_action >= 500)
    {
        switch (blink_phase)
        {
            case 0: /* LED0第一次亮 */
                LED0(0); LED1(1);
                blink_phase = 1;
                break;
            case 1: /* LED0第一次灭 */
                LED0(1);
                blink_phase = 2;
                break;
            case 2: /* LED0第二次亮 */
                LED0(0);
                blink_phase = 3;
                break;
            case 3: /* LED0第二次灭 */
                LED0(1);
                blink_phase = 4;
                break;
            case 4: /* LED1亮 */
                LED1(0);
                blink_phase = 5;
                break;
            case 5: /* LED1灭 */
                LED1(1);
                blink_phase = 0;
                break;
        }
        last_action = now;
    }
}

/* 优先级1模式：LED1闪两次，LED0闪一次，交替，间隔500ms */
static void run_priority1_mode(void)
{
    static uint32_t last_action = 0;
    uint32_t now = get_tick();

    if (now - last_action >= 500)
    {
        switch (blink_phase)
        {
            case 0: /* LED1第一次亮 */
                LED0(1); LED1(0);
                blink_phase = 1;
                break;
            case 1: /* LED1第一次灭 */
                LED1(1);
                blink_phase = 2;
                break;
            case 2: /* LED1第二次亮 */
                LED1(0);
                blink_phase = 3;
                break;
            case 3: /* LED1第二次灭 */
                LED1(1);
                blink_phase = 4;
                break;
            case 4: /* LED0亮 */
                LED0(0);
                blink_phase = 5;
                break;
            case 5: /* LED0灭 */
                LED0(1);
                blink_phase = 0;
                break;
        }
        last_action = now;
    }
}

int main(void)
{
    HAL_Init();
    sys_stm32_clock_init(RCC_PLL_MUL9);
    delay_init(72);
    led_init();
    beep_init();
    key_init();

    current_mode = MODE_NORMAL;
    sub_state = 2;   /* 直接运行，不经过切换序列 */

    while (1)
    {
        /* 检测按键，优先级由高到低：WK_UP > KEY0 > KEY1 （同时按下时优先响应高优先级）。任意按键均可立即打断当前模式并执行切换提示 */
        uint8_t key = key_scan(0);
        if (key != 0)
        {
            if (key == WKUP_PRES)
            {
                if (current_mode != MODE_SYNC)
                    enter_new_mode(MODE_SYNC);
            }
            else if (key == KEY0_PRES)
            {
                if (current_mode != MODE_PRIORITY0)
                    enter_new_mode(MODE_PRIORITY0);
            }
            else if (key == KEY1_PRES)
            {
                if (current_mode != MODE_PRIORITY1)
                    enter_new_mode(MODE_PRIORITY1);
            }
        }

        /* 如果正在切换序列中，执行切换过程 （高优先级任务）*/
        if (sub_state != 2)
        {
            process_switch_sequence();
            continue;										/* 关键点，直接跳过下面的模式运行，优先处理切换 */
        }

        /* 根据当前模式运行对应的闪烁逻辑 （低优先级任务）*/
        switch (current_mode)
        {
            case MODE_NORMAL:
                run_normal_mode();
                break;							/* 等待函数run_normal_mode()内部全部执行完后，才执行到这里 */
            case MODE_SYNC:
                run_sync_mode();
                break;
            case MODE_PRIORITY0:
                run_priority0_mode();
                break;
            case MODE_PRIORITY1:
                run_priority1_mode();
                break;
            default:
                break;
        }
    }
}
