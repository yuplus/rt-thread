# FRDM-MCXA366 板载 SLCD 设计文档

## 1. 概述

本文档描述 RT-Thread BSP `frdm-mcxa366` 中板载段码液晶（SLCD）的软件架构、硬件映射、API 与使用方法。

| 项目 | 说明 |
|------|------|
| 硬件 | FRDM-MCXA366 板载 6 位段码 LCD |
| 控制器 | MCXA366 片内 SLCD0（`LCD0`） |
| 占空比 | 1/4 Duty（4 个背板 COM） |
| 参考 | NXP AN14860、MCUXpresso SDK `driver_examples/slcd` |

## 2. 设计目标

- 在 RT-Thread 上驱动板载段码屏，支持数字、冒号、小数点显示。
- 板级逻辑与 NXP SDK 外设驱动分层：通用寄存器操作走 `fsl_slcd`，段码映射与业务 API 留在板级。
- 提供 MSH 例程，便于快速验证。
- 通过 Kconfig `BSP_USING_SLCD` 控制编译开关。

## 3. 软件架构

```text
┌─────────────────────────────────────────┐
│  applications/slcd_sample.c             │  MSH / 启动演示
├─────────────────────────────────────────┤
│  board/ports/drv_slcd.c/.h              │  板级：段码、时间、字符串
├─────────────────────────────────────────┤
│  packages/.../drivers/fsl_slcd.c/.h     │  NXP SDK：Init / WF / Blink
├─────────────────────────────────────────┤
│  board/.../pin_mux.c  BOARD_InitSLCDPins│  PORT0_12~27 → LCD_P16~31
└─────────────────────────────────────────┘
```

### 3.1 分层职责

| 层级 | 文件 | 职责 |
|------|------|------|
| 应用 | `applications/slcd_sample.c` | 启动显示、`slcd_show` / `slcd_time` 等命令 |
| 板级驱动 | `board/ports/drv_slcd.c` | 引脚初始化调用、时钟/复位、段码表、`12:30` 解析 |
| SDK 驱动 | `fsl_slcd.c` | `SLCD_Init`、`SLCD_SetFrontPlaneSegments`、背板相位等 |
| 引脚 | `pin_mux.c` | `BOARD_InitSLCDPins()`：Alt9 配置 LCD 引脚 |

### 3.2 为何保留两套文件

- `fsl_slcd`：芯片通用外设驱动，不感知本板玻璃的 digit/COM 走线。
- `drv_slcd`：本板专用，把“显示 12:30”翻译成具体 front-plane 波形。

`drv_slcd` **调用** `fsl_slcd`，不再直接写 `LCD0->GCR` / `WF` 做初始化。

## 4. 硬件与引脚

### 4.1 引脚复用

| LCD 信号 | 端口引脚 | Mux |
|----------|----------|-----|
| LCD_P16 ~ LCD_P31 | PORT0_12 ~ PORT0_27 | `kPORT_MuxAlt9` |

由 `BOARD_InitSLCDPins()` 统一配置。

### 4.2 前板 / 背板

```text
前板 (Front Plane)  digit6 .. digit1（左 → 右）
  | P27 P26 | P25 P24 | P23 P22 | P21 P20 | P19 P18 | P17 P16 |

背板 (Back Plane)   COM / Phase
  P28 → Phase A
  P29 → Phase B
  P30 → Phase C
  P31 → Phase D
```

- 使能引脚：`PEN` 低 32 位 `0xFFFF0000`（P16~P31）
- 背板掩码：`BPEN` `0xF0000000`（P28~P31）

### 4.3 段码与小数点

每个数字占用 **两个** front-plane 引脚。段码表 16 位编码：

```text
  15..8          7..0
  E G F P        D C B A
  (高字节→pin_hi) (低字节→pin_lo)
```

小数点 / 冒号（相位 A / SEGP）：

| 符号 | 宏 | 引脚 |
|------|-----|------|
| 上点 P1 | `SLCD_P1` | LCD_P23 |
| 上点 P2 | `SLCD_P2` | LCD_P21 |
| 上点 P3 | `SLCD_P3` | LCD_P19 |
| 下点 P4 | `SLCD_P4` | LCD_P25 |
| 下点 P5 | `SLCD_P5` | LCD_P27 |
| 下点 P6 | `SLCD_P6` | LCD_P17 |

常用组合：

- `SLCD_COLON` = `P1 | P4`：digit5 与 digit4 之间的冒号（`HH:MM`）
- `SLCD_DOT` = `P6`：digit3 与 digit2 之间的小数点

### 4.4 时钟

MCXA366 上 SLCD 使用 FRO16K：

```c
CLOCK_SetupFRO16KClocking(kCLKE_16K_COREMAIN);
RESET_ReleasePeripheralReset(kSLCD0_RST_SHIFT_RSTn);
```

## 5. 初始化流程

```text
rt_hw_slcd_init()
  ├─ BOARD_InitSLCDPins()
  ├─ CLOCK_SetupFRO16KClocking()
  ├─ RESET_ReleasePeripheralReset(kSLCD0)
  ├─ SLCD_GetDefaultConfig() + 板级参数
  │    duty = 1/4
  │    slcdLowPinEnabled = 0xFFFF0000
  │    backPlaneLowPin   = 0xF0000000
  ├─ SLCD_Init(LCD0, &config)
  ├─ SLCD_SetBackPlanePhase(P28~P31, A~D)
  └─ SLCD_StartDisplay(LCD0)
```

## 6. 板级 API

头文件：`board/ports/drv_slcd.h`

| API | 说明 |
|-----|------|
| `rt_hw_slcd_init()` | 初始化（可重复调用，内部防重入） |
| `rt_hw_slcd_clear()` | 清除 P16~P27 前板波形 |
| `rt_hw_slcd_display_all()` | 全段点亮（调试用） |
| `rt_hw_slcd_display_num(digit, num)` | digit=1~6，num=0~9 |
| `rt_hw_slcd_display_string(str)` | 最多 6 位数字；支持 `:` / `.` |
| `rt_hw_slcd_display_point(mask)` | 按位点亮 P1~P6 |
| `rt_hw_slcd_display_time(hh, mm)` | 显示 `HH:MM` |

### 6.1 字符串规则

- 从左到右填充 digit6 → digit1。
- 遇到 `:` 置位 `SLCD_COLON`；遇到 `.` 置位 `SLCD_DOT`。
- 空格或非法字符显示为空白。
- 不足 6 位右侧补空。

示例：

| 输入 | 显示效果 |
|------|----------|
| `"123456"` | `123456` |
| `"12:30"` | `12:30`（右侧两位空） |
| `"00:00.00"` | 需自行组合点；秒表场景可用 `P1\|P4\|P6` |

### 6.2 代码示例

```c
#include "drv_slcd.h"

rt_hw_slcd_init();
rt_hw_slcd_display_time(12, 30);
/* 或 */
rt_hw_slcd_clear();
rt_hw_slcd_display_string("12:30");
```

## 7. 配置与编译

### 7.1 Kconfig

路径：`Hardware Drivers Config` → `Onboard Peripheral Drivers` → **Enable onboard SLCD display**

```
CONFIG_BSP_USING_SLCD=y
```

对应 `rtconfig.h`：

```c
#define BSP_USING_SLCD
```

（应用 `scons --pyconfig-silent` 或 menuconfig 从 `.config` 生成；勿长期手改 `rtconfig.h`。）

### 7.2 参与编译的文件

| 文件 | 条件 |
|------|------|
| `board/ports/drv_slcd.c` | `BSP_USING_SLCD` |
| `applications/slcd_sample.c` | `#ifdef BSP_USING_SLCD` |
| `packages/.../fsl_slcd.c` | Libraries（SConscript / Keil） |
| `pin_mux.c` 中 `BOARD_InitSLCDPins` | 始终编译，由 init 调用 |

Keil：`project.uvprojx` 已加入 `drv_slcd.c`、`slcd_sample.c`、`fsl_slcd.c`，并包含 `board/ports` 头文件路径。

## 8. 例程与 MSH

`applications/slcd_sample.c` 在 `INIT_APP_EXPORT` 中初始化，默认显示 `12:30`。

| 命令 | 作用 |
|------|------|
| `slcd_demo` | 0~9 循环 → `12:30` → 全段 → `888888` |
| `slcd_show 12:30` | 显示字符串 |
| `slcd_time 12 30` | 显示时分 |
| `slcd_clear` | 清屏 |

## 9. 关键点与扩展

1. **对比度 / 低功耗**：`FSL_FEATURE_SLCD_LP_CONTROL` 下可通过 `slcd_config_t` 的 voltage trim、sample&hold、lowPowerWaveform 调节。
2. **闪烁**：可直接调用 `SLCD_StartBlinkMode()` / `SLCD_StopBlinkMode()`。
3. **图标**：本板玻璃若还有独立 icon 段，需按原理图扩展 pin/phase 表（当前实现覆盖 6 位数字 + 冒号/小数点）。
4. **包更新**：`packages/nxp-mcx-series-latest` 更新后确认 `MCXA366/SConscript` 仍包含 `fsl_slcd.c`。

## 10. 参考资料

- NXP AN14860：Using SLCD Controller on MCX A Series  
  https://github.com/nxp-appcodehub/an-mcxa366-slcd-example
- MCUXpresso SDK SLCD example（FRDM-MCXA366）  
  https://github.com/nxp-mcuxpresso/mcuxsdk-examples/tree/main/driver_examples/slcd
- MCUXpresso SLCD Driver API  
  https://mcuxpresso.nxp.com/api_doc/dev/86/group__slcd.html
