---
description: Rich Trading Bot order trimming and hedging — apply when working with CheckTrimming, InpTrimMode, partial trim, day profit trim, or drawdown recovery logic.
globs: ["Custumer/RichTradingBot/**"]
alwaysApply: false
---

# Rich Trading Bot — Cơ Chế Tỉa Lệnh (Order Trimming)

Dùng để giảm drawdown khi rổ lệnh lớn, bằng cách đóng có chọn lọc.

## Chế Độ (`ENUM_TRIM_MODE` — `InpTrimMode`)

Chỉ **1 chế độ** hoạt động tại một thời điểm (chọn qua dropdown, không còn là các cờ bool độc lập):

| Mode | Hành vi |
| :--- | :--- |
| `TRIM_OFF` | Tắt hoàn toàn (mặc định) |
| `TRIM_TARGET` | Tỉa theo mục tiêu lợi nhuận nổi (`InpTrimTarget`) |
| `TRIM_PARTIAL_DD` | Tỉa Một Phần khi DD% vượt `InpPartialTrimDD` |
| `TRIM_DAY_PROFIT` | Tỉa khi lãi ngày > \|lỗ lệnh tệ nhất\| |
| `TRIM_HEDGE` | Tỉa chéo: đóng lệnh lời + lệnh lỗ (chọn theo $ lỗ nhiều nhất) cùng lúc |
| `TRIM_HEDGE_PTS` | Tỉa chéo: giống `TRIM_HEDGE` nhưng chọn lệnh lỗ theo **số điểm giá âm nhiều nhất**, không phụ thuộc lot |

## Tham Số

| Tham số | Mặc định | Áp dụng cho mode | Mô tả |
| :--- | :--- | :--- | :--- |
| `InpTrimMode` | `TRIM_OFF` | — | Chọn chế độ tỉa lệnh |
| `InpTrimTrigger` | 5 | tất cả | Kích hoạt khi **tổng** số lệnh ≥ X |
| `InpTrimTarget` | 10.0 | `TRIM_TARGET`, `TRIM_HEDGE`, `TRIM_HEDGE_PTS` | Mục tiêu lợi nhuận ròng sau tỉa ($) |
| `InpPartialTrimDD` | 20.0 | `TRIM_PARTIAL_DD` | Ngưỡng DD% kích hoạt |
| `InpTrimMaxLoss` | 1 | tất cả | Số lệnh âm tối đa cần tỉa mỗi lần |
| `InpTrimMaxWin` | 1 | `TRIM_HEDGE`, `TRIM_HEDGE_PTS` | Số lệnh dương tối đa dùng để tỉa |

## Hành Vi Từng Mode (`CheckTrimming()`)

Điều kiện chung: `InpTrimMode != TRIM_OFF` **VÀ** tổng số lệnh ≥ `InpTrimTrigger`.

- **`TRIM_PARTIAL_DD`** — DD% hiện tại > `InpPartialTrimDD` → đóng lệnh có floating loss lớn nhất (`WorstTicket()`), lặp tối đa `InpTrimMaxLoss` lần.
- **`TRIM_DAY_PROFIT`** — `DayProfit > |worstFloatLoss|` → đóng lệnh tệ nhất (lấy lãi ngày bù lỗ floating), lặp tối đa `InpTrimMaxLoss` lần.
- **`TRIM_HEDGE`** — mỗi chu kỳ (tối đa `InpTrimMaxWin` chu kỳ): ghép lệnh tốt nhất (`BestTicket()`, phải >0) với tối đa `InpTrimMaxLoss` lệnh tệ nhất theo **$ lỗ** (`profits[i] < profits[wIdx]`); nếu tổng $ ≥ `InpTrimTarget` → đóng **cả hai phía** cùng lúc.
- **`TRIM_HEDGE_PTS`** — giống hệt `TRIM_HEDGE` về cấu trúc chu kỳ và điều kiện đóng theo `InpTrimTarget` ($), nhưng chọn lệnh "tệ nhất" để ghép cặp theo **số điểm giá âm nhiều nhất** (`WorstTicketByPoints`-style, `pts[i] < pts[wIdx]`) thay vì theo $ lỗ — tránh tình trạng luôn chọn trúng lệnh lot lớn (lỗ $ nhiều) trong khi một lệnh lot nhỏ đang kẹt giá xa hơn nhiều lại bị bỏ qua.
- **`TRIM_TARGET`** — `totalFloatProfit >= InpTrimTarget` → đóng lệnh tệ nhất nếu phần còn lại vẫn ≥ target, lặp tối đa `InpTrimMaxLoss` lần.

## Helper Functions

- `WorstTicket()` → ticket của lệnh có floating profit âm nhất ($)
- `WorstTicketByPoints()` → ticket của lệnh có giá âm nhiều điểm nhất, không phụ thuộc lot
- `BestTicket()` → ticket của lệnh có floating profit dương nhất
- `FloatProfit(posType)` → tổng profit + swap của tất cả lệnh (hoặc theo type)

## Remote Config Sync

Payload đồng bộ (`BuildConfigPayload`/`ApplyConfigPayload`) serialize `g_TrimMode` như 1 số nguyên (enum index), thay vì 3 cờ bool riêng (`TrimEnable`/`PartialTrim`/`TrimByDayProfit`) như thiết kế cũ — `RTB_CONFIG_FIELD_COUNT` đã giảm từ 156 xuống 153 do bớt 3 field.
