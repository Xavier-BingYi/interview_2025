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

- **成本與授權考量**：部分公司可能因成本考量逐步轉換資料庫。Oracle 的授權模式較昂貴，企業版需依 CPU 核心數計費，且部分進階功能需額外付費。相較之下，SQL Server 的標準版已涵蓋許多企業應用功能，授權與維護成本較低，因此公司可能在部分業務上選擇 SQL Server 以降低開支。

---

### 1.3 前端與後端的差異

| 類別   | 負責的部分                                                                 | 主要技術                                                                 |
|--------|----------------------------------------------------------------------------|--------------------------------------------------------------------------|
| 前端   | 負責使用者看到的介面（UI），如按鈕、表單、網頁畫面。 | HTML、CSS、JavaScript、React、Vue。 |
| 後端   | 負責處理邏輯與資料處理，如 API、資料庫存取、商業邏輯。 | C#（.NET）、Python、Java、SQL（資料庫）。 |

簡單來說，**前端是「讓使用者看到的東西」，後端是「處理資料與邏輯，讓前端顯示正確的內容」。**

---

### 前端與後端的分工

#### **前端（Frontend）**  
前端的主要工作是設計使用者介面（UI），並確保操作流暢，例如在「請假系統」中，前端負責：

- **登入畫面**：讓員工輸入帳號密碼登入系統。  
- **請假申請表單**：提供假別選擇、填寫請假時間，並提交申請。  
- **請假審核介面**：讓主管查看請假紀錄，並提供「批准」或「拒絕」按鈕。  
- **即時錯誤提示**：如「開始時間比結束時間晚」，前端會立即顯示錯誤訊息，避免無效請求送到後端。

#### **後端與資料庫管理（Backend）**  
後端負責處理系統邏輯、管理資料庫，確保系統運作正常，例如：

1. **開發應用系統的邏輯（C# / Java / Python）**  
   以「請假系統」為例，當員工提交請假申請時，後端會判斷：
   - **假期是否足夠？**  
   - **是否需要主管審核？**  
   - **請假成功後，是否寄 Email 給 HR？**  

   這些邏輯由後端負責執行與管理。  

2. **設計與管理資料庫（SQL Server / Oracle）**  
   請假系統的資料存放在資料庫中，例如 `LeaveRequests` 資料表，包含：
   - 員工編號  
   - 假別（年假、病假等）  
   - 開始與結束時間  
   - 核准狀態  

   後端需要確保資料的正確儲存、讀取與更新。  

3. **系統維護與除錯（Debug）**  
   如果 HR 系統發生錯誤，例如「主管批准假單時，資料沒更新」，工程師需要：
   - **檢查 C# / Java / Python 程式邏輯** → 可能是未正確更新資料庫。  
   - **檢查 SQL 資料庫** → 可能是 `UPDATE` 指令失敗，或權限不足導致無法更新資料。

---

### 前後端如何溝通？  
前端與後端透過 **API（Application Programming Interface）** 進行溝通：

1. **前端發送請求（Request）**，例如員工點擊「提交請假」後，系統發送 API 請求至後端。  
2. **後端處理請求**，驗證資料、更新資料庫，並回傳處理結果。  
3. **前端接收回應（Response）**，顯示「請假成功」或「請假失敗」的通知給使用者。  

這樣的合作方式確保系統能夠順暢運作，並即時提供使用者正確的資訊。

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

---

### 2.3 資料庫管理工具

- ✅ **[SQL Server Management Studio (SSMS)](https://aka.ms/ssmsfullsetup)**
  - 微軟官方 GUI 工具，方便管理 SQL Server。

---

### 2.4 開發步驟（以聯絡人管理系統為例）

**設定環境**

1️⃣ **建立資料庫與資料表**

   - 使用 SSMS 建立名為 `ContactDB` 的資料庫。
   - 在 `ContactDB` 中建立名為 `Contacts` 的資料表，包含以下欄位：
     - `Id`（主鍵，自動增量）
     - `Name`（聯絡人姓名）
     - `Phone`（聯絡人電話）
     - `Email`（聯絡人電子郵件）

2️⃣ **使用 Visual Studio 撰寫 C# 後端程式**

   - 建立一個新的 C# 專案，並引用 `Microsoft.Data.SqlClient` 套件。
   - 實作以下功能：
     - **新增聯絡人**：將使用者輸入的姓名、電話和電子郵件新增到 `Contacts` 資料表。
     - **查詢聯絡人**：根據姓名查詢聯絡人的詳細資訊。
     - **更新聯絡人**：根據聯絡人 ID 更新其姓名、電話或電子郵件。
     - **刪除聯絡人**：根據聯絡人 ID 刪除其資料。

3️⃣ **撰寫 SQL 語法，讓 C# 與資料庫溝通**

   - 使用參數化查詢來防止 SQL 注入攻擊。
   - 例如，新增聯絡人的 SQL 語法：
     ```csharp
     string insertQuery = "INSERT INTO Contacts (Name, Phone, Email) VALUES (@Name, @Phone, @Email)";
     using (SqlCommand command = new SqlCommand(insertQuery, connection))
     {
         command.Parameters.AddWithValue("@Name", name);
         command.Parameters.AddWithValue("@Phone", phone);
         command.Parameters.AddWithValue("@Email", email);
         command.ExecuteNonQuery();
     }
     ```

4️⃣ **（可選）開發 Web 介面**

   - 使用 ASP.NET MVC 或 Blazor 建立簡單的 Web 介面，讓使用者可以透過瀏覽器進行聯絡人管理操作。

---

### 2.5 參考資源

- **教學課程：實作 CRUD 功能 - ASP.NET MVC 搭配 EF Core**
  - 這篇教學詳細介紹了如何在 ASP.NET MVC 中實作 CRUD（建立、讀取、更新、刪除）功能，並使用 Entity Framework Core 進行資料庫操作。
  - :contentReference[oaicite:0]{index=0}

- **影片教學：Complete CRUD Operation in Asp.Net C# With SQL Server**
  - 這段影片教學展示了如何在 ASP.NET C# 中完成 CRUD 操作，並與 SQL Server 整合。
  - 


