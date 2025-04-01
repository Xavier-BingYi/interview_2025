# **1. Array**：適合按索引快速存取。
**學習重點**：先熟悉基本資料結構操作，掌握陣列的增刪改查技巧，並熟練前綴和、滑動視窗、單調堆疊等進階技巧。  

## **基礎陣列操作**
- [ ] **Easy** [Two Sum](https://leetcode.com/problems/two-sum/) (#1) - **經典 Hash Table + Array 應用**
    - 暴力解法
    ```cpp
    class Solution {
    public:
        vector<int> twoSum(vector<int>& nums, int target) {
            for (int i = 0; i < nums.size(); i++) {
                int complement = target - nums[i];
                for (int j = i + 1; j < nums.size(); j++) {
                    if (nums[j] == complement) {
                        return {i, j};
                    }
                }
            }
            return {};
        }
    };
    ```
    - 優化解法（使用哈希表）
    ```cpp

    ```
- [ ] **Easy** [Remove Duplicates from Sorted Array](https://leetcode.com/problems/remove-duplicates-from-sorted-array/) (#26) - **雙指標操作**
- [ ] **Easy** [Contains Duplicate](https://leetcode.com/problems/contains-duplicate/) (#217) - **基本查找**
- [ ] **Easy** [Intersection of Two Arrays II](https://leetcode.com/problems/intersection-of-two-arrays-ii/) (#350) - **Array + Hash Table**
- [ ] **Easy** [Move Zeroes](https://leetcode.com/problems/move-zeroes/) (#283) - **雙指標**
- [ ] **Medium** [Rotate Array](https://leetcode.com/problems/rotate-array/) (#189) - **環狀替換法**

## **最佳化計算**
- [ ] **Easy** [Best Time to Buy and Sell Stock](https://leetcode.com/problems/best-time-to-buy-and-sell-stock/) (#121) - **貪婪演算法**
- [ ] **Medium** [Maximum Product Subarray](https://leetcode.com/problems/maximum-product-subarray/) (#152) - **DP**
- [ ] **Medium** [Find Minimum in Rotated Sorted Array](https://leetcode.com/problems/find-minimum-in-rotated-sorted-array/) (#153) - **二分搜尋**
- [ ] **Medium** [Search in Rotated Sorted Array](https://leetcode.com/problems/search-in-rotated-sorted-array/) (#33) - **二分搜尋變形**
- [ ] **Hard** [Subarray Sum Equals K](https://leetcode.com/problems/subarray-sum-equals-k/) (#560) - **前綴和 + Hash Table**
- [ ] **Medium** [Product of Array Except Self](https://leetcode.com/problems/product-of-array-except-self/) (#238) - **前綴乘積**

## **滑動視窗**
- [ ] **Medium** [Longest Substring Without Repeating Characters](https://leetcode.com/problems/longest-substring-without-repeating-characters/) (#3) - **滑動視窗**
- [ ] **Hard** [Sliding Window Maximum](https://leetcode.com/problems/sliding-window-maximum/) (#239) - **單調佇列**
- [ ] **Medium** [Minimum Size Subarray Sum](https://leetcode.com/problems/minimum-size-subarray-sum/) (#209) - **雙指標**

## **位元運算（Bit Manipulation）**
- [ ] **Easy** [Single Number](https://leetcode.com/problems/single-number/) (#136) - **XOR 位元運算**
- [ ] **Easy** [Majority Element](https://leetcode.com/problems/majority-element/) (#169) - **Boyer-Moore 投票法**
- [ ] **Easy** [Missing Number](https://leetcode.com/problems/missing-number/) (#268) - **數學 XOR**
- [ ] **Medium** [Reverse Bits](https://leetcode.com/problems/reverse-bits/) (#190) - **位元翻轉**
- [ ] **Easy** [Number of 1 Bits](https://leetcode.com/problems/number-of-1-bits/) (#191) - **位元計數**
- [ ] **Medium** [Counting Bits](https://leetcode.com/problems/counting-bits/) (#338) - **DP + Bitwise**

## **矩陣（Matrix）**
- [ ] **Medium** [Spiral Matrix](https://leetcode.com/problems/spiral-matrix/) (#54) - **矩陣遍歷**
- [ ] **Medium** [Set Matrix Zeroes](https://leetcode.com/problems/set-matrix-zeroes/) (#73) - **矩陣標記法**
- [ ] **Hard** [Game of Life](https://leetcode.com/problems/game-of-life/) (#289) - **狀態壓縮**
- [ ] **Medium** [Rotate Image](https://leetcode.com/problems/rotate-image/) (#48) - **矩陣旋轉**

---

# **2. Hash Table**：用於快速查找鍵值對 (key, value)。
**學習重點**：理解哈希表的基本操作，如何有效地解決查詢和衝突問題，並熟悉進階應用，如雙雜湊、位元運算和 LRU Cache。  

## **基礎哈希操作**
- [ ] **Easy** [Valid Anagram](https://leetcode.com/problems/valid-anagram/) (#242) - **字母計數**
- [ ] **Easy** [Intersection of Two Arrays II](https://leetcode.com/problems/intersection-of-two-arrays-ii/) (#350) - **Hash Table + 多集合查找**
- [ ] **Easy** [Contains Duplicate](https://leetcode.com/problems/contains-duplicate/) (#217) - **簡單查找**
- [ ] **Easy** [Contains Duplicate II](https://leetcode.com/problems/contains-duplicate-ii/) (#219) - **滑動視窗 + Hash Map**
- [ ] **Medium** [Group Anagrams](https://leetcode.com/problems/group-anagrams/) (#49) - **字串分組**
- [ ] **Easy** [Isomorphic Strings](https://leetcode.com/problems/isomorphic-strings/) (#205) - **雙向映射**
- [ ] **Easy** [Word Pattern](https://leetcode.com/problems/word-pattern/) (#290) - **模式匹配**

## **進階哈希技術**
- [ ] **Easy** [Two Sum](https://leetcode.com/problems/two-sum/) (#1) - **Array + Hash Table 經典應用**
- [ ] **Medium** [Four Sum II](https://leetcode.com/problems/4sum-ii/) (#454) - **雙雜湊 Hashing**
- [ ] **Medium** [Subarray Sum Equals K](https://leetcode.com/problems/subarray-sum-equals-k/) (#560) - **前綴和 + Hash Table**
- [ ] **Medium** [Longest Consecutive Sequence](https://leetcode.com/problems/longest-consecutive-sequence/) (#128) - **Hash Set 優化查找**
- [ ] **Medium** [Top K Frequent Elements](https://leetcode.com/problems/top-k-frequent-elements/) (#347) - **Heap + Hash Map**
- [ ] **Easy** [Happy Number](https://leetcode.com/problems/happy-number/) (#202) - **數字循環 + Hash Set**

## **設計類**
- [ ] **Medium** [LRU Cache](https://leetcode.com/problems/lru-cache/) (#146) - **雙向鏈結串列 + Hash Map**
- [ ] **Medium** [Design HashMap](https://leetcode.com/problems/design-hashmap/) (#706) - **模擬 Hash Table**
- [ ] **Hard** [Design Twitter](https://leetcode.com/problems/design-twitter/) (#355) - **優先佇列 + Hash Map**

---

# **3. Linked List**：方便進行插入和刪除操作。
**學習重點**：掌握常見指針操作，熟悉鏈結串列中新增、刪除和查找節點的基本操作，並熟練掌握遞迴與雙向鏈結串列的運用。  

## **基礎操作**
- [ ] **Easy** [Reverse Linked List](https://leetcode.com/problems/reverse-linked-list/) (#206) - **反轉鏈結串列**
- [ ] **Easy** [Merge Two Sorted Lists](https://leetcode.com/problems/merge-two-sorted-lists/) (#21) - **合併兩個排序鏈結串列**
- [ ] **Easy** [Linked List Cycle](https://leetcode.com/problems/linked-list-cycle/) (#141) - **檢查環狀鏈結串列**
- [ ] **Medium** [Remove Nth Node From End of List](https://leetcode.com/problems/remove-nth-node-from-end-of-list/) (#19) - **刪除倒數第N個節點**
- [ ] **Medium** [Reorder List](https://leetcode.com/problems/reorder-list/) (#143) - **重排鏈結串列**

## **進階操作**
- [ ] **Medium** [Add Two Numbers](https://leetcode.com/problems/add-two-numbers/) (#2) - **兩個數字加法**
- [ ] **Medium** [Flatten a Multilevel Doubly Linked List](https://leetcode.com/problems/flatten-a-multilevel-doubly-linked-list/) (#430) - **多層雙向鏈結串列扁平化**
- [ ] **Medium** [Copy List with Random Pointer](https://leetcode.com/problems/copy-list-with-random-pointer/) (#138) - **隨機指針複製**
- [ ] **Medium** [Linked List Cycle II](https://leetcode.com/problems/linked-list-cycle-ii/) (#142) - **找出環狀鏈結串列的入口**
- [ ] **Easy** [Intersection of Two Linked Lists](https://leetcode.com/problems/intersection-of-two-linked-lists/) (#160) - **找出兩個鏈結串列的交點**

## **進階設計問題**
- [ ] **Medium** [Design Linked List](https://leetcode.com/problems/design-linked-list/) (#707) - **設計鏈結串列** 

---

# **4. Stack**：處理特定的操作順序。
**學習重點**：掌握堆疊操作，理解堆疊在特定問題中的應用，特別是括號配對和最小元素追蹤。

## **基礎操作**
- [ ] **Easy** [Valid Parentheses](https://leetcode.com/problems/valid-parentheses/) (#20) - **括號配對問題**
- [ ] **Easy** [Min Stack](https://leetcode.com/problems/min-stack/) (#155) - **最小堆疊問題**

## **進階應用**
- [ ] **Medium** [Daily Temperature](https://leetcode.com/problems/daily-temperatures/) (#739) - **日溫問題**：使用堆疊解決每個日子後的較高溫度
- [ ] **Easy** [Next Greater Element I](https://leetcode.com/problems/next-greater-element-i/) (#496) - **下一個更大的元素問題**
- [ ] **Hard** [Largest Rectangle in Histogram](https://leetcode.com/problems/largest-rectangle-in-histogram/) (#84) - **直方圖中的最大矩形面積**
- [ ] **Medium** [Evaluate Reverse Polish Notation](https://leetcode.com/problems/evaluate-reverse-polish-notation/) (#150) - **逆波蘭表示法求值**
- [ ] **Medium** [Valid Parenthesis String](https://leetcode.com/problems/valid-parenthesis-string/) (#678) - **有效的括號字串問題**

## **進階設計問題**
- [ ] **Easy** [Implement Stack using Queues](https://leetcode.com/problems/implement-stack-using-queues/) (#225) - **使用佇列實現堆疊**
- [ ] **Medium** [Design Browser History](https://leetcode.com/problems/design-browser-history/) (#1472) - **設計瀏覽器歷史紀錄**：使用堆疊來記錄網頁歷史

---

# **5. Tree**：應用於層次結構的資料管理。  
**學習重點**：挑戰遞迴與基礎遍歷，理解樹結構的遍歷方法（前序、中序、後序）。  

## **基本操作**
- [ ] **Easy** [Maximum Depth of Binary Tree](https://leetcode.com/problems/maximum-depth-of-binary-tree/) (#104) - **二叉樹最大深度**
- [ ] **Easy** [Symmetric Tree](https://leetcode.com/problems/symmetric-tree/) (#101) - **對稱樹**
- [ ] **Easy** [Invert Binary Tree](https://leetcode.com/problems/invert-binary-tree/) (#226) - **翻轉二叉樹**

## **進階問題**
- [ ] **Medium** [Binary Tree Level Order Traversal](https://leetcode.com/problems/binary-tree-level-order-traversal/) (#102) - **二叉樹層次遍歷**
- [ ] **Medium** [Convert Sorted Array to Binary Search Tree](https://leetcode.com/problems/convert-sorted-array-to-binary-search-tree/) (#108) - **將排序數組轉換為二叉搜尋樹**
- [ ] **Easy** [Path Sum](https://leetcode.com/problems/path-sum/) (#112) - **路徑總和問題**
- [ ] **Medium** [Binary Tree Zigzag Level Order Traversal](https://leetcode.com/problems/binary-tree-zigzag-level-order-traversal/) (#103) - **二叉樹之字形層次遍歷**

## **進階設計問題**
- [ ] **Hard** [Serialize and Deserialize Binary Tree](https://leetcode.com/problems/serialize-and-deserialize-binary-tree/) (#297) - **二叉樹的序列化與反序列化**

---

# **6. Queue**：處理特定的操作順序。  
**學習重點**：理解佇列的運作方式，學會用堆疊實現佇列等常見操作。  

## **基本操作**
- [ ] **Easy** [Implement Queue using Stacks](https://leetcode.com/problems/implement-queue-using-stacks/) (#232) - **用堆疊實現佇列**

## **進階應用**
- [ ] **Medium** [My Circular Queue](https://leetcode.com/problems/design-circular-queue/) (#622) - **圓形佇列設計問題**
- [ ] **Medium** [Design Hit Counter](https://leetcode.com/problems/design-hit-counter/) (#362) - **計數器設計**：使用佇列進行歷史記錄
- [ ] **Hard** [Sliding Window Maximum](https://leetcode.com/problems/sliding-window-maximum/) (#239) - **滑動視窗的最大值**：佇列應用於滑動視窗問題

---

# **7. String**：掌握字串處理技巧，了解字串比對和處理的常用方法。  
**學習重點**：掌握字串處理技巧，了解字串比對和處理的常用方法。  

- [ ] **Easy** [Longest Common Prefix](https://leetcode.com/problems/longest-common-prefix/) (#14) - **最長公共前綴**
- [ ] **Easy** [Valid Palindrome](https://leetcode.com/problems/valid-palindrome/) (#125) - **有效的回文**
- [ ] **Easy** [Reverse String](https://leetcode.com/problems/reverse-string/) (#344) - **反轉字串**
- [ ] **Medium** [String to Integer (atoi)](https://leetcode.com/problems/string-to-integer-atoi/) (#8) - **字串轉整數 (atoi)**
- [ ] **Medium** [Count and Say](https://leetcode.com/problems/count-and-say/) (#38) - **數字與字串的轉換問題**
- [ ] **Medium** [Longest Substring Without Repeating Characters](https://leetcode.com/problems/longest-substring-without-repeating-characters/) (#3) - **無重複字元的最長字串**

---

# **8. Sort**：了解排序演算法的基本概念，練習排序操作的效率和應用。  
**學習重點**：了解排序演算法的基本概念，練習排序操作的效率和應用。  

- [ ] **Easy** [Merge Sorted Array](https://leetcode.com/problems/merge-sorted-array/) (#88) - **合併排序陣列**
- [ ] **Easy** [Sort Colors](https://leetcode.com/problems/sort-colors/) (#75) - **排序顏色**
- [ ] **Medium** [Kth Largest Element in an Array](https://leetcode.com/problems/kth-largest-element-in-an-array/) (#215) - **陣列中的第K大元素**
- [ ] **Medium** [Find the Duplicate Number](https://leetcode.com/problems/find-the-duplicate-number/) (#287) - **尋找重複數字**
- [ ] **Medium** [Top K Frequent Elements](https://leetcode.com/problems/top-k-frequent-elements/) (#347) - **數組中出現頻率前K的元素**

---

# **9. Binary Search**：理解二分搜尋的基本原理，熟練掌握在有序數列中的查找技巧。  
**學習重點**：理解二分搜尋的基本原理，熟練掌握在有序數列中的查找技巧。  

- [ ] **Easy** [Binary Search](https://leetcode.com/problems/binary-search/) (#704) - **二分搜尋**
- [ ] **Medium** [First Bad Version](https://leetcode.com/problems/first-bad-version/) (#278) - **第一個壞版本**
- [ ] **Easy** [Search Insert Position](https://leetcode.com/problems/search-insert-position/) (#35) - **搜尋插入位置**
- [ ] **Medium** [Find Minimum in Rotated Sorted Array](https://leetcode.com/problems/find-minimum-in-rotated-sorted-array/) (#153) - **尋找旋轉排序數列中的最小值**
- [ ] **Medium** [Search in Rotated Sorted Array](https://leetcode.com/problems/search-in-rotated-sorted-array/) (#33) - **搜尋旋轉排序數列**
