# 1. 通訊協定

- [ ] 撰寫 UART Driver：  
  - [ ] 實作 `printf` 功能，並透過其進行除錯與驗證資料傳輸的正確性  
  - [ ] 加入中斷處理機制，實現 UART 接收功能並支援 `scanf` 輸入  

- [ ] 複習 SPI / I2C 通訊協定：  
  - 瞭解兩種通訊協定的時序特性、主從模式、應用場景及驅動設計要點

---

# 2. MCU 系統架構

- [ ] 記憶體架構（SRAM / Flash）與 Linker File (`flash.ld` 與 `ram.ld`)  
  - 瞭解如何配置 MCU 的記憶體區域，並根據不同應用需求撰寫 Linker Script，能夠正確地將程式碼與資料配置至 Flash 或 RAM，達成最佳效能與系統穩定性。 
  - 瞭解 `flash.ld` 和 `ram.ld` 在記憶體配置上的用途與差異

- [ ] MCU 選型標準：評估接口數量、定時器資源、記憶體需求及 Flash 使用情境  
  - 在選擇 MCU 時，能全面考量所需硬體資源，包括 I/O 接口、定時器、SRAM / Flash 容量等，並依據功能需求做出適當選擇。  

- [ ] 使用 FreeRTOS API 並理解其內部實作原理  
  - 熟悉 FreeRTOS 的核心 API，包括任務管理、佇列、訊號量等，並理解其在嵌入式系統中的排程與執行行為。

---

# 3. 作業系統概念

## 1. Process

### 進程管理 (Process Management)

- [ ] 了解進程與執行緒的區別，進程的生命週期（創建、執行、終止）  
- [ ] 進程控制塊 (PCB) 的結構與作用  
- [ ] 進程調度：瞭解如何在多進程環境中進行進程切換，並學習不同調度演算法的工作原理  

### 進程間通訊 (IPC)

- [ ] 管道 (Pipes)、共享記憶體、訊號量 (Semaphores) 等方式的進程間通訊

## 2. Memory

### 記憶體管理 (Memory Management)

- [ ] 了解靜態記憶體與動態記憶體的分配  

#### Heap 與 Stack 的區別：

- [ ] Stack：儲存函數的局部變數與返回地址，自動分配/釋放，速度快  
- [ ] Heap：透過 `malloc` 分配與 `free` 釋放，需要手動管理，速度較慢  

- [ ] 記憶體分配演算法：  
  - [ ] 首適配 (First-fit)  
  - [ ] 最佳適配 (Best-fit)  
  - [ ] 最差適配 (Worst-fit)  

- [ ] 頁式記憶體管理與分段式記憶體管理  
- [ ] 虛擬記憶體、頁表 (Page Table)、交換 (Swapping)

## 3. Threading & CPU Scheduling

### 執行緒管理 (Thread Management)

- [ ] 了解執行緒與進程的區別  
- [ ] 執行緒的創建、終止與調度  
- [ ] 多執行緒編程：特別是在 RTOS 中進行多任務設計  

### CPU 排程 (CPU Scheduling)

- [ ] 常見排程演算法：  
  - [ ] 先到先服務 (FCFS)  
  - [ ] 最短作業優先 (SJF)  
  - [ ] 優先級排程 (Priority Scheduling)  
  - [ ] 最短剩餘時間 (Shortest Remaining Time)  
  - [ ] 時間片輪轉 (Round Robin)  

- [ ] 記憶體管理與多執行緒運行下的 Stack / Heap 使用分析  

## 4. Synchronization & Deadlock

### 同步 (Synchronization)

- [ ] 理解同步的必要性與原則  
- [ ] 鎖：互斥鎖 (Mutex)、自旋鎖 (Spinlock)  
- [ ] 訊號量 (Semaphore)、條件變數 (Condition Variables)  

### 死鎖 (Deadlock)

- [ ] 死鎖的四大必要條件：互斥、占有且等待、不釋放、循環等待  
- [ ] 死鎖避免策略：銀行家演算法、資源分配圖  

## 5. File System & I/O System

### 檔案系統 (File System)

- [ ] 檔案儲存、檔案系統結構、基本檔案操作  
- [ ] 目錄結構：層級結構組織  
- [ ] 磁碟配置：磁碟區、FAT、iNode 結構  

### I/O 系統 (I/O System)

- [ ] I/O 管理與設備驅動程式工作原理  
- [ ] I/O 排程機制與效率分析  

## 6. RTOS

[FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel)

### RTOS 任務排程 (Task Scheduling)

- [ ] RTOS 在嵌入式系統中的應用與基本概念  
- [ ] 任務創建、切換與優先級設定  
- [ ] 了解 FreeRTOS 排程演算法  

### 訊號量 / 佇列 機制

- [ ] FreeRTOS 中的訊號量與佇列使用  
- [ ] 多任務協作實作方式（含事件組等）

### RTOS 記憶體管理

- [ ] 靜態 / 動態記憶體分配  
- [ ] 記憶體池與碎片處理、堆疊管理等實務策略  

---

# 4. 演算法與硬體基礎補充

- [ ] 資料結構：陣列、鏈結串列、堆疊、佇列，並在嵌入式環境中應用  
- [ ] 演算法：合併排序、插入排序、冒泡排序、快速排序，二分搜尋  
- [ ] 數學基礎與位元運算技巧：  
  - 常用運算符：`^`, `<<`, `>>`, `|`, `&`, `!`, `~`

---

# 5. 計算機組織 (Computer Organization)

## Ch1：Computer Abstractions and Technology

## Ch2：Instructions Language of the Computer  
- Instruction Set  

## Ch3：Arithmetic for Computers

## Ch4：The Processor  
- Pipeline  
- Hazard  

## Ch5：Large and Fast Exploiting Memory Hierarchy  
- Cache  
- Memory  
- Virtual Memory  

## Ch6：Parallel Processors from client to cloud


---

# 6. 專案

## 1. 模擬與除錯操作：熟悉 STM32 除錯工具與流程

- [ ] 熟練使用 STM32CubeIDE 等開發環境  
  - 包含設置中斷點、觀察變數與記憶體內容、查看暫存器狀態、進行單步執行等基本除錯技巧  
  - 理解除錯工具在嵌入式開發流程中的角色與使用時機

## 2. MCU 實作專案：Driver 控制 + 中斷機制 + FreeRTOS 整合

🔹 專案主題：STM32F429 + 觸控 LCD + FreeRTOS

### 一、設計目的（Motivation）

- **技術動機一：**  
  延續自我介紹中提到的體動式設備開發經驗，為了強化底層能力，開始自學 UART driver 程式設計，並進一步延伸至 LCD 與 Touch Panel driver。

- **技術動機二：**  
  為了理解 RTOS 在實務中的應用價值，自行學習 OS 相關課程並嘗試實作，期望透過此專案展示 FreeRTOS 的實際應用場景與效益。

### 二、開發環境與初始功能（Environment & Initial Feature）

- **開發平台：** STM32F429 開發板 + 觸控式 LCD  
- **功能設計：**
  - 顯示三個主頁面：A / B / C  
  - 螢幕上方設置「上一頁」與「下一頁」兩個觸控按鈕  
  - 使用者可點擊觸控區切換顯示頁面

### 三、系統架構與實作細節（System Architecture）

#### 1. 硬體初始化（Hardware Initialization）

- [ ] 設定系統時脈（Clock）與 I/O 腳位，確保 MCU 核心與外部裝置能正常運作  
- [ ] 配置 Timer 週期中斷，用於定時觸發事件，如觸控掃描與畫面更新

#### 2. LCD 顯示驅動（LTDC + Framebuffer）

- [ ] **LTDC 設定：**  
  - 使用 STM32F429 內建的 LTDC（LCD-TFT Display Controller）負責管理顯示輸出  
  - 撰寫初始化函式以設定螢幕解析度、同步時序與控制暫存器，確保顯示器可正常刷新

- [ ] **DMA2D 與 Framebuffer 操作：**  
  - 使用 DMA2D（影像加速器）處理圖像搬移與色彩轉換，加快畫面更新速度  
  - Framebuffer 資料預先配置於 SRAM，LTDC 持續從該記憶體位址讀取顯示內容  
  - 後續可擴充為雙 framebuffer 設計（double buffering），避免畫面撕裂（tearing）

#### 3. 畫面繪製函式設計（Screen Drawing Functions）

- [ ] 為每個主畫面撰寫獨立顯示函式：  
  - `draw_page_A()`、`draw_page_B()`、`draw_page_C()`，每個函式負責渲染對應頁面內容與版面配置  
- [ ] 函式內部調用 DMA2D 將圖像資料寫入 framebuffer，並由 LTDC 同步輸出至螢幕  
- [ ] 畫面佈局與元件位置（如文字、圖示）皆以座標控制，確保各頁面風格一致

#### 4. 觸控按鈕邏輯（Touch Button Logic）

- [ ] 定義螢幕區域，區分「上一頁」與「下一頁」兩個觸控按鈕區域  
- [ ] 實作 debounce 機制以防止觸控重複觸發，提高使用體驗與系統穩定性  
- 本專案使用 **polling 掃描方式** 搭配 **50ms 的 Timer 中斷週期性觸發**觸控掃描，簡化中斷邏輯，避免系統資源被過度占用

> **補充：Timer 在中斷架構與 RTOS 架構的角色比較**  
> 
> - **中斷架構（Non-RTOS）：**  
>   - Timer 必定與中斷結合使用，由中斷定期觸發事件（如觸控掃描）  
>   - 程式流程依賴旗標判斷與中斷主導，主迴圈僅負責判斷與切換狀態  
>   - 無任務排程，所有邏輯由中斷分派  
> 
> - **RTOS 架構：**  
>   - 任務執行可採用兩種方式：  
>     1. **主動輪詢（Polling）**：利用 `vTaskDelay()` 控制任務週期性執行  
>     2. **被動中斷（Interrupt-Driven）**：由 Timer 或外部中斷觸發，再喚醒特定任務處理事件  
>   - 流程主導者為 **RTOS scheduler**，中斷僅負責事件通知與喚醒任務  

- [ ] 未實作長按或滑動事件，後續可擴充進階觸控互動功能

#### 5. 主程式流程（Initial Version）

- [ ] 使用 FreeRTOS 建立兩個主要任務（Task）以實作多任務架構：  
  - **`TouchTask`**：定期掃描觸控事件，偵測按鈕操作並傳遞事件給主控制任務  
  - **`DisplayTask`**：接收觸控事件後切換對應頁面並執行畫面更新  
- [ ] 任務間透過 **Queue 或 Binary Semaphore** 溝通觸控事件，確保資料傳遞同步且非阻塞  
- [ ] 利用 Timer 中斷週期性（如每 50ms）觸發掃描觸控區域與更新畫面狀態，確保即時互動反應與畫面刷新頻率

### 四、為何將 RTOS 實作於專案（Why RTOS?）

#### 1. 設計上的疑問

- 若不使用 RTOS，單靠 `while(1)` 搭配 Timer 或 EXTI 中斷，也能實現畫面切換與觸控互動。
- 但當系統邏輯變複雜，是否仍適合持續使用非 RTOS 架構？

#### 2. 非 RTOS 架構下的實作方式

- [ ] 採用 `while(1)` 主迴圈搭配 **定時器中斷**（Timer Interrupt）作為事件驅動的核心流程：

  - **主迴圈**：持續執行，監聽觸控事件旗標（flag），並根據狀態更新畫面  
  - **Timer 中斷**：每 50ms 觸發一次，掃描觸控輸入並設定觸控事件旗標  

- [ ] 流程說明如下：

  - `main()`：初始化系統、LCD、觸控模組，進入 `while(1)`  
  - Timer 中斷處理函式：  
    - 每 50ms 掃描觸控事件  
    - 若偵測到觸控動作，設定 `touch_event_flag = 1`  
  - 主迴圈：  
    - 檢查 `touch_event_flag`  
    - 若為 1，則依觸控位置切換頁面並執行對應的 `draw_page_*()`  
    - 處理完畢後重設 flag 為 0，等待下次中斷

- [ ] 此架構雖無任務排程與同步控制，但對簡單互動流程已可滿足需求，具備**低開銷、易開發**等優點，可作為 RTOS 引入前的原型階段。

#### 3. 我的實作觀點與思考

1. **掌握中斷與非阻塞邏輯（Non-blocking flow）：**  
   即使不使用 RTOS，也能透過事件旗標與定時器妥善管理流程，維持程式邏輯簡潔清楚。

2. **RTOS 的價值與使用時機：**   
   RTOS 並非萬靈丹，但當系統邏輯變得**複雜、模組眾多、任務需並行協作**時，導入 RTOS 是具結構性與可維護性的選擇。

   - **優點：**  
     - 多任務併行，結構清楚  
     - 提升程式可維護性與擴充性  
     - 模組間可清晰分工與同步協作  

   - **潛在開銷：**  
     1. **任務切換（Context Switch）**：頻繁切換會耗費 CPU 時間  
     2. **排程負擔（Scheduler Overhead）**：即使系統負載低，也需持續執行排程器  
     3. **堆疊與記憶體管理**：每個任務需獨立 stack，易造成 RAM 使用緊張

3. **總結觀點：**  
   RTOS 並非所有應用都需導入，但在需要**任務擴展、模組獨立、協同執行**的情境下，導入 RTOS 架構能有效提升系統彈性與長期可維護性，對專案發展具正面效益。

### 五、進階功能：加入待機畫面（New Feature + RTOS Justification）

#### 1. 新增功能目的：體現 RTOS 優勢

- 增加 **自動待機頁面（Page D）**：
  - 當系統 **3 秒內無觸控操作**，自動切換至待機畫面
  - 一旦使用者觸控，**立即返回原先頁面**

- 此功能需長時間監控使用者互動狀態與時間邏輯，適合由 RTOS 任務模組化管理

#### 2. 任務設計（FreeRTOS）

> 下表為三個任務的功能、優先權與對應設計方式：

| 任務名稱   | 功能             | 優先權 | 實作方式                     |
|------------|------------------|--------|------------------------------|
| 觸控掃描   | 偵測觸控事件     | 高     | 高優先權 Task 或 Interrupt   |
| 畫面更新   | 切換與重繪畫面   | 中     | 一般優先權 Task              |
| 待機監控   | 計時無操作切畫面 | 低     | `vTaskDelay()` 或定時任務    |

- 觸控任務 ➝ 偵測並發送觸控事件  
- 畫面任務 ➝ 根據狀態變數切換顯示畫面  
- 待機任務 ➝ 每 1 秒執行一次，檢查是否超過 3 秒未操作

#### 3. 為何此功能能體現 RTOS 架構優勢

1. **任務分工清楚，程式邏輯易讀好維護**  
   - RTOS 架構下，各功能分由不同任務負責，互不干擾  
   - 相比傳統 `while(1)` + 中斷，邏輯不會交錯混亂

2. **即時反應，避免操作延遲**  
   - 透過任務優先權設定，確保觸控任務具有即時性  
   - 不會被畫面刷新、待機邏輯阻塞或干擾

3. **高度擴充性，方便新增模組**  
   - 若需擴充 WiFi、圖片下載、UART log，只需新增任務  
   - 不影響原有流程，提高可維護性與模組獨立性

### 六、收尾與總結（Conclusion）

#### 1. 比較中斷與 RTOS 方法寫多任務處理（Multiprocessing）的優缺點

- **中斷架構：**

  - **優點：**
    - 實現簡單，資源開銷低。
    - 適合簡單的實時任務處理，能夠快速響應硬體事件。

  - **缺點：**
    - 程式邏輯較為混亂，易於產生錯誤，維護困難。
    - 較難擴展多任務協作，當邏輯複雜時容易出現衝突或難以管理的狀況。
    - 無法有效處理複雜的同步與排程問題，可能導致任務間的相互影響或延遲。

- **RTOS 架構：**

  - **優點：**
    - 支援多任務並行處理，能夠清晰地分工與排程，保持程式結構簡潔。
    - 透過優先權機制與排程器，能夠有效處理多任務的協作，避免資源競爭。
    - 易於擴展，若未來需要新增功能，無需重構現有程式，只需添加新的任務。

  - **缺點：**
    - 需要較多的資源，如堆疊與內存，且任務切換會帶來額外的開銷。
    - 比較適合中到高複雜度的應用，對於簡單系統來說，可能會引入不必要的開銷。

#### 2. 中斷與 RTOS 的使用時機

- **中斷使用時機：**
  - 系統任務少且邏輯簡單。
  - 對即時性要求極高（如單一事件的偵測與反應）。
  - 目標為節省資源，且無需擴展性與模組彈性。

- **RTOS 使用時機：**
  - 任務之間邏輯獨立但需協作（例如：觸控偵測 + 顯示 + 傳輸）。
  - 專案規模中大型，需長期維護或多次功能擴充。
  - 預期模組會增加，需良好架構支持（如 UART log、網路模組、背景任務）。

#### 最後結論

本專案從最初的單頁顯示與觸控切換，逐步擴展至具備待機邏輯與多任務分工的架構，最終導入 RTOS 管理整體流程。透過此次實作，我學會：

- 如何區分與選擇適合的控制架構（中斷 vs. RTOS）
- 如何以 FreeRTOS 建構模組化、可維護的任務流程
- 如何評估系統負擔與效能平衡，選擇適當的工具與設計

---

# 7. 資料結構與演算法

## **1. Array**：適合按索引快速存取。
**學習重點**：先熟悉基本資料結構操作，掌握陣列的增刪改查技巧，並熟練前綴和、滑動視窗、單調堆疊等進階技巧。  

### **基礎陣列操作**
- [ ] **Easy** [Two Sum](https://leetcode.com/problems/two-sum/) (#1) - **經典 Hash Table + Array 應用**
- [ ] **Easy** [Remove Duplicates from Sorted Array](https://leetcode.com/problems/remove-duplicates-from-sorted-array/) (#26) - **雙指標操作**
- [ ] **Easy** [Contains Duplicate](https://leetcode.com/problems/contains-duplicate/) (#217) - **基本查找**
- [ ] **Easy** [Intersection of Two Arrays II](https://leetcode.com/problems/intersection-of-two-arrays-ii/) (#350) - **Array + Hash Table**
- [ ] **Easy** [Move Zeroes](https://leetcode.com/problems/move-zeroes/) (#283) - **雙指標**
- [ ] **Medium** [Rotate Array](https://leetcode.com/problems/rotate-array/) (#189) - **環狀替換法**

### **最佳化計算**
- [ ] **Easy** [Best Time to Buy and Sell Stock](https://leetcode.com/problems/best-time-to-buy-and-sell-stock/) (#121) - **貪婪演算法**
- [ ] **Medium** [Maximum Product Subarray](https://leetcode.com/problems/maximum-product-subarray/) (#152) - **DP**
- [ ] **Medium** [Find Minimum in Rotated Sorted Array](https://leetcode.com/problems/find-minimum-in-rotated-sorted-array/) (#153) - **二分搜尋**
- [ ] **Medium** [Search in Rotated Sorted Array](https://leetcode.com/problems/search-in-rotated-sorted-array/) (#33) - **二分搜尋變形**
- [ ] **Hard** [Subarray Sum Equals K](https://leetcode.com/problems/subarray-sum-equals-k/) (#560) - **前綴和 + Hash Table**
- [ ] **Medium** [Product of Array Except Self](https://leetcode.com/problems/product-of-array-except-self/) (#238) - **前綴乘積**

### **滑動視窗**
- [ ] **Medium** [Longest Substring Without Repeating Characters](https://leetcode.com/problems/longest-substring-without-repeating-characters/) (#3) - **滑動視窗**
- [ ] **Hard** [Sliding Window Maximum](https://leetcode.com/problems/sliding-window-maximum/) (#239) - **單調佇列**
- [ ] **Medium** [Minimum Size Subarray Sum](https://leetcode.com/problems/minimum-size-subarray-sum/) (#209) - **雙指標**

### **位元運算（Bit Manipulation）**
- [ ] **Easy** [Single Number](https://leetcode.com/problems/single-number/) (#136) - **XOR 位元運算**
- [ ] **Easy** [Majority Element](https://leetcode.com/problems/majority-element/) (#169) - **Boyer-Moore 投票法**
- [ ] **Easy** [Missing Number](https://leetcode.com/problems/missing-number/) (#268) - **數學 XOR**
- [ ] **Medium** [Reverse Bits](https://leetcode.com/problems/reverse-bits/) (#190) - **位元翻轉**
- [ ] **Easy** [Number of 1 Bits](https://leetcode.com/problems/number-of-1-bits/) (#191) - **位元計數**
- [ ] **Medium** [Counting Bits](https://leetcode.com/problems/counting-bits/) (#338) - **DP + Bitwise**

### **矩陣（Matrix）**
- [ ] **Medium** [Spiral Matrix](https://leetcode.com/problems/spiral-matrix/) (#54) - **矩陣遍歷**
- [ ] **Medium** [Set Matrix Zeroes](https://leetcode.com/problems/set-matrix-zeroes/) (#73) - **矩陣標記法**
- [ ] **Hard** [Game of Life](https://leetcode.com/problems/game-of-life/) (#289) - **狀態壓縮**
- [ ] **Medium** [Rotate Image](https://leetcode.com/problems/rotate-image/) (#48) - **矩陣旋轉**

---

## **2. Hash Table**：用於快速查找鍵值對 (key, value)。
**學習重點**：理解哈希表的基本操作，如何有效地解決查詢和衝突問題，並熟悉進階應用，如雙雜湊、位元運算和 LRU Cache。  

### **基礎哈希操作**
- [ ] **Easy** [Valid Anagram](https://leetcode.com/problems/valid-anagram/) (#242) - **字母計數**
- [ ] **Easy** [Intersection of Two Arrays II](https://leetcode.com/problems/intersection-of-two-arrays-ii/) (#350) - **Hash Table + 多集合查找**
- [ ] **Easy** [Contains Duplicate](https://leetcode.com/problems/contains-duplicate/) (#217) - **簡單查找**
- [ ] **Easy** [Contains Duplicate II](https://leetcode.com/problems/contains-duplicate-ii/) (#219) - **滑動視窗 + Hash Map**
- [ ] **Medium** [Group Anagrams](https://leetcode.com/problems/group-anagrams/) (#49) - **字串分組**
- [ ] **Easy** [Isomorphic Strings](https://leetcode.com/problems/isomorphic-strings/) (#205) - **雙向映射**
- [ ] **Easy** [Word Pattern](https://leetcode.com/problems/word-pattern/) (#290) - **模式匹配**

### **進階哈希技術**
- [ ] **Easy** [Two Sum](https://leetcode.com/problems/two-sum/) (#1) - **Array + Hash Table 經典應用**
- [ ] **Medium** [Four Sum II](https://leetcode.com/problems/4sum-ii/) (#454) - **雙雜湊 Hashing**
- [ ] **Medium** [Subarray Sum Equals K](https://leetcode.com/problems/subarray-sum-equals-k/) (#560) - **前綴和 + Hash Table**
- [ ] **Medium** [Longest Consecutive Sequence](https://leetcode.com/problems/longest-consecutive-sequence/) (#128) - **Hash Set 優化查找**
- [ ] **Medium** [Top K Frequent Elements](https://leetcode.com/problems/top-k-frequent-elements/) (#347) - **Heap + Hash Map**
- [ ] **Easy** [Happy Number](https://leetcode.com/problems/happy-number/) (#202) - **數字循環 + Hash Set**

### **設計類**
- [ ] **Medium** [LRU Cache](https://leetcode.com/problems/lru-cache/) (#146) - **雙向鏈結串列 + Hash Map**
- [ ] **Medium** [Design HashMap](https://leetcode.com/problems/design-hashmap/) (#706) - **模擬 Hash Table**
- [ ] **Hard** [Design Twitter](https://leetcode.com/problems/design-twitter/) (#355) - **優先佇列 + Hash Map**

---

## **3. Linked List**：方便進行插入和刪除操作。
**學習重點**：掌握常見指針操作，熟悉鏈結串列中新增、刪除和查找節點的基本操作，並熟練掌握遞迴與雙向鏈結串列的運用。  

### **基礎操作**
- [ ] **Easy** [Reverse Linked List](https://leetcode.com/problems/reverse-linked-list/) (#206) - **反轉鏈結串列**
- [ ] **Easy** [Merge Two Sorted Lists](https://leetcode.com/problems/merge-two-sorted-lists/) (#21) - **合併兩個排序鏈結串列**
- [ ] **Easy** [Linked List Cycle](https://leetcode.com/problems/linked-list-cycle/) (#141) - **檢查環狀鏈結串列**
- [ ] **Medium** [Remove Nth Node From End of List](https://leetcode.com/problems/remove-nth-node-from-end-of-list/) (#19) - **刪除倒數第N個節點**
- [ ] **Medium** [Reorder List](https://leetcode.com/problems/reorder-list/) (#143) - **重排鏈結串列**

### **進階操作**
- [ ] **Medium** [Add Two Numbers](https://leetcode.com/problems/add-two-numbers/) (#2) - **兩個數字加法**
- [ ] **Medium** [Flatten a Multilevel Doubly Linked List](https://leetcode.com/problems/flatten-a-multilevel-doubly-linked-list/) (#430) - **多層雙向鏈結串列扁平化**
- [ ] **Medium** [Copy List with Random Pointer](https://leetcode.com/problems/copy-list-with-random-pointer/) (#138) - **隨機指針複製**
- [ ] **Medium** [Linked List Cycle II](https://leetcode.com/problems/linked-list-cycle-ii/) (#142) - **找出環狀鏈結串列的入口**
- [ ] **Easy** [Intersection of Two Linked Lists](https://leetcode.com/problems/intersection-of-two-linked-lists/) (#160) - **找出兩個鏈結串列的交點**

### **進階設計問題**
- [ ] **Medium** [Design Linked List](https://leetcode.com/problems/design-linked-list/) (#707) - **設計鏈結串列** 

---

## **4. Stack**：處理特定的操作順序。
**學習重點**：掌握堆疊操作，理解堆疊在特定問題中的應用，特別是括號配對和最小元素追蹤。

### **基礎操作**
- [ ] **Easy** [Valid Parentheses](https://leetcode.com/problems/valid-parentheses/) (#20) - **括號配對問題**
- [ ] **Easy** [Min Stack](https://leetcode.com/problems/min-stack/) (#155) - **最小堆疊問題**

### **進階應用**
- [ ] **Medium** [Daily Temperature](https://leetcode.com/problems/daily-temperatures/) (#739) - **日溫問題**：使用堆疊解決每個日子後的較高溫度
- [ ] **Easy** [Next Greater Element I](https://leetcode.com/problems/next-greater-element-i/) (#496) - **下一個更大的元素問題**
- [ ] **Hard** [Largest Rectangle in Histogram](https://leetcode.com/problems/largest-rectangle-in-histogram/) (#84) - **直方圖中的最大矩形面積**
- [ ] **Medium** [Evaluate Reverse Polish Notation](https://leetcode.com/problems/evaluate-reverse-polish-notation/) (#150) - **逆波蘭表示法求值**
- [ ] **Medium** [Valid Parenthesis String](https://leetcode.com/problems/valid-parenthesis-string/) (#678) - **有效的括號字串問題**

### **進階設計問題**
- [ ] **Easy** [Implement Stack using Queues](https://leetcode.com/problems/implement-stack-using-queues/) (#225) - **使用佇列實現堆疊**
- [ ] **Medium** [Design Browser History](https://leetcode.com/problems/design-browser-history/) (#1472) - **設計瀏覽器歷史紀錄**：使用堆疊來記錄網頁歷史

---

## **5. Tree**：應用於層次結構的資料管理。  
**學習重點**：挑戰遞迴與基礎遍歷，理解樹結構的遍歷方法（前序、中序、後序）。  

### **基本操作**
- [ ] **Easy** [Maximum Depth of Binary Tree](https://leetcode.com/problems/maximum-depth-of-binary-tree/) (#104) - **二叉樹最大深度**
- [ ] **Easy** [Symmetric Tree](https://leetcode.com/problems/symmetric-tree/) (#101) - **對稱樹**
- [ ] **Easy** [Invert Binary Tree](https://leetcode.com/problems/invert-binary-tree/) (#226) - **翻轉二叉樹**

### **進階問題**
- [ ] **Medium** [Binary Tree Level Order Traversal](https://leetcode.com/problems/binary-tree-level-order-traversal/) (#102) - **二叉樹層次遍歷**
- [ ] **Medium** [Convert Sorted Array to Binary Search Tree](https://leetcode.com/problems/convert-sorted-array-to-binary-search-tree/) (#108) - **將排序數組轉換為二叉搜尋樹**
- [ ] **Easy** [Path Sum](https://leetcode.com/problems/path-sum/) (#112) - **路徑總和問題**
- [ ] **Medium** [Binary Tree Zigzag Level Order Traversal](https://leetcode.com/problems/binary-tree-zigzag-level-order-traversal/) (#103) - **二叉樹之字形層次遍歷**

### **進階設計問題**
- [ ] **Hard** [Serialize and Deserialize Binary Tree](https://leetcode.com/problems/serialize-and-deserialize-binary-tree/) (#297) - **二叉樹的序列化與反序列化**

---

## **6. Queue**：處理特定的操作順序。  
**學習重點**：理解佇列的運作方式，學會用堆疊實現佇列等常見操作。  

### **基本操作**
- [ ] **Easy** [Implement Queue using Stacks](https://leetcode.com/problems/implement-queue-using-stacks/) (#232) - **用堆疊實現佇列**

### **進階應用**
- [ ] **Medium** [My Circular Queue](https://leetcode.com/problems/design-circular-queue/) (#622) - **圓形佇列設計問題**
- [ ] **Medium** [Design Hit Counter](https://leetcode.com/problems/design-hit-counter/) (#362) - **計數器設計**：使用佇列進行歷史記錄
- [ ] **Hard** [Sliding Window Maximum](https://leetcode.com/problems/sliding-window-maximum/) (#239) - **滑動視窗的最大值**：佇列應用於滑動視窗問題

---

## **7. String**：掌握字串處理技巧，了解字串比對和處理的常用方法。  
**學習重點**：掌握字串處理技巧，了解字串比對和處理的常用方法。  

- [ ] **Easy** [Longest Common Prefix](https://leetcode.com/problems/longest-common-prefix/) (#14) - **最長公共前綴**
- [ ] **Easy** [Valid Palindrome](https://leetcode.com/problems/valid-palindrome/) (#125) - **有效的回文**
- [ ] **Easy** [Reverse String](https://leetcode.com/problems/reverse-string/) (#344) - **反轉字串**
- [ ] **Medium** [String to Integer (atoi)](https://leetcode.com/problems/string-to-integer-atoi/) (#8) - **字串轉整數 (atoi)**
- [ ] **Medium** [Count and Say](https://leetcode.com/problems/count-and-say/) (#38) - **數字與字串的轉換問題**
- [ ] **Medium** [Longest Substring Without Repeating Characters](https://leetcode.com/problems/longest-substring-without-repeating-characters/) (#3) - **無重複字元的最長字串**

---

## **8. Sort**：了解排序演算法的基本概念，練習排序操作的效率和應用。  
**學習重點**：了解排序演算法的基本概念，練習排序操作的效率和應用。  

- [ ] **Easy** [Merge Sorted Array](https://leetcode.com/problems/merge-sorted-array/) (#88) - **合併排序陣列**
- [ ] **Easy** [Sort Colors](https://leetcode.com/problems/sort-colors/) (#75) - **排序顏色**
- [ ] **Medium** [Kth Largest Element in an Array](https://leetcode.com/problems/kth-largest-element-in-an-array/) (#215) - **陣列中的第K大元素**
- [ ] **Medium** [Find the Duplicate Number](https://leetcode.com/problems/find-the-duplicate-number/) (#287) - **尋找重複數字**
- [ ] **Medium** [Top K Frequent Elements](https://leetcode.com/problems/top-k-frequent-elements/) (#347) - **數組中出現頻率前K的元素**

---

## **9. Binary Search**：理解二分搜尋的基本原理，熟練掌握在有序數列中的查找技巧。  
**學習重點**：理解二分搜尋的基本原理，熟練掌握在有序數列中的查找技巧。  

- [ ] **Easy** [Binary Search](https://leetcode.com/problems/binary-search/) (#704) - **二分搜尋**
- [ ] **Medium** [First Bad Version](https://leetcode.com/problems/first-bad-version/) (#278) - **第一個壞版本**
- [ ] **Easy** [Search Insert Position](https://leetcode.com/problems/search-insert-position/) (#35) - **搜尋插入位置**
- [ ] **Medium** [Find Minimum in Rotated Sorted Array](https://leetcode.com/problems/find-minimum-in-rotated-sorted-array/) (#153) - **尋找旋轉排序數列中的最小值**
- [ ] **Medium** [Search in Rotated Sorted Array](https://leetcode.com/problems/search-in-rotated-sorted-array/) (#33) - **搜尋旋轉排序數列**