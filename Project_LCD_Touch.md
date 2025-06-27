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

### ISR（Interrupt Service Routine）

ISR 是中斷發生時，CPU 所執行的特定函式。例如當：

- 按下按鈕（GPIO 邊緣變化）
- USART 收到資料
- 定時器時間到

這些事件皆可觸發對應的 ISR。CPU 會暫停主程式執行，自動跳轉至 ISR 完成處理後，再返回主程式。

---

STM32F4xx 系列支援使用 **`WFE`（Wait For Event）** 指令進入待命狀態，等待事件喚醒。這些事件可透過以下兩種方式產生：

---

### 模式一：中斷模式（Interrupt Mode）+ `SEVONPEND`

此為最常見的模式，會產生 IRQ 並觸發 ISR。

#### 設定步驟：

1. **EXTI**：
   - 設定 `RTSR`（Rising Trigger Selection Register）：啟用上升緣觸發
   - 設定 `FTSR`（Falling Trigger Selection Register）：啟用下降緣觸發
   - 設定 `IMR`（Interrupt Mask Register）：允許中斷產生

2. **NVIC**：
   - 啟用對應 IRQ 中斷來源（如 `EXTI0_IRQn`）
   - 設定中斷優先順序（可選）

#### 中斷範例：

| 中斷來源     | IRQ 編號 |
|--------------|----------|
| EXTI0        | 6        |
| USART1       | 37       |
| TIM2（定時器）| 28       |

**優點**：可進行邏輯處理，適用於需要即時反應的情境，例如按鈕輸入、UART 收發、定時器觸發等。

---

### 模式二：事件模式（Event Mode）

此模式不產生中斷（IRQ），也不跳入 ISR，僅送出一個「事件脈衝」。

#### 設定步驟：

1. **EXTI**：
   - 設定 `RTSR` / `FTSR`：設定觸發邊緣
   - 設定 `EMR`（Event Mask Register）：允許事件產生

#### 使用方式：

- 可與 `WFE` 配合，用於進入低功耗待命，等待事件喚醒。
- 可作為模組間事件傳遞觸發，如：透過事件驅動 DMA。

**適用情境**：不需 CPU 處理，只需喚醒或觸發其他硬體模組的低功耗應用場景。

---

## 4.2 中斷向量表

中斷向量表（Interrupt Vector Table）是一張儲存在記憶體開頭（通常位於 `0x00000000`）的表格，用來定義當中斷發生時，CPU 要跳轉執行的函式位置。

Reference Manual 中第 12.2 節 *External interrupt/event controller* 所附表格，其各欄位說明如下：

| 欄位名稱              | 說明 |
|-----------------------|------|
| **Position Priority** | 中斷編號（IRQ number），即 `NVIC_EnableIRQ(...)` 所需的參數 |
| **Type of Priority**  | 優先順序型態，`fixed` 表示不可更改，`settable` 表示可透過暫存器設定 |
| **Acronym**           | 中斷或異常的代號，例如 `EXTI0`, `HardFault`, `SysTick` 等 |
| **Description**       | 中斷的簡要用途說明 |
| **Address**           | 向量表中對應的記憶體位置（指向中斷服務函式的入口） |

---

**以 `EXTI Line 0`為例：**

- 它對應的 IRQ 編號是 **6**。
- 對應的中斷服務函式為 `EXTI0_IRQHandler()`。
- 在啟動檔（`startup.s`）中，向量表包含以下一行：

```assembly
.word EXTI0_IRQHandler   /* EXTI Line0 interrupt */
```

這表示當 IRQ6（EXTI0）發生時，CPU 將自動跳轉執行 `EXTI0_IRQHandler()` 函式。

開發者只需在 C 程式中定義此函式，例如：

```c
void EXTI0_IRQHandler(void) {
    // 中斷處理程式碼
}
```

只要函式名稱正確，編譯器與連結器便會自動將其連接至中斷向量表，**無需額外註冊或配置**。

---

**補充說明**

STM32 的啟動檔會在 `.isr_vector` 區段中，預先將每個中斷對應到正確的 IRQ 位置。開發者無需手動修改這張表，只需定義對應名稱的函式，即可完成中斷流程的銜接。

---

## 4.3 EXTI 線與觸發機制

EXTI 控制器內最多有 23 條「邊緣偵測線」（EXTI0 ~ EXTI22），每條線都能根據輸入訊號的變化，產生中斷（interrupt）或事件（event）請求。

每條 EXTI 線可**個別設定用途**（中斷或事件），並可選擇在 **上升沿、下降沿或雙邊緣** 觸發。這些設定分別對應下列暫存器：

- `EXTI_RTSR`：設定上升沿觸發（Rising Trigger Selection Register）
- `EXTI_FTSR`：設定下降沿觸發（Falling Trigger Selection Register）

是否允許該 EXTI 線產生中斷，則透過：

- `EXTI_IMR`：中斷遮罩暫存器（Interrupt Mask Register）  
  - 1 表允許中斷產生  
  - 0 表遮罩中斷（不觸發）

當中斷發生時，對應的「中斷掛起旗標」會被設為 1，紀錄此 EXTI 線已觸發中斷。該旗標儲存在：

- `EXTI_PR`：掛起暫存器（Pending Register）

> 注意：這個旗標 **不會自動清除**。你必須在對應的 ISR（例如 `EXTI0_IRQHandler()`）中，手動寫入 1 來清除它，否則中斷會持續重複觸發。

---

### EXTI 線與 GPIO 的對應規則

EXTI0 並不是固定對應到 PA0。事實上，**任一 Port（A~I）的第 0 腳位都可以對應到 EXTI0**，但不能對應到 EXTI1 或其他線。

| EXTI 線 | 可對應腳位（同一號碼） |
|---------|--------------------------|
| EXTI0   | PA0, PB0, PC0, ..., PI0 |
| EXTI1   | PA1, PB1, PC1, ..., PI1 |
| ...     | ...                      |
| EXTI15  | PA15, PB15, ..., PI15   |

### 重要限制

- 每條 EXTI 線 **一次只能對應到一個來源腳位**
- 例如：你只能選擇 **PA0 或 PB0** 對應到 EXTI0，不可同時接兩個
- 此對應關係由 `SYSCFG->EXTICR[n]` 暫存器設定（n = 0~3）

### EXTI 對應 GPIO 的設定方式（以 EXTI0 為例）

若你要讓 **PA0 對應 EXTI0**，則需設定：

```c
RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;  // 開啟 SYSCFG 時脈
SYSCFG->EXTICR[0] &= ~(0xF << 0);      // 將 EXTI0 對應到 Port A（0x0）
```

| EXTICR 範圍     | 控制哪條 EXTI 線 | 對應的 GPIO 腳位 |
|----------------|------------------|------------------|
| EXTICR[0] bits [3:0]  | EXTI0 | Px0 |
| EXTICR[0] bits [7:4]  | EXTI1 | Px1 |
| EXTICR[0] bits [11:8] | EXTI2 | Px2 |
| EXTICR[0] bits [15:12]| EXTI3 | Px3 |

---

這些設定完成後，EXTERNAL INTERRUPT 就能從對應的腳位觸發，並進入你所撰寫的中斷服務函式（ISR）。

---

## 4.4 EXTI 中斷觸發實作：按鈕觸發印出訊息

首先查看原理圖，可以發現開發板上的藍色按鈕接至 `PA0` 腳位。從電路設計來看，此按鈕為「上拉電阻 + 按下接地」的配置，也就是：

- 按鈕**未按下時為高電位**
- 按下時，腳位被拉至**低電位**

因此建議設定為「**下降緣觸發**」（falling edge trigger），也就是當電位從高轉低時產生中斷。

按鈕觸發中斷的流程如下：

1. **GPIO 腳位邊緣變化**（如按下按鈕）
2. **EXTI 偵測到變化並產生中斷事件**
3. **NVIC 接收到對應 IRQ 並判定可處理**
4. **CPU 跳轉至 ISR（中斷服務函式）執行**

因此，我們除了要設定 EXTI 模組產生中斷，也必須在 **NVIC 中啟用對應的 IRQ 中斷來源（例如 `EXTI0_IRQn`）**，並撰寫對應的 **中斷服務函式（ISR）** 來處理按鈕事件，例如印出 `"Button Pressed!"`。

---

### 步驟一：開啟 Port A 時脈並設定 PA0 為輸入模式

```c
rcc_enable_ahb1_clock(RCC_AHB1EN_GPIOA);                   // 啟用 GPIOA 時脈
gpio_set_mode(GPIOA_BASE, GPIO_PIN_0, GPIO_MODE_INPUT);    // 將 PA0 設為輸入模式
```

PA0 不需設定為 alternate function，因為 EXTI 使用的是 GPIO 輸入邊緣偵測，不走 AF。

---

### 步驟二：設定 SYSCFG，將 EXTI0 對應到 PA0

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

### 步驟三：設定邊緣觸發方式與中斷遮罩

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

`IMR` 是 **Interrupt Mask Register（中斷遮罩暫存器）**，用來控制哪一條 EXTI 線允許產生中斷（IRQ），哪一條被遮罩。

如果不設定 IMR，即使 EXTI 偵測到訊號變化，也 **無法觸發中斷服務函式（ISR）**。  
以下為中斷遮罩控制流程的示意：

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

若 IMR 對應位元未打開，例如 `IMR[0] = 0`，即使 PA0 有訊號觸發，最終也不會觸發中斷。

以下函式可設定是否啟用中斷遮罩功能：

```c
void exti_set_interrupt_mask(SYSCFG_EXTI_LINE exti_line, EXTI_InterruptMask enable) {
    uint32_t reg_addr = EXTI_BASE + EXTI_IMR_OFFSET;
    uint32_t mask = 1U << exti_line;
    uint32_t data = (uint32_t)enable << exti_line;

    io_writeMask(reg_addr, mask, data);
}
```

---

### 步驟四：啟用 NVIC 中斷

當 EXTI 偵測到邊緣事件且未被 IMR 遮罩後，會送出 IRQ 給 NVIC。此時必須在 **NVIC（中斷控制器）中啟用對應中斷來源**，否則 CPU 將不會進入中斷服務函式。

對於 `EXTI0`（對應腳位為 `PA0`），其中斷來源為 `EXTI0_IRQn`，IRQ 編號為 `6`。

#### 啟用 NVIC 中斷

以下為封裝的 NVIC 啟用函式：

```c
#define NVIC_ISER_BASE  0xE000E100U

void nvic_enable_irq(IRQn_Type irqn) {
    uint32_t reg_addr = NVIC_ISER_BASE + ((uint32_t)irqn / 32) * 4;
    uint32_t data = 1U << ((uint32_t)irqn % 32);

    io_writeMask(reg_addr, data, data);
}
```

在主程式中，啟用 EXTI0 中斷如下：

```c
nvic_enable_irq(EXTI0_IRQn);   // 啟用 NVIC 的 EXTI0 IRQ（編號 6）
```

---

### 步驟五：撰寫 ISR：EXTI0_IRQHandler

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