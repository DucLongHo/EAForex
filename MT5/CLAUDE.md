# CLAUDE.md

File này cung cấp hướng dẫn cho Claude Code (claude.ai/code) khi làm việc với code trong repository này.

## File Nguồn & Biên Dịch

File duy nhất: [RichTradingBot.cpp](RichTradingBot.cpp)

**Để biên dịch:** Đổi tên/copy thành `RichTradingBot.mq5`, mở trong MetaTrader 5 MetaEditor, nhấn `F7`. Không có hệ thống build nào khác. Không có automated test — kiểm tra bằng cách chạy EA trên tài khoản demo trong Strategy Tester.

## Kiến Trúc

Expert Advisor MQL5 một file duy nhất (~1200 dòng). Mô hình event-driven:

| Handler | Khi nào | Nhiệm vụ |
| :--- | :--- | :--- |
| `OnInit()` | Khởi động EA | Tạo indicator handles (`iMA`, `iBands`, `iIchimoku`), gọi `InitDCA()` + `InitPyra()`, đặt timer 1 giây |
| `OnTick()` | Mỗi tick giá | `CheckEntry()` + `CheckExit()` (khi Stealth Mode BẬT) |
| `OnTimer()` | Mỗi 1 giây | `UpdateDayProfit()`, reset bộ đếm, `CheckExit()` (Stealth TẮT), `CheckTrimming()`, `CheckTrailing()`, `CheckDCA()`, `CheckPyramiding()`, `UpdateGUI()` |
| `OnTradeTransaction()` | Khi có deal thêm vào | `UpdateDayProfit()` + `UpdateGUI()` ngay lập tức — không chờ timer |
| `OnDeinit()` | Thoát EA | `EventKillTimer()`, `RemoveGUI()`, giải phóng indicator handles |

## Tổ Chức Code (từ trên xuống dưới)

1. **Enums** — `ENUM_SIGNAL_MODE`, `ENUM_DIRECTION`, `ENUM_DCA_MODE`, `ENUM_TRAIL_MODE`
2. **Input groups** — Cài đặt cơ bản, tín hiệu vào lệnh, EMA, BB, Ichimoku, bộ lọc chung, 8 tầng DCA, 8 tầng Pyramiding, Trimming, Trailing, Exit
3. **Global state** — indicator handles, mảng `DCA_*[8]` và `PYRA_*[8]`, `LastOrderTime`, `LastEntryTime`, `InitBalance`, `MaxDrawdownPct`, `DayProfit`, `LastDay`, `TrailBuy/Sell`
4. **Hàm tiện ích** — `CountPos/Buy/Sell/All`, `CountPyra()`, `FloatProfit`, `TotalLot`, `LastOpenPrice`, `AvgOpenPrice`, `WorstTicket`, `BestTicket`, `CloseAll`, `CloseAllProfit`, `CloseAllLoss`, `NormLot`
5. **`OpenOrder()`** — hàm đặt lệnh trung tâm; `isDCA=true` bỏ qua `InpUseTakeProfit`/`InpUseStopLoss` và luôn đặt server TP/SL; `isPyra=true` điều khiển prefix comment pyramiding
6. **Hàm tín hiệu** — `SignalEMA`, `SignalBZZone`, `SignalIchimoku`, `SignalBB`, `SignalSimulated`, `GetSignal`
7. **Logic chiến lược** — `CheckEntry`, `CheckDCA`, `CheckPyramiding`, `CheckTrimming`, `CheckTrailing`, `CheckExit`
8. **Day P/L** — `UpdateDayProfit` (không lọc theo Magic — lệnh đóng thủ công qua MT5 terminal có `DEAL_MAGIC=0`)
9. **GUI** — `DrawHLine`, `Lbl`, `UpdateGUI`, `RemoveGUI`; tất cả object có prefix `"RTB_"`
10. **Event handlers** — `OnInit`, `OnDeinit`, `OnTick`, `OnTimer`, `OnTradeTransaction`

## Quy Ước Quan Trọng

**Mã hóa comment lệnh:** Mỗi position mang comment `"RTB|tp_pts|sl_pts"`. Lệnh gốc/DCA dùng format này. **Lệnh Pyramiding dùng `"RTP|tp_pts|sl_pts"`** (prefix `RTP`, không phải `RTB`) — `CountPyra()` kiểm tra prefix `"RTP|"` để phân biệt pyramiding với DCA mà không cần dựa vào bộ đếm runtime (an toàn khi EA restart). `CheckExit()` đọc comment để áp dụng TP/SL theo từng tầng bằng code (safety net dù broker đã đặt server TP).

**Xác định tầng DCA:** `dcaCount = count - 1` (số lệnh DCA đã mở, không tính lệnh gốc). Tầng được xác định bằng cách cộng dồn `DCA_MaxOrd[i]` cho đến khi `dcaCount < cumulative`. Các input `InpDCA*` dạng phẳng được copy vào mảng `DCA_*[8]` trong `OnInit()` thông qua `InitDCA()`. Pyramiding áp dụng cùng pattern với `PYRA_*[8]` qua `InitPyra()`.

**Fallback TP/SL của Pyramiding:** Nếu `PYRA_TP[lvl]==0` VÀ `PYRA_SL[lvl]==0`, lệnh pyramiding fallback về `InpTP_Points`/`InpSL_Points` với `isPyra=false` — được xử lý như lệnh gốc (comment `"RTB|0|0"`, section 2b của Stealth xử lý). Nếu một trong hai giá trị PYRA khác 0, tầng đó dùng giá trị tầng và được nhận dạng là lệnh pyramiding.

**Thứ tự các section trong `CheckExit()`:**
1. Per-position `ClosePerPips` (luôn chạy)
2a. DCA code TP/SL — đọc comment, luôn chạy (safety net khi server TP bị miss)
2b. Stealth TP/SL cho lệnh gốc — chỉ khi `InpStealthMode=true`
3. Basket profit target (`InpCloseProfit`)
4. Basket loss cut (`InpCloseLoss`)

**Tần suất gọi `CheckExit()`:** `OnTick` khi Stealth BẬT (độ chính xác tick cho lệnh gốc); `OnTimer` khi Stealth TẮT (1 giây, DCA đã có server TP/SL).

**Indicator handles:** Cả bốn handle (`hEMAFast`, `hEMASlow`, `hBB`, `hIchi`) đều được tạo bất kể `InpSignalMode` là gì. `OnInit` trả về `INIT_FAILED` nếu bất kỳ handle nào là `INVALID_HANDLE`.

**Lọc position:** Tất cả vòng lặp đều lọc theo `InpMagic` VÀ `_Symbol` trước khi xử lý bất kỳ position nào.

**`CloseAll()` dùng async mode** (`Trade.SetAsyncMode(true)`) để đóng lệnh hàng loạt mà không block — không được giả định rằng lệnh đã biến mất ngay sau khi hàm trả về.

## Tài Liệu Chi Tiết Từng Hệ Thống

Xem `.claude/rules/` để biết thêm chi tiết về từng hệ thống con:
- [rtb-overview.md](.claude/rules/rtb-overview.md) — tổng quan dự án, kiến trúc event
- [rtb-entry-signals.md](.claude/rules/rtb-entry-signals.md) — các chiến lược tín hiệu, luồng `GetSignal()`
- [rtb-dca.md](.claude/rules/rtb-dca.md) — thuật toán xác định tầng DCA, điều kiện kích hoạt
- [rtb-pyramiding.md](.claude/rules/rtb-pyramiding.md) — logic pyramiding, prefix comment `"RTP|"`, fallback TP/SL theo tầng
- [rtb-trailing.md](.claude/rules/rtb-trailing.md) — Basket vs Single trailing, `ApplyTrailToPos`
- [rtb-trimming.md](.claude/rules/rtb-trimming.md) — thứ tự ưu tiên trimming, điều khiển `InpTrimMaxLoss`/`InpTrimMaxWin`
- [rtb-exit.md](.claude/rules/rtb-exit.md) — các section `CheckExit()`, tương tác với Stealth Mode
- [rtb-gui.md](.claude/rules/rtb-gui.md) — bố cục panel, quy tắc màu sắc, logic Day P/L
- [rtb-settings.md](.claude/rules/rtb-settings.md) — tham chiếu đầy đủ các tham số
