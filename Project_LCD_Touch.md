# 1. 新增專案與硬體初始化（STM32F429ZI）

## 1.1 新增 STM32 專案

開發板名稱為 STM32F429I-DISC1，為 STMicroelectronics 官方推出的開發套件，全名為：

STM32F429 Discovery kit

---

## 1.2 低階記憶體存取函式（裸機開發基礎）

為方便直接操作 STM32 的 memory-mapped registers，可撰寫以下兩個函式，透過內嵌組語對記憶體位址進行寫入與讀取。

````c
void io_write(register uint32_t addr, register uint32_t val) {
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

此外，若要僅改寫特定位元，可使用下列「遮罩式寫入（write with mask）」函式：

````c
void io_writeMask(uint32_t addr, uint32_t data, uint32_t mask) {
	uint32_t val = io_read(addr);
	val = (val & ~mask) | (data & mask);  // 先清除 mask 位元，再寫入 data
	io_write(addr, val);
}
````

---

## 1.3 GPIO 控制教學（以 LED 為例）

### 1.3.1 原理圖解析

根據原理圖：

- PG13 控制綠色 LED（LD3）
- PG14 控制紅色 LED（LD4）

兩者皆為 LED 的陽極，陰極接至 GND，表示需透過 輸出高電位 來點亮 LED。  
因此，需控制 GPIO Port G 的 Pin13 與 Pin14。

---

### 1.3.2 GPIOG 匯流排查詢

在 STM32 架構中，各種外設模組（如 GPIO、USART、SPI、TIM 等）會掛載於不同的匯流排（Bus）上，主要的匯流排分類如下：

| 匯流排 | 掛載外設模組範例                     |
|--------|----------------------------------|
| AHB1   | GPIOA ~ GPIOK、DMA、CRC 等        |
| APB1   | USART2 ~ 5、I2C1 ~ 3、TIM2 ~ 7 等 |
| APB2   | USART1、SPI1、TIM1 等             |

若要確認 **GPIOG** 是連接在哪一條匯流排上，可以參考下列兩種方式：

---

#### 方法一：查閱 Data Sheet

打開 STM32F429 的 Data Sheet，參考：

- **Section 2.1 — Full compatibility throughout the family**
- **Figure 4 — Block diagram**

可發現 **GPIOG** 掛載於 **AHB1 匯流排（180 MHz）**。

---

#### 方法二：查閱 Reference Manual

查閱 **Reference Manual (RM0090)**，前往第 6 章 **RCC registers**，找到：

- **Section 6.3.10 — RCC_AHB1ENR (AHB1 peripheral clock enable register)**

可看到以下描述：

````c
Bit 6 GPIOGEN: IO port G clock enable  
This bit is set and cleared by software.  
0: IO port G clock disabled  
1: IO port G clock enabled
````

由此可知：**GPIOG 為 AHB1 匯流排的外設模組，啟用前需先打開其對應時脈。**

---

### 1.3.3 GPIOG 模組的時脈啟用（透過 RCC 設定）

根據前一節的查詢結果，若要啟用 **GPIOG**，必須先開啟其所屬匯流排 **AHB1** 的時脈供應。具體步驟如下：

---

#### Step 1：確認 RCC 起始位址

查閱 **STM32F429 Reference Manual** 的 **Section 2.3 — Memory Map**，可得知：

- **RCC（Reset and Clock Control）模組的起始位址為：`0x4002 3800`**

---

#### Step 2：確認 RCC_AHB1ENR 的位移量

根據 **Section 6.3.10 — RCC_AHB1ENR** 的說明：

- 控制 AHB1 匯流排外設時脈的寄存器為 `RCC_AHB1ENR`
- 其位移量（offset）為 `0x30`，因此完整記憶體位址為：

```
0x4002 3800 + 0x30 = 0x4002 3830
```

---

#### Step 3：啟用 GPIOG 的時脈

根據下列定義：

````c
Bit 6 GPIOGEN: IO port G clock enable  
This bit is set and cleared by software.  
0: IO port G clock disabled  
1: IO port G clock enabled
````

也就是說，**必須將 Bit 6 設為 1，GPIOG 才能正常運作**。否則即使寫入 MODER、ODR 等暫存器，也不會有任何效果，因為尚未供電。

---

#### 實作：AHB1 時脈啟用函式

為通用設計，我們可實作以下函式，用以設定 AHB1 匯流排任一模組的時脈啟用位元：

````c
#define RCC_BASE       0x40023800
#define RCC_AHB1ENR    0x30  // RCC_AHB1ENR offset 為 0x30

void rcc_enable_ahb1_clock(uint8_t bit_pos){
	uint32_t addr = RCC_BASE + RCC_AHB1ENR;
	uint32_t bitmask = 1U << bit_pos;
	io_writeMask(addr, bitmask, bitmask);  // 將對應位元設為 1
}
````

> 備註：若要啟用 GPIOG，應呼叫 `rcc_enable_ahb1_clock(6)`。

---

### 1.3.4 GPIO 控制 API 實作（設定模式與輸出）

完成匯流排時脈啟用後，即可透過設定 GPIO 模式（MODER）與輸出資料（ODR）來點亮 LED。

根據原理圖，我們需設定 **GPIOG_MODER** 與 **GPIOG_ODR** 這兩個 register，對 PG13、PG14 腳位進行控制。

---

#### 常數定義（建議寫於 `gpio.h` 或頂部）

````c
#define GPIOG_BASE           0x40021800  // GPIOG 起始位址（參考 Memory Map）
#define GPIO_MODER_OFFSET    0x00        // 模式控制暫存器
#define GPIO_ODR_OFFSET      0x14        // 輸出資料暫存器
````

---

#### GPIO 控制 API 實作

````c
// 設定 GPIO 腳位的模式（輸入、輸出、其他功能）
void gpio_set_mode(uint32_t port_base, uint8_t pin, uint8_t mode) {
    uint32_t reg_addr = port_base + GPIO_MODER_OFFSET;
    uint32_t shift = (pin & 0x0F) * 2;
    uint32_t data  = ((uint32_t)mode & 0x03U) << shift;
    uint32_t mask  = 0x03U << shift;

    // 說明：將對應腳位的 2-bit 模式欄位設為 mode（00=input, 01=output...）
    io_writeMask(reg_addr, data, mask);
}

// 設定 GPIO 腳位輸出值（1=高電位，0=低電位）
void gpio_set_outdata(uint32_t port_base, uint8_t pin, uint8_t val) {
    uint32_t reg_addr = port_base + GPIO_ODR_OFFSET;
    uint32_t shift = (pin & 0x0F);
    uint32_t data  = ((uint32_t)val & 0x01U) << shift;
    uint32_t mask  = 0x01U << shift;

    // 說明：僅改寫目標腳位的輸出狀態，其他腳位不受影響
    io_writeMask(reg_addr, data, mask);
}
````

---

#### 呼叫範例：讓 PG13 LED 亮起

````c
rcc_enable_ahb1_clock(6);                          // 開啟 GPIOG 時脈
gpio_set_mode(GPIOG_BASE, 13, 1);                  // 設定 PG13 為一般輸出模式
gpio_set_outdata(GPIOG_BASE, 13, 1);               // 將 PG13 輸出設為高電位（點亮 LED）
````

---

# 2. 使用 ST-LINK 與 Makefile 自行處理編譯與燒錄流程

本章記錄如何**完全不依賴 IDE**，自行透過命令列與 Makefile 編譯並燒錄 STM32 韌體至 MCU，過程中使用 STM32 ST-LINK Utility 進行 bin 檔燒錄。

---

## 2.1 手動轉換 IDE 產出的 ELF 為 BIN 並燒錄

首先利用 IDE 建構出的 `.elf` 檔，透過指令將其轉為 `.bin`，並用 ST-LINK Utility 燒錄：

```cmd
arm-none-eabi-objcopy -O binary Debug\2025_LCD_Touch.elf Debug\2025_LCD_Touch.bin
```

完成後，使用 **STM32 ST-LINK Utility** 將 `Debug\2025_LCD_Touch.bin` 燒錄進 MCU。

---

## 2.2 手動編譯原始碼為 ELF，再轉 BIN 並燒錄

完全不透過 IDE，直接使用命令列工具編譯原始碼為 `.elf` 檔，再轉為 `.bin` 進行燒錄。

### 編譯指令

```cmd
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb \
-T STM32F429ZITX_FLASH.ld \
Startup/startup_stm32f429zitx.s \
Src/main.c Src/mem_io.c Src/gpio.c Src/rcc.c \
Src\syscalls.c Src\sysmem.c \
-IInc -o 2025_LCD_Touch.elf
```

### 編譯指令各區塊說明

| 區塊                                         | 說明                                                             |
|--------------------------------------------|------------------------------------------------------------------|
| `arm-none-eabi-gcc`                         | ARM 的交叉編譯器，專為 Cortex-M 系列設計                          |
| `-mcpu=cortex-m4`                           | 指定目標處理器為 ARM Cortex-M4（STM32F429 屬於此架構）           |
| `-mthumb`                                   | 使用 Thumb 指令集（精簡指令，適用於 Cortex-M 系列）              |
| `-T STM32F429ZITX_FLASH.ld`                | 指定 linker script，定義記憶體配置與段區布局                       |
| `Startup/startup_stm32f429zitx.s`           | 啟動碼與中斷向量表組語檔案                                       |
| `Src/*.c`                                   | 所有主要的 C 原始碼檔案                                          |
| `-IInc`                                     | 指定標頭檔搜尋路徑（包含 `gpio.h`、`rcc.h` 等）                    |
| `-o 2025_LCD_Touch.elf`                     | 輸出執行檔，後續可進行轉換或 debug                               |

### ELF 轉為 BIN

```cmd
arm-none-eabi-objcopy -O binary 2025_LCD_Touch.elf 2025_LCD_Touch.bin
```

### 燒錄方式

透過 **STM32 ST-LINK Utility**，將 `2025_LCD_Touch.bin` 檔燒錄至 STM32 MCU。

---

## 2.3 使用 Makefile 一鍵編譯與產出燒錄檔

撰寫 `Makefile` 自動化整個流程，包括 `.elf` 編譯、`.bin` 轉換與 `.list` 反組譯輸出：

```makefile
CROSS_COMPILE := arm-none-eabi

CC := $(CROSS_COMPILE)-gcc
AS := $(CROSS_COMPILE)-as
OBJCOPY := $(CROSS_COMPILE)-objcopy
OBJDUMP := $(CROSS_COMPILE)-objdump

FLASHLD := STM32F429ZITX_FLASH.ld
TARGET = Debug\\2025_LCD_Touch
C_SOURCE := Src/main.c Src/mem_io.c Src/gpio.c Src/rcc.c Src\syscalls.c Src\sysmem.c
AS_SOURCE := Startup/startup_stm32f429zitx.s
INCLUDE := Inc

CFLAGS = -I$(INCLUDE) -mcpu=cortex-m4 -mthumb \
	 -Wl,-T$(FLASHLD) 

all: $(TARGET).bin

$(TARGET).bin: $(AS_SOURCE) $(C_SOURCE)
	$(CC) $(CFLAGS) $^ -o $(TARGET).elf
	$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin
	$(OBJDUMP) -S -x -D $(TARGET).elf > $(TARGET).list

clean:
	del $(TARGET).elf $(TARGET).bin $(TARGET).list $(TARGET).map
```

執行：

```cmd
make
```

產生出 `.bin` 燒錄檔後，使用 **STM32 ST-LINK Utility** 完成燒錄。

---

# 3. UART 傳輸與 printf 功能實作

### 3.1 ST-LINK 簡介與角色

因為 STM32 MCU 沒有原生 USB-to-PC 的通訊能力（或不方便實作 USB Stack），而 PC 通常也沒有 UART 腳位，所以需要透過 ST-LINK 模組作為橋樑。

STM32F103C8T6（U2）這顆 IC 是板上內建的 ST-LINK 模組控制器，扮演「USB ↔ SWD / UART」的轉換器角色。它將 PC 的 USB 訊號轉換為：

- **SWD（燒錄與除錯）**
- **UART（虛擬 COM 傳輸）**

讓 STM32F429ZI 能夠透過 USB 與 PC 溝通、下載程式、以及輸出 `printf()` 訊息。

---

### 3.1.1 SWD 功能：燒錄與 Debug

ST-LINK 可透過 SWD（Serial Wire Debug）介面控制 STM32F429ZI 進行燒錄與除錯。

SWD 是由 ARM Cortex-M MCU 內建的除錯通訊協定，只需要兩條線即可完成燒錄與 Debug：

- **SWDIO（Serial Wire Data I/O）**：雙向資料線
- **SWCLK（Serial Wire Clock）**：時脈線

根據原理圖，可以確認以下連線方式：

| STM32F429ZI 腳位 | 網路名稱    | ST-LINK (STM32F103) 腳位 |
|------------------|-------------|---------------------------|
| PA13             | STM_JTMS    | PB14（T_SWDIO）           |
| PA14             | STM_JTCK    | PB13（T_SWCLK）           |

這些是透過「Net Label（訊號名稱）」方式跨模組連線，實現 ST-LINK 與 F429 之間的 SWD 通訊。

---

### 3.1.2 UART 功能：虛擬 COM Port 傳輸

除了 SWD，ST-LINK 也模擬 USB-to-UART 功能，讓 PC 能以虛擬 COM Port 的方式接收 `printf()` 輸出。

根據原理圖，UART 的對應關係如下：

| 功能        | STM32F429ZI 腳位         | ST-LINK（F103）腳位       | 備註               |
|-------------|---------------------------|----------------------------|--------------------|
| TX（輸出）   | PA9（USART1_TX）          | PA3（ST-LINK RX）          | 資料傳送至電腦     |
| RX（輸入）   | PA10（USART1_RX）         | PA2（ST-LINK TX）          | 從電腦接收資料（可選） |

> 通常只需使用 TX 即可完成 `printf()` 輸出，RX 可視情況選用（例如接收指令、互動等）。

### 3.2 USART 模組簡介與 GPIO 設定

USART（Universal Synchronous/Asynchronous Receiver Transmitter）是 STM32 提供的串列通訊模組，支援同步與非同步傳輸模式。在一般應用中，多使用非同步模式（UART），搭配終端機進行資料輸出與接收。

根據 3.1 章節說明，為了讓 PC 能與 MCU 進行 UART 資料溝通，我們利用 ST-LINK 的 USB-to-UART 功能進行橋接。STM32F429ZI 上的 **USART1** 使用下列腳位完成通訊：

- **PA9**：USART1_TX（輸出，傳送至 PC）
- **PA10**：USART1_RX（輸入，從 PC 接收）

在實際使用 USART1 前，需完成以下兩步：

1. 啟用 USART1 模組的 RCC 時脈
2. 設定 GPIO 腳位為 Alternate Function 模式，並選定正確的 AF 編號

---

#### 3.2.1 啟用 RCC 時脈（USART1）

首先需查明 USART1 掛載在哪條匯流排上，以便正確設定 RCC（Reset and Clock Control）模組：

- **USART1 掛載於 APB2 匯流排**
- 啟用時需設定 `RCC_APB2ENR` 寄存器的 Bit 4（USART1EN）

這表示在 RCC 中的 APB2 時脈使能暫存器中，**將 Bit 4 設為 1** 即可開啟 USART1 模組的時脈供應。

---

#### 3.2.2 GPIO 腳位設定為 Alternate Function

USART1 使用 PA9 / PA10 腳位，這兩個腳位預設為 GPIO 模式，必須切換至 Alternate Function 模式才能運作：

- **模式設定**：將 PA9、PA10 切換為 Alternate Function 模式（MODER = 10）
- **功能對應設定**：指定 AF 編號為 **AF7**（對應 USART1）

這代表需設定：

| 腳位 | 功能     | 模式          | AF 編號 |
|------|----------|---------------|---------|
| PA9  | USART1_TX | Alternate Function | AF7     |
| PA10 | USART1_RX | Alternate Function | AF7     |

設定步驟包含：

1. 修改 `MODER` 暫存器，將目標腳位設定為 AF 模式
2. 修改 `AFRH` 暫存器，將腳位對應的 AF 設定為 7（USART1 專用）

---

### 3.3 USART 暫存器初始化（9600 Baud 為例）

* 設定 baud rate、啟用 TX（暫不使用 RX）
* 設定 CR1、BRR 等暫存器值

---

### 3.5 UART 傳送字元函式與簡易字串輸出

* 實作 `uart_write()` 傳送單一字元
* 實作 `uart_print()` 傳送字串

---

### 3.6 printf 函式重定向與整合

* 實作簡易 `my_printf()` 函式（支援 `%s`, `%d` 等格式）
* 或搭配 `syscalls.c` 重定向 `_write` 函式
* 讓 `printf("hello %d", 123);` 能透過 UART 輸出

---