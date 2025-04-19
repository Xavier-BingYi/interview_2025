# Chap 0：Operating System

> **主題：系統架構與使用者導向的電腦系統分類**

---

## 一、從系統架構的角度

### 1. Mainframe 系統

#### ▸ Batch（批次處理）
- 一次處理一個 Job，無互動。
- CPU 常 idle，效能低。

#### ▸ Multi-programming（多重程式設計）
- 多個程式佔用主記憶體，減少閒置時間。
- Spooling 技術提高 CPU 與 I/O 利用率。

#### ▸ Time-sharing（分時系統）
- 多使用者互動，CPU 快速切換任務。
- 回應時間 < 1 秒，支援鍵盤與螢幕。

---

### 2. Desktop Systems

- 單一使用者，操作便利，支援 GUI。
- 常見 OS：Windows, macOS, Linux。
- 較缺乏資源保護，易受病毒攻擊。

---

### 3. Parallel Systems（平行處理）

| 類型 | 特性 |
|------|------|
| **SMP（對稱）** | 每個處理器執行相同作業系統，需同步保護資料一致性。 |
| **AMP（非對稱）** | Master 分派任務給 slave，更常見於大型系統。 |

**優點：**
- 提升 Throughput。
- 降低成本（共用記憶體、裝置）。
- 提高可靠性。

---

### 4. Multi-core 與 Many-core

- **Multi-core**：2~8 核心，常見於 PC、手機。
- **Many-core**：10 核以上，用於高效能運算（如 GPU）。

---

### 5. 記憶體架構（Memory Access）

| 架構 | UMA | NUMA |
|------|-----|------|
| 記憶體存取時間 | 相同 | 不同 |
| 結構 | 所有 CPU 透過同樣方式接到主記憶體 | 系統分成多個 nodes，每個 node 有 local memory |
| 特性 | 易於管理、設計簡單 | 效能佳，需考慮記憶體區域性 |

---

## 二、使用者導向的系統分類

### 1. Distributed Systems（分散式系統）

#### ▸ 架構形式
- **Client-Server**：集中管理易建構，但 server 易成為 bottleneck。
- **Peer-to-Peer**：無主從，所有節點地位平等；可靠性高、擴展性好。

#### ▸ 特性
- Loosely coupled，透過網路通訊。
- 每台電腦擁有自己的 local memory，不共享記憶體。
- 支援 Resource Sharing、Load Balancing、Fault Tolerance。

---

### 2. Clustered Systems（叢集系統）

- 多台電腦透過高速 LAN（如 Infiniband）連接。
- 通常部署於同一區域內，具高效能與低延遲。

**分類：**
- **Asymmetric**：主機執行，備援機監控。
- **Symmetric**：多台同時執行並互相監控。

---

## 三、特殊目的系統

### 1. Real-Time Systems（即時系統）

| 分類 | 特性 | 範例 |
|------|------|------|
| **Hard** | 必須在 deadline 前完成，否則系統失敗 | 飛彈系統、車控 |
| **Soft** | 儘量準時完成，過時影響不大 | 串流影音、多媒體 |

- Scheduler 是系統設計的關鍵。
- 常用演算法：**EDF（Earliest Deadline First）**。
- 硬即時系統常無硬碟，只依賴主記憶體（避免不穩定延遲）。

---

### 2. Multimedia Systems（多媒體系統）

- 涉及音訊與視訊（如串流、直播）。
- 有時間同步需求（如 30 fps）。
- 多使用壓縮技術、On-demand 傳輸。

---

### 3. Handheld / Embedded Systems（手持／嵌入式系統）

- **裝置**：手機、PDA、嵌入式控制器。
- **限制**：記憶體小、處理器慢、電池壽命短、螢幕小。
- 通常使用**專用作業系統**。

---

## 系統對照總整理

| 系統類型 | 特性 | 架構類型 |
|----------|------|-----------|
| **Mainframe** | 批次、多工、共用主機 | Centralized |
| **Desktop** | 單人使用、互動方便 | 單機 |
| **Parallel** | 多核心、共享記憶體 | Tightly Coupled |
| **Distributed** | 獨立節點、透過網路溝通 | Loosely Coupled |
| **Clustered** | 多機同地連線、I/O 快 | LAN 架構 |
| **Real-Time** | 準時性導向、可靠性要求高 | 通常為嵌入式 |
| **Multimedia** | 視訊同步、反應速度 | 多為 Soft Real-Time |
| **Handheld** | 輕量化、低耗能 | 嵌入式／專用 OS |

---

# Chap 1：Introduction

> **主題：作業系統的基本概念與硬體保護機制**

---

## 一、作業系統的定義與目的

### 1. 作業系統是什麼？

- 作業系統是**常駐軟體**，用來控制與抽象硬體資源，供使用者應用程式使用。
- 提供一個「虛擬機器介面」，隱藏實體硬體細節。

```
Physical Machine ← OS → Virtual Machine
```

- 提供 Application Programming Interface (API)，使應用程式與硬體隔離。

---

### 2. 作業系統的三種角色

- **Resource Allocator**：資源分配器，確保效率與公平性。
- **Control Program**：控制使用者程式與 I/O 運作，防止誤用與錯誤。
- **Kernel**：常駐於系統中，負責最基本的作業系統功能。

---

### 3. 作業系統的目標

- **方便性（Convenience）**：讓系統好用（個人電腦需求）。
- **效率（Efficiency）**：有效使用硬體（共享系統需求）。

---

### 4. 作業系統的重要性

- 系統 API 是應用程式與硬體之間的唯一橋梁。
- 作業系統錯誤可能導致整台電腦重啟。
- 掌握 OS 技術＝掌握軟體與硬體產業主導權。
- 作業系統設計與電腦架構互相影響。

---

### 5. 現代常見作業系統

| 平台       | 作業系統                         |
|------------|----------------------------------|
| x86        | Linux（CentOS、Ubuntu）、Windows |
| PowerPC    | macOS                            |
| Mobile     | Android、iOS、Ubuntu Touch       |
| Embedded   | Embedded Linux、Windows CE       |

---

## 二、電腦系統結構與運作

### 1. 組成元件（四層）

1. 使用者（User）
2. 應用程式（Application Programs）
3. 作業系統（Operating System）
4. 硬體（Hardware）

---

### 2. 硬體架構與裝置控制

- 多個 CPU 與裝置控制器（Device Controllers）共用一條 Bus 與主記憶體。
- CPU 與裝置可同時執行，爭用記憶體存取（Memory Cycles）。
- I/O 流程：
  - 裝置 → 控制器 Local Buffer → CPU → 記憶體。

---

### 3. Busy-Wait I/O（繁忙等待）

- 最簡單的裝置操作方式。
- 缺點：CPU 不能處理其他工作，效率低。

---

### 4. Interrupt I/O（中斷驅動 I/O）

- 裝置完成操作時觸發中斷，通知 CPU。
- 中斷處理流程：
  1. 裝置驅動程式啟動 I/O
  2. 裝置完成後產生中斷
  3. CPU 呼叫中斷處理程式（Interrupt Handler）
  4. 處理完後回到原工作

- 現代作業系統皆為中斷驅動架構。

---

### 5. Trap vs Interrupt

| 類型   | 來源     | 範例                     |
|--------|----------|--------------------------|
| Interrupt | 硬體 | I/O 完成、Timer 到期等       |
| Trap      | 軟體 | 除以 0、系統呼叫、錯誤處理等 |

---

## 三、儲存階層與快取管理

### 1. 儲存裝置階層（Storage Hierarchy）

- 越上層 → 速度快、成本高、容量小。
- 越下層 → 容量大、速度慢、成本低。

| 層級         | 裝置範例        | 特性             |
|--------------|------------------|------------------|
| 寄存器        | CPU Register     | 最快、最小       |
| 快取（Cache） | SRAM             | 速度快、成本高   |
| 主記憶體      | DRAM             | 唯一 CPU 可直接存取 |
| 次儲存        | 硬碟、SSD        | 非揮發、大容量   |

---

### 2. RAM 類型

- **DRAM（動態）**：需定期重整、耗電少、成本低。
- **SRAM（靜態）**：不用重整、速度快、成本高，常用於 Cache。

---

### 3. 磁碟存取機制

- 傳輸時間 = 資料大小 ÷ 傳輸速率
- 隨機存取時間 = seek time + rotational latency

---

### 4. 快取（Caching）

- 常用資料從慢速儲存複製到快取區。
- 如果資料已在快取 → 直接使用。
- 若不在 → 從下層搬移到快取再使用。

---

### 5. 一致性與同步問題（Coherency & Consistency）

- 相同資料存在多個層級 → 一致性問題。
- 多任務存取時需同步最新資料版本。
- 分散式系統難以同步，資料可能不一致。

---

## 四、硬體保護機制

### 1. Dual-Mode Operation（雙模式操作）

- 區分兩種模式：
  - **User Mode**：執行使用者程式。
  - **Kernel Mode**（Monitor Mode）：執行 OS。
- Mode Bit 由硬體指示目前執行狀態。
- Interrupt、Trap 發生時自動進入 Kernel Mode。

---

### 2. I/O Protection

- 所有 I/O 指令為特權指令，只能在 Kernel Mode 執行。
- 防止使用者程式改寫中斷向量表或非法操作 I/O。

---

### 3. Memory Protection

- 防止程式間記憶體資料誤存／存取。
- 透過 Base Register（起始位址）與 Limit Register（範圍大小）定義合法記憶體區段。

---

### 4. CPU Protection

- 防止無窮迴圈佔用 CPU。
- 利用 **Timer**：
  - 每次 clock tick 減一。
  - 值為 0 時產生中斷，回到作業系統。
- Timer 是特權指令，用於 Time-sharing 管理。

---

# Chap 2：OS Structure

> **主題：作業系統的服務、應用介面與架構設計**

---

## 一、作業系統提供的服務（OS Services）

### 1. 核心服務項目
- **User interface**（使用者介面）
- **Program execution**（程式執行）
- **I/O operations**（I/O 操作）
- **File-system manipulation**（檔案系統管理）
- **Communication**（溝通：如 IPC）
- **Error detection**（錯誤偵測）
- **Resource allocation**（資源分配）
- **Accounting**（資源使用統計）
- **Protection and security**（保護與安全）

這些功能確保整個系統能高效並正確運作。

---

### 2. User Interface（使用者介面）

- **CLI（Command Line Interface）**：
  - 使用者輸入文字指令，Shell 負責解析與執行（如 BASH, CSHELL）
- **GUI（Graphical User Interface）**：
  - 透過滑鼠與圖示操作，如檔案拖曳與點選
- 現代系統通常同時提供 CLI 與 GUI。

---

### 3. Communication Models（溝通模型）

- **Message Passing**：透過訊息互相溝通（適合分散式系統）
- **Shared Memory**：彼此存取同一記憶體區域（適合多執行緒）

---

## 二、應用程式與作業系統的介面（API & System Call）

### 1. System Calls（系統呼叫）

- 應用程式透過 **System Call** 向作業系統請求服務。
- 常見類型：
  - **Process Control**：建立、終止程序、記憶體配置
  - **File Management**：開啟、關閉、建立、刪除檔案
  - **Device Management**：讀寫裝置
  - **Information Maintenance**：查詢系統時間等
  - **Communication**：傳送與接收訊息

---

### 2. API 與 System Call 的關係

- 使用者通常透過 **API（Application Programming Interface）** 來間接使用系統呼叫。
- API 由程式語言庫實作，如 **C Library (libc)**。
- 範例：
  - `malloc()` / `free()` → 底層使用 `brk()` 系統呼叫
  - `abs()` → 不需使用系統呼叫

---

### 3. 三大常見 API 類型

| 類型 | 平台 |
|------|------|
| **Win32 API** | Windows |
| **POSIX API** | Unix/Linux/macOS |
| **Java API** | JVM 平台 |

---

### 4. 為什麼使用 API 而不是直接用 System Call？

- **簡潔性（Simplicity）**：API 更適合開發者使用
- **可攜性（Portability）**：跨平台統一介面
- **效率（Efficiency）**：非所有功能都需進入核心層（Kernel）

---

### 5. System Call 的參數傳遞方式

1. 利用暫存器傳遞
2. 使用記憶體中的參數表（表格位址放在暫存器中）
3. 使用堆疊（Stack）

---

## 三、作業系統的架構設計（OS Structures）

### 1. 設計目標

- **User Goals**：易用、安全、快速
- **System Goals**：易開發、易維護、穩定與有效率

---

### 2. OS 架構類型

| 架構類型 | 特性 | 優缺點 |
|----------|------|--------|
| **Simple OS** | 單層或兩層架構（如 MS-DOS） | 不安全、難擴充 |
| **Layered OS** | 分層結構（如 Layer 0 ~ N） | 易除錯維護，但效率較低 |
| **Microkernel** | 把大部分功能移到使用者空間，採用訊息傳遞 | 易擴充與移植，效能略低 |
| **Modular OS** | 使用模組化設計，各元件獨立、可動態載入 | 彈性高（如 Solaris） |

---

### 3. Virtual Machine（虛擬機器）

- VM 將實體硬體與作業系統都視為可被模擬的對象。
- 每個程序都像擁有自己一台電腦。
- 優點：
  - 完整資源隔離與保護
  - 提高資源使用率（如雲端）
  - 支援系統相容性與 OS 開發測試

---

### 4. 常見虛擬化技術

| 類型 | 特性 |
|------|------|
| **Full Virtualization（VMware）** | 客戶系統無需修改，如同執行在實體硬體上 |
| **Para-virtualization（Xen）** | 客戶系統需修改，效率較高 |
| **Java Virtual Machine（JVM）** | 執行跨平台的 bytecode，具備 class loader、interpreter、JIT 編譯器 |

---

# Chap 3：Processes Concept

> **主題：程序（Process）與行程控制概念**

---

## 一、Process 基本概念

### 1. Program vs. Process
- **Program**：靜態的執行碼（存在於磁碟中）。
- **Process**：執行中的程式，是一個活躍的實體。
- 一個 process 包含：
  - Code（程式碼區）
  - Data（全域變數區）
  - Stack（暫存資料：參數、回傳位址、區域變數）
  - Heap（動態配置記憶體）
  - Program Counter、Registers
  - 相關資源（如已開啟檔案）

---

### 2. Threads（執行緒）
- 又稱 **輕量級程序（Lightweight Process）**。
- 一個 Process 可包含多個 Threads。
- 同一 Process 的 Threads：
  - **共享**：程式碼區、資料區、開啟的檔案等。
  - **獨立**：Program Counter、Register、Stack。

---

### 3. Process 狀態

| 狀態      | 說明                                 |
|-----------|--------------------------------------|
| New       | 正在建立中的程序                      |
| Ready     | 等待 CPU 的程序                      |
| Running   | 正在由 CPU 執行                      |
| Waiting   | 等待 I/O 或事件發生                  |
| Terminated| 執行結束                             |

---

### 4. Process Control Block（PCB）
- 作業系統用來追蹤每個 Process 的資料結構。
- 包含：
  - 狀態（State）、程式計數器（PC）
  - CPU 註冊、記憶體管理資訊（Base/Limit）
  - 開啟的 I/O 狀態、帳務資訊
- 所有 PCB 組成 queue：如 Ready Queue、Waiting Queue。

---

### 5. Context Switch（上下文切換）
- 系統將目前程序狀態存入 PCB，並載入下一個程序狀態。
- **Switch 過程為 Overhead**，應盡量減少發生頻率。
- 影響因素：記憶體速度、暫存器數量、硬體支援。

---

## 二、Process 排程（Scheduling）

### 1. Scheduling 目的
- 提高 CPU 使用率（Multiprogramming）。
- 提供互動性（Time-Sharing）。

---

### 2. 常見排程器類型

| 類型               | 說明                                     |
|--------------------|------------------------------------------|
| Long-term Scheduler| 控制系統內同時存在的 process 數量。          |
| Short-term Scheduler| 決定哪個 ready process 執行（CPU scheduler）。|
| Medium-term Scheduler| 負責 swap in/out process（如中斷式切換）。     |

---

### 3. Process Queues（程序排程隊列）
- **Job Queue**：所有進入系統的程序。
- **Ready Queue**：在主記憶體中等待執行的程序。
- **Device Queue**：等待特定 I/O 裝置的程序。

---

### 4. Process 之間的關係與操作

- 每個 process 由一個 Parent 建立（如 UNIX 的 fork()）。
- Process 結構形成一棵樹。
- 常見系統呼叫：
  - `fork()`：建立子程序。
  - `exec()`：將記憶體替換為新程式。
  - `wait()`：等待子程序結束。
  - `exit()`：結束程序，釋放資源。

---

### 5. Address Space 複製方式

| 類型 | 說明 |
|------|------|
| Duplicate | 完整複製 parent 的記憶體內容（傳統方式） |
| Copy-on-Write | 子程序僅在寫入時才複製記憶體，節省資源 |

---

## 三、程序間通訊（IPC）

### 1. IPC 定義與目的
- 多個程序或執行緒之間的資料交換機制。
- 分為：
  - **Independent Processes**：互不影響。
  - **Cooperating Processes**：需要通訊。

---

### 2. 通訊方式比較

| 方法           | 特性                                   |
|----------------|----------------------------------------|
| Shared Memory  | 快速、需額外同步處理。多用於同一系統內通訊。 |
| Message Passing| 較慢但同步性好。可用於不同系統之間通訊。   |

---

### 3. Shared Memory 概念
- 程式需明確建立共享區域。
- 負責決定資料格式與存取控制。
- 常見問題：**同步處理、資料一致性**

---

### 4. Producer-Consumer 問題（共享記憶體範例）
- 使用 buffer 作為雙向通訊介面。
- 必須處理：
  - Buffer 滿/空
  - 同步機制（避免 race condition）

---

### 5. Message Passing 方法

- 系統呼叫：
  - `send(destination, msg)`
  - `receive(source, msg)`
- **Direct Communication**：明確指定對象。
- **Indirect Communication**：使用 mailbox（信箱）。

---

### 6. 同步 vs 非同步

| 類型          | 說明                          |
|---------------|-------------------------------|
| Blocking Send | 傳送者等待接收者收到訊息。     |
| Non-blocking Send | 傳送者立即繼續執行。        |
| Blocking Receive | 接收者等待訊息可用。         |
| Non-blocking Receive | 沒有訊息則傳回 null。   |

---

### 7. Socket & RPC（遠端通訊）

#### Socket
- 低階網路通訊工具，由 IP + Port 組成。
- 雙方透過 `socket()`、`bind()`、`connect()` 等建立通訊。

#### RPC（Remote Procedure Call）
- 模擬 function call 跨網路。
- 使用 **Stub & Skeleton** 封裝資料與回傳值。
- 常見問題：
  - 資料格式（XDR）
  - 指標不可傳遞
  - 通訊失敗或重複執行（須處理 ACK、重送）

---

## 四、補充系統機制

### 1. Pipe（管道）

| 類型       | 特性                               |
|------------|------------------------------------|
| Ordinary Pipe | 需 parent-child 關係，只限本機溝通，單向 |
| Named Pipe    | 不需親子關係，多程序共用，支援雙向溝通 |

---

### 2. Java RMI（Remote Method Invocation）
- Java 實作的 RPC。
- 支援動態呼叫、分散物件、跨平台。

---

#
