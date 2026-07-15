---
description: Rich Trading Bot DCA (Dollar Cost Averaging / Martingale) logic — apply when working with CheckDCA, DCA levels, DCA_MaxOrd, DCA distance calculation, or DCA TP/SL behavior.
globs: ["Custumer/RichTradingBot/**"]
alwaysApply: false
---

# Rich Trading Bot — Chiến Lược DCA

## Nguyên Tắc Hoạt Động

- Hỗ trợ **8 tầng DCA** tùy chỉnh độc lập.
- Khoảng cách tính từ **lệnh cuối cùng được mở** (không phải lệnh gốc).
- Chạy trong `OnTimer()` (mỗi giây), riêng biệt cho BUY và SELL.

## Chế Độ DCA (`ENUM_DCA_MODE`)

| Mode | Hành vi |
| :--- | :--- |
| `DCA_STOP` | Dừng DCA — không mở thêm tại tầng này |
| `DCA_STEP` | Mở khi giá cách lệnh cuối ≥ `Dist` points |
| `DCA_STEP_TF` | Như `DCA_STEP` nhưng **bắt buộc** có xác nhận tín hiệu chiến lược hiện tại cùng chiều |

## Thuật Toán Xác Định Tầng (Cumulative MaxOrd)

```cpp
dcaCount = count - 1  // số lệnh DCA đã mở (không tính lệnh gốc)

// Tích lũy MaxOrd qua từng tầng để tìm lvl hiện tại
cumulative = 0
for i in 0..7:
    nextCum = cumulative + DCA_MaxOrd[i]
    if dcaCount < nextCum → lvl = i, break
    cumulative = nextCum
```

> `DCA_MaxOrd[i]` = số lần tầng `i` được phép kích hoạt trước khi chuyển tầng tiếp.

## Bảng Cấu Hình Mặc Định

| Tầng | Mode | Hệ số Lot | Số lần mở | Khoảng cách (pts) | TP (pts) | SL (pts) |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| T1 | Step | 1.5x | 2 | 1000 | 500 | 0 |
| T2 | Step | 2.0x | 2 | 1500 | 500 | 0 |
| T3 | Step | 2.5x | 2 | 2000 | 500 | 0 |
| T4 | Step | 3.0x | 2 | 2500 | 500 | 0 |
| T5 | Step | 3.5x | 2 | 3000 | 500 | 0 |
| T6 | Step | 4.0x | 2 | 3500 | 500 | 0 |
| T7 | Step | 5.0x | 2 | 4000 | 500 | 0 |
| T8 | Stop | 6.0x | 1 | 5000 | 500 | 0 |

## Mapping dcaCount → Tầng (Cấu Hình Mặc Định)

| dcaCount | Tầng | Khoảng cách | Hệ số Lot |
| :--- | :--- | :--- | :--- |
| 0, 1 | T1 | 1000pt | 1.5x |
| 2, 3 | T2 | 1500pt | 2.0x |
| 4, 5 | T3 | 2000pt | 2.5x |
| 6, 7 | T4 | 2500pt | 3.0x |
| 8, 9 | T5 | 3000pt | 3.5x |
| 10, 11 | T6 | 3500pt | 4.0x |
| 12, 13 | T7 | 4000pt | 5.0x |
| 14 | T8 | 5000pt | 6.0x → Stop |

**Tổng tối đa:** 1 lệnh gốc + 15 lệnh DCA = **16 lệnh** (còn bị giới hạn bởi `InpMaxBuy` / `InpMaxSell`).

## TP/SL DCA

- DCA orders **luôn đặt server TP/SL** nếu giá trị > 0, **bỏ qua** `InpUseTakeProfit` / `InpUseStopLoss`.
- Ngoài ra còn được kiểm tra bởi code trong `CheckExit()` section 2a (safety net).
- Comment lệnh DCA: `"RTB|<tp_pts>|<sl_pts>"` — bot đọc để đóng đúng tầng.

## Điều Kiện Kích Hoạt DCA

```
BUY DCA:  (lastOpenPrice - bid) >= DCA_Dist[lvl] * point
SELL DCA: (ask - lastOpenPrice) >= DCA_Dist[lvl] * point
```

Sau khi trigger, còn kiểm tra thêm:
1. `DCA_STEP_TF`: tín hiệu cùng chiều từ `GetSignal()`
2. `InpOrderDelay`: thời gian chờ giữa 2 lệnh
3. `count < InpMaxBuy/Sell`: chưa đạt giới hạn tối đa

## Lệnh Gốc Bị Cắt Giữa Chừng — `CheckOrigRestart()`

Lệnh gốc (comment đúng `"RTB|0|0"`) **không nằm trong slot DCA** — `RebuildDCAState()` loại trừ nó, và `CheckDCA()` chỉ quản lý các tầng DCA (`DCABuyPrices[]`/`DCASellPrices[]`), không quản lý lệnh gốc.

`OrigBuyPrice`/`OrigSellPrice` ghi nhớ giá lệnh gốc mỗi khi `TryOpenBuy()`/`TryOpenSell()` mở nó. Mỗi `OnTimer()`, `CheckOrigRestart(posType)` kiểm tra: nếu biến này khác 0 nhưng không còn vị thế `"RTB|0|0"` nào đang mở (nghĩa là lệnh gốc vừa bị cắt vì bất kỳ lý do gì — TP/SL/Trailing/`InpClosePerPips`/nút Close Profit/Close Loss/đóng thủ công) thì:

1. **Không đặt lệnh chờ (Limit/Stop) ở giá cũ.**
2. Gọi `ResetDCAState(posType)` — huỷ sạch mọi lệnh chờ (refill tầng + tầng mới) đang neo của chiều đó, reset `PeakDCABuy/Sell` và toàn bộ mảng `DCABuyPrices/Tickets/LimitTk[]`.
3. Mở **ngay lập tức bằng market** một lệnh gốc mới (`OpenOrder`) tại giá hiện tại.
4. Ghi giá lệnh gốc mới vào `OrigBuyPrice`/`OrigSellPrice` — chuỗi DCA tiếp theo sẽ tính từ lệnh gốc MỚI này (qua `LastOpenPrice()` vì `peak` đã về 0), không dùng lại lịch sử giá cũ.

**Không kích hoạt** khi `OrigBuyPrice`/`OrigSellPrice` đã bị xoá về 0 — xảy ra khi lệnh gốc đóng thông qua `CloseAll()` (nút Close All/Close Buy/Close Sell, Day Limit, Hedge Cut, hoặc Basket `InpCloseProfit`/`InpCloseLoss`) — các đường này chủ động xoá cờ để coi là "reset toàn bộ", chờ tín hiệu mới (`CheckEntry()`) mở lệnh gốc tiếp theo thay vì tự động mở lại ngay.

Chạy **trước** guard reset-toàn-phần đầu `OnTimer()` để vẫn xử lý được cả trường hợp lệnh gốc là vị thế **cuối cùng** của chiều đó khi bị cắt (không chờ tín hiệu mới như bình thường).
