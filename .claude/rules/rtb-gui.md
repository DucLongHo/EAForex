---
description: Rich Trading Bot GUI panel — apply when working with UpdateGUI, DrawHLine, Lbl, LblR, CreateChip, DrawGauge, GUI labels, Day P/L display, or OnTradeTransaction for realtime updates.
globs: ["Custumer/RichTradingBot/**"]
alwaysApply: false
---

# Rich Trading Bot — Giao Diện (GUI)

Panel tự động vẽ góc trên-trái màn hình, cập nhật mỗi giây qua `OnTimer()` và ngay lập tức qua `OnTradeTransaction()`. Bố cục kiểu "Console": header + chip trạng thái + card phân vùng + gauge — thay cho layout danh sách phẳng cũ.

## Bố Cục (từ trên xuống)

| Vùng | Nội dung | Object gốc |
| :--- | :--- | :--- |
| Header | Mark `◆ RTB CONSOLE` (trái) + giờ hiện tại (phải, canh phải) | `BGTitle`, `T`, `TClock` |
| Chips | 2x2: Signal / Direction / Mode+Role / Sync | `ChipSig`, `ChipDir`, `ChipMode`, `ChipSync` |
| Card Tài khoản | Balance, Initial, Day P/L, Float — viền trái màu cyan | `CardAcct`, `CardAcctBar`, `BalL/V`, `IniL/V`, `DayL/V`, `FloL/V` |
| Card Rủi ro | DD Now + DD Max, mỗi dòng có gauge ngang tỉ lệ đúng %; thêm dòng Hedge nếu `InpHedgeEnable` — viền trái màu amber | `CardRisk`, `CardRiskBar`, `DDL/V`, `DDG`, `MDDL/V`, `MDDG`, `HdgS` |
| Twin Buy/Sell | 2 card song song, viền trên xanh (Buy)/đỏ (Sell), mỗi card: nhãn+count, P/L cỡ lớn, Lot | `CardBuy(Bar)`, `BuyL/V/Lot`, `CardSell(Bar)`, `SelL/V/Lot` |
| Total | Tổng số lệnh, canh phải | `TotL`, `TotV` |
| Điều khiển lệnh | Nút Bot Toggle (Master)/Close All/Buy/Sell/Profit/Loss | `BG2`, `P2T`, `Btn...` |
| Thống kê | Card viền trái cyan, bảng lưới 5 cột (Date/Pips/Profit/Gain/Lot) Today/Week/Month/Year, màu theo dấu lãi/lỗ cho Pips/Profit/Gain, 2 dải nền xen kẽ nhạt sau dòng Today/Month + nút Xem Lịch trong header | `BG3`, `StatsBar`, `P3T`, `TH0-4`, `TR{r}L/P/$/G/V`, `RowStripe0/1` |
| Vào lệnh thủ công | Chỉ hiện khi `InpBotMode=MODE_SEMI_AUTO` | `BG4`, `P4T`, `BtnOpenBuy/Sell` |

## Chiều Cao Panel Co Giãn Theo Nội Dung

Khác bản cũ (chiều cao `BG` cố định `360+hOff`), giờ `UpdateGUI()` dùng con trỏ `y2` chạy tuần tự qua từng vùng, cộng dồn chiều cao thật (card Rủi ro tự giãn thêm khi bật Hedge) rồi mới tính `bgH`/`bg2Y`/`bg3Y`/`bg4Y` — không còn hằng số offset cố định như `374`/`514`/`671`.

Card Thống Kê (`BG3`) cũng co giãn theo cùng nguyên tắc: con trỏ `y3` chạy qua header + dòng tiêu đề cột + 4 dòng dữ liệu (Today/Week/Month/Year), rồi mới tính `bg3H` — không còn `YSIZE=115` cố định cứng. `bg4Y = bg3Y + bg3H + 42` và nhánh else `g_LastPanelBottom = bg3Y + bg3H` đọc trực tiếp `bg3H` này thay vì hằng số `157`/`115`.

**`g_LastPanelBottom`** (global) được `UpdateGUI()` ghi lại đáy thật của toàn panel mỗi lần vẽ — `UpdateCalendarPanel()` đọc biến này để tính chiều cao ô lịch, thay vì tự tính lại bằng công thức hằng số cũ (đã lệch sau khi bố cục co giãn).

## Helper Vẽ Mới

- **`LblR(name, text, xRight, y, clr, sz)`** — như `Lbl()` nhưng neo `ANCHOR_RIGHT_UPPER`, dùng cho các giá trị canh phải kiểu "nhãn : giá trị" trong card.
- **`CreateChip(name, text, lx, ly, lw, lh, bg, fg)`** — 1 rectangle nền phẳng + label chữ giữa dọc, dùng cho 4 chip trạng thái.
- **`DrawGauge(name, lx, ly, lw, lh, pct, trackClr, fillClr)`** — track full-width + fill tỉ lệ theo `pct` (0-100, tự clamp). Dùng cho DD Now/DD Max.
- **`CreateRect()` giờ cập nhật `XSIZE`/`YSIZE`/`BGCOLOR` ở MỌI lần gọi**, không chỉ lúc tạo object — bắt buộc để thanh gauge co giãn theo % mỗi giây (trước đây chỉ set kích thước 1 lần lúc tạo, đứng yên vĩnh viễn sau đó).

## Màu Ngữ Nghĩa (giữ nguyên logic, đổi bảng màu)

| Trạng thái | Điều kiện | Màu |
| :--- | :--- | :--- |
| Lãi / Buy Only | | `clrLimeGreen` |
| Lỗ / Sell Only | | `clrTomato` |
| DD cảnh báo | `ddPct`/`MaxDrawdownPct` > 20% | `clrOrangeRed` |
| DD nguy hiểm | `ddPct`/`MaxDrawdownPct` > 60% | `clrTomato` |
| Hướng Both | | cyan `C'111,217,238'` (chip nền `C'18,50,68'`) |
| Card/chip nền | | navy `C'20,28,44'` / `C'24,34,54'` |
| Viền card Tài khoản | | cyan `C'79,195,217'` |
| Viền card Rủi ro | | amber `C'240,166,63'` |

## Cập Nhật Realtime — Day P/L

```cpp
void OnTradeTransaction(...) {
    if(trans.type == TRADE_TRANSACTION_DEAL_ADD) {
        UpdateDayProfit();  // tính lại ngay
        UpdateGUI();        // vẽ lại ngay
    }
}
```

**`UpdateDayProfit()`** lọc lịch sử deal từ 00:00 hôm nay theo `DEAL_SYMBOL` (không filter magic):
- Bao gồm: tất cả deal `DEAL_ENTRY_OUT` / `DEAL_ENTRY_OUT_BY` trên symbol này
- **Không filter DEAL_MAGIC** vì lệnh đóng thủ công qua MT5 terminal có `DEAL_MAGIC=0`

## Object Prefix

Tất cả GUI object dùng prefix `"RTB_"` (biến `GUI`):
- `RTB_BG`, `RTB_BG2`, `RTB_BG3`, `RTB_BG4` — nền các vùng panel
- `RTB_T`, `RTB_TClock`, `RTB_Chip*`, `RTB_Card*`, `RTB_*L`, `RTB_*V` — mark, chip, card, nhãn/giá trị
- `RTB_TrailBuy`, `RTB_TrailSell` — đường trail (`DrawHLine`)

`RemoveGUI()` = `ObjectsDeleteAll(0, "RTB_")` — dọn sạch khi EA thoát.

## Object Cũ Đã Loại Bỏ

Bố cục danh sách cũ dùng các object `L0-L3` (đường phân cách), `Tim`, `Sig`, `Mod`, `Dir`, `Sync`, `Bal`, `Ini`, `DayP`, `FP`, `DD`, `MDD`, `BuyP`, `BuyC`, `SelP`, `SelC`, `Tot` — `UpdateGUI()` chủ động `ObjectDelete()` toàn bộ các tên này mỗi lần chạy để dọn sạch object mồ côi còn sót lại từ bản cũ trên chart đang chạy.

Các đường phân cách dọc/ngang cũ của bảng lưới (`P3VC1-4`, `P3HR0-3`) cũng được dọn theo cách này — thay bằng 2 dải nền xen kẽ (`RowStripe0/1`) đơn giản hơn để mắt bám dòng khi đọc ngang, không cần kẻ ô.
