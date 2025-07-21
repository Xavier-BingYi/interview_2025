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

## 5.2 LTDC GPIO 腳位初始化

為了讓 LTDC 顯示模組與 SDRAM 記憶體能正常運作，必須先將相關 GPIO 腳位設定為對應的 Alternate Function 模式。STM32F429I-DISC1 所使用的 RGB LCD 採用 LTDC 模組驅動，而畫面資料則儲存在外接 SDRAM，兩者皆透過大量 GPIO 腳位與內部匯流排連接。

### 5.2.1 LTDC 的三個時脈區域（Clock Domains）

LTDC 顯示控制器主要劃分為三個時脈區域（Clock Domains），各自負責不同的功能：

1. **AHB Clock Domain**：資料傳輸來源
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

### 5.2.2 LTDC 的輸出訊號（通往 LCD 面板）

以下為 LTDC 顯示輸出所使用的訊號腳位：

| 訊號腳位     | 說明                             |
|--------------|----------------------------------|
| `LCD_HSYNC`  | 水平同步訊號                     |
| `LCD_VSYNC`  | 垂直同步訊號                     |
| `LCD_DE`     | Data Enable：資料傳輸使能        |
| `LCD_CLK`    | Dot Clock：像素時脈              |
| `LCD_R[7:0]` | 紅色通道（8 位元）               |
| `LCD_G[7:0]` | 綠色通道（8 位元）               |
| `LCD_B[7:0]` | 藍色通道（8 位元）               |

這些訊號對應至 STM32 GPIO 所設定的 **AF14 腳位功能**，將在 `ltdc_gpio_init()` 中統一設定，最終將畫面資料輸出至 LCD-TFT 面板。

---

### 5.2.3 LTDC 的 RGB 資料線 GPIO 初始化設定

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

### 5.2.4 LTDC 的同步與資料控制 GPIO 初始化設定

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

## 5.3 SDRAM 與 FMC 控制器簡介

STM32F429 除了內建的 SRAM、Flash 等內部記憶體外，為了擴充儲存容量與提升資料存取效率，常會透過外部介面擴接多種記憶體模組，例如 SRAM、Flash 與 SDRAM。

在 LCD 顯示應用中，由於 frame buffer 體積龐大，STM32 的內部 SRAM 通常無法容納完整畫面資料。  
為了確保畫面顯示的流暢與穩定，系統常將圖像內容暫存於 **SDRAM**，再由 **LTDC（LCD-TFT Controller）模組** 自動掃描該記憶體區塊並輸出至 LCD 螢幕。因此，MCU 必須能有效地與 SDRAM 通訊與存取。

---

### 5.3.1 FMC 系統架構概述

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

### 5.3.2 SDRAM 控制器功能說明

根據 37.7.1（SDRAM controller main features），STM32 所內建的 SDRAM 控制器具備下列特性：

- 支援兩組獨立組態的 SDRAM Bank  
- 支援 8-bit、16-bit、32-bit 資料匯流排寬度  
- 提供 13 位元列位址與 11 位元欄位位址，並具備 4 個內部 Bank  
- 支援多種資料存取模式：Word（32-bit）、Half-word（16-bit）、Byte（8-bit）  
- SDRAM 時脈來源可設定為 HCLK / 2 或 HCLK / 3  
- 所有時序參數皆可程式化設定  
- 內建 Auto Refresh（自動刷新）機制，且可調整刷新速率  

---

### 5.3.3 SDRAM 對外介面腳位說明

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

## 5.4 SDRAM 初始化

### 5.4.1 SDRAM 記憶體 GPIO 腳位初始化（FMC 控制器）

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

### 5.4.2 SDRAM 初始化步驟

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

### 5.4.4 設定 FMC_SDCR 與 FMC_SDTR 暫存器（Bank1）

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

### 5.4.5 依序送出五個 JEDEC 初始化指令至 SDRAM

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
