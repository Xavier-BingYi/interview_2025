# Chap 0：Operating System

> **主題：系統架構與使用者導向的電腦系統分類**

---

## 1. 從系統架構的角度

### 1.1 Mainframe Systems

#### ▸ Batch System
- 最早的電腦系統之一，一次處理一個 Job，無法互動。
- 使用者將程式、資料與控制卡提交給 operator 排程。
- 作業系統負責 job 間的控制轉移（transfer control），無需人為干預。
- 缺點：
  - 無互動性（non-interactive）
  - 一次僅能執行一個 Job（single job）
  - CPU 常處於 idle 狀態，效率低

#### ▸ Multi-programming System
- 多個程式同時駐留在主記憶體中，透過 CPU scheduling 輪流執行。
- 透過 I/O 與運算的重疊（overlapping），提高資源利用率。
- 使用 **Spooling** 技術：I/O 可不經 CPU，僅在完成時通知 CPU。
- OS 核心職責：
  - Memory management（記憶體分配）
  - CPU scheduling（處理器分配）
  - I/O system（裝置管理）

#### ▸ Time-sharing System（= Multi-tasking）
- 支援多使用者互動，使用者可立即看到結果（response time < 1s）。
- 透過時間切片（time slice）方式在多個程序間快速切換。
- 常見裝置：鍵盤、螢幕。
- 作業系統額外職責：
  - Virtual memory（虛擬記憶體管理）
  - File system & Disk management（檔案與磁碟管理）
  - Process synchronization & Deadlock handling（同步與死結處理）

---

### 1.2 系統架構概述（Computer-System Architecture）

#### ▸ Desktop Systems
- 單人使用，常見 OS：Windows, macOS, Linux。
- 支援 GUI，注重便利性與反應速度。
- 缺點：較缺乏安全機制，易受病毒攻擊。

#### ▸ Parallel Systems（Tightly Coupled Systems）
- 多個 CPU 核心共享主記憶體與裝置，效能佳。
- 分為：
  - **SMP（Symmetric Multiprocessor）**：每個 CPU 狀態對等，需同步資料一致性。
  - **AMP（Asymmetric Multiprocessor）**：一顆 Master CPU 負責分配任務，適用大型系統。
- 優點：
  - 提升 throughput（運算吞吐量）
  - 成本較低（資源共享）
  - 可靠性高（部分 CPU 故障時系統可持續運作）

#### ▸ Multi-core vs. Many-core
- Multi-core：單一 CPU 上整合多個核心（2~8），常見於一般 PC。
- Many-core：核心數超過 10，甚至數十、數百，常見於 GPU、高效能運算。
- GPU 採 SIMD（Single Instruction Multiple Data）架構，適合資料平行處理（data parallelism）。

#### ▸ Memory Access Architecture
| 架構 | UMA | NUMA |
|------|-----|------|
| 記憶體存取時間 | 相同 | 不同 |
| 結構 | 所有 CPU 接到同一記憶體 | 系統分為多個 nodes，各有 local memory |
| 特性 | 效能一致、簡單設計 | 跨 node 存取速度較慢，需考慮 locality |

---

### 1.3 Distributed Systems（Loosely Coupled）

#### ▸ 架構與特性
- 多台電腦透過網路（I/O bus、LAN）互相連接。
- 每台電腦擁有自己的 local memory，不共享記憶體。
- 資料交換須透過網路進行。
- 易於擴充，可靠性高（單台故障不影響整體系統）。

#### ▸ 使用目的
- Resource sharing
- Load balancing / Load sharing
- Reliability / Fault-tolerance

#### ▸ 架構類型
- **Client-Server**：
  - 類似 Master-Slave 模式，server 負責分派與協調。
  - 缺點是 server 容易成為效能瓶頸（bottleneck）。
  - 可透過分散式伺服器與 backup server 提升可靠性。
- **Peer-to-Peer**：
  - 所有節點地位平等，依靠 protocols 或 rules 溝通。
  - 無單一故障點（no single point of failure）
  - 系統彈性高、可靠性強（如 Internet）

---

### 1.4 Clustered Systems

- 屬於分散式系統的特殊類型。
- 多台電腦透過高速 LAN（如 Infiniband）連線，部署在同一地區。
- 效能高、延遲低，常用於資料中心或高效能需求場景。
- 分為：
  - **Asymmetric clustering**：一台主機執行，其他待命。
  - **Symmetric clustering**：多台主機同時執行並互相監控。

---

## 2. 使用者導向的系統分類

### 2.1 Real-Time Systems

- 強調「準時完成」，而非「立即反應」。
- 任務必須在使用者定義的 deadline 前完成。
- Scheduler 必須保證任務能在期限內完成。

#### ▸ 類型
- **Hard Real-Time**：若錯過 deadline，系統將發生嚴重失敗（如飛彈控制）。
  - 通常不使用 secondary storage，只依賴記憶體。
- **Soft Real-Time**：盡量在期限內完成，錯過會影響品質但不致崩潰。
  - 常見於多媒體系統（如串流播放）

---

### 2.2 Multimedia Systems

- 處理音訊與視訊（例如直播、串流播放）
- 需處理時間同步與資料壓縮問題（30 fps 等）
- 常用 on-demand 技術與壓縮格式降低傳輸量

---

### 2.3 Handheld / Embedded Systems

- 例：PDA、手機、嵌入式裝置
- 特性：記憶體小、處理器慢、電池壽命短、螢幕小
- 使用專門的作業系統（specialized OS）

---

# Chap 1：Introduction

> **主題：作業系統的基本概念與硬體保護機制**

## 1. 作業系統基本定義與功能

### 1.1 OS 是什麼

#### ▸ OS 的本質與介面角色
- OS 是使用者程式與硬體之間 **主要且必要的介面**
- 管理所有資源存取，提供抽象化操作方式（virtual machine interface）

> 📌 註：課堂中強調 OS 是 "only" interface，但實務上不嚴謹，例如在 embedded systems 或 bare-metal programming 中，程式可直接操作硬體，不一定需要 OS。

#### ▸ OS 與其他軟體的差異
- OS 是「常駐系統」中的 permanent 軟體
- 相較於一般應用程式，OS 提供的 API 抽象化硬體，使上層程式無需直接控制硬體

---

### 1.2 OS 的核心功能與運作方式

#### ▸ OS 的主要角色與職責
1. **Resource Allocator**：管理並分配系統資源（CPU、memory、I/O），兼顧效率與公平性  
2. **Control Program**：控制 user programs 與 I/O 行為，防止錯誤或誤用  
3. **Coordinator**：協調多使用者與多程序的資源競爭  
4. **Interface Provider**：以 system call / API 提供應用程式操作系統資源的介面

#### ▸ 程式執行流程與 system call 運作
- 程式透過 compiler 編譯為 .o 檔，再進行 linking（會自動 link 到 system library）
- 例如 `printf()` 最終透過 C library 呼叫對應的 system call（如 `write()`），由 OS 執行輸出動作

#### ▸ 執行模式與驅動架構
- **User mode**：應用程式執行環境  
- **Kernel mode**：OS 本身執行的空間與權限範圍  
- 驅動程式（driver）屬於 OS，可動態安裝，用來控制 I/O 裝置

---

### 1.3 OS 的設計目標與重要性

#### ▸ OS 的設計目標
- **Convenience**：讓系統更容易使用（例：GUI）
- **Efficiency**：提升效能與資源使用效率

> ⚠️ 二者常互相衝突，例如 GUI 雖提升便利性但會消耗更多資源

#### ▸ 為什麼 OS 很重要
- OS API 是使用者應用程式與硬體之間的介面
- OS 一旦 crash，整台系統會停擺，**不得有錯誤（bug）**
- 掌握 OS 技術即可影響整個軟硬體生態（例：Microsoft 建立 Windows 平台主導權）

---

## 2. 電腦系統組織

---

## 3. 硬體保護（Hardware Protection）

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
