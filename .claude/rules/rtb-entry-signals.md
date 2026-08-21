---
description: Rich Trading Bot entry signal strategies — apply when working with CheckEntry, GetSignal, signal modes (EMA, BZ Zone, Ichimoku, Bollinger Bands, Simulated), or InpDirection logic.
globs: ["Custumer/RichTradingBot/**"]
alwaysApply: false
---

# Rich Trading Bot — Chiến Lược Vào Lệnh

## Hướng Giao Dịch (`InpDirection`)

| Giá trị | Enum | Hành vi |
| :--- | :--- | :--- |
| `Both` | `DIR_BOTH` | Vào cả BUY lẫn SELL theo tín hiệu |
| `Only Buy` | `DIR_ONLY_BUY` | Chỉ vào BUY, bỏ qua tín hiệu SELL |
| `Only Sell` | `DIR_ONLY_SELL` | Chỉ vào SELL, bỏ qua tín hiệu BUY |
| `Either` | `DIR_EITHER` | Tương tự `Both` (dùng kết hợp với Simulated) |

`GetSignal()` trả về: `+1` = BUY, `-1` = SELL, `0` = không có tín hiệu, `2` = cả hai (chỉ Simulated).

## Chiến Lược Tín Hiệu (`InpSignalMode`)

### 1. EMA Filter (`SIG_EMA`) — EMA 34 + 89
- **BUY:** EMA34 cắt lên EMA89 (**golden cross**) **HOẶC** giá hồi về cách EMA34 ≤ `InpEMAPullbackPts` points khi EMA34 > EMA89.
- **SELL:** EMA34 cắt xuống EMA89 (**death cross**) **HOẶC** giá hồi về cách EMA34 ≤ `InpEMAPullbackPts` points khi EMA34 < EMA89.
- Dùng bars 0–2 theo series; cross phát hiện tại bar 1 → 2.

### 2. BZ Zone Trader (`SIG_BZ_ZONE`)
- **BUY:** 3 nến xanh liên tiếp (Demand Zone — lực mua mạnh).
- **SELL:** 3 nến đỏ liên tiếp (Supply Zone — lực bán mạnh).
- **Nến xám / hỗn hợp:** Không có tín hiệu, đứng ngoài thị trường.
- Đọc 3 nến **đã đóng** (bar 1–3), tránh repaint.

### 3. Ichimoku Kinko Hyo (`SIG_ICHIMOKU`)
- **BUY:** Giá > Kumo Top **VÀ** Tenkan cắt lên Kijun **VÀ** Chikou > giá cách 26 bar.
- **SELL:** Giá < Kumo Bottom **VÀ** Tenkan cắt xuống Kijun **VÀ** Chikou < giá cách 26 bar.
- Buffer: 0=Tenkan, 1=Kijun, 2=SpanA, 3=SpanB, 4=Chikou.

### 4. Bollinger Bands (`SIG_BB`) — Đánh Đảo Chiều
- **BUY:** Close bar 1 ≤ Lower Band → chạm/phá đáy dải.
- **SELL:** Close bar 1 ≥ Upper Band → chạm/phá đỉnh dải.
- Tham số: `InpBBPeriod` (20), `InpBBDev` (2.0).

### 5. Simulated (`SIG_SIMULATED`) — Vào Lệnh Ngay
Không chờ tín hiệu kỹ thuật. Dùng để test nhanh hoặc vào lệnh thủ công.

| `InpDirection` | Hành vi |
| :--- | :--- |
| `Only Buy` | Mở 1 lệnh BUY khi chưa có lệnh BUY |
| `Only Sell` | Mở 1 lệnh SELL khi chưa có lệnh SELL |
| `Both` / `Either` | Mở **cả BUY lẫn SELL** đồng thời, mỗi hướng quản lý DCA/Trail/Trim độc lập |

### 6. Candle (`SIG_CANDLE`) — Theo Màu Nến Khung Hiện Tại
- **BUY:** Nến **đã đóng** (bar 1) trên `InpSignalTF` là nến tăng (`close > open`).
- **SELL:** Nến **đã đóng** (bar 1) trên `InpSignalTF` là nến giảm (`close < open`).
- Mỗi nến đóng đều phát tín hiệu (không cần cross/pullback như EMA, không cần 3 nến liên tiếp như BZ Zone).
- Không có xử lý riêng: nếu chưa có lệnh gốc theo chiều đó, `CheckEntry()` mở **lệnh đầu tiên** cùng chiều màu nến qua `TryOpenBuy()`/`TryOpenSell()` (chỉ mở khi `CountBuy()==0`/`CountSell()==0`, như mọi mode khác). Sau khi có lệnh, DCA (`CheckDCA()`, chiều lỗ — "âm") và Pyramiding (`CheckPyramiding()`, chiều lãi — "dương") quản lý tiếp theo cơ chế chuẩn ở [rtb-dca.md](rtb-dca.md) / [rtb-pyramiding.md](rtb-pyramiding.md), tính từ giá lệnh gốc này.

## Luồng `CheckEntry()`

```
OnTick → CheckEntry()
  → kiểm tra InpOrderDelay
  → GetSignal() [áp dụng InpDirection filter]
  → sig=2: TryOpenBuy() + TryOpenSell()
  → sig>0: TryOpenBuy()   [chỉ mở nếu CountBuy()==0]
  → sig<0: TryOpenSell()  [chỉ mở nếu CountSell()==0]
```

> Lệnh gốc chỉ mở **1 lệnh mỗi hướng**. Khi đã có lệnh → DCA (`CheckDCA()`) xử lý thêm.
