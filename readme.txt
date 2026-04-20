# 多功能按键控制LED实验 (MultiMode_LED_Control)

[![STM32](https://img.shields.io/badge/STM32-F103ZET6-blue.svg)](https://www.st.com)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ALIENTEK%20Elite-orange)](https://www.alientek.com)

> 一个基于正点原子精英板例程的嵌入式交互实验，整合LED、蜂鸣器、按键三大模块，通过状态机实现多种LED闪烁模式的实时切换，并采用非阻塞延时保证按键响应零延迟。

## 项目简介

本实验是学习STM32 GPIO输入/输出、定时器非阻塞延时、状态机设计及按键消抖的综合实践。实现了上电跑马灯、按键切换不同闪烁模式、蜂鸣提示等功能，代码结构清晰，便于扩展。

**技术亮点**：
- 使用 `HAL_GetTick()` 实现非阻塞延时，LED闪烁期间按键依然灵敏响应
- 基于状态机的模式切换框架，新增模式只需添加枚举和运行函数
- 模块化分层设计（BSP层 / 系统层 / 应用层），符合嵌入式工程规范
- 完善的按键扫描函数，支持软件消抖和连续/单次触发模式选择

##  功能特性

| 模式 | 触发方式 | 行为描述 |
|------|----------|----------|
| 跑马灯 | 上电默认 | LED0与LED1交替闪烁，间隔500ms |
| 同步闪烁 | 按下 `KEY_UP` | 两灯同时熄灭300ms → 蜂鸣100ms → 两灯同步闪烁（500ms间隔） |
| LED0优先 | 按下 `KEY0` | 熄灭+蜂鸣提示 → LED0闪两次、LED1闪一次，循环（500ms间隔） |
| LED1优先 | 按下 `KEY1` | 熄灭+蜂鸣提示 → LED1闪两次、LED0闪一次，循环（500ms间隔） |

> **按键优先级**：`KEY_UP` > `KEY0` > `KEY1`（同时按下时优先响应高优先级）。任意按键均可立即打断当前模式并执行切换提示。

##  硬件环境

| 组件 | 引脚 | 说明 |
|------|------|------|
| LED0 | PB5 | 低电平点亮 |
| LED1 | PE5 | 低电平点亮 |
| 蜂鸣器 | PB8 | 高电平开启 |
| KEY0 | PE4 | 上拉输入，低电平有效 |
| KEY1 | PE3 | 上拉输入，低电平有效 |
| KEY_UP (WK_UP) | PA0 | 下拉输入，高电平有效 |

- 开发板：正点原子精英STM32F103ZET6
- IDE：Keil MDK 5.23 (ARMCC)
- 驱动库：STM32 HAL库

##  代码结构

Project: atk_f103
├── KEY                         # 按键相关（实际文件在 Drivers/BSP 中）
├── Startup
│   └── startup_stm32f103xe.s   # 启动文件
├── User                        # 用户应用层
│   ├── main.c                  # 主程序，状态机与模式切换逻辑
│   ├── system_stm32f1xx.c      # 系统时钟配置（CMSIS）
│   └── stm32f1xx_it.c          # 中断服务函数
├── Drivers
│   ├── SYSTEM                  # 系统层（正点原子提供）
│   │   ├── delay.c/h           # 阻塞延时（备用）
│   │   ├── sys.c/h             # 时钟初始化等
│   │   └── usart.c/h           # 串口调试（未使用）
│   ├── STM32F1xx_HAL_Driver    # STM32 HAL 库
│   │   ├── stm32f1xx_hal.c
│   │   ├── stm32f1xx_hal_cortex.c
│   │   ├── stm32f1xx_hal_dma.c
│   │   ├── stm32f1xx_hal_gpio.c
│   │   ├── stm32f1xx_hal_gpio_ex.c
│   │   ├── stm32f1xx_hal_rcc.c
│   │   ├── stm32f1xx_hal_rcc_ex.c
│   │   ├── stm32f1xx_hal_uart.c
│   │   └── stm32f1xx_hal_usart.c
│   └── BSP                     # 板级支持包
│       ├── led.c/h             # LED 驱动（初始化、亮灭控制）
│       ├── beep.c/h            # 蜂鸣器驱动（初始化、开关控制）
│       └── key.c/h             # 按键驱动（初始化、扫描、消抖）
└── Readme
    └── readme.txt              # 说明文件
	

##  使用说明

1. **克隆仓库**  
   git clone https://github.com/Jane-Yang1/MultiMode_LED_Control.git
2. 打开工程
   使用 Keil MDK 打开 `Projects/MDK-ARM` 目录下的 `.uvprojx` 文件。
3. 编译与下载
   确保已安装 STM32F1xx 器件支持包，按 F7 编译，按 F8 下载到开发板。
4. 操作
   上电后自动运行跑马灯，按下不同按键体验模式切换。

## 版本更新

· v1.0 (2026-04)
  · 初始版本，实现四种闪烁模式及按键切换
  · 采用非阻塞延时，保证实时响应
  · 添加详细的代码注释和 README

## 未来计划

· 增加串口调试输出，实时打印当前模式
· 支持长按进入“呼吸灯”模式（PWM控制）