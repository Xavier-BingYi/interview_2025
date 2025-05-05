# 目錄

## Ch0：Operating System
1. [大型主機系統演進與多工系統](#1-大型主機系統演進與多工系統)
   - [1.1 大型主機系統與歷史演進](#11-大型主機系統與歷史演進)
   - [1.2 批次系統（Batch System）](#12-批次系統batch-system)
   - [1.3 多重程式系統（Multi-programming System）](#13-多重程式系統multi-programming-system)
   - [1.4 分時系統（Time-Sharing System）](#14-分時系統time-sharing-system)
   - [1.5 系統比較總結](#15-系統比較總結)
2. [電腦系統架構與運算模型](#2-電腦系統架構與運算模型)
   - [2.1 架構類型概觀](#21-電腦系統架構類型概觀)
   - [2.2 平行系統](#22-平行系統parallel-systems)
   - [2.3 記憶體存取架構](#23-記憶體存取架構memory-access-architectures)
   - [2.4 分散式系統架構](#24-分散式系統架構distributed-systems)
   - [2.5 叢集系統（Clustered Systems）](#25-clustered-systems叢集系統)
3. [特殊應用系統架構](#3-特殊應用系統架構)
   - [3.1 即時系統](#31-即時系統real-time-systems)
   - [3.2 多媒體系統](#32-多媒體系統multimedia-systems)
   - [3.3 手持與嵌入式系統](#33-手持與嵌入式系統handheld-and-embedded-systems)
   - [3.4 系統選擇與設計思維](#34-系統選擇與設計思維)
4. [作業系統基本概念練習題](#4-作業系統基本概念練習題)
   - [4.1 選擇題](#41-選擇題共-25-題)
   - [4.2 申論題](#42-申論題共-10-題)

## Ch1：Introduction
5. [作業系統基本定義與功能](#1-作業系統基本定義與功能)
   - [1.1 OS 是什麼](#11-os-是什麼)
   - [1.2 OS 的核心功能與運作方式](#12-os-的核心功能與運作方式)
   - [1.3 OS 的設計目標與重要性](#13-os-的設計目標與重要性)
6. [電腦系統組織](#2-電腦系統組織computer-system-organization)
   - [2.1 基本架構與 OS 角色](#21-電腦系統基本架構與作業系統角色)
   - [2.2 I/O 與忙碌等待](#22-io-操作流程與忙碌等待問題)
   - [2.3 中斷驅動 I/O](#23-中斷驅動-io-機制interrupt-driven-io)
   - [2.4 中斷處理與同步](#24-中斷處理細節與系統同步)
   - [2.5 Interrupt vs Trap](#25-interrupt-與-trap-的差異與效能考量)
7. [儲存階層與快取管理](#3-儲存階層與快取管理storage-hierarchy-and-cache-management)
   - [3.1 儲存設備與階層設計](#31-儲存設備層級與設計原則)
   - [3.2 RAM 類型](#32-ram-類型與存取特性)
   - [3.3 磁碟效能](#33-磁碟機運作與效能影響)
   - [3.4 Cache 機制](#34-快取記憶體與效能提升)
   - [3.5 Cache 一致性問題](#35-一致性問題與分散式挑戰)
8. [硬體保護與執行模式](#4-硬體保護與執行模式hardware-protection-and-execution-modes)
   - [4.1 系統保護需求](#41-系統保護的概念與需求)
   - [4.2 Dual-Mode 操作](#42-雙模式操作dual-mode-operation)
   - [4.3 I/O 保護與防範](#43-io-保護與惡意操作防範)
   - [4.4 記憶體保護](#44-記憶體保護memory-protection)
   - [4.5 CPU 保護與 Timer](#45-cpu-保護與-timer-機制)
9. [作業系統導論練習題](#5-作業系統導論練習題)
   - [5.1 選擇題](#51-選擇題共-20-題)
   - [5.2 申論題](#52-申論題共-8-題)

---

# Ch0：Operating System

> **主題：系統架構與使用者導向的電腦系統分類**

---

## 1. 大型主機系統演進與多工系統（Mainframe and Multiprogramming Systems）

---

### 1.1 大型主機系統與歷史演進

#### ▸ 系統分類與主機系統角色
- 主機系統（Mainframe Systems）為最早的電腦架構，起初設計僅能處理單一工作（Job）。
- 主要應用於需要高可靠性與安全性的領域，如醫院、銀行、大型資料處理中心。

#### ▸ 主機系統的三個發展階段
1. **Batch System（批次系統）**
2. **Multi-programming System（多重程式系統）**
3. **Time-sharing System（分時系統）**

---

### 1.2 批次系統（Batch System）

#### ▸ 執行流程
- 使用者將程式、資料與控制卡提交給系統操作員（operator）。
- 操作員依照資源需求將 Job 分類，並儲存至磁帶，送入主機依序處理。
- OS 僅負責將控制權從一個 Job 轉移至下一個，無需互動與決策。

#### ▸ 特徵與限制
- 無使用者互動，無法即時控制。
- 一次僅能處理一個 Job，CPU 常因 I/O 等待而閒置。
- CPU 利用率低，I/O 與 CPU 速度差異極大（1:1000）。

---

### 1.3 多重程式系統（Multi-programming System）

#### ▸ 系統特性
- 允許數個程式同時存在於主記憶體中，CPU 在程式之間切換以減少 idle time。
- 同時處理運算與 I/O，提升資源使用率。

#### ▸ Spooling 技術（Simultaneous Peripheral Operation On-Line）
- I/O 操作由系統代為處理，CPU 僅需在 I/O 完成時接收通知。
- 減少 CPU 對 I/O 的等待時間。

#### ▸ 作業系統任務
- **記憶體管理（Memory Management）**：分配記憶體空間給多個程式。
- **CPU 排程（CPU Scheduling）**：決定哪個程式獲得 CPU。
- **I/O 系統管理**：統一處理所有輸入輸出資源與例程。

---

### 1.4 分時系統（Time-Sharing System）

#### ▸ 系統特性
- 又稱為 Multi-tasking System，允許多名使用者同時與系統互動。
- CPU 透過高速切換提供類似即時的回應效果（response time < 1s）。
- 常見輸入裝置為鍵盤，輸出為螢幕。

#### ▸ 程式切換時機
- 程式結束、等待 I/O、或達到時間片上限（Time Quantum）時觸發切換。

#### ▸ 作業系統任務
- **虛擬記憶體管理（Virtual Memory）**：允許工作在記憶體與磁碟間切換，降低主記憶體壓力。
- **檔案與磁碟管理（File & Disk Management）**：處理使用者資料儲存與存取。
- **程序同步與死結管理（Synchronization & Deadlock）**：保障多個程序能安全同步執行。

---

### 1.5 系統比較總結

#### ▸ 三種系統比較表

| 系統類型         | 執行模式           | 系統目標         | OS 功能涵蓋                             |
|------------------|--------------------|------------------|----------------------------------------|
| Batch            | 單一使用者、單一工作 | 簡化控制流程     | 無明確 OS 功能                         |
| Multi-programming | 多個程式           | 資源利用率最大化 | 記憶體管理、CPU 排程、I/O 系統        |
| Time-sharing     | 多使用者、多程式   | 提升即時回應     | 加入虛擬記憶體、檔案系統、同步與死結處理 |

---

## 2. 電腦系統架構與運算模型（Computer System Architectures and Models）

---

### 2.1 電腦系統架構類型概觀

#### ▸ 三大系統類別
- **Desktop Systems（桌上型系統）**：單使用者導向，強調互動性與操作便利性。
- **Parallel Systems（平行系統）**：多核心或多處理器，強調運算效能與可靠性。
- **Distributed Systems（分散式系統）**：由多台電腦透過網路互聯而成，具備高度擴展性與容錯性。

---

### 2.2 平行系統（Parallel Systems）

#### ▸ 多處理器架構（Multiprocessor Systems）
- 又稱為 **Tightly Coupled Systems**，具備兩個以上 CPU，彼此透過共享記憶體通訊。
- 優點：**高吞吐量（throughput）**、**成本效益（economical）**、**可靠性高（reliability）**。

#### ▸ 架構類型
- **Symmetric Multiprocessor (SMP)**：每顆 CPU 地位相同，執行同一個 OS，需同步機制保護資料一致性。
- **Asymmetric Multiprocessor (AMP)**：主從式架構，Master CPU 管理工作分派，其餘 CPU 執行子任務，擴充性佳但資源浪費風險較高。

#### ▸ 多核心處理器（Multi-Core Processor）
- 單一 CPU 晶片內含多個核心（cores），可同時執行多個任務。
- On-chip 通訊速度快、省電、熱耗低。
- 每個核心有獨立的 register 與 L1 cache，可能共用 L2/L3 cache。

#### ▸ Many-Core 與 GPU 計算
- Many-Core：10 核心以上處理器，常見於 HPC 與 AI 運算。
- GPU 採用 **SIMD 架構（Single Instruction, Multiple Data）**，擅長資料平行處理，適用於矩陣、圖像處理等場景。

---

### 2.3 記憶體存取架構（Memory Access Architectures）

#### ▸ UMA（Uniform Memory Access）
- 常見於 **SMP 系統**，所有 CPU 對主記憶體擁有相同存取時間。
- 優點：系統設計簡單，任務可自由分派給任一 CPU。

#### ▸ NUMA（Non-Uniform Memory Access）
- 將系統分成多個節點（node），每個節點包含 CPU 與本地記憶體。
- 跨節點存取需透過連接通道，速度較慢，造成記憶體存取時間不一致。

---

### 2.4 分散式系統架構（Distributed Systems）

#### ▸ 定義與特色
- 又稱為 **Loosely Coupled Systems**，每台電腦有獨立 CPU 和 local memory，透過網路通訊。
- 擴展性高（可達數萬台節點）、具備高容錯能力與負載平衡。

#### ▸ 分散式系統的用途
- **Resource Sharing**：共享資源與服務。
- **Load Sharing**：動態分配工作量。
- **Reliability**：系統部分故障不會影響整體運作。

#### ▸ 架構分類
- **Client-Server 架構**：Server 管理資源並提供服務，Client 提出請求。
  - 優點：集中管理容易。
  - 缺點：Server 成為效能瓶頸與單點失效風險。
- **Peer-to-Peer 架構**：所有節點地位平等，無中央控制。
  - 優點：無單一故障點，可靠性高，動態彈性佳。
  - 例子：BitTorrent、Internet。

---

### 2.5 Clustered Systems（叢集系統）

#### ▸ 定義與結構
- 多台電腦透過 **區域網路（LAN）** 或高速互連架構（如 InfiniBand）緊密連結，並共用儲存裝置。
- 類似分散式系統，但傳輸延遲更低、通訊速度更快。

#### ▸ 架構類型
- **Asymmetric Clustering**：一台主機執行服務，其它主機待命備援。
- **Symmetric Clustering**：多台主機同時運作並互相監控。

---

## 3. 特殊應用系統架構（Special-Purpose System Architectures）

---

### 3.1 即時系統（Real-Time Systems）

#### ▸ 系統定義與目標
- Real-Time 並不代表「執行速度快」，而是**在預期的時間內完成任務**（符合 deadline）。
- 系統需保證在固定時間內作出反應，常應用於工業控制、醫療影像、科學實驗、武器系統等。

#### ▸ 系統分類
- **Hard Real-Time**：
  - 違反 deadline 將導致嚴重系統錯誤（如核能控制系統）。
  - 常不使用 secondary storage（如硬碟），資料儲存在 ROM 或 memory。
- **Soft Real-Time**：
  - 違反 deadline 可容忍，但會影響系統品質（如多媒體串流）。
  - 以 priority 控制排程，確保關鍵任務優先完成。

#### ▸ 系統設計挑戰
- 設計需考慮：
  - **Scheduling Algorithm**（排程演算法）
  - **資源配置與儲存限制**（無硬碟時的資料管理）
- 最常見的排程方式：**Earliest Deadline First (EDF)**。

---

### 3.2 多媒體系統（Multimedia Systems）

#### ▸ 系統特性
- 處理大量音訊與影像資料（如線上直播、ppStream、影音平台等）。
- 有即時處理需求，如 24~30fps 畫面播放。

#### ▸ 技術挑戰
- **時間限制（Timing Constraints）**：需在固定時間內傳送或解碼資料。
- **即時串流（Live Streaming）與隨選播放（On-demand）**：
  - 媒體資料不儲存於本地，只播放不儲存。
- **壓縮（Compression）技術**：
  - 降低傳輸壓力與儲存需求。

---

### 3.3 手持與嵌入式系統（Handheld and Embedded Systems）

#### ▸ 系統範例
- 手機、PDA（個人數位助理）、智慧裝置、嵌入式控制器等。
- 系統通常內建於硬體內，具備專用用途。

#### ▸ 設計限制
- **記憶體容量小**
- **處理器效能低**
- **電池壽命限制**
- **螢幕尺寸小**
- 作業系統需特別設計以符合這些硬體限制。

---

### 3.4 系統選擇與設計思維

#### ▸ 系統設計依用途選擇
- 不同應用目的對 OS 設計有不同取捨：
  - 嵌入式系統強調效率與小體積。
  - 多媒體系統強調即時處理。
  - 即時系統重視可預測性與穩定排程。
  
#### ▸ 系統類型對比

| 系統類型             | 特性關鍵字                         | 應用案例                      |
|----------------------|------------------------------------|-------------------------------|
| Real-Time            | Hard/Soft Deadline、Scheduler     | 醫療、工控、武器系統          |
| Multimedia           | 音影同步、壓縮、直播               | YouTube、串流平台             |
| Embedded / Handheld  | 小型、低功耗、專用用途             | 手機、智慧家電、車用電腦等    |

---

## 4. 作業系統基本概念練習題

---

### 4.1 選擇題（共 25 題）
> ✅ 單選題形式，幫助複習 Ch0 核心概念與面試常見題型。

1. 哪一種作業系統主要透過批次方式執行工作，無需使用者即時互動？  
   A. Time-sharing 系統  
   B. Batch 系統  
   C. Multi-programming 系統  
   D. Distributed 系統  

2. Multi-programming 系統的主要目的是什麼？  
   A. 提升系統安全性  
   B. 減少記憶體使用  
   C. 減少 CPU idle time  
   D. 增加硬碟空間  

3. 下列何者屬於 Time-sharing 系統的特徵？  
   A. 每次只能執行一個程式  
   B. 無需 CPU scheduling  
   C. 使用者可即時互動  
   D. 無需記憶體管理  

4. 在 Symmetric multiprocessing (SMP) 系統中，哪一項正確？  
   A. 每顆 CPU 執行不同 OS  
   B. 主從結構明確  
   C. 各處理器共享相同作業系統  
   D. 僅適用於大型機系統  

5. Distributed system 的主要優點為何？  
   A. 資料集中存取  
   B. 單點失敗風險低  
   C. 無需網路連接  
   D. CPU 不需排程  

6. 在 UMA 架構中，哪一項敘述正確？  
   A. 各 CPU 存取記憶體速度不一致  
   B. 所有 CPU 有均等存取主記憶體的時間  
   C. 僅主 CPU 可存取記憶體  
   D. 各 CPU 擁有獨立記憶體  

7. 下列何者屬於 Hard Real-Time 系統的應用範例？  
   A. 音樂串流  
   B. 網頁瀏覽器  
   C. 核電廠控制系統  
   D. 網路影片播放  

8. Peer-to-Peer 系統的優點是？  
   A. 集中式管理  
   B. 高可靠性  
   C. 效能瓶頸集中  
   D. 依賴主伺服器  

9. 下列何者通常與 SIMD 架構有關？  
   A. GPU  
   B. HDD  
   C. CPU Cache  
   D. BIOS  

10. NUMA 架構與 UMA 最大差異為？  
    A. 使用單核心處理器  
    B. 記憶體存取速度一致  
    C. 跨節點存取速度較慢  
    D. 僅有一台主機  

11. 在 Client-Server 架構中，常見缺點是什麼？  
    A. 管理困難  
    B. 多點失敗風險  
    C. Server 成為效能瓶頸  
    D. 所有節點無法溝通  

12. Multi-core processor 的優點之一是？  
    A. 增加耗電量  
    B. 降低傳輸速度  
    C. 降低功耗並提升溝通效率  
    D. 不需要作業系統支援  

13. GPU 屬於哪一種平行運算架構？  
    A. SISD  
    B. SIMD  
    C. MISD  
    D. MIMD  

14. 下列何者屬於 loosely coupled 架構？  
    A. SMP 系統  
    B. Parallel 系統  
    C. Distributed 系統  
    D. Multi-core 系統  

15. 在硬即時系統中，missing deadline 的結果是什麼？  
    A. 稍微降低系統效能  
    B. 提醒使用者稍後執行  
    C. 系統可能產生嚴重錯誤或崩潰  
    D. 啟動後備系統  

16. Multimedia 系統面臨的主要挑戰之一是？  
    A. 繪圖晶片成本  
    B. 實時解壓與傳輸的需求  
    C. 硬體不相容問題  
    D. 使用者權限控管  

17. 下列何者為軟即時系統（Soft Real-Time）的特徵？  
    A. 不能錯過任何 deadline  
    B. 延遲會導致災難性後果  
    C. 任務能夠根據優先權排程  
    D. 沒有任何排程需求  

18. 哪種系統架構最適合用於數百個節點的大規模運算？  
    A. SMP  
    B. Multi-core  
    C. Distributed  
    D. Batch  

19. 在 Clustered Systems 中，對稱叢集的特徵是？  
    A. 一台主機執行應用程式，其餘為備援  
    B. 所有主機均執行應用程式並互相監控  
    C. 主伺服器負責所有排程  
    D. 所有主機彼此獨立無連線  

20. 下列哪一項不是作業系統的基本功能？  
    A. 資料庫管理  
    B. 記憶體管理  
    C. 處理程序排程  
    D. 檔案系統管理  

21. 哪種技術可用來讓 I/O 與 CPU 同時運作？  
    A. Paging  
    B. Pipelining  
    C. Spooling  
    D. Polling  

22. Desktop 系統的最常見特徵是什麼？  
    A. 執行即時應用  
    B. 支援多用戶同步執行  
    C. 提供 GUI 與人機互動介面  
    D. 高延遲處理能力  

23. 在多處理器系統中，哪一項屬於非對稱架構？  
    A. 多核心處理器  
    B. SMP  
    C. Master-slave 模式  
    D. Peer-to-peer 模式  

24. TILE64 是什麼樣的處理架構？  
    A. 單核心高頻率  
    B. 多層快取記憶體  
    C. 網格型多核心處理器  
    D. 雙系統混合設計  

25. 在作業系統中，哪一項功能與 Deadlock 處理最有關？  
    A. 處理器切換  
    B. 虛擬記憶體  
    C. 同步機制  
    D. 網路傳輸  

---

#### 參考答案

> ✅ 選擇題解析區，快速對照與自我檢討。

1. **B**：Batch 系統無需使用者互動，按順序執行作業。
2. **C**：Multi-programming 讓 CPU 不因 I/O 阻塞而閒置。  
3. **C**：Time-sharing 系統允許多位使用者即時互動。  
4. **C**：SMP 架構下，所有 CPU 執行同一作業系統。  
5. **B**：分散式系統不依賴單一伺服器，容錯性高。  
6. **B**：UMA 架構中 CPU 存取記憶體時間一致。  
7. **C**：硬即時系統需保證準時完成任務，如核電控制器。  
8. **B**：P2P 結構節點平等，具有高可靠性。  
9. **A**：GPU 使用 SIMD 運算，適合大規模資料並行處理。  
10. **C**：NUMA 架構中跨節點存取記憶體較慢。  
11. **C**：Client-Server 結構常出現 Server 效能瓶頸問題。  
12. **C**：多核心處理器在同一晶片中降低能耗並提升效能。  
13. **B**：GPU 採用 SIMD 架構進行資料平行運算。  
14. **C**：Distributed systems 是 loosely coupled 架構代表。  
15. **C**：硬即時系統若錯過 deadline，可能導致系統失敗。  
16. **B**：Multimedia 系統需處理即時壓縮與播放的挑戰。  
17. **C**：Soft Real-Time 系統允許延遲，但會根據優先權調度。  
18. **C**：分散式系統適合大規模多節點的運算環境。  
19. **B**：對稱叢集中的多台主機彼此監控、共同執行。  
20. **A**：資料庫管理並非作業系統的核心功能。  
21. **C**：Spooling 可讓 I/O 與 CPU 並行作業，提升效率。  
22. **C**：Desktop 系統著重於人機互動與 GUI 介面。  
23. **C**：Master-slave 是非對稱多處理架構的代表。  
24. **C**：TILE64 是一種網格型（mesh）多核心處理器設計。  
25. **C**：同步機制可避免或處理系統資源競爭造成的 Deadlock。 

---

### 4.2 申論題（共 10 題）
> ✅ 深論式問題，強化理解並訓練敘述能力，可應用於開放式面試題。

1. 請說明 Batch 系統的工作流程與其主要缺點。  
2. Multi-programming 系統如何提升 CPU 使用效率？請說明其原理並舉例。  
3. 試比較 Multi-programming 與 Time-sharing 系統在使用者互動上的差異。  
4. 說明 Spooling 技術的概念，並解釋它為何可以提升 I/O 效率。  
5. 請比較 UMA 與 NUMA 的架構差異，並說明其對記憶體存取速度的影響。  
6. 試分析 SMP（對稱多處理器）與 AMP（非對稱多處理器）的運作差異與應用情境。  
7. 請說明 Distributed System 的架構特性，以及其在 fault tolerance 上的優勢。  
8. 在 Client-Server 與 Peer-to-Peer 架構中，哪一種更適合用於高可用性系統？請說明理由。  
9. 解釋 Real-Time 系統中的 Hard 與 Soft Real-Time 的差異，並舉出各自的應用實例。  
10. 請說明 Multimedia 系統在作業系統設計上的挑戰，並指出至少兩項關鍵技術。

---

#### 參考答案

> ✅ 申論題參考解析，可用於自我檢查與面試答題練習。

1. **Batch 系統的工作流程與缺點**
   使用者提交一批作業（程式、資料與控制指令），由作業員分類並依序放入磁帶，再由系統依序執行。主要缺點包括：無即時互動、一次只能執行一個作業、CPU 經常閒置。

2. **Multi-programming 提升 CPU 使用率的方式**  
   可同時將多個程式放入記憶體，當某程式等待 I/O 時，CPU 可切換執行其他程式，達成資源重疊使用，降低 idle time。例如一程式執行磁碟讀取時，另一程式可進行運算。

3. **Multi-programming vs. Time-sharing 在互動性上的比較**  
   Multi-programming 著重於 CPU 資源使用率，無法提供即時互動；Time-sharing 透過快速切換與分時機制，讓多位使用者感受像是獨占系統，提供即時反應。

4. **Spooling 技術與 I/O 效率**  
   Spooling 將 I/O 作業寫入暫存區（如磁碟）而非直接與裝置溝通，避免 CPU 等待 I/O 完成，提升效率。例：列印作業先寫入磁碟，等印表機空閒時再逐一處理。

5. **UMA vs. NUMA 架構差異**  
   UMA（Uniform Memory Access）中所有 CPU 存取記憶體時間相同；NUMA（Non-Uniform Memory Access）中，CPU 存取本地記憶體快，跨節點存取慢。NUMA 適合大規模系統，需考慮資料配置以避免效能瓶頸。

6. **SMP vs. AMP 的比較**  
   SMP（Symmetric Multiprocessing）：所有處理器共享作業系統與記憶體，彼此平等；AMP（Asymmetric Multiprocessing）：Master CPU 控制其他 Slave CPU，適用於大型系統，彈性較差但易管理。

7. **Distributed System 架構與容錯能力**  
   每台機器有獨立記憶體與處理能力，透過網路溝通。具去中心化特性，若某台機器故障，其他節點仍能持續運作，提升系統可靠性與擴充性。

8. **Client-Server vs. P2P 在高可用性系統中的選擇**  
   P2P 架構無中心節點，每台節點角色平等，避免單點失效，更具高可用性。Client-Server 雖管理簡單，但 Server 容易成為 bottleneck 或 failure point。

9. **Hard vs. Soft Real-Time 的差異與應用**  
   Hard Real-Time：若超過 deadline 即為系統失敗（如飛彈控制、核電系統）；Soft Real-Time：延遲容許但不佳（如串流影音）。Hard 更嚴謹，Soft 則彈性高。

10. **Multimedia 系統的挑戰與關鍵技術**  
   挑戰包括即時處理需求、資料量大、壓縮需求與資源配置。關鍵技術如壓縮技術（codec）、時序控制（streaming buffer）、即時排程機制等。


---

# Ch1：Introduction

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

## 2. 電腦系統組織（Computer-System Organization）

---

### 2.1 電腦系統基本架構與作業系統角色

#### ▸ 電腦系統的基本組成
- 由一個或多個 CPU、裝置控制器（device controllers）與共用記憶體（shared memory）組成，並以匯流排（bus）互連。
- 採用 von Neumann 架構，CPU、Memory、I/O Devices 串接成一個完整運作系統。

#### ▸ 系統運作目標
- 實現 CPU 與各裝置的**並行執行（concurrent execution）**。
- 需要 OS **控制**與**協調**多個程序和裝置對記憶體的競爭。

#### ▸ OS 的控制與協調
- OS 負責管理硬體資源存取，防止程序之間互相干擾或破壞記憶體資料。
- 確保程式間**正確共享記憶體**與**資源獨立性**。

---

### 2.2 I/O 操作流程與忙碌等待問題

#### ▸ 裝置控制器（Device Controller）結構
- 每個 controller 負責一種特定裝置，並有本地 buffer。
- 控制器設有 **Status Register** 和 **Data Register**：
  - Status Register：顯示裝置當前狀態（如 idle 或 busy）。
  - Data Register：小型緩衝區，暫存傳輸資料。

#### ▸ 資料交換流程
- **Device ↔ Controller Buffer**：由控制器直接處理。
- **Buffer ↔ Memory**：由 CPU 發出指令搬移。

#### ▸ 忙碌等待（Busy-Waiting）
- 最簡單的 I/O 方式：CPU 不斷輪詢裝置是否就緒。
- 例：將字串一個字元一個字元寫入資料暫存器，並在每次傳送後 busy wait 等待裝置就緒。

#### ▸ Busy Waiting 的缺點
- CPU 必須空等，無法執行其他工作，導致效率極低。
- 無法支援多工（multitasking），系統效能嚴重受限。

---

### 2.3 中斷驅動 I/O 機制（Interrupt-Driven I/O）

#### ▸ 中斷機制概述
- 中斷（Interrupt）允許裝置主動通知 CPU，而不是由 CPU 主動輪詢。
- 改善 busy waiting 的缺點，使 CPU 可進行其他計算，提升系統效率。

#### ▸ 中斷處理流程
1. Device driver 透過 system call 啟動 I/O。
2. Controller 執行實際資料傳輸。
3. 傳輸完成後，controller 發送 interrupt signal。
4. CPU 接收中斷並切換至 Interrupt Handler。
5. Interrupt Handler 處理完成後，返回原本程式。

#### ▸ 硬體中斷與軟體中斷
- **Hardware Interrupt**：裝置事件（如滑鼠移動、鍵盤輸入）觸發。
- **Software Interrupt（Trap）**：程式主動呼叫 system call 或產生錯誤（如 division by zero）。

#### ▸ 中斷向量表（Interrupt Vector）
- 系統維護中斷向量（array of function pointers），每個中斷對應一個 Handler。
- 安裝驅動程式時，更新中斷向量以指向新的中斷處理程序。

---

### 2.4 中斷處理細節與系統同步

#### ▸ Resident Monitor 與 Service Routine
- Resident Monitor 常駐記憶體，負責中斷控制與管理。
- 當中斷發生時，查詢 interrupt vector 找到對應 service routine 執行。

#### ▸ 軟體中斷處理流程
- User program 呼叫 system call（如 read、write）→ 系統產生 software interrupt。
- OS 根據 call number 進行 switch-case，找到對應的服務例程執行，完成後回到 user 程式。

#### ▸ 中斷的同步與保護機制
- 當中斷處理進行中時，會暫時 disable 其他低優先權的中斷（interrupt masking）。
- 確保高優先權中斷能即時處理，避免 lost interrupt 和系統狀態混亂。
- 必須保存中斷發生時的 register、program counter（PC）、memory state，確保能正確 resume。

---

### 2.5 Interrupt 與 Trap 的差異與效能考量

#### ▸ Interrupt 與 Trap 比較
- **Interrupt**：來自外部裝置（硬體中斷）。
- **Trap**：由使用者程式內部事件（錯誤或 system call）主動觸發。

#### ▸ 中斷處理開銷
- 中斷發生時需要儲存大量上下文（context saving），會帶來額外負擔（overhead）。
- 太頻繁的中斷會降低整體系統效能。

#### ▸ 系統效能最佳化
- 使用 Assembly Code 撰寫中斷服務程序，確保極速處理。
- 嚴格區分高、低優先權中斷，必要時 mask 掉低優先權中斷以提升效率。

---

## 3. 儲存階層與快取管理（Storage Hierarchy and Cache Management）

---

### 3.1 儲存設備層級與設計原則

#### ▸ 儲存階層的基本架構
- 資料儲存採用 **分層式架構（hierarchical architecture）**，上層裝置速度快、容量小、價格高，下層裝置相反。
- 常見層級包括：**register → cache → main memory → SSD → disk → tape**。
- 層級設計平衡了**成本、容量與存取速度**，提供最佳效能與彈性。

#### ▸ 主要與次級儲存裝置
- **Main Memory** 是唯一能被 CPU 直接存取的大型儲存裝置（如 DRAM）。
- **Secondary Storage**（如 HDD、SSD）無法直接由 CPU 存取，資料需搬到主記憶體後才能使用。
- 擴展性大，具備 **non-volatile**（斷電不遺失）特性，適合儲存長期資料。

---

### 3.2 RAM 類型與存取特性

#### ▸ DRAM（Dynamic RAM）
- 使用單一電晶體儲存每個 bit，體積小、成本低、耗電少，但速度較慢。
- 必須定期刷新（refresh），常用於主記憶體（main memory）。

#### ▸ SRAM（Static RAM）
- 每個 bit 使用六個電晶體，速度快但體積大、成本高、耗電高。
- 適用於 cache 記憶體，提供更快速的暫存能力。

#### ▸ 隨機存取特性（Random Access）
- RAM 屬於 **random access memory**，無論存取哪個位置，延遲一致。
- 有助於系統預測效能表現，與硬碟等非隨機存取裝置不同。

---

### 3.3 磁碟機運作與效能影響

#### ▸ 磁碟存取時間組成
- 存取時間 = **Positioning Time**（包含 seek time + rotational latency）+ **Transfer Time**
- 傳統磁碟速度慢、非 uniform access，不適合即時系統使用。

#### ▸ SSD 與硬碟比較
- SSD 無機械結構，無 seek time，存取速度快，屬於非揮發性。
- 若為大量、連續存取情況，SSD 與 HDD 差距反而變小（transfer time 為主）。

---

### 3.4 快取記憶體與效能提升

#### ▸ Cache 概念與原理
- 快取是一種**暫存常用資料**的高速儲存空間，提升存取效率。
- 資料會從慢速儲存層級暫存到較快層級（如 CPU cache）。

#### ▸ Cache Hit 與 Miss
- 若資料存在 cache → **cache hit**，可快速讀取。
- 若資料不存在 → **cache miss**，需逐層向下尋找，花費更多時間。

#### ▸ 區域性（Locality）特性
- 多數程式存取具有 **temporal（時間）與 spatial（空間）區域性**，提高 cache 命中率。
- 若資料不重複使用（如 big data scan），cache 效益反而下降。

---

### 3.5 一致性問題與分散式挑戰

#### ▸ Cache Coherency 問題
- 多層快取存在同一份資料的 copy，若修改未同步會產生不一致（coherency issue）。
- 特別在 **multi-core processor** 中，每個核心都有獨立 cache，更易發生此問題。
- 需使用 **cache coherence protocol**（如 MESI）來確保一致性。

#### ▸ Consistency in Multi-thread & Distributed System
- 在多執行緒（multi-thread）與分散式系統（distributed system）中，資料共享與同步更為複雜。
- 系統需設計一致性協定與同步機制，以保障使用者程式讀到的資料為正確版本。
- 例如 Google 等大型系統，有時會**犧牲一致性（consistency）來換取效能與擴充性（scalability）**。

---

## 4. 硬體保護與執行模式（Hardware Protection and Execution Modes）

---

### 4.1 系統保護的概念與需求

#### ▸ 為什麼需要保護機制
- Protection 並非指安全性（security），而是避免多個程式或使用者同時使用電腦時，彼此干擾或破壞系統。
- 例如：某使用者的程式 crash，不應影響其他程式或導致整台系統關機。

#### ▸ 作業系統如何實現保護
- OS 必須能夠控管對 I/O、記憶體、CPU 等硬體資源的存取，並保證各程序間獨立運作。
- 實現保護機制的基礎在於硬體支援，例如模式切換與指令限制。

---

### 4.2 雙模式操作（Dual-Mode Operation）

#### ▸ 執行模式的區分
- 現代系統透過 mode bit 將執行模式分為：
  - **User Mode**：執行使用者程式。
  - **Kernel Mode（Monitor Mode）**：執行作業系統內部程式。

#### ▸ 模式切換與中斷
- 當 system call 或 interrupt 發生時，系統自動將 mode bit 從 1 切換為 0，進入 Kernel Mode。
- OS 處理完成後，再將 mode bit 切回 1，回到 User Mode。

#### ▸ 特權指令（Privileged Instructions）
- 僅能在 Kernel Mode 執行，如 I/O 操作、改寫中斷向量、變更記憶體設定等。
- 若在 User Mode 嘗試執行，會觸發中斷並由 OS 處理（通常終止程式）。

---

### 4.3 I/O 保護與惡意操作防範

#### ▸ 為何需要 I/O 保護
- I/O 裝置（如鍵盤、滑鼠、硬碟）為共享資源，若使用者能自由控制，將可能造成資料衝突或系統崩潰。
- 因此，**所有 I/O 指令皆為特權指令**，只能由 OS 控制與調度。

#### ▸ 潛在風險與攻擊方式
- 若未限制記憶體權限，惡意程式可改寫 OS 的中斷向量，讓硬體中斷執行惡意程式碼。
- 類似攻擊包含：改寫驅動程式、變更中斷處理流程、任意傳送資料等。

---

### 4.4 記憶體保護（Memory Protection）

#### ▸ 保護哪些內容
- 重要結構：中斷向量表（interrupt vector）、中斷服務程式（ISR）、OS 資料區。
- 避免程式存取、改寫不屬於自身的記憶體空間。

#### ▸ 硬體支援機制
- 使用兩個 register：
  - **Base register**：記憶體起始位置。
  - **Limit register**：可存取的記憶體範圍。
- 每次存取都會比對是否落在合法範圍，否則觸發錯誤（如 segmentation fault）。

#### ▸ 保護的實際效果
- 每個程式只能操作自己的記憶體，避免讀取他人資料或干擾系統結構。
- 所有記憶體管理操作須經過 OS 核可。

---

### 4.5 CPU 保護與 Timer 機制

#### ▸ 為什麼需要 CPU 保護
- 若單一程式無限制佔用 CPU，其他程式將無法執行，造成資源壟斷與死當風險。
- 保護機制能強制程式交出 CPU 控制權，確保系統公平運作。

#### ▸ Timer 機制的應用
- 作業系統設置 **計時器（timer）**，每經過固定時間產生一次中斷。
- 當中斷發生，CPU 會跳出目前程式，由 OS 根據排程策略（scheduling）選擇下一個程式執行。

#### ▸ 中斷與程序切換
- Timer 中斷是 context switch 的契機。
- 可實現 time-sharing、確保每個程序公平執行並預防無限迴圈霸佔 CPU。

---

以下是根據您提供的 Chap1\_Introduction.pdf 製作的練習題內容，已完整依照您指定的 Markdown 格式排版（含選擇題與申論題）：

---

## 5. 作業系統導論練習題

---

### 5.1 選擇題（共 20 題）

> ✅ 單選題形式，涵蓋 Ch1 重點與常見考題。

1. 下列何者不是作業系統的主要目標？
   A. 提供便利的操作介面
   B. 管理硬體資源
   C. 提高使用者成本
   D. 增進系統效率

2. 作業系統中所謂的「resource allocator」是指？
   A. 使用者帳號管理
   B. 系統啟動流程
   C. 資源的分配與管理者
   D. 網路流量監控

3. 哪一項功能是作業系統提供給使用者的「便利性功能」？
   A. 磁碟格式化
   B. 命令行介面與 GUI
   C. 虛擬記憶體
   D. 中斷處理機制

4. 下列何者不屬於作業系統的功能？
   A. 硬體抽象
   B. 資源分配
   C. 使用者程式開發
   D. 系統保護

5. 在作業系統中，作業（job）與程式（program）的主要差異為何？
   A. Job 必定為圖形介面程式
   B. Job 由使用者控制，program 不會執行
   C. Job 為執行中的 program，與資源結合
   D. Program 是執行緒的一部分

6. 一個作業系統若能同時支援多位使用者，應具備何種能力？
   A. Time-sharing
   B. Single-tasking
   C. Instruction pipelining
   D. None of the above

7. 為了讓應用程式不需直接接觸硬體，作業系統提供了什麼？
   A. 硬體驅動程式
   B. System calls
   C. Command prompt
   D. Memory cache

8. 下列何者屬於 system call 的用途？
   A. 啟動瀏覽器
   B. 建立或刪除檔案
   C. 設定網頁內容
   D. 儲存資料庫紀錄

9. 在雙模（Dual-Mode）作業系統中，兩種模式是？
   A. Idle Mode 與 Busy Mode
   B. User Mode 與 Kernel Mode
   C. CPU Mode 與 IO Mode
   D. Interactive 與 Batch

10. 為何需要特權指令（privileged instruction）？
    A. 簡化程式語法
    B. 提升使用者效能
    C. 保護系統資源安全
    D. 節省磁碟空間

11. 哪一項不是屬於 system call 的類型？
    A. Process control
    B. File manipulation
    C. HTML rendering
    D. Communication

12. 作業系統的哪一個角色與保護（protection）機制最相關？
    A. UI 設計者
    B. Scheduler
    C. Resource allocator
    D. 系統管理員

13. 為什麼作業系統會提供虛擬機器（Virtual Machine）功能？
    A. 減少系統耗電
    B. 模擬不同平台運作
    C. 儲存多個 BIOS 設定
    D. 提升滑鼠操作體驗

14. 使用者程式（User programs）與作業系統核心（Kernel）主要差別為？
    A. 使用者程式需有管理權限
    B. 核心無法使用 system call
    C. 核心可執行特權指令
    D. 使用者程式可直接操作硬體

15. 作業系統的哪些設計目標是互相衝突的？
    A. 效率與成本
    B. 使用性與效能
    C. 安全性與排程
    D. GUI 與 CLI

16. 當程式欲執行需存取硬體之操作，需透過何種方式實作？
    A. Compiler 呼叫 BIOS
    B. User-mode 呼叫 shell
    C. 呼叫 system call 進入 kernel mode
    D. 將執行權限交給硬碟控制器

17. 哪一項不是 system program 的範例？
    A. Text editor
    B. Command interpreter
    C. File management 工具
    D. Microsoft Excel

18. 為何需要 Dual-Mode 操作模式？
    A. 減少記憶體消耗
    B. 節省輸出裝置時間
    C. 分隔使用者程式與作業系統操作權限
    D. 加快使用者登入速度

19. 作業系統中使用 interrupt 的主要目的為何？
    A. 提供備份支援
    B. 減少網路頻寬
    C. 讓 CPU 能處理突發事件
    D. 儲存使用者指令紀錄

20. Kernel mode 與 user mode 的主要區別是什麼？
    A. Kernel mode 無法執行應用程式
    B. User mode 擁有全部控制權限
    C. Kernel mode 可存取所有系統資源與硬體
    D. User mode 可切換中斷

---

#### 參考答案

> ✅ 選擇題解析區，快速對照與自我檢討。

1. **C**：作業系統的目標是提升效率與便利性，不是提高成本。  
2. **C**：作業系統會扮演資源管理者，負責公平有效率地分配硬體資源。  
3. **B**：命令列與圖形介面屬於便利性功能，協助使用者與系統互動。  
4. **C**：作業系統不會主動提供應用程式開發，僅提供執行環境。  
5. **C**：作業為執行中的程式，並結合必要的系統資源。  
6. **A**：Time-sharing 系統可允許多使用者同時登入並交替使用系統。  
7. **B**：System call 是應用程式進入核心模式的方式，用來執行底層操作。  
8. **B**：像是開檔案、刪檔案這些都屬於 system call 的操作範疇。  
9. **B**：Dual Mode 是 User Mode 與 Kernel Mode，保障系統安全性與穩定性。  
10. **C**：特權指令涉及硬體與系統核心操作，應受限制以防止濫用。  
11. **C**：HTML rendering 與 system call 無關，屬於應用程式層級的行為。  
12. **C**：作業系統核心作為資源管理者，需負責系統的保護與安全。  
13. **B**：虛擬機器模擬不同平台，有助於跨平台開發與隔離運行環境。  
14. **C**：Kernel 可執行特權指令，直接操作硬體；User 程式無此權限。  
15. **B**：追求使用便利性與系統效能常需在設計上取捨平衡。  
16. **C**：需透過 system call 轉入 kernel mode 才能進行硬體相關操作。  
17. **D**：Excel 屬於應用層軟體，非 system program。  
18. **C**：Dual Mode 可保護作業系統不受使用者程式干擾。  
19. **C**：Interrupt 可讓 CPU 中斷正常流程，處理外部事件。  
20. **C**：Kernel mode 擁有存取所有資源與裝置的權限。

---

### 5.2 申論題（共 8 題）

> ✅ 深論題型，用以強化理解作業系統基本原理與設計理念。

1. 請簡述作業系統的三大目標，並分別說明其重要性。
2. 說明為何需要 Dual-Mode Operation，並舉例解釋特權指令與保護機制之間的關聯。
3. 請比較 System Call 與 System Program 的差異與用途。
4. 請解釋作業系統中 Resource Allocator 的功能，並舉例說明其管理的資源種類。
5. 請簡述虛擬機器（Virtual Machine）的概念，並說明其優缺點。
6. 說明 Interrupt 的概念與用途，並舉出三種常見中斷來源。
7. 請說明作業系統為何需要提供 user mode 與 kernel mode 的區分，並說明對系統安全性的影響。
8. 請分析作業與程式的差異，並解釋「作業是一個程式的執行」的意涵。

---

#### 參考答案

> ✅ 申論題參考解析，可用於自我檢查與面試答題練習。

1. **作業系統三大目標：**（1）操作便利性：讓使用者能簡單操作；（2）效率提升：有效管理 CPU、記憶體等資源；（3）資源共享與保護：讓多使用者或程式安全共享系統。  

2. **Dual-Mode Operation 與特權指令關係：** 系統分為 User Mode 與 Kernel Mode。特權指令只能在 Kernel Mode 下執行，以防止使用者程式任意操作硬體。例如：I/O 操作、設定中斷等。  

3. **System Call vs. System Program：** System Call 是作業系統提供的 API，用於進入核心執行底層操作；System Program 是一般系統工具（如編輯器、shell），建構在 system call 之上。  

4. **Resource Allocator 功能：** 管理 CPU、記憶體、I/O 等資源分配，避免資源衝突與浪費。例如：排程器決定哪個 process 使用 CPU。  

5. **虛擬機器（VM）概念與優缺點：** VM 提供模擬硬體平台，讓多個作業系統同時運作。優點：隔離性高、易於測試；缺點：效率較低、需耗費資源。  

6. **Interrupt 功能與來源：** Interrupt 是系統用來處理異常事件的機制。常見來源：鍵盤輸入、硬碟 I/O 完成、系統計時器。  

7. **User Mode 與 Kernel Mode 區分：** 有助於防止使用者程式誤用硬體或操作系統資源，提升系統安全性與穩定性。  

8. **作業 vs. 程式差異：** 程式是靜態指令集合，作業是執行中的程式，結合了程式碼、資料與資源（如暫存器、開啟檔案等），是動態的運行單元。

---

# Ch2：OS Structure

> **主題：作業系統的服務功能、應用介面與系統結構設計**

## 1. 作業系統服務功能（OS Services）

### 使用者介面：CLI 與 GUI  
### 程式執行與 I/O 操作  
### 檔案系統操作與通訊機制  
### 錯誤偵測、資源分配與帳務管理  
### 保護與安全機制（Protection & Security）

## 2. 應用程式介面與系統呼叫（OS-Application Interface）

### 系統呼叫與 API 基礎  
### 常見系統呼叫分類（Process, File, Device, Info, Communication）  
### System Call vs API vs Library  
### System Call 的範例與使用方式  
### 系統呼叫參數傳遞方式（Registers / Table / Stack）

## 3. 作業系統結構設計（OS Structure）

### 使用者目標與系統目標  
### 系統架構類型總覽  
### 單層架構（Simple OS）與階層式架構（Layered）  
### 微核心架構（Microkernel）與模組化架構（Modular）  
### 虛擬機架構（Virtual Machine, JVM）  
### 模擬器與虛擬化應用（Full/Para-Virtualization）

---

# Ch3：Processes

> **主題：程序的基本概念、排程、操作與程序間通訊（IPC）**

## 1. 程序基本概念（Process Concept）

### 程式與程序的區別  
### 程序的記憶體結構（Code, Stack, Heap, Data）  
### Threads 與 CPU 使用單位  
### 程序狀態（New, Ready, Running, Waiting, Terminated）  
### 程序控制區塊（Process Control Block, PCB）  
### Context Switch（上下文切換）

## 2. 程序排程（Process Scheduling）

### 排程概念與多工模式（Multiprogramming / Time-Sharing）  
### 排程隊列：Job Queue、Ready Queue、Device Queue  
### 三種 Scheduler：Long-Term、Short-Term、Medium-Term  
### Ready/Wait 狀態切換與排程動機

## 3. 程序操作（Operations on Processes）

### 程序建立（fork、exec、wait）  
### 記憶體複製與 Copy-on-Write  
### 程序終止與 Parent-Child 關係  
### UNIX/Linux 的程序控制範例  
### Process Tree 與 PID 配置

## 4. 程序間通訊（Interprocess Communication, IPC）

### IPC 目的與分類：獨立 vs 協作程序  
### 溝通方式：Shared Memory vs Message Passing  
### Message Passing 類型：Direct / Indirect  
### Synchronization（阻塞與非阻塞）  
### Socket 與 RPC（遠端程序呼叫）  
### 管道（Pipes）：Ordinary Pipes 與 Named Pipes  
### RMI（Remote Method Invocation）與分散式物件


