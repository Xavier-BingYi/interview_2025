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

# 9. 即時作業系統 (RTOS)

## 9.1 即時應用程式 (Real-Time Application, RTA)

### 9.1.1 基本觀念
「Real-Time」不是指越快越好，而是指**在限定時間內一定能完成**。  

舉例：假設氣囊系統需要在 50 毫秒內展開，不管 CPU 再快，只要無法保證 50 毫秒內一定完成，就不算 Real-Time。  

所以 Real-Time 的核心是 **確定性（Determinism）**，而不是單純的速度。  

在一般電腦或伺服器（GPOS）中，「效能高 (performance)」很重要，但在即時系統中更關心的是**能否準時完成工作**。  

舉例：  
- **音樂播放** → 即使 CPU 使用率低，但如果聲音延遲 2 秒，體驗就算失敗。  
- **飛彈控制** → 就算運算結果正確，如果計算時間過長，飛彈已經偏離目標。  

因此 Real-Time 系統設計時，通常會**犧牲部分效能，換取時間上的可預測性**。  

簡單來說：  
**Real-Time ≠ 快速 ≠ 高效能，而是能保證時間限制。**

---

### 9.1.2 保證性 vs 速度
即時系統 (Real-Time System) 強調的是**保證**，而不是**純粹速度**。  

- 一個 Web Server 可能很快回應 1 個 request，但無法保證每次都在 100ms 內完成。  
- 相比之下，RTOS 的目標是：「即使系統很忙，也能確保最重要的任務在時間內完成」。  

在即時系統中，**正確答案 + 準時產出**才算正確。  

- **NRTA (Non-Real-Time Application)**：回應時間差異大，不可預測。  
- **RTA (Real-Time Application)**：回應時間穩定，幾乎固定。  

即時系統的關鍵在於：**Response time 不大幅波動，即使不是最快，也要穩定。**

---

### 9.1.3 Time Deterministic 與應用分類
**Time Deterministic**：Real-Time 系統的核心特徵，代表其**反應時間可預測，不會隨機波動**。  

- **Hard Real-Time（無法容忍延誤）**
  - Missile Guidance and Control System（飛彈導引控制）  
  - Anti-Lock Braking System (ABS，自動防鎖死煞車系統)  
  - Airbag Deployment（氣囊展開）  

- **Soft Real-Time（允許小延遲）**
  - Stock Market Website（股市網站，允許些許延遲）  
  - VOIP（網路語音，延遲小可接受，但不宜過大）  

---

## 9.2 即時作業系統 (Real-Time Operating System, RTOS)

### 9.2.1 基本觀念
RTOS 是一種專門設計來執行**具備精準時間控制**與**高可靠度**的應用程式的作業系統。  

不同於一般作業系統（Windows、Linux），RTOS 不只要求功能正確，更要求**在限定時間內保證完成**。  

三個核心要點：  
- **Interrupt & Exception Handling** → 中斷必須在固定時間內響應，不能無限延遲。  
- **Critical Section Handling** → 進入臨界區時，RTOS 必須控制鎖定時間不可過長。  
- **Scheduling** → 必須確保高優先權任務能即時被排程。  

---

### 9.2.2 類型比較
- **GPOS (General Purpose OS，一般用途作業系統)**  
  範例：Linux、Windows 10、iOS、Android  
  設計重點：使用者體驗、功能豐富、資源管理、效能。  

- **RTOS (Real-Time OS，即時作業系統)**  
  範例：VxWorks、QNX、FreeRTOS、INTEGRITY  
  設計重點：時間確定性、快速響應、可靠性。  

- **Embedded OS（嵌入式作業系統）**  
  範例：iOS、Android  
  針對嵌入式裝置設計（手機、平板），但不一定保證即時性。  

- **Real-Time OS（即時作業系統）**  
  範例：VxWorks、QNX、FreeRTOS  
  強調任務在特定時間內必須完成。  

- **Embedded Real-Time OS（嵌入式即時作業系統）**  
  結合嵌入式與即時性，例如 FreeRTOS、QNX（能在嵌入式平台運作，並提供即時性）。  

---

### 9.2.3 Task Scheduling
- **GPOS**  
  - 排程追求**高吞吐量 (Throughput)**。  
  - 系統可能延遲高優先權任務，先處理多個低優先權任務。  
  - **吞吐量**：單位時間內完成的任務數量總和。  
  - 採用 **公平策略 (Fairness Policy)**，讓所有任務都有機會執行，避免「餓死 (Starvation)」。  
  - 例子：  
    - Linux/Windows 常用公平排程或時間片輪轉。  
    - 桌機同時跑 Chrome、Word、Spotify → 系統會平衡資源，而不是專注在某一程式。  
    - 玩遊戲時，若系統同時在背景更新，遊戲反應可能延遲。  

- **RTOS**  
  - 任務排程永遠依照**優先權 (priority)**。  
  - **高優先權任務**只有在遇到更高優先權任務時才會被中斷。  
  - 大多數 RTOS 使用 **Preemptive Scheduling（搶佔式排程）**：高優先權任務可隨時中斷低優先權任務。  

---

### 9.2.4 Latency（延遲）
- **Latency**：從事件發生到系統回應的時間差。  
- **Task Switching Latency**：事件觸發 → 任務實際開始在 CPU 上執行的時間差。  

例子：  
- **t1**：事件發生（例：車禍偵測）。  
- **t2**：對應任務開始執行（例：氣囊展開）。  
- **Task Switching Latency = t2 – t1**。  

特徵：  
- 越短、越穩定 → 系統越能滿足即時性。  
- **GPOS** → 延遲不可預測，隨負載增加而變動。  
- **RTOS** → 延遲有上限 (bounded)，即使系統忙碌，也能保證在時間內切換。  

---

### 9.2.5 Priority Inversion 與 Preemption
- **Priority Inversion（優先權反轉）**  
  - 高優先權任務被低優先權任務阻塞，導致系統延誤。  
  - **GPOS** → 不重要，系統偏重公平性與 throughput。  
  - **RTOS** → 大問題，會破壞即時性。  
  - **解法**：Priority Inheritance（低優先權任務暫時繼承高優先權，避免阻塞）。  

- **Preemption（搶佔）**  
  - 定義：強制將一個任務移出 CPU，讓另一個任務執行，即使前者未完成。  
  - **RTOS**  
    - 任務依照優先權執行。  
    - 高優先權任務就緒時，可在有限且可預測的時間內搶下 CPU。  
    - 低優先權任務會被強制讓出 CPU。  
    - RTOS 的 Kernel 幾乎所有操作都**可搶佔**，確保高優先權任務即時執行。  
  - **GPOS**  
    - 當 Kernel 處理系統呼叫（如檔案 I/O、排程）時，通常不可被中斷。  
    - 可能導致高優先權任務被延誤。  

---

## 9.3 多工 (Multi-Tasking)

### 9.3.1 基本觀念
- **Multi-Tasking**：在單一系統中同時處理多個任務的能力。  
- 實際上 CPU 在某一瞬間只能執行一個任務，多工是靠 **快速切換 (Context Switching)** 來實現的，讓使用者產生「同時執行」的錯覺。  

---

### 9.3.2 多工運作方式
- 多工透過 **時間分片 (Time Slicing)** 讓 CPU 輪流分配時間給不同任務。  
- 一個應用程式 (Application) 可以分成多個子任務 (Tasks)。  
- CPU 在 t1、t2、t3… 不斷切換不同任務。  
- 使用者角度 → Task1、Task2、Task3 像是同時在執行。  
- 實際情況 → CPU 交錯分配時間，造成「同時」的假象。  

**單核心 vs 多核心差異**  
- **單核心處理器**：任一時刻只能執行一個任務，多工靠快速切換。  
- **雙核心 / 四核心處理器**：可在同一時間真正執行 2 個或 4 個任務。  
- 核心數越多 → **並行 (Parallelism)** 程度越高。  

---

### 9.3.3 Scheduler（排程器）
- **Scheduler**：負責決定「什麼任務該執行、何時執行」。  
- 任務不會自己跑，必須由 Scheduler 按照 **排程政策 (Scheduling Policy)** 來調度。  

**常見的 Scheduling Policy**  
- **Round Robin（時間片輪轉）**  
- **Priority Based（優先權導向）**  

---

## 9.4 FreeRTOS 介紹與檔案結構

### 9.4.1 FreeRTOS 簡介
- FreeRTOS 是一個 **開源 (Open Source)**、**即時作業系統 (RTOS)**。  
- 特點：小巧、可移植、易於使用，專門設計給嵌入式系統。  

#### 支援的處理器架構 (Supported Architectures)
- **ARM**：ARM7、ARM9、Cortex-M0/M3/M4/M7、Cortex-A  
- **Atmel**：AVR、SAM 系列 (SAM3、SAM4、SAM7、SAM9)  
- **Intel**：x86、8052  
- **STMicroelectronics**：STM32、STR7  
- **Texas Instruments (TI)**：MSP430、Hercules、RM42、Stellaris  
- **Espressif**：ESP8266、ESP32  
- **NXP**：LPC1000、LPC2000、LPC4000  
- **Microchip PIC**：PIC18、PIC24、dsPIC、PIC32  
- 其他：Renesas (RX, H8S)、Fujitsu、Freescale、Xilinx (MicroBlaze, Zynq) 

---

### 9.4.2 檔案結構與層級關係
- **核心入口檔案**：`FreeRTOS.h`  
  - 包含 `projdefs.h`、`FreeRTOSConfig.h`、`portable.h`、`stddef.h`  
  - `portable.h` 會再 include `portmacro.h`、`mpu_wrappers.h`  
- **核心功能檔**：`tasks.c`、`queue.c`、`timers.c`、`heap_x.c` 都會 include `FreeRTOS.h`  
- **移植相關檔案**：  
  - 每個架構 (ARM Cortex-M0/M3、AVR、MSP430 ...) 都有自己的 **port.c**  
  - **port.c 功能**：  
    - Context Switch（任務切換，儲存/恢復暫存器）  
    - SysTick Timer 設定（產生系統 tick）  
    - 中斷優先權配置  

---

### 9.4.3 FreeRTOS 資料夾架構
- **安裝後主要資料夾**：
  1. **Demo**：各種 IDE/MCU 的範例專案，方便移植  
  2. **Source**：核心程式碼（最重要）  
  3. **License**：授權資訊  

- **Source 資料夾內容**：
  - **核心檔案**：RTOS kernel code  
  - **include**：RTOS 核心 header files  
  - **Portable**：架構相關程式碼（CPU / Compiler）  
  - **Compiler x/y**：不同編譯器對應的移植  
  - **MemMang**：不同的 heap 管理策略 (heap_1.c ~ heap_5.c)  

- **分層架構**：
  - **核心層**（portable 無關）：通用 RTOS 功能  
  - **移植層**（portable）：處理硬體/編譯器差異  

- **使用者最常修改的檔案**：`FreeRTOSConfig.h`  
  - 設定 Task 數量、Stack 大小、Tick 頻率、API 啟用/關閉等  

---

### 9.4.4 FreeRTOS Kernel 功能

FreeRTOS Kernel 提供的功能涵蓋 **任務管理、時間管理、通訊同步、除錯與安全機制**，這些都是在嵌入式開發中常用的核心特性。  

#### 任務管理
- **Pre-emptive 或 Co-operative Operation**  
  支援搶佔式（Pre-emptive）或合作式（Co-operative）多工模式。  
- **Flexible Task Priority Assignment**  
  任務優先權可靈活設定，支援不同等級的即時需求。  

#### 時間管理
- **Software Timers**  
  提供軟體計時器，讓任務能依時間事件觸發。  
- **Tick Hook Functions**  
  系統時脈中斷 (Tick interrupt) 觸發時的回呼函式，可用於週期性檢查或統計。  
- **Idle Hook Functions**  
  CPU 閒置時的回呼函式，可用於省電或背景任務。  

#### 通訊與同步
- **Queues**  
  提供佇列機制，用於任務間或任務與中斷之間的通訊。  
- **Binary Semaphores**  
  二元信號量，常用於事件通知或簡單的資源同步。  
- **Counting Semaphores**  
  計數型信號量，用於管理多個相同資源的存取。  
- **Recursive Semaphores**  
  支援同一任務多次鎖定同一資源（遞迴信號量）。  
- **Mutexes (Mutual Exclusion)**  
  提供互斥鎖，避免資源競爭問題。  

#### 安全與除錯
- **Stack Overflow Checking**  
  支援堆疊溢出檢查，提高系統穩定性。  
- **Trace Hook Macros**  
  提供除錯與追蹤的鉤子 (hook)，方便開發者分析任務行為。  

---

### 9.4.5 Task 狀態 (Task States)

在 FreeRTOS 中，每個 Task（任務）在不同時間點可能會處於以下狀態：

#### Ready
- **定義**：Task 已經準備好執行，但正在等待 CPU 排程器分配執行時間。  
- **轉換方式**：  
  - 當 Scheduler 選中它時 → 轉換到 **Running**。  
  - 當被 `vTaskSuspend()` 呼叫 → 轉換到 **Suspended**。  

#### Running
- **定義**：Task 正在 CPU 上執行。  
- **轉換方式**：  
  - 被搶佔 (Preemption) 或時間片用完 → 回到 **Ready**。  
  - 呼叫阻塞 API（例如 `vTaskDelay()`、等待 Queue/Semaphore） → 進入 **Blocked**。  
  - 呼叫 `vTaskSuspend()` → 進入 **Suspended**。  

#### Blocked
- **定義**：Task 正在等待某事件或資源，例如：  
  - 等待 Queue/Semaphore  
  - 等待延遲時間結束  
- **轉換方式**：  
  - 當事件發生或資源可用 → 回到 **Ready**。  

#### Suspended
- **定義**：Task 被明確暫停（`vTaskSuspend()`），完全不會被 Scheduler 考慮。  
- **轉換方式**：  
  - 當 `vTaskResume()` 被呼叫 → 回到 **Ready**。  

---

### 9.4.6 Idle Task 與 Hook Functions

#### Idle Task 特性
- **Idle Task 自動建立**：當 RTOS 的 Scheduler 啟動時，自動建立 Idle Task，確保隨時至少有一個任務可執行。  
- **最低優先權**：Idle Task 永遠是系統中最低優先權的任務。  
- **不搶 CPU**：若有更高優先權的任務在 Ready 狀態，Idle Task 會讓出 CPU。  

#### Idle Task 的用途
1. **釋放記憶體**：回收被刪除任務所佔用的記憶體。  
2. **CPU 空閒時執行**：當沒有其他任務可跑時，Idle Task 會執行。  
3. **低功耗模式**：可在 Idle Task 加入 Hook Function，把 MCU 切換到低功耗模式。  

#### Idle Task Hook Function
- **定義**：Idle Task Hook 是一個回呼函式，允許開發者在 Idle Task 執行時插入自訂程式。  
- **啟用方式**：在 `FreeRTOSConfig.h` 中設定：  
  ```c
  #define configUSE_IDLE_HOOK 1
  ```
- **實作方式**：  
  ```c
  void vApplicationIdleHook(void) {
      // 例如：將 MCU 切換到低功耗模式
  }
  ```
- **注意事項**：Idle Hook Function 不能呼叫可能阻塞 (block) 的 API。  

#### Tick Hook Function
- **定義**：Tick Hook 是隨系統 Tick Interrupt 執行的回呼函式。  
- **用途**：常用來實作週期性功能（例如軟體計時器）。  
- **Tick 頻率**：由 `configTICK_RATE_HZ` 設定，例如 1000Hz = 每 1ms 一次 Tick。  
- **啟用方式**：在 `FreeRTOSConfig.h` 中設定：  
  ```c
  #define configUSE_TICK_HOOK 1
  ```
- **實作方式**：  
  ```c
  void vApplicationTickHook(void) {
      // 週期性工作，必須短小、不可使用大量堆疊
  }
  ```
- **注意事項**：  
  - `vApplicationTickHook()` 在中斷服務例程 (ISR) 內執行。  
  - 需保持執行時間極短，避免影響整體即時性。  

---

## 9.5 FreeRTOS task.c 任務管理解析

### 9.5.1 TCB 結構（tskTaskControlBlock）

#### 任務建立與清單掛載流程
- **prvInitialiseNewTask()**  
  初始化 TCB 的主要欄位，包含任務名稱、優先權、堆疊起始位置，並建立初始堆疊框架（模擬任務第一次被切入執行的上下文）。  
  這個函式可以看到每個 TCB 欄位為何存在，以及如何被設定。

- **prvAddTaskToReadyList() / prvAddNewTaskToReadyList()**  
  將 TCB 的 `xStateListItem` 插入對應優先權的 Ready List。  
  Ready List 採用尾插入方式，確保同優先權任務之間能以 round-robin 方式時間分片（受 `configUSE_TIME_SLICING` 影響）。

- **prvAddCurrentTaskToDelayedList()**  
  當任務呼叫 `vTaskDelay()` 或等待資源時，會將其 `xStateListItem` 插入 Delayed List。  
  插入時會依「喚醒 tick」排序，以便系統 Tick 來時能正確搬回 Ready。  
  由於 Tick 計數可能溢出，FreeRTOS 使用「兩個 Delayed List」交替管理，以正確處理溢出情況。

看懂這三個函式後，就能將 **TCB 欄位** 與 **任務生命週期** 對應起來。  
其他如 **vTaskSwitchContext()**（切換當前任務）、**xTaskIncrementTick()**（處理 Tick 更新與延遲到期任務）則可再逐一檢視，補齊完整流程。

#### TCB 欄位解說

`typedef struct tskTaskControlBlock { ... } tskTCB` 中的主要欄位：

- **volatile StackType_t *pxTopOfStack**  
  指向任務目前的堆疊頂端，用於保存與還原任務的上下文（CPU 寄存器狀態）。  
  在內容切換（context switch）時，排程器會把 CPU 暫存器推入 `pxTopOfStack` 指向的堆疊；恢復時則從此處彈出寄存器。

- **ListItem_t xStateListItem**  
  任務狀態清單節點，用來將任務掛入不同狀態清單（Ready / Delayed / Suspended）。  
  任務在不同狀態間切換時，主要是透過這個欄位移動。

- **ListItem_t xEventListItem**  
  任務事件等待清單節點，用於將任務掛入 Queue、Semaphore、EventGroup 或 Notification 的等待清單。  
  當事件就緒時，清單會依排序決定喚醒順序。常見做法是將「反相後的優先權」存入 `xItemValue`，確保高優先權任務優先出列。

- **UBaseType_t uxPriority**  
  任務目前的有效優先權（數值越大優先權越高，0 為最低）。  
  排程器依據此欄位決定任務應插入的 Ready List，以及是否觸發搶佔。

- **StackType_t *pxStack**  
  指向任務堆疊的起始位址，用於計算與管理任務堆疊範圍。

- **char pcTaskName[configMAX_TASK_NAME_LEN]**  
  任務名稱字串，主要用於除錯與觀察（例如透過 trace 或 debug 工具）。  

- **其他依設定條件存在的欄位**  
  例如當 `configUSE_TASK_NOTIFICATIONS == 1` 時，會額外定義以下欄位：  
  - `volatile uint32_t ulNotifiedValue[configTASK_NOTIFICATION_ARRAY_ENTRIES]`  
  - `volatile uint8_t ucNotifyState[configTASK_NOTIFICATION_ARRAY_ENTRIES]`  
  這些欄位用於 **Task Notification** 機制，作為比 Queue 更輕量的 IPC／喚醒方式，用於事件同步或簡單資料傳遞，可取代部分 semaphore / queue 的使用場景。

#### 欄位初始化與使用位置對照

**欄位在哪裡被設定**
- **pcTaskName、uxPriority、pxStack、pxTopOfStack**  
  在 `prvInitialiseNewTask()` 中初始化：設定任務名稱、優先權、堆疊起始位址，並建立初始堆疊框架（模擬第一次切入時的 context）。

- **xStateListItem**  
  在 `prvAddNewTaskToReadyList()` 或 `prvAddTaskToReadyList()` 中，將 `xStateListItem` 插入對應優先權的 Ready List。

- **xEventListItem**  
  建立任務時會設定其 owner（指向對應的 TCB）。實際插入事件等待清單發生於 Queue / Semaphore / EventGroup 等等待操作（對應程式碼在 `queue.c` 等模組）。

**欄位與清單的使用**
- **Ready 清單**  
  由 `pxReadyTasksLists[]` 管理。  
  - 插入：`prvAddTaskToReadyList()`，採尾插入以實現同優先權任務的 round-robin 輪轉（受 `configUSE_TIME_SLICING` 影響）。  
  - 選任務：`taskSELECT_HIGHEST_PRIORITY_TASK()` 會選擇 Ready 清單中最高優先權的任務。

- **Delayed 清單**  
  若系統有多個任務，**通常建議適度用 delay 或阻塞等待**，否則任務會一直佔用 CPU，其他任務可能得不到執行機會。
  任務延遲或等待資源時，`prvAddCurrentTaskToDelayedList()` 會將 `xStateListItem` 依「喚醒 tick」排序插入 Delayed List。  
  在系統 Tick 更新時，`xTaskIncrementTick()` 檢查到期任務，並將其移回 Ready List。  
  由於 Tick 可能溢出，系統維護兩個 Delayed List 以正確處理溢出情況。

- **目前執行任務**  
  由 `pxCurrentTCB` 指向當前執行中的任務。  
  在 `vTaskSwitchContext()` 中透過排程演算法更新此指標，完成任務切換。

- **通知欄位**  
  `ulNotifiedValue[]` 與 `ucNotifyState[]` 由 Task Notification API 使用。  
  這些欄位會在 `tasks.c` 與 `queue.c` 中被讀寫，用於事件同步與資料傳遞。

#### TCB 與任務生命週期流程圖

「小地圖」把 TCB 放進生命週期（只看主流程，不追枝節）

**建立 → Ready**
- `xTaskCreate()`  
  → `prvInitialiseNewTask()`  
    - 設定 `pcTaskName / uxPriority / pxStack / pxTopOfStack`  
    - 建立初始堆疊框架（模擬任務第一次執行的 context）  
  → `prvAddNewTaskToReadyList()`  
    - 將 `TCB.xStateListItem` 以尾插入方式放入 `pxReadyTasksLists[uxPriority]`  
    - 同優先權清單採尾插，以利 round-robin  

**執行 → 休眠（延遲或等資源）→ 回到 Ready**
- 任務執行中呼叫 `vTaskDelay()` / `vTaskDelayUntil()` 或等待資源失敗  
  → `prvAddCurrentTaskToDelayedList()`  
    - 將 `xStateListItem` 依「喚醒 tick」排序插入 Delayed List  
    - 使用 `list item value = 喚醒 tick` 做排序  
- `xTaskIncrementTick()`  
  - 每個 Tick 檢查到期任務，將到期項目從 Delayed List 移回 Ready List  
  - 系統維護兩個 Delayed List（當前清單與溢出清單），以正確處理 Tick 溢出  

**換人（排程切換）**
- `vTaskSwitchContext()`  
  → `taskSELECT_HIGHEST_PRIORITY_TASK()`  
    - 依 `uxTopReadyPriority` 取出最高優先權的 Ready 清單  
  → `listGET_OWNER_OF_NEXT_ENTRY()`  
    - 取得下一個 TCB  
    - 同優先權透過「走訪 + 尾插」達成 round-robin（受 `configUSE_TIME_SLICING` 影響）  
  → 更新 `pxCurrentTCB` 指向被選中的任務  

**補充說明**
- **Ready**：`pxReadyTasksLists[]`（每個優先權一條 list），插入用 `vListInsertEnd()`（尾插）。  
- **Delayed**：以喚醒 tick 排序，插入用 `vListInsert()`；兩個 Delayed List 處理 tick 溢出。  
- **等待事件**：任務等待 Queue / Semaphore / EventGroup 時，使用 `xEventListItem` 掛入事件等待清單；事件就緒後由對應 API 將任務移回 Ready。  
- **讓出 CPU**：顯式 yield（`taskYIELD` / `portYIELD_WITHIN_API`）會把同優先權的目前任務移至該 Ready 清單末端，以利時間片輪轉。  

---

### 9.5.2 就緒/延遲清單 (Ready/Delayed Lists)

說明 **FreeRTOS 如何透過「清單 (list)」管理不同狀態的任務**，並讓 TCB 的 `xStateListItem` 在清單之間移動，完成 **就緒、延遲、喚醒** 等流程。

#### 找到清單的定義

在 `tasks.c` 裡有幾個全域清單變數：

- **`pxReadyTasksLists[]`**  
  儲存每個優先權層級的 Ready List。陣列大小 = `configMAX_PRIORITIES`。  

- **`xDelayedTaskList1 / xDelayedTaskList2`**  
  兩個延遲任務清單，用來處理「延遲到期」與「tick 溢出」的情況。  

- **`pxDelayedTaskList / pxOverflowDelayedTaskList`**  
  指標，分別指向「目前使用的延遲清單」與「溢出清單」。  
  Tick 遞增時，如果發生溢出，兩個清單會交換角色。  

- **`xPendingReadyList`**  
  暫存「即將轉回 Ready，但當前還不能切換」的任務清單（例如中斷中解除阻塞的任務，會先放這裡，等退出中斷後再搬回 Ready List）。

#### Ready List 怎麼用
- 任務要變成 Ready 狀態時，呼叫 **`prvAddTaskToReadyList()`**：  
  1. 依據 `uxPriority` 找到對應的 Ready List（`pxReadyTasksLists[prio]`）。  
  2. 用 **`listINSERT_END()`** 把該任務的 `xStateListItem` 尾插入清單。  
  3. 這樣同優先權任務就能形成「先進先出」的隊列。  

- 執行時，排程器用 **`listGET_OWNER_OF_NEXT_ENTRY()`** 走訪 Ready List，實現 **round-robin（時間片輪轉）**。

---

### 9.5.3 目前任務指標（pxCurrentTCB）

- **`pxCurrentTCB`**  
  在單核心 FreeRTOS 系統中，這是一個 **指標**，指向當前正在執行的任務的 TCB。  
  換句話說，排程器決定誰要執行時，會把 `pxCurrentTCB` 指向那個任務的 TCB。  

- **用途**  
  - **內容切換（context switch）**：  
    在切換任務時，系統會先把目前任務的上下文（CPU 暫存器等）存回 `pxCurrentTCB->pxTopOfStack`，  
    再把新的 `pxCurrentTCB` 所指的堆疊內容載回 CPU，完成切換。  
  - **API 呼叫時找到自己**：  
    許多 FreeRTOS API（例如 `xTaskGetCurrentTaskHandle()`）其實就是回傳 `pxCurrentTCB`，讓任務知道「我是誰」。  

- **限制**  
  在多核 SMP 版本的 FreeRTOS 裡，每個核心都會有自己的 `pxCurrentTCB`（通常是一個陣列），因為每個核心同時可能在執行不同的任務。  
  在單核版本中，只有一個 `pxCurrentTCB`。

---

### 9.5.4 加入 Ready（prvAddTaskToReadyList）

- **`prvAddTaskToReadyList()`**  
  這個函式（實際上是巨集）會把任務的 `xStateListItem` 插入到對應優先權的 **Ready List**。  

- **核心動作：`listINSERT_END()`**  
  採用「尾插入」，也就是把任務放到該優先權 Ready List 的最後。  
  這麼做的效果是：  
  - 多個同優先權任務時，能形成 FIFO 隊列。  
  - 配合 `listGET_OWNER_OF_NEXT_ENTRY()`，實現 **round-robin 輪轉**。  

- **附帶處理**  
  在插入之前，FreeRTOS 會：  
  1. 透過 `taskRECORD_READY_PRIORITY()` 更新「最高優先權 bitmap」，確保排程器能快速找到 Ready 任務。  
  2. 呼叫 trace hook（若啟用），用於除錯或追蹤分析。 

---

### 9.5.5 選最高優先權任務（taskSELECT_HIGHEST_PRIORITY_TASK）

- **`taskSELECT_HIGHEST_PRIORITY_TASK()`**  
  這個巨集/函式會做兩件事：  
  1. 利用 **`uxTopReadyPriority`** 找出目前 Ready 任務中最高優先權的清單。  
     - `uxTopReadyPriority` 是由 Ready 任務的優先權 bitmap 快速計算出來的。  
  2. 從該優先權的 Ready List 取出下一個任務（owner）。  

- **如何取任務？**  
  使用 `listGET_OWNER_OF_NEXT_ENTRY()` 從清單裡依序取出下一個 TCB，並把 `pxCurrentTCB` 指向它。  
  - 如果清單裡只有一個任務，就一直回到自己。  
  - 如果有多個任務，因為 Ready List 採「尾插 + 走訪」策略，就會形成 **round-robin**（時間片輪轉）。  

- **Round-robin 的理解**  
  - 多個同優先權任務時，系統不會只讓其中一個一直執行。  
  - 每個 Tick 中斷（若 `configUSE_TIME_SLICING = 1`），排程器會讓目前任務移到尾端，換下一個。  
  - 這樣能確保公平性，每個同優先權任務都能輪流使用 CPU。

---

### 9.5.6 建立任務（xTaskCreate / xTaskCreateStatic）

- **API 層**  
  - `xTaskCreate()`：動態配置堆疊與 TCB，然後呼叫內部初始化流程。  
  - `xTaskCreateStatic()`：使用使用者提供的靜態記憶體（堆疊與 TCB），不依賴動態配置。  

- **內部流程**
  1. **`prvInitialiseNewTask()`**  
     - 初始化 TCB 欄位（任務名稱、優先權、堆疊起始位址等）。  
     - 建立「初始堆疊框架」（模擬任務第一次執行時 CPU 暫存器的狀態）。  
  2. **`prvAddNewTaskToReadyList()`**  
     - 把這個新建立的任務插入對應優先權的 Ready List（尾插）。  
     - 同時更新 Ready bitmap，確保排程器能找到它。  

- **結果**  
  新任務被放進 Ready 狀態，等待排程器挑選。  
  如果它的優先權比目前任務還高，則可能立刻觸發搶佔，馬上開始執行。

---

### 9.5.7 延遲 / 睡眠（vTaskDelay 等阻塞 API）

- **API 層**  
  - `vTaskDelay()`：讓任務睡眠指定的 Tick 數。  
  - `vTaskDelayUntil()`：週期性延遲，確保固定週期執行。  
  - 其他阻塞 API（如 `xQueueReceive()`、`xSemaphoreTake()` 帶 block time）也可能讓任務進入延遲狀態。  

- **內部流程**
  1. 呼叫 **`prvAddCurrentTaskToDelayedList()`**  
     - 計算「喚醒 tick」= `xTickCount + 延遲時間`。  
     - 將任務的 `xStateListItem` 插入 Delayed List，並以喚醒 tick 作為排序 key。  
  2. 把當前任務從 Ready 狀態移除，排程器改去執行其他 Ready 任務。  

- **xTaskIncrementTick() 的角色**  
  - 每個 Tick 中斷時，系統檢查 Delayed List。  
  - 如果有任務的喚醒 tick 到期，就將它從 Delayed List 移回 Ready List。  
  - 因為 tick 計數可能溢出，系統維護兩個 Delayed List，避免 tick 值比較出錯。  

- **結果**  
  任務會進入 **Blocked/Delayed 狀態**，直到：  
  - 延遲時間到期 → 自動被移回 Ready List。  
  - 或者等待的資源在超時前可用 → 提前移回 Ready List。

---

### 9.5.8 刪除任務（vTaskDelete + Idle 任務回收）

- **API 層**  
  - `vTaskDelete( TaskHandle_t xTask )`：刪除指定任務。  
  - `vTaskDelete(NULL)`：刪除自己（自殺）。  

- **內部流程**
  1. **將任務移除執行路徑**  
     - 任務被刪除後，會先從所有狀態清單（Ready/Delayed/事件清單）中移除。  
     - 然後放入 **`xTasksWaitingTermination`** 清單，等待後續回收。  
  2. **為什麼不馬上 free？**  
     - 如果任務自己呼叫 `vTaskDelete(NULL)`，此時它仍在使用自己的堆疊執行。  
     - 若立即釋放 TCB 與堆疊，會導致系統崩潰。  
     - 因此 FreeRTOS 採用「延後釋放」設計。  
  3. **Idle 任務回收**  
     - Idle 任務會週期性呼叫 **`prvCheckTasksWaitingTermination()`**。  
     - 它會檢查 `xTasksWaitingTermination` 清單，釋放任務的堆疊與 TCB（若為動態配置）。  

- **結果**  
  被刪除的任務不會立刻釋放，而是掛到 `xTasksWaitingTermination`。  
  Idle 任務在安全的時機回收資源，避免任務「自殺」時直接 free 記憶體導致崩潰。  

---

### 9.5.9 任務生命週期與核心觀念

#### 任務在 FreeRTOS 的典型生命週期

1. **建立**  
   - 由 API `xTaskCreate()` 建立任務。  
   - 內部呼叫 `prvInitialiseNewTask()` 初始化 TCB 與堆疊框架。  
   - 接著 `prvAddNewTaskToReadyList()` 把任務掛入對應優先權的 Ready List（`pxReadyTasksLists[prio]`）。  
   → 任務此時進入 **Ready 狀態**。

2. **選人（排程）**  
   - 在 `vTaskSwitchContext()` 中，排程器呼叫 `taskSELECT_HIGHEST_PRIORITY_TASK()`。  
   - 依 `uxTopReadyPriority` 找出最高優先權 Ready List，選擇下一個任務。  
   - 更新 `pxCurrentTCB = next`，並完成 context switch。  
   → 任務此時進入 **Running 狀態**。

3. **睡覺（延遲/阻塞）**  
   - 任務主動呼叫 `vTaskDelay()` / `vTaskDelayUntil()`，或等待 Queue/Semaphore 等資源。  
   - 內部透過 `prvAddCurrentTaskToDelayedList()`，把任務插入 Delayed List（依喚醒 tick 排序）。  
   - 同時從 Ready List 移除，讓出 CPU。  
   → 任務此時進入 **Blocked/Delayed 狀態**。

4. **到點（喚醒）**  
   - 每次 Tick 中斷，`xTaskIncrementTick()` 會檢查 Delayed List。  
   - 如果任務的喚醒 tick 到期，將其從 Delayed List 移回 Ready List。  
   - 或者當事件發生（Queue/Semaphore 可用），任務也會被移回 Ready List。  
   - 如果喚醒的任務優先權比當前任務高，會觸發搶佔切換。  
   → 任務此時回到 **Ready 狀態**，等待再次執行。

---

#### Q1. 什麼是 TCB？請解釋 TCB。

- **定義**  
  TCB（Task Control Block，任務控制塊）是 FreeRTOS 用來管理任務的核心資料結構。  
  每個任務都對應一個 TCB，排程器透過它來追蹤任務狀態。

- **主要內容**  
  - `pxTopOfStack`：任務堆疊頂指標，用於保存/恢復上下文。  
  - `xStateListItem`：任務狀態節點，決定任務目前掛在哪個清單（Ready / Delayed / Suspended）。  
  - `xEventListItem`：事件等待節點，當任務在 Queue/Semaphore/EventGroup 等等待時使用。  
  - `uxPriority`：任務優先權。  
  - `pxStack`：任務堆疊的起始位址。  
  - `pcTaskName`：任務名稱，用於除錯。  
  - （可選）Task Notification 欄位：`ulNotifiedValue[]`、`ucNotifyState[]`。  

- **角色**  
  - 相當於「任務的身份證 + 狀態紀錄」。  
  - 排程器依靠 TCB 決定：這個任務目前在做什麼、該不該換成它執行。  

- **一句話總結**  
  **TCB 是任務在 FreeRTOS 裡的核心資料結構，排程器透過它管理任務的狀態、堆疊與優先權。**

---

#### Q2. 為什麼任務裡的 `while(1)` 輪詢要改成 `vTaskDelay()`？
- 如果任務單純 `while(1)` 不延遲，它會 **永遠處於 Ready 狀態**，排程器每次都會選到它。  
- 結果就是這個任務獨佔 CPU，其他任務可能完全沒機會執行。  
- 用 `vTaskDelay()`（或阻塞等待 Queue / Semaphore）可以讓任務進入 **Blocked/Delayed 狀態**，把 CPU 還給其他任務。  
- 核心原因：**FreeRTOS 採用 Ready/Delayed 清單來管理任務，必須靠阻塞或延遲來釋放 CPU 時間片**。  

---

#### Q3. 為什麼 ISR（中斷服務程序）要用「通知 / Queue」來喚醒任務？
- 中斷是系統裡最高即時性的事件，但 **ISR 不能直接切換任務**，因為它不會完整做排程器需要的上下文切換流程。  
- 正確做法：  
  - ISR 用 **Task Notification / Queue / Semaphore** 將事件傳遞給對應的任務。  
  - 這些 API 會把等待的任務從 **Delayed/Event 清單** 移到 **Ready 清單**。  
  - 等 ISR 結束後，系統會呼叫 `portYIELD_FROM_ISR()`（或類似機制），由排程器決定是否立即切換到那個任務。  
- 核心原因：**ISR 本身不能直接進行任務切換，必須透過清單操作（Ready/Delayed）讓排程器接手**。

---

#### Q4. 解釋任務如何建立、掛入 Ready/Delay 清單、被選出執行、再進入睡眠/喚醒。

- **建立**：呼叫 `xTaskCreate()`，內部透過 `prvInitialiseNewTask()` 建立 TCB 與初始堆疊，接著 `prvAddNewTaskToReadyList()` 把任務插入對應優先權的 Ready List。  
- **掛入 Ready**：任務的 `xStateListItem` 被尾插到 Ready List，與同優先權任務 round-robin 輪流。  
- **被選出執行**：排程器用 `taskSELECT_HIGHEST_PRIORITY_TASK()` 找到最高優先權的 Ready 任務，更新 `pxCurrentTCB`，並做 context switch。  
- **進入睡眠/阻塞**：呼叫 `vTaskDelay()` 或等待資源時，任務會被 `prvAddCurrentTaskToDelayedList()` 移到 Delayed List，依「喚醒 tick」排序。  
- **喚醒**：系統 tick 透過 `xTaskIncrementTick()` 檢查到期任務，或事件觸發，把任務移回 Ready List，等待下次被排程。  

---

#### Q5. 為什麼同優先權任務能輪流執行（round-robin）？
- Ready List 採用 **尾插入（listINSERT_END）**，同優先權的任務會被 FIFO 排列。  
- 排程器使用 **`listGET_OWNER_OF_NEXT_ENTRY()`** 走訪 Ready List。  
- 每個 Tick 到來（若 `configUSE_TIME_SLICING = 1`），當前任務會被移到尾端，下一個同優先權任務被選中。  
- → 確保同優先權任務能公平輪流使用 CPU。

---

#### Q6. 什麼情況會觸發任務切換（yield/context switch）？
- **新任務或高優先權任務 Ready**：  
  建立新任務或事件喚醒任務時，若它的優先權比當前任務高，排程器會馬上切換。  
- **時間片到期**：  
  當前任務與其他 Ready 任務同優先權，Tick 到來時會進行 round-robin 輪轉。  
- **顯式呼叫 yield**：  
  任務呼叫 `taskYIELD()`，或某些 API（如 `xQueueSend()` 失敗）內部觸發 yield，讓排程器重新挑選任務。  
- **ISR 觸發切換**：  
  ISR 透過 `portYIELD_FROM_ISR()` 告知排程器在中斷結束後立即切換任務。 

---

## 9.6 FreeRTOS list.c 鏈結清單解析

### 9.6.1 資料結構

#### 1. **List_t**（清單控制結構）
```c
typedef struct xLIST {
    UBaseType_t uxNumberOfItems;     // 清單中的節點數量
    ListItem_t *pxIndex;             // 走訪清單時的游標，記錄上一次取出的節點
    MiniListItem_t xListEnd;         // End Marker，固定存在的「假節點」
} List_t;
```
- **uxNumberOfItems**：清單長度。  
- **pxIndex**：走訪時記錄「上一次取到的節點」，支援 round-robin。  
- **xListEnd**：永遠存在的 **MiniListItem_t**，作為「清單頭/尾的標記」。  
  - 形成 **環狀結構**：`head → ... → tail → xListEnd → head`。  
  - 保證清單永遠有邊界，不需額外處理 NULL。  

---

#### 2. **ListItem_t**（清單節點）
```c
struct xLIST_ITEM {
    TickType_t xItemValue;         // 排序用的值（例：任務喚醒 tick，或優先權）
    struct xLIST_ITEM *pxNext;     // 下一個節點
    struct xLIST_ITEM *pxPrevious; // 前一個節點
    void *pvOwner;                 // 擁有此節點的物件 (通常是 TCB)
    struct xLIST *pxContainer;     // 此節點目前所在的 List_t
};

typedef struct xLIST_ITEM ListItem_t;
```
- **xItemValue**：排序依據，數值越小越靠前（例：任務延遲的到期時間）。  
- **pxNext / pxPrevious**：雙向鏈結，支援前後走訪。  
- **pvOwner**：指向「此節點屬於誰」（通常是任務的 TCB）。  
- **pxContainer**：指向「此節點目前在哪個清單」。 

---

#### 3. **MiniListItem_t**（清單頭 End Marker）
```c
struct xMINI_LIST_ITEM {
    TickType_t xItemValue;         // 固定設為最大值，確保此節點永遠在清單最後
    struct xLIST_ITEM *pxNext;
    struct xLIST_ITEM *pxPrevious;
};
```
- **特殊的 ListItem_t**：沒有 `pvOwner` 與 `pxContainer`。  
- **用途**：  
  - 作為 **清單頭尾的假節點**，確保清單永遠是環狀。  
  - 其 `xItemValue` 固定為最大值，因此任何正常節點都會插在它前面。  
  - 透過判斷是否回到此節點，就能知道清單是否走訪完畢。

---

### 9.6.2 清單初始化

#### 1. **vListInitialise()**：初始化一個清單
- 功能：建立一個「乾淨的清單」。  
- 主要動作：  
  1. 把 `uxNumberOfItems` 設為 0（清單一開始沒有節點）。  
  2. 設定 `pxIndex` 指向 `xListEnd`（游標預設在 end marker 上）。  
  3. 初始化 `xListEnd`（這個 MiniListItem_t）：  
     - `xItemValue` 設為最大值（`portMAX_DELAY`）。  
     - `pxNext` 和 `pxPrevious` 都指回自己 → 形成「自環」。  

初始化後，清單的結構是：
```
xListEnd <-> xListEnd   （空清單）
```
- **vListInitialise()** = 建立一條空清單，裡面只有一個 end marker (`xListEnd`)，保證清單永遠環狀且有邊界。  

---

#### 2. **vListInitialiseItem()**：初始化單一節點
- 功能：準備一個「獨立節點」（通常對應某個任務的 `xStateListItem`）。  
- 主要動作：  
  - 把節點的 `pxContainer` 設為 `NULL`，表示目前不屬於任何清單。  
  - 其他欄位（如 `xItemValue`）會在插入清單時再設定。  

這樣能避免一個節點「同時掛在兩個清單」，確保清單一致性。  

- **vListInitialiseItem()** = 準備一個清單節點，尚未加入任何清單，等呼叫 `vListInsert()` / `vListInsertEnd()` 時才會掛上去；插入時會同時設定 `pvOwner` 指回它所屬的 TCB。  

---

#### FreeRTOS 中的對應關係
- 一個 **任務 (Task)** = 一個 **TCB** (`tskTaskControlBlock`)。  
- 每個 TCB 裡都有一個 **`xStateListItem`**（型別為 `ListItem_t`）。  
- 因此，**節點代表的就是一個任務**。  
- `pvOwner` 會指回 TCB 本身，讓清單節點和任務產生雙向關聯。  

---

### 9.6.3 插入與移除

#### 0. 火車系統的隱喻

##### 建造火車（`vListInitialise()`）
`vListInitialise()` 就像是建造一列新的火車（`List_t`，例如 **Ready List** 或 **Delayed List**）。  

- 一開始火車沒有任何車廂（`uxNumberOfItems = 0`）。  
- 系統會先放上一節特殊的 **哨兵車廂**（`xListEnd`），它同時扮演火車的頭與尾，確保清單是環狀且不會出現 `NULL`。  
- 此時巡查車廂的指標（`pxIndex`）只能指向這節哨兵車廂 `xListEnd`。 

##### 車廂與工單（`ListItem_t`）
每一節車廂（`ListItem_t`）都附有一份工單，內容包含：  

- **重要性值**（`xItemValue`）：數字越小代表越重要，插入時會排在越前面。  
  - 哨兵車廂的 `xItemValue` 永遠是最大值，確保它始終位於最後。  
- **相鄰車廂**（`pxNext` / `pxPrevious`）：記錄車廂的前後鏈結。  
- **車廂的擁有者**（`pvOwner`）：通常指向任務的 **TCB**，代表哪位「乘客」。  
- **所屬火車**（`pxContainer`）：指向該車廂目前所在的火車（某個 `List_t`，例如 Ready List 或 Delayed List）。  

##### 任務與車廂的關聯
每個任務（乘客）的 **TCB** 會準備常見的兩張工單：  

- `xStateListItem` → 用來放進 **Ready / Delayed / Suspended List**。  
- `xEventListItem` → 用來放進 **Event List**（等待事件時）。  

舉例：  
- 當 `xStateListItem` 在 Ready 狀態，就會被插入 **Ready List**。  
- 當任務呼叫 `vTaskDelay()`，這個 `xStateListItem` 會從 Ready List 移出，插入 **Delayed List**。  

若要新增車廂，則需填寫一份申請單（`vListInsert()`），指定要加入的火車（`pxList`）以及對應的工單（`pxNewListItem`）。  

---

#### 1. `vListInsert()`（排序插入）
- **用途**：根據 `xItemValue` 排序，把節點插到清單裡正確的位置。  
- **常見場景**：  
  - **Delayed List**：任務呼叫 `vTaskDelay()` 時，會把 `xStateListItem` 插入 Delayed List，排序依據就是「喚醒 tick」。  
- **重點**：  
  - `xItemValue` 小的節點會排在前面。  
  - `xListEnd.xItemValue` 永遠是最大值 → 確保它在最後。  
- **比喻**：  
  就像把新車廂（節點）插入一列火車（清單，例如 Delayed List）。  
  插入前需要填寫工單，內容包含：  
  - 要插入的火車 (`pxList`)  
  - 車廂的工單 (`pxNewListItem`)  

  系統會依 **重要性值（`xItemValue`）** 自動安排車廂的位置，並更新它與相鄰車廂的關聯，以及所屬火車資訊。  

###### 函式原型
```c
void vListInsert( List_t * const pxList,
                  ListItem_t * const pxNewListItem )
```

- `pxList`：要插入的清單（例如某個 Delayed List）。  
- `pxNewListItem`：要被插入的節點（`ListItem_t`，通常隸屬某個 TCB）。  

---

#### 2. `vListInsertEnd()`（尾插入）
- **用途**：將節點直接插入清單的最後一個有效項目前，也就是 **永遠放在 `xListEnd` 的前面**。  
  這種方式不考慮 `xItemValue`，純粹按照「先來後到」的順序排列。  
- **常見場景**：  
  - **Ready List**：在同一個優先權層級下，所有任務都會放在相同的 Ready List。  
    當一個任務用完時間片（time slice）或主動讓出 CPU 時，它會透過 `vListInsertEnd()` 被放到該 Ready List 的尾端。  
    這樣其他同優先權的任務就能輪到 CPU，實現 **Round-robin（輪詢排程）**。  
- **重點**：  
  - 通常由呼叫端在插入前就把 `pvOwner` 設為對應的 **TCB**，`vListInsertEnd()` 本身僅負責鏈結節點，不會主動修改 `pvOwner`。  
  - 由於是尾插法，不會因 `xItemValue` 改變順序，因此適合用在 **公平輪流執行** 的場景。    

##### 函式原型
```c
void vListInsertEnd( List_t * const pxList,
                     ListItem_t * const pxNewListItem )
```

- `pxList`：要插入的清單（通常是某個 Ready List）。  
- `pxNewListItem`：要插入的節點（`ListItem_t`，對應某個任務的 TCB）。   

---

#### 3. `uxListRemove()`（移除節點）
- **用途**：  
  將某個節點（`ListItem_t`）從它所在的清單（`List_t`）移除，並更新清單狀態。  
- **常見場景**：  
  - 任務從 **Ready → Blocked**（例如呼叫 `vTaskDelay()` 進入延遲）。  
  - 任務從 **Blocked → Ready**（例如事件觸發、延遲時間到期）。  
  這些情況都需要先從原清單移除，再掛到新清單。  
- **運作方式**：  
  1. 斷開當前節點與前後鏈結（`pxNext` / `pxPrevious`），保持清單完整。  
  2. 將該節點的 `pxContainer` 清為 `NULL`，避免同一節點同時存在於多個清單。  
  3. 清單的節點計數 `uxNumberOfItems` 減 1，維護清單長度正確。  
- **重點**：  
  - 移除過程會自動處理指標調整，確保清單維持雙向鏈結的正確性。  
  - 若 `pxIndex` 正指向被移除的節點，實作會把 `pxIndex` 退回到前一個節點（`pxPrevious`），以維持走訪一致性。 

##### 函式原型
```c
UBaseType_t uxListRemove( ListItem_t * const pxItemToRemove )
```

- `pxItemToRemove`：要被移除的節點。  
- **回傳值**：移除後，清單剩餘的節點數量（`uxNumberOfItems`）。   

---

#### 4. 狀態轉換流程總結
- **Ready → Blocked**：`uxListRemove()` + `vListInsert()`  
- **Blocked → Ready**：`uxListRemove()` + `vListInsertEnd()`   

---

### 9.6.4 清單走訪

在 FreeRTOS 中，清單不只用來存放節點，還需要 **公平地巡訪**（走訪），特別是在 **同優先權任務的時間片輪轉（Round-robin）** 中。

#### `listGET_OWNER_OF_NEXT_ENTRY()`
- **用途**：  
  從目前清單索引（`pxIndex`）往下取下一個節點的 `pvOwner`（通常是任務的 TCB）。  
  - 若走到哨兵節點（`xListEnd`），會自動回到清單的第一個有效節點，確保清單是「環狀」的。  
  - `listGET_OWNER_OF_NEXT_ENTRY()` 的存在，就是為了讓 FreeRTOS 的排程器能 **公平地在同優先權任務之間做 Round-robin**，避免某一個任務被永遠選中。  

- **常見場景**：  
  - **Ready List**：在同優先權清單內，透過 `listGET_OWNER_OF_NEXT_ENTRY()`，讓任務能依序輪流取得 CPU，實現 **Round-robin 排程**。  

- **運作方式（火車隱喻）**：  
  - 假設 Ready List（優先權 2）裡有三個任務：`T1, T2, T3`。  
  - 想像巡查員（`pxIndex`）在火車上巡視車廂，初始情況：`pxIndex` 指向哨兵（車頭/車尾）。  
  - 每呼叫一次 `listGET_OWNER_OF_NEXT_ENTRY()`，巡查員就往下一節車廂移動，並報告車廂的乘客（`pvOwner`）。  
  - 若巡查到哨兵車廂（代表車尾），巡查員會自動繞回到車頭，開始新一輪的巡視，確保 `T1 → T2 → T3 → T1 …` 的公平輪流。

#### 巨集定義
```c
#define listGET_OWNER_OF_NEXT_ENTRY( pxTCB, pxList )        \
    do {                                                    \
        List_t * const pxConstList = ( pxList );            \
        /* 將索引移動到下一個節點 */                         \
        ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext; \
                                                             \
        /* 若遇到哨兵節點，則跳到第一個有效節點 */            \
        if( ( pxConstList )->pxIndex == ( ListItem_t * ) &( ( pxConstList )->xListEnd ) ) \
        {                                                    \
            ( pxConstList )->pxIndex = ( pxConstList )->pxIndex->pxNext; \
        }                                                    \
                                                             \
        /* 取出當前節點的擁有者（通常是任務的 TCB） */          \
        ( pxTCB ) = ( pxConstList )->pxIndex->pvOwner;       \
    } while(0)
```

**參數說明**  
- `pxList`：目標清單（例如某個 Ready List）。  
- `pxTCB`：輸出變數，用來接收當前節點的擁有者（通常是任務的 TCB）。  

---

### 9.6.5 任務管理中的應用對照

在 FreeRTOS 中，不同種類的清單（List）扮演著 **任務狀態管理** 的角色。透過前面介紹的清單操作（插入、移除、走訪），系統能維持 **Ready / Blocked / Suspended** 等狀態轉換。本節對照各清單在任務管理中的實際應用。

---

#### 1. Ready List（就緒清單）
- **用途**：存放所有「準備執行」的任務，依優先權分層。  
- **操作方式**：  
  - 使用 **尾插入**（`vListInsertEnd()`），新任務會插在尾端。  
  - 保證同優先權下是 **FIFO**，並且搭配 `listGET_OWNER_OF_NEXT_ENTRY()` 完成 **Round-robin 輪流排程**。  
- **火車比喻**：  
  - 一條 Ready 火車就是一個優先權層級。  
  - 每個任務（乘客）上車後，會排在隊伍最後。當時間片用完時，任務下車再重新排到最後。  
  - 確保大家輪流坐到駕駛位（CPU）。

---

#### 2. Delayed List（延遲清單）
- **用途**：存放呼叫 `vTaskDelay()` 或 `vTaskDelayUntil()` 的任務，直到指定 tick 到期才會被喚醒。  
- **操作方式**：  
  - 使用 **排序插入**（`vListInsert()`），依照「到期 tick」由小到大排列。  
  - 系統每個 tick 都會檢查表頭的任務是否到期，若到期則移回 Ready List。  
- **火車比喻**：  
  - 火車上依「鬧鐘時間」排序，誰先到時間誰先下車。  
  - 哨兵車廂的 `xItemValue` 永遠最大，保證排序不會亂。

---

#### 3. Event List（事件清單）
- **用途**：存放等待事件（例如 Queue、Semaphore、Mutex）的任務。  
- **操作方式**：  
  - 插入時會依 **任務優先權排序**（高優先權在前）。  
  - 當事件發生（例如 Queue 有資料），清單頭的最高優先權任務會先被喚醒。  
- **火車比喻**：  
  - 任務在車站排隊領獎品（事件）。  
  - 排隊規則不是先來先到，而是 **位階高（優先權大）的乘客站在最前面**，確保他們先上車。

---

#### 4. xPendingReadyList（待搬移清單）
- **用途**：存放在 **中斷服務常式（ISR）** 中被喚醒的任務。  
- **操作方式**：  
  - ISR 不能直接修改 Ready List（因為與排程器共用，容易出現競態）。  
  - 因此先把任務放進 `xPendingReadyList`，等退出中斷後，再一次性移到對應的 Ready List。  
- **火車比喻**：  
  - ISR 就像臨時站務員，先把任務旅客集中到「臨時月台」。  
  - 當中斷結束，系統再把這些旅客送回對應的 Ready 火車，避免在繁忙時段插隊出錯。

---

### 9.6.6 重點整理與常見考題

#### FreeRTOS 清單模組核心觀念
1. **清單的角色**  
   - FreeRTOS 使用雙向鏈結串列（List_t）來管理任務狀態。  
   - 每個任務的 TCB 內含 `xStateListItem`、`xEventListItem`，分別掛到不同清單。  

2. **主要 API 功能**  
   - `vListInitialise()`：建立清單，放上哨兵節點。  
   - `vListInsert()`：排序插入（依 `xItemValue`，如延遲時間）。  
   - `vListInsertEnd()`：尾插入（FIFO，用於 Ready List）。  
   - `uxListRemove()`：從清單移除節點，並維護鏈結正確性。  
   - `listGET_OWNER_OF_NEXT_ENTRY()`：巡訪下一節點，用於 Round-robin。  

3. **清單與任務狀態對照**  
   - **Ready List**：尾插入 + 巡訪 → FIFO + Round-robin。  
   - **Delayed List**：排序插入 → Tick 到期順序喚醒。  
   - **Event List**：依優先權反序排序，高優先權先被喚醒。  
   - **xPendingReadyList**：暫存 ISR 喚醒的任務，中斷結束後再搬回 Ready List。  

---

#### Q1. 為什麼 Ready List 要用尾插入（`vListInsertEnd()`）？
- 保證同優先權任務 **FIFO** 排列。  
- 搭配 `listGET_OWNER_OF_NEXT_ENTRY()` 走訪，確保 **Round-robin 輪流**。  
- 避免同一個任務一直霸佔 CPU。  

---

#### Q2. 為什麼 Delayed List 要用排序插入（`vListInsert()`）？
- 任務呼叫 `vTaskDelay()` 後，要依「喚醒 Tick 值」排序。  
- Tick 中斷時只需檢查清單頭，就能快速判斷是否有任務到期。  
- **效率高，不需要全清單遍歷**。  

---

#### Q3. Event List 的排序依據是什麼？為什麼？
- 排序依 **任務優先權（Priority）**，而非 Tick 值或 FIFO。  
- 事件發生時，必須讓 **高優先權任務** 優先被喚醒，才能達到即時性。  
- 範例：多個任務等待 Queue → Queue 有資料時，高優先權任務先獲得。  

---

#### Q4. 為什麼需要 xPendingReadyList？
- ISR 中不能直接修改 Ready List（避免與排程器競態）。  
- 所以先把任務插入 xPendingReadyList。  
- 中斷結束後，由核心統一搬回 Ready List。  
- 確保 **中斷與排程器互動安全**。  

---

#### Q5. 為什麼同優先權任務能公平輪流執行？
- Ready List 採用 **尾插入（FIFO）**。  
- `listGET_OWNER_OF_NEXT_ENTRY()` 每次巡訪取下一個任務。  
- Tick 到期後，當前任務會被移到尾端，下一個任務上 CPU。  
- → 實現 **Round-robin（時間片輪轉）**。  

---

#### Q6. 清單在 FreeRTOS 任務管理中的地位
- **Ready List** → 管理「誰可以立刻跑」。  
- **Delayed List** → 管理「誰要等多久」。  
- **Event List** → 管理「誰在等事件」。  
- **xPendingReadyList** → 管理「ISR 喚醒的任務」。  
- **一句話總結**：清單是 FreeRTOS 的「鐵路系統」，所有任務都在不同的火車上輪流進出，排程器只需檢查清單，就能知道下一步該換誰執行。 

---

## 9.7 FreeRTOS queue.c 任務間通訊解析

理解 queue.c 的重點在於：  
- 如何建立與初始化 Queue。  
- 如何在任務間傳遞資料。  
- 如何處理阻塞與喚醒。  
- 為什麼同一套機制可以涵蓋 Queue / Semaphore / Mutex。

---

### 9.7.0 任務間通訊與同步基本概念

**Queue（佇列）、Semaphore（信號量）與 Mutex（互斥鎖）** 都是基於 Queue 機制實作出來的，不同之處在於應用場景。

---

#### 1. Queue（佇列）

- **定義**：一種 **FIFO（先進先出）** 的資料結構，可以把 Queue 想像成一個帶規則的 **buffer（緩衝區）**。  

- **用途**：  
  - 在 FreeRTOS 中，Queue 是 **任務間通訊（message passing）** 的主要方式，可傳遞資料或指令。  
  - 也是 Semaphore 與 Mutex 的基礎。  

- **Queue 的本質**  
  在 FreeRTOS 裡，Queue 其實就是一個 **環形緩衝區（circular buffer）**，外加兩個等待佇列：  
  - `xTasksWaitingToSend`：當 Queue **滿了** → 想送資料的任務被掛進來。  
  - `xTasksWaitingToReceive`：當 Queue **空了** → 想收資料的任務被掛進來。 

所以 Queue 不只是單純的 FIFO buffer，還負責管理「任務的同步與阻塞」。

- **基本運作**：  
  - 任務 A（Producer）將資料放進 Queue。  
  - 任務 B（Consumer）從 Queue 取出資料。  
  - 如果 Queue **滿了** → Producer 會被阻塞，直到有空位。  
  - 如果 Queue **空了** → Consumer 會被阻塞，直到有資料。

##### Queue 的月台信箱隱喻

Queue 就像月台上的一個 **信箱**，用來存放「信件」（資料或指令）。  

- **存放規則（FIFO）**  
  - 信件放入的順序就是被取出的順序（先放先取），確保傳遞的公平與一致。  
  - 信箱有固定的容量（Queue length），一旦裝滿就必須等到有空位才能再放新信件。 

- **乘客（任務）與信件**  
  - 乘客（任務）在出發前會查看信箱是否有**可取用的**信件（Queue 不區分訊息的「屬主」）。  
  - 若成功取信，就上名為 **Ready List** 的火車，等待 CPU 載走開始執行。  
  - 若沒有且允許等待，任務就改搭**事件火車（Event List）**，先在月台上等候條件達成。  
  - 多位乘客可能同時競爭同一封信，最終誰能優先取得，取決於**任務優先權**與**排隊順序**。 

- **月台等待區（List_t）**  
  - **信箱滿了**：想投遞信件的乘客改搭 `xTasksWaitingToSend` 這班**事件火車**等待空位。  
  - **信箱空了**：想收信的乘客改搭 `xTasksWaitingToReceive` 這班**事件火車**等待新信件。  
  - 這些事件火車專門載著「等待中」的乘客，直到條件被滿足才會被轉送到 Ready List。    
  - 如果乘客等待過久（timeout），就會自動放棄，直接回到 Ready List，繼續進行其他工作。

- **列車長（Scheduler）的調度**  
  - 當信件狀態改變（投遞或收取成功）或等待逾時，列車長會把對應乘客從事件火車**換乘**到 **Ready List**。  
  - 若被喚醒乘客的**優先權更高**，列車長會立刻讓他下車直奔 CPU，搶占目前正在執行的任務。  

#### 2. Semaphore（信號量）

- **定義**：Semaphore 是一種 **同步機制** 與 **資源管理機制**。  
  - **同步（Synchronization）**：用來在不同任務或中斷之間「發訊號」，確保事件的先後順序。  
    - 例子：任務要等 ISR 發出「完成」訊號後才能繼續。  
  - **資源計數（Resource Counting）**：用來表示「目前還有多少資源可用」，避免多個任務同時搶用有限資源。  
    - 例子：有 5 個連線插槽，就發 5 張票；任務用完要歸還，別人才拿得到。  

- **種類**：  
  - **Binary Semaphore（二元信號量）**：本質就是一個長度 = 1 的 Queue，裡面存放「0 或 1」，常用於事件通知。  
    - 例如：ISR 結束後，透過 Binary Semaphore 喚醒等待的任務。  
  - **Counting Semaphore（計數型信號量）**：就像長度 > 1 的 Queue，用來記錄可用資源數。
    - 例如：有 5 個相同資源，最多允許 5 個任務同時取得。  

- **特性**：  
  - 本質上是「容量有限制的 Queue」，`itemSize = 0`（不存放資料，只存放「資源計數」）。  

##### Semaphore 的月台信箱隱喻

Semaphore 就像月台上的一個 **票務亭**，不是用來傳遞信件，而是發放「月台票」（資源使用權）。  

- **Binary Semaphore（二元信號量）**  
  - 票務亭只有 **1 張月台票**。  
  - 當票被拿走時，後來的乘客只能在等待火車（Event List）上等，直到票被歸還。  
  - 這就像 **一次只允許一個任務通行**，常用於事件通知。  
  - 例子：ISR 把票投入票亭 → 任務就能領到票並被喚醒。  

- **Counting Semaphore（計數型信號量）**  
  - 票務亭有 **多張相同的月台票**。  
  - 最多有 N 個乘客同時持票上車；當票用完，其他人只能排隊等待。  
  - 適合表示 **有限資源池**（例如 5 台相同的機器）。  

- **特性**  
  - 跟 Queue 不同，票務亭 **不存放「內容物」**，只有票的數量。  
  - 所以可以把它看作「itemSize = 0 的 Queue」，專門用來控管資源使用。  
  - 若乘客等候過久（timeout），就會放棄排隊，回到 Ready List 做其他工作。  

- **列車長（Scheduler）的角色**  
  - 當票務亭補回票（例如任務釋放或 ISR 投票），列車長會通知等待中的乘客，讓他們從事件火車換乘到 Ready List。
  - 若被喚醒乘客的優先權較高，列車長會立刻安排他上車，搶占目前執行中的任務。  

#### 3. Mutex（互斥鎖）

- **定義**：Mutex（Mutual Exclusion，互斥鎖）是一種 **任務之間的資源保護機制**。  
  - 用來確保 **同一時間只有一個任務能存取共享資源**（例如：同一塊記憶體、同一個硬體介面）。  
  - 本質上和 Binary Semaphore 類似，但多了 **優先權繼承（Priority Inheritance）** 的機制，避免高優先權任務被低優先權任務阻塞太久。  

- **用途**：  
  - 保護臨界區（Critical Section）。  
  - 確保資源同一時間只會被一個任務佔用。  
  - 常用於 **共享裝置存取**（UART、SPI、I2C 等）。  

- **特性**：  
  - 本質是 Binary Semaphore 的特化版，但有 **任務擁有者（Owner）** 的概念。  
  - 任務取得 Mutex 後必須由**同一個任務釋放**，不可被其他任務或 ISR 直接歸還。  
  - 具有 **優先權繼承**：當低優先權任務持有 Mutex 時，若有高優先權任務等待該 Mutex，低優先權任務會暫時「提升優先權」，直到釋放 Mutex，避免 **優先權反轉（Priority Inversion）**。  

- **舉例**：  
  - 多個任務要同時寫一個 UART，為了避免輸出混亂，可以用 Mutex 來確保一次只允許一個任務操作 UART。  

##### Mutex 的月台票亭隱喻

Mutex 就像月台上的一個 **專屬票亭**，發放一張「專屬票」，用來確保資源一次只被一位乘客（任務）獨佔。  

- **專屬票的特性**  
  - 只有 **原本拿票的乘客** 才能歸還，不能代還。  
  - 一旦乘客持有票，就能獨佔某個資源（例如：售票窗口、候車室、共用變數）。  
  - 不像 Semaphore 那樣只管「票數」，Mutex 更強調「擁有者」的概念。  

- **優先權繼承（Priority Inheritance）**  
  - 如果低優先權的乘客持有票，而有高優先權的乘客正在等票：  
    - 列車長會暫時幫低優先權乘客「升級月台票」→ 讓他被優先處理。  
    - 當低優先權乘客完成任務並歸還票後，再恢復原本的等級。  
  - 這樣可以避免 **優先權反轉（Priority Inversion）**，確保高優先權乘客不會被卡太久。  

- **列車長（Scheduler）的角色**  
  - 當票被歸還時，列車長會通知等待中的乘客，從事件火車換乘到 Ready List。  
  - 若新喚醒的乘客優先權較高，就會立即上車搶占 CPU。  

- **應用場合**  
  - 常用於需要「互斥」的情境，例如共享硬體外設、共用全域變數、或必須序列化的臨界區操作。

---

### 9.7.1 核心資料結構

**Queue_t（struct QueueDefinition）**  

這是 *queue.c* 的核心結構，所有的 **Queue / Semaphore / Mutex** 都基於它實作。  
不同 `FreeRTOSConfig.h` 設定會影響欄位數量，但核心不變。

#### 結構原型（精簡版）
```c
typedef struct QueueDefinition
{
    int8_t * pcHead;        /* Queue 儲存區起始位址 */
    int8_t * pcWriteTo;     /* 下一個寫入位置 */

    union
    {
        QueuePointers_t xQueue;     /* 當作 Queue 使用 */
        SemaphoreData_t xSemaphore; /* 當作 Semaphore 或 Mutex 使用 */
    } u;

    List_t xTasksWaitingToSend;    /* 等待投遞的任務清單 */
    List_t xTasksWaitingToReceive; /* 等待接收的任務清單 */

    volatile UBaseType_t uxMessagesWaiting; /* 當前訊息數量 */
    UBaseType_t uxLength;                   /* Queue 長度（能容納多少項目） */
    UBaseType_t uxItemSize;                 /* 每個項目的大小 */

    volatile int8_t cRxLock;  /* Receive 鎖定計數 */
    volatile int8_t cTxLock;  /* Send 鎖定計數 */

    /* 其他欄位由設定選項決定，例如：
       - ucStaticallyAllocated（記憶體配置方式）
       - pxQueueSetContainer（Queue Set 支援）
       - uxQueueNumber / ucQueueType（Trace facility） */
} xQUEUE;

typedef xQUEUE Queue_t;
```

#### 欄位解析

- **資料區管理**  
  - `int8_t * pcHead;`  
    Queue 儲存區的起始位址（信箱的開頭）。  

  - `int8_t * pcWriteTo;`  
    下一個要寫入的位置（信件將投遞到這裡）。  

  - `union { QueuePointers_t xQueue; SemaphoreData_t xSemaphore; } u;`  
    Queue 與 Semaphore 共用的額外資訊。   

- **任務等待清單**  
  - `List_t xTasksWaitingToSend;`  
    任務想投遞資料但 Queue 滿了，就被掛到這班火車等待。  

  - `List_t xTasksWaitingToReceive;`  
    任務想收資料但 Queue 空了，就被掛到這班火車等待。   

- **狀態資訊（信箱狀態）**
  - `volatile UBaseType_t uxMessagesWaiting;`  
    當前 Queue 中的訊息數量（信箱裡有幾封信）。  

  - `UBaseType_t uxLength;`  
    Queue 可容納的項目數量（信箱大小，不是 byte 數）。  

  - `UBaseType_t uxItemSize;`  
    每個項目的大小（每封信的大小）。    

- **Lock / Unlock 機制**
  - `volatile int8_t cRxLock;`  
    Queue 鎖定期間的 **Receive 操作次數**。  

  - `volatile int8_t cTxLock;`  
    Queue 鎖定期間的 **Send 操作次數**。  

  （避免中斷與任務同時操作 Queue 時產生競爭問題，解鎖後一次性處理通知。）  

- **條件編譯的擴充欄位**

  - `pxQueueSetContainer;`  
    若 Queue 被加入 Queue Set，指向它的容器。  

---

### 9.7.2 建立與初始化

Queue / Semaphore / Mutex 的建立，就像在月台上「蓋一個新的信箱或票務亭」，差別只在於用途不同，但施工流程其實是一樣的。

#### 主要 API

- `xQueueGenericCreate()`  
  建立通用 Queue，就像建造一個「通用信箱」，不管是 Queue / Semaphore / Mutex，最後都會走到這個流程，內部同時處理靜態/動態配置。

- `xQueueCreateMutex()`  
  建立互斥鎖（Mutex 本質是 itemSize = 0 的 Queue，並加入擁有者與優先權繼承機制），就像蓋一個 **專屬票亭**，只有一張票，並且加上「票必須本人歸還」與「優先權繼承」規則。 

- `xQueueCreateCountingSemaphore()`  
  建立計數型信號量（本質是長度 > 1、itemSize = 0 的 Queue），如同蓋一個 **多票票亭**，一次可發放 N 張相同的票。

- `xQueueCreateStatic()`  
  靜態建立 Queue，需由使用者提供控制結構與儲存空間，自己準備磚頭與水泥（記憶體與 buffer），不靠系統倉庫（heap）。  

#### 初始化流程

1. **配置記憶體 → 蓋信箱 / 票亭基座** 
   - 動態建立：呼叫 `pvPortMalloc()` 分配 Queue_t + buffer，如同系統工人（`pvPortMalloc()`）幫你拿磚頭蓋好。    
   - 靜態建立：使用者提供控制區與 buffer，自己帶材料（buffer + 控制結構）給工人。  

2. **初始化 Queue_t → 擺放信箱內部格子**  
   - 設定 `pcHead` → buffer (信箱)起始位址。  
   - 設定 `pcWriteTo` → buffer 起點（或第一個可用位置），下一封信要放的格子。
   - 清空 `uxMessagesWaiting` → 一開始信箱是空的。   
   - 設定 `uxLength`、`uxItemSize` → 貼上「容量標籤」，規定這個信箱能放多少封、每封大小多少。    

3. **初始化等待清單 → 設置等候區火車**    
   - `xTasksWaitingToSend` → 清空，準備放入「Queue 滿時等待的任務」，「想寄信卻滿了」的乘客，專屬等候火車。    
   - `xTasksWaitingToReceive` → 清空，準備放入「Queue 空時等待的任務」，「想收信卻沒有」的乘客，專屬等候火車。   

4. **初始化 Lock 狀態 → 上鎖檢查**  
   - 將 `cRxLock`、`cTxLock` 設為「未鎖定」。  
   - 意思是信箱剛蓋好，還沒開始整理。  
   - 如果之後需要暫時封箱（中斷或任務同時操作），會先把信件暫放一旁，等解鎖後再一次性處理通知。  

#### 實務重點

- **動態 vs 靜態建立**  
  - 動態：使用 `xQueueCreate()` → 底層呼叫 `xQueueGenericCreate()` → `pvPortMalloc()` 分配。  
  - 靜態：使用 `xQueueCreateStatic()`，由應用程式提供記憶體，確保可控性與避免 heap 依賴，更適合嵌入式。  

- **不同類型的差異**  
  - Queue → 正規信箱，能存放一封封信件（資料）。  
  - Semaphore → 票務亭，只管票數，不管內容。  
  - Mutex → 專屬票亭，票只能由原乘客歸還，還有「優先權繼承」規則避免大乘客卡太久。

- **共通觀念**  
  - 不論是 Queue / Semaphore / Mutex，本質上都是先「建立 Queue_t 並初始化」，再依類型套上額外邏輯。  

---

### 9.7.3 任務間通訊 API

任務之間透過 Queue 交換資料或訊號，核心在於「**傳送**」與「**接收**」，並且支援 **阻塞 / 超時 / ISR 場景**。

#### 傳送 API
- `xQueueGenericSend()`（基底函式，其他傳送 API 都會呼叫它）
  - **Queue 有空位** → 立即把資料寫入，返回成功。就像把信件直接投進信箱。 
  - **Queue 滿了** →  
    - 若 `xTicksToWait > 0` → 任務被掛到 `xTasksWaitingToSend`（事件清單），進入阻塞直到有空位。就像信箱塞滿了，乘客只好先搭「等待火車」等信箱有空格。  
    - 若 `xTicksToWait = 0` → 立即返回失敗，不阻塞。就像乘客不願意排隊，直接放棄寄信。  

- 常見封裝：  
  - `xQueueSendToBack()` → 傳送到 Queue 尾端（最常用）。  
  - `xQueueSendToFront()` → 傳送到 Queue 前端，類似插隊寄信，讓緊急信件優先處理。  
  - `xQueueOverwrite()` → 針對長度 = 1 的 Queue，覆蓋舊資料，常用於「狀態更新」，只保留最新的一封信。  

#### 接收 API
- `xQueueReceive()`  
  - **Queue 有資料** → 立即讀取，返回成功。就像乘客打開信箱拿到信件。  
  - **Queue 空了** →  
    - 若 `xTicksToWait > 0` → 任務被掛到 `xTasksWaitingToReceive`，進入阻塞直到有資料。就像信箱暫時沒有信，乘客搭上「等候火車」等待新信件投遞。  
    - 若 `xTicksToWait = 0` → 立即返回失敗，代表不願意等待。  

- 常見封裝：  
  - `xQueuePeek()` → 偷看 Queue 頭端資料但不移除，就像打開信箱看內容但不拿出來。  

- **關鍵控制**  
  - `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`：Queue 的底層資料區（buffer）可能同時被多個角色操作，例如 A 任務正在送信（`xQueueSend`）、B 任務正在收信（`xQueueReceive`），而中斷服務常式（ISR）也可能透過 `xQueueSendFromISR()` 操作同一個 Queue。  
    - 呼叫 `taskENTER_CRITICAL()` → 暫時關閉中斷（或至少提升遮罩等級），確保接下來的程式片段不會被打斷。  
    - 完成操作後，再呼叫 `taskEXIT_CRITICAL()` → 恢復中斷。  
    - 整個流程就是 **critical section（臨界區）保護機制**，避免多核心或中斷同時打開信箱導致資料衝突（data corruption）。  
  - 阻塞與喚醒都透過 **Event List 機制** 完成，列車長（Scheduler）會根據狀態把等待的乘客換乘到 Ready List。  

#### ISR 版本
- `xQueueGenericSendFromISR()` / `xQueueReceiveFromISR()`  
  - ISR 中不能阻塞，只能立即返回。就像「快遞員」只能當場投遞或取走，不允許排隊等候。  
  - 若成功操作 Queue，會透過 `pxHigherPriorityTaskWoken` 通知列車長，在中斷結束後立即切換任務，讓高優先權乘客直接上車執行。  

---

### 9.7.4 特殊應用 API

#### Mutex（互斥鎖）
- **建立**：`xQueueCreateMutex()`  
- **操作**：  
  - `xQueueTakeMutexRecursive()`  
  - `xQueueGiveMutexRecursive()`  
- **特性**：  
  - 基於 Queue 實作，`itemSize = 0`。  
  - 必須由「持有票的任務」自己歸還，不能代還，就像專屬票只能由原乘客退回。  
  - 支援 **優先權繼承（Priority Inheritance）**：  
    - 若低優先權任務持有 Mutex，而高優先權任務在等候，系統會暫時提升低優先權任務的優先級，讓他快點釋放資源，避免 **優先權反轉（Priority Inversion）**。  

#### Semaphore（信號量）
- **Binary Semaphore（二元信號量）**  
  - 本質是容量 = 1 的 Queue。  
  - 常用於 **事件通知**，例如 ISR 投遞一張票 → 喚醒等待的任務。  
  - 就像票務亭只有一張票，誰先領到誰先上車。  

- **Counting Semaphore（計數型信號量）**  
  - 本質是容量 > 1 的 Queue。  
  - 可同時允許 N 個任務取得資源，常用於「有限資源池」（例如多台相同機器）。  
  - 就像票務亭有多張票，可以同時發給多位乘客。  

- **關鍵 API**  
  - `xQueueSemaphoreTake()`：用於任務等待資源或事件。就像乘客去票亭領票，若沒有票就只能排隊等候。   

---

### 9.7.5 重點整理與常見考題

#### 重點整理
- **Queue_t（核心結構）**：Queue / Semaphore / Mutex 都基於同一個結構實作。  
- **建立方式**：`xQueueGenericCreate()` 為共用基底，Mutex / Semaphore 只是額外包裝與邏輯。  
- **初始化流程**：配置記憶體 → 初始化 Queue_t → 初始化等待清單 → 設定 Lock 狀態。  
- **任務通訊 API**：支援傳送 / 接收，並提供阻塞、超時、ISR 版本。  
- **Critical Section**：透過 `taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()` 保護臨界區，避免中斷或多核心同時存取 Queue。  
- **特殊應用**：  
  - Mutex：支援優先權繼承，避免優先權反轉。  
  - Binary Semaphore：事件通知（如 ISR 喚醒任務）。  
  - Counting Semaphore：管理有限資源池。  

#### Q1. Queue 滿 / 空時任務怎麼處理？
- **Queue 滿**：  
  - `xTicksToWait > 0` → 任務阻塞，掛到 `xTasksWaitingToSend` 等待清單。  
  - `xTicksToWait = 0` → 立即返回失敗。  
- **Queue 空**：  
  - `xTicksToWait > 0` → 任務阻塞，掛到 `xTasksWaitingToReceive` 等待清單。  
  - `xTicksToWait = 0` → 立即返回失敗。  
- **面試回覆**：  
「Queue 滿或空時，任務會依 `xTicksToWait` 決定是否阻塞，阻塞的任務會被掛到對應等待清單，否則立即返回。」

#### Q2. Queue 本質是什麼？
- **核心**：環形緩衝區（circular buffer）+ 兩個等待清單（Send/Receive）。  
- **面試回覆**：  
「Queue 本質上就是一個環形緩衝區，加上兩個等待清單，分別管理等待傳送和等待接收的任務。」

#### Q3. Queue 與 Semaphore/Mutex 的差別在哪？為什麼共用 Queue_t？
- Queue：用來傳遞資料。  
- Semaphore：用來表示資源數量或事件發生。  
- Mutex：用來做互斥，支援優先權繼承。  
- 三者共用 Queue_t，因為都需要「任務同步」和「等待清單」，差別只在規則。  
- **面試回覆**：  
「三者的差別在於用途不同，但都需要任務同步與等待清單，因此共用 Queue_t，只是把行為規則套上去。」

#### Q4. 為什麼 Queue 可以實現 Mutex / Semaphore？
- 因為 Queue_t 已經具備：  
  - 任務阻塞 / 喚醒機制（等待清單）。  
  - 狀態管理（信件數量可當票數）。  
- 所以只要把 `itemSize` 設為 0，就能變成 Mutex 或 Semaphore。  
- **面試回覆**：  
「因為 Queue_t 內建了等待清單與狀態管理，只要把資料單元大小設為 0，就能改成資源控制用途，所以能延伸實作 Mutex 與 Semaphore。」

#### Q5. 說出 Queue 建立 API 有哪些？差別在哪？
- 動態：`xQueueCreate()` → 透過 `pvPortMalloc()` 配置記憶體。  
- 靜態：`xQueueCreateStatic()` → 使用者自行提供控制結構與 buffer。  
- **面試回覆**：  
「Queue 有動態與靜態兩種建立方式，動態依賴 heap，靜態由使用者提供記憶體，更適合嵌入式環境。」

#### Q6. 如何用 Queue 在任務之間傳遞資料？
- 任務 A：`xQueueSend()` 把資料送進 Queue。  
- 任務 B：`xQueueReceive()` 從 Queue 取出資料。  
- 支援阻塞、超時，保證同步。  
- **面試回覆**：  
「Queue 是最常用的任務間通訊方式，一端送、一端收，系統會管理同步與阻塞機制。」

#### Q7. `xQueueOverwrite()` 適用於什麼情境？
- 僅能用於 **Queue 長度 = 1**。  
- 新資料會覆蓋舊資料。  
- 常用於 **狀態更新**，例如感測器最新數值。  
- **面試回覆**：  
「`xQueueOverwrite()` 適合只需要最新值的情境，例如感測器讀值，它會覆蓋舊資料，確保接收端拿到的是最新狀態。」

#### Q8. ISR 中使用 Queue API 與任務中有何不同？
- 任務中：允許阻塞等待。  
- ISR 中：不能阻塞，只能立即返回。  
- **面試回覆**：  
「任務可以用阻塞 API，但 ISR 不允許阻塞，所以在 ISR 中必須用特別設計的非阻塞版本。」

#### Q9. 為什麼 ISR 不能用一般 API，要用 FromISR 版本？
- 一般 API（例如 `xQueueSend()`）可能阻塞，而 ISR 絕對不能阻塞。  
- FromISR 版本會立即返回，並支援 `pxHigherPriorityTaskWoken` 在中斷結束後切換任務。  
- **面試回覆**：  
「ISR 中必須用 FromISR 版本，因為 ISR 不能阻塞。這些 API 設計為立即返回，並能在中斷結束後安全喚醒高優先權任務。」

#### Q10. Queue 的 Lock 機制 (cRxLock / cTxLock)

- **作用是什麼？**  
  - `cRxLock`：在 Queue 被鎖定期間，記錄「接收操作（Receive）」的次數。  
  - `cTxLock`：在 Queue 被鎖定期間，記錄「傳送操作（Send）」的次數。  
  - 主要用途：避免在 **中斷或多任務同時操作 Queue** 時，立即觸發多次喚醒或通知，導致效能浪費或狀態混亂。  

- **為什麼需要「暫存通知，解鎖後一次處理」？**  
  - 在臨界區（critical section）內，若每次操作都立刻喚醒等待任務，可能造成「重複切換任務」的開銷。  
  - 使用 Lock 機制後：  
    - 系統會先「累積計數」→ 把這段期間發生的操作記錄下來。  
    - 等解鎖時，再一次性處理喚醒或通知，確保效率與一致性。  
  - 就像「整理信箱時先上鎖，把投遞和收取的動作先記錄下來，等解鎖後再一次通知乘客」，避免中途插隊或反覆叫人。  

> **面試回覆**：  
「Queue 的 Lock 機制透過 `cRxLock` 和 `cTxLock` 記錄接收和傳送操作，在鎖定期間先暫存事件，等解鎖後再一次性處理通知。這樣能避免頻繁任務切換，提高效率，並確保狀態一致。」

---

## 9.8 FreeRTOS heap_4.c 記憶體管理解析

### 9.8.1 記憶體管理的基本概念
在 FreeRTOS 中，`heap_x.c` 檔案位於 `portable/MemMang` 資料夾，屬於 **記憶體管理的移植層**，提供多種不同策略的動態記憶體配置實作。  

主要用途：  
- 動態建立 **Task（任務）**  
- 動態建立 **Queue（佇列）**  
- 動態建立 **Semaphore（訊號量）**  
- 動態建立 **Mutex（互斥鎖）**

#### 火車比喻與 heap_x.c 的關聯
火車系統都不是憑空冒出來的：  
- **Task（乘客）** 需要 TCB（身份證）。  
- **Queue（信箱）** 需要 buffer（信箱格子）。  
- **List（火車）** 需要車廂（ListItem）。  

這些建材都必須從 **heap_x.c** 來配置。  

`heap_x.c` 就像「月台旁的倉庫」：  
- 建立任務 → 從 heap 拿 TCB + stack。  
- 建立 Queue → 從 heap 拿 Queue_t + buffer。  
- 建立 ListItem → 從 heap 拿 ListItem 結構。  
- 建立 Semaphore / Mutex → 同樣從 heap 拿控制結構。  

可以將倉庫（heap）想成原本是一條長長的連續空間：  
- 假設先後借了 100 單位與 200 單位的建材，在不同時間單獨歸還後 → 倉庫會出現許多小碎塊。  
- 長時間下來，即使總空間足夠，仍可能因為被切碎而找不到「一塊連續的大空間」來分配。  

所以這個倉庫有五種管理方式：  
- 有些方式只會一直切割建材（heap_1），用過就不回收。  
- 有些方式會允許歸還，但不會合併（heap_2）。  
- 有些方式直接借用標準函式庫的 malloc/free（heap_3）。  
- 有些方式會在歸還時，將相鄰的建材推在一起，避免碎片化（heap_4、heap_5）。 

某些管理方式（如 heap_4）會檢查：  
- **如果左右鄰居也是空的** → 把它們合併成一個更大的空閒區塊。  
- **如果不是** → 就單獨保留這塊。  

#### 各種 `heap_x.c` 策略比較

1. `heap_1.c` — 單純配置（Only malloc）
  - 僅支援配置（malloc），**不支援釋放（free）**。  
  - **優點**：最簡單、最快速，無碎片化問題。  
  - **缺點**：記憶體一旦配置就無法回收，僅適合任務/資源數量固定的系統。  

2. `heap_2.c` — 可回收但不合併
  - 支援 malloc/free。  
  - 記憶體釋放後會回到 free list，但**不會合併相鄰空間**。  
  - **缺點**：長時間運行容易產生碎片化。 

3. `heap_3.c` — 包裝標準函式庫
  - 直接呼叫 **C 標準函式庫的 malloc/free**。  
  - **優點**：方便、開發快速。  
  - **缺點**：不可預測延遲，行為依賴平台 malloc 實作，不適合即時系統。  

4. `heap_4.c` — 可回收且合併（最常用）
  - 支援 malloc/free。  
  - 釋放記憶體時會嘗試**合併相鄰空閒區塊**，減少碎片化。  
  - **優點**：穩定性與效率兼具，使用最廣泛。  
  - **適合**：大部分中小型嵌入式系統。 

5. `heap_5.c` — 多區域支援
  - 功能與 heap_4 類似，但可管理 **多個非連續記憶體區域**。  
  - **適合**：有多段 RAM 或外部 RAM 的複雜系統。  

#### 為什麼選用 heap_4？
- **特點**：支援 malloc/free，能釋放並合併相鄰區塊，效率與穩定性比 heap_2 更佳。  
- **適用場景**：  
  - 系統運行時間長，需要避免碎片化。  
  - 記憶體需求動態變化，例如任務或 Queue 會在不同時間建立/刪除。  
- **實務觀念**：  
  - 若系統對即時性要求極高，建議還是優先採用靜態配置（例如 `xTaskCreateStatic()`）。  
  - heap_4 適合作為一般嵌入式專案的預設選擇。  

---

### 9.8.2 heap_4 核心資料結構

#### BlockLink_t 結構
- **用途**  
  `BlockLink_t` 是 **heap_4.c 用來管理空閒記憶體區塊的鏈結節點**。  
  每個尚未被使用的區塊都會附上一張這樣的「庫存單」，由它們串起來形成 **Free List（空閒清單）**，方便記憶體管理程式分配與回收。  

- **結構原型**
  ```c
  typedef struct A_BLOCK_LINK
  {
      struct A_BLOCK_LINK * pxNextFreeBlock; /**< 下一個空閒區塊 */
      size_t xBlockSize;                     /**< 區塊大小（含這個控制結構本身） */
  } BlockLink_t;
  ```

- **重要欄位**  
  1. `struct A_BLOCK_LINK * pxNextFreeBlock`  
     - 指向下一個空閒區塊。  
     - 所有空閒區塊會依「**位址排序**」串成單向鏈結清單，釋放時方便檢查相鄰區塊是否能合併。   

  2. `size_t xBlockSize`  
     - 記錄區塊大小（包含這個控制結構本身）。  
     - **最高位元（MSB）** 被用來標記狀態：  
       - MSB = 1 → 區塊已被分配。  
       - MSB = 0 → 區塊仍在 Free List 中。 

- **火車比喻**  
  - **BlockLink_t** 就像是「庫存單」：  
    - `pxNextFreeBlock` → 指出「下一堆建材在哪裡」。  
    - `xBlockSize` → 寫著「這堆建材有多大」，角落打勾（MSB）標示「在用 / 空閒」。  
  - 管理員只要沿著庫存單清單走，就能掌握所有空閒區塊大小與位置，並快速判斷是否能合併。  

#### Free List（空閒清單）
- **用途**  
  Free List 是由多個 `BlockLink_t` 串起來的**單向鏈結清單**，代表「目前可用的空閒記憶體區塊」。  
  它本身不是一個結構，而是一條由哨兵節點維護的清單。  

- **組成與特性**  
  1. **按位址排序**  
     - 所有 free block 依照記憶體位址由**小到大**排序。  
     - 這樣釋放時只要檢查前後，就能判斷是否能合併。  

  2. **單向鏈結清單**  
     - 透過 `pxNextFreeBlock` 串起來，不需要雙向鏈結，降低額外成本。  

  3. **特殊節點（哨兵）**  
     - **起始哨兵（xStart）** → 指向第一個有效 free block，本身不是一個可分配的 free block，它只是個「固定頭節點」，所以它的 `xBlockSize` 沒有實際意義 → 設為 0。  
     - **結尾哨兵（pxEnd）** → 被放在 heap 的結尾，用來標記清單結束。最後一個 free block 的 `pxNextFreeBlock` 會指向 `pxEnd`，而 `pxEnd->xBlockSize = 0`，本身不是可分配區塊。 

- **運作流程**  
  - **配置 (pvPortMalloc)**：    
    - 從 Free List 的開頭（`xStart.pxNextFreeBlock`）**依序尋訪**，採用 **first-fit**：找到第一個 `xBlockSize >= 實際需求大小` 的 free block。  
    - 若該區塊「比需求大很多」，且**切割後的剩餘空間仍 ≥ 最小可用區塊大小**（含標頭與對齊），就**拆成兩塊**：  
      - 一塊分配給「呼叫者」（例如 Task、Queue 等 RTOS 元件）。  
      - 剩餘部分保留在 Free List，更新其 BlockLink_t 與鏈結。  
    - 對分配出去的區塊：**設置 `xBlockSize` 的 MSB**（標記為「已分配」），並回傳給呼叫者「資料區指標」（位於 BlockLink_t 標頭之後）。
   
  - **釋放 (vPortFree)**：  
    - 由傳入指標回推到 BlockLink_t 標頭，並**清除 `xBlockSize` 的 MSB**（從「已分配」改為「空閒」）。  
    - 沿著**按位址排序**的 Free List，找到它**應插入的位置**（落在兩個相鄰 free block 之間），插回清單。  
    - 檢查前一塊與下一塊是否與本區塊**相鄰（地址連續）**：  
      - 若相鄰則**合併**（更新 `xBlockSize`，調整 `pxNextFreeBlock`），以減少碎片化。  

---

### 9.8.3 初始化流程

`heap_4.c` 在編譯時會從 **RAM** 中保留一塊「固定大小的記憶體區域」，作為 FreeRTOS 專用的 heap。  
當使用 FreeRTOS API 建立 **Task、Queue、Semaphore、Mutex** 時，底層會呼叫 `pvPortMalloc()` 分配記憶體。  
這個 `pvPortMalloc()` 並不是標準 C 的 `malloc()`，而是 FreeRTOS 提供的專屬配置器。

#### 用途  
在系統第一次呼叫 `pvPortMalloc()` 之前，`heap_4` 需要先透過 `prvHeapInit()` 完成初始化。  
這個初始化流程會建立 **哨兵節點** 以及 **初始 Free List**，確保後續的配置與釋放能正常運作。  

系統剛啟動時，heap 內容還是「空的」，因此 Free List 幾乎等於整個 heap 區域。  
初始化完成後：  
- **xStart（頭哨兵）** 是一個固定的控制節點，`xBlockSize = 0`，它的 `pxNextFreeBlock` 指向第一個真正的 free block。  
- **第一個 free block** 代表整個 heap 的可用空間（扣除哨兵與結尾控制用的保留區）。  
- **第一個 free block 的 `pxNextFreeBlock`** 則指向 **pxEnd（尾哨兵）**，表示鏈結清單結束。  

因此，在剛完成 `prvHeapInit()` 的狀態下，Free List 裡僅有一個「可分配的區塊」，覆蓋幾乎整個 heap 空間。 

#### 函式原型（精簡版）
```c
static void prvHeapInit( void )
{
    BlockLink_t *pxFirstFreeBlock;
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;
    portPOINTER_SIZE_TYPE uxStartAddress, uxEndAddress;

    /* 將 heap 起始位址對齊 */
    uxStartAddress = ( portPOINTER_SIZE_TYPE ) ucHeap;
    if( ( uxStartAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
    {
        uxStartAddress += ( portBYTE_ALIGNMENT - 1 );
        uxStartAddress &= ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );
        xTotalHeapSize -= ( size_t )( uxStartAddress - ( portPOINTER_SIZE_TYPE ) ucHeap );
    }

    /* 建立起始哨兵 (xStart) */
    xStart.pxNextFreeBlock = ( void * ) uxStartAddress;
    xStart.xBlockSize = 0;

    /* 建立結尾哨兵 (pxEnd) */
    uxEndAddress = uxStartAddress + xTotalHeapSize;
    uxEndAddress -= xHeapStructSize;               /* 保留標頭空間 */
    uxEndAddress &= ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK );
    pxEnd = ( BlockLink_t * ) uxEndAddress;
    pxEnd->xBlockSize = 0;
    pxEnd->pxNextFreeBlock = NULL;

    /* 建立第一個 free block，覆蓋整個 heap 區域 */
    pxFirstFreeBlock = ( BlockLink_t * ) uxStartAddress;
    pxFirstFreeBlock->xBlockSize = ( size_t )( uxEndAddress - ( portPOINTER_SIZE_TYPE ) pxFirstFreeBlock );
    pxFirstFreeBlock->pxNextFreeBlock = pxEnd;

    /* 初始化全域統計量 */
    xFreeBytesRemaining = pxFirstFreeBlock->xBlockSize;
    xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
}
```  

#### 初始化步驟概述
0. **對齊起點並調整可用大小**  
   - 在 `heap_4.c` 裡，heap 是一個靜態陣列；它的起始位址（`&ucHeap[0]`）由編譯器配置，**不保證符合 CPU 對齊**，因此初始化時必須做 **alignment**。  
   - 若對齊需求是 8 bytes，則例如：  
     `0x1003 → 0x1008`、`0x1009 → 0x1010`。  
   - 若原本 heap 起點是 `0x1002`，對齊到 `0x1008` 後，`0x1002 ~ 0x1007` 這 6 bytes 不能用，需自 `configTOTAL_HEAP_SIZE` 扣除，得到實際可用大小。

1. **建立起始哨兵（xStart）**  
   - `xStart` 為 **全域 BlockLink_t 結構本體**，位於 `.bss/.data`（**不在 heap 內**）。  
   - 設定 `xStart.xBlockSize = 0`。  
   - 作為固定頭節點，`xStart.pxNextFreeBlock` 指向 **第一個 free block**（即對齊後的 heap 起點）。  

2. **建立結尾哨兵（pxEnd）**  
   - `pxEnd` 為 **指標變數**（在 `.bss/.data`），**指向位於 heap 尾端的一個 BlockLink_t 哨兵節點本體**。  
   - 初始化時將這個哨兵節點本體放在 **heap 的最後**（扣除標頭大小並再次對齊後的位置），並設 `pxEnd->xBlockSize = 0`。  
   - **最後一個 free block 的 `pxNextFreeBlock` 會指向 `pxEnd`**，用以標記 Free List 的結尾。  

3. **建立第一個 free block**  
   - 系統初始化時，整個 heap（`ucHeap[]` 陣列，經過 alignment 調整後）會被描述成一個大的 free block。  
   - 這個 free block 的大小需要扣除：  
     - 起始對齊時浪費掉的 bytes。  
     - 尾端哨兵所需的空間（`sizeof(BlockLink_t)`，注意：`pxEnd` 是指標，不是額外的空間）。  
   - 因此：  
     `xBlockSize = configTOTAL_HEAP_SIZE - alignment_overhead - sizeof(BlockLink_t)`  
   - 並設定 `pxNextFreeBlock` 指向 `pxEnd`，表示這是唯一一塊 free block，後面緊接著尾端哨兵。  

4. **串接 Free List**  
   - 初始化完成後，Free List 的鏈結關係為：  
     `xStart → 第一個 free block → pxEnd`  
   - 此時 Free List 中只有 **一個有效的 free block**（涵蓋整個可用的 heap 區域）。  
   - 後續每次配置 (pvPortMalloc) 就會從這個 free block 中切出一塊，釋放 (vPortFree) 則可能將區塊合併回來。 

---

### 9.8.4 重點整理與常見考題 

#### 重點整理
- **heap_x.c 系列**：提供五種記憶體管理策略（heap_1 ~ heap_5），由使用者在設定檔選擇。  
- **BlockLink_t**：每個 free block 前的標頭，記錄大小 (`xBlockSize`) 與下一個區塊位置 (`pxNextFreeBlock`)，MSB 用來標記是否已被分配。  
- **Free List**：所有 free block 依照位址由小到大串成單向鏈結清單，方便插入與合併。  
- **哨兵節點**：  
  - `xStart`（頭節點，不在 heap 內，`xBlockSize=0`）。  
  - `pxEnd`（尾端節點，本體放在 heap 末尾，`xBlockSize=0`）。  
- **初始化流程**：  
  - 對齊 heap 起點並調整大小 → 建立 xStart、pxEnd → 建立第一個 free block → 串接 Free List。  
- **配置流程 (pvPortMalloc)**：first-fit 找到合適區塊 → 必要時拆分 → 設置 MSB → 更新鏈結。  
- **釋放流程 (vPortFree)**：清除 MSB → 插回 Free List → 檢查相鄰區塊 → 可合併則合併，降低碎片化。  
- **實務考量**：  
  - heap_4 可合併，長時間運行較穩定。  
  - 但 RTOS 中仍建議能靜態配置就靜態配置，以保證即時性與可控性。  

#### Q1. FreeRTOS 提供哪幾種 heap 管理策略？差別在哪？  
- **heap_1**：只 malloc 不 free，簡單快速。  
- **heap_2**：可 free，但不合併碎片。  
- **heap_3**：直接呼叫標準 malloc/free，不保證即時性。  
- **heap_4**：可 free 並合併相鄰碎片，最常用。  
- **heap_5**：支援多個不連續的記憶體區域。  
- **面試回覆**：  
「heap_4 是最常用的，因為它支援 malloc/free 並能合併碎片，適合長時間運行的系統。」

#### Q2. BlockLink_t 結構裡的兩個欄位（pxNextFreeBlock / xBlockSize）各自的用途是什麼？  
- `pxNextFreeBlock`：指向下一個 free block，形成鏈結清單。  
- `xBlockSize`：紀錄區塊大小，MSB 用來標記該區塊是否已分配。  
- **面試回覆**：  
「BlockLink_t 是每塊 free block 的標頭，記錄大小和下一塊位置，MSB 標示該區塊是否在用。」

#### Q3. Free List 為什麼要照位址排序？  
- 好處：釋放時只需檢查前後區塊即可判斷能否合併。  
- **面試回覆**：  
「因為 Free List 依位址排序，所以釋放時能快速找到位置並合併相鄰區塊，避免碎片化。」

#### Q4. 初始化時，xStart / 第一個 free block / pxEnd 三者的關係是什麼？  
- xStart：頭哨兵，指向第一個 free block。  
- 第一個 free block：涵蓋整個 heap 的主要空間。  
- pxEnd：尾哨兵，標記清單結尾。  
- **面試回覆**：  
「初始化後，Free List 就是 `xStart → 第一個 free block → pxEnd`。」

#### Q5. 為什麼 heap 起點要做 alignment？  

- **硬體原因**  
  - 多數 CPU 的記憶體匯流排以「字 (word) 對齊」來存取。  
  - 例如：32-bit CPU 一次讀 4 bytes，64-bit CPU 一次讀 8 bytes。  
  - 若資料從未對齊位址開始（如 0x1003），CPU 可能要做「兩次存取再合併」，甚至在某些架構上直接觸發 **Bus Fault**。  

- **效能原因**  
  - 對齊存取僅需一次 cycle；未對齊存取會多出額外 cycle 或特殊處理，導致效能下降。  

- **軟體相容性**  
  - C 語言標準允許編譯器假設結構體成員是對齊的。  
  - 如果 malloc 回傳未對齊位址，結構體存取可能失敗或造成 crash。  

- **面試回覆**  
「因為 CPU 的匯流排和指令集要求記憶體對齊。對齊後 CPU 一次就能正確抓到整個 word；未對齊則會降低效能，甚至導致 Bus Fault。因此 heap 初始化時要把起點調整到 alignment 邊界，浪費掉的空間會直接扣掉。」

#### Q6. 剛初始化完成時，Free List 的鏈結長什麼樣子？  
- 結構：`xStart → 第一個 free block → pxEnd`。  
- **面試回覆**：  
「剛開始時只有一個大 free block，頭尾哨兵把它包住。」

#### Q7. 為什麼需要 pxEnd 這個尾端哨兵？  
- 明確標記 Free List 的結尾，簡化操作。  
- 沒有 pxEnd，最後一塊 free block 的處理會變複雜。  
- **面試回覆**：  
「pxEnd 是尾哨兵，用來標記結尾，沒有它會增加額外判斷。」  

#### Q8. 初始化時第一個 free block 的大小怎麼計算？  
- 公式：  
  `xBlockSize = configTOTAL_HEAP_SIZE - alignment_overhead - sizeof(BlockLink_t)`  
- **面試回覆**：  
「第一個 free block 的大小 = 總大小 − 對齊浪費 − 尾端哨兵空間。」

#### Q9. heap_4 如何降低碎片化風險？與 heap_2 相比有什麼優勢？若系統需要長期穩定運行，是否建議使用 heap_4？  
- **heap_2**：釋放區塊後不會合併 → 容易產生碎片，長時間運行後即使總空間足夠，也可能找不到一塊連續大空間。  
- **heap_4**：釋放時會檢查前後相鄰區塊，若連續則合併 → 有效降低碎片化風險。  
- **長期穩定運行**：heap_4 適合大部分情況，但若系統強調即時性與高度可控，仍建議優先使用 **靜態配置**（如 `xTaskCreateStatic()`），完全避免碎片問題。  
- **面試回覆**：  
「heap_4 釋放時會合併相鄰區塊，能降低碎片化風險，這是它比 heap_2 更穩定的原因。不過如果系統需要絕對的穩定與即時性，會更建議使用靜態配置。」

#### Q10. 為什麼 FreeRTOS 建議盡量用靜態配置（xTaskCreateStatic 等），而不是完全依賴 heap？  
- malloc/free 在即時系統中有延遲與碎片化風險。  
- 靜態配置可預測且可控。  
- **面試回覆**：  
「RTOS 中為了即時性，盡量用靜態配置，heap_4 適合一般需求，但靜態更安全可靠。」

---

## 9.9 FreeRTOSConfig.h 系統配置解析

### 9.9.1 這份檔案負責什麼？（角色定位）

- `FreeRTOSConfig.h` 是 **FreeRTOS 專案中最重要的設定檔**。  
- 它不是核心程式碼的一部分，而是**專案自己提供的檔案**，由開發者來定義哪些功能要打開、資源要怎麼分配。  
- 可以把它想像成 **總開關面板**，其他檔案像 `task.c`、`queue.c`、`list.c`、`heap_x.c` 都會依照這裡的開關來決定行為。

1. **核心參數（Kernel basics）**
   - 例如：
     - `configUSE_PREEMPTION`：是否用搶佔式排程。
     - `configTICK_RATE_HZ`：系統心跳頻率（通常設 1000 → 每 1ms 一次）。
     - `configMAX_PRIORITIES`：最大任務優先權數。
     - `configMINIMAL_STACK_SIZE`：Idle task / 小任務的堆疊大小。
     - `configTOTAL_HEAP_SIZE`：整個 FreeRTOS 動態記憶體池大小。
   - 這些決定了 **RTOS 的基本運作規則**。

2. **功能開關（Features）**
   - 控制哪些 RTOS 功能要啟用：
     - `configUSE_MUTEXES` → Mutex 支援。
     - `configUSE_COUNTING_SEMAPHORES` → Counting semaphore。
     - `configUSE_TASK_NOTIFICATIONS` → Task Notification。
     - `configUSE_TIMERS` → 軟體定時器（需要一個 Timer 服務任務）。
     - `configUSE_QUEUE_SETS` → Queue Sets。
     - `configUSE_STREAM_BUFFERS` / `configUSE_MESSAGE_BUFFERS` → Stream/Message buffer。
   - 這些就像決定「專案裡要不要裝這些模組」。

3. **中斷安全界線（IRQ gates）**
   - 在 Cortex-M，**中斷優先權數字越小，越高優先**。
   - `configMAX_SYSCALL_INTERRUPT_PRIORITY` → 設定 **能夠安全呼叫 FreeRTOS API 的最高中斷優先權**。  
     - 比這個優先權高的 IRQ：不能呼叫任何 FreeRTOS API。  
     - 比這個優先權低的 IRQ：可以安全呼叫 `xxxFromISR()`。
   - 這是避免「高優先 IRQ 打斷 RTOS 臨界區」的安全線。

4. **診斷鉤子（Hooks / Assert）**
   - 用來 debug 或插入自訂程式碼：
     - `configUSE_IDLE_HOOK` → Idle task 每次循環會呼叫。
     - `configUSE_TICK_HOOK` → 每次 tick interrupt 呼叫。
     - `configUSE_MALLOC_FAILED_HOOK` → 記憶體配置失敗呼叫。
     - `configCHECK_FOR_STACK_OVERFLOW` → 任務堆疊溢位檢查。
     - `configASSERT(x)` → 條件不成立時停住，方便 debug。
   - 這些是「保護與除錯用的安全機制」。

可以把 FreeRTOS 想像成一個「火車月台系統」：
- **queue / list / heap** 是建材（信箱、火車、倉庫）。  
- 這些建材要不要存在、能不能用，都要先透過 **`FreeRTOSConfig.h` 的開關**來決定。  
- **IRQ gates** 就像月台的「閘門」，決定哪些乘客（中斷服務程式）能進來搭火車（呼叫 API）。  
- **Hooks / Assert** 則像安全員，發現異常就拉下緊急煞車。

所以 `FreeRTOSConfig.h` 就是整個 RTOS 的**總開關面板**，決定了核心參數（tick、優先權、堆疊）、功能模組（mutex、notification、timer）、中斷能不能安全呼叫 API，以及除錯機制（assert、stack overflow check）。所有任務、Queue、Heap 能不能運作，基本上都受它影響。

---

### 9.9.2 必要核心設定（Minimal viable set）

> **目的**：這些參數是最小必要的設定，能讓 FreeRTOS 順利進入排程，並且讓像 `vTaskDelay()` 這樣的 API 可以正確運作。沒有這些，系統連基本的多工都跑不起來。

---

#### 1. 排程與時脈（Scheduling & Clock）

- **`configUSE_PREEMPTION = 1`**
  - 是否啟用 **搶佔式排程（Preemptive Scheduling）**。  
  - 設為 1 → 高優先權任務能隨時中斷低優先權任務。  
  - 設為 0 → 變成合作式（Cooperative），任務要自己「讓出 CPU」。  
  - 大部分情況（特別是即時系統），都設為 1。

- **`configCPU_CLOCK_HZ = SystemCoreClock`**
  - 告訴 RTOS **CPU 的核心時脈頻率**。  
  - 在 STM32 系列，`SystemCoreClock` 是 CMSIS 提供的全域變數，代表實際 CPU Hz（例如 180 MHz）。  
  - FreeRTOS 會用這個值去計算 SysTick reload 值。

- **`configTICK_RATE_HZ = 1000`**
  - 定義 **一秒有多少個 tick**。  
  - 1000 → 每 1ms 發生一次 SysTick 中斷。  
  - `vTaskDelay(100)` 就表示延遲 100 個 tick，也就是 100ms。

- **`configMAX_PRIORITIES = 7`**
  - 任務優先權的級數（0 ~ 6）。  
  - 數字越大 = 優先權越高。  
  - 不需要設太大，因為每個優先權層級都會佔用一些資源。

- **`configMINIMAL_STACK_SIZE = 128`**
  - Idle Task 以及簡單任務的基準堆疊大小。  
  - 單位是 **word（通常 4 bytes）**，所以 128 → 512 bytes。  
  - 複雜任務需要自己在 `xTaskCreate()` 指定更大的堆疊。

- **`configTOTAL_HEAP_SIZE = 16*1024`**
  - FreeRTOS 內部用來配置記憶體的池大小（bytes）。  
  - 建立任務（TCB + Stack）、Queue、Semaphore 都會從這裡分配。  
  - 太小 → 任務建立失敗；太大 → 佔用 SRAM。  
  - 面試常會問：「你怎麼決定 heap 大小？」 → 可以回答：「根據任務數量和 Stack 預估」。

- **`configUSE_16_BIT_TICKS = 0`**
  - 決定 tick counter 用 16-bit 還是 32-bit。  
  - 設 0 → 用 32-bit，約可數十天不溢位。  
  - 設 1 → 用 16-bit，節省記憶體，但最多只能計數到 65535 tick（若 1kHz = 65 秒就溢位）。  
  - Cortex-M 系列一般都設 0。

- **`configIDLE_SHOULD_YIELD = 1`**
  - Idle Task 是否要主動讓出 CPU。  
  - 設為 1 → Idle Task 如果有人跟它同優先權，會主動切換。  
  - 一般保持 1。

---

#### 2. API 可用性（Which APIs are included）

- **`INCLUDE_vTaskDelay = 1`**
  - 是否允許使用 `vTaskDelay()`。  
  - 這是最常用的 API → 用來讓任務延遲一段時間。

- **`INCLUDE_vTaskDelayUntil = 1`**
  - 是否允許使用 `vTaskDelayUntil()`。  
  - 適合用在 **週期性任務**，能保證固定頻率，而不是單純「睡一段時間」。

> 這兩個通常必開，不然最基本的 Task 延遲功能都沒辦法用。

---

#### 3. 核心例外對應（CMSIS Handler Mapping）

FreeRTOS 在 Cortex-M 平台上，必須依靠三個「特殊例外中斷」才能運作。  
這些 Handler 要正確對應到 CMSIS 預設的名稱，不然系統根本不會進入排程。

**必要的三個 Handler**

- **`vPortSVCHandler = SVC_Handler`**  
  - SVC (Supervisor Call) 用來啟動第一個任務。  
  - 當你呼叫 `vTaskStartScheduler()`，系統會透過一次 **SVC 呼叫**，把控制權切到第一個 Ready 任務。  
  - 如果這個 Handler 沒接好 → RTOS 永遠停在 `main()`，任務不會啟動。

- **`xPortPendSVHandler = PendSV_Handler`**  
  - PendSV (Pended Supervisor Call) 是 **任務切換（Context Switch）的主角**。  
  - 每次 RTOS 決定要換任務，就會觸發一次 PendSV。  
  - 如果這個 Handler 沒接好 → 任務永遠不會切換（系統卡死在同一個任務）。

- **`xPortSysTickHandler = SysTick_Handler`**  
  - SysTick 是 Cortex-M 提供的「系統時基定時器」。  
  - FreeRTOS 預設用它來產生 **tick interrupt**，每次中斷 tick count +1。  
  - `vTaskDelay()`、時間片輪轉、任務延遲/喚醒全靠它。  
  - 如果這個 Handler 沒接好 → `vTaskDelay()` 無效，系統沒有時間感。

**若不用 SysTick，改用一般 Timer（如 TIM6/TIM7）**

在 STM32 等 Cortex-M 專案中，FreeRTOS **預設使用 SysTick** 作為 RTOS tick 來源，因此 `FreeRTOSConfig.h` 會定義：

```c
#define xPortSysTickHandler SysTick_Handler
```

但若 SysTick 已被 **HAL 或其他模組佔用**，可改用一般 Timer（例如 TIM6）。  

做法如下：  

1. **在 Timer 的 IRQ Handler 中呼叫 FreeRTOS 的 Tick 函式：**
   ```c
   void TIM6_DAC_IRQHandler(void)
   {
       if(LL_TIM_IsActiveFlag_UPDATE(TIM6))
       {
           LL_TIM_ClearFlag_UPDATE(TIM6);
           xPortSysTickHandler(); // 通知 FreeRTOS：tick +1
       }
   }
   ```

2. **修改 `FreeRTOSConfig.h`**  
   - 不再將 `xPortSysTickHandler` 映射到 `SysTick_Handler`。  
   - 因為 tick 來源已經改為 TIM6。   

3. **確保 Timer 更新頻率 = `configTICK_RATE_HZ`**  
   - 若 `configTICK_RATE_HZ = 1000`，Timer 必須每 **1ms 觸發一次中斷**。  
   - 計算方式：  
     ```
     Timer頻率 = CPU時脈 / (Prescaler + 1)
     更新週期 = (ARR + 1) / Timer頻率
     ```
   - 必須精準算到 1ms，否則 `vTaskDelay(100)` 就不會是 100ms。
  
> 補充 :
1. **Handler 沒映射好** → RTOS 無法進入排程。  
2. **SysTick 和 HAL 打架** → HAL 的 `SysTick_Handler()` 和 RTOS 衝突，時間會亂。  
3. **Timer 設錯頻率** → `vTaskDelay(100)` 執行結果不是 100ms，而是過快或過慢。  

**小結**

在 Cortex-M 上，FreeRTOS 必須依賴三個核心例外：   

- **SVC**：啟動第一個任務   
- **PendSV**：負責任務切換。  
- **SysTick 或 Timer**：提供系統時間基準（tick）。  

這三個 Handler 必須在 `FreeRTOSConfig.h` 正確映射到 CMSIS 預設名稱，不然 RTOS 根本跑不起來。  
如果不用 SysTick，而是改用 Timer（像 TIM6），就要在 Timer IRQ handler 裡呼叫 `xPortSysTickHandler()`，並保證 Timer 設定頻率與 `configTICK_RATE_HZ` 一致，這樣 `vTaskDelay()` 和排程才會正常。

---

### 9.9.3 中斷優先權與「可呼叫 API」的安全線

**核心觀念**  
在 FreeRTOS 中，**不是所有中斷都能呼叫 FreeRTOS API**（即使是 `FromISR` 版本）。  

原因是 FreeRTOS API 在內部會操作核心資料結構，這些操作需要在臨界區 (Critical Section) 保護。如果**高優先權中斷**在臨界區期間進來並呼叫 API，就可能破壞排程器，導致**死鎖或錯誤**。  

因此 FreeRTOS 規定：**只有優先權數字大於 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的中斷，才可以安全呼叫 API**。  
否則必須用 **flag、信號量或事件** 通知任務，由任務再處理。 

#### 1. 優先權數字的意義
- 在 STM32F4（Cortex-M4F），有 **4 個優先權位元** → 可設定 0 ~ 15。  
- **數字越小 → 優先權越高**。  

#### 2. FreeRTOS 的規定
- **`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5`**  
  → 意味著 **優先權 ≤ 5** 的中斷 **不能呼叫 FreeRTOS API**。  
- **只有優先權 > 5 的中斷（數字大）才可以安全呼叫 FreeRTOS API**。

#### 3. 口訣
- **高優先（小數字）= 快，但不能進 RTOS**。  
- **低優先（大數字）= 慢，但可以呼叫 RTOS API**。  

**範例**：  
把 **USART IRQ** 設成優先權 7~10，這樣就能安全地在 UART 中斷裡用 `xQueueSendFromISR()`。

#### 4. ISR 呼叫 FreeRTOS API 有什麼限制？

要注意 `configMAX_SYSCALL_INTERRUPT_PRIORITY`。在 STM32F4，優先權數字 ≤ 5 的中斷不能呼叫 RTOS API，只有優先權數字大於 5 的中斷才行。

---

### 9.9.4 常見地雷與檢查清單（Checklist）

1. **PRIGROUP 與 `configPRIO_BITS` 不一致** → NVIC 優先權解讀錯亂，ISR 呼叫 API 直接爆。  
2. **把高優先（小數字）IRQ 放進 RTOS** → 例如把 `USART` 設成 3 然後呼叫 `xQueueSendFromISR()` → **違規**。  
3. **SysTick 來源切換**：若不用 `SysTick_Handler` 當 tick（改 TIM6/TIM7），記得：
   - 關 `xPortSysTickHandler` 映射、改在對應 IRQ 內呼叫 `xPortSysTickHandler()` 或移植層替代流程。  
4. **Heap 不足**：任務建立失敗或 queue 建立失敗 → 先調 `configTOTAL_HEAP_SIZE`，再檢查 `heap_x.c` 選型。  
5. **Hook 誤開**：Tick Hook 空轉太重或 Idle Hook 阻塞。Bring-up 階段建議全部關閉，逐步開啟。  
6. **Timer Task 優先權過低**：延遲回呼卡住 → 建議設為次高（當前設定：`MAX_PRIORITIES-1`）。

---

### 9.9.5 Demo 與專案 FreeRTOS 設定差異

以下是 **Demo 與專案設定檔的對照差異**：  

#### 只在 Demo 有、專案沒有
- `INCLUDE_vTaskPrioritySet`  
- `INCLUDE_uxTaskPriorityGet`  
- `INCLUDE_vTaskDelete`  
- `INCLUDE_vTaskCleanUpResources`  
- `INCLUDE_vTaskSuspend`  
- `configUSE_APPLICATION_TASK_TAG`  
- `configGENERATE_RUN_TIME_STATS`（*Demo=0，但仍有定義*）  
- `configUSE_CO_ROUTINES`  
- `configMAX_CO_ROUTINE_PRIORITIES`  

#### 只在專案有、Demo 沒有
- `configUSE_PORT_OPTIMISED_TASK_SELECTION`  
- `configUSE_DAEMON_TASK_STARTUP_HOOK`  
- `configUSE_TASK_NOTIFICATIONS`  
- `configUSE_QUEUE_SETS`  
- `configUSE_STREAM_BUFFERS`  
- `configUSE_MESSAGE_BUFFERS`  
- `configSUPPORT_STATIC_ALLOCATION`  
- `configSUPPORT_DYNAMIC_ALLOCATION`  

#### 雙方皆有，但數值不同的巨集
- `configMAX_PRIORITIES`：Demo = 5；專案 = 7  
- `configMINIMAL_STACK_SIZE`：Demo = 130（unsigned short）；專案 = 128（uint16\_t）  
- `configTOTAL_HEAP_SIZE`：Demo = 75\*1024；專案 = 16\*1024  
- `configMAX_TASK_NAME_LEN`：Demo = 10；專案 = 16  
- `configUSE_IDLE_HOOK`：Demo = 1；專案 = 0  
- `configUSE_TICK_HOOK`：Demo = 1；專案 = 1（相同，但專案註記為暫時橋接）  
- `configUSE_TRACE_FACILITY`：Demo = 1；專案 = 0  
- `configCHECK_FOR_STACK_OVERFLOW`：Demo = 2；專案 = 0  
- `configUSE_MALLOC_FAILED_HOOK`：Demo = 1；專案 = 0  
- `configTIMER_TASK_PRIORITY`：Demo = 2；專案 = `configMAX_PRIORITIES-1`  
- `configTIMER_QUEUE_LENGTH`：Demo = 10；專案 = 8  
- `configLIBRARY_LOWEST_INTERRUPT_PRIORITY`：Demo = `0xf`；專案 = `15`（等價表示）  

> 其餘如 `configCPU_CLOCK_HZ`、`configTICK_RATE_HZ`、`configUSE_MUTEXES`、`configUSE_RECURSIVE_MUTEXES`、`configUSE_COUNTING_SEMAPHORES`、`configQUEUE_REGISTRY_SIZE`、`configPRIO_BITS`、`configKERNEL_INTERRUPT_PRIORITY`、`configMAX_SYSCALL_INTERRUPT_PRIORITY`、CMSIS handler 映射等，兩邊設定皆存在且語意一致。  

---

# 10. FreeRTOS 整合實作（空專案／無 .ioc 版）

> 目標：加檔、編譯／連結與 NVIC 設定、HAL timebase 改 TIM6、三個 handler 接管、Smoke Test 驗證，以及把現有 `while(1)` 拆成 Tasks 的落地建議。  

---

## 10.0 備份與分支

- 在 Git 開新分支：`feat/rtos-bringup`
- 保留當前可正常運作的裸機版（可隨時回退）

---

## 10.1 匯入檔案與編譯

### 10.1.1 建議目錄結構（專案樹）

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

### 10.1.2 加入 FreeRTOSConfig.h

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

並在`clock.c`定義HCLK 值：

```c
/* 把數值改成實際的 HCLK 頻率（Hz） */
uint32_t SystemCoreClock = 180000000u;
```

---

### 10.1.3 設定 Compiler Include Paths

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

## 10.2 FreeRTOS Bring-up（無 HAL）：Handler 映射、NVIC 優先權與 Smoke Test

### 10.2.1 Handler 對應（臨時橋接）

> 目的：把 **SVC/PendSV/SysTick** 導到 FreeRTOS 的處理器，同時**確保 SysTick 只給 RTOS 使用**，不與其他實作衝突。  
> 本專案未導入 HAL，因此臨時橋接時不呼叫 `HAL_IncTick()`，而是自行維護軟體計數 `uwTick`；若未來導入 HAL 或改用硬體 timebase（例如 TIM6）時，再把 Tick Hook 內改為 `HAL_IncTick()` 或關閉 Tick Hook 即可。

#### 1. 確認啟動檔的向量表名稱

確保 FreeRTOS 會用到的三個中斷入口點：`SVC_Handler` / `PendSV_Handler` / `SysTick_Handler` 已經在啟動檔（`startup_stm32f429zitx.s`）定義好。  
打開 `startup_stm32f429zitx.s`，搜尋 `Vectors`（或向量表段落），確認清單裡有 `SVC_Handler`、`PendSV_Handler`、`SysTick_Handler`。如果這三個名稱已經存在，則**無需修改**。  
若發現名稱不同（例如 `SVC_IRQHandler`、`SysTickIRQHandler`），就需在 FreeRTOS porting 時做**對應修改**（可在啟動檔改名，或改用 wrapper）。

#### 2. 映射三個 Handler 到 CMSIS 名稱
打開 `Inc/FreeRTOS/FreeRTOSConfig.h`，先搜尋檔案是否已有相同巨集，避免重複定義；若有就直接改成下列值。
```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```
> 補充：同檔案需有 `extern uint32_t SystemCoreClock;`，且專案中需**定義**此變數（在時鐘初始化的 `.c` 檔賦值為實際 HCLK），以供 `port.c` 設定 SysTick 之用。

#### 3. 開啟 Tick Hook（臨時橋接）
```c
#define configUSE_TICK_HOOK 1   /* 臨時橋接用；未來改用硬體 timebase（如 TIM6）或導入 HAL 後可關回 0 */
```

#### 4. 實作最小版 vApplicationTickHook（先可為空）
> `configUSE_TICK_HOOK` 設為 1 之後，必須在任一個 `.c` 檔提供此函式以避免連結錯誤。可先為空，下一步再填入 `uwTick` 邏輯。  
> 例如新建 `Src/FreeRTOS/hooks.c`（或任何方便的檔案）加入：
```c
#include "FreeRTOS.h"
#include "task.h"

void vApplicationTickHook(void)
{
    /* no-op for now; 下一步會加入自增 uwTick */
}
```

#### 5. 無 HAL 的系統 Tick：自管 `uwTick` 與簡易延遲
因為本專案為純裸機開發，沒有引入 STM32Cube HAL，所以無法使用 `HAL_IncTick()`，而是需要自己維護一個 `uwTick`。HAL 其實也是用這個全域計數來提供 `HAL_GetTick()`/`HAL_Delay()`。你可以手動模擬它：

**(a) 在同一個 `.c` 檔（例如 `hooks.c`）內定義 `uwTick` 與 Hook：**
```c
#include "FreeRTOS.h"
#include "task.h"

/* 模擬 HAL 的系統 tick counter */
volatile uint32_t uwTick = 0;

#if ( configUSE_TICK_HOOK == 1 )
void vApplicationTickHook(void)
{
    /* 每次 FreeRTOS SysTick 來時，自增 1ms */
    uwTick++;
}
#endif

/* 提供類似 HAL_GetTick 的功能 */
uint32_t GetSysTick(void)
{
    return uwTick;
}
```

**(b) 如需要簡易延遲，可提供一個忙等＋讓出 CPU 的版本：**
```c
void DelayMs(uint32_t delay)
{
    uint32_t start = GetSysTick();
    while ((GetSysTick() - start) < delay)
    {
        taskYIELD();  /* 建議呼叫 FreeRTOS 讓出 CPU */
    }
}
```

> 提醒：  
> 1) `uwTick` 應只在一個 `.c` 檔內定義；若其他檔案要用，請在對應的 `.h` 宣告 `extern volatile uint32_t uwTick;` 與 `uint32_t GetSysTick(void);`。  
> 2) `uwTick` 的時間基準與 `configTICK_RATE_HZ` 一致；若把 `configTICK_RATE_HZ` 設成 1000，則 `uwTick` 單位即為 ms。

---

### 10.2.2 NVIC 與 FreeRTOS 優先權設定（無 CMSIS 版）

不論有沒有 HAL，都要先把 **NVIC／優先權** 配置好，才能安全使用 FreeRTOS 的 **FromISR** 介面。

因為本專案「沒有 CMSIS 的 `stm32f4xx.h`」，這裡改用「**不需要 CMSIS**」的版本，直接用工程裡已有的工具（`io_writeMask()`、`nvic_set_priority()` 等）來完成 NVIC 設定。  
> 提醒：請確認 `FreeRTOSConfig.h` 與此處一致：`#define configPRIO_BITS 4`。若不是 4，`NVIC_PRIO()` 的位移也要同步修改（`<< (8 - configPRIO_BITS)`）。

並在 `main.c`（**時鐘就緒後**、**建立 tasks 前**、**啟用 IRQ 之前**）呼叫一次：

```c
/* 寫 AIRCR：把優先權分組設成 4 個 preempt bits（= PRIGROUP=3）
   注意：寫入 AIRCR 時一定要帶 VECTKEY=0x5FA，否則硬體會忽略 */
static void nvic_set_priority_grouping_4bits(void)
{
    const uint32_t SCB_AIRCR_ADDR   = 0xE000ED0CUL;
    const uint32_t VECTKEY_FIELD    = 0xFFFFUL << 16;
    const uint32_t PRIGROUP_FIELD   = 0x7UL    << 8;
    const uint32_t VECTKEY_WRITE    = 0x5FAUL  << 16;  // 必須帶入的 key
    const uint32_t PRIGROUP_4BITS   = 3UL      << 8;   // 等價 NVIC_PRIORITYGROUP_4

    io_writeMask(SCB_AIRCR_ADDR,
                 VECTKEY_WRITE | PRIGROUP_4BITS,
                 VECTKEY_FIELD | PRIGROUP_FIELD);
}

/* 把「人類看得懂的優先權數字 0..15」換成 NVIC 寄存器實際要寫的 8-bit 值
    MCU 有 4 個優先權位（configPRIO_BITS=4），所以左移 (8-4)=4 bits */
static inline NVIC_Priority NVIC_PRIO(uint32_t p /*0..15*/)
{
    return (NVIC_Priority)( (p & 0x0F) << 4 );
}

static void NVIC_ConfigForFreeRTOS(void)
{
    nvic_set_priority_grouping_4bits();

    /* 目前使用的 IRQ：EXTI0、EXTI15_10、TIM7
       凡是 ISR 會呼叫 FreeRTOS 的 ...FromISR()，preempt priority 要 >= 5 */
    nvic_set_priority(EXTI0_IRQn,     NVIC_PRIO(5));
    nvic_set_priority(EXTI15_10_IRQn, NVIC_PRIO(5));
    nvic_set_priority(TIM7_IRQn,      NVIC_PRIO(6));
}

int main(void)
{
    SystemInit();
    // SystemClock_Config(); // 若有做時鐘設定

    NVIC_ConfigForFreeRTOS(); // <<< 建 task 之前先呼叫

    // 建立 tasks…
    // vTaskStartScheduler();
}
```

---

### 10.2.3 Smoke Test：建立最小 Tasks 並暫停 super-loop

在 `Src/main.c` 的 `#include` 區塊底下加這些宣告（與 hooks.c 對齊）：
```c
#include "FreeRTOS.h"
#include "task.h"
extern volatile uint32_t uwTick;  // 在 hooks.c 裡有定義這個全域 tick
```

把「這三段」貼進 `main.c`：

**A. 兩個 Task**（放在 `main.c` 上方，函式區即可）
```c
static void vBlink500(void *arg)
{
    (void)arg;
    static uint8_t red = 0; // PG14
    for (;;)
    {
        red ^= 1;
        gpio_set_outdata(GPIOG_BASE, GPIO_PIN_14, red);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vBeat1000(void *arg)
{
    (void)arg;
    for (;;)
    {
        // 用軟體 tick 當心跳觀察值
        usart_printf("[beat] uwTick=%lu\r\n", (unsigned long)uwTick);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**B. 在 `main()` 初始化結束後、原本 `while(1)` 之前加入「建立 Task + 啟動排程器」**
```c
    // --- FreeRTOS smoke test ---
    xTaskCreate(vBlink500,  "Blink", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vBeat1000,  "Beat",  256, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
```

**C. 先把原本的 super-loop 暫時停用**（避免和 RTOS 併行；之後你可以把那段搬成一個 Task）
```c
#if 0
    while (1) {
        ... // 原本的主循環
    }
#endif

    for(;;){} // 一般到不了這裡；若到了通常是 heap 不足或 ISR 優先權不當
```

並在 `Inc/FreeRTOS/FreeRTOSConfig.h` 補 **API 開關**，在文件末端（或合適區塊）加入：

```c
/*-----------------------------------------------------------
 * FreeRTOS API 開關（至少要這兩個）
 *----------------------------------------------------------*/
#define INCLUDE_vTaskDelay        1
#define INCLUDE_vTaskDelayUntil   1
```

> 驗收重點：  
> 1) 專案可編譯／連結；2) 上電後不 HardFault；3) LED 以 500ms 節拍閃爍；  
> 4) `uwTick` 每秒在 log 中遞增；5) 若後續計畫在 ISR 內使用 FromISR API，請確保該 IRQ 的 preempt priority **數值 ≥ 5**。

---

## 10.3 把 EXTI 事件改成 RTOS 同步 + 移除 super-loop 依賴

### 10.3.1 將 EXTI 旗標改為 RTOS 同步（FromISR → Semaphore）

**目的**：super-loop 已停用，原本 `button_event_pending` / `touch_event_pending` 的旗標沒人輪詢。改為在 ISR 內用 `xSemaphoreGiveFromISR()` 喚醒對應任務。

#### 修改 `exti.c` 檔頭（加入 RTOS 標頭 & 外部 handle）

> **更正**：`exti.c` 需要「看得到」在 `main.c` 內建立的 semaphore，所以 `exti.c` 要保留 `extern` 聲明；而 `main.c` 的 `semButton/semTouch` 不能加 `static`（否則符號只在檔內可見）。

```c
#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t semButton;  // 由 main.c 建立（非 static）
extern SemaphoreHandle_t semTouch;   // 由 main.c 建立（非 static）
```

#### exti.c 內改 ISR：以 give 取代設旗標

```c
void EXTI0_IRQHandler(void) {
    exti_clear_pending_flag(SYSCFG_EXTI0);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semButton, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void EXTI15_10_IRQHandler(void) {
    exti_clear_pending_flag(SYSCFG_EXTI15);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semTouch, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

> 小抄：確保這兩個 EXTI 的 **NVIC preempt priority 數值 ≥ 5**（與 `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY=5` 相容），才可安全呼叫 `…FromISR()`。

---

### 10.3.2 心跳任務：改用 RTOS tick（避免依賴 `uwTick`）

現在 vBeat1000() 仍印 uwTick，但這個變數不一定有在 tick hook 裡自增，容易看起來「不動」。直接用 RTOS 的 tick 最穩。

把原本印 `uwTick` 的心跳改用 `xTaskGetTickCount()`：

```c
static void vBeat1000(void *arg)
{
    (void)arg;
    for (;;)
    {
        usart_printf("[beat] tick=%lu\r\n", (unsigned long)xTaskGetTickCount());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

> 提醒：刪掉檔頭的 `extern volatile uint32_t uwTick;`；  
> `FreeRTOSConfig.h` 需開 `#define INCLUDE_xTaskGetTickCount 1`（多數預設就有）。

---

### 10.3.3 把 super-loop 的邏輯改成任務

#### 在 `main.c` 檔頭（或全域區）宣告 semaphore

> **更正**：請宣告為**全域（非 static）**，讓 `exti.c` 的 `extern` 能連結到。

```c
SemaphoreHandle_t semButton = NULL;
SemaphoreHandle_t semTouch  = NULL;
```

#### 新增 UI 任務：固定更新 LCD、處理觸控模式 timeout

```c
/* 每 16ms 更新一次畫面 + 檢查觸控模式是否該結束 */
static void TaskUI(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();

    for (;;)
    {
        /* 原本 super-loop 的 timeout 判斷 */
        if (touch_state) {  /* 1=觸控模式, 0=旋轉模式 */
            uint32_t now = micros_now(TIMER2);
            if ((int32_t)(now - touch_until_us) >= 0) {
                touch_state = 0;  // 退出觸控模式
            }
        }

        /* 原本 super-loop 的畫面刷新 */
        lcd_update();

        vTaskDelayUntil(&last, pdMS_TO_TICKS(16)); // 約 60 FPS
    }
}
```

#### 在 `main()`（啟動 scheduler 之前）建立同步物件與任務

```c
/* --- RTOS 同步物件 --- */
semButton = xSemaphoreCreateBinary();
semTouch  = xSemaphoreCreateBinary();

/* --- 中斷事件對應的任務（已經有函式本體）--- */
xTaskCreate(TaskButton, "Button", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
xTaskCreate(TaskTouch,  "Touch",  512, NULL, tskIDLE_PRIORITY + 2, NULL);

/* --- 畫面刷新任務 --- */
xTaskCreate(TaskUI, "UI", 512, NULL, tskIDLE_PRIORITY + 1, NULL);

/* --- 心跳任務（可留作觀察系統節拍）--- */
xTaskCreate(vBeat1000, "Beat", 256, NULL, tskIDLE_PRIORITY + 1, NULL);

/* 最後啟動排程器 */
vTaskStartScheduler();
```

> 事件流程：  
> EXTI0/15 進中斷 → `xSemaphoreGiveFromISR()` → `TaskButton` / `TaskTouch` 被喚醒處理一次；  
> 螢幕持續亮與刷新 → `TaskUI` 週期性呼叫 `lcd_update()`；  
> 觸控模式 2 秒超時 → `TaskUI` 內檢查 `touch_state` 與 `touch_until_us`。

---

### 10.3.4 刪除 `vBlink500()`（避免搶同一顆 LED）

`vBlink500()` 只是早期 smoke test；現在 `TaskButton` 會快閃同一顆 LED（PG14）。移除 `vBlink500()` 及其 `xTaskCreate()` 可避免兩個任務互相覆蓋 LED 狀態。

---





































































































































































































---

# 11.

---

## 11.1 SDRAM 與 FMC 控制器簡介

STM32F429 除了內建的 SRAM、Flash 等內部記憶體外，為了擴充儲存容量與提升資料存取效率，常會透過外部介面擴接多種記憶體模組，例如 SRAM、Flash 與 SDRAM。

在 LCD 顯示應用中，由於 frame buffer 體積龐大，STM32 的內部 SRAM 通常無法容納完整畫面資料。  
為了確保畫面顯示的流暢與穩定，系統常將圖像內容暫存於 **SDRAM**，再由 **LTDC（LCD-TFT Controller）模組** 自動掃描該記憶體區塊並輸出至 LCD 螢幕。因此，MCU 必須能有效地與 SDRAM 通訊與存取。

### 11.1.1 FMC 系統架構概述

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

### 11.1.2 SDRAM 控制器功能說明

根據 37.7.1（SDRAM controller main features），STM32 所內建的 SDRAM 控制器具備下列特性：

- 支援兩組獨立組態的 SDRAM Bank  
- 支援 8-bit、16-bit、32-bit 資料匯流排寬度  
- 提供 13 位元列位址與 11 位元欄位位址，並具備 4 個內部 Bank  
- 支援多種資料存取模式：Word（32-bit）、Half-word（16-bit）、Byte（8-bit）  
- SDRAM 時脈來源可設定為 HCLK / 2 或 HCLK / 3  
- 所有時序參數皆可程式化設定  
- 內建 Auto Refresh（自動刷新）機制，且可調整刷新速率  

---

### 11.1.3 SDRAM 對外介面腳位說明

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

## 11.2 SDRAM 初始化

### 11.2.1 SDRAM 記憶體 GPIO 腳位初始化（FMC 控制器）

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

### 11.2.2 SDRAM 初始化步驟

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

### 11.2.3 設定 FMC_SDCR 與 FMC_SDTR 暫存器（Bank1）

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

### 11.2.4 依序送出五個 JEDEC 初始化指令至 SDRAM

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
