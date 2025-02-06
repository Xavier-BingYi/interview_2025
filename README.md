
# 參考資料
- 筆記：<https://hackmd.io/@zoanana990/S1RySgR3A>
- MarkDown語法：<https://hackmd.io/@eMP9zQQ0Qt6I8Uqp2Vqy6w/SyiOheL5N/%2FBVqowKshRH246Q7UDyodFA?type=book>
- 面試常問：<https://hackmd.io/@6PPVul2mS7OX2GuGXlUCDA/behavior_questions>


# 待辦事項

## 面試計畫 (以下完成後，遞履歷)
### 1. 通訊協定與基礎練習
- **SPI / I2C / UART**：複習並實作這些通訊協定，確保能正確使用 `printf` 進行 debug。

### 2. 中斷機制理解
- **研究與實作中斷**：EXTI（外部中斷）與 NVIC（中斷控制器）的運作機制。
- **參考範例**：
  - https://github.com/zoanana990/qubitas/blob/main/driver/exti.c
  - https://github.com/zoanana990/qubitas/blob/main/driver/nvic.c

### 3. MCU 系統架構
- **STM32 系統啟動流程**：
  - 了解 STM32 的開機配置 (MCU 怎麼開機、怎麼燒進去、從哪裡開機)
  - 查看 STM32 SPEC 的 2.4 Boot configuration，確認使用的引腳和記憶體（有兩根 pin 告訴我從哪一個記憶體開機）。
- **Linker file**：
  - 理解為什麼要使用 `flash.ld` 而非 `ram.ld`。

### 4. 計算機組織 (從之乎找文章)
- **Cache**：理解快取的作用、結構及其效能影響。
    - [Cache 介紹](https://zhuanlan.zhihu.com/p/102293437?utm_psn=1837905279519420417)
- **Pipeline**：學習指令流水線處理，分析其優化效能的方法。(Pipeline就是流水線；學校教的是五極流水線；這是大陸用詞)
    - [Pipeline 介紹](https://zhuanlan.zhihu.com/p/453232311)

### 5. 作業系統 (從之乎找文章)
- **Lock 和 Semaphore**：理解在多線程環境中如何運用這些同步機制。

## 面試期間計畫 (遞完履歷到面試期間)
### 1. 編譯流程與工具
- **Makefile**：查看 Makefile，包含需要了解 `startup.c` 和 `linker.ld` 的作用。

### 2. 刷題練習
- **群聯三題**：
  - [群聯三題（1）](https://m033010041.github.io/2020/02/18/phison-interview-practice/)
  - [群聯三題（2）](https://hackmd.io/@jQSlhiN3QaGkugpbD4NDAA/rkvNkyQfi)
  
- **LeetCode** (https://leetcode.com/problem-list/linked-list/)
  1. 排序 (Sorting)
  2. Linked List（會順便練到 structure 與 queue）：*206、*707 特別重要
  3. 熟練指標 (pointer)

---

### Linked List 刷題順序與建議

#### 基礎題目（必刷）(707會的話就等於懂link list ; 146會的話就等於同時搞懂link list跟cache)

1. **Reverse Linked List** (題目 #206)  
   反轉 Linked List，是最基礎的操作題目，能幫助你熟悉如何處理 Linked List 的指標。

2. **Merge Two Sorted Lists** (題目 #21)  
   合併兩個已排序的 Linked List，這題有助於你練習如何同時遍歷兩個 Linked List 並進行操作。

3. **Linked List Cycle** (題目 #141)  
   檢查 Linked List 是否有循環，這題是經典的快慢指標（Tortoise and Hare Algorithm）題目，對學習指標操作非常有幫助。

4. **Remove Duplicates from Sorted List** (題目 #83)  
   去除排序好的 Linked List 中的重複元素，這題讓你熟悉遍歷 Linked List 並進行修改。

5. **Intersection of Two Linked Lists** (題目 #160)  
   查找兩個 Linked List 的交點，這題要求你對指標的操作更為熟練，並處理長度不同的 Linked List。

#### 進階題目（開始挑戰）

這些題目會讓你更深入了解 Linked List，並涉及一些額外的資料結構或技巧，建議在基礎題目有信心後再開始挑戰：

6. **Palindrome Linked List** (題目 #234)  
   判斷 Linked List 是否為回文，這題會涉及如何使用額外的資料結構來儲存資料，並且需要反轉部分 Linked List。

7. **Delete Node in a Linked List** (題目 #237)  
   刪除指定節點，這題讓你練習如何處理指標的修改，對基礎指標操作非常重要。

8. **Add Two Numbers** (題目 #2)  
   給定兩個由 Linked List 表示的數字，進行相加，這題可以幫助你練習如何處理數據並進行數值運算。

9. **Rearrange Linked List** (題目 #143)  
   重新排列 Linked List 的節點順序，這題能幫助你理解如何在 Linked List 中進行更複雜的操作。

10. **Remove N-th Node from End of List** (題目 #19)  
    刪除倒數第 N 個節點，這題需要用雙指標技巧來遍歷 Linked List，對指標操作的理解有很大挑戰。

11. **Swap Nodes in Pairs** (題目 #24)  
    每兩個節點交換位置，這題讓你練習如何操作兩個相鄰節點，對提升你對 Linked List 操作的熟練度有幫助。

12. **Rotate List** (題目 #61)  
    將 Linked List 旋轉 k 次，這題需要你理解如何重新安排 Linked List 的結構，挑戰指標操作技巧。

#### 進階挑戰題目（挑戰極限）

這些題目屬於較為進階的題目，會涉及多層次結構或額外的資料結構，適合當你有足夠信心後挑戰：

13. **Flatten a Multilevel Doubly Linked List** (題目 #430)  
    扁平化多層次的雙向 Linked List，這題相對較難，要求你理解雙向 Linked List 以及如何處理多層次結構。

14. **Copy List with Random Pointer** (題目 #138)  
    複製包含隨機指標的 Linked List，這題挑戰理解如何處理隨機指標，並且考驗如何複製整個 Linked List。

15. **LRU Cache** (題目 #146)  
    實現一個最近最少使用（LRU）快取系統，這題雖然與 Linked List 有關，但重點是資料結構應用，讓你加深對 Linked List 在實際應用中的理解。

---

### 初學者排序 (Sorting) 題目推薦順序 (75可以不看，因為最佳解不是使用sorting ； 會56 merge就等於會sorting 看56之前可以先看88 )

1. **Sort Colors** (題目 #75)  
   這是一個簡單的題目，讓你練習排序三個顏色，通常使用「荷蘭國旗問題」的解法。這是學習排序的好起點，能幫助你理解如何處理特殊的排序情況。

2. **Merge Intervals** (題目 #56)  
   這題與排序有關，需要先對區間進行排序，然後合併重疊的區間。這不僅可以幫助你練習排序，還能加深你對處理區間問題的理解。

3. **Insert Interval** (題目 #57)  
   這題與合併區間類似，但在此題中需要先將新的區間插入已經排序好的區間中。理解如何處理這類情況有助於你掌握排序後的資料插入技巧。

4. **Kth Largest Element in an Array** (題目 #215)  
   這題會用到排序來找到第 k 大的元素，常見的解法有使用快速排序（QuickSelect）來加速查找過程。這可以幫助你練習使用排序演算法來解決實際問題。

5. **Top K Frequent Elements** (題目 #347)  
   排序與頻率統計相結合，通過這題來練習如何利用排序來解決實際問題。這會幫助你理解排序與計數排序的應用。

6. **Find Median from Data Stream** (題目 #295)  
   雖然這題本身並不完全是排序問題，但需要用到動態排序資料結構（如最小堆和最大堆）來實現高效的中位數查找。這題是理解排序與資料結構結合的好題目。

---

#### 完成以上題目後，若還有時間，可以挑戰以下題目：

7. **Sort an Array** (題目 #912)  
   實現一個排序算法，這是一個比較基本的題目，要求你實現各種常見的排序算法，如快速排序、歸併排序等。

8. **Smallest Range II** (題目 #910)  
   在給定的一組數字中，將排序後的數字範圍縮小，這題可以幫助你練習對已排序數列的進一步處理。

9. **Wiggle Sort II** (題目 #324)  
   這是一個稍微複雜的排序題目，涉及到奇偶位置的排序，能夠挑戰你對排序和交換的理解。

## 3. 要複習的
### 更多的硬體理解：除了基本的MCU架構與組件，對具體硬體的操作、設計與調試有一定的經驗，特別是與IC設計、硬體協同開發相關的內容。
### 通訊協定與接口：像是I2C、SPI、UART等，以及更高級的網路協議，這些都是嵌入式系統中不可或缺的部分。
### 實際開發經驗：雖然理論基礎很重要，但實際操作經驗，特別是針對某些IC的驅動開發與調試，以及對微控制器平台的使用，通常更能體現一個工程師的實力。
### 程式語言與工具：熟悉C語言、RTOS、以及相關的開發工具（如IDE、編譯器、除錯工具）是基礎，但對於進階的韌體開發，還需要了解如何優化效能，進行多任務處理、資源管理等。
### 面試心得：https://www.ptt.cc/bbs/Tech_Job/M.1691075443.A.114.html

### 要讀的
1. 台達工作時 GPIO & SPI 怎麼控的
2. **SPI 的 mode (clock/phase) 與協議**、全/半雙工、baud rate，以及遇到的 bug；若除頻後的速度高於 slave 可支援的速度會發生什麼事（掉封包？）
3. **Polling task 跟 interrupt 的差異**；thread 是基於 polling（了解 thread，請 ChatGPT 寫一個 pthread sample code），查看作業系統第三、四章
4. **作業系統 Task** 的概念
5. 面試可能會問：為什麼在某些情況下選擇 polling 而非中斷

## 4. 奇景問過的問題
1. "曾經開發過甚麼專案,請畫出架構圖,之後討論。"
2. 如何確保示波器測量正確與準確？
3. "高階語言與組合語言的差別"：高階語言無需分架構，組合語言則需分架構，涉及計算機組織。
4. V_il、V_ih、V_ol、V_oh 分別代表甚麼意思？
5. 你用過什麼軟體開發工具？先問清楚問題，甚麼是軟體開發工具，例如，vscode、gcc 等，還是在問語言。
6. 專業考試的程式題包含排序，例如給一個陣列，從大排到小。
7. 請詳細描述 Interrupt Handler 與 Work Queue 的差別？為何要將工作放在 work queue 執行，而不直接在 interrupt handler 中執行？
8. 專業考試可能包含 I2C 通訊協定、C 語言排序、三用電錶及示波器操作。

## 上班後需要練習
### 1. 作業系統
- **學習課程**： [作業系統課程 (YouTube Playlist)](https://youtube.com/playlist?list=PL9jciz8qz_zyO55qECi2PD3k6lgxluYEV&si=ovEepz80kEPKwIKi)
- **學習重點**：
  - Context Switch 概念
  - IPC & Thread（可同時參考 FreeRTOS 或相關案例）
  - Multi-thread 和 Multi-process 的 IPC 概念（需熟悉第三章 Process）
  - Preemptive、Round Robin（第五章，Scheduler）
  - Mutex、Semaphore、Spinlock（第六章，建議參考 Google）
  - Deadlock 防範（第七章，建議參考 Google）
- **範例學習**：
  - [mini-arm-os - Timer Interrupt](https://github.com/jserv/mini-arm-os/tree/master/05-TimerInterrupt)
  - [mini-arm-os - Threads](https://github.com/jserv/mini-arm-os/tree/master/07-Threads)

### 2. 計算機組織
- **學習課程**：[計算機組織課程 (YouTube Playlist)](https://www.youtube.com/playlist?list=PLj6E8qlqmkFvSHyGAFqY4sX0Ee7eLqmq2)
- **計算機組織筆記**：[HackMD 筆記](https://hackmd.io/@HsuChiChen/computer_organization)

### 3. FreeRTOS
- 學習如何呼叫 FreeRTOS API，並了解其內部運作。

### 4. 專案：實作簡易作業系統 (mini-arm-os)
- **目標**：創建一個簡易作業系統，模擬基本指令操作。
  1. 實現鍵盤輸入，並在 MCU 上顯示輸入的文字（使用 Tera Term）。
  2. 開發 Command 處理邏輯，按下 Enter 後 MCU 可以執行該 Command。
  3. 類似 CMD 指令 `dir` 可顯示資訊，依據此概念，實作在 Tera Term 打字，可以跳出對應的數值。
  4. 查看所有 Process 的狀態，實現背景程式每 5 秒閃爍一次 LED；並新增指令可刪除背景程式，並允許新增新背景程式的指令。

### 5. 考古題
- **指標操作 (Pointer)**：
  - `pointer++`（指標加法）、`mem addr`（記憶體位址）、`function pointer`（函數指標）
- **值呼叫 (Call by Value) 與引用呼叫 (Call by Reference)**：
  - function 的 call by value 相關考題，通常搭配指標一起考。
  - C++ 的 Call by Reference。
- **其他重點**：
  - Static 變數的生命週期
  - Union、Struct、Enum
  - Bitfield
  - Volatile 關鍵字
  - Bitwise 操作
  - Linked List：插入 (insert)、刪除 (remove)、排序 (sort)
- **推薦資源**：
  - [HackMD 考題總結](https://hackmd.io/@Rance/SkSJL_5gX)（包含上述重點）

### 6. 若選擇軟體工程職位，需額外研讀
1. **資料結構**：軟韌當目標不用看，寫軟體要看。
2. **計算機結構**：台灣的講得不好，建議參考 Berkeley 的 CS152 課程。
3. **Linux Kernel**：Memory Management（想進google要看）


# 更新程式碼到 Git 上（使用 Sourcetree）
在使用 Git 進行版本控制時，檔案通常分為兩種類型：有追蹤的檔案和沒有追蹤的檔案。

**有追蹤的檔案**
- 有追蹤的檔案：這些檔案已經被 Git 追蹤，您可以看到哪些行被新增或刪除。

**沒有追蹤的檔案**
- 沒有追蹤的檔案：這些檔案尚未被 Git 追蹤，需要先將它們添加到 Git 的追蹤中。

## 步驟 1: 將檔案加入追蹤
首先，需要將修改過的檔案添加到 Git 的暫存區。這樣做會告訴 Git 哪些檔案的變更，應該被記錄。
```commandline=
git add <你的檔案名稱>
```
如果要添加所有修改過的檔案，可以使用：
```commandline=
git add .
```
## 步驟 2: 提交變更
在將檔案添加到暫存區後，需要撰寫提交信息（commit message）來描述這些變更。
```commandline=
git commit -m "你的提交訊息"
```
提交訊息應該簡潔明瞭，清楚描述這次變更的內容。

## 步驟 3: 上傳到伺服器
最後，可以將提交的變更上傳到遠端伺服器（如 GitHub）。
```commandline=
git push
```

# 計算機基本概念
## 計算機組織（Computer Organization）
計算機組織主要研究硬體層面的組成部分如何協同工作，包括：
- CPU 的內部架構：包含暫存器、ALU（算術邏輯單元）、控制單元等組件。
- 記憶體層次架構：包括 Cache（快取）和 RAM（隨機存取記憶體）之間的數據交換與互動。

### CPU（中央處理器）
CPU 是計算機中進行數據處理的核心元件，包含以下重要的子模塊：
1. 暫存器：CPU 內部的一組小型存儲器，用於暫時存放數據和指令，為 ALU 提供運算數據。
2. ALU（算術邏輯單元）：負責執行算術（加減乘除）和邏輯（與、或、非）運算。
3. 控制單元：負責從記憶體中抓取指令，並協調 CPU 內部的各個組件來執行這些指令。

以上這些組件都集成在同一顆 CPU IC（集成電路）中，它們共同協助完成數據處理和指令執行。

### RAM（隨機存取記憶體）
RAM 是 CPU 外部的一顆獨立 IC，用於存儲程式和數據。CPU 通過總線（Bus）與 RAM 進行連接，讀取需要處理的數據和指令，並將處理結果回寫到 RAM。

### Cache（快取記憶體）
Cache 是位於 CPU 內的一部分小型快速存儲器，負責暫存經常使用的數據。它的目的是減少 CPU 與 RAM 之間的訪問延遲，加快數據存取速度。Cache 通常包含在 CPU IC 中，幫助提升 CPU 整體運行效率。

## 計算機結構（Computer Architecture）
計算機結構偏向於設計和優化計算機系統，使其具備更高的效能。主要涉及以下內容：

1. 指令集架構（ISA）：定義 CPU 如何理解和執行指令。指令集是處理器與軟體之間的界面。
2. 處理器設計原理：研究如何提高處理效能，採用技術如超純量架構（同時執行多條指令）和管線化（分段處理指令）。
3. 記憶體管理：包括多層次的記憶體系統設計，如 Cache 層次結構和虛擬記憶體，目的是更有效地管理數據存取。

## 嵌入式系統（Embedded Systems）
嵌入式系統專注於在特定應用中使用處理器和記憶體的設計。通常會涉及部分 CPU 的內部架構和記憶體層次知識，並且強調資源有限環境下的效能優化。嵌入式系統通常會搭配特殊的處理器和記憶體組件，實現特定功能，例如物聯網裝置、工業控制系統等。
