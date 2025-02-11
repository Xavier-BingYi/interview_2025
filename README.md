# SQL 入門

## 1. 工作環境與技術概念

### 1.1 C# 與資料庫的關係

- **C# 是主要的開發語言**，負責編寫應用程式的邏輯，如處理使用者輸入、商業邏輯計算等。

- **資料庫（如 Oracle / SQL Server）**，負責儲存資料與提供查詢功能。

- **SQL（結構化查詢語言）** 是用來讓程式語言與資料庫互動的指令語言，像是「我要從資料庫取出某個人的資料」，是 C# 與資料庫溝通的語法。

> **類比：** 在韌體開發中，C# 環境就像**主站**，資料庫是**從站**，而 SQL 是**通訊協議**。

---

### 1.2 為什麼一間公司會同時使用 Oracle 與 SQL Server？

- **不同部門或歷史系統需求**：有些系統可能原本使用 Oracle，而新系統選擇 SQL Server。

- **業務需求**：某些資料庫適合處理大數據（如 Oracle），而有些則適合企業內部應用（如 SQL Server）。

- **成本與授權考量**：部分公司可能因成本因素逐步轉換資料庫。

根據 [Guru99](https://www.guru99.com/zh-CN/oracle-vs-sql-server.html) 的比較，Oracle 和 SQL Server 在多平台支持、語法、查詢優化等方面存在差異。

---

### 1.3 前端與後端的差異

| 類別   | 負責的部分                                                                 | 主要技術                                                                 |
|--------|----------------------------------------------------------------------------|--------------------------------------------------------------------------|
| 前端   | 負責使用者看到的介面（UI），如按鈕、表單、網頁畫面。 | HTML、CSS、JavaScript、React、Vue。 |
| 後端   | 負責處理邏輯與資料處理，如 API、資料庫存取、商業邏輯。 | C#（.NET）、Python、Java、SQL（資料庫）。 |

簡單來說，前端是「讓使用者看到的東西」，後端是「處理資料與邏輯，讓前端顯示正確的內容」。

後端與資料庫管理，具體內容包括：

- **開發企業內部應用系統的邏輯（C#）**：例如，建立「請假系統」，當員工提交請假申請時，系統會判斷：
  - 這個人有沒有足夠的假？
  - 這筆請假資料要不要傳給主管審核？
  - 請假成功後，要不要寄 Email 給 HR？

  這些邏輯就是後端的工作。

- **設計與管理資料庫（SQL Server / Oracle）**：設計「請假系統」的資料表，例如 LeaveRequests，裡面存放：
  - 員工編號
  - 假別（年假、病假等）
  - 開始與結束時間
  - 核准狀態

  這些資料存在哪裡、如何查詢，都是後端的工作。

- **維護與除錯（Debug）系統錯誤**：如果 HR 系統發生錯誤，例如「主管批准假單時，資料沒更新」，工程師需要：
  - 檢查 C# 程式邏輯 → 可能是沒正確更新資料庫。
  - 檢查 SQL 資料庫 → 可能是 UPDATE 指令失敗。

## 2. 開發練習

### 2.1 開發工具（IDE）

- ✅ **[Visual Studio Community](https://visualstudio.microsoft.com/)**（免費）
  - **開發 C# 後端邏輯**。
  - 支援 **Windows 應用程式、Web 應用程式**。
  - 安裝時勾選 `.NET 框架開發` & `ASP.NET 和 Web 開發`。

---

### 2.2 資料庫

- ✅ **[SQL Server Express](https://www.microsoft.com/zh-tw/sql-server/sql-server-downloads)**（免費）
  - 微軟官方 SQL Server **輕量版**，適合開發與測試。
  - **搭配管理工具**：[SQL Server Management Studio (SSMS)](https://aka.ms/ssmsfullsetup)

> **（可選）Oracle 練習環境**：[Oracle Database Express](https://www.oracle.com/database/technologies/xe-downloads.html)（學習門檻較高）

---

### 2.3 資料庫管理工具

- ✅ **[SQL Server Management Studio (SSMS)](https://aka.ms/ssmsfullsetup)**
  - 微軟官方 GUI 工具，方便管理 SQL Server。

- ✅ **[DBeaver](https://dbeaver.io/)**（可選）
  - **跨平台資料庫管理工具**，支援 SQL Server、Oracle、MySQL。

---

### 2.4 Web 伺服器（若要開發網頁版請假系統）

- ✅ **[IIS Express](https://docs.microsoft.com/en-us/iis/extensions/introduction-to-iis-express/iis-express-overview)**（內建於 Visual Studio）
  - ASP.NET MVC / Web API 開發時的測試伺服器。

- ✅ **[Postman](https://www.postman.com/)**（測試 API）
  - 若請假系統有 API，Postman 可測試 API 請求與回應。

---

### 2.5 可選工具（開發與除錯）

- ✅ **[Git](https://git-scm.com/)**（版本控制，推薦搭配 GitHub / GitLab）
- ✅ **[Docker](https://www.docker.com/)**（若要使用容器化技術管理 SQL Server 或 Web 應用程式）

---

### 2.6 開發步驟（以請假系統為例）

**設定環境**

1️⃣ **安裝 SQL Server Express 與 SSMS**，建立 **請假申請表 (LeaveRequests)**

2️⃣ **使用 Visual Studio 撰寫 C# 後端程式**

3️⃣ **撰寫 SQL 語法**，讓 C# 與資料庫溝通

4️⃣ **（可選）開發 Web 介面**，使用 ASP.NET MVC 或 Blazor

