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

# 5. LCD 圖形架構與 LTDC 顯示流程

## 5.1 簡介嵌入式圖形系統

介紹嵌入式圖形系統的基本架構，包括 MCU、LCD、frame buffer、DMA、圖形函式庫等核心組件。

### 5.1.1 圖形應用程式（Graphics Application）

嵌入式圖形應用是一種基於微控制器的互動式系統，會在顯示裝置上呈現以下圖形元素：

1. **色彩（Colours）**  
   - 文字顏色（Color of the text）  
   - 背景色或圖層顏色（Background color / layer color）

2. **形狀（Shapes）**  
   - 方塊（Box）  
   - 圓形（Circle）  
   - 箭頭（Arrows）  
   - 線條（Lines）

3. **圖片（Images）**  
   - 任意圖片（如 BMP、JPG、PNG，需轉碼處理）

4. **文字（Texts）**  
   - 靜態或動態字串顯示

5. **圖形效果（Graphical Effects）**  
   - 捲動、滑動、滑入滑出、按壓與釋放等動畫

6. **元件（Widgets）**  
   - 按鈕、單選鈕、核取方塊等 GUI 控制元件

7. **影片（Videos）**  
   - 僅適用於具備高記憶體與解碼能力的系統

8. **3D 繪圖（3D Rendering）**  
   - 需具備浮點單元（FPU）與高運算資源支援

9. **遊戲應用（Gaming）**  
   - 小型互動遊戲或圖形化操作介面

---

### 5.1.2 嵌入式圖形系統結構（System Architecture） 

#### 基本模組構成

1. **Microcontroller（主控器）**
   - **CPU**：執行應用邏輯與資料處理  
   - **RAM**：儲存 frame buffer、變數與繪圖中間資料  
   - **Flash**：儲存圖片、字型與程式碼等靜態資源  

2. **Interface（通訊介面）**
   - MCU 與顯示器之間的連接介面，常見有：
     - **RGB（DPI）**：高速並列像素資料傳輸
     - **SPI / 串列匯流排（DBI）**：透過指令與資料寄存器控制 LCD
     - **DSI（Display Serial Interface）**：專為顯示而設計的高速串列通訊介面  

3. **Display（顯示器）**
   - 可搭配的顯示裝置類型包括：
     - **LCD**（TFT、IPS）
     - **OLED**
     - **LED**
     - **CRT**
     - **Plasma**

---

#### 核心組成元件說明（Important Components）

**Microcontroller（主控端）**

- **Processor（處理器）**  
  負責執行主程式、處理繪圖邏輯、解碼影像資料，並更新 frame buffer。

- **RAM（作為 frame buffer）**  
  用於儲存目前要顯示的畫面資料，每筆像素資訊（如 RGB565 / RGB888）會由顯示器持續讀取。

- **Flash（快閃記憶體）**  
  儲存靜態資源（圖像、字型、文字等），程式運行期間會載入並轉為像素資料顯示。

**Display Module（顯示模組）**

- **Glass（顯示玻璃）**  
  實際呈現畫面給使用者觀看的區域。

- **Driver Chip（驅動晶片）**  
  解譯 MCU 傳來的訊號，並產生所需的電氣信號與驅動電壓，控制像素發光顯示正確顏色。

---

#### 其他關鍵模組（Other Important Components）

除了基本的 MCU、介面與顯示器外，嵌入式圖形系統還常見以下重要模組：

---

**1. 顯示控制器（Display Controller）**

- 位於 MCU（Host）端  
- 負責產生顯示所需的時序訊號（例如：VSYNC、HSYNC、DE、PCLK）  
- 傳送像素資料至顯示器（TFT-LCD 等）  
- STM32 中常見為 LTDC（LCD-TFT Display Controller）

---

**2. 外部記憶體（External Memories）**

- **外部 Flash**：當內建 Flash 不足時，用於儲存程式碼、圖片、字型等大型資源  
- **外部 RAM**：作為 frame buffer 或中介繪圖資料使用，尤其在內建 RAM 不足時必須擴充

---

**3. 圖形函式庫（Graphics Library）**

- 常見如 **LVGL**、**TouchGFX**  
- 提供高階 API，可快速繪製元件、動畫、圖層等 GUI 元素  
- 抽象化硬體操作，提升開發效率並支援多樣化 UI 效果

---

**4. UI 設計工具（UI Designer Tool）**

- 與圖形函式庫整合的視覺化工具  
- 可透過拖拉方式設計互動式介面，快速建立 GUI 佈局與行為邏輯  
- 範例：LVGL Studio、TouchGFX Designer

---

**5. 觸控模組（Touch Sensing）**

- **觸控面板（Touch Panel）**：感應使用者的觸控行為（座標）  
- **觸控控制器（Touch Controller）**：接收面板訊號，轉換為數位資訊並通知主控端（MCU）

---

**6. DMA（Direct Memory Access）**

- 將資料從 Flash 複製至 frame buffer 或從 frame buffer 傳送到顯示器  
- 可不經過 CPU 干預，提升效率與顯示即時性  
- 在高頻刷新（如 60Hz）場景中特別重要，可避免 CPU 負載過高

---

### 5.1.3 圖像渲染流程（Rendering Pipeline）

Frame buffer 是一塊專用記憶體區域，用來儲存「目前要顯示的影像畫面」的像素資料。  
每一筆像素資料包含顏色資訊（如 RGB888），顯示器會依照畫面刷新頻率（如 60Hz）連續掃描該區域，並將畫面輸出到螢幕上。

嵌入式圖形系統中，顯示影像的典型資料流程如下：

```
Flash（圖片儲存） 
  ↓
CPU（渲染處理，轉為像素）
  ↓
RAM（frame buffer）
  ↓
Interface（如 RGB、SPI）
  ↓
Display（顯示畫面）
```

也就是說，**MCU 同時扮演控制器與圖形渲染引擎**的角色，需完成圖片讀取、像素渲染與畫面輸出。

---

## 5.2 嵌入式 LCD 顯示開發平台與架構總覽

---

### 5.2.1 常見的 LCD 開發板選項

#### Option 1：32F429IDISCOVERY  
搭載 STM32F429ZI MCU

- 板子上已內建 LCD 螢幕（直接連接）  
- ST 官方推薦用來學習圖形顯示開發  
- 提供 Mini USB 介面，可與電腦連接進行開發與燒錄  
- 推薦使用（Recommended）

#### Option 2：32F746GDISCO  
搭載 STM32F746NG MCU

- 螢幕與主板分離（模組化設計）  
- 支援高解析度顯示與觸控功能  
- 功能更強、資源更豐富，但開發相對複雜

#### Option 3：STM32F7508-DK  
搭載 STM32F750N8 MCU

- 採用模組化 LCD 設計  
- 支援圖形顯示與觸控功能  
- 成本較低，功能略少於 Option 2

#### Option 4：STM32F4DISCOVERY + External LCD  
搭配 LCD Shield 使用

##### 開發板與螢幕連接方式
- STM32F4DISCOVERY 開發板 + FASTBIT 32F407DISC IoT-LCD Shield  
- 透過排針插座接上 Shield  
- LCD 為外接模組，並可使用 Mini USB 與電腦連接  

##### 額外顯示方式（另一種組法）
- 可改用 SPI 介面直接連接 LCD（不使用擴充板）  
- 使用 2.4 吋 SPI 介面觸控 TFT 模組（240x320）

#### Option 5：NUCLEO-F4x STM32 Nucleo-64  
開發板型號

##### 顯示模組連接方式
- 透過 SPI 介面連接外接 LCD 模組

##### 顯示模組資訊
- 2.4 吋 SPI Interface  
- 240x320 Touch Screen TFT  
- 彩色顯示模組（Colour Display Module）

#### 各開發板圖形顯示能力比較表

| Option | 開發板名稱         | MCU           | Core       | RAM (KiB) | Flash | SDRAM     | 外部 Flash | LCD 控制器 | Chrom-ART | LCD 螢幕 |
|--------|--------------------|---------------|------------|-----------|--------|-----------|-------------|--------------|-------------|------------|
| 1      | 32F429IDISCOVERY   | STM32F429ZI   | Cortex-M4  | 256       | 2 MiB  | Yes (64Mbit) | No          | Yes          | Yes         | Yes 240×320 |
| 2      | 32F746GDISCO       | STM32F746NG   | Cortex-M7  | 340       | 1 MiB  | Yes (128Mbit) | Yes (128Mbit) | Yes        | Yes         | Yes 480×272 |
| 3      | STM32F7508-DK      | STM32F750N8   | Cortex-M7  | 340       | 64 KiB | Yes (128Mbit) | Yes (128Mbit) | Yes        | Yes         | Yes 480×272 |
| 4      | STM32F4DISCOVERY   | STM32F407VG   | Cortex-M4  | 192       | 1 MiB  | No          | No          | No           | No          | No         |
| 5      | NUCLEO-F4x         | ??            | Cortex-M4  | ??        | ??     | No          | No          | No           | No          | No         |

### 5.2.2 圖形展示實作架構與推薦平台

以下列出幾個可用於實作圖形展示專案的開發板或組合，包含內建 LCD 的開發板與透過 SPI 外接顯示模組的解決方案。

---

#### 32F429IDISCOVERY  
**開發板名稱：** 32F429IDISCOVERY Discovery kit

**MCU 內部模組：**
- Cortex-M4 核心：負責執行應用程式邏輯  
- SRAM：儲存畫面資料（可配合外部 SDRAM 使用）  
- FLASH：儲存程式碼與靜態資源  
- LTDC（LCD-TFT Display Controller）：控制圖形資料輸出至 LCD

**LCD 顯示模組（240x320 TFT）：**
- LCD Driver Chip：處理像素資料並驅動面板  
- RGB display：實際顯示畫面的 TFT 彩色螢幕

**開發板與 LCD 的連接介面：**

| 類型             | 說明                                               |
|------------------|----------------------------------------------------|
| Display signals  | 22 Pins，輸出 RGB 像素資料，由 LTDC 控制         |
| SPI 接口         | 雙線 SPI，用於設定 LCD 驅動晶片（非畫面資料）     |
| 控制訊號         | 4 條線，例如 RESET、CS、WR、RD 等控制腳位         |

---

#### 32F746GDISCOVERY  
**開發板名稱：** 32F746GDISCOVERY  
**MCU：** STM32F746NG（Cortex-M7）

**MCU 內部模組：**
- Arm Cortex-M7 核心  
- SRAM  
- FLASH  
- LTDC（LCD-TFT Display Controller）

**與 LCD 之間的介面：**
- 使用 **40-pin 並列 RGB 介面** 連接 LCD 模組

**顯示模組構成：**
- LCD Driver Chip（顯示驅動 IC）  
- RGB Display 面板（解析度：480x272）

---

#### 32F407DISCOVERY + SPI-based LCD module（FASTBIT LCD Shield）  
**開發板名稱：** STM32F407 DISC-1

**LCD 模組介面：**
- 使用 **SPI 傳輸命令與 RGB 畫面資料**

**額外控制訊號：**
- 包含 RESET、CS、DC 等常見控制腳位

**接線參考：**
| LCD 腳位   | 功能說明     |
|------------|--------------|
| SDI (MOSI) | 資料輸入     |
| SCK        | SPI 時脈     |
| DC_RS      | 資料/命令切換 |
| CS         | 裝置選擇     |
| RESET      | 硬體重置     |
| GND / VCC  | 電源與接地   |

---

## 5.3 像素格式與圖像資料儲存原理

### 5.3.1 RGB 顯示器與色彩模型介紹

RGB 顯示器是一種能夠接收 **紅（Red）**、**綠（Green）**、**藍（Blue）** 三原色訊號的顯示裝置。  
這三個顏色的強度組合決定了螢幕上每一個像素所顯示的顏色。

**RGB 色彩模型（RGB Color Model）**  
RGB 是一種 **加法色彩系統（Additive Color Model）**，透過疊加不同強度的紅、綠、藍光，  
可產生多種顏色，包括白色、黃色、洋紅、青色等。

---

### 5.3.2 圖形顯示常用術語簡介

以下是與嵌入式 LCD 圖形顯示相關的常見術語：

- **Pixel（像素）**  
  螢幕上最小的顯示單位，每個像素可單獨顯示一種顏色。

- **Pixel Density（像素密度，PPI：Pixels Per Inch）**  
  每英吋所包含的像素數量，用來衡量畫面的精細程度。

- **Pixel Color Depth（色彩深度，Bit Depth）**  
  每個像素所能表示的色彩位元數，例如 16-bit（RGB565）、24-bit（RGB888）。

- **Pixel Format（像素格式）**  
  定義像素資料在記憶體中的排列方式，例如 RGB565、ARGB8888 等格式。

- **Resolution（解析度）**  
  顯示器的寬度與高度的像素數，例如 240×320 表示寬 240 像素、高 320 像素。

---

### 5.3.3 Pixel（像素）

**定義：**  
Pixel（Picture Element）是圖像中最小的單位，代表一個獨立的顏色資訊。每個像素的色彩資料會儲存在記憶體中，圖像中的每個 pixel 對應一筆記憶體資料，用來構成整張圖像。

#### 實例說明：

以一張解析度為 **480×270** 的彩色圖片為例：

- 寬度：480 像素  
- 高度：270 像素  
- 每個像素具有獨立的色彩值  
- Bit Depth：32-bit（每個像素使用 32 位元儲存顏色資訊）

當圖片放大後，可清楚看到由像素格（Pixel Grid）構成的圖像，其中每一格即為一個像素。

#### Pixel 色彩組成：

每個像素通常由下列三個主要色光值構成：

- R（紅色強度）
- G（綠色強度）
- B（藍色強度）

範例：

- R = 255, G = 0, B = 0 → 顯示為「純紅色」

不同的 R/G/B 組合可呈現出多種顏色，組合成最終的畫面。

#### 像素在記憶體中的排列：

圖像實際上是由數百至數百萬個像素以 **網格（Grid）** 形式排列而成。  
每個像素儲存其 RGB 色彩值，讓螢幕能夠準確地重建原始影像。

#### 像素佔用的記憶體大小

每個 Pixel 佔用多少記憶體，取決於其 **像素格式（Pixel Format）** 或 **色彩深度（Color Depth）**。  
這通常以 **bpp（bits per pixel）** 表示，例如：

- **16 bpp**：每個像素佔用 2 Bytes，例如 `RGB565`
- **24 bpp**：每個像素佔用 3 Bytes，例如 `RGB888`
- **32 bpp**：每個像素佔用 4 Bytes，例如 `ARGB8888`（含透明度 Alpha）

#### 常見 Pixel Formats（Color Formats）

| Pixel Format   | 說明（Meaning）                                              |
|----------------|--------------------------------------------------------------|
| **ARGB8888**   | 32 bits；含 Alpha 通道；RGB 各 8 bits                         |
| **RGB888**     | 24 bits；每個顏色 8 bits；不含 Alpha                         |
| **RGB565**     | 16 bits；R=5 bits, G=6 bits, B=5 bits；不含 Alpha            |
| **ARGB1555**   | 16 bits；1 bit Alpha + 5 bits R/G/B                          |
| **ARGB4444**   | 16 bits；每通道 4 bits（含 Alpha）                           |
| **RGB666**     | 16 bits；每通道 6 bits；不含 Alpha                           |
| **ARGB2222**   | 8 bits；每通道 2 bits（含 Alpha）                            |
| **ABGR2222**   | 8 bits；每通道 2 bits（Alpha + BGR）                         |
| **L8**         | 8-bit 灰階亮度（或 LUT 色彩查表索引）                         |
| **L8_RGB888**  | 8-bit 索引對應 RGB888 查表色彩；LUT size = 24               |
| **L8_RGB565**  | 8-bit 索引對應 RGB565 查表色彩；LUT size = 16               |
| **AL44**       | 4-bit Alpha + 4-bit Luminance（亮度）                         |
| **AL88**       | 8-bit Alpha + 8-bit Luminance（亮度）                         |
| **L4**         | 4-bit 灰階亮度                                               |
| **A8**         | 8-bit Alpha；無 RGB                                           |
| **A4**         | 4-bit Alpha；無 RGB                                           |
| **GRAY4**      | 4-bit 灰階強度值                                             |
| **GRAY2**      | 3-bit 灰階強度值                                             |
| **BW**         | 1-bit 黑白顯示（Black & White）                               |

---

### 5.3.4 常見像素格式與應用場景

#### ARGB8888 像素格式說明

若圖像採用 **ARGB8888** 像素格式，代表每個像素會佔用 **32 bits（4 Bytes）** 記憶體空間，  
並包含以下 **4 個分量（Component）**，每個分量各佔 8 bits：

1. **Alpha（透明度）**：8 bits，用來表示像素的不透明程度（Opacity）
2. **Red（紅色）**：8 bits，表示紅色強度
3. **Green（綠色）**：8 bits，表示綠色強度
4. **Blue（藍色）**：8 bits，表示藍色強度

> 記憶體中每個像素以 ARGB 的順序排列（從高位元到低位元）：  
> `0xAARRGGBB`

例如：
- `0xFFFF0000` ➝ 完全不透明的純紅色（Alpha=255, R=255, G=0, B=0）

此格式常用於圖層合成與 GUI 開發，可支援透明效果與豐富色彩。

#### 灰階色彩（Grayscale）與 B&W 顯示比較

##### GRAY8 格式說明

- GRAY8 使用 8 bits 來表示每個像素的亮度值。
- 色彩強度從 0（黑）到 255（白）連續變化。
- 當 `R = G = B` 時，該像素即為灰階顏色。

##### 不同顯示模式對照：

| 顯示模式     | 特徵與應用 |
|--------------|------------|
| **True Color** | RGB 每通道各 8 bits，可呈現豐富真實色彩（如 ARGB8888） |
| **Grayscale**  | 僅儲存亮度資訊，通常使用 GRAY8；常見於影像處理與低功耗顯示 |
| **B&W**        | 每個像素僅 1 bit（黑或白），適用於文字顯示、點陣 LCD |

> B&W 不支援陰影與漸層；Grayscale 則能呈現更細膩的亮度差異。

#### L8 格式的記憶體配置與應用優勢

**L8** 是一種 **索引型像素格式（Indexed Pixel Format）**，  
每個像素僅佔用 **8 bits（1 Byte）**，但不直接儲存 RGB 色彩值。  
相反地，這 8-bit 整數是用來查找預先定義的 **色彩查找表（CLUT, Color Look-Up Table）** 中的對應顏色。  
CLUT 最多支援 256 筆顏色，每筆通常為 **24-bit（RGB888）** 或 **32-bit（ARGB8888）**。

##### 記憶體用量比較（以解析度 480 × 270 為例）

- **使用 RGB888 格式：**
  - 每像素：3 Bytes  
  - 總記憶體使用量：480 × 270 × 3 = **388,800 Bytes ≈ 380.5 KiB**

- **使用 L8 格式 + RGB888 查表：**
  - 像素索引資料區：480 × 270 × 1 = **129,600 Bytes**
  - 色彩查表（CLUT）：256 × 3 = **768 Bytes**
  - 總記憶體使用量：約 129,600 + 768 = **130,368 Bytes ≈ 127.3 KiB**

> **L8 格式大幅降低記憶體用量**，特別適用於色彩種類有限、資源受限的嵌入式應用場景。

##### 記憶體用量與格式比較表

| 項目                 | RGB888 格式           | L8 格式 + RGB CLUT         |
|----------------------|------------------------|------------------------------|
| 每像素大小           | 24 bits（3 Bytes）     | 8 bits（1 Byte）             |
| 色彩儲存方式         | 每個像素儲存 RGB 值    | 每個像素儲存查表索引值       |
| 額外記憶體需求       | 無                     | 需額外 CLUT 區域（最多 256 筆） |
| 查表結構             | 無                     | `CLUT[256] × 3 Bytes`        |
| 總記憶體（480×270）  | 約 380.5 KiB           | 約 127.3 KiB                 |
| 適用情境             | 真實色彩圖像（照片）   | 色彩種類有限的 GUI、圖示等   |

> L8 格式常見於嵌入式顯示系統，如圖形使用者介面（GUI）、圖示、選單等，能在有限記憶體下保有基本色彩效果。

---

### 5.3.5 顯示器的像素密度（Pixel Density, PPI）

#### PPI 介紹

- **PPI（Pixels Per Inch）** 是衡量顯示器解析度的單位，表示每英吋所擁有的像素數。
- PPI 越高，表示單位面積內像素越多，畫面越細膩。
- 高 PPI 可提升圖片與文字顯示的清晰度與閱讀舒適度。

#### 視覺範例：

- 如果 PPI = 8，表示在 1×1 英吋區域內排入 8×8 = 64 個像素。
- 現代手機、平板等裝置通常具有 300 PPI 以上的高像素密度。

#### 如何計算 PPI？

若顯示器解析度為 **W × H 像素**，對角尺寸為 **D 吋**，則：

```text
PPI = √(W² + H²) / D
```

##### 範例：
- 顯示尺寸：5.8 吋
- 解析度：2280 × 1080
- 計算：
  ```text
  PPI = √(1080² + 2280²) / 5.8 ≈ 438
  ```

#### PPI 越高的優勢：

- 更細緻的圖像與文字呈現
- 小尺寸裝置也能顯示高解析度內容
- 適合閱讀、影片、照片與高品質 GUI

> **補充：** DPI（Dots Per Inch）常用於列印領域，而 PPI 用於顯示裝置。

---

### 5.3.6 Alpha 通道與透明度控制（Opacity）

#### Alpha Component 介紹

Alpha 是像素中的「透明度（Opacity）」成分，常見於 ARGB 格式。  
它是一個可選的分量，用來控制像素與背景色的融合程度。

- 若採用 8-bit Alpha，透明度數值範圍為 0 至 255：
  - 255 表示完全不透明（Fully Opaque）
  - 0 表示完全透明（Fully Transparent）

具有 Alpha 值的像素格式，稱為 ARGB 色彩格式。

#### 透明度對照示意（以圖像為例）

| Alpha 數值 | 對應透明度 | 顯示效果       |
|------------|-------------|----------------|
| 0          | 0%          | 完全透明       |
| 64         | 25%         | 輕微透明       |
| 128        | 50%         | 半透明         |
| 192        | 75%         | 幾乎不透明     |
| 255        | 100%        | 完全不透明     |

透明度越高，像素越不受背景影響；透明度越低，背景越容易透出。

#### 補充說明

- 當圖片使用 ARGB8888 格式時，每個像素包含 4 個分量：Alpha、Red、Green、Blue，各佔 8 bits。
- 透明效果常見於 GUI 介面、圖層疊加、動態淡入淡出等場景。

### 5.3.7 BMP 與 JPEG 圖片格式比較與記憶體估算

#### 記憶體估算（以 ARGB8888 與 480×270 為例）

- 每像素佔用：32 bits（ARGB8888）

計算公式：

```
記憶體 = Width × Height × (bpp ÷ 8)
       = 480 × 270 × (32 ÷ 8)
       = 518,400 bytes ≈ 506.25 KiB
```

#### BMP（Bitmap）圖像格式特性

- 為 **未壓縮格式（Uncompressed）**，直接儲存每個像素的原始色彩資料。
- 因為資料未壓縮，檔案較大，但方便直接顯示到螢幕。
- BMP 檔案由 **檔頭（Header）+ 像素資料（Pixel Data）** 組成。

#### JPEG 圖像格式特性

- JPEG 為 **壓縮格式（Compressed）**，體積小但無法直接顯示。
- MCU 上若要顯示 JPEG，需先經過「JPEG 解碼器」還原成像素資料。
- 可使用 **MCU 的 JPEG 硬體外設** 或 **中介軟體（middleware）** 進行解碼。
- 解碼後的資料為 bitmap 格式，才可傳送給顯示模組。

---

## 5.4 STM32F429 高效能時脈架構與初始化流程

### 5.4.1 時脈架構

在 STM32F429 中，系統主時脈來源為 **SYSCLK**，其最大頻率為 **180 MHz**。該時脈依序經過下列分頻機制後，提供給不同匯流排：

- **AHB Prescaler**  
  輸出為 **HCLK**，即 AHB bus 的時脈，最大為 **180 MHz**  
  → 由 **SYSCLK** 分頻而來

- **APB1 Prescaler**  
  輸出為 **PCLK1**，供 APB1 匯流排（如 USART2、TIM2 等），最大為 **45 MHz**  
  → 由 **HCLK** 分頻而來

- **APB2 Prescaler**  
  輸出為 **PCLK2**，供 APB2 匯流排（如 USART1、TIM1、LTDC 等），最大為 **90 MHz**  
  → 由 **HCLK** 分頻而來

這些時脈設定對 LCD 顯示功能至關重要，因為：

- **LTDC（LCD-TFT 顯示控制器）**
- **DMA2D（圖像加速器）**

等模組通常依賴 AHB 或 APB 匯流排提供的時脈作為操作基準。若設定不當，可能導致畫面閃爍、顯示異常或無法驅動面板。

**建議設定組合（高效能應用）**

| 匯流排   | 時脈目標值 | 對應預除器  | 原始來源 |
|----------|-------------|----------------|----------|
| HCLK     | 180 MHz      | SYSCLK ÷ 1     | SYSCLK   |
| PCLK1    | 45 MHz       | HCLK ÷ 4       | HCLK     |
| PCLK2    | 90 MHz       | HCLK ÷ 2       | HCLK     |

此組配置為 STM32F429 最常見且安全的高效能設定，亦為 STM32CubeMX 等初始化工具的預設輸出結果，特別適合應用於需高匯流排頻寬的場景，如 FMC SDRAM、LTDC 與 DMA2D 同時運作。

---

### 5.4.2 系統主時脈（SYSCLK）初始化流程

為讓 STM32F429 可穩定以高頻運作，並正確提供各匯流排與外設所需時脈（尤其是與圖像處理相關的模組如 LTDC、DMA2D、FMC SDRAM 等），需依下列步驟完成主時脈來源（SYSCLK）的初始化：

1. 設定主 PLL（Phase Locked Loop）參數（M、N、P 值），決定 PLL 輸出頻率。
2. 設定 PLLSAI（另一組獨立的 PLL），主要用於 LCD、音訊等外設。
3. 設定各匯流排的預除器（AHB / APB1 / APB2）。
4. 啟用主 PLL 模組。
5. 等待 PLL 鎖定完成，確認 **PLLRDY** 位元為 1。
6. 將 PLL 輸出選作系統主時脈（SYSCLK）。
7. 等待時脈切換完成，確認系統已使用 PLLCLK 為 SYSCLK。
8. 設定 Flash 訪問等待週期（Flash wait state）以符合高 HCLK 存取需求。

> 說明：根據 CPU 時脈（HCLK）與供電電壓，需在 FLASH_ACR 寄存器中正確設定 LATENCY 位元，否則可能導致 Flash 讀取錯誤。

9. 啟用 PLLSAI。
10. 等待 PLLSAI 鎖定完成，確認 **PLLSAIRDY** 位元為 1。

---

### 5.4.3 Flash 存取延遲設定（FLASH Read Latency）

在 STM32F429 系列中，當系統主時脈（SYSCLK）與 CPU 時脈（HCLK）提高時，必須對內部 Flash 設定正確的「存取延遲（Wait States）」，以確保 MCU 能正確讀取 Flash 資料，避免硬體錯誤或系統不穩。

Flash 存取延遲由 `FLASH_ACR` 暫存器中的 `LATENCY` 欄位控制，代表 MCU 存取內部 Flash 時需等待的 CPU 週期數。

#### 為什麼需要 Flash Wait States？

內部 Flash 的反應速度固定，當 HCLK 過高時，資料尚未完成準備就被 CPU 存取，將導致：

- 程式執行錯誤
- 硬體中斷（HardFault）
- MCU 無預警重啟或系統當機

因此，**必須依據供應電壓（V<sub>DD</sub>）與 HCLK 頻率，設定適當的等待週期**。

> 請參考 RM0090 Reference Manual 第 3.5 節  
> Table 12 – *Number of wait states according to CPU clock (HCLK) frequency*  
> 適用於 STM32F42xxx 和 STM32F43xxx 系列

#### 設定步驟

1. **查詢目前系統的 HCLK 頻率與供應電壓 V<sub>DD</sub>**
2. **查表取得對應的 Wait States 數值（WS）**
3. **將該數值寫入 `FLASH->ACR` 暫存器中的 `LATENCY` 欄位**

```c
// 例：180 MHz HCLK，VDD = 3.3V（屬於 2.7V–3.6V 區間）
// 需設定為 5 WS（等於 6 個 CPU 週期延遲）
FLASH->ACR = (5 << FLASH_ACR_LATENCY_Pos);
```

#### 注意事項

- **開機預設為 HSI 16 MHz，對應 LATENCY = 0，無需等待**
- 一旦使用 PLL 拉高 HCLK，**必須先行設定 Wait States**
- 若在未設定延遲前就啟用高 HCLK，會導致系統執行錯誤

---

### 5.4.4 Over-drive 模式啟用（以支援 180 MHz 運作）

在 STM32F429 系列中，若欲讓系統時脈（HCLK）運作於最高的 180 MHz，**必須啟用 Over-drive 模式**。該模式會讓內部電源調節器提供更高壓力，支援高時脈穩定運作。

STM32 提供三種 Power Scale 等級（VOS），依據設定等級與是否啟用 Over-drive 模式，可對應不同的 HCLK 上限值。

| Power Scale | VOS 設定 | Over-drive 狀態 | HCLK 最大值 |
|-------------|----------|------------------|--------------|
| Scale 3     | 0b01     | 無法使用         | 120 MHz      |
| Scale 2     | 0b10     | OFF / ON         | 144 / 168 MHz|
| Scale 1     | 0b11     | OFF / ON         | 168 / **180 MHz** ✅ |

> 建議使用 Power Scale 1 搭配 Over-drive 模式，為 STM32F429 達成 180 MHz 運行的必要條件。

---

## 5.5 LCD 顯示初始化與 LTDC/SPI 控制流程

### 5.5.1 LTDC 與 LCD 顯示控制概念

LTDC（LCD-TFT Display Controller）負責將 MCU 記憶體中的畫面像素資料（frame buffer）持續地轉換為 RGB 訊號，並傳送到 LCD 模組。它就像「畫布更新器」，會自動掃描 SDRAM 中的畫面內容，將其送至 LCD 面板顯示。

> 前提條件：LCD 模組必須先透過 **SPI** 正確初始化，否則即使 LTDC 傳出資料，LCD 也無法顯示任何畫面。

LCD 顯示模組內建如 **ILI9341**、**ST7789** 等 LCD 驅動晶片，這些晶片在開機後需要 MCU 經由 **SPI 介面**傳送指令來完成初始化動作。

常見初始化設定包含：

- 設定顯示方向（橫向 / 直向）
- 開啟顯示功能（Display ON）
- 設定顏色格式、偏壓、掃描方向等

若未透過 SPI 完成這些設定，LCD 模組將無法進入工作狀態，即使 LTDC 傳送畫面資料也不會顯示內容。

此外，**SPI 有時也可作為畫面資料傳輸介面**，例如當沒有使用 LTDC 或 RGB 硬體輸出時，SPI 亦可直接傳送像素資料給 LCD（但效能較低）。

> 例如：STM32F407x 系列 + 外接 SPI 顯示模組，就以 SPI 同時完成初始化與畫面資料傳輸。

---

### 5.5.2 ILI9341 顯示模組介面概觀（STM32F429 DISC）

STM32F429 驅動 ILI9341 LCD 的過程可分為兩個階段：

#### Programming Interface（設定階段）

使用 GPIO + SPI 傳送控制指令給 ILI9341，使 LCD 進入可用狀態。

| STM32 腳位 | ILI9341 腳位功能             | 類型   |
|------------|-------------------------------|--------|
| PA7        | RESX（重置）                   | GPIO   |
| PC2        | CSX（片選）                    | GPIO   |
| PD13       | WRX_D/CX（資料 / 指令選擇）     | GPIO   |
| PF9        | SDI / SDA（SPI MOSI）          | SPI5   |
| PF7        | D/CX_SCL（SPI CLK）            | SPI5   |

#### Data Interface（顯示階段）

使用 LTDC 輸出 RGB 介面資料，將 frame buffer 中的內容實際畫出來顯示於 LCD 上。

---

### 5.5.3 GPIO 腳位設定為 SPI 功能模式

若 MCU 使用 SPI 功能，對應的 GPIO 腳位必須設定為 Alternate Function（替代功能）模式，設定步驟如下：

#### 設定步驟

1. **將指定的 GPIO 腳位設為 Alternate Function 模式**
2. **設定對應的 Alternate Function 編號（AF 編號）**
   - 可從 STM32 的 datasheet 查表對照
3. **不需設定 Pull-up / Pull-down**
   - SPI 傳輸不需要內部上拉或下拉電阻，保持浮接狀態即可

### SPI 設定參數建議

| 設定項目       | 建議值與說明                                                                 |
|----------------|------------------------------------------------------------------------------|
| **SPI 模式**   | Half-duplex Controller（單向傳輸，MCU 為控制端）                            |
| **資料格式**   | 8-bit，MSB first（高位元先傳）                                               |
| **CPOL / CPHA**| 依據 LCD 模組 datasheet（如 ILI9341）設定正確的時脈極性與相位               |
| **SPI 時脈**   | 小於 6 MHz（根據模組最大支援頻率設定）                                       |
| **片選控制**   | 由軟體手動控制（Chip Select handled by software）                           |

---

### 5.5.4 LCD 顯示初始化流程（STM32F429 DISC）

1. **設定主系統時脈（System Clock）**
2. **設定 AHB 與 APB 匯流排時脈**
3. **設定像素時脈（DOTCLOCK）**
4. **啟用與設定 SPI 周邊模組**
5. **配置 framebuffer 於 RAM 中**
6. **透過 SPI 傳送 LCD 初始化指令**
   - 初始化 ILI9341 或其他顯示控制器
7. **初始化與啟用 LTDC 模組**
8. **將 framebuffer 畫面資料透過 LTDC 傳送至 LCD**
9. **確認中斷已正確啟用，並實作 ISR（中斷服務函式）**

---

### 5.5.5 LTDC 設定步驟（基本三階段）

1. **LTDC 腳位初始化**  
   根據開發板的電路圖（schematic），確認哪些 GPIO 腳位作為 LTDC 專用訊號，並設定為對應的 Alternate Function 模式。

2. **LTDC 周邊模組初始化**  
   設定 LTDC 控制器本體的參數，如同步極性、總時脈週期、顯示面積等。

3. **LTDC 圖層初始化**  
   初始化圖層（layer 1 或 layer 2，或兩者皆用），包括：
   - 畫面緩衝區（framebuffer）起始位址
   - 畫面尺寸
   - 顯示格式（RGB565、ARGB8888 等）
   - 圖層混合設定（若使用多層）

---

### 5.5.6 LTDC 信號腳位總覽（LCD-TFT signals）

| LCD-TFT 信號   | I/O | 說明                             |
|----------------|-----|----------------------------------|
| **LCD_CLK**     | O   | 畫素時脈輸出（Pixel Clock）       |
| **LCD_HSYNC**   | O   | 水平同步訊號（Horizontal Sync）   |
| **LCD_VSYNC**   | O   | 垂直同步訊號（Vertical Sync）     |
| **LCD_DE**      | O   | 數據有效訊號（Data Enable）       |
| **LCD_R[7:0]**  | O   | 8-bit 紅色資料輸出（Red）         |
| **LCD_G[7:0]**  | O   | 8-bit 綠色資料輸出（Green）       |
| **LCD_B[7:0]**  | O   | 8-bit 藍色資料輸出（Blue）        |

> 所有訊號皆為輸出（Output），由 MCU 控制並傳送至 LCD 顯示模組。

---

### 5.5.7 STM32F429 LTDC 對應 18-bit 顯示器（RGB565）接法說明

STM32F429 DISC 開發板內建的 LTDC（LCD-TFT Display Controller）與 LCD 面板之間透過 18 條 RGB 資料線連接，對應如下：

| 顏色通道 | 使用 LTDC 輸出腳位（STM32F429） | 備註                        |
|----------|----------------------------------|-----------------------------|
| **Red**  | R0 ~ R5（共 6 條線）             | 對應 LCD R5 ~ R0           |
| **Green**| G0 ~ G5（共 6 條線）             | 對應 LCD G5 ~ G0           |
| **Blue** | B0 ~ B5（共 6 條線）             | 對應 LCD B5 ~ B0           |

- 合計使用 **18 條 RGB 資料線**（6-bit R + 6-bit G + 6-bit B）。
- LCD 使用的是 **RGB565 模式**，與 24-bit RGB（888）不同。
- STM32 LTDC 模組支援 24-bit 輸出，因此需特別設定為 16-bit RGB565 模式。

---

## 5.6 LCD 顯示控制與 LTDC 圖層機制解析

### 5.6.1 LCD 同步時序參數與解析（以 ILI9341 為例）

#### 同步時序示意圖（參考 ILI9341 顯示驅動器）

下圖為典型 LCD 顯示的同步時序，主要包含以下區段：

- 水平同步（HSYNC）
- 垂直同步（VSYNC）
- 水平與垂直消隱區（Front/Back Porch）
- 有效顯示區域（Active Area）

對應計算公式如下：

```
Total Width（總像素數）  = Hsync + HBP + Active Width + HFP  
Total Height（總列數）   = Vsync + VBP + Active Height + VFP
```

> 實際參數需依照所用 LCD Driver（如 ILI9341）之 datasheet 設定。

#### ILI9341 時序參數建議值（解析度 240 x 320）

| 參數項目                    | 符號  | 建議值 | 單位     |
|-----------------------------|--------|--------|----------|
| 水平同步寬度               | Hsync  | 10     | DOTCLK   |
| 水平後消隱區（Back Porch）| HBP    | 20     | DOTCLK   |
| 有效寬度（Active Width）   | HAddr  | 240    | DOTCLK   |
| 水平前消隱區（Front Porch）| HFP    | 10     | DOTCLK   |
| 垂直同步寬度               | Vsync  | 2      | Line     |
| 垂直後消隱區（Back Porch）| VBP    | 2      | Line     |
| 有效高度（Active Height）  | VAddr  | 320    | Line     |
| 垂直前消隱區（Front Porch）| VFP    | 4      | Line     |

> 此組參數適用於 240×320（QVGA）顯示器，像素時脈約 6.25 MHz。可依實際需求調整。

#### 名詞補充

- **DOTCLK**：像素時脈（Pixel Clock），每個 clock 傳輸一個像素資料
- **Line**：指一列像素的掃描時間（包含消隱區）
- **Active Area**：實際可見的顯示範圍
- LTDC 模組需根據這些時序參數產生正確的 RGB 信號

---

### 5.6.2 顯示週期計算與更新率（以 QVGA 為例）

#### 一、參數定義

- AW：Active Width，有效顯示寬度（像素）
- AH：Active Height，有效顯示高度（行）
- HSW：Horizontal Sync Width，水平同步脈波寬度
- HBP：Horizontal Back Porch，水平後消隱區
- HFP：Horizontal Front Porch，水平前消隱區
- VSW：Vertical Sync Width，垂直同步脈波寬度
- VBP：Vertical Back Porch，垂直後消隱區
- VFP：Vertical Front Porch，垂直前消隱區
- T_DOTCLK：每一個像素的傳輸時間（秒）

#### 二、基本計算公式

1. **總寬度（TW）**  
   `TW = AW + HSW + HBP + HFP`（單位：像素）

2. **傳送一列所需時間（T_line）**  
   `T_line = TW × T_DOTCLK`（單位：秒）

3. **總列數（TL）**  
   `TL = AH + VSW + VBP + VFP`

4. **傳送一幀所需時間（T_frame）**  
   `T_frame = TL × T_line`

5. **畫面更新率（Frame Rate）**  
   `FrameRate = 1 / T_frame`

#### 三、範例計算（ILI9341 + QVGA）

已知參數如下：

```
DOTCLK = 6.25 MHz → T_DOTCLK = 1 / 6.25 MHz = 0.16 μs
AW  = 320
HSW = 10
HBP = 20
HFP = 10
AH  = 240
VSW = 2
VBP = 4
VFP = 2
```

代入公式計算：

```
TW = 320 + 10 + 20 + 10 = 360 pixels
TL = 240 + 2 + 4 + 2 = 248 lines
T_line  = 360 × 0.16 μs = 57.6 μs
T_frame = 248 × 57.6 μs = 14.3 ms
FrameRate ≈ 1 / 0.0143 ≈ 69.9 Hz
```

- 每幀畫面顯示時間約為 14.3 毫秒  
- 螢幕更新率約為 69.9 Hz，適合大多數 LCD 顯示需求  
- 若需更高更新率，可考慮調高 DOTCLK 或減少時序參數

---

### 5.6.3 LTDC 圖層（Layer）架構與顯示原理

#### 圖層特性與控制機制

LTDC（LCD-TFT Display Controller）內建 **兩個圖層（Layer 1 與 Layer 2）**，可靈活應用於不同顯示需求。其主要特性如下：

- 支援 **獨立或同時啟用兩個圖層**
- 每個圖層可設定：
  - **預設顏色（Default Color）**
  - **對應的 Framebuffer 起始位址**
  - **視窗位置與大小（Windowing）**
- 支援 **Alpha 混合（Alpha Blending）**
  - 可指定 **常數透明度（Constant Alpha）**
  - 或使用像素內建透明度（Pixel Alpha）
- 大部分圖層控制暫存器屬於 **Shadow Registers**
  - 寫入後需額外觸發 **Shadow Reload** 才會生效

#### 顯示合成原理

LTDC 顯示畫面由三個區域組成：

1. **Background Area**：背景顏色區，為畫面預設填色（如：藍色）
2. **Layer 1**：可顯示第一層圖像（例：左側黃色區塊）
3. **Layer 2**：可顯示第二層圖像（例：右側黑色區塊）

顯示流程採 **自上而下（Top-Down）合成**，圖層優先順序如下：

```
Layer 2 → Layer 1 → Background
```

圖層行為說明：

- 若僅啟用 Layer 1：
  - 顯示左側黃色畫面 + 其餘區域為背景色（藍色）
- 若同時啟用 Layer 1 與 Layer 2：
  - 左側顯示黃色、右側顯示黑色
  - 背景顏色會被兩層圖像覆蓋，不再顯示

#### 混合（Blending）選項

- **啟用混合（Blending Enable）**：
  - LTDC 依據各圖層的 Alpha 值進行色彩混合與透明度疊加
  - 可實現漸層、透明、淡入淡出等效果

- **關閉混合（Blending Disable）**：
  - 每層圖像以不透明方式覆蓋下層畫面（無透明度效果）

---

### 5.6.4 LTDC 混色機制（Blending）與 Constant Alpha 實例說明

#### 圖層與顏色設定

| 區域               | 顏色（RGB）   | 說明                        |
|--------------------|---------------|-----------------------------|
| Layer-1            | rgb(0, 0, 128) | 上層圖層顯示的深藍色         |
| Background（BG）   | rgb(0, 0, 48)  | 背景圖層顯示的黑藍色         |
| Blended Result     | rgb(0, 0, 123) | 混合後的最終顯示顏色         |

#### 混色參數定義

- **Constant Alpha（固定透明度）**：  
  Layer-1 設定為 240（8-bit 範圍內，最大為 255）

- **Alpha 值換算**：  
  ```
  Constant Alpha = 240 / 255 ≈ 0.94
  ```

- **混合因子設定**：
  - `BF1 = Constant Alpha` = 0.94
  - `BF2 = 1 - Constant Alpha` = 0.06

- **混合公式（每個像素通道）**：
  ```
  Output_Color = BF1 × Top_Color + BF2 × Background_Color
  ```

#### 實際計算（以藍色通道為例）

```
Top Color (Layer-1, Blue) = 128  
Background Color (BG, Blue) = 48

Blended Color (Blue) = 0.94 × 128 + 0.06 × 48
                     = 120.32 + 2.88
                     ≈ 123
```

因此，混合後顯示的藍色通道為 `rgb(0, 0, 123)`。

#### 符號說明

- `C`：Top Layer 顏色（例如 Layer-1）
- `Cs`：下層或背景顏色（例如 BG）

---

# 6. LCD 控制實作與顯示測試

## 6.1 LCD 顯示介面介紹與 STM32F429I-DISC1 架構分析
> LCD 類型、控制器、RGB 並行模式的特性與顯示流程概論

### 6.1.1 LCD 控制分類與控制器晶片概述

#### 常見 LCD 控制方式與 STM32 對應模組

| 控制方式               | 簡介             | 使用介面                         | 對應 STM32 模組                         | 傳輸方式 | 備註                                                                 |
|------------------------|------------------|----------------------------------|------------------------------------------|----------|----------------------------------------------------------------------|
| **SPI**                | 最簡單且常見     | MOSI, SCK, CS, DC                | SPI                                      | 序列     | 傳輸速度較慢，通常 MCU 每畫一筆就傳送一筆資料                        |
| **FMC（8080 並列）**   | 中階主流方案     | RD, WR, DB0~DB15                 | FMC（Flexible Memory Controller）        | 並列     | 適用於 LCD 為 MCU Interface 類型，如 ST7796、ILI9341 等              |
| **RGB 並行介面**       | 高速顯示方案     | HSYNC, VSYNC, DOTCLK, RGB888     | **LTDC**（LCD-TFT Display Controller）   | 並行即時 | 可直接驅動 LCD 面板，類似顯示卡原理，支援即時畫面更新                 |
| **DSI / HDMI / LVDS**  | 高階應用介面     | 差動訊號線對                     | DSI Host（僅部分 STM32 系列支援）       | 並行高速 | 多用於高解析觸控螢幕，常見於行動裝置顯示技術                         |

#### 常見 LCD 控制器與製造商對照表

| 控制器型號   | 製造商               | 常見解析度 | 支援介面                        | 備註                                                              |
|--------------|----------------------|------------|----------------------------------|-------------------------------------------------------------------|
| **ILI9341**  | 奕力科技 Ilitek       | 240×320    | SPI、8080 並列、RGB 並行        | 市面最常見型號之一，支援多種介面，相關開發資源充足                |
| **ST7789**   | 矽創電子 Sitronix     | 240×240    | SPI                              | 僅支援序列傳輸，常見於小尺寸顯示模組                              |
| **ST7796**   | 矽創電子 Sitronix     | 320×480    | 8080 並列、RGB 並行              | 常見於 3.5 吋顯示模組，與 ILI9486 具高相容性                      |
| **ILI9486**  | 奕力科技 Ilitek       | 320×480    | 8080 並列、RGB 並行              | 色彩與亮度表現優異，常見於中高階顯示模組                          |

---

### 6.1.2 ILI9341 控制器功能與顯示模式解析

#### ILI9341 控制器詳細介紹

**ILI9341** 是由 **台灣奕力科技（Ilitek）** 設計的 LCD 控制器晶片，廣泛應用於 2.4 ~ 2.8 吋 QVGA (240×320) TFT LCD 模組中。此晶片支援多種傳輸介面與像素格式，具備良好的相容性與開發彈性。

##### 支援介面：

- **SPI**（3 線或 4 線模式）
- **8080 MCU 並列介面**（8-bit 或 16-bit）
- **RGB 並行介面**（同步訊號驅動，含 HSYNC / VSYNC / DOTCLK）

> 若使用 RGB 並行介面，需額外提供同步訊號與像素時脈，由 MCU 或 LTDC 模組提供。

##### 支援像素格式（Pixel Format）：

- **RGB565**（16-bit，R:5bit / G:6bit / B:5bit）  
　常見於 SPI 或 MCU 並列介面，資料量較小。
- **RGB666**（18-bit，R/G/B 各佔 6bit）  
　**適用於 RGB 並行介面，對應實體線制為 R0~R5、G0~G5、B0~B5，共 18 條資料線**。
- **RGB888**（24-bit，R/G/B 各佔 8bit）  
　需面板與控制器均支援，實務上 ILI9341 僅內部轉換並非真實 24bit 顯示。

#### ILI9341 的顯示介面模式比較

| 模式名稱        | 說明                                                       | MCU 傳輸方式與角色                      |
|------------------|------------------------------------------------------------|-----------------------------------------|
| **命令模式**     | LCD 接收 MCU 傳送的命令與資料，內部控制顯示邏輯（常見於 SPI 或 8080 並列） | MCU 每次畫圖需傳送命令與像素資料        |
| **RGB 並行模式** | LCD 僅接收畫面像素資料，不處理命令，根據同步時序即時顯示畫面           | MCU 透過 LTDC 模組產生同步訊號與資料輸出 |

---

### 6.1.3 STM32F429I-DISC1 LCD 架構與顯示原理

根據 ST 官方文件《UM1670》第 6.9 節說明，STM32F429I-DISC1 開發板上搭載的 TFT LCD 模組具備以下特性：

- **控制器晶片**：ILI9341（內建於模組內部）
- **顯示解析度**：QVGA（240 × 320 dots）
- **顯示介面**：RGB 並行介面（透過 LTDC 模組驅動）
- **同步傳輸訊號**：HSYNC、VSYNC、DOTCLK
- **色彩深度**：262K 色（6 位元/色 → 對應 RGB666 線制）
- **工作電壓**：典型值 2.8V

#### LTDC（LCD-TFT Display Controller）模組介紹

LTDC 是 STM32F429 內建的顯示控制模組，用於驅動 RGB 並行介面顯示器。其角色類似「硬體顯示卡」，負責自動產生畫面所需的像素資料與同步訊號。其功能如下：

- 自動從 **frame buffer（影像緩衝區）** 中讀取像素資料
- 主動產生 LCD 所需的同步時序與畫素訊號（如 HSYNC、VSYNC、DOTCLK）
- 將畫面資料以並行形式輸出至 LCD（如 ILI9341），無需 MCU 逐筆傳送
- 支援多層圖層（Layer 0 / Layer 1），可用於合成背景與前景圖層（例如 OSD）

開發流程中，MCU 會先將畫面內容填入 RAM（通常為 SDRAM），再由 LTDC 依照設定的時序自動「掃描」這塊記憶體，並同步輸出資料至 LCD 控制器顯示。

#### RGB 並行介面顯示原理

RGB 並行介面是一種即時顯示的傳輸方式，不依賴命令控制，其主要特性如下：

- 每個像素以 **RGB666** 格式輸出（R/G/B 各 6-bit，共 18-bit），但實務上多數模組採用 **RGB565**（16-bit）作為畫面輸入格式。
- 顯示資料以「像素為單位」連續傳輸，並透過同步訊號實現逐行掃描，達成即時顯示。
- 雖然畫面資料不再經由 SPI 傳送，但在進入 RGB 並行顯示模式之前，仍需透過 SPI（或 8080 並列介面）傳送初始化命令（command），以設定 ILI9341 的啟動參數與顯示模式。
- 當初始化完成並切換至 RGB 並行模式後，**MCU 將不再傳送任何控制命令**，而是持續透過 RGB 資料線與同步訊號輸出畫面內容。此時，**ILI9341 僅作為被動的顯示終端**，即時接收資料並顯示畫面。
- 顯示所需同步訊號包括：
  - **HSYNC**（水平同步）
  - **VSYNC**（垂直同步）
  - **DOTCLK**（像素時脈）
  - **DE**（Display Enable，部分面板使用）

此類顯示機制與 **VGA** 或 **HDMI** 類似，畫面更新完全依賴顯示控制器的即時掃描，而非由 MCU 傳送命令逐筆控制。

#### SPI 與 RGB 顯示架構比較

| 比較項目   | SPI LCD                         | LTDC RGB LCD                          |
|------------|----------------------------------|----------------------------------------|
| 傳輸方式   | MCU 每次畫圖需逐筆傳送像素資料     | LTDC 自動掃描並輸出整塊畫面               |
| 儲存方式   | MCU 自行處理繪圖並送資料           | 畫面資料存於 SDRAM 的 frame buffer       |
| 顯示效能   | 傳輸速度慢，畫面更新易閃爍         | 高速傳輸，顯示穩定流暢                   |
| 控制方式   | LCD 控制器解析命令與資料           | LCD 僅接收 RGB 資料流，不解析命令          |
| 開發難度   | 程式邏輯簡單                     | 需設定時序參數與 SDRAM 接口               |

---

### 6.1.4 開發流程與系統架構總結

STM32F429I-DISC1 開發板上的 LCD 模組採用 **ILI9341 控制器**，透過 **RGB 並行介面** 與 **LTDC（LCD-TFT Display Controller）模組** 連接。整體顯示系統由 LTDC 自動產生同步訊號與畫面資料，並輸出至 LCD 顯示器，**無需 MCU 逐筆傳送像素資料**。

畫面資料來源於一塊記憶體區域（稱為 **frame buffer**），LTDC 會依照設定持續掃描該區域內容並更新畫面。此 frame buffer 可以配置於：

- **外部 SDRAM**（適合高解析度或多圖層顯示）
- **內部 SRAM**（適用於低解析度、靜態畫面或記憶體需求低的應用）

開發者可依實際需求與系統資源，在內部 SRAM 或外部 SDRAM 中擇一配置 frame buffer，以達成平衡的效能與資源使用。

#### LCD 顯示流程整理：

1. **初始化 RGB 腳位**
   - 呼叫 `ltdc_gpio_init()` → 將所有 RGB 並行輸出腳位設定為 Alternate Function 模式（AF14）

補充：**若使用 SDRAM 作為 frame buffer，MCU 啟動後需先完成 FMC 與 SDRAM 的初始化作業**
   - 呼叫 `sdram_gpio_init()` 設定 FMC 所需的外部匯流排腳位
   - 呼叫 `sdram_init()` 執行 SDRAM 初始化序列（包括時序與模式寄存器配置）
   - SDRAM 初始化成功後，位址區段 `0xD0000000` 起可作為可讀寫的 frame buffer 區域
   - *若 SDRAM 初始化失敗，LTDC 將無法正確讀取畫面資料，可能導致顯示異常或出現雜訊*

2. **LTDC 初始化並指定 frame buffer 起始位址**
   - 呼叫 `ltdc_clock_init()` → 設定像素時脈來源（例如透過 PLLSAI 輸出）
   - 呼叫 `ltdc_init()` → 設定畫面解析度、同步時序參數、像素格式（如 RGB565 或 RGB888），以及 frame buffer 的起始位址

3. **MCU 將畫面資料（如純紅色）寫入 frame buffer**
   - 呼叫 `lcd_clear_red()` → 將 frame buffer 中的每個像素填入紅色資料（通常為固定顏色的 16-bit 或 24-bit 值）

4. **LTDC 自動掃描 frame buffer 並透過 RGB 並行介面輸出畫面**
   - 呼叫 `ltdc_start_display()` → 啟用 shadow reload，LTDC 開始依照設定參數驅動畫面輸出
   - 畫面資料經由 `HSYNC`、`VSYNC`、`DOTCLK` 與 RGB 資料線傳送至 LCD 面板
   - **MCU 不需逐筆傳送畫面資料，顯示更新由 LTDC 硬體自動完成**

---

## 6.2 SPI 初始化 ILI9341 控制器
> 說明透過 SPI 傳送初始化命令、切換成 RGB 模式、設定 pixel format

### 6.2.1 SPI 模組功能總覽與傳輸邏輯

#### STM32F429 SPI 特性（SPI features）

根據《Reference Manual》28.2.1 節所述，STM32F429 的 SPI 模組具備以下功能特性：

- **全雙工同步傳輸**（Full-duplex synchronous transfers）：透過三條線實現同步資料雙向傳輸。
- **單向同步傳輸**（Simplex synchronous transfers）：可使用兩條線進行單向或雙向的資料傳輸。
- 支援 **8-bit 或 16-bit 資料框格式**，可依需求設定。
- 可配置為 **主機（Master）或從機（Slave）模式**。
- 提供 **8 種主機時脈分頻器（baud rate prescaler）**，主機最高輸出頻率可達 `fPCLK/2`。
- 從機可接收的時脈頻率同樣最高為 `fPCLK/2`。
- 可程式化的 **時脈極性（CPOL）與時脈相位（CPHA）**，以支援不同裝置的時序要求。
- 支援 **位元順序設定**：可選擇 **MSB-first** 或 **LSB-first** 資料傳送方式。
- 具備獨立的 **傳送（Tx）與接收（Rx）旗標**，並支援中斷觸發機制。
- 支援 **DMA 傳輸模式**：內建 1-byte 資料暫存器，支援 DMA 控制的 Tx / Rx 請求。

#### STM32F429 SPI 功能描述（SPI functional description）

**SPI 外部腳位定義**（通常接到外部設備的四個腳位）：

| 腳位       | 名稱                    | 說明                                                                 |
|------------|-------------------------|----------------------------------------------------------------------|
| **MISO**   | Master In / Slave Out   | 主機接收、從機傳送的資料腳位。在主機模式下用於接收資料；在從機模式下用於傳送資料。 |
| **MOSI**   | Master Out / Slave In   | 主機傳送、從機接收的資料腳位。在主機模式下用於傳送資料；在從機模式下用於接收資料。 |
| **SCK**    | Serial Clock            | 提供同步時脈訊號。在主機模式下輸出給從機；在從機模式下接收時脈訊號。              |
| **NSS**    | Slave Select            | 用來選擇從機（optional）。此腳位可作為片選訊號（chip select），避免多個從機同時使用資料線。 |

- 若設定 STM32 SPI 為從機模式，NSS 腳位會作為**輸入腳**使用。當主機（例如另一顆 MCU）選取 STM32 從機時，必須將 NSS 拉低；通常會接到主機端的 GPIO 腳位控制。

- 若 STM32 SPI 模組處於主機模式（`MSTR=1`），並希望由硬體自動控制 NSS 腳位為輸出，則需在 `SPI_CR2` 暫存器中設定 `SSOE=1`（Slave Select Output Enable）。

- 若設定 STM32 為主機（`MSTR=1`），但 NSS 腳位**以輸入方式接收到低電位**，STM32 會認為有其他主機試圖接管匯流排，此時會觸發錯誤狀態（**Master Mode Fault**）：`MSTR` 位元會自動被清除，SPI 模組將退回從機模式以避免匯流排衝突。

- STM32 SPI 模組預設為 **MSB-first（最高有效位元先出）** 傳輸格式，SCK 腳提供時脈訊號，確保資料在一致的時序下同步傳送與接收。

#### Clock Phase（CPHA）與 Clock Polarity（CPOL）說明

SPI 傳輸的時序關係可透過軟體設定，使用 `SPI_CR1` 暫存器中的 CPOL 與 CPHA 兩個位元：

**CPOL（Clock Polarity，時脈極性）**：  
控制 SCK 腳位在閒置時的電位（資料未傳輸時的預設狀態）：

- `CPOL = 0` ➜ 閒置為低電位（Low）  
- `CPOL = 1` ➜ 閒置為高電位（High）

> 此設定影響主機與從機的同步性，須一致。

**CPHA（Clock Phase，時脈相位）**：  
控制在哪一個 SCK 的邊緣取樣資料：

- `CPHA = 0` ➜ 第 1 個時脈邊緣為取樣點  
- `CPHA = 1` ➜ 第 2 個時脈邊緣為取樣點

SCK 的上升/下降緣依據 CPOL 決定：

| CPOL | CPHA | SCK 閒置狀態 | 資料於哪個邊緣取樣 |
|------|------|--------------|---------------------|
| 0    | 0    | 低電位       | 第一次上升緣        |
| 0    | 1    | 低電位       | 第二次上升緣        |
| 1    | 0    | 高電位       | 第一次下降緣        |
| 1    | 1    | 高電位       | 第二次下降緣        |

- SCK 閒置狀態必須符合 CPOL 設定：
  - `CPOL = 1` ➜ 閒置時須拉高 SCK  
  - `CPOL = 0` ➜ 閒置時須拉低 SCK

#### 資料框格式（Data frame format）

資料傳輸時可以設定為：

- MSB-first（高位元先出）
- LSB-first（低位元先出）  
（由 `SPI_CR1` 暫存器中的 `LSBFIRST` 位元設定）

每一個 **資料框（Data frame）** 可以是：

- 8-bit 或 16-bit  
（根據 `SPI_CR1` 中的 `DFF` 位元設定）

所選的格式將同時應用於 **傳送（Tx）與接收（Rx）**。

---

### 6.2.2 SPI 主機模式設定與傳輸流程（Master Mode Configuration & Data Flow）

當 SPI 被設定為主機模式時，序列時脈（serial clock）將由 **SCK 腳位輸出**。

#### 配置步驟（Procedure）

1. 設定 `BR[2:0]` 位元（位於 `SPI_CR1`）：  
   定義序列時脈（SCK）的頻率（即 SPI 傳輸速度）。

2. 設定 `CPOL` 與 `CPHA` 位元：  
   用來指定資料傳輸與序列時脈之間的四種對應關係（即時序模式）。  
   ※ 若啟用 TI 模式，此步驟可略過。

3. 設定 `DFF` 位元：  
   選擇資料框格式為 8-bit 或 16-bit。

4. 設定 `LSBFIRST` 位元（位於 `SPI_CR1`）：  
   控制資料傳輸順序為 MSB-first（預設）或 LSB-first。  
   ※ 若啟用 TI 模式，此步驟亦可省略。

5. 根據需求設定 NSS 管理模式：

   - NSS 作為輸入腳（硬體管理模式）：  
     NSS 腳需在完整傳輸期間維持高電位。
   
   - NSS 軟體管理模式（SSM/SSI）：  
     設定 `SSM=1` 與 `SSI=1`，模擬 NSS 高電位。
   
   - NSS 作為輸出腳（主機自動控制）：  
     設定 `SSOE=1`（位於 `SPI_CR2`），由硬體自動控制 NSS。

   ※ 在 TI 模式下，NSS 控制邏輯由硬體處理，此步驟可略過。

6. 設定 `FRF` 位元（位於 `SPI_CR2`）：  
   決定是否使用 TI 協定模式。

7. 啟用 SPI 模組並切換至主機模式：  
   設定 `MSTR=1` 與 `SPE=1`。  
   ※ 這兩個位元會保持有效，前提是 NSS 維持高電位。

#### 傳送流程（Transmit Sequence）

當一個位元組（byte）被寫入 Tx buffer 時，將啟動以下流程：

- 資料會平行載入至移位暫存器（shift register，來源為內部匯流排）。
- 在第一次時脈傳輸時，資料會從 MOSI 腳以串列方式傳出。
- 傳輸順序由 `LSBFIRST` 決定，支援 MSB-first 或 LSB-first。
- 資料從 Tx buffer 載入移位暫存器時，會自動設置 `TXE` 旗標（Tx buffer empty）。
  - 若 `SPI_CR2` 中啟用了 `TXEIE`，則會觸發中斷。

#### 接收流程（Receive Sequence）

當傳輸完成後，接收方行為如下：

- 收到的資料會從移位暫存器搬入 Rx buffer，並自動設置 `RXNE` 旗標（Rx buffer not empty）。
- 若 `SPI_CR2` 中啟用了 `RXNEIE` 位元，則會觸發中斷通知。
- 當主程式讀取 `SPI_DR` 暫存器時，資料會從 Rx buffer 被取出，並清除 `RXNE` 旗標。

※ 注意：在寫入 Tx buffer 前，軟體必須先確認 `TXE=1`，否則可能覆蓋尚未傳送完成的資料。

---

### 6.2.3 SPI 資料傳輸與接收流程（Data Transmission and Reception Procedures）

#### Rx 與 Tx 暫存器行為說明

- **接收階段（Rx）**  
  外部資料透過 MISO 腳位輸入後，會先進入內部的 **移位暫存器（Shift Register）**，接著平行搬移至 **Rx buffer**。  
  使用 `SPI_DR` 暫存器進行讀取，即可取得此接收資料。

- **傳輸階段（Tx）**  
  寫入 `SPI_DR` 暫存器的資料，會先載入 **Tx buffer**，再傳送至移位暫存器，最終透過 MOSI 腳位串列輸出。

> 簡而言之：**寫入 `SPI_DR` 表示傳送資料；讀取 `SPI_DR` 表示接收資料。**

#### 主機模式下的傳輸啟動流程（Start Sequence in Master Mode）

SPI 提供四種傳輸模式，透過 `BIDIMODE`、`RXONLY`、`BIDIOE` 位元組合設定：

| 模式類型               | 設定條件                       | 備註說明                     |
|------------------------|--------------------------------|------------------------------|
| 全雙工模式             | `BIDIMODE=0`, `RXONLY=0`       | 最常見的預設模式，可同步收發 |
| 單向接收模式           | `BIDIMODE=0`, `RXONLY=1`       | 僅啟用接收功能               |
| 單線傳送模式           | `BIDIMODE=1`, `BIDIOE=1`       | 傳送與接收共用一條線，僅傳送 |
| 單線接收模式           | `BIDIMODE=1`, `BIDIOE=0`       | 傳送與接收共用一條線，僅接收 |

> 實務上，大多數 SPI LCD 控制器僅支援主機傳送資料至 LCD，並不回傳資料。建議直接採用預設的 **全雙工模式**，僅接出 MOSI、SCK、NSS（CS），MISO 可不接。

#### 全雙工傳輸流程（Full-Duplex Mode）

1. 傳送流程由主機寫入 `SPI_DR`（Tx buffer）開始。
2. 資料從 Tx buffer 載入移位暫存器，並依據時脈（SCK）透過 MOSI 串列傳出。
3. 若 MISO 腳位連接且從機有回傳資料，將會載入移位暫存器，並平行搬入 Rx buffer，供主機讀取。

#### 狀態旗標與資料處理（Status Flags and Flow Control）

##### TXE（Transmit Buffer Empty）

- 當資料從 Tx buffer 傳至移位暫存器後，`TXE=1`，表示 Tx buffer 已空，可再寫入。
- 若 `SPI_CR2.TXEIE = 1`，此事件可觸發中斷。
- **軟體在寫入 `SPI_DR` 前，必須確認 `TXE=1`，否則可能覆蓋前一筆尚未完成傳送的資料。**

##### RXNE（Receive Buffer Not Empty）

- 資料由移位暫存器搬入 Rx buffer 後，`RXNE=1`。
- 表示可透過讀取 `SPI_DR` 取得接收資料，並會自動清除 `RXNE`。
- 若 `SPI_CR2.RXNEIE = 1`，此事件亦可觸發中斷。

##### BSY（Busy）

- SPI 傳輸進行中時，`BSY=1`。
- 在停用 SPI（清除 SPE 位元）前，應等待 `BSY=0`，確保最後一筆資料傳送完成。

### 6.2.4 SPI 腳位定義與 GPIO 初始化實作（SPI Pin Mapping and GPIO Setup）

#### SPI 腳位連接分析（Pin Mapping for SPI）

根據 LCD 模組 **FRD240C48003-B** 的 datasheet 中第 4 節 *Interface Specification*，在 SPI 模式下所使用的控制腳位如下：

| 腳位編號 | LCD 模組標示 | 功能說明                                                                 |
|----------|----------------|--------------------------------------------------------------------------|
| 30       | `SDA`          | SPI 模式下作為 **Serial input/output signal**（序列資料輸入腳，MOSI）       |
| 36       | `WR`           | SPI 或 SPI_RGB 模式下作為 **Data/Command 選擇腳位**                        |
| 37       | `RS`           | SPI 模式下作為 **Serial clock signal**，對應 SPI 的 `SCK` 腳位              |
| 38       | `CS`           | 作為 **晶片選擇（Chip Select）** 控制腳位                                  |

根據 ILI9341 資料手冊《第 4 章 Pin Descriptions》所述，若使用 RGB 介面作為顯示輸出，則仍須透過序列介面（serial interface）進行初始化與控制設定。

而從 **STM32F429I-DISC1 開發板原理圖**可知，ILI9341 的 `IM[3:0]` 設定為 `0110`，對應資料手冊中定義的 **4-line SPI（8-bit data serial）模式**。

根據《第 7.1.8 節 Serial Interface》，此模式包含四條訊號線：MOSI、SCK、D/C（資料/命令）、CS（片選）。  
然而從原理圖可見，僅有 MOSI 與 SCK 連接至 STM32 的 **SPI5 硬體模組**，其餘 D/C 與 CS 則透過 **GPIO 腳位控制**。

| 功能別稱        | LCD 腳位標示 | 電氣名稱 | STM32F429 對應接腳         |
|-----------------|---------------|----------|-----------------------------|
| MOSI            | `SDA`         | PF9      | **SPI5_MOSI**               |
| SCK（時脈）     | `RS/SCL`      | PF7      | **SPI5_SCK**                |
| D/C（資料/命令） | `WR`          | PD13     | **GPIO 控制腳（WRX_DCX）**  |
| CS（片選）      | `CS`          | PC2      | **GPIO 控制腳（CSX）**      |

> 備註：LCD 並未使用 MISO 腳，屬於單向傳輸結構，故 SPI 模式可採用 **全雙工模式（BIDIMODE=0, RXONLY=0）**，僅實際連接 MOSI 與 SCK 兩線。

#### SPI 腳位初始化程式碼

以下為對應 SPI5 所需 GPIO 腳位之初始化程式，包含 MOSI、SCK、D/C、CS：

````C
void spi_gpio_init(void)
{
    // Enable AHB1 clocks for GPIOF, GPIOD, and GPIOC
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOF);  // for SCK, MOSI
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOD);  // for D/C (WRX)
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);  // for CS

    // Set PF9 to SPI5_MOSI (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_9, ALTERNATE_AF5);

    // Set PF7 to SPI5_SCK (AF5)
    gpio_set_mode(GPIOF_BASE, GPIO_PIN_7, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOF_BASE, GPIO_PIN_7, ALTERNATE_AF5);

    // Set PD13 as output for D/C (Data/Command select)
    gpio_set_mode(GPIOD_BASE, GPIO_PIN_13, GPIO_MODE_OUTPUT);

    // Set PC2 as output for CS (Chip Select)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_2, GPIO_MODE_OUTPUT);
}
````

### 6.2.5  STM32 SPI 硬體初始化

#### 步驟一、設定 `BR[2:0]` 位元

這邊需設定 SPI 的序列時脈（SCK，Serial Clock），也就是 SPI 傳輸速度。此頻率由 SPI 控制暫存器 1（`SPI_CR1`）中的 `BR[2:0]` 位元負責控制，作用是設定 **波特率預除值（baud rate prescaler）**，將 SPI 時脈來源 `fCK` 進行分頻處理。

根據 STM32 的架構，`fCK` 為所屬匯流排（APB1 或 APB2）的時脈。以 STM32F429 為例，**SPI5 掛載於 APB2 匯流排**，因此：

> `fCK_SPI5 = fCK_APB2`

在 **裸機開發（bare-metal）** 環境下，MCU 上電後系統時脈（SYSCLK）預設採用內部高速震盪器 **HSI** 作為時脈來源，頻率為 **16 MHz**。  
若未修改系統時脈配置，根據預設設定：

- APB2 匯流排的除頻參數 `PPRE2 = 0`（未分頻）
- 則有：`fCK_APB2 = SYSCLK / 1 = 16 MHz`

所以 SPI5 的輸入時脈為：

```
fCK_SPI5 = 16 MHz
```

根據 ILI9341 的技術手冊，**SPI 模式的 Serial Clock Cycle（寫入）最小為 100 ns**，也就是完整一個高＋低的 SCK 週期不可短於 100ns，  
換算為頻率為：

```
fSCK_max = 1 / 100 ns = 10 MHz
```

若超過該頻率，將違反時序規範，可能導致從設備無法接收資料、產生畫面亂碼或顯示異常。

因此此例中，將 `BR[2:0]` 設定為 `000`，表示除以 2，即：

```
fSCK = fCK / 2 = 16 MHz / 2 = 8 MHz
```

程式碼範例：

````c
void spi_cr1_write_field(spi_cr1_field_t field, uint32_t value){
    uint32_t addr = SPI_BASE + SPI_CR1_OFFSET;
    uint8_t shift = 0;
    uint8_t width = 0;

    switch (field) {
        case SPI_CR1_CPHA:         shift = 0;  width = 1; break;
        case SPI_CR1_CPOL:         shift = 1;  width = 1; break;
        case SPI_CR1_MSTR:         shift = 2;  width = 1; break;
        case SPI_CR1_BR:           shift = 3;  width = 3; break;
        case SPI_CR1_SPE:          shift = 6;  width = 1; break;
        case SPI_CR1_LSBFIRST:     shift = 7;  width = 1; break;
        case SPI_CR1_SSI:          shift = 8;  width = 1; break;
        case SPI_CR1_SSM:          shift = 9;  width = 1; break;
        case SPI_CR1_RXONLY:       shift = 10; width = 1; break;
        case SPI_CR1_DFF:          shift = 11; width = 1; break;
        case SPI_CR1_CRCNEXT:      shift = 12; width = 1; break;
        case SPI_CR1_CRCEN:        shift = 13; width = 1; break;
        case SPI_CR1_BIDIOE:       shift = 14; width = 1; break;
        case SPI_CR1_BIDIMODE:     shift = 15; width = 1; break;
        default: return;
    }

    uint32_t mask = ((1U << width) - 1U) << shift;
    uint32_t data = ((uint32_t)value & ((1U << width) - 1U)) << shift;
    io_writeMask(addr, data, mask);
}

// According to ILI9341 spec, SPI max clock = 10 MHz
// Set SPI baud rate to fPCLK / 2 = 8 MHz (assuming default fPCLK = 16 MHz)
spi_cr1_write_field(SPI_CR1_BR, 0b000);  // 0b000: baud rate divisor = 2
````

#### 步驟二、設定 `CPOL` 與 `CPHA` 位元：  

用來指定資料傳輸與序列時脈之間的四種對應關係（即 SPI 時序模式）。  
根據 ILI9341 資料手冊中《RM 第 7.1.8 節 Serial Interface》所附表格可觀察出，`SCL` 預設為低電位（常駐於 low），且每次資料的有效觸發點發生於 **上升緣（rising edge）**。  
此外，在《RM 第 7.1.10 節》中亦明確說明：

> *"ILI9341 latches the SDA (input data) at the rising edges of SCL (serial clock)"*

此敘述進一步驗證時序邏輯，表示 ILI9341 會在時脈的上升緣擷取輸入資料。

綜合以上資訊，可確認 ILI9341 所需的 SPI 時序為：

````c
// CPOL: CK=0 when idle; CPHA: capture on first clock edge
spi_cr1_write_field(SPI_CR1_CPOL, 0);
spi_cr1_write_field(SPI_CR1_CPHA, 0);
````

#### 步驟三、設定 `DFF` 位元：  

由於ILI9341 的 `IM[3:0]` 設定為 `0110`，對應資料手冊中定義的 **4-line SPI（8-bit data serial）模式**，故將資料框格式選擇為 8-bit。

````C
// Set data frame format to 8-bit (DFF=0)
spi_cr1_write_field(SPI_CR1_DFF, 0);
````

#### 步驟四、設定 `LSBFIRST` 位元（位於 `SPI_CR1`）： 

根據 ILI9341 控制器的技術手冊〈7.1.9 Write Cycle Sequence〉，在進行寫入操作時，資料的傳輸順序為 **MSB-first（最高位元先傳）**，傳輸格式為：

```
D7 → D6 → ... → D0
```

因此，STM32 的 `LSBFIRST` 位元應設定為 `0`，採用預設的 MSB-first 傳輸模式：

````c
    // Use MSB-first format (LSBFIRST = 0, default)
    spi_cr1_write_field(SPI_CR1_LSBFIRST, 0);
````

#### 步驟五、根據需求設定 NSS 管理模式：

`CR1` 中的 Bit 9 `SSM`（Software Slave Management）負責決定是否啟用軟體從機管理模式：

- 若設為 `0`，表示停用軟體從機管理，系統會依賴實體 `NSS` 腳位的電位，由外部硬體拉高或拉低 `NSS`。此模式常見於 SPI 從機的應用。
- 若設為 `1`，表示啟用軟體從機管理，此時必須透過 Bit 8 `SSI` 來模擬 `NSS` 狀態，實體 `NSS` 腳位將被忽略。

而 Bit 8 `SSI`（Internal Slave Select）僅在 `SSM = 1` 時才會生效。該位元的值會直接作用於 SPI 的從機選擇邏輯，用來模擬是否被選中的狀態。

由於 STM32F429 開發板中 LCD 的 **CS（Chip Select）腳是透過 GPIO 控制**，且 LCD 的顯示資料主要透過 **LTDC 傳輸**，SPI 僅用於發送初始化指令（例如設定暫存器），因此此處採用軟體 NSS 管理模式：

````c
// Enable software slave management (SSM = 1)
// Simulate NSS as not selected (SSI = 1)
spi_cr1_write_field(SPI_CR1_SSM, 1);
spi_cr1_write_field(SPI_CR1_SSI, 1);
````

#### 步驟六、啟用 SPI 模組並切換至主機模式：  

設定 `MSTR = 1` 將 SPI 切換為主機（Master）模式，設定 `SPE = 1` 則啟用 SPI 周邊模組。  
注意：這兩個位元在啟用時，**NSS 必須維持為高電位**，否則可能導致 SPI 無法正確啟動。

````C
// Configure as SPI master (MSTR = 1)
// Enable SPI peripheral (SPE = 1)
spi_cr1_write_field(SPI_CR1_MSTR, 1);
spi_cr1_write_field(SPI_CR1_SPE, 1);
````
### 6.2.6 ILI9341 初始化流程實作

若系統使用 STM32 的 LTDC 模組作為 RGB 並聯資料傳輸介面，仍需**在畫面顯示前，透過 SPI 接口對 ILI9341 LCD 進行初始化設定**。  
該初始化過程主要包含 LCD 內部暫存器的設置，SPI 命令的傳輸時需將 **D/CX 設為 0（Command 模式）**，以區分資料類型。

#### 步驟一、軟體重置（Software Reset）

執行 `0x01` 指令可對 ILI9341 進行一次軟體重置，此操作會將所有暫存器還原為預設值，但 **畫面記憶體 GRAM 內容不會被清除**。

重置後需延遲 **至少 5 毫秒**，以確保模組有足夠時間完成暫存器初始化程序。

##### 指令格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX   |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|--------|
| 0    | 1   | L   | XX     | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 1  | `0x01` |

##### 程式碼範例：

````c
// Step 1: Software Reset
spi_lcd_write_command(0x01); // 0x01 = Software Reset command (D/CX = 0)
delay_us(5000);              // Wait ≥ 5ms as required by ILI9341 datasheet
````

#### 步驟二、退出休眠（Sleep Out）

當完成軟體重置後，LCD 模組會自動進入低功耗的「休眠模式」。此時需透過額外指令 0x11（Sleep Out）將模組喚醒，以進行後續操作。執行 Sleep Out 後，應延遲至少 5 毫秒，若模組原本處於 Sleep In 模式，則建議延遲 120 毫秒，以確保內部電源與時脈穩定後再進行後續指令再執行後續初始化（例如開啟顯示）。

##### 指令格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX   |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|--------|
| 0    | 1   | L   | XX     | 0  | 0  | 0  | 1  | 0  | 0  | 0  | 1  | `0x11` |

##### 程式碼範例：

````c
	spi_lcd_write_command(0x11);
	delay_us(120000);
````

#### 步驟三、設定RGB介面控制訊號

根據 ILI9341 技術手冊 §8.3.1「RGB Interface Signal Control (B0h)」與 §7.2.1「RGB Interface Selection」的說明，該控制器支援兩種 RGB 傳輸模式，可透過 `RCM[1:0]` 位元進行設定：

- **RCM = "10"（DE 模式）**：以 DE 腳作為畫面資料的有效區域控制依據，適用於 STM32 LTDC 預設設計。
- **RCM = "11"（SYNC 模式）**：不使用 DE 腳，而是透過暫存器配置開窗範圍，常見於對時序控制要求精密的應用。

由於 STM32 的 LTDC 模組預設採用 DE 模式，因此本設計選擇設定為：

- **ByPass_MODE（D7）**：設為 `0`，表示資料將直接傳送至內部暫存器（Direct to Shift Register），為預設值。
- **RCM[1:0]（D6:D5）**：設為 `10`，選擇 DE 模式。
- **D4（Reserved）**：設為 `0`。
- **VSPL（D3）**：設為 `1`，垂直同步訊號為高電位有效。
- **HSPL（D2）**：設為 `1`，水平同步訊號為高電位有效。
- **DPL（D1）**：設為 `0`，資料於 DOTCLK 上升緣擷取。
- **EPL（D0）**：設為 `0`，表示 DE 為高電位時代表有效顯示資料。

##### 指令與參數格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX   |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|--------|
| 0    | 1   | L   | XX     | 1  | 0  | 1  | 1  | 0  | 0  | 0  | 0  | `0xB0` |
| 1    | 1   | L   | XX     | 0  | 1  | 0  | 0  | 1  | 1  | 0  | 0  | `0x4C` |

##### 程式碼範例：

````c
// Step 3: Configure RGB interface signal mode
spi_lcd_write_command(0xB0);    // RGB Interface Signal Control
spi_lcd_write_data(0x4C);       // DE mode, VS/HS polarity high, DOTCLK rising
````

#### 步驟四、設定像素格式（Pixel Format Set）

從硬體原理圖可確認，ILI9341 的 D[17:0] 腳位皆有接至 STM32 的 LTDC 模組，表示 LCD 採用 18-bit RGB 並聯輸入（262K 色）。
因此需透過 0x3A 指令設定 RGB Interface 使用的像素格式，也就是 DPI[2:0] 位元。

在使用 RGB 介面時，僅需設定 DPI[2:0] 即可，DBI[2:0] 可忽略。因為 DBI 是對應 MCU 並列匯流排（例如 8080 或 6800 模式），本設計未使用此類介面。

本例設定如下：

DPI[2:0] = 101：代表 16-bit RGB，1 pixel

DBI[2:0] = 101（雖設定為相同值，但實際會被忽略）

此設定可確保 ILI9341 正確接收來自 STM32 LTDC 的 RGB 資料，並進行顯示。

##### 指令與參數格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX  |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|------|
| 0    | 1   | L   | XX     | 0  | 0  | 1  | 1  | 1  | 0  | 1  | 0  | 0x3A |
| 1    | 1   | L   | XX     | 0  | 1  | 0  | 1  | 0  | 1  | 0 | 1  | 0x55 |

##### 程式碼範例：

````c
// Step 4: Set pixel format to 18-bit RGB (DPI[2:0] = 101)
spi_lcd_write_command(0x3A);     // 0x3A = Pixel Format Set
spi_lcd_write_data(0x55);        // 0x55 = 16-bit RGB (DPI[2:0] = 101, DBI ignored)
````

#### 步驟五、設定其他介面控制參數（Interface Control）

執行 `0xF6` 指令後，需連續傳送三個 data 參數，用以設定各種顯示介面與資料傳輸的行為。其中較重要的控制項如下：

**RIM（RGB Interface Mode）：**

- 設為 `0`：代表使用標準傳輸模式（18-bit RGB，1 transfer/pixel）
- 設為 `1`：表示使用壓縮模式（6-bit RGB interface，3 transfer/pixel）

**DM\[1:0]（Display Operation Mode）：** 控制顯示操作所使用的訊號來源。必須選擇一種顯示方式：

| DM[1] | DM[0] | 顯示操作模式                                    |
|-------|-------|--------------------------------------------------|
| 0     | 0     | 使用 **內部時脈模式**（Internal clock operation） |
| 0     | 1     | 使用 **RGB 介面模式**（RGB Interface Mode）       |
| 1     | 0     | 使用 **VSYNC 介面模式**（VSYNC Interface Mode）   |
| 1     | 1     | **關閉顯示模式**（Setting disabled）              |

##### 指令格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX   |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|--------|
| 0    | 1   | L   | XX     | 1  | 1  | 1  | 1  | 0  | 1  | 1  | 0  | `0xF6` |

##### 程式碼範例：

````c
	// Step 5: Interface Control
	spi_lcd_write_command(0xF6);   // Interface Control
	spi_lcd_write_data(0x01);      // Data 1: WEMODE=1 (avoid overflow), others default
	spi_lcd_write_data(0x00);      // Data 2: MDT=00, EPF=00 (default, unused in RGB)
	spi_lcd_write_data(0x01);      // Data 3: ENDIAN=0, DM=01 (RGB mode), RIM=0
````

#### 步驟六、設定記憶體存取控制（Memory Access Control）

Memory Access Control 控制了畫面「掃描方向、翻轉方向與顏色順序」。

| 位元   | 名稱  | 說明                               |
| ---- | --- | -------------------------------- |
| D7   | MY  | Row address order（上下顛倒）          |
| D6   | MX  | Column address order（左右顛倒）       |
| D5   | MV  | Row/Column exchange（橫豎翻轉）        |
| D4   | ML  | Vertical refresh order（掃描順序）     |
| D3   | RGB | RGB/BGR 順序切換                     |
| D2   | MH  | Horizontal refresh order（橫向掃描順序） |
| D1–0 | X   | 保留，無作用                           |

當參數值設定為 `0x00` 時，代表以下行為：

- 掃描方向：從上到下、左到右（左上為起點）
- 無翻轉、無旋轉
- 使用 RGB 顏色順序（預設）

##### 指令與參數格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX  |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|------|
| 0    | 1   | L   | XX     | 0  | 0  | 1  | 1  | 0  | 1  | 1  | 0  | `0x36` |
| 1    | 1   | L   | XX     | 0  | 0  | 0  | 0  | 0  | 0  | 0  | 0  | `0x00` |

##### 程式碼範例：

````c
// Step 6: Set Memory Access Control (scan direction, RGB/BGR order)
	spi_lcd_write_command(0x36);     // 0x36 = Memory Access Control
	spi_lcd_write_data(0x00);        // MY=0, MX=0, MV=0, ML=0, BGR=0, MH=0
````

#### 步驟七、開啟顯示功能（Display ON）

執行 `0x29` 指令可將 ILI9341 的顯示輸出功能正式啟用，使 LCD 面板開始顯示畫面內容。  

在使用 **RGB 並聯介面** 並由 STM32 的 **LTDC 模組輸出資料**的情況下，此指令會啟用 ILI9341 的 **外部影像輸入功能**，讓面板開始根據來自 LTDC 的資料更新畫面。

此指令應在完成所有顯示參數初始化（如掃描方向、像素格式、介面模式等）後再執行。

##### 指令格式：

| D/CX | RDX | WRX | D17–8 | D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 | HEX  |
|------|-----|-----|--------|----|----|----|----|----|----|----|----|------|
| 0    | 1   | L   | XX     | 0  | 0  | 1  | 0  | 1  | 0  | 0  | 1  | `0x29` |

##### 程式碼範例：

````c
	// Step 7: Turn on display
	spi_lcd_write_command(0x29);     // 0x29 = Display ON
````

---

## 6.3 LTDC GPIO 腳位初始化
> 設定 RGB 資料線、時序線為 AF14，對應 LCD_R/G/B/HSYNC 等

在 STM32F429I-DISC1 開發板中，若要正確啟用 RGB 並行顯示介面並由 LTDC 模組驅動 LCD 螢幕，必須先將對應的大量 GPIO 腳位設定為 Alternate Function 模式（AF14），以正確連接至 LTDC 模組內部匯流排。

---

### 6.3.1 LTDC 的三個時脈區域（Clock Domains）

根據《RM0090》第 16.3.1 與 16.3.2 節，LCD-TFT 顯示控制器（LTDC）內部劃分為 **三個獨立的時脈區域**，各自負責不同功能模組與資料處理流程：

---

#### 1. **AHB Clock Domain（HCLK）**

> 由主系統匯流排 HCLK 驅動，負責圖層資料的存取與搬移，主要處理從記憶體讀取畫面資料至 FIFO，以及圖層的 frame buffer 設定。

- **AHB interface**
  - 負責從 frame buffer（配置於內部 SRAM 或外部 SDRAM）讀取畫面資料
  - 每個圖層資料以 Layer 為單位傳入（Layer 0 / Layer 1）
  - 控制圖層啟用、frame buffer 起始位址與大小等暫存器皆屬此區域
  - 代表暫存器：`LTDC_LxCR`、`LTDC_LxCFBAR`、`LTDC_LxCFBLR`、`LTDC_LxCFBLNR`

---

#### 2. **APB2 Clock Domain（PCLK2）**

> 由 APB2 匯流排提供，用於 LTDC 模組的全域設定與中斷控制，屬於低頻控制區域，所有暫存器皆由 MCU 直接存取。

- **Configuration and Status Registers**
  - 控制 LTDC 模組的所有設定，包括：
    - 畫面解析度與同步時序參數（HSYNC / VSYNC 等）
    - 圖層屬性、背景顏色、顯示啟用等控制項
  - 處理 LTDC 中斷產生、狀態讀取與旗標清除
  - 所有暫存器皆由 MCU 經由 APB2 匯流排進行存取
- 代表暫存器：  
  - `LTDC_SRCR`（Shadow Reload 控制）  
  - `LTDC_IER`、`LTDC_ISR`、`LTDC_ICR`（中斷啟用 / 狀態 / 清除）

---

#### 3. **Pixel Clock Domain（LCD_CLK）**

> 此為運作頻率最高的時脈區域，與 LCD 面板的像素刷新頻率同步。主要負責畫面輸出、時序控制、像素格式轉換與圖層混色等任務。

- **Layer FIFO（Layer0 / Layer1）**
  - 每層擁有獨立的 64 × 32-bit FIFO 暫存器
  - 用於緩衝圖層資料，平衡記憶體存取與即時輸出之間的時序差異

- **PFC（Pixel Format Converter）**
  - 將各圖層原始像素格式（如 RGB565、ARGB8888 等）轉換為內部標準格式（通常為 RGB888）

- **Blending unit**
  - 對 Layer 0 / Layer 1 執行像素混合與透明度處理（alpha blending）
  - 可實現多圖層合成、OSD 疊加等效果

- **Dithering unit**
  - 若目標 LCD 面板僅支援 18-bit 顏色（RGB666），則會啟用抖動演算法，將高位元色彩（如 24-bit）轉換為較低位元輸出
  - 可減少顏色斷層，提升顯示平滑度

- **Timing generator**
  - 產生 LCD 面板所需的同步訊號：
    - `LCD_HSYNC`（水平同步）
    - `LCD_VSYNC`（垂直同步）
    - `LCD_DE`（顯示啟用）
    - `LCD_CLK`（像素時脈）
  - 確保資料輸出與 LCD 掃描時序對齊，避免顯示錯位或撕裂

- **RGB 資料輸出**
  - 最終畫面像素資料經由下列腳位輸出至 LCD 面板：
    - `LCD_R[7:0]`、`LCD_G[7:0]`、`LCD_B[7:0]`
  - 若使用 RGB666 模式，實際只接每色前 6 位（低 2 位不使用）

- 代表暫存器：
  - `LTDC_SSCR`、`LTDC_BPCR`、`LTDC_AWCR`、`LTDC_TWCR`
  - `LTDC_LxWHPCR`、`LTDC_LxDCCR`、`LTDC_LxBFCR` 等

#### Pixel Clock 設定說明

- `LCD_CLK` 的輸出頻率應依據 LCD 面板的刷新需求設定（通常為 6 ~ 10 MHz）
- 此時脈由 **PLLSAI** 輸出，並透過 RCC 模組設定（參見 RCC 章節）

---

### 6.3.2 LCD-TFT 腳位與訊號介面（LCD-TFT pins and signal interface）

以下為 LTDC 顯示輸出所使用的訊號腳位：

| LTDC 訊號      | I/O 類型 | 說明                                 |
|----------------|----------|--------------------------------------|
| `LCD_CLK`      | O（輸出） | 像素時脈輸出（Clock Output）        |
| `LCD_HSYNC`    | O（輸出） | 水平同步訊號（Horizontal Sync）     |
| `LCD_VSYNC`    | O（輸出） | 垂直同步訊號（Vertical Sync）       |
| `LCD_DE`       | O（輸出） | 顯示資料啟用（Not Data Enable）     |
| `LCD_R[7:0]`   | O（輸出） | 紅色通道資料（8-bit Red data）      |
| `LCD_G[7:0]`   | O（輸出） | 綠色通道資料（8-bit Green data）    |
| `LCD_B[7:0]`   | O（輸出） | 藍色通道資料（8-bit Blue data）     |

這些訊號對應至 STM32 GPIO 的 **AF14（Alternate Function 14）** 腳位功能，將在 `ltdc_gpio_init()` 函式中統一初始化，並用於將畫面像素資料輸出至 LCD 面板。

若 LTDC 設定為輸出 **RGB888（24-bit）**，但實際面板僅支援 **RGB565（16-bit）** 或 **RGB666（18-bit）**，則必須將 RGB 資料接至 LTDC 對應通道的「**高位元（MSB）**」腳位。

**RGB565 介面接法對照表：**

| 顏色通道 | 面板端資料線     | LTDC 對應腳位         |
|----------|------------------|------------------------|
| Red      | `R[4:0]`         | `LCD_R[7:3]`           |
| Green    | `G[5:0]`         | `LCD_G[7:2]`           |
| Blue     | `B[4:0]`         | `LCD_B[7:3]`           |

這樣接法可確保資料對齊至有效高位，避免顯示畫面偏暗或顏色異常。

---

### 6.3.3 LTDC 可程式化同步參數（Synchronous Timings）

根據《RM0090》第 16.4.1 節 *LTDC Global configuration parameters*，LTDC 模組可透過暫存器設定 LCD 所需的水平與垂直同步時序，包括：

- 同步脈衝寬度（Sync Width）
- 後沿（Back Porch）
- 前沿（Front Porch）
- 有效顯示區域（Active Area）

這些參數將直接影響畫面更新的節奏與顯示解析度。

#### 同步參數定義說明

| 英文縮寫        | 中文名稱                          | 說明                                       |
|-----------------|-----------------------------------|--------------------------------------------|
| HSYNC           | 水平同步脈衝寬度                   | 控制每一行掃描起始點的脈衝寬度             |
| VSYNC           | 垂直同步脈衝寬度                   | 控制每一幀掃描起始點的脈衝寬度             |
| HBP             | 水平後沿（Horizontal Back Porch） | 同步脈衝結束後到資料開始顯示的空白區段     |
| HFP             | 水平前沿（Horizontal Front Porch）| 資料顯示完畢後到下次同步脈衝的空白區段     |
| VBP             | 垂直後沿（Vertical Back Porch）   | 垂直同步結束後到畫面開始顯示前的空白區段   |
| VFP             | 垂直前沿（Vertical Front Porch）  | 畫面顯示結束後到下次垂直同步的空白區段     |
| Active Area     | 有效顯示區域                       | LCD 上真正顯示像素的畫面區域               |

#### 暫存器設定公式與對應說明

- **HSYNC 與 VSYNC 寬度**：  
  設定於 `LTDC_SSCR` 暫存器：  
  `HSYNC Width - 1` 與 `VSYNC Width - 1`

- **HBP 與 VBP（後沿）**：  
  設定於 `LTDC_BPCR` 暫存器：  
  `HSYNC Width + HBP - 1` 與 `VSYNC Width + VBP - 1`

- **Active Width 與 Active Height（有效顯示區域）**：  
  設定於 `LTDC_AWCR` 暫存器：  
  `HSYNC Width + HBP + Active Width - 1`  
  `VSYNC Width + VBP + Active Height - 1`  
  （支援最大解析度：1024x768）

- **Total Width（總寬度）**：  
  設定於 `LTDC_TWCR` 暫存器：  
  `HSYNC Width + HBP + Active Width + HFP - 1`  
  其中 HFP 為水平前沿時間

- **Total Height（總高度）**：  
  設定於 `LTDC_TWCR` 暫存器：  
  `VSYNC Height + VBP + Active Height + VFP - 1`  
  其中 VFP 為垂直前沿時間

#### 備註與實務說明

- **HBP / HFP**：分別為每行畫面資料顯示前與顯示後的水平方向緩衝區（空白區域）
- **VBP / VFP**：分別為每幀畫面上下方的垂直方向緩衝時間（等待區段）

當 LTDC 被啟用後，顯示時序會以 `(X,Y) = (0,0)` 為起點，此點對應於垂直同步期間的第一個水平同步像素。畫面資料依序經過後沿、有效區域與前沿時間。

這些參數對 LCD 顯示穩定性極為關鍵，**具體數值需依據 LCD 面板的 datasheet 提供，並正確對應至上述暫存器配置**。

#### 時序參數

此顯示器為 2.4 吋 QVGA（240×320 dots），使用 ILI9341 控制器，依據 ILI9341 datasheet 所建議的典型參數，設定如下：

| 參數項目              | 值   | 說明                                  |
|-----------------------|------|---------------------------------------|
| **HSYNC Width**       | 10   | 水平同步脈衝寬度（Horizontal Sync Pulse Width） |
| **HBP**               | 20   | 水平後沿（Horizontal Back Porch）     |
| **HFP**               | 10   | 水平前沿（Horizontal Front Porch）    |
| **VSYNC Width**       | 2    | 垂直同步脈衝寬度（Vertical Sync Pulse Width） |
| **VBP**               | 2    | 垂直後沿（Vertical Back Porch）       |
| **VFP**               | 4    | 垂直前沿（Vertical Front Porch）      |
| **Active Width**      | 240  | 有效顯示寬度（水平解析度）            |
| **Active Height**     | 320  | 有效顯示高度（垂直解析度）            |


##### 更新頻率計算（以 DOTCLK = 8 MHz 為例）

- **水平總週期（Horizontal Total）**：  
  `HSYNC + HBP + Active Width + HFP = 10 + 20 + 240 + 10 = 280 DOTCLK`

- **垂直總週期（Vertical Total）**：  
  `VSYNC + VBP + Active Height + VFP = 2 + 2 + 320 + 4 = 328 行`

- **畫面更新頻率（Frame Rate）**：
  ```
  Frame Rate = DOTCLK / (Horizontal Total × Vertical Total)
             = 8,000,000 / (280 × 328)
             = 8,000,000 / 91,840
             ≒ 87.1 Hz
  ```

此更新頻率約為 **87.1 Hz**，符合 ILI9341 顯示控制器的建議更新範圍（典型為 70 Hz，最大不超過 100 Hz），  
可確保畫面穩定顯示且無閃爍、花屏等異常現象。

#### 實際設定值

| 暫存器             | 設定值                             | 說明                         |
| --------------- | ------------------------------- | -------------------------- |
| `HSYNC Width`   | 10 - 1 = **9**                  | `LTDC_SSCR` 的 Horizontal部分 |
| `VSYNC Width`   | 2 - 1 = **1**                   | `LTDC_SSCR` 的 Vertical 部分  |
| `HBP`           | 10 + 20 - 1 = **29**             | `LTDC_BPCR` 的 Horizontal部分 |
| `VBP`           | 2 + 2 - 1 = **3**               | `LTDC_BPCR` 的 Vertical 部分  |
| `Active Width`  | 10 + 20 + 240 - 1 = **269**      | `LTDC_AWCR` 的 Horizontal部分 |
| `Active Height` | 2 + 2 + 320 - 1 = **323**       | `LTDC_AWCR` 的 Vertical 部分  |
| `Total Width`   | 10 + 20 + 240 + 10 - 1 = **279** | `LTDC_TWCR` 的 Horizontal部分 |
| `Total Height`  | 2 + 2 + 320 + 4 - 1 = **327**   | `LTDC_TWCR` 的 Vertical 部分  |

---

### 6.3.4 LTDC 時序起點與關閉行為說明

根據 RM0090 說明，當 LTDC 模組啟用時，顯示時序的產生方式如下：

- 時序從座標 **(X, Y) = (0, 0)** 開始，該點對應於：
  - 第一行垂直同步區（VSYNC）中的第一個水平同步像素（HSYNC）
- 接著依序輸出以下區域：
  1. 後沿區（Back Porch）
  2. 有效畫面區（Active Area）
  3. 前沿區（Front Porch）

#### 停用時的行為（LTDC disabled）

當 LTDC 被關閉時：

- Timing Generator（時序產生器）會被重設為：
  - `X = Total Width - 1`
  - `Y = Total Height - 1`
- 並保持該像素直到下一次 VSYNC 起始為止
- 同時：
  - FIFO（畫面資料暫存器）會被清空
  - 僅持續輸出空白區（blanking data）

此機制可避免畫面殘影或誤輸出，常見於切換顯示來源或暫停顯示時。

#### 同步時序設定範例（640×480 顯示器）

以下為一組典型 LCD 模組（解析度 640x480）之時序設定參考：

| 項目                           | 值               |
|--------------------------------|------------------|
| 水平 / 垂直同步寬度（Sync）     | H: 8 像素、V: 4 行 |
| 水平 / 垂直後沿（Back Porch）   | H: 7 像素、V: 2 行 |
| 有效畫面區（Active Area）       | 640 × 480        |
| 水平 / 垂直前沿（Front Porch）  | H: 6 像素、V: 2 行 |

#### 實際暫存器對應設定值

| 暫存器         | 設定值          | 說明（括號為位元解析）                                         |
|----------------|------------------|---------------------------------------------------------------|
| `LTDC_SSCR`    | `0x00070003`     | HSW = 0x7、VSH = 0x3（對應 8-1 與 4-1）                         |
| `LTDC_BPCR`    | `0x00E00005`     | AHBP = 0xE、AVBP = 0x5（8+7-1 與 4+2-1）                         |
| `LTDC_AWCR`    | `0x028E01E5`     | AAW = 0x28E、AAH = 0x1E5（8+7+640-1 與 4+2+480-1）              |
| `LTDC_TWCR`    | `0x029400E7`     | TOTALW = 0x294、TOTALH = 0xE7（8+7+640+6-1 與 4+2+480+2-1）     |
| `LTDC_THCR`    | `0x000001E7`     | 最終行計數器，TOTALH 最後一行位置 = 0xE7                       |

---

### 6.3.5 LTDC 設定步驟（Programming Procedure）

LTDC 初始化需依照下列步驟進行，以確保時序、圖層、畫面輸出等功能能正確啟動：

#### 一、全域參數初始化（控制器本體）

1. 啟用 LTDC 的時脈（於 RCC 設定中啟用）
2. 設定 LCD 面板所需的 Pixel Clock（通常透過 PLLSAI 計算）
3. 設定同步時序參數：
   - VSYNC、HSYNC、Back Porch、Front Porch、Active Area
   - 寫入 `LTDC_SSCR`、`LTDC_BPCR`、`LTDC_AWCR`、`LTDC_TWCR`
4. 設定同步訊號與資料輸出極性（於 `LTDC_GCR` 中）
5. 若需指定背景顏色，設定 `LTDC_BCCR`
6. 若需使用中斷功能，設定 `LTDC_IER` 與 `LTDC_LIPCR`

#### 二、圖層參數設定（Layer 0 / Layer 1）

7. 設定圖層視窗範圍（Windowing）
   - `LTDC_LxWHPCR`（水平位置）與 `LTDC_LxWVPCR`（垂直位置）
8. 設定像素格式（Pixel Format）
   - `LTDC_LxPFCR`
9. 設定 Frame Buffer 起始位址
   - `LTDC_LxCFBAR`
10. 設定畫面一列的 Pitch（位元組長度）
    - `LTDC_LxCFBLR`
11. 設定總行數（Line Count）
    - `LTDC_LxCFBLNR`
12. 若使用 CLUT（Color Look-Up Table）：
    - 設定 CLUT 內容與寫入地址：`LTDC_LxCLUTWR`
13. 若需要 Alpha Blending 或預設底色：
    - 設定 `LTDC_LxDCCR` 與 `LTDC_LxBFCR`

#### 三、啟用圖層與控制器

14. 啟用圖層與（若有）CLUT：
    - `LTDC_LxCR`
15. 若需啟用 Dithering 或 Color Keying：
    - `LTDC_GCR` 與 `LTDC_LxCKCR`
16. 載入 Shadow Registers：
    - 透過 `LTDC_SRCR`（Immediate 或 Vertical Blank reload）
17. 啟用 LTDC 控制器：
    - `LTDC_GCR`

#### 補充說明

- 所有圖層暫存器為 **Shadow Register**，需透過 `LTDC_SRCR` 實際載入
- 若尚未 reload，重複寫入會覆蓋尚未生效的暫存設定
- 除 CLUT 以外，其他參數皆可「on-the-fly」修改，但修改後仍需 reload 才會生效

---

### 6.3.6 LTDC 的 RGB 資料線 GPIO 初始化設定

根據 LCD 模組 **FRD240C48003-B** 的原理圖可知，其 RGB 各通道僅接出 6 條資料腳位（R0~R5、G0~G5、B0~B5），表示該模組採用 **RGB666（18-bit）** 顯示格式。此格式常見於嵌入式系統，能有效節省 GPIO 腳位，同時提供良好的顯示品質。

#### 顏色格式比較

| 格式    | 每通道位元數 | 總位數  | 說明                                 |
|---------|---------------|---------|--------------------------------------|
| RGB888  | 8 bit × 3     | 24 bits | 真正全彩顯示，每通道支援 256 色階    |
| RGB666  | 6 bit × 3     | 18 bits | 常見於 MCU 應用，節省 GPIO 腳位      |
| RGB565  | 5-6-5         | 16 bits | 最省資源格式，與 MCU 相容性最佳      |

#### Red 通道（R0~R5）

| 資料位元 | STM32 GPIO |
|----------|-------------|
| R0       | PC10        |
| R1       | PB0         |
| R2       | PA11        |
| R3       | PA12        |
| R4       | PB1         |
| R5       | PG6         |
| R6~R7    | 未接        |

#### Green 通道（G0~G5）

| 資料位元 | STM32 GPIO |
|----------|-------------|
| G0       | PA6         |
| G1       | PG10        |
| G2       | PB10        |
| G3       | PB11        |
| G4       | PC7         |
| G5       | PD3         |
| G6~G7    | 未接        |

#### Blue 通道（B0~B5）

| 資料位元 | STM32 GPIO |
|----------|-------------|
| B0       | PD6         |
| B1       | PG11        |
| B2       | PG12        |
| B3       | PA3         |
| B4       | PB8         |
| B5       | PB9         |
| B6~B7    | 未接        |

#### GPIO 初始化範例程式

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

### 6.3.7 LTDC 的同步與資料控制 GPIO 初始化設定

LCD 顯示需要除畫素資料（RGB）外，還需搭配數條 **時序控制訊號（Timing Control Signals）**，用來同步畫面掃描與資料輸出。

這些訊號不屬於畫面內容本身，而是協助 LCD 正確更新畫面的重要時脈與同步腳位。

#### 控制訊號對應關係

| 原理圖名稱 | LTDC 訊號名稱 | 功能說明               | STM32 GPIO 腳位 |
|------------|----------------|------------------------|------------------|
| HSYNC      | `LCD_HSYNC`    | 水平同步訊號           | `PC6`            |
| VSYNC      | `LCD_VSYNC`    | 垂直同步訊號           | `PA4`            |
| ENABLE     | `LCD_DE`       | Data Enable（資料使能）| `PF10`           |
| DOTCLK     | `LCD_CLK`      | Dot Clock（像素時脈）  | `PG7`            |

#### GPIO 初始化範例程式

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

## 6.4

完成 ILI9341 顯示控制器所需的 SPI 初始化設定與 LTDC 所用 GPIO 腳位配置之後，即可正式進入 LTDC 的初始化流程。

#### 步驟一、啟用 LTDC 模組時脈

在啟用任何 STM32 模組功能之前，第一步通常都是先啟用該模組的時脈。根據《RM0090 Reference Manual》2.3 節 *Memory map*，LTDC 模組掛載於 APB2 匯流排，對應 `RCC_APB2ENR` 暫存器的 bit 26。

````c
#define RCC_APB2EN_LTDC 26  // LTDC EN bit = bit 26

// Enable LTDC clock on APB2
rcc_enable_apb2_clock(RCC_APB2EN_LTDC);
````

#### 步驟二、設定像素時脈（Pixel Clock）

##### LTDC 像素時脈來源與 PLLSAI 組成邏輯

根據《RM0090 Reference Manual》第 6.2 節 *Clocks*，LTDC 的像素時脈（Pixel Clock）**不是來自 SYSCLK**，而是透過一組獨立於主 PLL 的專用時脈模組 —— **PLLSAI** 所產生。

雖然 PLLSAI 是一組獨立的 PLL，但它與主 PLL 共享相同的輸入來源與前段除頻器設定，包含：

- `PLLSRC`：輸入時脈來源選擇，可為 HSI（預設，16 MHz）或 HSE（外部晶體振盪器）
- `PLLM[5:0]`：前段除頻器，用於將輸入時脈降至 1–2 MHz 之間（VCO 要求）

上述兩者皆在 `RCC_PLLCFGR` 中設定。

另一方面，PLLSAI 則擁有獨立的啟用開關（`PLLSAION`）與輸出設定參數，包括：

- `PLLSAIN`：VCO 倍頻因子
- `PLLSAIR`：/R 分支除頻因子（供 LCD 使用）

這些參數則在 `RCC_PLLSAICFGR` 中設定。

因此，即使主系統運行於 HSI、HSE，或由主 PLL 所產生的 SYSCLK，LTDC 的像素時脈仍需**額外透過 PLLSAI 的 `/R` 分支單獨產生**。

為簡化流程，本範例暫以預設的 HSI 作為 PLLSAI 的輸入來源。實務上若需更穩定、精準的顯示品質，建議改用 HSE 並搭配高精度晶體振盪器。

當輸入時脈決定後（透過 `PLLSRC` + `PLLM`），時脈會經過除頻後進入 PLLSAI 模組，通過 **VCO（Voltage-Controlled Oscillator）** 倍頻後，產生內部高頻輸出，計算公式為：

```
f(VCO) = f(input) × PLLSAIN / PLLM
```

接著，VCO 的輸出經由 `/R` 分支，再由 `RCC_DCKCFGR` 設定的 `DIVR` 進行最終除頻，產生 **`PLLLCDCLK`**，提供給 LTDC 作為像素時脈來源（Pixel Clock）。

##### ILI9341 顯示時序與建議 Pixel Clock 頻率

根據 ILI9341 手冊第 244 頁的時序規格表，`tCYCD`（DOTCLK 時脈週期）的最小值為 100ns，這代表 Pixel Clock 的最大頻率約為 **10 MHz**。  
若超過此頻率，將違反 LCD 控制器的時序要求，可能導致畫面抖動或資料錯誤。

另外，在第 54 頁所提供的顯示時序範例中，當解析度為 320x240，畫面更新率為 70Hz 時：

- 一行總點數 = 2（前緣）+ 320（有效像素）+ 2（後緣） = 324
- 一幀總行數 = 10（前緣）+ 20（Sync Pulse）+ 240（有效畫素）+ 10（後緣） = 280

故一幀所需的總像素時脈（dot clocks）為：
```
324 × 280 = 90,720 dots/frame
```

以 70Hz 更新頻率計算，所需的 DOTCLK 頻率約為：
```
90,720 × 70 = 6.35 MHz
```

這代表 ILI9341 在典型顯示設定下，Pixel Clock 頻率需求約為 **6.35 MHz**。

因此，綜合最小實務需求與最大時序限制，建議將 Pixel Clock 設定在 **6.5 MHz 到 9 MHz** 之間，能有效避開邊界條件問題，確保時序合法且畫面顯示穩定流暢。

##### 範例程式：

根據《RM0090 Reference Manual》第 6.3.2 節的建議，為降低 PLL 抖動（jitter），VCO 的輸入頻率建議設定為 **2 MHz**。  
若輸入時脈來源使用預設的 **HSI（16 MHz）**，則應將 `PLLM` 設定為 **8**（16 ÷ 8 = 2 MHz）。

接著根據第 6.3.24 節，VCO 的輸出頻率必須介於 **100 ~ 432 MHz** 間。此範例設定 `PLLSAIN = 128`，剛好對應：

```
f(VCO) = 2 MHz × 128 = 256 MHz（合法範圍內）
```

接著透過 `PLLSAIR = 4` 除頻，再透過 `PLLSAIDIVR = 2`（代表除以 8）產生最終的 LCD Pixel Clock：

```
PLLLCDCLK = 256 ÷ 4 ÷ 8 = 8 MHz
```

此頻率落於 ILI9341 可接受的 DOTCLK 範圍（最大 10 MHz），符合時序要求。

````c
// Step 2: Configure PLLSAI to generate Pixel Clock for LTDC
//         VCO = 2 MHz (HSI / 8) × 128 = 256 MHz
//         PLLLCDCLK = 256 / 4 / 8 = 8 MHz (via /R and DIVR)

rcc_pllcfgr_write_field(LTDC_PLLCFGR_PLLSRC, 0x0);           // 0 = HSI
rcc_pllcfgr_write_field(LTDC_PLLCFGR_PLLM,   8);             // PLL input clock / 8

rcc_pllsaicfgr_write_field(LTDC_PLLSAICFGR_PLLSAIN, 128);    // PLLSAI VCO multiplier
rcc_pllsaicfgr_write_field(LTDC_PLLSAICFGR_PLLSAIR, 4);      // PLLSAI /R division

rcc_dckcfgr_write_field(LTDC_DCKCFGR_PLLSAIDIVR, 0b10);      // DIVR = /8 for final LCD clock

rcc_cr_write_field(LTDC_CR_PLLSAION, 1);                     // Enable PLLSAI
````

#### 步驟三、設定同步時序參數

VSYNC、HSYNC、垂直與水平的後沿 / 前沿（Back Porch / Front Porch）、有效顯示區（Active Area）等前置時序需依照面板規格書與 **16.4 LTDC programmable parameters** 所描述的方式進行設定。

````c
//LTDC Timing Configuration Parameters
#define LTDC_HSYNC        10    // Horizontal Sync Pulse Width (HSW)
#define LTDC_HBP          20    // Horizontal Back Porch (HBP)
#define LTDC_ACTIVE_WIDTH 240   // Active Pixels per Line
#define LTDC_HFP          10    // Horizontal Front Porch (HFP)

#define LTDC_VSYNC        2     // Vertical Sync Height (VSH)
#define LTDC_VBP          2     // Vertical Back Porch (VBP)
#define LTDC_ACTIVE_HEIGHT 320  // Active Lines per Frame
#define LTDC_VFP          4     // Vertical Front Porch (VFP)


// Derived Register Values (Based on RM0090 Formulas)
// Synchronization Size Configuration Register (SSCR)
#define LTDC_SSCR_HSW     (LTDC_HSYNC  - 1)
#define LTDC_SSCR_VSH     (LTDC_VSYNC  - 1)

// Back Porch Configuration Register (BPCR)
#define LTDC_BPCR_AHBP    (LTDC_HSYNC + LTDC_HBP - 1)
#define LTDC_BPCR_AVBP    (LTDC_VSYNC + LTDC_VBP - 1)

// Active Width Configuration Register (AWCR)
#define LTDC_AWCR_AAW     (LTDC_HSYNC + LTDC_HBP + LTDC_ACTIVE_WIDTH - 1)
#define LTDC_AWCR_AAH     (LTDC_VSYNC + LTDC_VBP + LTDC_ACTIVE_HEIGHT - 1)

// Total Width Configuration Register (TWCR)
#define LTDC_TWCR_TOTALW  (LTDC_HSYNC + LTDC_HBP + LTDC_ACTIVE_WIDTH + LTDC_HFP - 1)
#define LTDC_TWCR_TOTALH  (LTDC_VSYNC + LTDC_VBP + LTDC_ACTIVE_HEIGHT + LTDC_VFP - 1)

// Step 3: Configure LTDC timing registers
ltdc_sscr_set_field(LTDC_SSCR_FIELD_HSW, LTDC_SSCR_HSW);
ltdc_sscr_set_field(LTDC_SSCR_FIELD_VSH, LTDC_SSCR_VSH);

ltdc_bpcr_set_field(LTDC_BPCR_FIELD_AHBP, LTDC_BPCR_AHBP);
ltdc_bpcr_set_field(LTDC_BPCR_FIELD_AVBP, LTDC_BPCR_AVBP);

ltdc_awcr_set_field(LTDC_AWCR_FIELD_AAW, LTDC_AWCR_AAW);
ltdc_awcr_set_field(LTDC_AWCR_FIELD_AAH, LTDC_AWCR_AAH);

ltdc_twcr_set_field(LTDC_TWCR_FIELD_TOTALH, LTDC_TWCR_TOTALH);
ltdc_twcr_set_field(LTDC_TWCR_FIELD_TOTALW, LTDC_TWCR_TOTALW);
````

#### 步驟四、設定 LTDC 同步訊號與時脈極性（對應 ILI9341 SPI 設定）

根據 ILI9341 控制器的 SPI 初始化設定，本系統選擇 **DE 模式** 作為顯示介面，因此在設定 `LTDC_GCR` 暫存器時，需對應各同步訊號與時脈的極性配置如下：

| ILI9341 SPI 初始化位元 | 意義說明                                   | 設定值 | 對應 LTDC_GCR 位元 | LTDC 設定值 |
|------------------------|--------------------------------------------|--------|---------------------|--------------|
| **VSPL（D3）**         | 垂直同步訊號極性，`1 = 高電位有效`         | `1`    | `VSPOL`（bit 30）   | `1`          |
| **HSPL（D2）**         | 水平同步訊號極性，`1 = 高電位有效`         | `1`    | `HSPOL`（bit 31）   | `1`          |
| **EPL（D0）**          | DE 訊號極性，`0 = 高電位有效`              | `0`    | `DEPOL`（bit 29）  | `1`          |
| **DPL（D1）**          | 資料擷取時機，`0 = DOTCLK 上升緣擷取`      | `0`    | `PCPOL`（bit 28）   | `0`          |

> 補充 : `DPL = 0` 代表資料在 DOTCLK 上升緣擷取，對應 `PCPOL = 0`（表示不反相，保持原始時脈方向）

````c
// Step 4: Configure polarity settings in LTDC_GCR
//         Match the ILI9341 SPI initialization parameters:
//         - VSPL = 1 → VSYNC is active high   → VSPOL = 1
//         - HSPL = 1 → HSYNC is active high   → HSPOL = 1
//         - EPL  = 0 → DE is active high      → DEPOL = 1
//         - DPL  = 0 → Data on DOTCLK rising  → PCPOL = 0
ltdc_gcr_set_field(LTDC_GCR_FIELD_VSPOL, 1);   // VSYNC polarity: active high
ltdc_gcr_set_field(LTDC_GCR_FIELD_HSPOL, 1);   // HSYNC polarity: active high
ltdc_gcr_set_field(LTDC_GCR_FIELD_DEPOL, 1);   // DE polarity: active high
ltdc_gcr_set_field(LTDC_GCR_FIELD_PCPOL, 0);   // Pixel clock polarity: normal (rising edge latch)
````

---

#### 步驟五、設定背景色（BCCR）

根據《RM0090 Reference Manual》第 16.3.3 節 *LCD-TFT pins and signal interface* 所述，  
若 LCD 面板僅支援 RGB666 且透過 18-bit 並聯介面與 LTDC 相連，則面板端的 RGB 資料線需接至 LTDC 輸出的高位元（MSB），  
也就是說：紅色應接 `LTDC_R[7:2]`、綠色應接 `LTDC_G[7:2]`、藍色應接 `LTDC_B[7:2]`。

因此，儘管 `LTDC_BCCR` 寫入的格式仍為 RGB888（每通道 8-bit），  
但為了確保色彩正確顯示於 RGB666 面板上，**需將每通道的顏色值限制在高 6 bits，低 2 bits 清零**，如下所示：

````c
uint8_t to_rgb565_r(uint8_t r8) {
    return r8 >> 3; // R: 8-bit to 5-bit
}

uint8_t to_rgb565_g(uint8_t g8) {
    return g8 >> 2; // G: 8-bit to 6-bit
}

uint8_t to_rgb565_b(uint8_t b8) {
    return b8 >> 3; // B: 8-bit to 5-bit
}

void ltdc_bccr_set_rgb565_color(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t addr = LTDC_BASE + LTDC_BCCR_OFFSET;

    uint32_t r = to_rgb565_r(red) << 3;
    uint32_t g = to_rgb565_g(green) << 2;
    uint32_t b = to_rgb565_b(blue) << 3;

    uint32_t data = (r << 16) | (g << 8) | b;
    uint32_t mask = 0x00FFFFFFU;

    io_writeMask(addr, data, mask);
}

// Step 5: Set LTDC background color (BCCR)
ltdc_bccr_set_rgb565_color(255,0,0);  // Set background to red (R=255, G=0, B=0)
````

---

#### 步驟六、設定 LTDC 中斷功能（IER / LIPCR）

設定 `LTDC_IER` 與 `LTDC_LIPCR` 暫存器中的中斷（如有需要）

- **`LTDC_IER`（LTDC Interrupt Enable Register）**：  
  用於啟用 LTDC 的各種中斷功能，例如行中斷（Line Interrupt）、FIFO 欠載中斷、傳輸錯誤中斷、暫存器重載完成中斷等。當應用情境中需要對這些事件即時回應（如動態更新畫面）時，才需啟用對應的中斷來源。

- **`LTDC_LIPCR`（Line Interrupt Position Configuration Register）**：  
  此暫存器可設定在哪一行產生行中斷，常用於在畫面掃描至特定位置時進行操作，例如雙緩衝切換或 frame buffer 更新，以避免畫面撕裂（tearing）問題。

**備註**：  
目前僅需顯示一個滿版紅色畫面（靜態畫面），無需進行動態更新或中斷處理。因此，本步驟中的中斷設定可暫時略過。LTDC 將自動且持續地從 frame buffer 掃描資料至 LCD，無需額外干預。

如需後續進行畫面更新或雙緩衝處理，再考慮啟用相關中斷機制即可。

---

#### 步驟七、設定 LTDC 圖層參數（Layer 設定）

##### 7.1 LTDC Layerx（x = 1 或 2）水平與垂直視窗位置設定暫存器（LTDC_LxWHPCR / LTDC_LxWVPCR）

這兩個暫存器分別用來設定圖層 Layer1 或 Layer2 在畫面中的**水平與垂直顯示區域**（即：第一個與最後一個像素/掃描線的位置），此區域應包含於由 `AWCR` 定義的有效顯示範圍（Active Area）內。

- **水平視窗位置設定（LTDC_LxWHPCR）**

畫面中第一個可見像素的位置為：
```
WHSTPOS[11:0] = AHBP + 1 = 29 + 1 = 30
```

畫面中最後一個可見像素的位置為：
```
WHSPPOS[11:0] = AAW = 269
```

- **垂直視窗位置設定（LTDC_LxWVPCR）**

畫面中第一條可見掃描線的位置為：
```
WVSTPOS[10:0] = AVBP + 1 = 3 + 1 = 4
```

畫面中最後一條可見掃描線的位置為：
```
WVSPPOS[10:0] = AAH = 323
```

````c
// 7.1 Set Layer 1 window position within the active display area (defined by AWCR)

// Set horizontal window start and stop positions (in pixel clocks)
ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSTPOS, LTDC_LAYER_WHSTPOS);  // Start = AHBP + 1 (first visible pixel)
ltdc_lxwhpcr_set_field(1, LTDC_LXWHPCR_FIELD_WHSPPOS, LTDC_LAYER_WHSPPOS);  // Stop  = AAW (last visible pixel)

// Set vertical window start and stop positions (in scan lines)
ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSTPOS, LTDC_LAYER_WVSTPOS);  // Start = AVBP + 1 (first visible line)
ltdc_lxwvpcr_set_field(1, LTDC_LXWVPCR_FIELD_WVSPPOS, LTDC_LAYER_WVSPPOS);  // Stop  = AAH (last visible line)
````

##### 7.2 設定 Layer1 的像素格式（LTDC_LxPFCR）與 RGB666 面板對應說明

LTDC Layerx 像素格式設定暫存器（LTDC_LxPFCR）用來設定圖層（Layer 1 或 Layer 2）使用的像素格式（Pixel Format）。  
LTDC 會從指定的 Frame Buffer 中讀取畫素資料，並將其轉換為內部統一格式 **ARGB8888**，以供後續顯示處理。

由於 LCD 面板僅接收 **18 條資料線**（6R + 6G + 6B，即 RGB666），  
而 STM32 的 LTDC 並**不支援 RGB666** 作為 framebuffer 像素格式，因此選擇設定為 **RGB565**（PF[2:0] = `010`）。

````c
#define LTDC_PIXEL_FORMAT_RGB888     0x1
#define LTDC_PIXEL_FORMAT_RGB565     0x2

void ltdc_lxpfcr_set_field(uint8_t layerx, uint32_t pixel_format) {
    uint32_t addr = LTDC_BASE + LTDC_LXPFCR_OFFSET(layerx);
    uint32_t mask = 0x07U;
    uint32_t data = pixel_format & mask;
    io_writeMask(addr, data, mask);
}

ltdc_lxpfcr_set_field(1, LTDC_PIXEL_FORMAT_RGB565);
````

##### 7.3 設定圖層 Frame Buffer 起始位址（LTDC_LxCFBAR）

**LTDC Layerx Color Frame Buffer Address Register（LTDC_LxCFBAR）**

此暫存器用來設定圖層（Layer 1 或 Layer 2）對應的影像資料起始位址（Frame Buffer Start Address）。  
該位址需指向圖層畫面中「左上角像素的記憶體位置」，LTDC 將從此位址開始依序讀取圖層顯示用的像素資料。

**注意**：若未在 linker script 中另外保留此區域空間，該記憶體位置仍會被視為「可用 RAM」，可能會被以下使用者覆蓋：

- stack（堆疊）
- global / static 變數
- heap（malloc 配置）
- 其他 DMA 裝置

若要確保該區域安全，建議於 linker script 中手動保留，或使用 `__attribute__((section()))` 搭配 `.ld` 進行配置。

````c
#define FRAMEBUFFER_ADDR  0x20000000

void ltdc_lxcfbar_set_field(uint8_t layerx, uint32_t ram_addr) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBAR_OFFSET(layerx);
    io_writeMask(addr, ram_addr, 0xFFFFFFFF);
}

// 7.3 Set Layer 1 frame buffer start address
ltdc_lxcfbar_set_field(1, FRAMEBUFFER_ADDR);
````

##### 7.4 設定圖層影像緩衝列長度與間距（Line Length & Pitch）

LTDC Layerx 顏色影像緩衝區長度設定暫存器（`LTDC_LxCFBLR`）  
用來定義圖層（Layer 1 或 Layer 2）中，每一列像素資料的長度（Line Length）與列與列之間的記憶體間距（Pitch）。

- **CFBLL[12:0]**：Color Frame Buffer Line Length  
  設定每列像素資料的實際長度（位元組），需加上固定偏移量 `+3`  
  ➝ 計算方式：`LineLength = width × bytesPerPixel + 3`

- **CFBP[12:0]**：Color Frame Buffer Pitch  
  設定每列結束後跳到下一列資料起點的偏移量（位元組）  
  ➝ 計算方式：`Pitch = width × bytesPerPixel`

以 **RGB565（每像素 2 bytes）**，畫面寬度為 **240 像素** 為例：

```
LineLength = 240 × 2 + 3 = 483 = 0x01E3
Pitch      = 240 × 2     = 480 = 0x01E0
暫存器值   = 0x01E001E3
```

````c
void ltdc_lxcfblr_set_field(uint8_t layerx, ltdc_lxcfblr_field_t field, uint32_t lcd_width) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLR_OFFSET(layerx);
    uint32_t shift = 0, data = 0;

    switch (field) {
        case LTDC_LXCFBLR_FIELD_CFBLL:
            shift = 0;
            data = (lcd_width * 2 + 3) << shift;
            break;
        case LTDC_LXCFBLR_FIELD_CFBP:
            shift = 16;
            data = (lcd_width * 2) << shift;
            break;
        default: return;
    }

    uint32_t mask = 0x1FFF << shift;
    io_writeMask(addr, data, mask);
}

// 7.4 Set frame buffer line length and pitch (in bytes)
ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBLL, LTDC_ACTIVE_WIDTH); // Line length = width * bytes per pixel + 3
ltdc_lxcfblr_set_field(1, LTDC_LXCFBLR_FIELD_CFBP, LTDC_ACTIVE_WIDTH);  // Pitch = width * bytes per pixel
````

---

##### 7.5 設定圖層掃描線數量（Frame Buffer Line Number）

LTDC Layerx Color Frame Buffer Line Number Register（`LTDC_LxCFBLNR`）  
用來設定圖層對應的影像緩衝區掃描線數量（Frame Buffer Line Number），即圖層的垂直解析度（Active Height）。

此設定決定了 LTDC 每次顯示更新時，從影像記憶體中讀取多少條掃描線作為顯示內容。

- **CFBLNBR[10:0]**：Frame Buffer Line Number  
  設定圖層的有效掃描線數量，亦即畫面高度。例如當顯示高度為 320 行時，應設定為 `320`（十進位）。

````c
void ltdc_lxcfblnr_set_field(uint8_t layerx, uint32_t height) {
    uint32_t addr = LTDC_BASE + LTDC_LXCFBLNR_OFFSET(layerx);
    uint32_t mask = 0x7FF;
    io_writeMask(addr, height, mask);
}

// 7.5 Set number of active display lines (height)
ltdc_lxcfblnr_set_field(1, LTDC_ACTIVE_HEIGHT);
````

##### 7.6 色彩查找表（CLUT）、預設顏色與圖層混色設定

在 LTDC 模組中，當使用某些像素格式（如 **L8（8-bit 色彩索引格式）**）時，需透過 CLUT（Color Look-Up Table，色彩查找表）將像素值對應至實際顯示的 RGB 顏色。此時，需透過暫存器 `LTDC_LxCLUTWR` 將 CLUT 表格內容寫入。

對於像素格式為 **L8、AL44、AL88** 等使用色彩索引的格式，LTDC 從 Frame Buffer 中讀取的是色彩的索引編號，而非 RGB 顏色值。顯示時，LTDC 會利用 CLUT 查表轉換為對應的 RGB 色彩。

例如：

- Frame Buffer 中的值為 `0x05`
- CLUT 中第 5 項對應的顏色為 `{Red=255, Green=0, Blue=0}`（紅色）
- 則該像素最終顯示為紅色

然而，本專案採用的像素格式為 **RGB565**，其像素資料中已直接包含紅、綠、藍的實際色彩值（R：5-bit，G：6-bit，B：5-bit），因此**不需使用 CLUT**，可略過 `LTDC_LxCLUTWR` 的設定。

暫存器 `LTDC_LxDCCR`（Default Color Configuration Register）可用於設定圖層中未顯示區域的預設顏色，例如圖層未覆蓋的畫面區域或透明區域。在圖層未完全覆蓋顯示區域時，此預設色將會顯示。不過，若圖層已完整覆蓋畫面，則預設色並不會出現，通常情況下**不需設定**該暫存器。

暫存器 `LTDC_LxBFCR`（Blending Factors Configuration Register）則用於設定圖層的混色（Blending）行為，例如多圖層重疊時的混色方式，或是含 Alpha 透明度的圖像合成效果。

然而，考量到本專案僅使用單一圖層（Layer 1），且無透明圖像顯示的需求，LTDC 模組將自動套用預設的混色行為，**不需進行任何額外的混色設定**，因此可略過對 `LTDC_LxBFCR` 的操作。

---

#### 步驟八、設定圖層控制暫存器（LTDC_LxCR）

當使用 L8、AL44 等需要 CLUT（Color Look-Up Table）的像素格式時，需在 `LTDC_LxCR`（Layer Control Register）中啟用對應功能，讓 LTDC 能夠透過色彩查找表正確顯示畫面。

除此之外，此暫存器亦可啟用色彩鍵值（Color Keying）功能，使指定顏色成為「透明區域」，用於圖層合成處理。

最重要的是，**需將圖層啟用（Layer Enable）功能打開，否則 LTDC 將不會顯示該圖層的內容**。

以下為 `LTDC_LxCR` 對應欄位的設定函式與範例：

````c
void ltdc_lxcr_set_field(uint8_t layerx, ltdc_lxcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_LXCR_OFFSET(layerx);
    uint32_t shift = 0;

    switch (field) {
        case LTDC_LXCR_FIELD_LEN:    shift = 0; break; // Layer Enable
        case LTDC_LXCR_FIELD_COLKEN: shift = 1; break; // Color Keying Enable
        case LTDC_LXCR_FIELD_CLUTEN: shift = 4; break; // CLUT Enable
        default: return;
    }

    uint32_t mask = 1U << shift;
    uint32_t data = value << shift;

    io_writeMask(addr, data, mask);
}

// Step 8: Enable Layer1
ltdc_lxcr_set_field(1, LTDC_LXCR_FIELD_LEN, 1);
ltdc_lxcr_set_field(2, LTDC_LXCR_FIELD_LEN, 0);
````

---

#### 步驟九、觸發 Shadow Register Reload（畫面更新觸發）

若需使用色彩抖動（dithering）以改善低色彩深度（如 RGB565）造成的漸層色帶問題，可透過 `LTDC_GCR` 暫存器啟用 Dither 功能。若需使用色鍵（color keying），將某個特定顏色設為透明區域，以實現多圖層合成效果，則可透過 `LTDC_LxCKCR` 設定透明顏色，並在 `LTDC_LxCR` 中啟用對應功能。目前專案進度尚未使用上述功能，因此這些設定可暫時略過。

STM32 LTDC 採用 **Shadow Register（陰影暫存器）** 機制，以避免畫面撕裂（tearing）現象。  
大部分圖層設定（如圖層開啟、座標、像素格式等）在寫入時，僅儲存在 shadow register 中，**不會立刻套用至硬體顯示器**。  
必須透過 `LTDC_SRCR`（Shadow Reload Register）顯式觸發，才會將 shadow register 的內容更新至顯示控制器。

> **所有圖層暫存器皆為 shadowed**，寫入後若未立即 reload，再次寫入會直接覆蓋暫存值，先前的設定也會遺失，導致顯示異常或設定無效。

##### Reload 模式說明

| 模式 | 說明 | 使用時機 |
|------|------|----------|
| **IMR**（Immediate Reload） | 立即套用 shadow register 內容 | 初始化階段或不在意撕裂時使用 |
| **VBR**（Vertical Blanking Reload） | 等待畫面顯示區結束，再套用設定，避免撕裂 | 用於動畫或切換畫面時需避免閃爍 |

目前為單層 RGB565 初始化階段，使用 `IMR` 即可：

````c
typedef enum {
    LTDC_SRCR_FIELD_IMR = 0,  // Immediate Reload
    LTDC_SRCR_FIELD_VBR = 1   // Vertical Blanking Reload
} ltdc_srcr_field_t;

void ltdc_srcr_set_field(ltdc_srcr_field_t field, uint32_t value) {
    uint32_t addr = LTDC_BASE + LTDC_SRCR_OFFSET;
    uint32_t mask = 1U << field;
    uint32_t data = value << field;

    io_writeMask(addr, data, mask);
}

// Step 9: Reload shadow registers immediately
ltdc_srcr_set_field(LTDC_SRCR_FIELD_IMR, 1);
````

---

#### 步驟十、啟用 LTDC 控制器（LTDC Enable）

完成所有圖層參數與時序設定後，需透過 `LTDC_GCR` 暫存器啟用整體 LCD-TFT 控制器。

設定 `LTDCEN` 位元（bit 0）為 1，即可正式啟動 LTDC 硬體邏輯與畫面更新。

````c
// Step 10: Enable LTDC controller (LTDC_GCR bit 0 = 1)
ltdc_gcr_set_field(LTDC_GCR_FIELD_LTDCEN, 1);
````

---

# 7. 計時器 (Timer)

為了讓 MCU 能自行計數時間，可以利用 **Timer**。  
在 STM32F429 中，計時器分成幾大類：  

- **Advanced-control timers (TIM1、TIM8)**：具備 PWM、死區時間、互補輸出等功能，常用於馬達控制。  
- **General-purpose timers (TIM2–TIM5、TIM9–TIM14)**：功能較完整，可做輸出比較、輸入捕捉、PWM，也能當基本定時器使用。  
- **Basic timers (TIM6、TIM7)**：僅具備最基本的時間基準產生功能，適合用於週期性中斷。  

若只是要實現 **週期性中斷（例如 1 ms tick）** 來閃 LED 或觸發事件，而不需要 PWM、死區時間、互補輸出等進階功能，只要參考 **第 20 章 Basic timers (TIM6/TIM7)** 即可。這兩個定時器專門用來做 **基本定時中斷**。

---

## 7.1 基本計時器 (Basic timers)

在 STM32F429 中，**TIM6** 與 **TIM7** 為基本計時器，由以下組成：  
- **16 位元自動重載計數器 (Auto-reload counter, CNT)**  
- **16 位元可程式化預分頻器 (Prescaler, PSC)**  

它們可以作為 **通用的時間基準產生器 (time-base generation)**，且兩者是 **完全獨立的**，不會共用任何資源。  

---

### TIM6 和 TIM7 的主要特性

- **16 位元自動重載向上計數器 (Up-counter)**  
- **16 位元可程式化預分頻器 (Prescaler)**，可將輸入時脈 (counter clock) 做分頻，範圍為 1–65536，並且可「即時 (on-the-fly)」修改。  
- 在 **更新事件 (Update event, UEV)** 發生時（亦即 **計數器溢位 overflow**），可觸發 **中斷或 DMA 請求**。  

---

### TIM6 和 TIM7 的工作流程

1. 定時器的時脈來源由 **RCC** 提供，稱為 **CK_INT**。  
   - 對於 TIM6/TIM7，來源是 **APB1 timer clock**。  
   - CK_INT 會直接送入 **PSC (Prescaler)**，有時也稱為 CK_PSC。  

2. **PSC 分頻**  
   - CK_INT 經過 PSC 分頻後，輸出 **CK_CNT**，作為計數器 CNT 的計數時脈。  

3. **CNT 遞增**  
   - CNT 以 CK_CNT 為週期加 1，直到達到 **ARR (Auto-reload Register)** 所設定的上限值。  

4. **更新事件 (UEV)**  
   - 當 CNT 達到 ARR 時，會自動產生 **更新事件 (UEV)**，並將 CNT 重置為 0，開始新一輪計數。  

透過這個流程，定時器就能週期性地產生 **tick**，作為系統的時間基準，或用來驅動外設（例如 DAC、LED 閃爍、週期性任務）。  

#### 控制位元

- **CEN 位元**（位於 `TIMx_CR1` 暫存器）  
  - 控制計數器啟動。  
  - `CEN = 1` → 啟動計數器，PSC 開始由 CK_INT 提供時脈。  
  - `CEN = 0` → 停止計數。 

- **UG 位元**（位於 `TIMx_EGR` 暫存器）  
  - 軟體觸發更新事件的控制位元。  
  - `UG = 1` → 立即強制產生一次 **更新事件 (UEV)**，將 PSC/ARR 的預載值載入，並在設定後自動清除。  

- **UIF 位元**（位於 `TIMx_SR` 暫存器）  
  - 狀態旗標，用來標記 **更新事件 (UEV)** 是否發生。  
  - 會在以下情況由硬體自動設為 1：  
    1. **計數器 CNT 溢位**（達到 ARR 並回到 0）。  
    2. **軟體觸發 UG**（`TIMx_EGR.UG = 1`，且 `URS=0`）。  
    3. 其他會產生 UEV 的情境（例如觸發模式下）。  
  - UIF 必須由 **軟體寫 0** 來清除（寫 1 無效）。  
  - 若 `UIE=1`，UIF=1 時會產生 **更新中斷**。

#### 更新事件 (UEV)

更新事件 (Update event, UEV) 是一種「狀態／行為」，其來源可以是：  
- **CNT 達到 ARR 溢位**  
- **UG 被軟體觸發**  

當 UEV 發生時，硬體會自動執行以下動作：  

- 將 **PSC / ARR 的預載值** 寫入實際暫存器。  
- 將 **CNT 重置為 0**（若為向上計數模式）。  
- 設定 **UIF 旗標**（位於 `TIMx_SR` 狀態暫存器）。  
- 若啟用了 `UIE`，則觸發 **中斷**；若啟用了 DMA，則觸發 **DMA 請求**。  

---

## 7.2 功能描述  

### 時間基準單元 (Time-base unit)

時間基準單元主要由以下三個暫存器組成：  

- **計數器暫存器 (Counter register, TIMx_CNT)**  
- **預分頻器暫存器 (Prescaler register, TIMx_PSC)**  
- **自動重載暫存器 (Auto-reload register, TIMx_ARR)**  

---

#### Buffered register（具緩衝的暫存器）

**Buffered register**，也就是常說的 **預載式暫存器 (preloaded register)**。  
當軟體寫入時，數值會先存放在 **preload register**，並不會立即影響硬體運作；只有在特定的 **事件（例如 Update Event, UEV）**、**同步信號**，或 **觸發 reload 動作**時，才會搬移至 **shadow register**，並真正生效。  

典型例子包括 Timer 的 **ARR、PSC**，以及 LTDC 的部分設定暫存器。這些暫存器具有 **preload register** 與 **shadow register** 的雙層結構：  

- **Preload register**：程式寫入時的暫存空間，等待同步事件觸發。  
- **Shadow register**：硬體實際運作時所使用的暫存器值。  

這種設計可避免外設在運作過程中因數值立即變動而造成不穩定，確保系統行為的連續性與可靠性。  
因此，**軟體讀取到的值（preload）** 與 **硬體實際使用的值（shadow）** 在某些時刻可能不同步，直到同步事件發生後才會一致。  

---

#### 自動重載暫存器 (ARR)

ARR 是 **預載式 (preloaded)** 暫存器。  

- 當程式讀寫 ARR 時，實際上訪問的是 **preload register**。  
- 預載值會在「更新事件 (UEV)」發生時，或根據 `TIMx_CR1` 中 **ARPE 位元**的設定，轉移到 **shadow register** 才會生效。  
- 更新事件來源可能是 **計數器溢位 (overflow)**，或是軟體觸發（若 `UDIS` 位元允許）。  

---

#### 計數器與啟動條件

- 計數器由 **PSC 分頻後的時脈 (CK_CNT)** 驅動。  
- 只有當 `TIMx_CR1` 中的 **CEN = 1** 時，CK_CNT 才會啟用並推動 CNT 遞增。  

---

#### 預分頻器描述 (Prescaler description)

- 預分頻器本身是一個 **16 位元計數器**，由 **TIMx_PSC 暫存器**設定分頻值。  
- 分頻範圍為 **1 到 65536**。  
- 計算公式：  

  ```
  CK_CNT = fCK_PSC / (PSC[15:0] + 1)
  ```

- 由於 PSC 具備 **buffered 機制**，因此分頻值可以「即時 (on-the-fly)」更改。  
- 新的分頻值會在下一個 **更新事件 (UEV)** 發生時才真正生效。  

---

### 時鐘來源 (Clock source)

在 STM32 的 Timer 架構裡，**CK_INT** 是定時器的內部時鐘來源 (internal clock)。  
它不是獨立震盪器，而是由 **RCC** 提供的 **APB1/APB2 匯流排時鐘 (PCLK1/PCLK2)**。  
其中 **TIM6/TIM7** 掛在 **APB1**，因此其 CK_INT 來自 **APB1 的計時器時鐘**。

在 RCC 的 *Clocks* 小節（7.2）有明文規則：定時器時鐘頻率由硬體自動決定，分為兩種情況：  
- **APB 預分頻器 = 1**：`TIMxCLK = PCLKx`  
- **APB 預分頻器 ≠ 1**：`TIMxCLK = 2 × PCLKx`  （此規則只適用 **Timer**，如 TIM1/…；USART/SPI/I2C 等一般外設仍用 PCLK 本身）

#### 範例

若系統時脈 **System clock = 180 MHz**  

- AHB 分頻 = 1 → **HCLK = 180 MHz**  
- APB1 分頻 = 4 → **PCLK1 = 45 MHz**  
- 由於 APB1 分頻 ≠ 1 → **Timer clock = PCLK1 × 2 = 90 MHz**  

因此，這個 **90 MHz** 就是 TIM6/TIM7 的 **CK_INT**（內部時鐘來源）。

---

### 計數模式 (Counting mode)

#### 計數流程
- 計數器從 **0** 開始遞增 → 達到 **ARR (Auto-reload Register)** 的值 → 產生 **計數器溢位事件 (counter overflow event)** → CNT 重新歸零並繼續計數。

---

#### 更新事件 (Update Event, UEV)

- **自動產生**：計數器 CNT 溢位時自動產生 UEV。  
- **軟體觸發**：`TIMx_EGR.UG = 1` 會強制產生一次 UEV。  
- **禁止條件**：若 `TIMx_CR1.UDIS = 1`，則 UEV 不會更新 shadow register。  
  - 此時 **counter 與 prescaler counter** 仍會從 0 重新開始。  
  - 但 **PSC 的分頻比** 不會重新載入，維持不變。  

---

#### URS (Update request selection)

- `URS = 0` → 每次 UEV 都會設置 **UIF 旗標**，並可能觸發 **中斷 / DMA**。  
- `URS = 1` → 軟體透過 **UG** 產生的 UEV 不會設置 UIF，因此不會觸發中斷 / DMA。  

---

#### 更新事件的效果

當 **UEV** 發生時，硬體會執行以下動作：  

1. **PSC shadow register** ← 載入 **PSC preload 值 (TIMx_PSC)**  
2. **ARR shadow register** ← 載入 **ARR preload 值 (TIMx_ARR)**  
3. **UIF 旗標**（位於 `TIMx_SR`）會被設置（是否設置取決於 `URS` 位元）。  

---

## 7.3 實作：以 TIM2 建立 1 µs 時基與時間差量測

本專案希望建立一個 **1 µs 解析度** 的時基，並提供可隨時讀取目前計數值的 API：  
`uint32_t timer_micros_now(void)`。  
設計目標是在每次進入主迴圈時讀取一次時間戳，並以「**本次 − 上次**」的**無號差值**計算經過時間（microseconds）：
```c
delta_us = (uint32_t)(now - prev);  // 使用無號減法，自然處理回繞
```
之後以 LED 閃爍作為示例應用。

原本考慮使用 **Basic timer（TIM6/TIM7）**；然而其 **ARR/CNT 為 16-bit**，即使把計數時脈設為 1 MHz（1 tick=1 µs），也會在 **約 65.536 ms** 回繞一次。若在主迴圈以「環形差值」累積時間，就必須 **保證迴圈 < 65.536 ms** 才不會漏掉回繞。

因此本節改採 **General-purpose timer 的 TIM2**。TIM2 之 **CNT/ARR 為 32-bit**，同樣設為 1 MHz 時計數在 **約 4294.967 秒（約 71.6 分）** 才回繞：  
- 主迴圈無需頻繁讀取，也較不易遺漏回繞。  
- 無號減法 `(uint32_t)(now - prev)` 天然處理回繞，邏輯更單純。

> 具體作法：以 **APB1 計時器時鐘**（本專案為 90 MHz）驅動 TIM2，設定 `PSC=89` → `CK_CNT=90 MHz/(89+1)=1 MHz`、`ARR=0xFFFFFFFF`，讓計數器自由運行作為全域微秒時間戳來源。

````c
void timer_init(void) {
	rcc_enable_apb1_clock(RCC_APB1EN_TIM2EN);

    timer_set_psc(TIMER2, 89);            // CK_CNT = 1 MHz
    timer_set_arr(TIMER2, 0xFFFFFFFFu);   // 32-bit free-running

    timer_cr1_set_field(TIMER2, TIMx_CR1_URS, 1); // avoid set UIF
    timer_egr_set_field(TIMER2, TIMx_EGR_UG, 1);  // update the PSC & ARR
    timer_cr1_set_field(TIMER2, TIMx_CR1_URS, 0);

    timer_cr1_set_field(TIMER2, TIMx_CR1_CEN, 1); // start
}

	uint8_t  led_state = 0;
	static const uint32_t LED_PERIOD_US = 2000000U;
	uint32_t next_deadline = micros_now(TIMER2) + LED_PERIOD_US;
  while (1) {
    uint32_t now_us = micros_now(TIMER2);
    int32_t  diff = (int32_t)(now_us - next_deadline);

    if (diff >= 0) {
      uint32_t missed = (uint32_t)diff / LED_PERIOD_US + 1u;

      next_deadline += missed * LED_PERIOD_US;
      led_state ^= 1;
      gpio_set_outdata(GPIOG_BASE, GPIO_PIN_13, led_state);

      if (led_state == 0)
        fill_framebuffer_rgb888(0x4B0082);
      else
        fill_framebuffer_rgb888(0x0000FF);
    }

}
````

---

# 8. 觸控面板（Touch Panel）— STMPE811

## 8.1 GPIO 與 EXTI 初始化

- **I2C3**：  
  - **PA8 = SCL**、**PC9 = SDA** → 皆設 **AF4 / Open-Drain / High speed / 無內部上拉**（板上已有 4.7 kΩ 上拉）。  
- **中斷 INT**：  
  - **PA15 = TP_INT1** → 設 **Input / 無內部上拉**，**EXTI falling-edge** 觸發。  
  - 注意：STMPE811 的 INT 為 **active-low**，直到 I²C 讀並清除控制器的中斷狀態才會釋放。 

```c
// I2C3 GPIO: PA8=SCL, PC9=SDA
void i2c3_gpio_init(void) {
    // GPIOA / GPIOC clock
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
    rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOC);

    // PA8 -> I2C3_SCL (AF4, OD, High, no pull)
    gpio_set_mode(GPIOA_BASE, GPIO_PIN_8, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOA_BASE, GPIO_PIN_8, ALTERNATE_AF4);
    gpio_set_output_type(GPIOA_BASE, GPIO_PIN_8, GPIO_OTYPE_OPENDRAIN);
    gpio_set_pupdr(GPIOA_BASE, GPIO_PIN_8, GPIO_PUPD_NONE);
    gpio_set_speed(GPIOA_BASE, GPIO_PIN_8, GPIO_SPEED_HIGH);

    // PC9 -> I2C3_SDA (AF4, OD, High, no pull)
    gpio_set_mode(GPIOC_BASE, GPIO_PIN_9, GPIO_MODE_ALTERNATE);
    gpio_set_alternate_function(GPIOC_BASE, GPIO_PIN_9, ALTERNATE_AF4);
    gpio_set_output_type(GPIOC_BASE, GPIO_PIN_9, GPIO_OTYPE_OPENDRAIN);
    gpio_set_pupdr(GPIOC_BASE, GPIO_PIN_9, GPIO_PUPD_NONE);
    gpio_set_speed(GPIOC_BASE, GPIO_PIN_9, GPIO_SPEED_HIGH);
}

void exti_gpio_init(void) {
	rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);
	gpio_set_mode(GPIOA_BASE, GPIO_PIN_15, GPIO_MODE_INPUT);
  gpio_set_pupdr(GPIOA_BASE, GPIO_PIN_15, GPIO_PUPD_NONE);
}

void exti_init(void) {
	exti_gpio_init();

  // Map EXTI line15 to port A, enable falling trigger
  exti_select_port(SYSCFG_EXTI15, SYSCFG_EXTICR_PORTA);
  exti_enable_falling_trigger(SYSCFG_EXTI15);
  exti_set_interrupt_mask(SYSCFG_EXTI15, EXTI_INTERRUPT_ENABLE);
  exti_clear_pending_flag(SYSCFG_EXTI15); // clear EXTI line15 pending

  // NVIC for EXTI lines 10..15
  nvic_enable_irq(EXTI15_10_IRQn);
}
```

---

## 8.2 STM32F429 I²C 介紹  

STM32F429 的 I²C 介面可以同時支援 **Master** 與 **Slave** 模式，具備多主機能力 (Multimaster Capability)。  

---

### 8.2.1 I²C 基本特性  

#### 1. 匯流排連接與傳輸速度  
I²C 介面透過資料腳位 **SDA** 與時脈腳位 **SCL** 連接至匯流排，支援：  
- **標準模式 (Standard Mode, Sm)：最高 100 kHz**  
- **快速模式 (Fast Mode, Fm)：最高 400 kHz**  

#### 2. 狀態旗標 (Status Flags)  
- 傳送器 / 接收器模式旗標  
- End-of-Byte 傳輸完成旗標  
- I²C 忙碌旗標  

#### 3. 中斷向量 (Interrupt Vectors)  
I²C 共有 **2 種中斷來源**：  
- 位址 / 資料傳輸成功中斷  
- 錯誤狀態中斷  

#### 4. 工作模式 (Operating Modes)  
I²C 介面可運作於下列四種模式之一：  
- **Slave transmitter** (從機傳送器)  
- **Slave receiver** (從機接收器)  
- **Master transmitter** (主機傳送器)  
- **Master receiver** (主機接收器)  

預設情況下，I²C 介面運作於 **Slave 模式**。  
- 當產生 **START 條件** 時，會自動從 Slave 切換為 Master。  
- 若發生仲裁失敗或 Stop 條件產生，則會切回 Slave 模式。

#### 5. Master 功能  
- 所有資料與位址皆以 **8-bit byte** 傳送，並以 **MSB (最高位元)** 先行。  
- 負責產生資料傳輸所需的 **時脈 (Clock generation)**。  
- 每次傳輸必須以 **Start condition** 開始，並以 **Stop condition** 結束：  
  - **Start condition**：SCL 保持高電位，SDA 由 **高 → 低**。  
  - **Stop condition**：SCL 保持高電位，SDA 由 **低 → 高**。  

在 I²C 匯流排中，每個 **Slave 裝置** 都需要一個唯一的位址 (**7-bit / 10-bit**)。  
Master 透過該位址指定要與哪個 Slave 溝通，並且支援 **General Call (廣播呼叫)**。

#### 6. 7-bit 位址模式  
在 **7-bit 模式** 下：  
- 第一個 byte 在 Start 條件後即為位址 (由 Master 傳送)。  
- 每一個 byte 傳輸需要 **8 個 SCL 週期**。  
- 傳送完 8 bits 後，會有第 **9 個 SCL 週期**，此時 Slave 需回覆 **ACK (確認位元)**。  

資料框架 (Frame) 組成：  
```
[7-bit Slave Address] + [R/W bit]
```
- R/W bit：最後一個 bit 表示 **讀取 (1)** 或 **寫入 (0)**。  

---

### 8.2.2 I²C Master 模式  

當匯流排上透過 **START 位元** 產生 Start 條件時，I²C 介面會進入 **Master 模式**。  

在此模式下，外設輸入時脈頻率至少需為：  
- **2 MHz**（標準模式 Sm）  
- **4 MHz**（快速模式 Fm）  

#### Master 模式設定流程  

1. 在 **I2C_CR2** 暫存器中設定外設輸入時脈，確保定時正確。  
2. 設定 **CCR (Clock Control Register)**，定義 SCL 高低電平時間。  
3. 設定 **TRISE 暫存器**，配置最大上升時間。  
4. 設定 **I2C_CR1** 暫存器，啟用外設。  
5. 在 **I2C_CR1** 暫存器中設置 **START 位元**，產生 Start 條件。  

#### SCL 主時脈產生 (SCL Master Clock Generation)  

**CCR 位元** 用於產生 SCL 的高電平與低電平，分別由上升沿與下降沿控制。  

由於 Slave 可能進行 **Clock Stretching**（拉低 SCL 延遲傳輸），因此 MCU 會在 **TRISE 暫存器** 所設定的時間後，檢查匯流排上的 SCL 狀態：  
- **若 SCL 為低電位**：表示有 Slave 正在拉住匯流排，高電平計數器會暫停，直到偵測到 SCL 為高電位，以確保 **SCL 最小高電平時間**。  
- **若 SCL 為高電位**：高電平計數器持續運行。  

SCL 時脈產生過程中會有 **迴路延遲**，即使沒有 Slave 進行 Clock Stretching 也存在。延遲來源包括：  
- **SCL 上升時間**（影響 VIH 偵測）  
- **SCL 輸入端濾波器延遲**  
- **內部 SCL 與 APB 時脈同步延遲**  

**TRISE 暫存器** 設定最大允許延遲，確保 SCL 頻率穩定，不受實際 SCL 上升時間影響。  

#### 起始條件 (Start Condition)  

設定 **START 位元** 會使 I²C 介面產生 Start 條件，並在 **BUSY 位元清除**時切換到 **Master 模式 (MSL 位元設置)**。  

一旦 Start 條件送出：  
- **SB 位元** 會由硬體自動設置。  
- 若 **ITEVFEN 位元** 已啟用，則會觸發中斷。  
- 接著 Master 需先讀取 **SR1 暫存器**，然後將 **Slave 位址** 寫入 **DR 暫存器**。  

#### Slave 位址傳送 (Slave Address Transmission)  

Slave 位址會透過內部移位暫存器 (Shift Register) 傳送至 SDA 線，在 **7-bit 位址模式** 下：  
1. 傳送一個位址 byte。  
2. 當位址傳送完成後，**ADDR 位元** 由硬體自動設置。
3. Master 需先讀取 **SR1 暫存器**，再讀取 **SR2 暫存器**。  

Master 會依據 **Slave 位址的 LSB 位元** 決定進入哪種模式：  
- **LSB = 0** → 進入 **Transmitter 模式**。  
- **LSB = 1** → 進入 **Receiver 模式**。  

**TRA 位元** 用來指示 Master 當前處於 **Transmitter** 還是 **Receiver 模式**。  

#### Master 傳送模式 (Master Transmitter)  

1. 在位址傳送完成並清除 **ADDR** 後，Master 將資料寫入 **DR 暫存器**，再透過內部移位暫存器傳送到 **SDA 線**。  
2. Master 會等待第一個資料位元組寫入 **I2C_DR**。  
3. 當接收到 **ACK 應答** 後，**TxE 位元** 會由硬體自動設置。  
4. 若 **TxE 已設置** 但新資料尚未寫入 **DR**，而前一筆資料傳輸已完成，則 **BTF 位元** 會被設置，介面會停住，直到新資料寫入 **I2C_DR**。  
   - 在此期間，SCL 會被拉低（**Clock Stretching**）。  

當最後一個資料位元組寫入 **DR** 後：  
- 軟體必須將 **STOP 位元** 設置以產生 **Stop 條件**。  
- 此時介面會自動返回 **Slave 模式 (MSL 位元清除)**。  
- 只要再次送出 **START 條件** (設定 START 位元)，硬體就會重新進入 **Master 模式**，開始新的一次傳輸。  

##### Master 傳送模式傳輸時序 (Master Transmitter Sequence)  

```
S → EV5 → Address → A → EV6 → EV8_1 → Data1/EV8 → A → Data2/EV8 → A → … → DataN/EV8 → A → EV8_2 → P
```
- **Legend**：S= Start, P= Stop, A= Acknowledge, EVx= Event (with interrupt if ITEVFEN = 1)
- **EV5**：SB=1，由讀取 SR1 並寫入 DR (地址) 清除。  
- **EV6**：ADDR=1，由讀取 SR1 與 SR2 清除。  
- **EV8_1**：TxE=1，移位暫存器為空，資料暫存器為空，寫入 Data1 至 DR。  
- **EV8**：TxE=1，移位暫存器非空，資料暫存器為空，由寫入 DR 清除。  
- **EV8_2**：TxE=1，BTF=1，必須由程式設定 **Stop**；TxE 與 BTF 會由 **Stop 條件** 自動清除。  

**Clock Stretching**：  
- EV5、EV6、EV8_1 與 EV8_2 事件在軟體處理完成前，SCL 會保持拉低。  
- 若 EV8 事件在下一個位元組傳輸開始前尚未完成，SCL 也會保持拉低，直到處理完成。  

#### Master 接收模式 (Master Receiver)  

當 Master 發出 **從機位址 (Slave address)** 並成功收到從機的應答 (即 **ADDR 旗標清除**)，I²C 介面便進入 **Master 接收模式**。  
此時資料會透過 **SDA 線** 傳入，並存放到 **資料暫存器 (DR, Data Register)**。  

每當 Master 接收 1 byte，必須在第 9 個 clock 週期送出回應：  
- **ACK**：表示「還要更多資料」，Slave 會繼續傳送下一個 byte。  
- **NACK**：表示「不要再傳了」，Slave 會在收到 NACK 後結束傳輸。  

當 **DR 暫存器** 有新資料可讀時，**RxNE (Receive Buffer Not Empty)** 旗標會被設置 (RxNE=1)。  
此時程式必須在下一筆資料到來前，讀出 DR 的內容。  

若 **DR 尚未讀出 (RxNE=1 未清除)**，而新的資料又到達，則 **BTF (Byte Transfer Finished)** 旗標會被設置 (BTF=1)。  
為避免 DR 中的資料被覆蓋，硬體會啟動 **Clock Stretching**，將 **SCL 拉低**，直到 CPU 讀走資料為止。  

##### 結束通訊 (Closing the Communication)  

Master 在接收最後一個 byte 時，會送出 **NACK**，通知 Slave 傳輸結束。  
Slave 收到 NACK 後，會釋放對 **SCL** 與 **SDA** 的控制權。  
通常 Master 在送出 NACK 後，會緊接著產生 **STOP** 或 **REPEATED START**，以結束或重新啟動本次通訊。  

具體流程：  
1. **倒數第二個 byte**：軟體需清除 **ACK 位元**，使最後一個 byte 自動回應 **NACK**。  
2. **倒數第二個 byte 讀取後**：軟體應設置 **STOP 或 START 位元**，以便在最後一個 byte 完成後，自動產生 STOP/RESTART。  
3. **若僅接收單一 byte**：需在 **EV6 (ADDR=1)** 事件時清除 ACK，並在 EV6 後立即設置 STOP。  

當 STOP 條件產生後，介面會自動回到 **Slave 模式** (MSL 位元清除)。  

##### Master 接收模式接收時序 (Master Receiver Sequence)  

```
S → EV5 → Address → A → EV6 → Data1 → A(1) → Data2/EV7 → A → Data3/EV7 → A → … → DataN/EV7_1 → NA → P/EV7
```

- Legend: S= Start, P= Stop, A= Acknowledge, NA= Non-acknowledge,
EVx= Event (with interrupt if ITEVFEN=1)
- **EV5**：SB=1，由讀取 SR1 後寫入 DR (地址) 清除  
- **EV6**：ADDR=1，由讀取 SR1 後讀取 SR2 清除  
- **EV7**：RxNE=1，由讀取 DR 清除，同時送出 ACK  
- **EV7_1**：RxNE=1，由讀取 DR 清除，並送出 NACK 與 STOP  
- **EV9**：ADD10=1，由讀取 SR1 並寫入 DR 清除（僅 10-bit 位址模式使用）  

補充：  
- **EV5、EV6、EV9** 在軟體處理完成前，SCL 會被拉低 (Clock Stretching)。  
- **EV7** 若未在下一個 byte 到來前處理完成，SCL 會保持拉低。  
- **EV7_1** 軟體動作必須在當前 byte 的 ACK 脈衝之前完成。   

##### 接收規則與特殊情況 (Reception Rules and Special Cases)  

- 在最後一個 byte 之前，必須清除 **ACK**。  
- **STOP 位元** 必須在最後一個 byte 接收完成後設置。  

**單一 byte 接收 (1-byte Reception)**
- 在進入接收模式（EV6，ADDR=1）之後，程式必須 清除 ACK 位元，並立即設置 **STOP**。  
- 當唯一的 byte 到來時，硬體就會在第 9 個 clock 自動送出 NACK，完成接收。  

**兩個 byte 接收 (2-byte Reception)**  
1. 等待 **ADDR=1**（SCL 保持拉低，直到 ADDR 清除）。  
2. 清除 **ACK**，並設置 **POS 位元**。  
3. 讀取 **SR1、SR2**，清除 ADDR。  
4. 等待 **BTF=1**（Data1 在 DR，Data2 在移位暫存器，SCL 拉低直到 Data1 被讀走）。  
5. 設置 **STOP**。  
6. 依序讀取 **Data1、Data2**。  

**多於兩個 byte 接收 (N > 2-byte Reception)**  
1. 等待 **BTF=1**（例如 Data N-2 在 DR，Data N-1 在移位暫存器，SCL 拉低直到 Data N-2 被讀走）。  
2. 清除 **ACK**。  
3. 讀取 **Data N-2**。  
4. 再次等待 **BTF=1**（Data N-1 在 DR，Data N 在移位暫存器）。  
5. 設置 **STOP**。  
6. 依序讀取 **Data N-1、Data N**。  

---

### 8.2.3 I²C 初始化與資料傳輸實作

#### I²C 初始化

首先，必須先開啟 **I2C3 所需的 RCC APB1 Clock**。  
接著，將 **CR1.PE** 暫時清零，關閉 I²C 外設，以便安全設定定時暫存器。  

依照 **Master 模式初始化流程**，設定步驟如下：

1. **I2C_CR2.FREQ**  
   - 設定外設輸入時脈頻率 (MHz)。  
   - 本專案 APB1 = 45 MHz，因此設為 **45**。  

2. **CCR (Clock Control Register)**  
   - 定義 SCL 高低電平時間。  
   - 選擇 **Fm 模式 (F/S=1)**，並設定 **DUTY=0**。  
   - 依公式：  
     ```
     Thigh = CCR × T_PCLK1
     Tlow  = 2 × CCR × T_PCLK1
     f_SCL = 1 / (3 × CCR × T_PCLK1)
     ```  
     帶入 fPCLK1 = 45 MHz，目標 fSCL = 300 kHz：  
     ```
     CCR = 45 MHz / (3 × 300 kHz) = 50
     ```  
     因此 **CCR = 50**。  

3. **TRISE (Rise Time Register)**  
   - 設定最大允許上升時間，公式為：  
     ```
     TRISE = (允許的最大上升時間 / PCLK1 週期) + 1
     ```  
   - Fm 模式規範最大允許的 SCL 上升時間是 **300 ns**。  
   - I2C_CR2 的 FREQ = 45（APB1=45 MHz），則：  
     ```
     T_PCLK1 = 1 / 45 MHz ≈ 22.2 ns
     TRISE = (300 ns / 22.2 ns) + 1 ≈ 13.5 + 1 ≈ 14.5 → 設為 15
     ```  
     因此 **TRISE = 15**。  

4. **CR1.PE**  
   - 設定完成後，將 **PE=1**，重新啟用 I²C 外設。  

5. **啟動傳輸 (Start Condition)**  
   - 當需要開始一次通訊時，在 **I2C_CR1** 中設置 **START=1**，即可產生 Start 條件，並自動切換至 Master 模式。 

````c
void i2c_init(void) {
    const uint32_t PCLK1_MHZ  = 45u;        // APB1 peripheral clock in MHz (I2C input clock)
    const uint32_t I2C_SCL_HZ = 300000u;    // Target SCL frequency (Fast-mode 300 kHz)

    // TRISE: configure with maximum SCL rise time + 1 (Fast-mode uses 300 ns)
    // TRISE = (t_r_max / T_PCLK1) + 1 = 0.3 * PCLK1_MHz + 1
    const uint32_t I2C_TRISE_REG = 15u;     // TRISE register value for Fast-mode (APB1=45 MHz)

    // CCR formula (Fast-mode, duty=0): CCR = PCLK1 / (3 * Fscl)
    uint32_t ccr = (PCLK1_MHZ * 1000000u) / (3u * I2C_SCL_HZ);

    i2c3_gpio_init();

    // Enable I2C3 peripheral clock on APB1 bus
    rcc_enable_apb1_clock(RCC_APB1EN_I2C3EN);

    // Disable I2C before configuring timing registers
    i2c_cr1_write_field(I2C3_BASE, I2C_CR1_PE, 0);

    // CR2.FREQ must be set to APB1 clock in MHz (valid range: 2..50)
    i2c_cr2_write_field(I2C3_BASE, I2C_CR2_FREQ, PCLK1_MHZ);

    // Configure Fast-mode (FS=1), DUTY=0, CCR[11:0] = divider value
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_FS, 1);
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_DUTY, 0);
    i2c_ccr_write_field(I2C3_BASE, I2C_CCR_CCR, ccr);

    // Configure maximum rise time
    i2c_trise_write_field(I2C3_BASE, I2C_TRISE_REG);

    // Enable I2C peripheral
    i2c_cr1_write_field(I2C3_BASE, I2C_CR1_PE, 1);
}
````

#### Master Transmit，單一位元組

目標：Master 對 7-bit Slave 傳送 1 byte（無 Repeated START）。
對應時序：`S → EV5 → Address → A → EV6 → EV8_1 → Data1/EV8 → A → EV8_2 → P`
Legend：S=Start、P=Stop、A=ACK、EVx=事件（SR1/SR2 旗標）

**依照 Master 傳送模式，步驟如下：**

1. **確認匯流排空閒（BUSY=0）**

   * 檢查 `SR2.BUSY`；為 0 表示匯流排可用。

2. **產生 Start（EV5）**

   * 設 `CR1.START=1` 產生 Start。
   * 等待 `SR1.SB=1`（EV5 成立），表示已進入 Master 狀態並可送出位址。

3. **送出從裝置位址（Write）與地址回應（EV6）**

   * 寫入 `DR = (slave_addr << 1) | 0`（LSB=0 代表寫入）。
   * 等待 `SR1.ADDR=1`（EV6），代表 Slave 已回 ACK。
   * 以「**讀 `SR1` → 讀 `SR2`**」清除 `ADDR`。
   * 若偵測到 `SR1.AF=1`（NACK on address），應立刻：`CR1.STOP=1`、清 `AF`，並結束此次通訊。

4. **資料階段：寫入資料（EV8\_1 / EV8）**

   * 等 `SR1.TXE=1`（資料暫存器空，EV8\_1/EV8）。
   * 寫入 1 個資料位元組到 `DR`（本案例為單一位元組）。
   * 在資料階段亦需監看 `SR1.AF`：若為 1（NACK on data），立即 `STOP` 並清 `AF`。

5. **傳輸完成判定（EV8\_2）**

   * 等待 `SR1.TXE=1` **且** `SR1.BTF=1`（EV8\_2），代表該位元組已完成移位/送出。

6. **結束通訊（Stop）**

   * 設 `CR1.STOP=1` 產生 Stop 條件，結束此次傳輸。
   * 視需求可再等待 `SR2.BUSY=0`，確認匯流排釋放（若下一步要 Repeated START，則不必等待）。

**注意事項（實務建議）：**

* **EV6 清除順序固定為「讀 `SR1` → 讀 `SR2`」**，否則狀態機會卡住。
* **NACK 監控（`SR1.AF`）**：位址或資料任一階段出現 NACK，都應 `STOP` 並清 `AF`。
* **多位元組寫入延伸**：中間每個位元組以 `TXE=1` 節奏寫入；**最後一個位元組**等到 `EV8_2（TXE=1 且 BTF=1）` 再送 `STOP`，確保資料完整推出匯流排。
* **錯誤防護**（可選）：同時監看 `SR1.BERR`（Bus error）、`SR1.ARLO`（Arbitration lost），並在異常時送 `STOP` 清旗標。

````c
int8_t i2c_master_write(uint32_t i2c_base, uint8_t slave_addr7, uint8_t reg, uint8_t data) {
	const int8_t I2C_ERR_NACK_ADDR  = -1;
	const int8_t I2C_ERR_NACK_DATA  = -2;

	// Ensure 7-bit address
    slave_addr7 &= 0x7Fu;

    // 1) Wait until bus is free
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    // 2) Generate START, wait EV5 (SB=1)
    i2c_cr1_write_field(i2c_base, I2C_CR1_START, 1);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_SB) == 0);

    // 3) Send slave address (write : LSB = 0)
    io_write(i2c_base + I2C_DR_OFFSET, (slave_addr7 << 1) | 0u);

    // 4) Wait EV6 (ADDR=1) then clear by SR1->SR2 read
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0); // clear flag
            return I2C_ERR_NACK_ADDR;
        }
    }
    (void)io_read(i2c_base + I2C_SR1_OFFSET);
    (void)io_read(i2c_base + I2C_SR2_OFFSET);

    // 5) Wait EV8_1 (TxE=1), write reg
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }
    io_write(i2c_base + I2C_DR_OFFSET, reg);

    // 6) Wait EV8 (TxE=1), write data
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }
    io_write(i2c_base + I2C_DR_OFFSET, data);

    // 7) Wait EV8_2 (TxE=1 && BTF=1) then STOP
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_TXE) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0); // clear flag
            return I2C_ERR_NACK_DATA;
        }
    }
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0) {
        if (i2c_sr1_read_field(i2c_base, I2C_SR1_AF)) {
            i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
            i2c_sr1_write_field(i2c_base, I2C_SR1_AF, 0);
            return I2C_ERR_NACK_DATA;
        }
    }

    // 8) Generate STOP
    i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);

    // 9) wait bus released (BUSY=0)
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    return 0;
}
````

#### Master Receiver，多位元組讀取

目標：Master 從 7-bit Slave 連續接收多個位元組，並在最後一個位元組送出 **NACK + STOP** 以結束通訊。
對應時序：
`S → EV5 → Address → A → EV6 → Data1 → A → Data2/EV7 → A → Data3/EV7 → A → … → DataN/EV7_1 → NA → P`
Legend：S=Start、P=Stop、A=ACK、NA=NACK、EVx=事件（SR1/SR2 旗標）

**依照 Master 接收模式，步驟如下：**

1. **確認匯流排空閒（BUSY=0）**

   * 檢查 `SR2.BUSY`；為 0 表示匯流排可用。

2. **產生 Start（EV5）**

   * 設 `CR1.START=1` 產生 Start。
   * 等待 `SR1.SB=1`（EV5 成立），表示已進入 Master 狀態並可送出位址。

3. **送出從裝置位址（Read）與地址回應（EV6）**

   * 寫入 `DR = (slave_addr << 1) | 1`（LSB=1 代表讀取）。
   * 等待 `SR1.ADDR=1`（EV6）。
   * 清除方式為 **讀 `SR1` → 讀 `SR2`**。

4. **資料接收流程**

   * 根據欲接收的長度 `len` 分為以下三種情況：

   **(A) 單一位元組（len==1）**

   * 在清除 `EV6` 前，先設 `ACK=0`，再設 `STOP=1`。
   * 清除 `ADDR` 後等待 `RXNE=1`，讀取一個位元組。
   * 因為 `ACK=0`，硬體會在最後一個 byte 自動送 NACK 並產生 STOP。

   **(B) 兩個位元組（len==2，不使用 POS）**

   * 在清除 `EV6` 前，設 `ACK=0`。
   * 清除 `ADDR` 後，等待 `BTF=1`（兩個資料位元組已到位）。
   * 設 `STOP=1`，再**連續讀兩次 DR**，即完成接收。

   **(C) 多於兩個位元組（len>2）**

   * 清除 `EV6` 前，設 `ACK=1`，確保中間資料自動回 ACK。
   * 持續以 `RXNE=1` 節奏讀取，直到剩下 **3 個位元組**。
   * **最後 3 bytes 序列：**

     1. 等 `BTF=1` → 設 `ACK=0` → 讀取一個位元組（第 N-2）。
     2. 再等 `BTF=1` → 設 `STOP=1` → 連續讀兩次 DR（第 N-1 與 N）。
   * 此時最後一個 byte 會自動回 NACK 並產生 STOP。

5. **結束通訊（Stop）**

   * STOP 條件送出後，介面會自動回到 Slave 模式（MSL=0）。
   * 視需求可等待 `SR2.BUSY=0` 確認匯流排釋放（若要 Repeated START，則可省略）。

**注意事項（實務建議）：**

* **ACK/STOP 的配置必須提早在清 ADDR 之前完成**，否則硬體會在錯誤的 byte 送出 NACK。
* **len>2 使用「剩 3 bytes」策略**，確保最後三個位元組在正確時機切換 ACK=0 與 STOP。
* **EV6 清除順序固定為「讀 `SR1` → 讀 `SR2`」**，否則狀態機會卡住。
* 建議在應用層額外監控 `AF`（Acknowledge Failure）、`BERR`（Bus Error）、`ARLO`（Arbitration Lost），並在異常情況下送 `STOP` 收尾。

````c
int8_t i2c_master_read(uint32_t i2c_base, uint8_t slave_addr, uint8_t *data, uint16_t len) {
    if (len == 0) return 0;
    slave_addr &= 0x7Fu;

    // 1) Wait until bus is free
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    // 2) Generate START, wait EV5 (SB=1)
    i2c_cr1_write_field(i2c_base, I2C_CR1_START, 1);
    while (i2c_sr1_read_field(i2c_base, I2C_SR1_SB) == 0);

    // 3) Send slave address (read : LSB = 1)
    io_write(i2c_base + I2C_DR_OFFSET, (slave_addr << 1) | 1u);

    if (len == 1) {
        // Single byte: ACK=0, STOP=1 before clearing ADDR
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_RXNE) == 0);
        data[0] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
    }
    else if (len == 2) {
        // Two bytes: ACK=0 before clearing ADDR
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
        data[0] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        data[1] = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
    }
    else {
        // More than two bytes
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_ADDR) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 1);
        (void)io_read(i2c_base + I2C_SR1_OFFSET);
        (void)io_read(i2c_base + I2C_SR2_OFFSET);

        uint16_t remaining = len;

        while (remaining > 3) {
            while (i2c_sr1_read_field(i2c_base, I2C_SR1_RXNE) == 0);
            *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
            remaining--;
        }

        // Last 3 bytes sequence
        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_ACK, 0);
        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        remaining--;

        while (i2c_sr1_read_field(i2c_base, I2C_SR1_BTF) == 0);
        i2c_cr1_write_field(i2c_base, I2C_CR1_STOP, 1);
        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        *data++ = (uint8_t)io_read(i2c_base + I2C_DR_OFFSET);
        remaining -= 2;
    }

    // Optional: wait until BUSY=0
    while (i2c_sr2_read_field(i2c_base, I2C_SR2_BUSY));

    return 0;
}
````

---

### 8.2.4 STMPE811 Initialization

以下為觸控螢幕控制器（TSC）的初始化步驟：  

1. **Clock Enable**  
   在 `SYS_CTRL2` 暫存器中，開啟與觸控相關的時脈：  
   - **GPIO**（用於 TP_INT1 中斷腳）  
   - **TSC**（觸控螢幕控制器本體）  
   - **ADC**（觸控座標轉換需要 ADC）  
   > 不需要的 **TS (temperature sensor)** 建議保持關閉以節省電力。  

2. **Operating Mode / Tracking Index**  
   透過 `TSC_CTRL` 設定觸控螢幕的操作模式：  
   - **OP_MOD = 000** → X, Y, Z acquisition（取得座標與壓力值）  
   - **TRACK = 010** → window tracking index 設為 8，可抑制微小抖動並保持靈敏度。  
   > 注意：`OP_MOD` 與 `TRACK` 必須在 **EN=0** 狀態下設定，最後再打開 EN。  

3. **Interrupt Configuration**  
   啟用觸控偵測狀態回報：  
   - 偵測到觸控時 → **TOUCH_DET flag = 1**，`INT` pin 由高轉低觸發中斷。  
   - 觸控結束時 → flag 清除，`INT` pin 回復高電位。  
   > 主機可透過中斷接收觸控狀態，而不需要持續 polling。  

   設定方式：  
   - `INT_CTRL`：  
     - **INT_POLARITY = 0** → active-low（對應電路圖中的上拉電阻）  
     - **INT_TYPE = 1** → edge-triggered  
     - **GLOBAL_INT = 1** → 啟用全域中斷  
   - `INT_EN`：開啟 **TOUCH_DET** (bit0)  
   - `INT_STA`：寫入 `0xFF` 清除所有中斷旗標（write-1-to-clear）。  

4. **TSC Configuration**  
   在 `TSC_CFG` 設定觸控取樣與延遲：  
   - **Panel Settling Time = 100 µs**（確保面板電壓穩定）  
   - **Touch Detect Delay = 100 µs**（避免誤觸，但反應速度快）  
   - **Averaging Method = 4 samples**（抗雜訊且不影響反應速度）  

5. **FIFO Threshold**  
   在 `TSC_FIFO_TH` 暫存器設定 FIFO 門檻值，用來決定當 FIFO 中累積多少筆觸控資料時觸發中斷。  
   - 設定為 **1** → 只要有一筆新的觸控資料就觸發中斷，能立即回覆觸控座標。  
   - 設定為 **大於 1** → 等到累積多筆資料後才觸發，可減少中斷次數。  
   由於此應用需要在每次觸碰後立即回應座標，因此設定為 **1**。  

6. **FIFO Reset / Status**  
   在 `TSC_FIFO_CTRL_STA` 暫存器中有 **FIFO_RESET** 位元：  
   - **預設**：FIFO 保持在 reset 狀態。  
   - 當 `TSC_CTRL.EN = 1`（啟用 TSC）後，FIFO reset 會 **自動解除**，FIFO 開始正常運作。  

   **實務建議**（重新上電或重新配置 TSC/ADC 後）：  
   - 先將 `FIFO_RESET = 1` 清空 FIFO，再寫回 `0` 釋放 reset，確保 FIFO 為乾淨狀態。  
   - 之後正常運作期間，僅在偵測到異常（如 overflow）或變更關鍵參數時再執行一次 reset。  

   **狀態監控**：  
   - `TSC_FIFO_CTRL_STA` 亦提供 **EMPTY / FULL / TH_TRIG** 等旗標，可用 **polling** 或 **中斷** 方式監控 FIFO 狀態。

10. **Enable TSC**  
   將 `TSC_CTRL.EN` 置 1，開始觸控偵測與資料擷取。  

---

**補充：**
- **Windowing Mode**：預設情況下，視窗覆蓋整個觸控面板；若只想偵測部分區域，可設定 `TSCWdwTRX`、`TSCWdwTRY`、`TSCWdwBLX` 與 `TSCWdwBLY`。  

- **FIFO Buffer**：STMPE811 內建 FIFO（最多 128 筆座標資料）。若需支援滑動/手寫筆跡，可設定 `TSC_FIFO_TH` 作為中斷門檻。FIFO 狀態可由 `TSC_FIFO_CTRL_STA` 或中斷旗標檢查，大小可從 `TSC_FIFO_Sz` 讀取。  
  > 初始化時 FIFO 預設為 reset 狀態，啟用 TSC 後會自動解除；重新修改 TSC/ADC 參數前，建議先停用 TSC 並 reset FIFO。  

- **Auto-Hibernate**：在自動休眠模式下，觸控偵測可喚醒裝置，但僅限於 **TSC 已啟用** 且 **TOUCH_DET 中斷已啟用** 的情況。  

- **Data Access**：座標資料可透過 `TSC_DATA_X/Y/Z` 或一次性讀取 `TSC_DATA_XYZ` / `TSC_DATA` 取得，不屬於初始化流程，而是在中斷或 polling 階段使用。  

---

# 9. FreeRTOS 整合實作（空專案／無 .ioc 版）

> 目標：加檔、編譯／連結與 NVIC 設定、HAL timebase 改 TIM6、三個 handler 接管、Smoke Test 驗證，以及把現有 `while(1)` 拆成 Tasks 的落地建議。  

---

## 9.0 備份與分支

- 在 Git 開新分支：`feat/rtos-bringup`
- 保留當前可正常運作的裸機版（可隨時回退）

---

## 9.1 匯入檔案與編譯

### 9.1.1 建議目錄結構（專案樹）

```
<ProjectRoot>/
├─ Inc/
│  └─ FreeRTOS/
│     └─ FreeRTOSConfig.h        ← 自行配置的系統設定（下節提供最小可編譯模板）
├─ Src/
│  └─ FreeRTOS/
│     ├─ tasks.c
│     ├─ queue.c
│     ├─ list.c
│     ├─ timers.c
│     ├─ event_groups.c
│     ├─ heap_4.c                ← 只能保留一個 heap_x.c（此處選 heap_4.c）
│     │
│     ├─ include/                ← FreeRTOS 公共 API 標頭（整包搬過來）
│     │    ├─ FreeRTOS.h, task.h, queue.h, timers.h, event_groups.h, semphr.h, …
│     │    └─ …（FreeRTOS-Kernel/include/ 內所有 .h）
│     │
│     └─ portable/
│        └─ GCC/
│           └─ ARM_CM4F/
│                ├─ port.c
│                └─ portmacro.h
│
└─ Middlewares/（可選）          ← 若要保留完整 Kernel 原始樹可放此處
```

**備註**
- `portmacro.h` 與 `port.c` 應位於 `portable/GCC/ARM_CM4F/`，只要 include path 正確即可。
- `stream_buffer.c`、`message_buffer.c` 只有在用到對應 API 時才需要加入。
- `croutine.c` 為舊功能，通常不需使用。
- `heap_4.c` 搭配 `configTOTAL_HEAP_SIZE` 可多次 `pvPortMalloc`/`vPortFree`，適合一般應用。

完成檔案放置後，在 Project Explorer 執行一次 Refresh（F5）。

---

### 9.1.2 加入 FreeRTOSConfig.h

於 `Inc` 新建資料夾 `FreeRTOS/`，新增檔案 `FreeRTOSConfig.h`，內容如下（最小可編譯模板）：

```c
#ifndef FREERTOS_FREERTOSCONFIG_H_
#define FREERTOS_FREERTOSCONFIG_H_

#include <stdint.h>
extern uint32_t SystemCoreClock;

/*-----------------------------------------------------------
 * Kernel basic
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 7 )
#define configMINIMAL_STACK_SIZE                ( ( uint16_t ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 16 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

/*-----------------------------------------------------------
 * Hooks（先全部關掉，之後需要再開）
 *----------------------------------------------------------*/
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_MALLOC_FAILED_HOOK            0
#define configCHECK_FOR_STACK_OVERFLOW          0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/*-----------------------------------------------------------
 * 同步機制 / 其他功能
 *----------------------------------------------------------*/
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TASK_NOTIFICATIONS            1
#define configUSE_TRACE_FACILITY                0

/* 事件群組 / 計時器 */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               ( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            ( configMINIMAL_STACK_SIZE * 2 )
#define configUSE_QUEUE_SETS                    1

/* Stream/Message Buffer（可用時再開；核心 .c 你未加入也沒關係） */
#define configUSE_STREAM_BUFFERS                1
#define configUSE_MESSAGE_BUFFERS               1

/*-----------------------------------------------------------
 * 可移植層相關（STM32F4 = 4 個優先權位）
 *----------------------------------------------------------*/
#define configPRIO_BITS                         4

/* 不要改這兩個 library 值，只調整數字 15 / 5 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5  /* 5(含)以上的 IRQ 不可呼叫 FreeRTOS API */

#define configKERNEL_INTERRUPT_PRIORITY  ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* 斷言 */
#define configASSERT( x )  if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/*-----------------------------------------------------------
 * 運行時統計（不用就關）
 *----------------------------------------------------------*/
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/*-----------------------------------------------------------
 * 映射到 CMSIS 例外處理名稱
 *----------------------------------------------------------*/
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/*-----------------------------------------------------------
 * 可選：記憶體分配模式（靜態/動態）
 *----------------------------------------------------------*/
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

#endif /* FREERTOS_FREERTOSCONFIG_H_ */
```

---

### 9.1.3 設定 Compiler Include Paths

在 Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths 加入：

1. **專案頭檔路徑**  
   ```
   <ProjectRoot>/Inc
   ```
   放置自己的 `.h`（gpio.h, spi.h, …）。

2. **FreeRTOS 核心標頭**  
   ```
   <ProjectRoot>/Src/FreeRTOS/include
   ```
   這裡有 `FreeRTOS.h, task.h, queue.h, timers.h …`

3. **移植層 (port layer)**  
   ```
   <ProjectRoot>/Src/FreeRTOS/portable/GCC/ARM_CM4F
   ```

4. **FreeRTOSConfig.h 路徑**  
   ```
   <ProjectRoot>/Inc/FreeRTOS
   ```

最後點 **Apply and Close**，接著先點 **Clean Project**  
→ 清掉舊的 object 檔與依賴檔，避免殘留。  
再點 **Build Project**  
→ 完整重新編譯，確認 FreeRTOS 的 `.c` 和 `.h` 都能正確找到。

---

## 9.2



















## 9.1 先看三個「必須條件」（Must-have）

1. **三個例外接到 RTOS**：  
   `SVC_Handler → vPortSVCHandler`、`PendSV_Handler → xPortPendSVHandler`、`SysTick_Handler → xPortSysTickHandler`
2. **SysTick 給 RTOS、HAL 改 TIM6**（建議）：避免 HAL 與 RTOS 抢 Tick。  
3. **NVIC 優先權規則**（F429 為 4 bits）：  
   - `configKERNEL_INTERRUPT_PRIORITY = 15（最低）`  
   - `configMAX_SYSCALL_INTERRUPT_PRIORITY = 5`（建議）  
   - **凡會呼叫 `…FromISR()` 的 IRQ**，其**優先權數值**要 **≥ 5**（數值越大＝優先序越低）。

## 9.2 檔案與路徑（Files & Include Paths）

**把 STM32CubeF4 套件裡的 FreeRTOS 源碼加入你的專案：**

- `Middlewares/Third_Party/FreeRTOS/Source/`
  - 必要：`tasks.c`, `queue.c`, `list.c`, `timers.c`, `event_groups.c`
  - 視使用：`stream_buffer.c`（用到 StreamBuffer 才加）
- `Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/`
  - `port.c`, `portmacro.h`
- 記憶體配置（擇一，建議 `heap_4.c` 或 `heap_5.c`）  
  - `Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c`
- （**若要** CMSIS-RTOS v2 介面）
  - `Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c`
  - `…/CMSIS_RTOS_V2/cmsis_os2.h`

**專案 Include Paths 增加：**
- `…/FreeRTOS/Source/include`
- `…/FreeRTOS/Source/portable/GCC/ARM_CM4F`
- （若用 CMSIS-RTOS v2）`…/FreeRTOS/Source/CMSIS_RTOS_V2`

**新增 `FreeRTOSConfig.h`**（放在你自己的 `Inc/` 或 `Core/Inc/`）

---

## 9.3 編譯 / 連結設定（Toolchain）

**C 語言編譯旗標（至少）：**
- `-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- 若任務會用到 `printf` 浮點：可加 `-u _printf_float`（可選，檔案體積會變大）

**Linker script（.ld）檢查：**
- RAM 空間足以擺 `FreeRTOS heap`（`configTOTAL_HEAP_SIZE`）與各 Task stack。  
- **不要**再使用 C 標準庫的 `malloc` 給任務；FreeRTOS 自己的 `pvPortMalloc` 才是主力。  
- 若會在多 Task 中使用 newlib（printf 等），建議 `configUSE_NEWLIB_REENTRANT = 1`。

---

## 9.4 三個 Handler 接管（Startup / IT 對接）

**方式 A｜改啟動檔 `startup_stm32f429xx.s`**  
把向量表中的：
- `SVC_Handler`      → `vPortSVCHandler`
- `PendSV_Handler`   → `xPortPendSVHandler`
- `SysTick_Handler`  → `xPortSysTickHandler`

**方式 B｜在 `stm32f4xx_it.c` 包裝（Wrapper）**  
> 若你不想改啟動檔，可在 IT 檔案中包裝三個同名函式：

````c
void SVC_Handler(void)    { vPortSVCHandler(); }
void PendSV_Handler(void) { xPortPendSVHandler(); }
void SysTick_Handler(void){ xPortSysTickHandler(); }
````

> ⚠️ 注意：**不要**同時存在兩份 `SysTick_Handler`（HAL 舊 timebase + RTOS），否則會衝突。

---

## 9.5 HAL Timebase 改 TIM6（讓 SysTick 專心給 RTOS）

作法一（推薦）：新增檔案 `stm32f4xx_hal_timebase_tim.c`，內容參考 ST 範本，讓 HAL 用 TIM6 每 1ms 觸發：

````c
/* 範例語意：初始化 TIM6 為 1kHz，並在 TIM6_DAC_IRQHandler 呼叫 HAL_IncTick() */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
  __HAL_RCC_TIM6_CLK_ENABLE();
  TIM_HandleTypeDef htim6;
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = (SystemCoreClock / 1000000) - 1; // 1 MHz
  htim6.Init.Period    = 1000 - 1;                        // 1 kHz
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  HAL_TIM_Base_Init(&htim6);
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 15, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  HAL_TIM_Base_Start_IT(&htim6);
  return HAL_OK;
}

void TIM6_DAC_IRQHandler(void){
  if (__HAL_TIM_GET_FLAG(&htim6, TIM_FLAG_UPDATE) != RESET){
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    HAL_IncTick(); // 交回 HAL 1ms 計數
  }
}
````

作法二：覆寫 `HAL_InitTick()` 令其使用 TIM6；**勿再用 SysTick 當 HAL timebase**。

---

## 9.6 `FreeRTOSConfig.h`（最小可用版 + 建議強化）

````c
#pragma once
#include "stm32f4xx.h"

/* --- 核心行為 --- */
#define configUSE_PREEMPTION            1
#define configCPU_CLOCK_HZ              ( SystemCoreClock )
#define configTICK_RATE_HZ              ( 1000 )
#define configMAX_PRIORITIES            7
#define configMINIMAL_STACK_SIZE        128        /* 單位：words */
#define configTOTAL_HEAP_SIZE           ( ( size_t )( 32 * 1024 ) )

/* --- 同步/計時 --- */
#define configUSE_MUTEXES               1
#define configUSE_COUNTING_SEMAPHORES   1
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       2
#define configTIMER_QUEUE_LENGTH        5
#define configTIMER_TASK_STACK_DEPTH    256

/* --- 中斷優先權（F429=4 bits）--- */
#define configPRIO_BITS                 4
#define configKERNEL_INTERRUPT_PRIORITY      ( 15 << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY ( 5  << (8 - configPRIO_BITS) )

/* --- Hook/Debug（建議打開）--- */
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configASSERT(x)                 if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
#define configUSE_MALLOC_FAILED_HOOK    1
/* 若多 Task 使用 newlib 函式庫（printf 等），打開這個： */
// #define configUSE_NEWLIB_REENTRANT      1

/* --- Cortex-M4F Port 對應 --- */
#define xPortPendSVHandler  PendSV_Handler
#define vPortSVCHandler     SVC_Handler
/* SysTick 由 RTOS 使用；HAL timebase 請改 TIM6。 */
````

> **Heap 選擇**：  
> - `heap_4.c`：最佳化連續配置/釋放（一般建議）  
> - `heap_5.c`：支援多區域存放（若你要把 heap 放到多塊 RAM）

---

## 9.7 最小 `main()` 骨架（拆你的 while(1) 成 Tasks）

> **重點**：stack size 參數單位是 **words**（非 bytes）。M4F 一個 word=4 bytes。

````c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* IPC */
SemaphoreHandle_t semTouch, semButton;
SemaphoreHandle_t i2cMutex;

/* Tasks */
static void Task_Touch(void* arg){
  for(;;){
    xSemaphoreTake(semTouch, portMAX_DELAY);
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
    /* 讀 I2C 觸控、清 FIFO/INT、更新狀態 */
    xSemaphoreGive(i2cMutex);
  }
}

static void Task_UI(void* arg){
  TickType_t t = xTaskGetTickCount();
  const TickType_t per = pdMS_TO_TICKS(33); // ~30Hz
  for(;;){
    /* 觸控超時邏輯、lcd_update() 重繪 */
    vTaskDelayUntil(&t, per);
  }
}

static void Task_Button(void* arg){
  for(;;){
    xSemaphoreTake(semButton, portMAX_DELAY);
    /* 去彈跳 + 模式切換 */
  }
}

int main(void){
  HAL_Init();
  SystemClock_Config();
  /* init GPIO/USART/SPI/I2C/LTDC/EXTI/TIM6(timebase)/TIM7(if used)… */

  /* 建 IPC */
  semTouch  = xSemaphoreCreateBinary();
  semButton = xSemaphoreCreateBinary();
  i2cMutex  = xSemaphoreCreateMutex();

  /* 建 Tasks（stack=words, prio 越大越高） */
  xTaskCreate(Task_Touch,  "touch",   768,  NULL, 3, NULL);
  xTaskCreate(Task_UI,     "ui",     1280,  NULL, 2, NULL);
  xTaskCreate(Task_Button, "button",  512,  NULL, 2, NULL);

  vTaskStartScheduler(); /* 啟動後不應返回 */
  for(;;){}              /* 若跑到這裡，多半是 heap 不夠或 handlers 未接 */
}
````

---

## 9.8 IRQ 端改 `…FromISR()`（僅送訊號，不做重活）

````c
void EXTI0_IRQHandler(void){
  BaseType_t hpw = pdFALSE;
  /* 清 EXTI 狀態位 … */
  xSemaphoreGiveFromISR(semButton, &hpw);
  portYIELD_FROM_ISR(hpw);
}

void EXTI15_10_IRQHandler(void){
  BaseType_t hpw = pdFALSE;
  /* 清 EXTI 狀態位 / 讀觸控 INT pin … */
  xSemaphoreGiveFromISR(semTouch, &hpw);
  portYIELD_FROM_ISR(hpw);
}

/* 若保留 TIM7 作節拍，也請改成 FromISR 通知/給信號即可 */
````

---

## 9.9 NVIC 優先權設定範例（F429）

> 原則：**會呼叫 `…FromISR()` 的 IRQ**，**優先權數值**要 ≥ 5。  
>（數值越大＝優先序越低；Kernel=15 最低）

````c
/* 範例：EXTI 與 TIM7 都可能 FromISR → 設成 5（或更低優先序，如 6~15） */
HAL_NVIC_SetPriority(EXTI0_IRQn,      5, 0);
HAL_NVIC_EnableIRQ(EXTI0_IRQn);

HAL_NVIC_SetPriority(EXTI15_10_IRQn,  5, 0);
HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

HAL_NVIC_SetPriority(TIM7_IRQn,       6, 0); // 若用 FromISR，數值也需 ≥ 5
HAL_NVIC_EnableIRQ(TIM7_IRQn);

/* 不會呼叫 FromISR 的高頻/關鍵 IRQ（如 LTDC 畫面同步）可用較高優先序（數值小於 5），
   但請避免在這些高優先序 IRQ 內呼叫任何 RTOS API。 */
````

---

## 9.10 Smoke Test（上電快速驗證：排程 + FromISR）

````c
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static SemaphoreHandle_t semButton;

static void TaskBlink(void *arg){
  TickType_t t0 = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(500);
  for(;;){
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    vTaskDelayUntil(&t0, period);
  }
}

static void TaskButton(void *arg){
  for(;;){
    xSemaphoreTake(semButton, portMAX_DELAY);
    for(int i=0;i<3;i++){
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

int main(void){
  HAL_Init(); SystemClock_Config();
  /* 確保 HAL timebase 已改 TIM6；SysTick 給 RTOS */
  /* 初始化 LED 與按鍵 EXTI（並把 EXTI 優先權數值設 >= 5） */

  semButton = xSemaphoreCreateBinary();
  xTaskCreate(TaskBlink,  "blink",  256, NULL, 1, NULL);
  xTaskCreate(TaskButton, "button", 256, NULL, 2, NULL);
  vTaskStartScheduler();
  for(;;){}
}

void EXTI0_IRQHandler(void){
  BaseType_t hpw = pdFALSE;
  __HAL_GPIO_EXTI_CLEAR_IT(BUTTON_Pin);
  xSemaphoreGiveFromISR(semButton, &hpw);
  portYIELD_FROM_ISR(hpw);
}
````

**預期：** 上電 LED 每 500ms 閃；按鍵觸發後快速閃 3 下。

---

## 9.11 常見故障與排查（Checklist）

- **`vTaskStartScheduler()` 後立刻回到 `for(;;){}`**  
  → 不是正常現象。多半是：  
  1) **heap 不夠** → 調大 `configTOTAL_HEAP_SIZE` 或減少 Task stack。  
  2) **三個 handler 未接** → SVC/PendSV/SysTick 仍指向舊 handler。  
  3) **沒有成功建立任何 Task** → 檢查 `xTaskCreate` 回傳值或加 `configASSERT`。

- **跑一陣子 HardFault**  
  → 可能：  
  1) **Port/FPU 旗標錯** → 確認 `ARM_CM4F` + `-mfpu=fpv4-sp-d16 -mfloat-abi=hard`。  
  2) 有 IRQ **優先序高於 `MAX_SYSCALL`** 卻呼叫了 `…FromISR()`。  
  3) 某 Task stack 太小 → 打開 `configCHECK_FOR_STACK_OVERFLOW=2`，觀察水位。

- **時間/延遲不準**  
  → HAL 仍在用 SysTick；請改 TIM6 當 HAL timebase。

- **printf 亂序/卡頓**  
  → 把大量列印放到 **Logger Task**；或開 `configUSE_NEWLIB_REENTRANT=1`。

---

## 9.12 把你現有程式搬到 RTOS（對照你專案）

- **Task_Touch（prio=3, 768 words）**  
  等 `semTouch` → `i2cMutex` 保護 → 讀座標/清 FIFO & INT → 更新狀態。
- **Task_UI（prio=2, 1280 words）**  
  `vTaskDelayUntil(33ms)` 設定固定刷新 → `lcd_update()` → 處理觸控超時。
- **Task_Button（prio=2, 512 words）**  
  等 `semButton` → `button_handle_event()` 去彈跳與模式切換。
- **互斥鎖**：所有 I²C 交易（包含觸控）一律 `xSemaphoreTake(i2cMutex)` 包住。  
- **IRQ 改 FromISR**：`EXTI0`/`EXTI15_10`/（若用）`TIM7`。  
- **移除 busy-wait**：任務用 `vTaskDelay/Until`；ISR 不延遲、不印 log。

---

## 9.13 可選強化（之後再加）

- **Software Timers**：把週期性任務換 RTOS Timer，減少 Task 數。  
- **DMA**：I²C/SPI 搭配 DMA + 二重緩衝（double buffer），降低 CPU 佔用。  
- **Cache（若換到 F7/H7）**：記得 DMA 前後做 D-Cache Clean/Invalidate。  
- **斷言/Hook**：實作 `vApplicationMallocFailedHook()`、`vApplicationStackOverflowHook()` 輔助除錯。

---

### 一句話總結
> **把   做齊 + 三個 Must-have 檢查到位**（handlers、TIM6 timebase、NVIC 優先權），先用 **Smoke Test** 亮燈驗證，接著把 `while(1)` 拆成 `Touch/UI/Button` 三個 Task，再把 EXTI 改成 `…FromISR()` 丟信號——**你的 FreeRTOS 就會順利跑起來**。  
> 有卡關，回到「三個必須條件」對表檢查，十之八九在那裡。






# 最短路徑（Zero→Run）

## 里程碑 M0：備份與分支

* 建新分支：`feat/rtos-bringup`
* 保留現在可正常跑的裸機版本（隨時可回退）

## 里程碑 M1：匯入檔案與編譯通過（未啟動排程）

* 加入 `tasks.c/queue.c/list.c/timers.c/event_groups.c/heap_4.c`、`portable/GCC/ARM_CM4F/port.c`、必要 headers。
* 加入 include paths（`Source/include`、`portable/GCC/ARM_CM4F`）。
* 編譯旗標：`-mthumb -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`。
* 新增 `FreeRTOSConfig.h`（用我給你的最小版）。
  **✅ 成功判定**：專案能完整編譯連結（尚未啟動 RTOS）。

## 里程碑 M2：三個 handler 接管 + HAL timebase 改 TIM6

* `SVC/PendSV/SysTick` 導到 `vPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler`（啟動檔或 wrapper 二選一）。
* 新增 `stm32f4xx_hal_timebase_tim.c` 或覆寫 `HAL_InitTick()`，讓 **HAL 用 TIM6 1kHz**。
  **✅ 成功判定**：仍可編譯；上電後無異常卡住；`HAL_GetTick()` 正常跳動、且 **SysTick 沒被 HAL 佔用**。

## 里程碑 M3：Smoke Test（驗證排程 + FromISR）

* 建兩個任務：`TaskBlink` 每 500ms 閃 LED；`TaskButton` 等 `semButton` 後快閃 3 下。
* EXTI 按鍵 IRQ 用 `xSemaphoreGiveFromISR(...); portYIELD_FROM_ISR(...)`。
* NVIC：`configPRIO_BITS=4`；`KERNEL_PRIORITY=15`；`MAX_SYSCALL_PRIORITY=5`；**所有會呼叫 `…FromISR()` 的 IRQ 優先權數值 ≥ 5**。
  **✅ 成功判定**：

  * 上電 LED 規律閃爍（證明 `vTaskStartScheduler()`、SysTick、排程 OK）。
  * 按鍵能喚醒 task（證明 FromISR + 優先權配置 OK）。

## 里程碑 M4：把你的 `while(1)` 拆成 Tasks

* 建 `Task_UI`（\~30Hz，用 `vTaskDelayUntil` 呼叫 `lcd_update()`，含觸控超時計時邏輯）。
* 建 `Task_Touch`（等 `semTouch` → 進 I²C 讀座標/清 FIFO/清 INT → 更新狀態；用 `i2cMutex` 保護）。
* 建 `Task_Button`（等 `semButton` → 原 `button_handle_event()` 去彈跳/模式切換）。
* EXTI0/EXTI15\_10 IRQ 改成只送 semaphore；**ISR 不做重活、不列印**。
  **✅ 成功判定**：畫面正常更新；觸控/按鍵行為與裸機版本一致或更順。

## 里程碑 M5：清理 busy-wait 與收尾

* 任務裡的 `delay_us()/忙等` 改 `vTaskDelay()/vTaskDelayUntil()`。
* 若保留 TIM7 作節拍：改為 FromISR 通知某個 Task，而非在 IRQ 內改狀態。
* 開啟 `configCHECK_FOR_STACK_OVERFLOW=2`、`configASSERT`；檢查各 Task 高水位。
  **✅ 成功判定**：連跑一段時間無 HardFault/無卡死，堆疊水位安全。

---

## 典型 Commit 切點（方便回退）

* `chore(rtos): add FreeRTOS core files and heap_4.c; config & includes`
* `feat(core): hook SVC/PendSV/SysTick to FreeRTOS; move HAL timebase to TIM6`
* `test(rtos): add blink/button smoke test with FromISR`
* `feat(app): split main loop into UI/Touch/Button tasks; add i2c mutex`
* `refactor(isr): EXTI uses semaphore only; remove busy-wait`
* `chore(cfg): tune priorities and stacks; enable overflow/malloc hooks`

---

## 風險熱點與快速排查

* **一開機就卡住**：三個 handler 沒接、或沒任何 task 建成功、或 heap 太小。
* **跑一會兒 HardFault**：用錯 `ARM_CM4F` port、FPU 旗標錯、或有 IRQ 優先序高於 `MAX_SYSCALL` 卻呼叫了 `…FromISR()`。
* **時間感怪怪的**：HAL 仍用 SysTick；改 TIM6。
* **printf 造成抖動**：集中到低優先權 Logger Task；必要時 `configUSE_NEWLIB_REENTRANT=1`。

---

## 小抄（優先權數字觀念）

* **數值越大＝優先序越低**；`Kernel=15` 最低。
* 會呼叫 `…FromISR()` 的 IRQ（EXTI/TIM7…）**數值 ≥ 5**。
* 不用 RTOS API 的高頻 IRQ（如 LTDC 同步）可設更高優先序（數值小），**但 ISR 內不得碰 RTOS**。






















































































---

# 9.

---

## 7.1 SDRAM 與 FMC 控制器簡介

STM32F429 除了內建的 SRAM、Flash 等內部記憶體外，為了擴充儲存容量與提升資料存取效率，常會透過外部介面擴接多種記憶體模組，例如 SRAM、Flash 與 SDRAM。

在 LCD 顯示應用中，由於 frame buffer 體積龐大，STM32 的內部 SRAM 通常無法容納完整畫面資料。  
為了確保畫面顯示的流暢與穩定，系統常將圖像內容暫存於 **SDRAM**，再由 **LTDC（LCD-TFT Controller）模組** 自動掃描該記憶體區塊並輸出至 LCD 螢幕。因此，MCU 必須能有效地與 SDRAM 通訊與存取。

### 7.1.1 FMC 系統架構概述

根據參考手冊第 37.1 節（FMC main features）與 37.2 節（Block diagram）描述，**FMC（Flexible Memory Controller）** 提供一個高度整合的外部記憶體控制模組，整體系統可分為三個主要區塊：

#### 1. AHB 匯流排介面與組態暫存器

- 透過系統內部的 **AHB3 匯流排** 與 MCU 核心連接。  
- 負責接收 MCU 發出的記憶體存取請求，並轉換為外部記憶體所需的協定。  
- 提供組態暫存器設定功能，可調整 SDRAM 的時序參數、啟用狀態等。  
- 輸入時脈來源為 **HCLK（主系統時脈）**。

#### 2. 三大記憶體控制器

- **NOR / PSRAM 控制器**：用於靜態記憶體（SRAM、NOR Flash）  
- **NAND / PC 卡控制器**：支援 NAND Flash 與 PCMCIA 裝置  
- **SDRAM 控制器**：支援同步動態隨機存取記憶體（SDRAM）與行動式 LPSDR SDRAM

#### 3. 外部裝置介面（I/O 腳位）

FMC 控制器經由下列腳位與外部 SDRAM 模組進行通訊：

| 腳位             | 功能說明                             |
|------------------|--------------------------------------|
| `FMC_SDCLK`      | SDRAM 時脈輸出                       |
| `FMC_SDNCAS`     | SDRAM 列位址選擇（CAS）              |
| `FMC_SDNRAS`     | SDRAM 行位址選擇（RAS）              |
| `FMC_SDNWE`      | SDRAM 寫入使能                       |
| `FMC_SDCKE[1:0]` | SDRAM Clock Enable（啟用時脈）        |
| `FMC_SDNE[1:0]`  | SDRAM Chip Select（晶片使能）         |

FMC 模組的主要功能包括：

- 將 AHB 匯流排的傳輸轉換為外部記憶體協定。  
- 滿足外部記憶體的時序要求與通訊延遲。  
- 管理所有外接裝置的地址線、資料線與控制訊號。  
- 每個外部裝置透過專屬的 Chip Select（CS 腳）進行選擇。  
- FMC 同一時間僅能與一個外部裝置進行操作。

---

### 7.1.2 SDRAM 控制器功能說明

根據 37.7.1（SDRAM controller main features），STM32 所內建的 SDRAM 控制器具備下列特性：

- 支援兩組獨立組態的 SDRAM Bank  
- 支援 8-bit、16-bit、32-bit 資料匯流排寬度  
- 提供 13 位元列位址與 11 位元欄位位址，並具備 4 個內部 Bank  
- 支援多種資料存取模式：Word（32-bit）、Half-word（16-bit）、Byte（8-bit）  
- SDRAM 時脈來源可設定為 HCLK / 2 或 HCLK / 3  
- 所有時序參數皆可程式化設定  
- 內建 Auto Refresh（自動刷新）機制，且可調整刷新速率  

---

### 7.1.3 SDRAM 對外介面腳位說明

根據 RM0090 第 37.7.2 節（**SDRAM External Memory Interface Signals**），  
STM32F429 的 FMC SDRAM 控制器透過下列 I/O 腳位與 SDRAM 晶片通訊。

啟用 SDRAM 時，相關 GPIO 腳位必須設定為 **Alternate Function 12（AF12）**，以啟用 FMC 模組功能。  
未使用的腳位則可配置為其他用途。

| SDRAM 信號       | I/O 類型 | 功能說明                                                      | 對應 Alternate Function |
|------------------|----------|----------------------------------------------------------------|--------------------------|
| `SDCLK`          | O        | SDRAM 時脈輸出                                                 | 無                       |
| `SDCKE[1:0]`     | O        | Clock Enable：<br>`SDCKE0` 對應 Bank1，`SDCKE1` 對應 Bank2     | 無                       |
| `SDNE[1:0]`      | O        | Chip Enable：<br>`SDNE0` 控制 Bank1，`SDNE1` 控制 Bank2        | 無                       |
| `A[12:0]`        | O        | 列位址（Row Address）                                          | `FMC_A[12:0]`            |
| `D[31:0]`        | I/O      | 雙向資料匯流排                                                 | `FMC_D[31:0]`            |
| `BA[1:0]`        | O        | Bank Address：選擇 SDRAM 晶片內部的 4 個 bank                 | `FMC_A[15:14]`           |
| `NRAS`           | O        | Row Address Strobe（行位址觸發）                               | 無                       |
| `NCAS`           | O        | Column Address Strobe（列位址觸發）                            | 無                       |
| `SDNWE`          | O        | SDRAM 寫入使能（Write Enable）

根據上表與原理圖中的接腳配置，我們使用了以下腳位：

- `PB5 = SDCKE1`（Clock Enable 1）
- `PB6 = SDNE1`（Chip Enable 1）

可由此確定：**本專案的 SDRAM 是連接在 FMC 的 Bank2 上（對應 `FMC_SDCR2`）**。  
後續初始化流程中，也應在 `FMC_SDCMR` 設定 `CTB2 = 1` 來操作 Bank2。

---

## 7.2 SDRAM 初始化

### 7.2.1 SDRAM 記憶體 GPIO 腳位初始化（FMC 控制器）

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
void fmc_sdram_gpio_init(void)
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

### 7.2.2 SDRAM 初始化步驟

#### 開啟 AHB3 時脈

根據《STM32F429 Datasheet》的 Block Diagram，**FMC 模組掛載於 AHB3 匯流排**，因此在初始化 SDRAM 之前，需先啟用 FMC 的 AHB3 時脈。

以下為 AHB3 Clock 啟用程式：

````c
#define RCC_AHB3ENR_FMCEN (1U << 0)

void rcc_enable_ahb3_clock(void) {
    uint32_t addr = RCC_BASE + RCC_AHB3ENR;
    io_writeMask(addr, RCC_AHB3ENR_FMCEN, RCC_AHB3ENR_FMCEN); // Enable FMC clock
}
````

#### 初始化流程

根據 RM0090 §37.7.3（**SDRAM controller functional description**）說明：  
所有 SDRAM 控制器的輸出訊號（包含地址與資料）皆在 **SDRAM 時脈（FMC_SDCLK）的下降緣（falling edge）** 發生變化。

SDRAM 的初始化完全由**軟體控制**。  
若同時使用兩個 SDRAM Bank（Bank1、Bank2），則必須於初始化階段**同時發出命令**，  
這可透過在 `FMC_SDCMR` 暫存器中設定 `CTB1` 與 `CTB2` 位元完成。

> 補充：FMC 外部記憶體的位址由 MCU 固定對映，**並非接續於內部 SRAM 之後**。  

1. 設定記憶體控制參數
- 設定 `FMC_SDCRx` 暫存器中的 SDRAM 規格參數  
- 其中 **SDCLK 時脈頻率**、**突發讀取（RBURST）**、**讀取延遲（RPIPE）** 僅能設定於 `FMC_SDCR1`

2. 設定記憶體時序參數
- 設定 `FMC_SDTRx` 暫存器中的時序參數，如 TRCD、TRP、TRC  
- 其中 **TRP 與 TRC 必須設定於 `FMC_SDTR1`**

3. 發出「時脈啟用命令（Clock Configuration Enable）」
- 在 `FMC_SDCMR` 中設定 `MODE = 001` 並設置目標 Bank（CTB1 / CTB2）  
- 此命令會讓 `SDCKE` 腳位輸出高電位，開啟 SDRAM 時脈輸出

4. 等待上電延遲時間
- 延遲時間典型值為 **100 μs**，實際值請參照 SDRAM datasheet

5. 發出「Precharge All」命令
- 在 `FMC_SDCMR` 中設定 `MODE = 010`，並設置目標 Bank（CTB1 / CTB2）

6. 發出「Auto-refresh」命令
- 在 `FMC_SDCMR` 中設定 `MODE = 011`，設置目標 Bank 以及自動刷新次數（`NRFS`）  
- 通常需執行 **8 次 Auto-refresh**，實際值見 SDRAM datasheet

7. 發出「Load Mode Register」命令
- 在 `FMC_SDCMR` 中設定 `MODE = 100`，在 `MRD` 欄位中填入對應值，並設置目標 Bank  
- 此命令會將資料寫入 SDRAM 的 Mode Register

> 注意：  
> - `CAS latency` 必須與 `FMC_SDCRx` 中設定一致  
> - `Burst Length` 應設為 1，對應 `M[2:0] = 000`  
> - 若 Bank1、Bank2 使用不同 Mode Register，必須分別發出兩次命令

8. 設定 Refresh 速率
- 設定 `FMC_SDRTR` 暫存器  
- Refresh Rate 為兩次自動刷新命令間的間隔，依 SDRAM 規格決定

至此，SDRAM 初始化完成，裝置已可接受正常的讀寫命令。  

#### 初始化範例程式











---

### 7.2.3 設定 FMC_SDCR 與 FMC_SDTR 暫存器（Bank1）

從 FMC 的觀點來看，外部記憶體被劃分為 6 個固定大小為 256 MByte 的 bank，FMC 控制器分配記憶體空間來對應不同種類的外部記憶體

| Bank編號   | 用途                | 裝置類型         |
| -------- | ----------------- | ------------ |
| Bank 1   | NOR Flash 或 PSRAM | 最多 4 個裝置     |
| Bank 2-3 | NAND Flash        | 每個 bank 一個裝置 |
| Bank 4   | PC Card           |              |
| Bank 5-6 | SDRAM             | 每個 bank 一個裝置 |



由於 SDRAM 通常掛接於 **FMC 的 Bank1 子模組（SDNE0）**，其地址空間對應為：

```
0xD0000000 ~ 0xDFFFFFFF（即 HADDR[28] = 0）
```

因此我們需對 **Bank1 對應的兩個暫存器**：

- `FMC_SDCR1`：控制參數設定（如資料寬度、列/行位元數、CAS 延遲等）
- `FMC_SDTR1`：時序參數設定（如 TRCD、TRP、TRAS、TWR 等）

進行初始化，以告訴 FMC 控制器該如何與外部 SDRAM 正確通訊。

> 🔍 補充：
> - 若使用 Bank2（SDNE1）則對應暫存器為 `FMC_SDCR2` 與 `FMC_SDTR2`。
> - Bank1、Bank2 可對應不同 SDRAM 模組，但初始化邏輯相同。



| 設定項目                            | 說明                                 | 文件章節                   |
| ------------------------------- | ---------------------------------- | ---------------------- |
| `FMC_SDCRx`（你設定 SDRAM 結構與時鐘）    | SDRAM 資料寬度、行列數、CAS、是否寫保護、HCLK/2 等等 | **§37.8.4 FMC\_SDCRx** |
| `FMC_SDTRx`（你設定 SDRAM 的 timing） | SDRAM 的 TMRD、TRP、TRCD 等 timing 設定  | **§37.8.5 FMC\_SDTRx** |


以下為程式碼範例：

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

### 7.2.4 依序送出五個 JEDEC 初始化指令至 SDRAM

| 動作                                                                | 解釋                      | 對應章節                                               |
| ----------------------------------------------------------------- | ----------------------- | -------------------------------------------------- |
| 發送 Clock enable / Precharge / Auto-refresh / Load mode register 等 | 控制 SDRAM 進入工作狀態的命令流程    | ✅ **§37.3.1 SDRAM memory initialization sequence** |
| 每個命令怎麼寫進寄存器                                                       | 解釋 `FMC_SDCMR` 寫法       | ✅ **§37.8.6 FMC\_SDCMR**                           |
| 設定 Refresh Rate                                                   | 需要計算、寫入 `FMC_SDRTR` 寄存器 | ✅ **§37.8.8 FMC\_SDRTR**                           |



Clock Configuration Enable

PALL (Precharge All)

Auto-refresh（2 次）

Mode Register 設定

設定 Refresh Rate 計數器


| 你寫的 function                | 實作功能                     | 文件章節                            |
| --------------------------- | ------------------------ | ------------------------------- |
| `fmc_sdcr_write_field()`    | 告訴 FMC：我的 SDRAM 規格是什麼    | **§37.8.4 FMC\_SDCRx**          |
| `fmc_sdtr_write_field()`    | 告訴 FMC：要用哪些時序參數控制 SDRAM  | **§37.8.5 FMC\_SDTRx**          |
| `fmc_sdram_init_sequence()` | 送 JEDEC SDRAM 指令，讓晶片真正開機 | **§37.3.1 + §37.8.6 + §37.8.8** |


| 步驟     | 對應功能（你寫的程式）                                     | RM0090 正確章節                                  | 說明                                                        |
| ------ | ----------------------------------------------- | -------------------------------------------- | --------------------------------------------------------- |
| Step 1 | 設定 SDRAM 結構參數（NC, NR, CAS...）                   | `fmc_sdcr_write_field()`                     | **§37.7.5.1 FMC\_SDCRx register**                         |
| Step 2 | 設定 SDRAM 時序參數（TRCD, TRP...）                     | `fmc_sdtr_write_field()`                     | **§37.7.5.2 FMC\_SDTRx register**                         |
| Step 3 | 送初始化指令序列（Clock enable、Auto-refresh、Load mode 等） | `fmc_sdram_init_sequence()`                  | ✅ **§37.7.3.1 Initialization sequence**（🔺這就是 JEDEC 指令步驟） |
| Step 4 | 寫入 SDRAM Command Register（MODE, CTB, NRFS 等欄位）  | 在 `fmc_sdram_init_sequence()` 中寫 `FMC_SDCMR` | **§37.7.5.3 FMC\_SDCMR register**                         |
| Step 5 | 設定 Refresh 週期參數                                 | 在 `fmc_sdram_init_sequence()` 中寫 `FMC_SDRTR` | **§37.7.5.4 FMC\_SDRTR register**                         |
