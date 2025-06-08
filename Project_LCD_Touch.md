# 新增專案與硬體初始化（STM32F429ZI）

## 1. 新增 STM32 專案

開發板名稱為 STM32F429I-DISC1，為 STMicroelectronics 官方推出的開發套件，全名為：

STM32F429 Discovery kit

---

## 2. 組語式記憶體存取工具函式（裸機存取）

為方便直接操作 STM32 的 memory-mapped registers，可撰寫以下兩個函式，透過內嵌組語對記憶體位址進行寫入與讀取。

````c
void io_write(register uint32_t addr, register uint32_t val) {
    /**
     * "r" 表示 val 會放入通用暫存器，
     * "Qo" 表示目標記憶體位置為 ARM 架構支援的具體位址。
     */
    asm volatile("str %1, %0"
            : : "Qo" (*(volatile uint32_t *) addr), "r" (val));
}

uint32_t io_read(register uint32_t addr) {
	uint32_t val;
    asm volatile("ldr %0, %1"
            : "=r" (val)
            : "Qo" (*(volatile uint32_t *) addr));
    return val;
}
````

---

## 3. GPIO 初始化背景與需求分析

### 3.1 原理圖解析

根據原理圖：

- PG13 控制綠色 LED（LD3）
- PG14 控制紅色 LED（LD4）

兩者皆為 LED 的陽極，陰極接至 GND，表示需透過 輸出高電位 來點亮 LED。  
因此，需控制 GPIO Port G 的 Pin13 與 Pin14。

---

### 3.2 GPIOG 所屬匯流排查找

在 STM32 架構中，GPIO、USART、SPI、TIM 等外設模組皆掛載於不同的匯流排（Bus）上，常見如：

| 匯流排 | 掛載外設模組 |
|--------|----------------|
| AHB1   | GPIO、DMA、CRC 等 |
| APB1   | USART2~5、I2C1~3、TIM2~7 |
| APB2   | USART1、SPI1、TIM1 等 |

要查 GPIOG 屬於哪一條匯流排，可參考以下兩種資料來源：

方法一：查閱 Data Sheet

打開 STM32F429 Data Sheet，在：

- Section 2.1 – Full compatibility throughout the family
- Figure 4 – Block diagram

中可以看到 GPIOG 接在 AHB1（180 MHz）匯流排上。

方法二：查閱 Reference Manual

打開 Reference Manual (RM0090)，前往6.3RCC registers，並在6.3.10 – AHB1 peripheral clock enable register (RCC_AHB1ENR)看到
Bit 6 GPIOGEN: IO port G clock enable
This bit is set and cleared by software.
0: IO port G clock disabled
1: IO port G clock enabled
也就可以確定GPIOG 接在 AHB1上

---

## 4. 啟用 GPIOG 的時脈

由上一步得知想開啟GPIOG 的時脈，就必須先確定AHB1 bus的RCC起始位址，從2.3 Memory map，可以看到RCC起始位址為0x4002 3800

接著從6.3.10 – AHB1 peripheral clock enable register (RCC_AHB1ENR)看到想要控制 RCC_AHB1ENR 暫存器中必須先對RCC位址做一個0x30的Address offset

且從下面描述，要控制 GPIOG，必須先將 Bit 6 設為 1，以啟用其時脈供應，否則對 GPIOG 的任何設定都將無效，因為模組電路尚未被時脈驅動。


````c
CJ
````

---

若你已完成上述步驟，接下來就可以開始設定 GPIOG_MODER（模式）與 GPIOG_ODR（輸出資料）來讓 LED 亮起。




arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -T STM32F429ZITX_FLASH.ld Startup/startup_stm32f429zitx.s Src/main.c Src/mem_io.c Src/gpio.c Src/rcc.c Src\syscalls.c Src\sysmem.c -IInc -o 2025_LCD_Touch.elf



arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -TC:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\STM32F429ZITX_FLASH.ld C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Startup\startup_stm32f429zitx.s C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\main.c C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\mem_io.c C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\gpio.c C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\rcc.c C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\syscalls.c C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Src\sysmem.c -IC:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Inc -o C:\Users\Xavier\Desktop\mine\SourceTree\interview_2025\2025_LCD_Touch\Debug\2025_LCD_Touch.elf


arm-none-eabi-objcopy -O binary Debug\2025_LCD_Touch.elf Debug\2025_LCD_Touch.bin




  | 區塊                                | 作用                                                             |
| --------------------------------- | -------------------------------------------------------------- |
| `arm-none-eabi-gcc`               | 使用 ARM 的交叉編譯器，為 STM32 編譯用                                      |
| `-mcpu=cortex-m4`                 | 告訴編譯器目標處理器是 ARM Cortex-M4（STM32F429 就是 M4）                     |
| `-mthumb`                         | 使用 Thumb 指令集（Cortex-M 系列用的精簡指令集）                               |
| `-nostartfiles`                   | 不連結 C runtime 的啟動檔（如 crt0.o）——因為我們自己提供 `startup_stm32f429zitx.s` |
| `-nostdlib`                       | 不連結標準 C 函式庫（如 printf），用於裸機系統                                   |
| `-T STM32F429ZITX_FLASH.ld`            | 指定 Linker script，定義記憶體位置、段區 `.text` `.bss` 等                   |
| `Startup/startup_stm32f429zitx.s`   | 提供啟動碼與中斷向量表的組語檔                                                |
| `Src/main.c Src/gpio.c Src/rcc.c` | 你主要的 C 原始碼檔案                                                   |
| `-IInc`                           | 指定標頭檔搜尋目錄（讓 `#include "gpio.h"` 等能找到）                          |
| `-o 2025_LCD_Touch.elf`           | 輸出的檔案名：`.elf` 執行檔（可拿去 objcopy 或 debug）                         |
