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

## 3.1 ST-LINK 簡介與角色

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

## 3.2 USART RCC 與 GPIO 設定

USART（Universal Synchronous/Asynchronous Receiver Transmitter）是 STM32 提供的串列通訊模組，支援同步與非同步傳輸模式。在一般應用中，多使用非同步模式（UART），搭配終端機進行資料輸出與接收。

根據 3.1 章節說明，為了讓 PC 能與 MCU 進行 UART 資料溝通，我們利用 ST-LINK 的 USB-to-UART 功能進行橋接。STM32F429ZI 上的 **USART1** 使用下列腳位完成通訊：

- **PA9**：USART1_TX（輸出，傳送至 PC）
- **PA10**：USART1_RX（輸入，從 PC 接收）

在實際使用 USART1 前，需完成以下兩步：

1. 啟用 USART1 模組的 RCC 時脈
2. 設定 GPIO 腳位為 Alternate Function 模式，並選定正確的 AF 編號

---

### 3.2.1 啟用 RCC 時脈（USART1）

由於我們確定是使用 **PA9 / PA10** 作為 USART 的 TX / RX 腳位，但若原理圖中未明確標示是使用 USART1、USART2 或其他模組，此時就需要查詢這些 GPIO 腳位所支援的 USART 功能對應關係。

可參考 **《DataSheet - STM32F427xx / STM32F429xx》** 的下列位置：

- **第 4 章：Pinouts and pin description**
- **Table 12：STM32F427xx and STM32F429xx alternate function mapping**

根據表格查詢結果如下：

| 腳位  | 支援功能（部分列出）                      |
|-------|--------------------------------------------|
| PA9   | TIM1_CH2, I2C3_SMBA, **USART1_TX**, DCMI_D0 |
| PA10  | TIM1_CH3, **USART1_RX**, OTG_FS_ID, DCMI_D1 |

由此可知，PA9 / PA10 僅支援 **USART1**，並無支援 USART2 或 USART3，因此可明確判斷目前所使用的是 **USART1** 模組。

---

接著需確認 USART1 掛載於哪一匯流排，以便正確設定 RCC（Reset and Clock Control）模組時脈來源。查詢方式有兩種：

**方法一：查閱 DataSheet**

- Section 2.1 — Full compatibility throughout the family  
- Figure 4 — Block diagram  

可確認 **USART1 掛載於 APB2 匯流排**（最大頻率為 90 MHz）

**方法二：查閱 Reference Manual**

- 《Reference Manual (RM0090)》
- 第 6 章：**RCC registers**
- Section 6.3.14 — **RCC_APB2ENR (APB2 peripheral clock enable register)**

參考原文描述如下：

````c
Bit 4 USART1EN: USART1 clock enable  
This bit is set and cleared by software.  
0: USART1 clock disabled  
1: USART1 clock enabled
````

這表示將 RCC_APB2ENR 寄存器中的 Bit 4 設為 1，即可啟用 USART1 模組的時脈供應。

---

程式碼範例如下：

````c
void rcc_enable_apb1_clock(uint8_t bit_pos){
    uint32_t addr = RCC_BASE + RCC_APB1ENR;
    uint32_t bitmask = 1U << bit_pos;
    io_writeMask(addr, bitmask, bitmask);
}

void rcc_enable_apb2_clock(uint8_t bit_pos){
    uint32_t addr = RCC_BASE + RCC_APB2ENR;
    uint32_t bitmask = 1U << bit_pos;
    io_writeMask(addr, bitmask, bitmask);
}

void usart_rcc_enable(USART_Module usart_num){
    switch (usart_num){
        case RCC_USART1EN:
            rcc_enable_apb2_clock(4);
            break;
        case RCC_USART6EN:
            rcc_enable_apb2_clock(5);
            break;
        case RCC_USART2EN:
            rcc_enable_apb1_clock(17);
            break;
        case RCC_USART3EN:
            rcc_enable_apb1_clock(18);
            break;
        case RCC_UART4EN:
            rcc_enable_apb1_clock(19);
            break;
        case RCC_UART5EN:
            rcc_enable_apb1_clock(20);
            break;
        default:
            return;
    }
}
````

---

### 3.2.2 GPIO 腳位設定為 Alternate Function

USART1 使用 PA9 與 PA10 腳位，預設為一般 GPIO 模式，需切換為 Alternate Function 模式並指定編號 **AF7**（對應 USART1），才能正常使用通訊功能。

對應設定如下：

| 腳位 | 功能       | 模式               | AF 編號 |
|------|------------|--------------------|---------|
| PA9  | USART1_TX  | Alternate Function | AF7     |
| PA10 | USART1_RX  | Alternate Function | AF7     |

---

#### 設定流程：

- **設定 GPIO 模式為 Alternate Function**

    ````c
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE);
    ````

- **設定 Alternate Function 編號為 AF7**

    可參考 DataSheet 第 4 章：**Pinouts and pin description**，其中的  
    **Table 12：STM32F427xx and STM32F429xx alternate function mapping** 列出各 GPIO 腳位對應的 Alternate Function。  
    例如 PA9 / PA10 皆對應 USART1，並屬於 **AF7** 功能。

    AF 編號可使用下列列舉方式定義：

    ````c
    typedef enum {
        ALTERNATE_AF0  = 0,  // SYS
        ALTERNATE_AF1  = 1,  // TIM1 / TIM2
        ALTERNATE_AF2  = 2,  // TIM3 / TIM4 / TIM5
        ALTERNATE_AF3  = 3,  // TIM8 / TIM9 / TIM10 / TIM11
        ALTERNATE_AF4  = 4,  // I2C1 / I2C2 / I2C3
        ALTERNATE_AF5  = 5,  // SPI1 ~ SPI6
        ALTERNATE_AF6  = 6,  // SPI2 / SPI3 / SAI1
        ALTERNATE_AF7  = 7,  // SPI3 / USART1 ~ USART3
        ALTERNATE_AF8  = 8,  // USART6 / UART4 ~ UART8
        ALTERNATE_AF9  = 9,  // CAN1 / CAN2 / TIM12 ~ TIM14 / LCD
        ALTERNATE_AF10 = 10, // OTG2_HS / OTG1_FS
        ALTERNATE_AF11 = 11, // ETH
        ALTERNATE_AF12 = 12, // FMC / SDIO / OTG2_FS
        ALTERNATE_AF13 = 13, // DCMI
        ALTERNATE_AF14 = 14, // LCD
        ALTERNATE_AF15 = 15  // SYS
    } GPIO_AlternateFunction;
    ````

    設定 AFR 暫存器的函式如下：

    ````c
    void gpio_set_alternate_function(uint32_t port_base, uint8_t pin, GPIO_AlternateFunction af) {
        uint32_t reg_addr, shift, data, mask;

        if (pin <= 7) {
            reg_addr = port_base + GPIO_AFRL_OFFSET;
            shift = pin * 4;
        } else if (pin <= 15) {
            reg_addr = port_base + GPIO_AFRH_OFFSET;
            shift = (pin - 8) * 4;
        } else {
            return;
        }

        data = ((uint32_t)af & 0x0F) << shift;
        mask = 0x0F << shift;

        io_writeMask(reg_addr, data, mask);
    }
    ````

    呼叫方式範例如下：

    ````c
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_9, ALTERNATE_AF7);   // USART1_TX
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_10, ALTERNATE_AF7);  // USART1_RX
    ````

---

## 3.3 USART 暫存器初始化（9600 Baud 為例）

### 3.3.1 匯流排時脈來源與 USART 的關係

STM32 的周邊模組（如 USART、SPI、TIM）皆掛載於不同的匯流排（Bus）上，每個匯流排有各自的時脈來源，這些時脈來源與主系統時脈（HCLK）並不相同。

以 USART 為例，各模組所屬的匯流排與對應的時脈如下：

| 周邊模組   | 所屬匯流排 | 對應時脈變數     |
|------------|-------------|------------------|
| USART1 / 6 | APB2        | `fCK_APB2`       |
| USART2 / 3 | APB1        | `fCK_APB1`       |

在進行波特率設定時，`fCK` 即為匯流排的時脈頻率，作為 USART 傳輸時脈的輸入來源，而**非系統主時脈 HCLK**。

---

#### 時脈設定範例（以 STM32F4 系列為例）

- 系統主時脈 HCLK = 180 MHz  
- APB2 匯流排預分頻值 = `/2` → `fCK_APB2` = 90 MHz  
- APB1 匯流排預分頻值 = `/4` → `fCK_APB1` = 45 MHz  

因此：

- 使用 USART1 或 USART6 時，波特率計算會以 **90 MHz** 為輸入時脈  
- 使用 USART2 或 USART3 時，波特率計算會以 **45 MHz** 為輸入時脈

---

#### 在裸機開發中的注意事項

若使用 STM32CubeIDE 或 STM32CubeMX 設定系統時脈，上述匯流排分頻結果會自動產生。  
但在 **裸機開發** 環境下，必須由使用者根據 **PLL 設定與匯流排分頻因子**，自行推算 `fCK_APB1` / `fCK_APB2` 的實際時脈值，並用於 USART 波特率計算。

---

#### 依據 ST 官方文件《RM0090》的預設時脈資訊

- **7.2.2 - HSI clock：**  
  預設內部時脈 HSI 為 16 MHz。

- **7.2.6 - System clock (SYSCLK)：**  
  系統時脈（SYSCLK）可由 HSI、HSE 或 PLL 選擇，  
  MCU 上電後預設選用 **HSI** 作為時脈來源。

- **7.3.3 - RCC_CFGR register：**  
  `Bits 15:13 – PPRE2: APB high-speed prescaler (APB2)`  
  此欄位由軟體設定，用來控制 APB2 的除頻倍率：

  ```
  0xx: AHB clock 不除頻（/1）
  100: 除以 2（/2）
  101: 除以 4（/4）
  110: 除以 8（/8）
  111: 除以 16（/16）
  ```

---

#### 預設情況下 USART 的時脈來源

首先，我們可以透過程式確認目前 APB2 是否啟用了除頻功能：

````c
uint32_t cfgr = io_read(RCC_BASE + 0x08);
uint32_t ppre2 = (cfgr >> 13) & 0x7;  // PPRE2 分頻器
````

如果 `ppre2 == 0`，表示目前 **APB2 並未進行分頻**（除頻倍率為 /1）。

因此，在**未修改任何系統時脈配置**下，若使用 USART1（掛載於 APB2）傳輸，時脈來源為：

```
USART1 的時脈 fCK = fCK_APB2 = SYSCLK / PPRE2 = HSI / 1 = 16 MHz（實測預設值）
```

---

#### 預設上電時的時脈總覽

| 項目             | 預設值                       |
|------------------|------------------------------|
| 系統時脈 SYSCLK  | `HSI` = 16 MHz               |
| HCLK             | = SYSCLK = 16 MHz            |
| APB2 Prescaler   | `/1` → `fCK_APB2 = 16 MHz`    |

---

### 3.3.2 波特率計算原理與過取樣模式

在 USART 傳輸中，資料以位元（bit）為單位透過 TX 腳送出。為了讓接收端（Rx）與傳送端（Tx）能以相同的時間基準同步傳輸資料，必須設定一致的通訊速率，稱為 **波特率（Baud Rate）**。

此波特率的計算，是根據匯流排提供的時脈來源（fCK）除上一個分頻值（USARTDIV）得到的。

**波特率公式：**

根據《RM0090 Reference Manual》第 30.3.4 節 *Fractional baud rate generation*，標準 USART 模式下的波特率為：

````c
Tx/Rx baud = fCK / [8 × (2 − OVER8) × USARTDIV]
````

| 符號         | 說明                                          |
|--------------|-----------------------------------------------|
| `fCK`        | USART 模組所屬匯流排的時脈頻率（APB1 或 APB2）  |
| `OVER8`      | 過取樣設定，定義於 `USART_CR1` 暫存器的 bit 15 |
| `USARTDIV`   | 波特率分頻值，寫入 `USART_BRR` 暫存器中         |

---

**過取樣模式說明（Oversampling）：**

USART 為了精確判斷每一個資料位元的邊界與狀態，會對每個 bit 進行「多次取樣」，稱為過取樣模式。

| 模式                | 每位元取樣次數 | OVER8 值 | 精準度 | 資源消耗     | 適用場景           |
|---------------------|----------------|----------|--------|--------------|--------------------|
| 16 倍過取樣（預設） | 16 次          | 0        | 高     | 高（耗時脈） | 高速、高可靠性     |
| 8 倍過取樣          | 8 次           | 1        | 較低   | 低（省時脈） | 低速、省電、低誤差 |

→ 在 8 倍過取樣下，波特率公式中的除數會變小，因此可在較低時脈下仍維持同樣波特率，適合低功耗應用。

---

**USARTDIV 與 BRR 寄存器格式：**

`USARTDIV` 是一個「無號固定小數點數」，分為：

- **整數部分（Mantissa）**
- **小數部分（Fraction）**

該值會寫入至 `USART_BRR` 暫存器，格式如下：

| 過取樣模式    | OVER8 值 | 小數位元數 | `BRR` 寫法                             |
|---------------|-----------|-------------|----------------------------------------|
| 16 倍過取樣   | 0         | 4 bits      | `BRR = mantissa << 4 | fraction`       |
| 8 倍過取樣    | 1         | 3 bits      | `BRR = mantissa << 4 | (fraction & 0x7)` 且 bit[3] 必須為 0 |

> 注意：當你寫入 `USART_BRR` 後，硬體會立即根據該值重新計算內部時脈，**不可在正在傳輸期間修改 BRR**，否則可能導致通訊中斷或資料遺失。

---

**程式範例：**
````c
void usart_brr(uint32_t usart_base, uint32_t usart_div_x100) {
    uint32_t reg_addr = usart_base + USART_BRR_OFFSET;

    // 讀取 OVER8 設定值（bit 15）
    uint32_t cr1_over8 = (read_io(usart_base + USART_CR1_OFFSET) >> USART_CR1_OVER8) & 0x01;

    // 拆解 USARTDIV 為整數 + 小數百分比（×100）
    uint32_t mantissa = usart_div_x100 / 100;
    uint32_t fraction = usart_div_x100 - mantissa * 100;

    if (cr1_over8 == 0) {
        // (fraction / 100) * 16 = 0 → use (fraction * 16 + 50) / 100 ； 加 50 是為了四捨五入
        fraction = (fraction * 16 + 50) / 100;
    } else {
        // (fraction / 100) * 8 = 0 → use (fraction * 8 + 50) / 100
        fraction = ((fraction * 8 + 50) / 100) & 0x07;  // 只保留 3 bits，小數最高不能超過 7
    }

    // 組合 mantissa 與 fraction，寫入 BRR
    uint32_t val = (mantissa << 4) | fraction;
    io_write(reg_addr, val);
}

void usart_set_baudrate(uint32_t usart_base, uint32_t fck, uint32_t baudrate){
	uint32_t cr1_over8 = (read_io(usart_base + USART_CR1_OFFSET) >> USART_CR1_OVER8) & 0x01;

	uint32_t usart_div_x100 = (fck*100) / (baudrate * 8 * (2 - cr1_over8));

	usart_brr(usart_base, usart_div_x100);
}
````

---

### 3.3.3 資料傳送流程與 USART 傳輸器原理

根據《RM0090 Reference Manual》第 30.3.2 節 *Transmitter*，USART 傳輸器支援傳送 8-bit 或 9-bit 資料，長度由 `USART_CR1` 中的 **M 位元** 所設定。

當啟用 `TE`（Transmit Enable）= 1 時，傳輸器開始運作，將資料從「傳輸移位暫存器（Transmit Shift Register）」輸出至 TX 腳位（同步模式下同時輸出時脈至 CK 腳位）。資料以 **最低有效位元（LSB）先出** 的順序傳送。

> 注意：**TE 不可在傳輸進行中關閉**（即不可將 TE 清為 0），否則會導致傳輸中斷。

---

**資料傳送結構：**

- `USART_DR` 實際對應的資料暫存器為 **TDR（Transmit Data Register）**
- 當 CPU 將資料寫入 `USART_DR` 時，資料會被送至傳輸移位暫存器
- 傳輸格式：每個字元前有 1 個 **Start Bit（邏輯低）**，結尾有 1 或多個 **Stop Bit（邏輯高）**
- 支援 Stop Bit 長度設定為：0.5 / 1 / 1.5 / 2 bits
- 啟用 TE 後，USART 會自動送出一個 **Idle Frame（空閒幀）** 作為啟動訊號

---

**USART 傳送初始化與傳送流程步驟：**

1. **啟用 USART 模組**

   將 `USART_CR1` 暫存器中的 **UE（USART Enable）位元** 設為 1。

2. **設定資料長度**

   透過 `USART_CR1` 的 **M 位元** 選擇傳送字元長度（8 或 9 位元）。

3. **設定停止位元數量**

   在 `USART_CR2` 中設定 **STOP[1:0] 位元**，可選 0.5 / 1 / 1.5 / 2 stop bits。

4. **（可選）啟用 DMA 傳輸功能**

   若需使用 DMA 傳輸多筆資料，請：
   - 在 `USART_CR3` 中啟用 **DMAT 位元**
   - 配置 DMA 控制器（詳見 multi-buffer communication 說明）

5. **設定傳輸速度**

   在 `USART_BRR` 暫存器中設定所需的波特率（baud rate）。

6. **啟用傳送器**

   將 `USART_CR1` 中的 **TE（Transmit Enable）位元** 設為 1，啟用傳送器。
   > 啟用後會自動傳送一個 idle frame 作為初始訊號。

7. **寫入要傳送的資料**

   根據《RM0090》第 30.6.1 節的說明：

   > 當資料被寫入 `USART_DR`（對應 TDR 暫存器）時，  
   > **TXE（Transmit Data Register Empty）旗標會被清為 0**，表示暫存器忙碌中。  
   > 資料被移入傳輸移位暫存器後，**TXE 會重新設為 1**，表示可再寫入新資料。

   實作邏輯上應遵循以下順序：

   - **僅在 TXE = 1 時寫入 `USART_DR`**，表示暫存器為空，可寫入新資料  
   - 資料寫入後，TXE 立即變為 0，直到硬體完成搬移後才會重新設為 1  
   - 每筆資料皆需重複上述流程（單一緩衝區傳輸模式）

8. **等待傳輸完成**

   最後一筆資料寫入後，等待 **TC（Transmission Complete）= 1**
   - 代表最後一幀資料已完成傳送
   - 若接下來要關閉 USART，務必先確認 TC = 1，以免資料遺失

---

**程式範例：**
````c
usart_cr1_write_bit(USART1_BASE, USART_CR1_UE, 1); // // Enable USART (UE = 1)
usart_cr1_write_bit(USART1_BASE, USART_CR1_M, 0); // Set word length to 8 bits (M = 0)
usart_cr2_write_bits(USART1_BASE, USART_CR2_STOP, 2, USART_STOP_1); // Set stop bits to 1 bit (STOP[1:0] = 00)
usart_cr1_write_bit(USART1_BASE, USART_CR1_TE, 1); // Enable transmitter (TE = 1)

void usart_write(uint32_t usart_base, uint8_t data){
    uint32_t reg_addr = usart_base + USART_DR_OFFSET;

    while (usart_SR_read_bit(usart_base, USART_SR_TXE) == 0);

    io_write(reg_addr, (uint32_t)data);
}

void usart_wait_complete(uint32_t usart_base) {
    while (usart_SR_read_bit(usart_base, USART_SR_TC) == 0);
}

void usart_print(uint32_t usart_base, const char *str){
    while (*str) {
    	usart_write(usart_base, (uint8_t)*str);
        str++;
    }
    usart_wait_complete(usart_base);
}
````

---

## 3.4 printf 函式整合與字串輸出 

### 3.4.1 USART 字串輸出與 Tera Term 顯示驗證

開啟 Tera Term，建立新連線後選擇 ST-Link 提供的虛擬 COM 埠（例如 COM3），  
並將 Baud Rate 設為與 MCU 相同（例如：9600），其餘設定如下：

- Data bits：8 bits  
- Stop bits：1 bit
- 編碼接收（可選）：UTF-8 或 ISO8859-1（建議用於純 ASCII 顯示）

當 MCU 執行下列程式時：

```c
usart_print(USART1_BASE, "Hello\r\n");
```

Tera Term 終端機應能顯示：

```
Hello
```

---

#### 若 Tera Term 無法正確顯示內容，請依序檢查以下常見錯誤：

1. **USART RCC 未開啟**  
   請確認 `RCC->APB2ENR` 中已啟用 USART1（Bit 4）。

2. **GPIO PORTA 未開啟（用於 PA9/PA10）**  
   若使用 USART1 的 TX 輸出，需啟用 `GPIOA` 並將 `PA9` 設為 AF7 模式。  
   建議加入：
   ```c
   rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
   ```

3. **`FCK_APB2` 設定錯誤**  
   預設情況下，MCU 上電時：
   - `SYSCLK = HSI = 16 MHz`
   - `PPRE2 = 0b000`（無除頻）
   - ⇒ `FCK_APB2 = 16 MHz`

   若你誤設：
   ```c
   #define FCK_APB2 8000000  // 錯誤
   ```
   將導致 USART 實際輸出為 19200 baud，而非預期的 9600。  
   解法為改為：
   ```c
   #define FCK_APB2 16000000  // 正確
   ```

4. **波特率（Baud Rate）設定錯誤**  
   請確認 MCU 計算用的 `FCK_APB2` 值正確，否則 USART 實際輸出速率將與 Tera Term 不符。

5. **資料太快導致刷屏或看不到**  
   建議加入簡單延遲測試顯示內容：
   ```c
   for (volatile int i = 0; i < 1000000; i++);  // 約略延遲 1 秒
   ```

---

### 3.4.2 usart_printf() 實作：支援格式化輸出（%d, %x, %p, %s, %c, %%）

為了讓裸機環境下的 STM32 也能使用類似 `printf()` 的字串輸出功能，以下實作一個簡易版的 `usart_printf()` 函式，透過字元與變數組合完成動態格式輸出。

---

#### usart_printint()：整數轉字串輸出

````c
void usart_printint(int data, int base, int is_signed)
{
	static char digits[] = "0123456789ABCDEF";
	char buf[16];
	int count = 0;
	int is_negative = 0;
	int abs_data;

	if(is_signed && data < 0){
		is_negative = 1;
		abs_data = -data;
	} else {
		abs_data = data;
	}
	if (abs_data == 0){
		buf[count++] = digits[0];
	}else{
		while (abs_data != 0){
			buf[count++] = digits[abs_data % base];
			abs_data = abs_data / base;
		}
	}

	if(is_negative)
		buf[count++] = '-';

	while(--count >= 0)
		usart_write(USART1_BASE, buf[count]);
}
````

此函式可將整數轉換為指定進位（10 進位或 16 進位）後的字串，再透過 USART 輸出。若 `is_signed=1` 且數值為負，會自動補上負號。

- `base` = 10 → 十進位（對應 %d）
- `base` = 16 → 十六進位（對應 %x 或 %p）

---

#### usart_printf()：支援多種格式的字串輸出

````c
// Mini printf function for USART: supports %d, %x, %p, %s, %c, %%
void usart_printf(const char *fmt, ...)	
{
	int state = 0;  // 0 = normal text, 1 = after '%' waiting for format specifier
	int *arg_ptr = (int*)(void*)&fmt + 1;  // == (int*)((void*)&fmt + 4);pointer to the first variable argument

    for (int i = 0; fmt[i] != '\0'; i++){  // walk through each character in format string
        if (state == 0){
            if (fmt[i] == '%'){
                state = 1;  // enter format mode
            }else{
                usart_write(USART1_BASE, fmt[i]);
            }
        }else if (state == 1){
            if (fmt[i] == 'd'){
                usart_printint(*arg_ptr, 10, 1);
                arg_ptr = arg_ptr + 1;
            } else if (fmt[i] == 'x' || fmt[i] == 'p'){
                usart_printint(*arg_ptr, 16, 1);
                arg_ptr = arg_ptr + 1;
            } else if (fmt[i] == 's'){
                char *string = (char*)*arg_ptr;  // get string pointer from argument
                if (string == 0)
                    string = "(null)";
                for (int c = 0; *string != '\0'; c++){
                    usart_write(USART1_BASE, *string);
                    string = string + 1;  // move to next char
                }
                arg_ptr = arg_ptr + 1;  // move to next argument
            } else if (fmt[i] == 'c'){
                usart_write(USART1_BASE, (char)*arg_ptr);
                arg_ptr = arg_ptr + 1;
            } else if (fmt[i] == '%'){
                usart_write(USART1_BASE, '%');
            } else{  // Unrecognized specifier: output as % + character
                usart_write(USART1_BASE, '%');
                usart_write(USART1_BASE, fmt[i]);
            }
            state = 0;
        }
    }
}
````

- 輸入格式字串 `fmt`，包含文字與格式符號（specifier）
- 支援 `%d`、`%x`、`%p`、`%s`、`%c`、`%%`
- 使用 `arg_ptr` 指向格式字串後的第一個變數參數，逐個處理

---

#### 範例支援格式說明：

| 格式符號 | 說明                  | 備註             |
|----------|-----------------------|------------------|
| `%d`     | 有號十進位整數         | 呼叫 `usart_printint(..., 10, 1)` |
| `%x`     | 十六進位整數（小寫）    | 基本以大寫輸出 ABCDEF |
| `%p`     | 指標，實際等同 `%x` 處理 |                 |
| `%s`     | 字串輸出（char*）     | 支援 NULL 判斷    |
| `%c`     | 單一字元               |                 |
| `%%`     | 輸出單一 `%` 符號     |                 |

---

#### 特殊設計說明

- `arg_ptr = (int*)(void*)&fmt + 1;`：此行為手動取得第一個變數參數的位址，模擬 `varargs` 行為。雖可在嵌入式環境中運作，但此方式依賴平台堆疊結構，較不具可攜性。建議實務上改用 C 標準函式庫提供的變數參數機制：

  ```c
  #include <stdarg.h>
  ```

  搭配 `va_list`、`va_start`、`va_arg` 等標準巨集使用，可提升程式的可攜性與可維護性。

- 在處理 `%s` 時，雖參數實際為 `char*`，但透過 `int*` 讀取會被視為一個整數（即字串的起始位址）。因此需強制轉型為 `char*`，才能正確逐字輸出字串內容。

---

# 4. 外部中斷（EXTI）與中斷控制器（NVIC）

在 STM32 中，中斷的產生與處理由以下三個模組分工合作：

- **EXTI（External Interrupt/Event Controller）**：負責偵測外部腳位（如 PA0）上的電氣變化（如上升沿、下降沿），並產生中斷或事件。
- **NVIC（Nested Vectored Interrupt Controller）**：負責管理所有中斷來源（不只 EXTI），包含是否啟用中斷、設定優先順序等。
- **ISR（Interrupt Service Routine，中斷服務函式）**：開發者撰寫的函式，當中斷發生時，CPU 會自動跳轉至此函式執行對應的處理邏輯。

---

## 4.1 中斷觸發與事件機制 

### 4.1.1 ISR（Interrupt Service Routine）

ISR 是中斷發生時，CPU 所執行的特定函式。例如當：

- 按下按鈕（GPIO 邊緣變化）
- USART 收到資料
- 定時器時間到

這些事件皆可觸發對應的 ISR。CPU 會暫停主程式執行，自動跳轉至 ISR 處理對應事件，結束後再返回主程式。

---

STM32F4xx 系列支援使用 **`WFE`（Wait For Event）** 指令進入待命狀態，等待事件喚醒。這些事件可透過以下兩種方式產生：

---

### 4.1.2 模式一：中斷模式（Interrupt Mode）

此為最常見的模式。

在 STM32 中，**EXTI（External Interrupt/Event Controller）** 是負責偵測 GPIO 邊緣變化的模組。當 EXTI 偵測到腳位變化後，會產生對應的 IRQ 編號並傳給 **NVIC（Nested Vectored Interrupt Controller）**。

**NVIC** 是 Cortex-M4 處理器內建的中斷控制器，負責統一管理所有中斷來源，判斷是否啟用該中斷並交由 CPU 處理。當條件符合時，Cortex-M4 會自動進入中斷處理程序，跳轉至對應的 **ISR（Interrupt Service Routine，中斷服務函式）**。

中斷觸發後，由 Cortex-M4 硬體自動完成：
- 將主程式狀態（如 PC、xPSR）推入堆疊
- 跳轉至對應 ISR 位址（由向量表定義）

此階段與處理過程**不需 EXTI 再參與**。

#### 優點：

- 可直接撰寫 ISR 處理邏輯
- 反應速度快，適用於即時控制場景
- 常用於按鈕輸入、UART 收發、定時器觸發等

---

#### 中斷來源與對應 IRQ 編號範例：

| 中斷來源       | IRQ 編號 |
|----------------|----------|
| EXTI0          | 6        |
| USART1         | 37       |
| TIM2（定時器） | 28       |

---

#### 中斷觸發處理流程：

```
[GPIO 腳位 (如 PA0) 邊緣變化]
        ↓
[EXTI 偵測到變化 → 產生對應 IRQ 編號（如 EXTI0 = IRQ6）]
        ↓
[NVIC 收到 IRQ，判斷是否啟用與優先順序是否允許]
        ↓
[若允許 → Cortex-M4 硬體自動跳轉對應的 ISR]
        ↓
[CPU 執行 ISR，執行完自動返回主程式]
```

---

#### 設定步驟：

1. **EXTI 設定**：
   - `RTSR`（Rising Trigger Selection Register）：啟用上升緣觸發
   - `FTSR`（Falling Trigger Selection Register）：啟用下降緣觸發
   - `IMR`（Interrupt Mask Register）：允許該 EXTI 線產生中斷

2. **NVIC 設定**：
   - 啟用對應 IRQ（如 `EXTI0_IRQn`）
   - 設定中斷優先順序（可選）

---

### 4.1.3 模式二：事件模式（Event Mode）

此為 EXTI 的另一種應用方式，**不會產生 IRQ、也不會觸發 ISR**，而是透過「event（事件脈衝）」傳遞給系統，用於 **喚醒 CPU 或觸發其他硬體模組（如 DMA、Timer 等）**。

在此模式下，EXTI 偵測 GPIO 邊緣變化後，僅會產生 event，不會進入中斷處理流程。因此整個流程**不會經過 NVIC，也不需撰寫 ISR**。

---

#### 使用情境：

- 配合 `WFE` 使用，實現低功耗待命與喚醒機制
- GPIO 邊緣觸發 Timer、DMA 等模組動作，而不需要 CPU 介入
- 系統中需要快速反應但不需軟體邏輯處理的交握情境
- 適用於低功耗設計，不需進入 ISR 處理的應用場景

---

#### 運作流程：

```
[GPIO 腳位（如 PA0）邊緣變化]
        ↓
[EXTI 偵測事件 → 發出 event（非 IRQ）]
        ↓
[若有設定 EMR，則產生事件脈衝 → 可喚醒 CPU 或觸發硬體]
        ↓
[系統內部模組（WFE / DMA / Timer）接收事件，自行處理]
```

---

#### 設定步驟：

1. **EXTI 設定**：
   - `RTSR` / `FTSR`：設定上升/下降緣觸發
   - `EMR`（Event Mask Register）：允許產生 event（非中斷）
   - **不需設定 `IMR`**，因為此模式不涉及中斷處理

---

## 4.2 中斷向量表

中斷向量表（Interrupt Vector Table）是一張儲存在記憶體開頭的表格，用來定義當中斷或例外（Exception）發生時，CPU 要跳轉執行的處理函式位址。  
**預設位置為 `0x00000000`**，但實際上此位置是由處理器內部的 **VTOR（Vector Table Offset Register）** 指定，**可透過程式變更為其他位置（如 `0x08000000`，常見於 Bootloader 應用）**。

---

### 4.2.1 向量表格式與來源說明

根據 Reference Manual 第 12.2 節 *External interrupt/event controller* 所附的中斷表格，這是 MCU 啟動時放置於 `.isr_vector` 區段中的向量表內容。  
該表格省略了 Initial SP（初始堆疊指標）與部分保留欄位，通常從 `Reset` 項開始顯示。其欄位意義如下：

| 欄位名稱              | 說明 |
|-----------------------|------|
| **Position Priority** | 中斷編號（IRQ number），即 `nvic_enable_irq(...)` 所需的參數 |
| **Type of Priority**  | 優先順序型態，`fixed` 表示不可更改，`settable` 表示可透過暫存器設定 |
| **Acronym**           | 中斷或例外的代號，例如 `EXTI0`, `HardFault`, `SysTick` 等 |
| **Description**       | 中斷來源的簡要用途說明 |
| **Address**           | 向量表中儲存的處理函式位址（即 ISR 的入口位址） |

> Cortex-M 架構中，向量表前 16 筆為核心例外（Exception），從第 16 筆起才是 NVIC 管理的 IRQ 中斷項目（如 EXTI、USART、TIM 等）。

---

### 4.2.2 EXTI 中斷範例與 ISR 實作

以 EXTI Line 0 為例：

- 對應的 IRQ 編號為 **6**
- 對應的中斷服務函式名稱為 `EXTI0_IRQHandler()`
- 在啟動檔（通常為 `startup_stm32f4xx.s`）中，向量表會包含以下項目：

````assembly
.word EXTI0_IRQHandler   /* EXTI Line0 interrupt */
````

這表示當 IRQ6（EXTI0）發生時，CPU 將從向量表讀出該位址，**自動跳轉執行 `EXTI0_IRQHandler()` 函式**。

---

#### ISR 實作

開發者僅需在 C 程式中定義同名函式即可：

````c
void EXTI0_IRQHandler(void) {
    // 中斷處理邏輯
}
````

只要函式名稱正確，編譯器與連結器便會自動將其綁定至向量表中對應位置，**無需手動註冊或修改向量表**。

---

#### 補充說明

STM32 的啟動檔會在 `.isr_vector` 區段中，預先將每個中斷對應到正確的 IRQ 位置。這份向量表通常由 **CMSIS 或 ST 官方啟動碼提供**，開發者只需正確定義對應名稱的 ISR，即可完成中斷流程整合。

---

## 4.3 EXTI 線與觸發機制

### 4.3.1 EXTI 基本功能與觸發原理

EXTI 控制器最多支援 23 條邊緣偵測線（EXTI0 ~ EXTI22），每條 EXTI 線皆可獨立設定成 **中斷（Interrupt）** 或 **事件（Event）** 模式，並可選擇以下任一種觸發方式：

- 上升沿（Rising Edge）
- 下降沿（Falling Edge）
- 雙邊緣（Rising + Falling）

對應設定的控制暫存器如下：

| 功能            | 暫存器名稱       | 說明                               |
|-----------------|------------------|------------------------------------|
| 上升沿觸發設定   | `EXTI_RTSR`       | 設為 1 表示此線會對上升沿產生反應  |
| 下降沿觸發設定   | `EXTI_FTSR`       | 設為 1 表示此線會對下降沿產生反應  |
| 是否允許產生中斷 | `EXTI_IMR`        | 中斷遮罩暫存器，1=允許，0=遮罩     |
| 中斷掛起狀態     | `EXTI_PR`         | 中斷觸發時，對應 bit 會設為 1      |

> 注意：`EXTI_PR` 中的掛起旗標（Pending Flag）**不會自動清除**，需在 ISR 中手動寫入 1 才能清除，否則中斷會持續觸發。

---

### 4.3.2 EXTI 與 GPIO 對應規則與設定

#### EXTI 線與 GPIO 腳位的對應關係

EXTI 線 **並非固定對應到某個 Port**。以 `EXTI0` 為例，它可選擇對應至 `PA0 ~ PI0` 中任一腳位（取決於實體封裝與功能支援），但同一時間只能對應一個。

| EXTI 線  | 可對應的 GPIO 腳位     |
|----------|-------------------------|
| EXTI0    | PA0, PB0, PC0, ..., PI0 |
| EXTI1    | PA1, PB1, PC1, ..., PI1 |
| ...      | ...                     |
| EXTI15   | PA15, PB15, ..., PI15   |

#### 設定限制

- 每條 EXTI 線 **同一時間僅可對應一個 GPIO 腳位**
- 例如：EXTI0 可對應至 PA0 **或** PB0，但不可同時綁定

---

## 4.4 EXTI 中斷觸發實作：按鈕觸發印出訊息

開發板上的藍色按鈕連接至腳位 `PA0`，其電路設計為「上拉電阻 + 按下接地」的配置。也就是說：

- **按鈕未按下時**，腳位維持在 **高電位（邏輯 1）**
- **按鈕按下時**，腳位會被拉至 **低電位（邏輯 0）**

因此，建議將 EXTI 設為「**下降緣觸發**（falling edge trigger）」，當使用者按下按鈕、電位從高轉低時，觸發中斷事件。

按鈕觸發中斷的整體流程如下：

```text
GPIO 腳（如 PA0）
  ↓  
EXTI 邊緣偵測器（RTSR/FTSR）
  ↓  
IMR（允許或遮罩）
  ↓  
NVIC 傳送 IRQ 給 CPU
  ↓  
CPU 跳轉執行 ISR
```

為完成上述流程，需依序完成以下設定：

1. 將 PA0 設定為輸入模式  
2. 將 EXTI0 對應至 PA0  
3. 設定邊緣觸發條件與開啟中斷遮罩（IMR）  
4. 在 NVIC 中啟用對應 IRQ（EXTI0_IRQn，編號 6）  
5. 撰寫對應的 ISR，並於其中清除中斷掛起旗標  

---

### 4.4.1 GPIO 與 EXTI 初始化（PA0 作為輸入）

在 EXTI 中斷中，GPIO 腳位作為「邊緣偵測輸入」，無需設定為 Alternate Function（AF），僅需將其設為輸入模式即可：

```c
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);                   // 啟用 GPIOA 時脈
gpio_set_mode(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_INPUT);    // 將 PA0 設為輸入模式
```

---

### 4.4.2 將 EXTI0 對應至 PA0（SYSCFG 設定）

EXTI 控制器的每一條線（EXTI0 ~ EXTI15）都需指定其對應的 GPIO 腳位，這項對應關係是透過 `SYSCFG_EXTICR` 暫存器來設定的。

根據 Reference Manual 第 9 章 *System configuration controller (SYSCFG)*，第 9.3.3 至 9.3.6 節分別介紹了四個 EXTI 映射暫存器：

- `SYSCFG_EXTICR1`：對應 EXTI0 ~ EXTI3  
- `SYSCFG_EXTICR2`：對應 EXTI4 ~ EXTI7  
- `SYSCFG_EXTICR3`：對應 EXTI8 ~ EXTI11  
- `SYSCFG_EXTICR4`：對應 EXTI12 ~ EXTI15  

以將 EXTI0 對應至 PA0 為例，可使用下列函式：

```c
void exti_select_port(SYSCFG_EXTI_LINE exti_line, uint8_t port_code) {
    uint32_t reg_addr;
    uint32_t shift = (exti_line % 4) * 4;
    uint32_t mask  = 15U << shift;
    uint32_t data  = ((uint32_t)port_code << shift);

    switch (exti_line / 4) {
        case 0: reg_addr = SYSCFG_BASE + SYSCFG_EXTICR1_OFFSET; break;
        case 1: reg_addr = SYSCFG_BASE + SYSCFG_EXTICR2_OFFSET; break;
        case 2: reg_addr = SYSCFG_BASE + SYSCFG_EXTICR3_OFFSET; break;
        case 3: reg_addr = SYSCFG_BASE + SYSCFG_EXTICR4_OFFSET; break;
        default: return; // 防呆：僅允許 EXTI0 ~ EXTI15
    }

    io_writeMask(reg_addr, data, mask);
}
```

在主程式中，初始化順序如下：

```c
rcc_enable_apb2_clock(RCC_APB2EN_SYSCFG);                 // 啟用 SYSCFG 時脈
exti_select_port(SYSCFG_EXTI0, SYSCFG_EXTICR_PORTA);      // 將 EXTI0 對應至 Port A（即 PA0）
```

上述設定會將 `SYSCFG_EXTICR1` 的最低 4-bit 設為 `0x0`，代表選擇 Port A 為 EXTI0 的輸入來源。

---

### 4.4.3 設定下降沿觸發與開啟中斷遮罩

#### 邊緣觸發設定

根據 Reference Manual 第 12.3.3 與 12.3.4 節，EXTERNAL INTERRUPT（EXTI）模組使用以下兩個暫存器設定觸發邊緣：

- `EXTI_RTSR`：設定 **上升沿（Rising edge）** 觸發
- `EXTI_FTSR`：設定 **下降沿（Falling edge）** 觸發

以下函式可用來設定指定 EXTI 線的觸發方式：

```c
void exti_enable_rising_trigger(SYSCFG_EXTI_LINE exti_line) {
    uint32_t reg_addr = EXTI_BASE + EXTI_RTSR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);
}

void exti_enable_falling_trigger(SYSCFG_EXTI_LINE exti_line) {
    uint32_t reg_addr = EXTI_BASE + EXTI_FTSR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);
}
```

---

#### 中斷遮罩設定：EXTI_IMR

`IMR`（Interrupt Mask Register，中斷遮罩暫存器）用來控制哪一條 EXTI 線允許產生中斷（IRQ），哪一條會被遮罩。

若未設定 IMR，則即使 EXTI 偵測到邊緣變化，也**不會觸發對應的中斷服務函式（ISR）**。  
例如：若 `IMR[0] = 0`，即使 `PA0` 有訊號變化，最終也不會送出 IRQ 中斷請求。

以下為設定 IMR 是否啟用的控制函式：

```c
void exti_set_interrupt_mask(SYSCFG_EXTI_LINE exti_line, EXTI_InterruptMask enable) {
    uint32_t reg_addr = EXTI_BASE + EXTI_IMR_OFFSET;
    uint32_t mask = 1U << exti_line;
    uint32_t data = (uint32_t)enable << exti_line;

    io_writeMask(reg_addr, mask, data);
}
```

---

### 4.4.4 NVIC 中斷啟用

當 EXTI 偵測到腳位的邊緣事件且對應中斷未被 IMR 遮罩後，會送出 IRQ 給 NVIC。此時必須在 **NVIC（Nested Vectored Interrupt Controller，中斷控制器）中啟用對應的中斷來源**，否則 CPU 將無法跳轉至中斷服務函式（ISR）執行。

---

#### NVIC 結構與 IRQ 編號對應方式

STM32F429 所使用的 NVIC 模組由 ARM Cortex-M4 核心內建提供，而非 ST 自行設計。根據《Cortex-M4 User Guide》第 4.2 節 *Nested Vectored Interrupt Controller* 說明：

> 中斷啟用的暫存器為 `NVIC_ISERx`（Interrupt Set-Enable Registers）  
> 每組為 32-bit 寬度，控制 32 個中斷來源  
> 起始位址為：`0xE000E100`

| 暫存器名稱 | 可控制的 IRQ 範圍 | 位址         |
|------------|-------------------|--------------|
| ISER0      | IRQ 0 ~ 31        | 0xE000E100   |
| ISER1      | IRQ 32 ~ 63       | 0xE000E104   |
| ISER2      | IRQ 64 ~ 95       | 0xE000E108   |
| ...        | ...               | ...          |

每組暫存器間間隔 4 bytes。

---

#### EXTI0 IRQ 編號對應 ISER 暫存器

欲啟用 `EXTI0`（對應腳位為 `PA0`），其中斷來源為 `EXTI0_IRQn`，IRQ 編號為 **6**，即可透過以下方式判斷對應哪一組 ISER 暫存器：

- 例如 `irqn = 6`：`6 / 32 = 0` → 落在 `ISER0`
- 例如 `irqn = 37`：`37 / 32 = 1` → 落在 `ISER1`

---

#### 通用 NVIC 中斷啟用函式

下列函式可根據 IRQ 編號自動計算對應暫存器位址與位元位置，進行中斷啟用：

````c
#define NVIC_ISER_BASE  0xE000E100U  // NVIC Interrupt Set-Enable Registers (ISER0 base)

typedef enum {
    WWDG        = 0,   // Window Watchdog interrupt
    PVD         = 1,   // PVD through EXTI line detection
    TAMP_STAMP  = 2,   // Tamper and TimeStamp interrupts
    RTC_WKUP    = 3,   // RTC Wakeup interrupt
    FLASH       = 4,   // Flash global interrupt
    RCC         = 5,   // RCC global interrupt
    EXTI0       = 6    // EXTI Line0 interrupt
} IRQn;

void nvic_enable_irq(IRQn irqn) {
    // 每 32 個中斷共用一組 32-bit ISER 暫存器，位址間隔為 4 bytes
    uint32_t reg_addr = NVIC_ISER_BASE + ((uint32_t)irqn / 32U) * 4U;

    // 計算該中斷在對應 ISER 暫存器中的位元遮罩
    uint32_t bit_mask = 1U << (irqn % 32U);

    // 寫入對應位置以啟用中斷（Set Enable）
    io_writeMask(reg_addr, bit_mask, bit_mask);
}
````

---

#### 實際使用範例

```c
nvic_enable_irq(EXTI0);   // 啟用 NVIC 的 EXTI0 IRQ（IRQn = 6）
```

---

### 4.4.5 ISR：撰寫中斷服務函式

每一個 IRQ 都對應一個「中斷服務函式」，EXTI0 對應的函式為 `EXTI0_IRQHandler()`。

```c
void EXTI0_IRQHandler(void) {
    // 清除中斷旗標（必須清除，否則會重複進入中斷）
    exti_clear_pending_flag(SYSCFG_EXTI0);

    // 實際處理邏輯
    printf("Button Pressed!\r\n");
}
```

其中 `exti_clear_pending_flag()` 函式實作如下：

```c
void exti_clear_pending_flag(SYSCFG_EXTI_LINE exti_line) {
    uint32_t reg_addr = EXTI_BASE + EXTI_PR_OFFSET;
    uint32_t data = 1U << exti_line;

    io_writeMask(reg_addr, data, data);  // 寫 1 清除對應中斷旗標
}
```

> 注意：**若未清除 EXTI_PR 的 pending bit，則會一直觸發中斷，導致 CPU 反覆進入 ISR。**

---

# 5. LCD 控制實作與顯示測試

## 5.1 LCD 顯示介面介紹與 STM32F429I-DISC1 架構分析

---

### 5.1.1 常見 LCD 控制方式與 STM32 對應模組

| 控制方式                  | 簡介        | 使用介面                          | 對應 STM32 模組                            | 傳輸方式 | 備註                                                             |
|---------------------------|-------------|-----------------------------------|--------------------------------------------|----------|------------------------------------------------------------------|
| **SPI**                   | 最簡單常見   | MOSI, SCK, CS, DC                 | SPI                                        | 序列     | 速度慢，通常 MCU 寫一筆畫一筆                                     |
| **FMC（8080 並列）**      | 中階常見     | RD, WR, DB0~DB15                  | FMC (Flexible Memory Controller)           | 並列     | LCD 為 MCU Interface 時使用（如 ST7796、ILI9341 也支援）         |
| **RGB 並行介面**          | 高速顯示     | HSYNC, VSYNC, DOTCLK, RGB888      | **LTDC** (LCD-TFT Display Controller)      | 並行即時 | 直接「驅動螢幕」，類似繪圖卡                                        |
| **DSI / HDMI / LVDS**     | 更高階應用   | 差動線對                           | DSI Host（僅部分 STM32 有支援）           | 並行高速 | 用於高解析、觸控 LCD，如智慧型設備顯示                            |

---

### 5.1.2 常見 LCD 控制器與 ILI9341 介紹

---

#### 常見 LCD 控制器與其製造商對照表

| 控制器型號   | 製造商               | 常見解析度 | 支援介面                       | 備註                                             |
|--------------|------------------------|------------|----------------------------------|--------------------------------------------------|
| **ILI9341**  | 奕力科技 Ilitek         | 240×320    | SPI、8080 並列、RGB 並行         | 最常見型號之一，支援多種介面，開發資源豐富         |
| **ST7789**   | 矽創電子 Sitronix       | 240×240    | SPI                             | 僅支援序列傳輸，常見於小尺寸模組                   |
| **ST7796**   | 矽創電子 Sitronix       | 320×480    | 8080 並列、RGB 並行             | 常見於 3.5 吋模組，與 ILI9486 兼容性高             |
| **ILI9486**  | 奕力科技 Ilitek         | 320×480    | 8080 並列、RGB 並行             | 色彩與亮度表現佳，常見於中高階顯示模組             |

---

#### ILI9341 控制器詳細介紹

**ILI9341** 是由 **台灣奕力科技（Ilitek）** 設計的 LCD 控制器晶片，廣泛應用於 2.4 ~ 2.8 吋 QVGA (240×320) TFT LCD 模組中。此晶片支援多種傳輸介面與像素格式，具備良好的相容性與開發彈性。

##### 支援介面：

- **SPI**（3 線或 4 線模式）
- **8080 MCU 並列介面**（8-bit 或 16-bit）
- **RGB 並行介面**（同步訊號驅動）

##### 支援像素格式：

- **RGB565**（16-bit，最常見格式）
- **RGB666 / RGB888**（需配合硬體支援）

---

#### ILI9341 的顯示介面模式比較

| 模式名稱        | 說明                                                       | MCU 傳輸方式與角色                      |
|------------------|------------------------------------------------------------|-----------------------------------------|
| **命令模式**     | LCD 接收 MCU 傳送的命令與資料，內部控制顯示邏輯（常見於 SPI 或 8080 並列） | MCU 每次畫圖需傳送命令與像素資料        |
| **RGB 並行模式** | LCD 僅接收畫面像素資料，不處理命令，根據同步時序即時顯示畫面           | MCU 透過 LTDC 模組產生同步訊號與資料輸出 |

---

### 5.1.3 STM32F429I-DISC1 LCD 架構與顯示原理

根據 ST 官方文件《UM1670》第 6.9 節，STM32F429I-DISC1 開發板上的 LCD 模組具備以下特性：

- **控制器**：ILI9341  
- **解析度**：QVGA (240×320)  
- **顯示介面**：RGB 並行介面  
- **傳輸方式**：HSYNC、VSYNC、DOTCLK、RGB888

---

#### LTDC（LCD-TFT Display Controller）模組介紹

LTDC 是 STM32F429 內建的顯示控制模組，用於驅動 RGB 並行介面顯示器。其角色類似「硬體顯示卡」，負責自動產生畫面所需的像素資料與同步訊號。其功能如下：

- 自動從 **frame buffer（影像緩衝區）** 中讀取像素資料
- 主動產生 LCD 所需的同步時序與畫素訊號（如 HSYNC、VSYNC、DOTCLK）
- 將畫面資料以並行形式輸出至 LCD（如 ILI9341），無需 MCU 逐筆傳送
- 支援多層圖層（Layer 0 / Layer 1），可用於合成背景與前景圖層（例如 OSD）

開發流程中，MCU 會先將畫面內容填入 RAM（通常為 SDRAM），再由 LTDC 依照設定的時序自動「掃描」這塊記憶體，並同步輸出資料至 LCD 控制器顯示。

---

#### RGB 並行介面顯示原理

RGB 並行介面是一種即時且不需命令控制的顯示方式，其特點如下：

- 每個像素以 **RGB888** 格式輸出（R/G/B 各 8-bit，共 24-bit），但實際模組多採用 **RGB565**（16-bit）
- 畫面資料以「像素為單位」連續傳輸，並搭配時序訊號實現逐行掃描
- MCU 不再送出 command，ILI9341 僅作為 RGB 資料流的接收與顯示裝置
- 顯示需搭配以下同步訊號：
  - **HSYNC**（水平同步）
  - **VSYNC**（垂直同步）
  - **DOTCLK**（像素時脈）
  - **DE**（Display Enable，部分面板使用）

這類機制與 VGA / HDMI 類似，畫面會由顯示控制器持續掃描顯示，而非由 MCU 傳送命令逐筆控制。

---

#### 與 SPI 控制方式的差異比較

| 比較項目   | SPI LCD                         | LTDC RGB LCD                          |
|------------|----------------------------------|----------------------------------------|
| 傳輸方式   | MCU 每次畫圖需逐筆傳送像素資料     | LTDC 自動掃描並輸出整塊畫面               |
| 儲存方式   | MCU 自行處理繪圖並送資料           | 畫面資料存於 SDRAM 的 frame buffer       |
| 顯示效能   | 傳輸速度慢，畫面更新易閃爍         | 高速傳輸，顯示穩定流暢                   |
| 控制方式   | LCD 控制器解析命令與資料           | LCD 僅接收 RGB 資料流，不解析命令          |
| 開發難度   | 程式邏輯簡單                     | 需設定時序參數與 SDRAM 接口               |

---

### 5.1.4 開發流程與系統架構總結

STM32F429I-DISC1 開發板的 LCD 模組採用 **ILI9341 控制器**，並以 **RGB 並行模式**連接至 **LTDC 顯示控制模組**。整體顯示架構建立於 **LTDC + SDRAM frame buffer** 上，由 LTDC 自動掃描記憶體內容並輸出畫面資料至 LCD，**無需 MCU 手動逐筆畫圖**。

#### LCD 顯示流程整理：

0. **初始化 RGB 腳位**
   - 呼叫 `ltdc_gpio_init()` → 將所有 RGB 並行輸出腳設為 Alternate Function 模式（AF14）

1. **MCU 啟動後，依序初始化 FMC 與 SDRAM**
   - 呼叫 `sdram_gpio_init()` 設定腳位 → `sdram_init()` 執行初始化序列
   - SDRAM 初始化成功後，`0xD0000000` 即可作為可讀寫的 frame buffer 空間
   - *若未正確初始化 SDRAM，LTDC 將無法讀取有效畫面資料，可能導致畫面無法顯示、顯示異常或出現雜訊*

2. **LTDC 初始化並指定 frame buffer 起始位址**
   - 呼叫 `ltdc_clock_init()` → 設定 pixel clock（例如透過 PLLSAI 輸出）
   - 呼叫 `ltdc_init()` → 設定解析度、同步時序、像素格式（通常為 RGB565）與 frame buffer 起始位址（0xD0000000）

3. **MCU 將畫面資料（如純紅色）寫入 SDRAM 的 frame buffer**
   - 呼叫 `lcd_clear_red()` → 將整塊 SDRAM 填上紅色像素資料（0xF800）

4. **LTDC 模組自動讀取 SDRAM 並透過 RGB 並行介面輸出畫面**
   - 呼叫 `ltdc_start_display()` → 啟用 shadow reload，LTDC 開始傳送畫面
   - 資料經由 `HSYNC`、`VSYNC`、`DOTCLK` 與 RGB 資料線輸出至 LCD 面板
   - MCU 不需再手動傳送指令，畫面更新由硬體 LTDC 自動完成

---

#### 顯示架構關鍵整理：

- LCD 顯示流程依賴 **LTDC + SDRAM** 架構，不再透過 MCU 操作 LCD 命令
- SDRAM frame buffer 位於 **0xD0000000**，由 LTDC 模組自動讀取並顯示
- **顯示流程模組劃分：**
  1. SDRAM：儲存畫面資料
  2. LTDC：產生時序訊號與畫面輸出
  3. RGB 並行介面：傳送畫素資料至 LCD 控制器
  4. MCU 僅需初始化與資料填入，不負責畫面輸出時序

> 備註：若使用其他 LCD 模組（如 SPI），MCU 將需負責每筆像素的傳輸，但本板為 RGB 並行架構，所有畫面輸出由 LTDC 接管。

---

## 5.2 GPIO 腳位初始化：LTDC 與 SDRAM

為了讓 LTDC 顯示模組與 SDRAM 記憶體能正常運作，必須先將相關 GPIO 腳位設定為對應的 Alternate Function 模式。STM32F429I-DISC1 所使用的 RGB LCD 採用 LTDC 模組驅動，而畫面資料則儲存在外接 SDRAM，兩者皆透過大量 GPIO 腳位與內部匯流排連接。

本節將依序完成：
1. 設定 LTDC 所需的 RGB 並行輸出腳為 **AF14**
2. 設定 FMC 控制 SDRAM 所需的地址線與資料線腳位為 **AF12**

### 5.2.1 LTDC GPIO 腳位初始化

#### LTDC 的三個時脈區域（Clock Domain）

LTDC 顯示控制器主要分為三個時脈區域，各自負責不同功能：

---

1. **AHB clock domain**：資料傳輸來源  
   - 畫面資料來自 **SDRAM**（frame buffer）
   - 透過 AHB 匯流排傳送至 LTDC 內部的 Layer FIFO 暫存區

2. **APB2 clock domain**：控制設定與中斷通知  
   - MCU 可透過 APB2 匯流排存取 LTDC 的控制暫存器與狀態暫存器
   - 用於設定解析度、Layer 位置、像素格式、啟用參數等
   - 同時提供中斷通知功能（畫面更新完成、FIFO 狀態、錯誤等）

3. **Pixel clock domain**：畫面處理與輸出核心  
   - 含圖層混合、像素格式轉換、抖動處理等模組，搭配 Timing Generator 產生畫面同步訊號
   - 包含以下功能：
     - **Layer FIFO**：兩層圖層暫存（每層 64×32-bit），可獨立啟用/關閉
     - **Pixel Format Converter（PFC）**：像素格式統一轉換（如 RGB565、ARGB8888）
     - **Blending Unit**：混合 Layer0 / Layer1 畫素資料（支援透明度混合）
     - **Dithering Unit**：將高位元顏色轉換為較低位元輸出，避免顏色斷層
     - **Timing Generator**：產生畫面時序訊號，如 `HSYNC`、`VSYNC`、`DE`、`CLK`
   - 最終輸出資料經由 RGB 並行介面（如下所示）送往 LCD 面板

---

#### 輸出訊號（通往 LCD 面板）

| 訊號腳位         | 說明 |
|------------------|------|
| `LCD_HSYNC`      | 水平同步訊號 |
| `LCD_VSYNC`      | 垂直同步訊號 |
| `LCD_DE`         | Data Enable：資料傳輸使能 |
| `LCD_CLK`        | Dot Clock：像素時脈 |
| `LCD_R[7:0]`     | 紅色通道（8 bit） |
| `LCD_G[7:0]`     | 綠色通道（8 bit） |
| `LCD_B[7:0]`     | 藍色通道（8 bit） |

這些訊號對應至 STM32 GPIO 所設定的 **AF14 腳位功能**，會在 `ltdc_gpio_init()` 中統一設定，最終將畫面資料輸出至 LCD-TFT 面板。

---

#### LCD RGB 資料線 GPIO 初始化設定

根據 LCD 模組 **FRD240C48003-B** 的原理圖可知，其 RGB 各通道僅接出 6 條資料腳位（R0~R5、G0~G5、B0~B5），表示該模組採用 **RGB666（18-bit）** 顯示格式。此格式常見於嵌入式系統，能有效節省 GPIO 腳位，同時提供良好的顯示品質。

---

##### 顏色格式比較：

| 格式     | 每通道位元 | 總位元數 | 說明                             |
|----------|-------------|-----------|----------------------------------|
| RGB888   | 8 bit × 3   | 24 bits   | 真正全彩顯示，每通道 256 色階     |
| RGB666   | 6 bit × 3   | 18 bits   | 常見於 MCU 應用，節省 GPIO 腳位   |
| RGB565   | 5-6-5       | 16 bits   | 最省資源格式，與 MCU 相容性最佳   |

---

##### Red 通道（R0~R5）：

| 資料位元 | STM32 GPIO |
|----------|------------|
| R0       | PC10       |
| R1       | PB0        |
| R2       | PA11       |
| R3       | PA12       |
| R4       | PB1        |
| R5       | PG6        |
| R6~R7    | 未接       |

---

##### Green 通道（G0~G5）：

| 資料位元 | STM32 GPIO |
|----------|------------|
| G0       | PA6        |
| G1       | PG10       |
| G2       | PB10       |
| G3       | PB11       |
| G4       | PC7        |
| G5       | PD3        |
| G6~G7    | 未接       |

---

##### Blue 通道（B0~B5）：

| 資料位元 | STM32 GPIO |
|----------|------------|
| B0       | PD6        |
| B1       | PG11       |
| B2       | PG12       |
| B3       | PA3        |
| B4       | PB8        |
| B5       | PB9        |
| B6~B7    | 未接       |

---

##### GPIO 初始化範例程式：

````C
void ltdc_gpio_init(void) 
{
    // Enable GPIO clocks
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOB);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

    // RED pins (R0~R5)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // R0
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE); // R1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_0, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // R2
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // R3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_12, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE); // R4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_1, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // R5
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    // GREEN pins (G0~G5)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // G0
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // G2
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_10, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // G3
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOC_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE); // G4
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_7, ALTERNATE_AF14);

    gpio_set_mode(GPIOD_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE); // G5
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_3, ALTERNATE_AF14);

    // BLUE pins (B0~B5)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE); // B0
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_6, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // B1
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_11, ALTERNATE_AF14);

    gpio_set_mode(GPIOG_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // B2
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_12, ALTERNATE_AF14);

    gpio_set_mode(GPIOA_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE); // B3
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_3, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE); // B4
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_8, ALTERNATE_AF14);

    gpio_set_mode(GPIOB_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE); // B5
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_9, ALTERNATE_AF14);
}
````

---

#### LCD 同步與資料控制 GPIO 初始化設定

LCD 顯示需要除畫素資料（RGB）外，還需搭配數條 **時序控制訊號（Timing Control Signals）**，用來同步畫面掃描與資料輸出。

這些訊號不屬於畫面內容本身，而是協助 LCD 正確更新畫面的重要時脈與同步腳位。

---

##### 控制訊號對應關係：

| 原理圖名稱 | LTDC 訊號名稱 | 功能說明         | STM32 GPIO 腳位 |
|------------|----------------|------------------|------------------|
| HSYNC      | `LCD_HSYNC`    | 水平同步訊號     | `PC6`           |
| VSYNC      | `LCD_VSYNC`    | 垂直同步訊號     | `PA4`           |
| ENABLE     | `LCD_DE`       | Data Enable（使能）| `PF10`          |
| DOTCLK     | `LCD_CLK`      | Dot Clock（像素時脈）| `PG7`        |

---

##### GPIO 初始化範例程式：

````c
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

// LCD control signals
gpio_set_mode(GPIOF_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE);  // LCD_DE
gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_10, ALTERNATE_AF14);

gpio_set_mode(GPIOG_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);   // LCD_CLK
gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_7, ALTERNATE_AF14);

gpio_set_mode(GPIOC_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);   // LCD_HSYNC
gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_6, ALTERNATE_AF14);

gpio_set_mode(GPIOA_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);   // LCD_VSYNC
gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_4, ALTERNATE_AF14);
````

---

### 5.2.2 SDRAM 記憶體 GPIO 腳位初始化（FMC 控制器）

#### SDRAM 與 FMC 控制器簡介

STM32F429 除了內建的 SRAM、Flash 等內部記憶體之外，為了擴充儲存容量與提升資料存取效率，常會透過外部介面擴接多種記憶體模組，例如 SRAM、Flash、以及 SDRAM。

在 LCD 顯示應用中，MCU 為了能一次性輸出整個畫面像素資料，通常會先將圖像內容暫存於 **SDRAM**，再由 **LTDC 顯示模組自動掃描該記憶體內容並輸出至畫面**。因此，MCU 必須能有效地與 SDRAM 通訊與存取。

STM32F429 提供的 **FMC（Flexible Memory Controller）** 模組是一個高整合度的記憶體控制單元，能支援多種外部記憶體介面，包含：

- SRAM / PSRAM
- NOR Flash / NAND Flash
- **SDRAM（同步動態隨機存取記憶體）**

FMC 具備自動控制時序、命令序列、暫存區與匯流排協定轉換能力，能讓 MCU 以與內部記憶體相同的方式存取外部 SDRAM。

在 STM32F429 系統中，**SDRAM 是透過 FMC（Flexible Memory Controller）模組，掛接於 AHB3 匯流排上運作**。  
所有與 SDRAM 通訊的 GPIO 腳位，皆必須設定為 **AF12（Alternate Function 12，對應 FMC 模組）** 模式。

根據《RM0090 Reference Manual》的 **第 37.4.3 節 SDRAM address mapping** 所述：  
只要 `HADDR[28] = 0`，便表示 SDRAM 掛接於 **FMC 的 Bank1 子模組**，並由 `FMC_SDCR1`、`FMC_SDTR1` 等暫存器進行控制。

而在 **第 37.7.2 節 SDRAM external memory interface signals** 中，列出了 SDRAM 所需的硬體訊號腳位，包含：

- **地址線（A0~A12）**
- **資料線（D0~D31）**（實際可能僅接至 D15）
- **控制線**：如 `SDCKE1`（Clock Enable）、`SDNE1`（Chip Enable）、`SDCLK`（SDRAM Clock）等

這些腳位皆需設定為 **AF12 模式**，以對應 FMC 功能，完成 SDRAM 的初始化與資料交換操作。

> 補充：FMC 外部記憶體的位址由 MCU 固定對映，**並非接續於內部 SRAM 之後**。  
> 開發板上的外部 SDRAM（32MB）會被映射至獨立的位址區間 `0xD0000000`，與內部 SRAM 無重疊或連接。  
> **此區域即為 LTDC 使用的 frame buffer 儲存位置**，MCU 可透過讀寫該記憶體區段，間接控制 LCD 顯示內容。

---

#### SDRAM 與 GPIO 對應關係整理（依原理圖）

從原理圖中可確認，SDRAM 僅使用以下訊號與 STM32F429 MCU 相連：

- **位址線（Address）**：接至 A0 ~ A11
- **資料線（Data）**：接至 D0 ~ D15
- **控制線（Control）**：僅接上部分常用訊號，其餘視應用需求補充

---

##### SDRAM 控制訊號對應關係：

| 原理圖名稱 | FMC 訊號名稱 | 功能說明                     | STM32 GPIO 腳位 |
|------------|---------------|------------------------------|------------------|
| CLK        | `SDCLK`       | SDRAM 時脈來源               | `PG8`            |
| CKE        | `SDCKE1`      | Clock Enable（時脈啟用）     | `PB5`            |
| /CS        | `SDNE1`       | SDRAM 晶片啟用（Chip Enable）| `PB6`            |
| /WE        | `SDNWE`       | 寫入使能（Write Enable）     | `PC0`            |
| /RAS       | `SDNRAS`      | 列位址啟動（Row Addr Strobe）| `PF11`           |
| /CAS       | `SDNCAS`      | 行位址啟動（Col Addr Strobe）| `PG15`           |

---

##### SDRAM 地址線對應（A0 ~ A11）：

| 資料位元 | STM32 GPIO |
|----------|------------|
| A0       | PF0        |
| A1       | PF1        |
| A2       | PF2        |
| A3       | PF3        |
| A4       | PF4        |
| A5       | PF5        |
| A6       | PF12       |
| A7       | PF13       |
| A8       | PF14       |
| A9       | PF15       |
| A10      | PG0        |
| A11      | PG1        |

---

##### SDRAM 資料線對應（D0 ~ D15）：

| 資料位元 | STM32 GPIO |
|----------|------------|
| D0       | PD14       |
| D1       | PD15       |
| D2       | PD0        |
| D3       | PD1        |
| D4       | PE7        |
| D5       | PE8        |
| D6       | PE9        |
| D7       | PE10       |
| D8       | PE11       |
| D9       | PE12       |
| D10      | PE13       |
| D11      | PE14       |
| D12      | PE15       |
| D13      | PD8        |
| D14      | PD9        |
| D15      | PD10       |

---

#### GPIO 初始化範例程式：

````C
void fmc_gpio_init(void)
{
    // Enable GPIO clocks
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOB);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOE);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOG);

    // Address lines A0~A11
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // A0
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // A1
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_1, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_2, GPIO_MODE_ALTERNATE);  // A2
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_2, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_3, GPIO_MODE_ALTERNATE);  // A3
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_3, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_4, GPIO_MODE_ALTERNATE);  // A4
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_4, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_5, GPIO_MODE_ALTERNATE);  // A5
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_5, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // A6
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_12, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_13, GPIO_MODE_ALTERNATE); // A7
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_13, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // A8
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // A9
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // A10
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // A11
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_1, ALTERNATE_AF12);

    // Data lines D0~D15
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // D0
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // D1
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // D2
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_1, GPIO_MODE_ALTERNATE);  // D3
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_1, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);  // D4
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_7, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // D5
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // D6
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_9, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // D7
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_10, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // D8
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_11, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_12, GPIO_MODE_ALTERNATE); // D9
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_12, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_13, GPIO_MODE_ALTERNATE); // D10
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_13, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_14, GPIO_MODE_ALTERNATE); // D11
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_14, ALTERNATE_AF12);
    gpio_set_mode(GPIOE_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // D12
    gpio_set_alternate_function(GPIOE_BASE, GPIO_PIN_15, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // D13
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);  // D14
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_9, ALTERNATE_AF12);
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_10, GPIO_MODE_ALTERNATE); // D15
    gpio_set_alternate_function(GPIOD_BASE, GPIO_PIN_10, ALTERNATE_AF12);

    // Control signals
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);  // SDCLK
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_8, ALTERNATE_AF12);
    gpio_set_mode(GPIOB_BASE, GPIO_PIN_5, GPIO_MODE_ALTERNATE);  // SDCKE1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_5, ALTERNATE_AF12);
    gpio_set_mode(GPIOB_BASE, GPIO_PIN_6, GPIO_MODE_ALTERNATE);  // SDNE1
    gpio_set_alternate_function(GPIOB_BASE, GPIO_PIN_6, ALTERNATE_AF12);
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_0, GPIO_MODE_ALTERNATE);  // SDNWE
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_0, ALTERNATE_AF12);
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_11, GPIO_MODE_ALTERNATE); // SDNRAS
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_11, ALTERNATE_AF12);
    gpio_set_mode(GPIOG_BASE, GPIO_PIN_15, GPIO_MODE_ALTERNATE); // SDNCAS
    gpio_set_alternate_function(GPIOG_BASE, GPIO_PIN_15, ALTERNATE_AF12);
}
````

---

## 5.3 FMC 參數初始化

### 5.3.1 開啟 AHB3 時脈

根據《STM32F429 Datasheet》的 Block Diagram，**FMC 模組掛載於 AHB3 匯流排**，因此在初始化 SDRAM 之前，需先啟用 FMC 的 AHB3 時脈。

以下為 AHB3 Clock 啟用程式：

````c
#define RCC_AHB3ENR_FMCEN (1U << 0)

void rcc_enable_ahb3_clock(void) {
    uint32_t addr = RCC_BASE + RCC_AHB3ENR;
    io_writeMask(addr, RCC_AHB3ENR_FMCEN, RCC_AHB3ENR_FMCEN); // Enable FMC clock
}
````

---

### 5.3.2 設定 FMC_SDCR 與 FMC_SDTR 暫存器（Bank1）

由於 SDRAM 預設會掛接於 **FMC 的 Bank1 子模組**（`HADDR[28] = 0`），而 Bank1 是由 `FMC_SDCR1` 與 `FMC_SDTR1` 兩個暫存器所控制，因此需對這兩個暫存器進行基本設定，例如 SDRAM 的資料寬度、記憶體大小、CAS 延遲、列/行位元數等。以下為程式碼範例：

````c
#define FMC_BASE 0xA0000000

#define FMC_SDCR_OFFSET(bank)  (0x140 + ((bank - 1) * 4))
#define FMC_SDTR_OFFSET(bank)  (0x148 + ((bank - 1) * 4))

typedef enum{
    FMC_SDCR_NC = 0,
    FMC_SDCR_NR,
    FMC_SDCR_MWID,
    FMC_SDCR_NB,
    FMC_SDCR_CAS,
    FMC_SDCR_WP,
    FMC_SDCR_SDCLK,
    FMC_SDCR_RBURST,
    FMC_SDCR_RPIPE
} fmc_sdcr_field_t;

typedef enum {
    FMC_SDTR_TMRD = 0,   // [3:0]
    FMC_SDTR_TXSR,       // [7:4]
    FMC_SDTR_TRAS,       // [11:8]
    FMC_SDTR_TRC,        // [15:12]
    FMC_SDTR_TWR,        // [19:16]
    FMC_SDTR_TRP,        // [23:20]
    FMC_SDTR_TRCD        // [27:24]
} fmc_sdtr_field_t;

void fmc_init(void){
	rcc_enable_ahb3_clock();

	// Configure FMC_SDCR
    fmc_sdcr_write_field(1, FMC_SDCR_NC,    0x01); // Column = 9 bits
    fmc_sdcr_write_field(1, FMC_SDCR_NR,    0x01); // Row = 12 bits
    fmc_sdcr_write_field(1, FMC_SDCR_MWID,  0x01); // Memory width = 16-bit
    fmc_sdcr_write_field(1, FMC_SDCR_NB,    0x01); // Bank number = 4 banks
    fmc_sdcr_write_field(1, FMC_SDCR_CAS,   0x02); // CAS latency = 2 cycles
    fmc_sdcr_write_field(1, FMC_SDCR_WP,    0x00); // Write protect disable
    fmc_sdcr_write_field(1, FMC_SDCR_SDCLK, 0x02); // SDRAM clock = HCLK/2
    fmc_sdcr_write_field(1, FMC_SDCR_RBURST,0x01); // Enable burst read
    fmc_sdcr_write_field(1, FMC_SDCR_RPIPE, 0x00); // Read pipe delay = 0

    // Configure FMC_SDTR
    fmc_sdtr_write_field(1, FMC_SDTR_TMRD,  2); // Load to Active delay
    fmc_sdtr_write_field(1, FMC_SDTR_TXSR,  7); // Exit self-refresh delay
    fmc_sdtr_write_field(1, FMC_SDTR_TRAS,  4); // Self refresh time
    fmc_sdtr_write_field(1, FMC_SDTR_TRC,   7); // Row cycle delay
    fmc_sdtr_write_field(1, FMC_SDTR_TWR,   2); // Write recovery time
    fmc_sdtr_write_field(1, FMC_SDTR_TRP,   2); // Row precharge delay
    fmc_sdtr_write_field(1, FMC_SDTR_TRCD,  2); // Row to column delay
}
````

### 5.3.3 依序送出五個 JEDEC 初始化指令至 SDRAM

Clock Configuration Enable

PALL (Precharge All)

Auto-refresh（2 次）

Mode Register 設定

設定 Refresh Rate 計數器