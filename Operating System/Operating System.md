# 目錄

## Ch0：Operating System
1. [大型主機系統演進與多工系統](#1-大型主機系統演進與多工系統Mainframe-and-Multiprogramming-Systems)
   - [1.1 大型主機系統與歷史演進](#11-大型主機系統與歷史演進)
   - [1.2 批次系統（Batch System）](#12-批次系統batch-system)
   - [1.3 多重程式系統（Multi-programming System）](#13-多重程式系統multi-programming-system)
   - [1.4 分時系統（Time-Sharing System）](#14-分時系統time-sharing-system)
   - [1.5 系統比較總結](#15-系統比較總結)
2. [電腦系統架構與運算模型](#2-電腦系統架構與運算模型Computer-System-Architectures-and-Models)
   - [2.1 架構類型概觀](#21-電腦系統架構類型概觀)
   - [2.2 平行系統](#22-平行系統parallel-systems)
   - [2.3 記憶體存取架構](#23-記憶體存取架構memory-access-architectures)
   - [2.4 分散式系統架構](#24-分散式系統架構distributed-systems)
   - [2.5 叢集系統（Clustered Systems）](#25-clustered-systems叢集系統)
3. [特殊應用系統架構](#3-特殊應用系統架構Special-Purpose-System-Architectures)
   - [3.1 即時系統](#31-即時系統real-time-systems)
   - [3.2 多媒體系統](#32-多媒體系統multimedia-systems)
   - [3.3 手持與嵌入式系統](#33-手持與嵌入式系統handheld-and-embedded-systems)
   - [3.4 系統選擇與設計思維](#34-系統選擇與設計思維)
4. [作業系統概念統整與測驗練習](#4-作業系統概念統整與測驗練習)
   - [4.1 核心觀念重點整理](#41-核心觀念重點整理)
   - [4.2 選擇題（共 10 題）](#42-選擇題共-10-題)
   - [4.3 申論題（共 8 題）](#43-申論題共-8-題)

## Ch1：Introduction
1. [作業系統基本定義與功能](#1-作業系統基本定義與功能)
   - [1.1 OS 是什麼](#11-os-是什麼)
   - [1.2 OS 的核心功能與運作方式](#12-os-的核心功能與運作方式)
   - [1.3 OS 的設計目標與重要性](#13-os-的設計目標與重要性)
2. [電腦系統組織](#2-電腦系統組織computer-system-organization)
   - [2.1 基本架構與 OS 角色](#21-電腦系統基本架構與作業系統角色)
   - [2.2 I/O 與忙碌等待](#22-io-操作流程與忙碌等待問題)
   - [2.3 中斷驅動 I/O](#23-中斷驅動-io-機制interrupt-driven-io)
3. [儲存階層與快取管理](#3-儲存階層與快取管理storage-hierarchy-and-cache-management)
   - [3.1 儲存設備與階層設計](#31-儲存設備層級與設計原則)
   - [3.2 RAM 類型](#32-ram-類型與存取特性)
   - [3.3 磁碟效能](#33-磁碟機運作與效能影響)
   - [3.4 Cache 機制](#34-快取記憶體與效能提升)
   - [3.5 Cache 一致性問題](#35-一致性問題與分散式挑戰)
4. [硬體保護與執行模式](#4-硬體保護與執行模式hardware-protection-and-execution-modes)
   - [4.1 系統保護需求](#41-系統保護的概念與需求)
   - [4.2 Dual-Mode 操作](#42-雙模式操作dual-mode-operation)
   - [4.3 I/O 保護與防範](#43-io-保護與惡意操作防範)
   - [4.4 記憶體保護](#44-記憶體保護memory-protection)
   - [4.5 CPU 保護與 Timer](#45-cpu-保護與-timer-機制)
5. [作業系統與硬體互動總結與測驗練習](#5-作業系統與硬體互動總結與測驗練習)
   - [5.1 核心觀念重點整理](#51-核心觀念重點整理)
   - [5.2 選擇題](#52-選擇題共-20-題)
   - [5.3 申論題](#53-申論題共-8-題)

## Ch2：OS Structure

1. [作業系統服務功能](#1-作業系統服務功能os-services)
   - [1.1 使用者導向的核心服務](#11-使用者導向的核心服務)
   - [1.2 系統導向的管理服務](#12-系統導向的管理服務)
   - [1.3 使用者介面](#13-user-interface使用者介面)
   - [1.4 通訊模式](#14-communication通訊模式)
2. [應用程式介面與系統呼叫（OS-Application Interface）](#2-應用程式介面與系統呼叫os-application-interface)
   - [2.1 System Call 與 OS 的介面](#21-system-call-與-os-的介面)
   - [2.2 API：應用程式介面與函式庫](#22-api應用程式介面與函式庫)
   - [2.3 API 標準與虛擬機支援](#23-api-標準與虛擬機支援)
   - [2.4 使用 API 的三大好處](#24-使用-api-的三大好處)
   - [2.5 System Call 的參數傳遞方式](#25-system-call-的參數傳遞方式)
3. [作業系統結構設計](#3-作業系統結構設計os-structure)
   - [3.1 系統設計目標與思維差異](#31-系統設計目標與思維差異)
   - [3.2 作業系統的四種典型架構](#32-作業系統的四種典型架構)
   - [3.3 虛擬機設計與執行方式](#33-虛擬機設計與執行方式)
4. [作業系統結構練習題](#4-作業系統結構練習題)
   - [4.1 選擇題](#41-選擇題共-20-題)
   - [4.2 申論題](#42-申論題共-8-題)

## Ch3：Processes

1. [程序基本概念（Process Concept）](#1-程序基本概念process-concept)
   - [1.1 程序的定義與組成](#11-程序的定義與組成)
   - [1.2 作業的狀態與生命週期](#12-作業的狀態與生命週期)
   - [1.3 Process Control Block（PCB）](#13-process-control-blockpcb)
   - [1.4 Process 表示與 Context Switch](#14-process-表示與-context-switch)
   - [1.5 程序創建與終止](#15-程序創建與終止)
2. [程序排程（Process Scheduling）](#2-程序排程process-scheduling)
   - [2.1 排程時機與分類](#21-排程時機與分類)
   - [2.2 Dispatcher（派遣器）](#22-dispatcher派遣器)
   - [2.3 排程準則（Scheduling Criteria）](#23-排程準則scheduling-criteria)
   - [2.4 排程演算法（Scheduling Algorithms）](#24-排程演算法scheduling-algorithms)
   - [2.5 排程演算法比較（Scheduling Comparison）](#25-排程演算法比較scheduling-comparison)
3. [程序操作（Operations on Processes）](#3-程序操作operations-on-processes)
   - [3.1 Process Creation（程序建立）](#31-process-creation程序建立)
   - [3.2 Process Termination（程序終止）](#32-process-termination程序終止)
   - [3.3 程序間的關係模型](#33-程序間的關係模型)
   - [3.4 程序識別與控制](#34-程序識別與控制)
4. [程序間通訊（Interprocess Communication, IPC）](#4-程序間通訊interprocess-communication-ipc)
   - [4.1 IPC 的基本概念與目的](#41-ipc-的基本概念與目的)
   - [4.2 IPC 的主要模型](#42-ipc-的主要模型)
   - [4.3 Direct 與 Indirect Communication](#43-direct-與-indirect-communication)
   - [4.4 Synchronization（同步）與 Buffering（緩衝區）](#44-synchronization同步與-buffering緩衝區)
   - [4.5 IPC 實務範例：Linux 中的 IPC](#45-ipc-實務範例linux-中的-ipc)
   - [4.6 典型使用場景比較](#46-典型使用場景比較)
5. [程序與通訊練習題](#5-程序與通訊練習題)
   - [5.1 選擇題（共 20 題）](#51-選擇題共-20-題)
   - [5.2 申論題（共 8 題）](#52-申論題共-8-題)

## 作業

1. [OS 作業一：NACHOS 架構與實作指引](#os-作業一nachos-架構與實作指引)

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

| 系統類型                  | 執行模式       | 系統目標      | OS 功能涵蓋                          | 現代是否仍在使用？      | 現代對應情境                              |
| --------------------- | ---------- | --------- | -------------------------------- | -------------- | ----------------------------------- |
| **Batch**             | 單一使用者、單一工作 | 簡化控制流程    | 無明確 OS 功能，作業一次送出，執行完再看結果         | ✅ 少數主機仍使用      | 批次金融處理、大型資料報表（如 COBOL on Mainframe） |
| **Multi-programming** | 多個程式共存於記憶體 | 資源使用率最大化  | 記憶體管理、CPU 排程、I/O 處理與 Spooling 技術 | ✅ 現代 OS 核心機制之一 | 所有作業系統（Linux、Windows、macOS）         |
| **Time-sharing**      | 多使用者、多程式   | 提升即時互動與反應 | 加入虛擬記憶體、檔案系統、多工支援、同步與死結處理        | ✅ 現代主流架構       | 桌面系統、伺服器、多使用者登入環境等                  |

#### ▸ 補充說明

- **Batch System**：
  - 適合無需互動、計算時間長的任務（如薪資計算、稅務報表）。
  - 效率低、回應慢，但在特定產業中仍有需求（如 IBM z 系列主機）。

- **Multi-programming System**：
  - 現代所有作業系統都具備此技術。
  - 即便是單一使用者系統（如個人電腦），背景也會同時執行許多程式（防毒、音樂播放器、瀏覽器等）。

- **Time-sharing System**：
  - 對應到現在的「多使用者作業系統」（multi-user OS）。
  - 如：一台 Linux 伺服器可同時讓多位使用者透過 SSH 登入並執行不同任務。

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

- 又稱為 **Tightly Coupled Systems**，具備兩個以上 CPU，彼此透過共享記憶體進行通訊。
- 優點包括：
  - **高吞吐量（Throughput）**：可同時處理更多任務。
  - **成本效益高（Economical）**：相較多台電腦，系統整合成本更低。
  - **可靠性高（Reliability）**：部分 CPU 故障仍可維持系統運作。

---

#### ▸ 架構類型比較

- **SMP（Symmetric Multiprocessor）**：每顆 CPU 地位相同，執行同一個作業系統。所有 CPU 共用排程器與記憶體，需使用同步機制來確保資料一致性。
- **AMP（Asymmetric Multiprocessor）**：主從式架構，由 Master CPU 分派工作，其餘 CPU 執行子任務。各 CPU 可執行不同作業系統或任務，互不干擾，適合任務明確分工的系統。

---

#### ▸ 架構應用比較表

| 架構類型                                | 是否主流應用               | 常見應用場景                             | 說明                                             |
|-----------------------------------------|----------------------------|------------------------------------------|--------------------------------------------------|
| **AMP（Asymmetric Multiprocessing）**   | ✅ 高異質性嵌入式系統主流     | MCU + DSP、ARM + FPGA、車載 ECU         | 適合異質核心，各 CPU 可執行不同任務或作業系統       |
| **SMP（Symmetric Multiprocessing）**    | ✅ 多核心 SoC 架構主流       | 高階 ARM SoC（如手機、Raspberry Pi）    | 適合核心等同、需共享排程與記憶體的多工環境         |

---

#### ▸ 補充說明

SMP 架構著重於**多工排程與使用者體驗**。像我們使用手機或電腦時，可以同時看影片、滑 IG、背景下載，系統會盡量讓你感覺不到延遲（delay）。這是因為 SMP 會平均分配任務給每個核心，讓效能發揮到最大，因此手機與 PC 幾乎都是採用 SMP 架構。

但嵌入式系統的重點不同，更重視**即時性（Real-time Determinism）**。舉例來說，像伺服馬達控制或感測器讀值，只要延遲幾毫秒就可能造成控制失效。因此我們會選用 AMP 架構，讓每個核心可以各自執行專屬任務互不干擾。例如：
- 一個核心專門負責控制演算法運算（如 PID）。
- 另一個核心處理螢幕顯示或通訊。

這樣就算畫面 lag，也不會影響核心控制邏輯，系統穩定性與即時性都能兼顧。

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
- 常見於 **SMP 系統**，所有 CPU 存取主記憶體的延遲相同。
- 優點：系統設計簡單，任務可自由分派給任一 CPU。
- 缺點：多個 CPU 同時存取時容易造成記憶體瓶頸（bandwidth contention）。

#### ▸ NUMA（Non-Uniform Memory Access）
- 常見於 **AMP 系統**，每個節點（node）包含自己的 CPU 和本地記憶體。
- CPU 存取本地記憶體較快，跨節點存取則需透過互連通道，延遲較高。
- 常見於高階伺服器與異質多核心嵌入式系統（如 Linux + RTOS），藉由獨立記憶體實現資源隔離與穩定性。
- 缺點：需注意資料配置與核心綁定，否則頻繁跨節點存取會影響效能。

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
- **Client-Server 架構**：系統分為服務端（Server）與請求端（Client），資源集中由 Server 管理，便於控管與安全設計。
  - 優點：集中管理容易，適合大型企業系統。
  - 缺點：Server 易成效能瓶頸，存在單點失效風險。
  - 應用範例：網站服務（Web Server）、雲端平台（如 Google Cloud）、Email 系統（SMTP/IMAP）。
- **Peer-to-Peer 架構（P2P）**：所有節點地位平等，無中央控制。節點可互為 Server 與 Client，系統具高度可靠性與擴展性。
  - 優點：無單點故障，動態彈性佳。
  - 缺點：管理與安全性較困難。
  - 應用範例：檔案共享（如 BitTorrent）、區塊鏈平台（如 Ethereum）、去中心化應用（Web3）。

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
- **Real-Time 並不代表執行速度快**，而是指系統**必須在預期時間內完成任務（符合 deadline）**。
- 廣泛應用於：工業控制、醫療影像、科學實驗、武器系統等。
- 系統需具備可預測的反應與回應時間，排程器（scheduler）是實現即時性的關鍵元件。

#### ▸ 系統分類
- **Hard Real-Time System**（強即時系統）：
  - 對於系統來說，每個任務的 **deadline（截止時間）都必須準時完成**。
  - **若錯過 deadline，系統會產生嚴重錯誤或整體失效**（如：核能電廠控制系統、飛彈導航）。
  - 為了保證即時性，這類系統通常**不使用硬碟等慢速的 secondary storage**，資料會儲存在速度較快且延遲可預測的 memory 或 ROM 中。

- **Soft Real-Time System**（弱即時系統）：
  - 系統也會嘗試在 deadline 前完成任務，但**偶爾延遲是可以接受的**，不會造成系統崩潰，只是效能或使用者體驗下降。
  - 常見於多媒體應用，如影音串流畫面延遲或畫質暫降。
  - 透過 **priority-based scheduling（優先權排程）** 確保關鍵任務先處理。

#### ▸ 系統設計挑戰
- 核心挑戰在於如何設計：
  - **即時排程演算法**（如 Earliest Deadline First, EDF）
  - **記憶體與儲存策略**：避免使用慢速儲存裝置，確保時間可預測性。
  - **資源配置策略**：確保關鍵任務獲得足夠 CPU 資源以達成即時反應。

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

## 4. 作業系統概念統整與測驗練習

---

### 4.1 核心觀念重點整理

1. **Multi-programming System（多重程式系統）**
- 多個程式可同時駐留主記憶體。
- 作業系統透過記憶體管理、CPU 排程、I/O 管理與 Spooling 技術來管理程式切換。
- 可在一個程式等待 I/O 時執行其他程式，提升 CPU 使用率、減少 idle time。
- 為現代作業系統的基本機制。

---

2. **Time-Sharing System（分時系統）**
- 支援多位使用者同時互動。
- 利用時間片（time slice）快速切換 CPU，讓使用者感覺像獨佔系統。
- 達成即時互動的效果。

---

3. **Multiprocessor System（多處理器系統）**
- 系統中具備兩顆以上 CPU，透過共享記憶體協作運作。
- 架構類型：
  - **SMP（Symmetric Multiprocessing）**：CPU 地位對等，共享 OS 與記憶體，常見於桌機與伺服器。
  - **AMP（Asymmetric Multiprocessing）**：主從式分工，主 CPU 管理任務，子 CPU 執行任務。常見於嵌入式系統（如 Linux + RTOS）。

---

4. **記憶體存取架構（Memory Access Architectures）**
- **UMA（Uniform Memory Access）**
  - 所有 CPU 存取主記憶體的延遲相同。
  - 常見於 SMP 系統。
  - 優點：架構簡單。
  - 缺點：多核心競爭記憶體會造成瓶頸。

- **NUMA（Non-Uniform Memory Access）**
  - 每個節點（node）擁有獨立的 CPU 與本地記憶體。
  - 跨節點存取延遲較高，需透過互連通道。
  - 應用於 AMP 系統或大型 SMP 系統（如伺服器）。
  - 優點：資源隔離、效能擴展。
  - 缺點：需注意資料與核心綁定。

---

5. **Multi-core vs Multiprocessor vs SMP（常見混淆）**

- **Multiprocessor**：系統中安裝多顆獨立 CPU（如雙插槽伺服器）。
- **Multi-core**：一顆 CPU 晶片內整合多個核心（cores）。

---

**以 TI AM64x（例如 AM6442）為例說明：**

| 核心類型                                 | 數量             | 用途                                | 架構              |
|------------------------------------------|------------------|-------------------------------------|-------------------|
| **Cortex-A53**                           | 2                | 跑 Linux（應用層）                   | 支援 SMP          |
| **Cortex-R5F**                           | 2–4              | 控制／即時處理（RTOS 或 bare-metal） | 可組 AMP          |
| **MCU Subsystem（含 1 顆 R5F）**         | 1                | 負責開機流程與安全監控               | 通常跑 bootloader |
| **PRU（Programmable Real-Time Unit）**   | 2                | 精準 I/O 控制／使用者自定義邏輯        | 獨立子系統        |

---

**對應作業系統名詞如下：**

| 作業系統名詞                             | 在 AM64x 上的具體對應說明                                                                 |
|------------------------------------------|--------------------------------------------------------------------------------------------|
| **Multiprocessor System**                | 若與另一顆 SoC（如外部 MCU）協同運作，可稱為 Multiprocessor System                         |
| **Multi-core**                           | 單一 AM64x SoC 上整合多種核心（A53、R5F、PRU）                                             |
| **SMP（Symmetric Multiprocessing）**     | A53 雙核心共用記憶體並執行同一作業系統（Linux）→ 屬於 SMP 架構                            |
| **AMP（Asymmetric Multiprocessing）**    | A53 跑 Linux、R5F 跑 RTOS（如 FreeRTOS）→ 為 AMP 應用架構                                 |
| **Multi-programming System**             | A53 跑 Linux 時，OS 同時載入多個程式並透過排程器管理切換 → 實現 Multi-programming           |
| **Time-Sharing System**                  | A53 的 Linux 以時間片切換使用者程序（如 terminal + background download）→ 實現 Time-sharing |
| **UMA / NUMA**                           | AM64x 通常為 UMA（共享記憶體），若手動隔離記憶體分區，也可模擬 NUMA 行為                   |
| **Real-Time System**                     | R5F 核心執行 RTOS 或 bare-metal，即為 Real-Time 系統；亦可配置為 Hard Real-Time 環境         |

---

6. **Real-Time Systems（即時系統）**
- Real-Time 強調的是「在可預測時間內完成任務」，非速度快。
- 常應用於工控、醫療、飛控、軍事等領域。
- 排程器（scheduler）為核心設計挑戰之一。

**系統分類：**
- **Hard Real-Time**
  - 任務不能遲到，否則系統失效。
  - 不使用硬碟等慢速裝置，改用記憶體或 ROM。
  - 例：核能控制、飛彈導航。

- **Soft Real-Time**
  - 遲到可接受但會影響體驗。
  - 常用於多媒體應用（如影音串流）。
  - 採用優先權排程（priority-based scheduling）。

---

7. **Distributed System vs Clustered System**
- **Distributed System（分散式系統）**
  - 節點鬆散耦合，透過網路協作。
  - 節點獨立管理，對外呈現多系統。
  - 範例：P2P、IoT、區塊鏈。

- **Clustered System（叢集系統）**
  - 節點部署於 LAN，緊密耦合。
  - 協同提供單一服務，對外呈現單一系統。
  - 分為 Symmetric / Asymmetric Cluster。
  - 應用於 HPC、高可用伺服器等場景。

---

### 4.2 選擇題（共 10 題）

1. Multi-programming 技術的主要目的是？
   A. 提高 GPU 運算效能
   B. 增加主記憶體容量
   C. 提高 CPU 使用率
   D. 支援即時互動

2. 下列哪一個敘述最能描述 Time-Sharing 系統？
   A. 同時支援多顆處理器
   B. 系統一次只能執行一個工作
   C. 多個使用者可即時互動
   D. 系統需滿足嚴格 deadline

3. 在 SMP 架構中，哪一項為正確描述？
   A. 每個 CPU 執行不同作業系統
   B. CPU 間資源獨立，不互相干擾
   C. 所有 CPU 執行同一作業系統並共享記憶體
   D. 僅適用於單一核心處理器

4. NUMA 架構的主要挑戰是什麼？
   A. CPU 數量限制
   B. 記憶體存取延遲不一致
   C. GPU 效能不足
   D. 無法支援虛擬記憶體

5. 在 AM64x 晶片中，將 A53 跑 Linux、R5F 跑 RTOS 的系統架構稱為？
   A. SMP
   B. AMP
   C. NUMA
   D. UMA

6. 下列何者不屬於 Multi-core 晶片的優點？
   A. 節省空間
   B. 核心間支援資料共享
   C. 每顆核心皆執行不同 OS
   D. 提升整體處理效能

7. Real-Time 系統的最大特點是什麼？
   A. 執行速度快
   B. 可隨意延遲任務
   C. 預測性與即時回應能力
   D. 支援多個使用者同時登入

8. 硬即時系統（Hard Real-Time）的應用最可能出現在哪裡？
   A. 影片播放平台
   B. 核能電廠控制系統
   C. 一般文書處理系統
   D. 社群媒體伺服器

9. 下列關於 UMA 的敘述何者為真？
   A. 每個 CPU 使用不同作業系統
   B. 每個節點皆有獨立記憶體
   C. CPU 存取記憶體延遲一致
   D. 僅能應用於嵌入式系統

10. 使用 AM64x 時，哪一種記憶體架構最可能出現？
    A. UMA
    B. NUMA
    C. Distributed RAM
    D. Cache-only model
---

以下是配合你剛才重新命題的選擇題所對應的答案區，格式與你提供的一致：

---

#### 參考答案

1. **C**：Multi-programming 的主要目的在於提升 CPU 使用率，減少 idle time。
2. **C**：Time-Sharing 允許多位使用者即時互動，是現代多使用者系統的基礎。
3. **C**：SMP 架構中，所有 CPU 共用 OS 且共享記憶體與排程器。
4. **B**：NUMA 的最大挑戰是不同節點間存取記憶體延遲不同，需謹慎設計排程與資料配置。
5. **B**：A53 跑 Linux、R5F 跑 RTOS 的組合屬於 AMP（異質對稱多工）。
6. **C**：Multi-core 晶片內核心共享同一作業系統，不會各自執行不同 OS。
7. **C**：Real-Time 系統的關鍵在於「反應時間可預測」，非純粹運算速度。
8. **B**：核能電廠控制系統需嚴格即時性，典型 Hard Real-Time 應用。
9. **C**：UMA（均勻記憶體存取）架構的特點是所有 CPU 存取延遲一致。
10. **A**：AM64x 晶片內部核心多共享相同 DRAM，預設為 UMA 架構。

---

### 4.3 申論題（共 8 題）

1. 請說明 Multi-programming 與 Time-sharing 系統在系統資源配置、CPU 使用率與使用者互動體驗上的不同，並舉例說明在嵌入式系統中是否適合使用 Time-sharing。

2. 請比較 SMP（Symmetric Multiprocessing）與 AMP（Asymmetric Multiprocessing）在系統設計與應用場景上的差異，並說明你所在專案中是否遇過 AMP 架構的實務情境。

3. NUMA 與 UMA 架構在作業系統開發與排程策略上有何不同？請說明在韌體開發中如何因應 NUMA 帶來的資料延遲問題。

4. 請解釋 Multi-core、Multiprocessor 與 SMP 的概念差異，並以 TI AM64x 晶片為例，說明它們在實務中的應用與關聯性。

5. 為何 Real-Time System 不等於運作速度快？請比較 Hard Real-Time 與 Soft Real-Time 系統在記憶體使用、I/O 管理與排程策略上的設計差異。

6. 當你設計一個 LCD 顯示與按鍵輸入的控制邏輯時，如何透過 Linux Thread 與 Shared Memory 與 RTOS 協作？請說明其中涉及的 IPC 技術與可能的同步問題。

7. 分散式系統（Distributed System）與叢集系統（Clustered System）有何本質差異？請說明各自的應用優勢，並指出其在高可靠性系統中的角色。

8. 若你的系統需同時支援 Linux + RTOS（如 A53 跑 Linux，R5F 跑 FreeRTOS），請描述如何設計有效的資料交握（Handshaking）與通訊架構，並說明相關的系統挑戰與對策。

---

#### 參考答案

1. **Multi-programming 與 Time-sharing 系統比較**
   Multi-programming 透過讓多個程式同時駐留記憶體並由作業系統排程切換，以提高 CPU 使用率；而 Time-sharing 則引入時間片（time slice）與使用者互動，讓多位使用者能感受到「同時操作」的體驗。嵌入式系統若以即時性為主，較少使用 Time-sharing，但若具備人機互動需求（如 UI 顯示），則可在上層使用 Time-sharing 架構。

2. **SMP 與 AMP 的實務比較與應用場景**
   SMP 中所有 CPU 共用作業系統與記憶體，適合多核心均等任務的系統；AMP 則將任務分配給不同角色的 CPU，各自執行獨立作業系統或控制邏輯。在我所參與的 AM64x 專案中，A53 核心執行 Linux，R5F 執行 RTOS，屬典型 AMP 架構，彼此透過 shared memory 與 IPC 協作。

3. **NUMA 與 UMA 在排程與資源配置上的挑戰**
   UMA 架構下所有核心存取主記憶體延遲一致，方便任務動態分配。NUMA 則需考慮核心與記憶體的對應關係，避免跨節點存取延遲過高。OS 若未妥善綁定資料與對應核心，會造成效能下降。在韌體設計上應儘量區隔資料區域並固定排程。

4. **Multi-core、Multiprocessor 與 SMP 在 AM64x 上的應用**
   AM64x 為 Multi-core SoC，內含多種處理核心；若與外部 MCU 協同運作則為 Multiprocessor 系統。兩顆 A53 共用 Linux 與記憶體，即構成 SMP 架構。不同概念適用於不同層級的系統整合說明。

5. **Real-Time 不等於快；Hard vs Soft RT 的設計差異**
   Real-Time 的核心是「可預測性」與「符合 deadline」。Hard RT 不容許任何任務延遲，需使用 ROM 或 SRAM 儲存、靜態配置記憶體與使用高優先排程；Soft RT 容許偶爾延遲，適合影音處理或 UI 顯示，採較具彈性的排程方式。

6. **Linux Thread 與 RTOS 的協作架構與挑戰**
   典型作法是 A53 使用 thread 掃描按鍵與更新畫面，R5F 使用 RTOS 控制感測回授並透過 shared memory 傳送結果。需注意同步與 race condition 問題，可透過 lock、flag 或 double buffer 實現資料一致性與穩定性。

7. **Distributed vs Clustered 系統差異與應用**
   Distributed 系統節點鬆散耦合，彼此獨立，常見於 IoT、P2P、區塊鏈；Clustered 系統部署於同一 LAN，協作提供單一服務，應用於 HPC 與高可用伺服器。前者具動態彈性與可靠性，後者重效能整合與集中管理。

8. **Linux + RTOS 的 IPC 設計與挑戰**
   當 A53 跑 Linux，R5F 跑 RTOS 時，資料交握需透過 shared memory、mailbox 或 RPMsg 等方式進行。挑戰包含資料一致性、記憶體映射正確性、同步機制選擇與除錯困難。建議使用封包格式定義、cache flush 與中斷通知等方式強化穩定性。

---

# Ch1：Introduction

> **主題：作業系統的基本概念與硬體保護機制**

---

## 1. 作業系統基本定義與功能

---

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

#### ▸ 裝置控制器（Device Controller）
- 一般為獨立晶片，位於 **CPU 與外部裝置（Device）之間**，負責資料傳輸與狀態管理。
- 每個控制器專責一種特定類型的裝置，並內建 **本地緩衝區（local buffer）**。
- 控制器內通常包含兩個主要暫存器：
  - **Status Register**：回報裝置目前狀態（如 idle、busy、error）。
  - **Data Register**：作為暫存區，暫時存放即將傳輸或剛接收的資料。

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
你的敘述基本正確，以下是幫你修飾後、語意更清晰並符合系統層級用語的版本：

#### ▸ 中斷處理流程

1. **Device Driver** 透過 **System Call** 向作業系統發出 I/O 請求。
2. 作業系統將命令傳給 **Device Controller**，控制器開始初始化並執行實際的資料傳輸（通常傳至其 **local buffer**）。
3. 傳輸完成後，控制器發出 **中斷訊號（Interrupt Signal）** 給 CPU。
4. CPU 收到中斷後，暫停當前執行流程，切換至對應的 **Interrupt Handler（中斷處理常式）**。
5. Interrupt Handler 處理完畢後，CPU 透過 **中斷返回（Interrupt Return）** 回到原本被中斷的程式繼續執行。

#### ▸ 中斷向量表（Interrupt Vector）
- 作業系統維護一張 **中斷向量表（Interrupt Vector Table）**，實為一組 function pointers 陣列，每個欄位對應一個中斷號（signal number）及其對應的 **Interrupt Handler（中斷處理常式）**。
- 當中斷發生時，系統根據中斷號查表，呼叫對應的 **Service Routine**（負責處理該中斷的程式碼）。
- 當安裝新裝置並載入對應 driver 時，作業系統會同時更新向量表中對應的 function pointer，更新向量表中的對應中斷處理程序，讓新的 handler 接手該中斷號的處理。

#### ▸ 硬體中斷與軟體中斷

- **硬體中斷（Hardware Interrupt）**：由外部裝置（如鍵盤、滑鼠、網卡）觸發，透過中斷向量表查找對應的 handler 進行處理。每個裝置驅動程式（driver）會註冊中斷號與對應的 Service Routine。

- **軟體中斷（Software Interrupt / Trap）**：由程式主動或錯誤行為觸發。
  - **System Call**：像 `printf()` 這類呼叫，其實是透過 trap 切換控制權給 OS。
  - **Exception / Fault**：例如除以 0、記憶體錯誤等，程式無法繼續執行，OS 接手處理（如顯示 "Segmentation fault"）。

- OS 會根據中斷來源使用不同處理流程：
  - **硬體中斷**：查中斷向量表。
  - **軟體中斷**：由 OS 自訂的分派機制（如 switch-case）選擇對應處理程序。

> 📌 軟體中斷稱為 **trap**，表示 CPU 主動「陷入」OS，由 OS 接管控制權，處理 system call 或例外錯誤。

#### ▸ 中斷的同步與保護機制
- 當系統中有多個程式並行執行且中斷頻繁發生時，容易產生同步問題。為了避免 lost interrupt 或系統狀態錯亂，作業系統通常會採取以下保護措施：
  - 使用 **Interrupt Masking**：在處理高優先權中斷時，暫時屏蔽低優先權中斷，避免處理流程被干擾。
  - 使用 **快速、精簡的中斷服務常式（Service Routine）**，多以組合語言實作以確保執行速度極快。
- 當中斷發生時，需要儲存大量上下文（context saving），會帶來額外負擔（overhead）系統必須立即保存當前執行狀態，包括 **暫存器（Register）**、**程式計數器（PC）**、**關鍵記憶體內容**，以便中斷處理完成後，能安全返回原本的執行點，確保程式不中斷地繼續執行。

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
- 每個 bit 由一個電晶體與電容構成，體積小、成本低、功耗低，但速度較慢。
- 廣泛用於 **主記憶體（Main Memory）**。

#### ▸ SRAM（Static RAM）
- 每個 bit 需使用六個電晶體構成，速度快但體積大、成本高、耗電較高。
- 常用於 **快取記憶體（Cache）**，提供高速存取以提升系統效能。

#### ▸ 隨機存取特性（Random Access）
- RAM 屬於 **隨機存取記憶體**，無論存取哪個位址，其延遲時間基本一致。
- 有助於確保程式執行時的穩定性與預測性，與硬碟等順序存取裝置不同，後者存取延遲會依資料位置而異。

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
- 快取是一種 **暫存常用資料** 的高速儲存空間。
- CPU 存取資料時會先查找最近的快取層級（如 L1、L2、L3），若資料存在（**cache hit**），可快速取出；若不存在（**cache miss**），則需往主記憶體甚至磁碟等較慢層級查找。
- 系統會自動將常用資料從慢速儲存暫存到高速層級，以提升整體運算效能。

#### ▸ 區域性（Locality）特性
- Cache 效能提升依賴「**區域性原則（Locality of Reference）**」：
  - **時間區域性**：剛用過的資料很快還會再用。
  - **空間區域性**：鄰近的資料很可能被用到。
- 某些情境下不建議使用 Cache，例如：
  - 巨量資料處理（Big Data Processing）僅掃描一次、不重複使用。
  - 資料量大到連主記憶體都無法容納。
- 此時使用快取反而增加負擔，故資料密集型系統（Data-Intensive Systems）常直接繞過 Cache 設計。

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
- 實現保護機制的基礎在於硬體支援，因為硬體是無法被修改的，不像軟體可能被惡意程式竄改，例如模式切換與指令限制。

---

### 4.2 雙模式操作（Dual-Mode Operation）

#### ▸ 執行模式的區分
- 現代作業系統透過硬體中的 mode bit（模式位元，存在某個狀態暫存器中）來區分執行權限：
  - `Mode bit = 1`：**User Mode**，用於執行使用者層程式，權限受限。
  - `Mode bit = 0`：**Kernel Mode（又稱 Monitor Mode）**，用於執行作業系統核心程式，擁有完整控制權。

#### ▸ 模式切換與中斷
- 當發生 **System Call** 或 **Interrupt** 時，系統會自動將 mode bit 從 1 切換為 0，進入 Kernel Mode。
- 作業系統處理完畢後，會將 mode bit 切回 1，返回使用者程式繼續執行。

#### ▸ 特權指令（Privileged Instructions）
- 特權指令是被硬體架構設計為只能在 Kernel Mode 下執行的操作（如 x86 架構）。
- 常見指令包括：I/O 裝置操作、修改中斷向量表、設定記憶體保護等。
- 若在 User Mode 嘗試執行特權指令，將會觸發中斷（trap），由作業系統介入處理，通常終止該程式以保護系統安全。

---

### 4.3 I/O 保護與惡意操作防範

#### ▸ 為何需要 I/O 保護
- I/O 裝置（如鍵盤、滑鼠、硬碟）為多個程序共享的系統資源，若使用者程式能任意存取，將可能造成資料衝突、裝置混亂，甚至導致系統當機。
- 為避免此情形，**所有 I/O 指令皆被設計為特權指令（Privileged Instructions）**，只能在 Kernel Mode 由作業系統執行，使用者程式無法直接操作。

#### ▸ 潛在風險與攻擊方式
- 雖然特權指令保護了 I/O 操作，但若 **記憶體保護機制不足**，仍可能遭惡意程式繞過。
- 例如，若使用者可任意存取記憶體，就可能修改 **中斷向量表（Interrupt Vector Table）**，使硬體中斷跳至惡意程式碼執行。
- 常見攻擊手法包含：
  - 改寫驅動程式邏輯。
  - 篡改中斷處理程序流程。
  - 透過共享 I/O 線路發送惡意資料。
- 因此，**記憶體保護與 I/O 特權限制**需同時並存，才可確保系統整體安全性。

---

#### ▸ 保護哪些內容
- 關鍵結構需受到保護，例如：
  - **中斷向量表（Interrupt Vector）**
  - **中斷服務程式（Interrupt Service Routine, ISR）**
  - **作業系統的資料與程式區**
- 避免使用者程式任意存取或修改不屬於自身的記憶體區段。

#### ▸ 硬體支援機制
- 現代硬體會搭配兩個重要暫存器來保護記憶體範圍：
  - **Base Register**：記錄可存取記憶體的起始位址。
  - **Limit Register**：記錄從 base 起可存取的最大位移範圍。
- 每次存取都會比對是否落在合法範圍，否則觸發錯誤（如 segmentation fault）。

#### ▸ 保護的實際效果
- 每個程式只能操作自己的記憶體，避免讀取他人資料或干擾系統結構。
- Base 與 Limit register 的設定只能透過 **Privileged Instruction（特權指令）** 修改。
  - 例如：當程式呼叫 `malloc()` 動態配置記憶體時，實際上是由作業系統修改對應的 limit，讓程式能合法使用更多空間。
- 所有記憶體管理操作須經過 OS 核可。

---

### 4.5 CPU 保護與 Timer 機制

#### ▸ 為什麼需要 CPU 保護
- 若單一程式無限制佔用 CPU，其他程式將無法執行，造成資源壟斷與死當風險。
- 舉例：當程式進入 `while(1)` 的無限迴圈，系統仍可正常運作，代表作業系統保有 CPU 控制權。
- 透過保護機制，OS 能強制回收 CPU，避免任一程式壟斷資源。

#### ▸ Timer 機制的應用
- 作業系統會設置一個 **計時器（Timer）**，每經過固定時間產生一次 **中斷（Interrupt）**。
- 當中斷觸發時，CPU 會跳離當前程式，轉由 OS 排程下一個程序，實現輪流執行。

#### ▸ 中斷與程序切換
- Timer 中斷是實現 **Context Switch（上下文切換）** 的契機。
- 可用來實作 **Time-Sharing（分時機制）**，確保多個程序能公平地使用 CPU，防止無限迴圈佔用資源。

---

## 5. 作業系統與硬體互動總結與測驗練習

---

### 5.1 核心觀念重點整理

1. **作業系統角色與 API 抽象化**
- OS 提供應用程式與硬體之間的抽象層，使程式無需直接操作硬體。
- 主要職責包含：
  - Resource Allocator：管理 CPU、memory、I/O 等資源。
  - Control Program：管控程式與硬體操作，避免錯誤干擾。
  - Coordinator：協調多程序、多使用者間的競爭。
  - Interface Provider：提供 System Call 介面操作底層資源。

---

2. **執行模式與特權指令**
- OS 透過 mode bit 區分執行模式：
  - Mode bit = 1：User Mode，僅限執行非特權操作。
  - Mode bit = 0：Kernel Mode，可執行特權指令。
- 特權指令（如記憶體設定、I/O 操作）僅能於 Kernel Mode 執行，否則觸發 Trap。

---

3. **裝置控制與中斷處理流程**
- Device Controller 位於 CPU 與外部裝置之間，負責資料傳輸與狀態管理。
- 控制器內含：
  - Status Register：裝置狀態回報。
  - Data Register：暫存資料。
- 中斷處理流程：
  1. Device Driver 發出 I/O 要求。
  2. Controller 傳輸完成後發出 Interrupt。
  3. CPU 切入中斷處理，執行對應 Handler。
  4. 處理完畢後恢復原程式執行。

---

4. **中斷向量表與 Trap 機制**
- 中斷向量表是由 function pointer 組成的陣列，根據中斷號觸發對應 Handler。
- 軟體中斷（Software Interrupt）又稱 Trap，通常由 system call 或 exception 觸發，由 OS 負責處理。

---

5. **中斷同步與保護措施**
- 為避免系統狀態錯亂與遺失中斷，OS 採取：
  - Interrupt Masking：暫時屏蔽低優先權中斷。
  - 簡化中斷處理常式。
  - Context Saving：儲存當前執行狀態，便於後續恢復。

---

6. **記憶體與儲存層級架構**
- 儲存採階層架構：register → cache → DRAM → SSD/HDD → tape。
- DRAM 用於主記憶體，速度較慢但容量大；SRAM 用於 cache，速度快但體積大。
- RAM 屬於隨機存取，存取延遲一致，利於效能預測。

---

7. **Cache 概念與一致性挑戰**
- Cache 用於暫存常用資料，提升資料存取速度。
- 多核心系統需注意 Cache Coherency 問題，避免資料不一致，常用 MESI 協定解決。

---

8. **CPU 保護與 Timer 中斷**
- 為防止程序無限佔用 CPU，OS 利用 Timer 產生中斷進行排程切換。
- Timer 中斷為 time-sharing 實作的核心機制，實現程序公平與穩定執行。

---

### 5.2 選擇題（共 20 題）

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

### 5.3 申論題（共 8 題）

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

> OS 是系統中唯一的 API 與資源管理者，所有應用程式都需透過 System Call 執行任務。本章探討：
> - OS 所提供的各項 Services（服務功能）
> - API 與 System Call 的差異與關係
> - 常見的 OS 架構設計方法（如 Layered、Microkernel、Virtual Machine）

---

## 1. 作業系統服務功能（OS Services）

作業系統提供各式服務（Services），主要目的是協助應用程式順利執行各項任務，並確保系統整體的安全性與效率。這些服務通常透過 System Call 實作，也稱為 Service Routine 或 Interrupt Routine。

---

### 1.1 使用者導向的核心服務

1. **User Interface（使用者介面）**：
   - 提供使用者與系統互動的方式，分為：
     - **CLI（Command Line Interface）**：文字介面，功能完整，適合進階操作與系統開發。
     - **GUI（Graphical User Interface）**：圖形介面，較直覺，但彈性與效能較低。

2. **Program Execution**：負責程式的建立、載入與執行。

3. **I/O Operations**：處理輸入/輸出設備的操作請求（如鍵盤、螢幕、磁碟等）。

4. **File-System Manipulation（檔案操作）**：
   - 提供檔案建立、開啟、關閉、寫入、刪除等操作。
   - 作業系統負責管理檔案結構與存取權限，保障資料一致性與安全性。

5. **Communication（程序間通訊）**：
   - 支援 Process 間的資料交換。
   - 可跨機（如 TCP/IP）或在本機內進行（Thread 間或 Process 間）。
   - 兩種模型：
     - **Message Passing**：經由 OS 管理的記憶體複製機制傳遞資料。
     - **Shared Memory**：透過共用記憶體區塊直接溝通，效率較高但需同步機制。

6. **Error Detection（錯誤偵測）**：
   - 偵測執行過程中的錯誤與異常，屬於系統維護的一部分。
   - 包括硬體錯誤、程式錯誤與資源管理錯誤。

---

### 1.2 系統導向的管理服務

7. **Resource Allocation（資源分配）**：
   - 包含 CPU 時間、記憶體、I/O 裝置與檔案的分配。
   - 如：CPU Scheduling、Memory Allocation、Open File Limits 等。

8. **Accounting（資源記錄）**：
   - 監控系統中各 Process 的資源使用狀況。
   - 可用於統計分析或設定使用限制，必要時由系統管理者調整。

9. **Protection and Security（保護與安全性）**：
   - 預防惡意使用與存取，確保資料與系統安全。
   - 基本機制如登入驗證（帳號密碼）、資源存取控制（Access Control）。

這三項屬於系統內部管理功能，雖然與一般使用者關聯較少，卻對系統穩定性與效率至關重要。

---

### 1.3 User Interface（使用者介面）

- **CLI（Command Line Interface）**：以文字指令與系統互動，功能完整，透過 Shell（如 Bash、CShell）解析指令並轉交 OS。
- **GUI（Graphic User Interface）**：以圖形操作為主，較直覺易用，但功能彈性與效率不如 CLI。
- 多數系統同時支援 CLI 與 GUI，但 CLI 更適合進行大型系統建構或自動化流程。

> 📌 Shell 是介於 CLI 與 OS 間的中介程式，可自訂別名與外觀設定，如使用 `alias` 設定個人化指令。

---

### 1.4 Communication（通訊模式）

作業系統提供兩種主要的 Process 間溝通模式：

- **Message Passing（訊息傳遞）**：
  - 透過 System Call 實作資料的送出與接收
  - 資料需多次複製（memory copy），安全但效能較低
  - 適用於跨機器的網路溝通，如 Socket Programming

- **Shared Memory（共享記憶體）**：
  - 程式共享特定記憶體區塊直接交換資料
  - 效能較高，但需透過 OS 分配記憶體並進行權限管理
  - 多執行緒（Thread）之間預設共用部分記憶體
  - 易導致 Synchronization 問題或 Deadlock（後續章節說明）

> 📌 系統記憶體劃分為：
> - User Space：應用程式使用區
> - Kernel Space：OS 核心專用區，僅允許在 Kernel Mode 執行的程式存取

---

## 2. 應用程式介面與系統呼叫（OS-Application Interface）

作業系統提供的系統呼叫（System Call）是應用程式與核心溝通的唯一管道，而應用程式介面（API）則是開發者與作業系統之間的橋樑。本節將探討兩者的關係與角色差異，並說明參數傳遞方式與實務設計細節。

---

### 2.1 System Call 與 OS 的介面

#### ▸ System Call 的角色
- System Call 是應用程式向作業系統請求服務的正式方式。
- 本質是一種 **Software Interrupt**，觸發時 CPU 從 User Mode 切換至 Kernel Mode。
- 常見的呼叫類型包括：
  - Process control：`fork()`、`exit()`、`wait()`
  - File management：`open()`、`read()`、`write()`
  - Device management：`ioctl()`、`read()`、`write()`
  - Info maintenance：`getpid()`、`alarm()`、`time()`
  - Communication：`send()`、`recv()`、`pipe()`、`shmget()`

#### ▸ 與 CLI / GUI 的關係
- CLI / GUI 是**使用者對 OS 的介面**，而程式則透過 function call（如 API、System Call）與 OS 溝通。
- 每項系統功能通常會有一個或多個函式（或中斷處理程式）來支援。

---

### 2.2 API：應用程式介面與函式庫

#### ▸ API 與 System Call 的關係
- System Call 為 OS 的底層介面，而 API 是開發者的高階介面。
- API 多由 **C Library（如 glibc）** 或 Java Library 提供，進一步封裝或組合多個 System Call。
- 並非所有 API 都會觸發 System Call，例如：
  - `abs()` 僅做數學計算，不需進入核心
  - `malloc()` 會觸發 `brk()` 等 System Call

#### ▸ 執行流程示意
1. 程式呼叫 `printf()` ➝
2. 進入 C Library ➝
3. 呼叫 `write()` 的 System Call ➝
4. 產生 Software Interrupt ➝ 進入 Kernel ➝
5. OS 執行對應 Service ➝ 回傳結果

#### ▸ Library 的實作多樣性
以 `exp2(x, y)` 為例，Library 可能有多種實作：
- `for` 迴圈版本（重複乘法）
- 位元位移版本（`x << y`）
- 硬體指令版本（`HW_EXP()`）

> 📌 API 僅定義介面格式（function signature），而實作由 Library 決定。

---

### 2.3 API 標準與虛擬機支援

#### ▸ 常見 API 標準

- **Win32 API**：提供 Windows 系統下的檔案、網路、UI 控制等操作，常見於桌面應用開發。
- **POSIX API**：廣泛應用於 Unix/Linux/macOS 等系統，強調 API 介面一致性與程式可攜性（Portability）。
- **Java API**：定義在 JVM 虛擬機中執行的標準函式介面，具有高度抽象與跨平台能力。

#### ▸ POSIX API 補充（重點）

- POSIX（Portable Operating System Interface for Unix）是一套統一的 API 標準，用來整合不同 Unix-like 系統的開發介面。
- 儘管各作業系統的 System Call 與底層實作可能不同，但只要遵循 POSIX API 定義，就能讓應用程式在多平台上執行。
- 實現方式為：程式碼不變，僅需針對目標系統重新編譯即可，如 Linux ↔ macOS。
- POSIX API 的存在，讓開發者可編寫一次、部署多平台，降低開發與維護成本。
- 例：Pthread（POSIX Thread Library）是符合 POSIX 標準的多執行緒函式庫，如 `pthread_create()`、`pthread_join()`。

> 📌 POSIX 的設計目的不僅是標準化，更是為了解決早期 UNIX 系統分裂問題，實現「一致的開發介面」與「程式可攜性」。

#### ▸ JVM 的運作邏輯

- Java 程式會編譯為 Bytecode，並於 JVM 中執行。
- JVM 負責將 Bytecode 轉譯成實體硬體平台的指令（如 x86、ARM）。
- Java 程式不直接使用作業系統 API，而是透過 Java API 操作虛擬機內的抽象資源。

> 📌 JVM 提供平台獨立性，但會引入執行期轉譯成本。JVM 的跨平台優勢來自對每個目標系統量身打造的 JVM 實作，而非單一可執行檔。

---

### 2.4 使用 API 的三大好處

#### ▸ Simplicity（簡化）
- 提供統一且直觀的介面，減少開發者進入核心的需求。
- 使用者可專注撰寫應用程式邏輯，不必處理中斷與暫存器。

#### ▸ Portability（可攜性）
- 只要符合 API 標準（如 POSIX），即可跨平台重編譯使用。
- 程式碼不依賴特定系統實作，提高可移植性。

#### ▸ Efficiency（效率）
- 系統會優化 API 的實作，避免不必要的系統切換。
- 善用使用者層（User Space）即可完成的操作，不進入 OS 核心。

---

### 2.5 System Call 的參數傳遞方式

#### ▸ 常見三種方式

1. **Register 傳遞**：  
   直接將參數寫入暫存器（如 EAX、EBX…），速度快但數量有限。

2. **Memory Table**：  
   使用者程式將參數寫入記憶體中的資料結構，並將其位址（pointer）傳給作業系統。作業系統擁有存取該記憶體區域的權限，能依此位址讀取整個參數內容並完成對應處理。

3. **Stack 傳遞**：  
   透過堆疊（Stack）將參數依序推入與彈出，傳遞方式與一般函式呼叫機制相同，具有良好可讀性與延伸性。

---

## 3. 作業系統結構設計（OS Structure）

作業系統並非一體成形，而是可依照不同需求與系統目標進行架構設計。從早期的簡易結構，到現代主流的模組化設計與虛擬機技術，各種架構各有特色與優劣，亦反映了不同時代在安全性、維護性、效能與擴充性之間的取捨。

---

### 3.1 系統設計目標與思維差異

#### ▸ 使用者目標（User Goals）
- 易於操作、可靠、安全、效率高（以互動回應為主）
- 如 GUI 介面直覺、程式不當機、資料不遺失等

#### ▸ 系統設計者目標（System Goals）
- 易於設計、實作與維護，穩定且效率高（以資源管理為主）
- 強調模組清楚、容易除錯、可擴充

> 📌 結論：雙方的「效率」定義不同 ➝ 使用者重互動即時性，設計者重整體資源使用

---

### 3.2 作業系統的四種典型架構

#### ▸ Simple Structure（簡易架構）
- 代表系統：MS-DOS、早期 UNIX。
- 沒有明確模組劃分，功能混合寫在核心中。
- 修改其中一處程式碼可能導致整體系統錯誤。
- 易於實作，但難以維護與擴充。
- 缺乏保護與控制機制，不適用於大型多任務系統。

---

#### ▸ Layered Architecture（層次架構）
- 將 OS 切分為多層，每層只可呼叫下一層。
- 上層介面依賴下層提供的功能。
- 優點：
  - 層次結構清楚、模組間耦合性低。
  - 易於除錯與管理，每層可以獨立測試。
- 缺點：
  - 劃分層次不易。
  - 若某些功能橫跨多層，可能產生效能瓶頸。

---

#### ▸ Microkernel 架構
- 核心功能極小化：只包含 Process 管理、Memory 管理與基本 IPC。
- 其他模組如檔案系統、驅動程式等皆移至 User Mode。
- 使用訊息傳遞（Message Passing）與核心互動。
- 優點：
  - 系統穩定性高，模組故障不會癱瘓整體系統。
  - 易於更新與移植。
- 缺點：
  - 過多訊息交換可能造成效能降低。
  - 系統設計更複雜。

---

#### ▸ Modular OS（模組化架構）
- 範例：現代 Linux 採用。
- OS 提供模組化框架，允許在執行時動態載入/移除模組（如驅動程式）。
- 模組間透過定義良好的介面溝通。
- 優點：
  - 易於維護、彈性高，提升模組重用性。
  - 系統不需重新編譯即可擴充功能。
- 所有模組仍運行於 Kernel Space，避免 Microkernel 的通訊負擔。

---

### 3.3 虛擬機設計與執行方式

#### ▸ Virtual Machine（虛擬機架構）
- 在既有系統上再建立抽象層，模擬整個硬體系統
- 每個 VM 拿到一份「看似獨立的硬體環境」

#### ▸ 運作概念
- 每個 Guest OS 執行在 Host OS 上的 user space 中
- 當 Guest OS 嘗試執行特權指令 ➝ 被中斷並由 Host 處理
- 若 CPU 支援 VM Mode，可直接由硬體協助攔截/模擬

> 📌 若指令為 Critical Instruction（無法攔截），則必須透過特殊轉譯或硬體支援

#### ▸ 優點與應用場景
- **隔離性強**：VM 間彼此獨立，適用資安與教學
- **相容性好**：舊系統可在 VM 中執行
- **研發環境**：可安全測試不穩定程式（如 kernel 修改）
- **雲端共享**：資源共享與分配利器（Cloud / SaaS）

#### ▸ 虛擬化技術類型
1. **Full Virtualization（完全虛擬化）**：
   - Guest OS 無需修改即可運作（例：VMware）
   - 模擬完整硬體，實作複雜、效率稍低

2. **Para-Virtualization（半虛擬化）**：
   - Guest OS 須經修改才能相容（例：Xen）
   - 效能較佳，較能進行全域最佳化資源管理

3. **JVM（Java Virtual Machine）**：
   - 編譯為 bytecode 於 JVM 執行，不與硬體直接對應
   - 利用 JIT（Just-In-Time Compiler）進行即時翻譯加速執行
   - 提供安全、封裝良好的執行環境，適合網路應用與跨平台需求

> 📌 Java 的 JVM 本質上就是一種虛擬機設計概念的實例，與 Nachos 類似但更進階完整

---

## 4. 作業系統結構練習題

---

### 4.1 選擇題（共 20 題）

1. 作業系統中的 System Call 主要功能為何？  
   A. 儲存資料至磁碟  
   B. 提供應用程式與核心之間的溝通介面  
   C. 顯示應用程式圖形畫面  
   D. 執行 CLI 指令  

2. 關於 Microkernel 架構，下列敘述何者正確？  
   A. 所有模組皆在 Kernel Space 執行  
   B. 所有模組皆可直接控制硬體  
   C. 只保留核心功能在 Kernel，其他模組執行於 User Space  
   D. 無需透過訊息傳遞機制進行模組溝通  

3. POSIX API 的主要優勢為何？  
   A. 提供 Windows GUI 支援  
   B. 執行效能高於 Java API  
   C. 提供統一標準以利跨平台移植  
   D. 專為微控制器環境設計  

4. 關於 System Call 的參數傳遞方式，下列何者描述正確？  
   A. Register 傳遞可容納大量參數  
   B. Stack 傳遞無法應用於 System Call  
   C. Memory Table 可透過指標傳遞多個參數  
   D. 所有參數皆需先轉換成 JSON  

5. 下列哪一項最符合 Modular OS 的特性？  
   A. 系統啟動前須載入所有模組  
   B. 所有元件皆執行於 User Space  
   C. 模組可於執行中動態載入或移除  
   D. 每個模組皆需單獨編譯成核心  

6. 關於 API 與 System Call 的關係，下列敘述何者正確？  
   A. 所有 API 均為 System Call 的別名  
   B. API 會觸發 GUI 顯示  
   C. API 是封裝 System Call 的開發者介面  
   D. System Call 為 C Library 的封裝  

7. 作業系統提供 Shared Memory 的目的為何？  
   A. 增加記憶體空間  
   B. 降低中斷次數  
   C. 讓 Process 間直接共享資料區域  
   D. 快速存取核心資料  

8. Layered OS 架構的優點為何？  
   A. 效能最佳  
   B. 可交叉呼叫所有模組  
   C. 結構清楚、模組獨立  
   D. 適合用於嵌入式系統  

9. CLI 相對於 GUI 的優勢為何？  
   A. 執行速度慢但視覺友善  
   B. 不適合程式開發  
   C. 功能完整且可自動化操作  
   D. 僅適用於 Linux  

10. JVM 提供跨平台能力的原因是？  
    A. JVM 支援所有硬體架構  
    B. Bytecode 可直接於所有 OS 執行  
    C. JVM 將 Bytecode 轉譯為當前平台指令  
    D. Java 可轉換成 POSIX API  

11. Modular OS 架構中，為何可動態載入模組？  
    A. 核心支援即時重編譯  
    B. 模組以獨立進程運作  
    C. 核心提供模組化介面並支援熱插拔  
    D. 每個模組皆由作業系統重新載入  

12. 為什麼 Microkernel 較適用於嵌入式系統？  
    A. 資源使用率高  
    B. 架構簡單  
    C. 可重編核心程式碼  
    D. 模組獨立且易於移植  

13. 當程式呼叫 `printf()` 時，會觸發哪種作業系統服務？  
    A. Process Control  
    B. File Management  
    C. Device Management  
    D. Communication  

14. 哪種架構強調將作業系統分層，以降低模組耦合？  
    A. Monolithic  
    B. Modular  
    C. Layered  
    D. Hybrid  

15. 為何許多程式設計者不需直接接觸 System Call？  
    A. System Call 由 OS 自動產生  
    B. API 封裝底層複雜操作  
    C. System Call 屬於 GUI 的一部分  
    D. 所有作業系統皆禁用 System Call  

16. 下列哪一種溝通方式最可能導致 Synchronization 問題？  
    A. Message Passing  
    B. Shared Memory  
    C. POSIX Pipe  
    D. Socket 通訊  

17. 記憶體表格傳參數（Memory Table）的優點是什麼？  
    A. 參數可加密處理  
    B. 適合少量簡單參數  
    C. 可集中管理並一次傳遞多個參數  
    D. 無需 kernel 存取記憶體  

18. 作業系統結構中，哪一種方式可提高模組重用性？  
    A. Simple Structure  
    B. Layered  
    C. Microkernel  
    D. Modular  

19. 哪種 API 標準支援 Unix-like 系統的一致介面？  
    A. Win32 API  
    B. Java API  
    C. POSIX API  
    D. .NET API  

20. 為何在設計 System Call 時需考慮參數傳遞效率？  
    A. 頻繁切換 GUI 對效能影響大  
    B. 大部分參數需轉為 JSON  
    C. System Call 進入 Kernel 需上下文切換  
    D. API 設計語法複雜

---

#### 參考答案

1. **B**：System Call 是應用程式與作業系統核心之間的主要溝通橋樑，用於請求系統服務。  
2. **C**：Microkernel 架構僅保留最核心功能，其他服務如驅動與檔案系統皆在 User Space 執行。  
3. **C**：POSIX 提供一致的 API 標準，使程式可在不同 Unix-like 系統間移植。  
4. **C**：Memory Table 將參數結構放入記憶體，並傳遞指標給 OS，適合參數較多的情境。  
5. **C**：Modular OS 允許在系統執行中動態載入或移除模組，是現代作業系統的重要特性。  
6. **C**：API 為開發者提供易用介面，底層可能呼叫多個 System Call，達成高階功能。  
7. **C**：Shared Memory 允許多個 Process 共用一塊記憶體區，用於快速交換資料。  
8. **C**：Layered 架構將系統功能分層，提升模組獨立性與可維護性。  
9. **C**：CLI 支援腳本、自動化與複雜指令操作，是系統開發與管理的首選。  
10. **C**：JVM 將平台中立的 Bytecode 轉譯成特定平台指令，使 Java 程式具備跨平台能力。  
11. **C**：Modular OS 提供核心模組管理機制，支援熱插拔與模組動態維護。  
12. **D**：Microkernel 架構模組化程度高、元件獨立，利於嵌入式系統的穩定與擴充。  
13. **C**：`printf()` 實際會透過 C Library 封裝的 `write()` System Call 執行裝置輸出。  
14. **C**：Layered OS 將系統功能劃分為清楚層次，強化模組間隔離與解耦合。  
15. **B**：開發者透過高階 API 編程即可間接使用 System Call，無需手動進入核心。  
16. **B**：Shared Memory 因多方同時存取，易導致競爭條件與同步問題。  
17. **C**：Memory Table 可一次傳遞多個參數並集中管理，適用於複雜系統呼叫。  
18. **D**：Modular 架構支援模組化與動態管理，提升模組重用性與系統彈性。  
19. **C**：POSIX API 是 Unix-like 系統共通介面，提供程式設計者一致的呼叫標準。  
20. **C**：System Call 需進入 Kernel Mode，伴隨上下文切換與效能考量，因此需設計有效的參數傳遞機制。

---

### 4.2 申論題（共 8 題）

1. 請說明 System Call 的基本概念與其在作業系統中的角色。
2. 請比較 API 與 System Call 的差異與聯繫，並說明兩者各自的使用時機。
3. 為何現代作業系統多採用 Modular OS 架構？請列出其優點並舉例說明。
4. 說明 Layered OS 與 Microkernel 架構在系統設計上的主要差異與優劣。
5. Java 程式如何透過 JVM 跨平台執行？JVM 的設計如何影響效能？
6. 為何 POSIX API 在系統設計中被廣泛採用？其對軟體開發者有何影響？
7. 請說明 Shared Memory 與 Message Passing 的通訊機制差異與使用場景。
8. 在設計系統呼叫時，應如何考量參數傳遞方式以兼顧效能與安全性？

---

#### 參考答案

1. **System Call 的角色與概念：**  
   System Call 是應用程式與作業系統溝通的橋樑，允許使用者程式執行核心層級的功能，如檔案存取、行程控制與裝置操作。其本質是一種軟體中斷，能將系統由 User Mode 切換至 Kernel Mode。

2. **API 與 System Call 差異與關聯：**  
   API 是應用程式可見的高階函式介面，通常由函式庫（如 glibc）實作，內部可能會呼叫一個或多個 System Call。API 提供封裝與抽象，使用更直覺，而 System Call 為進入作業系統核心的具體機制。

3. **Modular OS 架構的優點與應用：**  
   Modular OS 支援模組動態載入與卸除，讓每個子系統獨立開發與維護，提高可擴充性與除錯效率。此架構廣泛應用於 Linux 核心，可針對需求安裝特定驅動或監控模組，效能與彈性兼具。

4. **Layered OS 與 Microkernel 架構比較：**  
   Layered OS 以層次區分功能，每層僅與相鄰層互動，強調清晰結構；Microkernel 則將大多數服務移至 User Space，僅保留最基本功能於核心，提高穩定性與模組化，但可能犧牲效能。

5. **JVM 的平台中立性與效能影響：**  
   Java 程式先編譯為 Bytecode，在 JVM 虛擬機中執行，使其不依賴實體平台，實現一次撰寫、多處執行。但此架構須額外轉譯與抽象操作，可能導致效能低於原生執行。

6. **POSIX API 的作用與優勢：**  
   POSIX 提供統一的作業系統 API 標準，使程式可在不同 Unix-like 系統間無需重寫程式邏輯，只需重新編譯。對開發者而言，可提升程式的可攜性與系統間一致性。

7. **Shared Memory vs. Message Passing：**  
   Shared Memory 讓多個 Process 共用記憶體區塊，溝通速度快但需管理同步；Message Passing 則透過 OS 協助的資料複製與封包傳遞，雖效能較低但具備更高的安全性與隔離性。

8. **System Call 參數傳遞考量：**  
   系統呼叫可透過 Register、Memory Table 或 Stack 傳遞參數。需依參數數量與資料結構選擇適當方式，兼顧速度（如 register）與彈性（如記憶體指標），同時確保系統安全與效率。

---

# Ch3：Processes

> **主題：程序的基本概念、排程、操作與程序間通訊（IPC）**

## 1. 程序基本概念（Process Concept）

程序（Process）是系統執行中最基本的單位。理解程序的定義、生命週期與其組成，是後續學習排程、同步與溝通等主題的基礎。

---

### 1.1 程序的定義與組成

#### ▸ 程式與程序差異
- **程式（Program）** 是靜態的指令集合（例如 C 程式碼檔案）。
- **程序（Process）** 是程式執行後的動態實體，包含程式碼、資料、堆疊與其他中介資訊。

> 📌 同一份程式可同時啟動多個程序，每個程序彼此獨立。

#### ▸ 程序組成項目（Process Components）
1. **程式碼區（Code）**：程式本體的指令區。
2. **資料區（Data）**：全域變數、靜態資料等。
3. **堆疊區（Stack）**：暫存函式參數、區域變數與返回位址。
4. **堆區（Heap）**：動態配置記憶體使用（如 `malloc()`）。

---

### 1.2 作業的狀態與生命週期

#### ▸ 常見五種程序狀態
1. **New**：程式初啟動，尚未排入執行序列。
2. **Ready**：已完成初始化，等待 CPU 分配。
3. **Running**：正在使用 CPU 執行中。
4. **Waiting**：等待某事件完成（如 I/O）。
5. **Terminated**：程序執行結束或被強制終止。

> ✅ 這些狀態形成一個有限狀態機（Finite State Machine），程序會在其間來回轉換。

#### ▸ 狀態轉移常見情境
- **Ready ➝ Running**：由排程器選中。
- **Running ➝ Waiting**：等待 I/O 或資源。
- **Running ➝ Ready**：時間片用完，被強制中斷。
- **Waiting ➝ Ready**：等待事件完成。
- **Running ➝ Terminated**：程式正常結束或錯誤終止。

---

### 1.3 Process Control Block（PCB）

PCB 是作業系統用來追蹤每個程序狀態的資料結構，每個程序都有一個對應的 PCB。

#### ▸ PCB 包含資訊
- Process ID（PID）：程序編號。
- 程式計數器（PC）：下一條執行指令的位置。
- CPU 暫存器內容：中斷時需儲存的狀態。
- 排程資訊：優先權、排程隊列等。
- 記憶體管理資訊：程式碼與資料在記憶體的位置。
- 帳務資訊：執行時間、資源使用量。
- I/O 狀態資訊：開啟檔案、裝置清單。

> 📌 PCB 是 CPU 切換程序（Context Switch）時必須儲存與載入的核心結構。

---

### 1.4 Process 表示與 Context Switch

#### ▸ Process Table
- 系統會建立一張「Process Table」，紀錄所有正在執行的程序之 PCB。

#### ▸ Context Switch（上下文切換）
- CPU 需要將一個程序的執行狀態儲存，再載入另一程序的狀態。
- 通常發生在：
  - 時間片結束
  - I/O 等待
  - 系統中斷或高優先程序抵達

> 📌 Context Switch 開銷高（涉及大量暫存器與記憶體操作），頻繁切換會影響效能。

---

### 1.5 程序創建與終止

#### ▸ 程序建立方式
- **UNIX/Linux**：常用 `fork()` 產生新程序，子程序複製父程序記憶體內容。
- **exec()**：用來替換目前程序的記憶體內容並執行新程式。

#### ▸ 程序終止方式
- 自動結束：程式執行結束時觸發 `exit()`。
- 系統強制：OS 或其他程序（如父程序）呼叫 `kill()` 終止其執行。
- 錯誤終止：例外狀況導致崩潰（如 segmentation fault）。

以下是依照你指定格式、並根據 PDF p.13\~p.21 所整理的筆記內容：

---

## 2. 程序排程（Process Scheduling）

作業系統的核心職責之一就是在多個程序之間分配 CPU。程序排程（Scheduling）策略會影響系統的效能、回應時間與資源使用效率。本節將說明基本觀念、排程時機與各種常見演算法。

---

### 2.1 排程時機與分類

#### ▸ 排程發生時機

* **Process 進入 Ready Queue 時**：程序準備好執行，進入等候區。
* **Process 結束或 Blocked**：CPU 釋出，需重新分配。
* **I/O 完成後回到 Ready Queue 時**：等待排程執行。

#### ▸ Scheduling 類型

1. **Long-term（長期排程）**：

   * 控制系統中進入的 process 數量（jobs ➝ processes）。
   * 通常在 batch 系統中較常見。

2. **Short-term（短期排程）**：

   * 又稱 CPU scheduler，負責挑選 Ready Queue 中的下一個 process。
   * 發生頻繁，是核心中的關鍵元件。

3. **Medium-term（中期排程）**：

   * 暫時將 process 移出記憶體（swapping out），以節省資源。
   * 稍後再將其換回執行（swapping in）。

> 📌 Real-time 系統的 scheduler 必須保證 deadline，否則會造成嚴重後果。

---

### 2.2 Dispatcher（派遣器）

* Dispatcher 負責實際切換到 CPU 上的選定 process。
* 操作包括：

  * 切換上下文（Context Switch）
  * 切換至使用者模式（User Mode）
  * 跳至 process 執行點（程式計數器）

> 📌 Dispatcher Delay：從排定一個 process 到實際執行間的時間，影響系統即時性。

---

### 2.3 排程準則（Scheduling Criteria）

不同系統根據目標選擇不同的衡量標準：

* **CPU Utilization**：CPU 使用率，越高越有效。
* **Throughput**：每單位時間內完成的 process 數量。
* **Turnaround Time**：一個 process 從提交到完成所需的時間。
* **Waiting Time**：在 Ready Queue 中等待的總時間。
* **Response Time**：系統對使用者輸入的回應延遲。

> 📌 Interactive 系統重視 Response Time；Batch 系統重視 Throughput 和 CPU Utilization。

---

### 2.4 排程演算法（Scheduling Algorithms）

#### ▸ FCFS（First-Come First-Served）

* 非搶佔式，依照抵達順序執行。
* 簡單但容易產生 **Convoy Effect**（慢程序拖住快程序）。

#### ▸ SJF（Shortest Job First）

* 執行時間最短者優先。
* 最佳平均等待時間。
* 缺點：預測執行時間困難；非搶佔式會餓死長作業。

#### ▸ SRTF（Shortest Remaining Time First）

* SJF 的搶佔式版本，根據剩餘時間決定排程。
* 需準確估計剩餘執行時間。

#### ▸ Priority Scheduling

* 指定權重，優先執行高優先權程序。
* 可為搶佔式或非搶佔式。
* 易產生 **Starvation（飢餓）** 問題 ➝ 可用 Aging 解決。

#### ▸ RR（Round Robin）

* 每個 process 依序取得固定時間片（time quantum）。
* 公平性佳，適用於 time-sharing 系統。
* 時間片太短 ➝ context switch overhead 高；太長 ➝ 回應差。

#### ▸ Multilevel Queue Scheduling

* 將 process 分類放入不同 Queue（如 Foreground / Background）。
* 每層使用不同演算法，且層級間優先順序不同。
* 無法跨 Queue 移動。

#### ▸ Multilevel Feedback Queue

* 與 Multilevel Queue 類似，但允許 process 根據行為跨 Queue 移動。
* 新 process 通常優先（放在高階 Queue）。
* 常用於 Interactive 系統，兼顧反應速度與資源效率。

> 📌 Feedback Queue ➝ 若 process 常用 CPU ➝ 降級至較低 Queue。

---

### 2.5 排程演算法比較（Scheduling Comparison）

| 演算法            | 類型       | 是否搶佔  | 平均等待時間   | 優點               | 缺點                 |
| -------------- | -------- | ----- | -------- | ---------------- | ------------------ |
| FCFS           | 非搶佔      | 否     | 中等       | 實作簡單             | 容易產生 convoy effect |
| SJF            | 非搶佔      | 否     | 最低       | 最佳平均等待時間         | 需預測執行時間、會餓死長作業     |
| SRTF           | 搶佔       | 是     | 很低       | 優化反應時間           | 開銷大、需準確估算執行時間      |
| Priority       | 搶佔 / 非搶佔 | 是 / 否 | 視優先權設定而異 | 可靈活控制任務順序        | 易餓死低優先權程序          |
| Round Robin    | 搶佔       | 是     | 適中       | 公平性佳             | 時間片設計困難            |
| Multi-Queue    | 搶佔 / 非搶佔 | 視設定   | 中等       | 各類型 process 分層處理 | 無法跨 Queue          |
| Feedback Queue | 搶佔       | 是     | 彈性       | 動態調整、適應性強        | 複雜、需調參             |

---

以下是依照 PDF 第三章（p.22\~p.30）內容整理的筆記，並符合你要求的 **Markdown (.md)** 格式，將其歸於章節：

---

## 3. 程序操作（Operations on Processes）

作業系統不僅要負責程序的建立與排程，也需處理程序間的關係與同步問題。本章介紹父子程序的產生方式、程序終止的處理機制，以及常見的程序間關係模型。

---

### 3.1 Process Creation（程序建立）

#### ▸ 常見建立方式

* 程序可透過 **System Call `fork()`** 產生子程序（child process）。
* 子程序可透過 **`exec()`** 系列呼叫載入新程式（常見於 shell）。

#### ▸ 程序的繼承特性

* 子程序會複製父程序的記憶體區塊與狀態（資料段、堆疊、開啟檔案等）。
* 可選擇：繼續父程序的程式（複製）、或載入新程式（取代）。

> 📌 Linux 預設 `fork()` + `exec()` 搭配使用，形成新程序並執行指定程式。

---

### 3.2 Process Termination（程序終止）

#### ▸ 正常與異常終止

* **正常終止**：程式執行完畢、資源釋放（`exit()`）。
* **異常終止**：由其他程序或 OS 終止（如 `abort()` 或錯誤事件）。

#### ▸ 終止方式與行為

* 父程序可主動終止子程序：

  * 子程序出錯
  * 子程序不再需要
  * 父程序終止時，OS 通常也會同步終止其所有子程序（遞迴終止）

> 📌 Unix 系統提供 `wait()` 與 `waitpid()` 讓父程序等待子程序結束，避免殭屍程序（Zombie）。

---

### 3.3 程序間的關係模型

#### ▸ Independent Process（獨立程序）

* 程序之間沒有互相影響，不共享資料，也不需同步。
* 適合處理彼此無交集的任務（如：文字編輯器與音樂播放程式）。

#### ▸ Cooperating Process（合作程序）

* 程序間可共享資料或透過 IPC（Inter-Process Communication）同步行為。
* 使用場景：

  * 多個 Process 操作同一資料結構（如：佇列）
  * Producer-Consumer 模型：生產與消費資料之間需有順序性與同步性

> 📌 合作程序需要同步與互斥機制（如 Semaphore、Mutex 等）以避免競爭條件（Race Condition）。

---

### 3.4 程序識別與控制

#### ▸ Process ID（PID）

* 每個程序都有唯一的識別碼，由作業系統配置。
* 系統呼叫如 `getpid()`、`getppid()` 可取得自己的 PID 與父程序 PID。

#### ▸ 程序控制相關 System Calls

* `fork()`：產生新程序
* `exec()`：載入新程式
* `wait()`：父程序等待子程序終止
* `exit()`：結束當前程序
* `kill(pid, sig)`：傳送訊號給指定程序，可用於終止

> 📌 程序控制是多工環境中的基本建構要素，後續章節將結合程序溝通與同步進行進一步探討。

---

以下是依據 PDF 檔案（p.31–p.52）的內容與順序，所整理的第四章節筆記，格式與前幾章一致，並以 Markdown 呈現：

---

## 4. 程序間通訊（Interprocess Communication, IPC）

程序間通訊（IPC）是多程序系統中不可或缺的一環。程序之間可能需要交換資料、共享資源，或同步作業。IPC 的設計方式會影響整體系統的效率與可靠性。

---

### 4.1 IPC 的基本概念與目的

#### ▸ 為何需要 IPC？

* 單一程序往往無法完成所有任務。
* 程序間需協調彼此行為與資料交換，例如：

  * 前處理 ➝ 處理 ➝ 後處理（典型工作流程）
  * 使用者程式與作業系統或服務程式互動
* 提升模組化、可維護性與效率。

#### ▸ IPC 的設計重點

* 提供有效率的資料傳遞與同步方式。
* 保證正確性、安全性、與執行順序。

---

### 4.2 IPC 的主要模型

#### ▸ Message Passing（訊息傳遞）

* **概念**：資料透過訊息在程序間傳遞。
* **特性**：

  * 程序間 **無共享記憶體**。
  * 使用 OS 提供的 API，如 `send()` / `receive()`。
* **同步方式**：

  * **Blocking（同步）**：sender / receiver 會等待對方完成動作。
  * **Non-Blocking（非同步）**：不等待，先送出或收資料後繼續執行。

#### ▸ Shared Memory（共享記憶體）

* **概念**：多個程序共同存取一塊記憶體。
* **特性**：

  * 執行效率高，無須經過 OS 核心多次複製資料。
  * 須自行管理同步（如使用 semaphore）。
* **應用**：

  * 適合頻繁交換大量資料的場景。

---

### 4.3 Direct 與 Indirect Communication

#### ▸ Direct Communication（直接命名）

* Sender 與 Receiver 須明確指明對方名稱。
* 系統提供簡單 API（e.g., `send(P1, msg)`）。
* 須管理誰能與誰通訊（雙向 vs 單向）。

#### ▸ Indirect Communication（間接命名）

* 利用 **Mailbox（信箱）或 Port（通訊端口）** 作為中介。
* 程序傳送 / 接收皆針對 mailbox 操作，不指定對方名稱。
* 較具彈性，可實作多對一或一對多通訊。

---

### 4.4 Synchronization（同步）與 Buffering（緩衝區）

#### ▸ 同步（Synchronization）

* 通訊的兩端是否需等候對方？

  * **Blocking Send**：Sender 等待 Receiver 接收完訊息。
  * **Blocking Receive**：Receiver 等待訊息出現。
  * **Non-blocking**：雙方皆可獨立執行，不需等待。

#### ▸ 緩衝策略（Buffering）

* 用於儲存送出但尚未被接收的訊息。
* **三種常見策略**：

  1. **Zero Capacity**：無緩衝空間，Sender 必須等 Receiver。
  2. **Bounded Capacity**：固定大小，Sender 若滿則需等待。
  3. **Unbounded Capacity**：理論上無上限，Sender 不需等待。

---

### 4.5 IPC 實務範例：Linux 中的 IPC

#### ▸ IPC 機制

* **Pipe（管道）**：單向傳輸，父子程序之間溝通。
* **FIFO（命名管道）**：具名稱，可在無親屬關係程序間使用。
* **Message Queue**：OS 提供訊息佇列管理功能。
* **Shared Memory + Semaphore**：高效交換資料並進行同步。
* **Socket**：支援網路上的 IPC，跨主機程序也能通訊。

> 📌 在 Linux 中可使用 `ipcs` 指令查看 IPC 資源，如 Message Queue、Shared Memory、Semaphore。

---

### 4.6 典型使用場景比較

| 機制            | 特點          | 優點           | 缺點       |
| ------------- | ----------- | ------------ | -------- |
| Pipe          | 單向、匿名、暫態    | 簡單、低成本       | 限於親屬程序   |
| FIFO          | 單向、具名稱      | 支援無關程序通訊     | 較不彈性     |
| Message Queue | 系統管理、有訊息格式  | 結構清楚、支援優先權   | 較複雜      |
| Shared Memory | 記憶體共用、需自行同步 | 效率高、適合大量資料傳輸 | 同步困難     |
| Socket        | 支援本地/遠端程序通訊 | 跨機器、跨平台      | 建置與使用較繁瑣 |

> 📌 IPC 的選擇取決於通訊模式、資料量、效率要求與系統支援能力。

---

## 5. 程序與通訊練習題

本章節針對「程序概念、排程策略、程序操作與程序間通訊」進行重點回顧。

---

### 5.1 選擇題（共 20 題）

1. 作業系統中負責管理程序生命週期的主要結構是？
   - (A) Memory Map
   - (B) File Descriptor Table
   - (C) Process Control Block (PCB)
   - (D) Interrupt Vector

2. 在下列哪一種程序狀態轉換中會觸發 context switch？
   - (A) Ready → Running
   - (B) Waiting → Ready
   - (C) Running → Waiting
   - (D) Running → Terminated

3. 若系統使用 Preemptive Scheduling，表示：
   - (A) 程序會持續執行直到完成
   - (B) 每個程序都同時執行
   - (C) CPU 可強制切換執行中的程序
   - (D) 所有程序需手動釋放 CPU

4. 下列哪個系統呼叫可用來建立子程序？
   - (A) exec()
   - (B) fork()
   - (C) wait()
   - (D) exit()

5. 在 Unix 系統中，`fork()` 之後子程序與父程序的差異為何？
   - (A) 子程序擁有獨立的 PID
   - (B) 子程序與父程序共用記憶體空間
   - (C) 子程序會立即終止
   - (D) 子程序會執行不同程式碼段

6. 若使用 `exec()` 取代原本的程式：
   - (A) 會產生新的程序
   - (B) 原本程式被中斷暫存
   - (C) 原程序映像被替換
   - (D) 執行緒仍然保留

7. 下列哪一種 IPC 模式可跨系統實作？
   - (A) Shared Memory
   - (B) Message Queue
   - (C) Pipe
   - (D) TCP Socket

8. 若要在同一台電腦上兩程序共享記憶體，以下哪個為佳？
   - (A) TCP
   - (B) Pipe
   - (C) Shared Memory
   - (D) Semaphore

9. 下列哪一項不是 PCB 所紀錄的資訊？
   - (A) 程序狀態
   - (B) 開啟檔案表
   - (C) 中斷向量表
   - (D) CPU 暫存器內容

10. IPC 中的 Message Passing 模式主要依賴：
    - (A) Semaphore
    - (B) Signal
    - (C) 中斷觸發
    - (D) 系統呼叫與複製記憶體

11. 下列何者不屬於 Process 的特徵？
   A. 有獨立記憶體空間  
   B. 擁有程序識別碼（PID）  
   C. 可以同時存在於多個 CPU 上  
   D. 包含暫存器與程式計數器等上下文資訊  

12. 哪一個排程策略最有可能導致「飢餓」現象？
   A. Round Robin  
   B. Shortest Job First  
   C. First-Come, First-Served  
   D. Multilevel Queue  

13. 下列關於 Context Switch 的敘述，何者正確？
   A. Context Switch 僅發生在 User Mode  
   B. Context Switch 不需要儲存程序狀態  
   C. Context Switch 將增加排程的延遲  
   D. Context Switch 是系統內建的自動儲存機制  

14. 呼叫 `fork()` 後若父子程序都呼叫 `exec()`，結果為何？
   A. 父子程序會互相同步  
   B. 系統會中止其中一個程序  
   C. 各自的程式記憶體空間被新的程式覆蓋  
   D. 兩者將共享新的程式內容  

15. 下列何者不是 IPC（Inter-Process Communication）常見方法？
   A. Memory Mapping  
   B. Message Queue  
   C. Pipe  
   D. Signal Mask  

16. 使用 Message Passing 模型時，為確保傳遞資料的正確性與順序性，常需：
   A. 使用系統呼叫一次傳完所有資料  
   B. 採用同步式傳送與接收  
   C. 改用共享記憶體方式傳輸  
   D. 讓程序進入 idle 模式等資料  

17. 一個程序呼叫 `wait()` 的用途是：
   A. 等待鍵盤輸入  
   B. 等待所有 Thread 結束  
   C. 等待子程序終止  
   D. 等待排程器喚醒  

18. 在 Shared Memory 模型中，下列何者屬於開發者的責任？
   A. 管理共享記憶體區域權限  
   B. 建立中斷處理程序  
   C. 控制 CPU 指派邏輯  
   D. 提供資料一致性與同步  

19. 在 Unix/Linux 中，下列哪一個 IPC 方法最適合小型且暫時性資料交換？
   A. Message Queue  
   B. Pipe  
   C. Shared Memory  
   D. TCP Socket  

20. 如果一個程序正在被其他程序等待（如 `wait()`），其狀態會被標記為？
   A. Running  
   B. Ready  
   C. Blocked  
   D. Zombie  

---

#### 參考答案

1. **C**：Process Control Block（PCB）是作業系統用來管理程序的主要資料結構，包含程序狀態、ID、暫存器內容等資訊。
2. **C**：Preemptive Scheduling 允許 OS 在必要時中斷執行中的程序（Running → Ready），因此會觸發 context switch。
3. **C**：Preemptive（搶佔式）排程允許系統主動中斷執行中程序以分配 CPU 給其他程序。
4. **B**：`fork()` 是用來建立子程序的 System Call，產生一個與父程序幾乎相同的新程序。
5. **A**：`fork()` 後父子程序會有不同的 PID，程式碼相同但為獨立程序。
6. **C**：`exec()` 不會產生新程序，而是用新的程式內容覆蓋當前程序映像。
7. **D**：TCP Socket 是唯一可以跨系統進行程序間通訊的方式。
8. **C**：在同一台電腦上使用 Shared Memory 可達到最快的資料交換效率。
9. **C**：中斷向量表是與硬體設備相關的結構，不屬於單一程序（PCB）擁有。
10. **D**：Message Passing 模式主要透過系統呼叫（send/recv）及資料複製來完成程序間通訊。
11. **C**：一個程序無法「同時」存在於多個 CPU 上，這屬於多核心排程機制，非單一 Process 特徵。  
12. **B**：Shortest Job First 可能長期忽略執行時間長的作業，導致飢餓（Starvation）。  
13. **C**：Context Switch 是排程器切換程序時必經程序，會引入延遲，因為需要儲存與載入上下文資訊。  
14. **C**：`exec()` 會用新程式覆蓋原本記憶體內容，父子程序會執行不同程式，但各自獨立空間。  
15. **D**：Signal Mask 是用於訊號管理，不屬於 IPC 方法；IPC 主要包括 Shared Memory、Pipe、Message Queue 等。  
16. **B**：為避免資料錯亂或遺失，Message Passing 常採用同步傳送與接收機制（Blocking Send/Receive）。  
17. **C**：`wait()` 會讓父程序等待子程序結束並回收其資源，否則子程序會變成 Zombie。  
18. **D**：Shared Memory 需要使用者自行實作同步（如 Lock、Semaphore）來避免資料不一致問題。  
19. **B**：Pipe 是適合小型、短暫的資料傳輸（如父子程序之間），速度快但功能有限。  
20. **C**：當程序等待某事件完成（如 I/O、子程序結束），其狀態為 Blocked，等待系統通知再進入 Ready。

---

### 5.2 申論題（共 8 題）

1. 請說明 Process Control Block（PCB）的角色與其包含的重要欄位。
2. 試比較 Preemptive 與 Non-preemptive Scheduling 在效能與公平性上的差異。
3. 說明程序五種典型狀態與可能發生的狀態轉移。
4. 描述 `fork()` 與 `exec()` 系統呼叫的差異與典型應用場景。
5. 何謂 Context Switch？此動作會帶來哪些效能影響？
6. 程序間通訊常見方式為何？比較 Message Passing 與 Shared Memory 的優缺點。
7. 在父子程序間進行同步與等待時，可使用哪些機制？請舉例說明。
8. 多個程序競爭同一資源時可能產生哪些問題？作業系統如何防止？

---

#### 參考答案

1. **Process Control Block（PCB）的角色與欄位：**  
   PCB 是 OS 中用來儲存程序資訊的資料結構。每當產生一個新程序時，系統會建立對應的 PCB。  
   常見欄位包含：  
   - Process ID（PID）  
   - 程序狀態（Running, Ready, Blocked...）  
   - Program Counter（目前執行位置）  
   - CPU Register（暫存器內容）  
   - Memory 管理資訊（程式碼區、堆疊區、資料區）  
   - Scheduling 資訊（優先權、排程佇列）  
   - I/O 狀態與開啟的檔案資訊  

2. **Preemptive vs. Non-preemptive Scheduling 差異：**  
   - **Preemptive（可搶佔）：** 系統可強制中斷當前程序，將 CPU 分配給其他程序。  
     - 優點：提高反應速度與公平性。  
     - 缺點：Context Switch 次數增加，可能降低整體效能。  
   - **Non-preemptive（不可搶佔）：** 一旦程序佔用 CPU，直到主動釋放才可切換。  
     - 優點：Context Switch 少，實作簡單。  
     - 缺點：若程序執行過久，其他程序需長時間等待，降低互動性。

3. **程序五種典型狀態與轉移：**  
   - **New**：程序剛建立尚未執行。  
   - **Ready**：等待 CPU 執行。  
   - **Running**：正在執行中。  
   - **Waiting（Blocked）**：等待 I/O 或事件完成。  
   - **Terminated**：執行結束，準備釋放資源。  
   - 常見轉移：New → Ready → Running → Waiting → Ready → Running → Terminated。

4. **`fork()` vs. `exec()` 差異與應用：**  
   - `fork()`：建立與原程序幾乎相同的新程序（子程序），兩者共用程式碼但有獨立記憶體空間。常用於程式分岔、背景處理。  
   - `exec()`：用其他程式取代目前程序內容，常搭配 `fork()` 使用，以執行不同程式。

5. **Context Switch 定義與效能影響：**  
   Context Switch 是指 OS 在多工系統中切換不同程序所需進行的作業，需儲存當前程序狀態並載入新程序狀態。  
   - 效能影響：每次切換需耗費時間與資源（如記憶體快取失效），過多切換會降低 CPU 整體效率。

6. **Message Passing vs. Shared Memory：**  
   - **Message Passing：**  
     - 優點：簡單、安全、易同步。  
     - 缺點：資料需複製、效能較低。  
   - **Shared Memory：**  
     - 優點：速度快、資料共享效率高。  
     - 缺點：需自行處理同步與競爭問題。  
   - 應用選擇：不同系統與資料交換量需求會決定採用哪一方式。

7. **父子程序的同步與等待機制：**  
   - 常見方法：  
     - `wait()`：父程序等待子程序結束。  
     - Signal、Semaphore、Pipe 等 IPC 機制。  
   - 範例：父程序使用 `wait()` 確保子程序執行完成後才釋放資源。

8. **資源競爭問題與 OS 解法：**  
   - 問題：如 Deadlock、Starvation、Race Condition。  
   - 預防方式：  
     - 使用 Semaphore 或 Mutex 做同步控制。  
     - 避免循環等待條件。  
     - 實作 Deadlock Avoidance 或 Detection 演算法。

---

#  作業

## OS 作業一：NACHOS 架構與實作指引  
> 對應教材：周志遠《作業系統》 Ch2 - OS Structure (A)

> **作業主題：實作並理解 NACHOS 系統架構、System Call 流程、Interrupt 機制與虛擬機模擬原理**

---

### 1. 作業目的與概念

- ✅ 練習 trace 作業系統執行流程
- ✅ 實作 System Call 與理解中斷（Interrupt）流程
- ✅ 理解 NACHOS 作為虛擬作業系統的架構與運作方式  
- ⏰ 作業期限：兩週內完成

---

### 2. NACHOS 是什麼？

- NACHOS 是一個 **模擬作業系統**，不是跑在硬體上，而是跑在 Linux（或 macOS）上。
- 本質上是建立一個 **Virtual Machine**，模擬 **MIPS 架構的 CPU** 與作業系統。

---

### 3. Virtual Machine 架構運作說明

```text
你的程式碼（C++） 
    ↓ 編譯成 MIPS 指令
    ↓
Nachos Machine Simulation
    ↓
轉譯為 x86 指令
    ↓
Linux 真實 CPU 執行
```

- NACHOS 包含 MIPS Instruction 的模擬器 ➝ 轉成 x86 ➝ 由 Linux 執行
- 讓你「以為」MIPS 程式真的執行起來了

---

### 4. 作業內容與操作事項

- ✏️ 加入兩個 System Call：
  - `PrintInt()`（輸出整數）
  - `Open()` / `Close()`（模擬開啟與關閉檔案）

- 📂 需理解 syscall、interrupt 流程，並修改以下模組：
  - `machine/`（硬體模擬層）
  - `userprog/`（系統呼叫處理）
  - `threads/`（整體執行流程控制）
  - `test/`（user 程式放置區）

---

### 5. NACHOS 專案結構總覽

| 資料夾      | 說明 |
|------------|------|
| `lib/`     | 工具函式庫，不需理會 |
| `machine/` | 模擬 CPU / 記憶體（作業一要修改） |
| `threads/` | 控制作業系統執行方式 |
| `userprog/`| 處理 system call |
| `test/`    | 放置 user 程式 |
| `filesys/` | 作業四用到的真實檔案系統 |
| `network/` | 網路模擬，不用處理 |

---

### 6. System Call 實作流程

```text
user.c 程式呼叫 PrintInt() 
    ↓
include syscall.h
    ↓
呼叫 Nachos Kernel 中的 Handler
    ↓
觸發 Interrupt ➝ 進入對應處理函式
    ↓
執行 PrintInt、Open、Close 等
```

- System Call 是你與 OS 溝通的橋樑
- 所有呼叫都經由 `syscall.h` 定義與中介層
- 你只需要「接起來」，大部分程式已寫好

---

### 7. 組語 `.s` 檔案處理

- 為什麼會看到組語（`.s`）？
  - 因為 System Call 切換需要用 Assembly 實作
- 實際只需填入幾行程式
- 通常是 copy/paste 替換即可完成

---

### 8. 編譯與執行注意事項

```bash
make clean   # 清掉舊的編譯檔
make         # 使用 Nachos 的 Makefile 編譯
nachos -x test_program # 執行 user program
```

- 不可使用 GCC 編譯 user 程式，必須使用 Nachos 指令流程
- Output 會與 Linux 混在一起 → 用 Counter 確認是否是 Nachos 執行的

---

### 9. File System 的「假的實作」

- Nachos 並沒有完整實作 File System
- 前幾次作業中，檔案操作（open/close）其實是 **轉交給 Linux 處理**
- 作業四才會實作部分的真正 FS

---

### 10. Debug / Trace 建議

- **不要盲目 trace！** 需先理解整體架構再開始
- 建議流程：
  1. 從 `main()` 或 `start()` 開始
  2. 找到 system call 的觸發點
  3. 跟著中斷處理流程走
  4. 接回你新增的功能（如 PrintInt）

---

### 額外建議與工具

- 編輯器建議使用 `vim`
  - 可直接在 Linux 上撰寫與編譯
  - 學會基本指令即可
- 每次修改請務必使用 `make clean`，避免使用到舊版本檔案
- 有範例與 FAQ 請參考助教與課程網頁

---

### 作業一總結

- 加入 System Call（PrintInt、Open、Close）
- Trace System Call 與 Interrupt 流程
- 學會在 Nachos 結構中定位、修改、執行程式
- 記得使用 `counter` 確認是否是 Nachos 執行成功\
