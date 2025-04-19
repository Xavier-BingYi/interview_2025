# 1. 通訊協定 (SPI / I2C / UART)
- [ ] 撰寫 SPI / I2C / UART 的 driver
- [ ] 使用 `printf` 除錯並驗證資料傳輸
- [ ] 測試 I2C loopback（MCU 自己接自己）

# 2. MCU 系統架構
- [ ] **記憶體架構（SRAM / Flash）與 Linker file (`flash.ld` 與 `ram.ld`)**
  - 了解如何配置 MCU 的記憶體區域，並根據不同需求設定 Linker file，將程式碼和資料正確地放置於不同的記憶體區域。掌握如何使用 `flash.ld` 和 `ram.ld` 來分配 Flash 和 RAM 的記憶體區域，以便達成最佳效能與系統穩定性。

- [ ] **測試將固件放入 RAM (`ram.ld`) 運行**
  - 進行固件測試時，將程式碼加載到 RAM 中執行可以加速執行速度，並避免對 Flash 記憶體的修改。了解如何在 Linker file 中配置這一部分，並根據需要將程式碼和數據放入 RAM 以提高執行效率。

- [ ] **MCU 選型標準：評估接口數量、定時器資源、記憶體需求及 Flash 使用與執行需求**
  - 在選擇 MCU 時，評估所需的硬體資源，這包括接口數量、定時器資源、記憶體大小以及 Flash 記憶體的使用需求。選擇合適的 MCU 應該根據應用需求，確保硬體資源符合預期的功能需求。

- [ ] **如何選擇支援 Linux 或 RTOS 作業系統的 MCU？**
  - 了解不同 MCU 的處理能力、記憶體配置及處理器架構（例如是否具備記憶體管理單元 MMU 等功能）。若需要選擇支援 Linux 的 MCU，通常應考慮更高階的處理器架構，例如 ARM Cortex-A 系列，而非單純的 MCU。此部分需考量處理器架構、記憶體管理單元（MMU）的需求，以及如何整合外部儲存裝置（如 NAND Flash）來支援 Linux 系統的運行。

# 3. 中斷處理 (EXTI & NVIC)
- [ ] 使用 EXTI 觸發 GPIO 切換 LED
- [ ] 分析何時需要使用 Pulling，何時需要使用 Interrupt
- [ ] 嘗試在 RTOS 環境下使用中斷喚醒 Task

# 4. 作業系統概念

## 1. Process
### **進程管理 (Process Management)**
  - [ ] 了解進程與執行緒的區別，進程的生命週期（創建、執行、終止）。
  - [ ] 進程控制塊 (PCB) 的結構與作用。
  - [ ] 進程調度：瞭解如何在多進程環境中進行進程切換，並學習不同調度演算法的工作原理。
### **進程間通訊 (IPC)**：
  - [ ] 管道 (Pipes)、共享記憶體、訊號量 (Semaphores) 等方式的進程間通訊。

## 2. Memory
### **記憶體管理 (Memory Management)**
  - [ ] 了解靜態記憶體與動態記憶體的分配。
  - [ ] **Heap 與 Stack 的區別**：
    - [ ] Stack：堆疊區是用來儲存函數的局部變數和函數調用的返回地址，分配與釋放速度非常快。每當函數被呼叫時，系統會自動為其分配 Stack 空間，並在函數返回時釋放。
    - [ ] Heap：堆區是用來儲存動態分配的記憶體區塊，如通過 `malloc` 或 `free` 來分配與釋放記憶體。Heap 內存的管理需要開發者手動管理，並且其分配與釋放比 Stack 慢。
  - [ ] **記憶體分配演算法**：
    - [ ] 首適配 (First-fit)、最佳適配 (Best-fit)、最差適配 (Worst-fit)。
  - [ ] **頁式記憶體管理 (Paging)** 和 **分段式記憶體管理 (Segmentation)** 的概念及應用。
  - [ ] 虛擬記憶體、頁表 (Page Table) 和交換 (Swapping) 的概念。

## 3. Threading & CPU Scheduling
### **執行緒管理 (Thread Management)**
  - [ ] 了解執行緒與進程的區別，執行緒的創建、終止及調度。
  - [ ] **多執行緒編程**，特別是如何在 RTOS 中使用多執行緒來執行多任務。
### **CPU 排程 (CPU Scheduling)**
  - [ ] 了解各種 CPU 排程演算法：
    - [ ] 先到先服務 (FCFS)、
    - [ ] 最短作業優先 (SJF)、
    - [ ] 優先級排程 (Priority Scheduling)、
    - [ ] 反應時間最短 (Shortest Remaining Time)、
    - [ ] 時間片輪轉 (Round Robin)。
  - [ ] **記憶體管理與執行緒**：了解在多執行緒運行的情況下，Stack 和 Heap 的使用。每個執行緒都擁有自己的 Stack，但所有執行緒通常會共享同一個 Heap。

## 4. Synchronization & Deadlock
### **同步 (Synchronization)**
  - [ ] 理解同步的必要性，學習常見的同步原則。
  - [ ] **鎖 (Locks)**：互斥鎖 (Mutex)、自旋鎖 (Spinlock)。
  - [ ] **訊號量 (Semaphore)** 和 **條件變數 (Condition Variables)** 的使用。
### **死鎖 (Deadlock)**
  - [ ] 了解死鎖的四大必要條件：互斥、占有且等待、不釋放、循環等待。
  - [ ] 死鎖避免策略：如銀行家演算法、資源分配圖。

## 5. File System & I/O System
### **檔案系統 (File System)**
  - [ ] 了解檔案系統的基本概念，包括檔案的儲存、檔案系統的結構、檔案的操作。
  - [ ] **目錄結構**：如何組織檔案與目錄的層級結構。
  - [ ] **磁碟配置**：磁碟區與檔案分配表（FAT）、iNode 結構等。
### **I/O 系統 (I/O System)**
  - [ ] 了解 I/O 管理的基本概念及設備驅動程式的工作原理。
  - [ ] **I/O 排程**：如何有效地管理與調度 I/O 請求。

## 6. RTOS (FreeRTOS)
### **RTOS 任務排程 (Task Scheduling in RTOS)**
  - [ ] 了解 RTOS 的基本概念及其在嵌入式系統中的應用。
  - [ ] 研究如何在 RTOS 中進行多任務處理，包括任務創建、切換、優先級等。
  - [ ] **FreeRTOS 排程演算法**：了解 FreeRTOS 使用的演算法及其如何決定任務執行順序。
### **RTOS 訊號量 (Semaphore) / 佇列 (Queue) 機制**
  - [ ] 了解如何在 FreeRTOS 中使用訊號量和佇列來進行同步與通訊。
  - [ ] 學習如何在 RTOS 中進行多任務協作，並理解訊號量、佇列、事件組等機制的應用。
### **RTOS 記憶體管理 (Memory Management in RTOS)**
  - [ ] 了解 RTOS 中記憶體的分配與管理方式，特別是記憶體池、靜態/動態記憶體分配等。
  - [ ] 研究 RTOS 的記憶體管理策略，如記憶體碎片管理、任務堆疊等。

# 5. 演算法與硬體基礎補充
- [ ] **數位邏輯與硬體結構**：學習基本的數位邏輯閘、時序電路與設計，並理解硬體如何執行指令。
- [ ] **多工處理與中斷優先級**：了解如何在 MCU 中使用中斷處理多任務，並理解中斷優先級的概念。
- [ ] **資料結構**：復習常見資料結構，如陣列、鏈結串列、堆疊、佇列、樹等，以及如何在嵌入式環境中運用這些結構。
- [ ] **基礎演算法**：學習簡單排序演算法（如選擇排序、插入排序、冒泡排序）以及搜尋演算法（如二分搜尋、線性搜尋）。
- [ ] **數學基礎**：理解基本的數學運算、位運算和二進位數字操作，這對編寫高效的嵌入式系統程式碼非常重要。
- [ ] **電源管理與效能優化**：學習如何在嵌入式系統中管理電源，優化程式的效能，並理解如何根據硬體的需求進行優化。

# 6. 計算機組織 (Computer Organization)

## 計算機組織概念 (Computer Organization Concepts)
- [ ] 了解計算機的基本組成部分：中央處理單元 (CPU)、記憶體、輸入輸出設備、總線等。
- [ ] **指令集架構 (ISA)**：學習指令集架構的基本概念，了解不同指令集如 RISC、CISC 之間的區別。
- [ ] 了解處理器如何與記憶體進行交互，並理解載入、執行、儲存指令的過程。

## 中央處理單元 (CPU)
- [ ] **ALU (算術邏輯單元)**：理解 ALU 在 CPU 中的作用，並學習常見的算術運算和邏輯運算。
- [ ] **控制單元 (CU)**：了解控制單元的功能，學習它如何協調 CPU 內部的各項工作。
- [ ] **暫存器 (Registers)**：了解 CPU 中的各種暫存器，如程式計數器 (PC)、指令暫存器 (IR) 等，以及它們在指令執行中的作用。

## 記憶體組織 (Memory Organization)
- [ ] 了解記憶體的層次結構：快取記憶體 (Cache)、主記憶體 (RAM)、外部記憶體 (如硬碟)。
- [ ] **快取記憶體 (Cache Memory)**：學習快取記憶體的作用、快取層次結構以及快取命中率 (hit rate) 的概念。
- [ ] **記憶體分配與管理**：了解如何分配、管理及存取記憶體，並學習基本的記憶體保護技術。

## 指令週期 (Instruction Cycle)
- [ ] 了解 CPU 如何執行一條指令：取指、解碼、執行、寫回等步驟。
- [ ] 研究指令管線 (Pipeline) 的概念，理解指令並行執行的方式以及如何提高指令執行的效能。

## 總線架構與通訊 (Bus Architecture and Communication)
- [ ] 了解如何通過總線進行資料傳輸，並學習常見的總線架構如 PCI、I2C、SPI 等。

## 中斷與例外處理 (Interrupts and Exceptions)
- [ ] 了解中斷機制的基本概念，如何通過中斷處理異常情況或外部事件。
- [ ] 學習中斷優先級與中斷向量表的工作原理，理解如何設計高效的中斷處理程序。

## 執行緒與並行處理 (Threads and Parallel Processing)
- [ ] 了解處理器如何支援執行緒的管理，並學習多執行緒如何協同運作來提高效能。
- [ ] 學習多核處理器架構，理解如何將任務分配給多個核心進行並行處理。

# 7. 其他
- [ ] 設計一個簡單的 MCU 專案來驗證所學
- [ ] 嘗試將 MCU 專案與 RTOS 整合
- [ ] 進行模擬除錯，熟悉 STM32 除錯工具

# 8. 資料結構與演算法

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