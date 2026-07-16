---
description: Rich Trading Bot Technicals gauge — apply when working with GetTechnicalRatingScore, VoteMA/VoteRSI/Vote*, HullMA, VWMA, UltimateOscillator, StochRSICalc, DrawTechnicalsGauge, or the Technicals panel next to the calendar.
globs: ["Custumer/RichTradingBot/**"]
alwaysApply: false
---

# Rich Trading Bot — Đồng Hồ Technicals (Strong Sell..Strong Buy)

Tái hiện lại công thức "Technical Rating" công khai của TradingView ngay trong EA — tự tính, không phụ thuộc API bên ngoài (TradingView không có API công khai cho chỉ số này).

## Phương Pháp (26 thành phần → điểm -1..+1)

**Nhóm Moving Average (15 thành phần)** — mỗi cái vote Buy(+1) nếu giá > MA, Sell(-1) nếu giá < MA:
- SMA và EMA tại 6 kỳ: 10, 20, 30, 50, 100, 200 (12 vote, qua `hTechMASMA[6]`/`hTechMAEMA[6]`)
- Hull MA(9) — tính tay qua `HullMA()`/`LWMA()`, MQL5 không có mode dựng sẵn
- VWMA(20) — tính tay qua `VWMA()` (giá đóng cửa gia quyền theo tick volume), MQL5 không có mode dựng sẵn
- Ichimoku — `VoteIchimoku()`: Buy nếu SpanA>SpanB, Kijun>SpanA, Tenkan>Kijun, giá>Tenkan (điều kiện trạng thái hiện tại, khác với `SignalIchimoku()` ở chỗ không yêu cầu sự kiện cắt)

**Nhóm Oscillator (11 thành phần)** — luật Buy/Sell riêng từng cái, theo đúng tài liệu TradingView:
| Chỉ báo | Kỳ | Buy | Sell |
| :--- | :--- | :--- | :--- |
| RSI | 14 | <30 và tăng | >70 và giảm |
| Stochastic | 14,3,3 | %K,%D<20 và %K>%D | %K,%D>80 và %K<%D |
| CCI | 20 | <-100 và tăng | >100 và giảm |
| ADX | 14 | +DI>-DI, ADX>20, ADX tăng | +DI<-DI, ADX>20, ADX giảm |
| Awesome Oscillator | — | cắt lên 0 hoặc saucer tăng | cắt xuống 0 hoặc saucer giảm |
| Momentum | 10 | tăng so kỳ trước | giảm so kỳ trước |
| MACD | 12,26,9 | MACD>Signal | MACD<Signal |
| Stochastic RSI | 3,3,14,14 | downtrend, K,D<20, K>D | uptrend, K,D>80, K<D |
| Williams %R | 14 | <-80 và tăng | >-20 và giảm |
| Bull/Bear Power | 13 | uptrend, BearPower<0 và tăng | downtrend, BullPower>0 và giảm |
| Ultimate Oscillator | 7,14,28 | >70 | <30 |

`Stochastic RSI`/`Ultimate Oscillator` tính tay qua `StochRSICalc()`/`UltimateOscillator()` — MQL5 không có hàm dựng sẵn cho 2 cái này.

**Xu hướng nền (uptrend/downtrend)** dùng cho Stochastic RSI và Bull/Bear Power: TradingView không công bố công thức xu hướng chính xác cho 2 chỉ báo này — `TechUptrend()` xấp xỉ bằng giá đóng cửa so với SMA(50) (tái dùng `hTechMASMA[3]`).

## Tổng Hợp Điểm

```
maScore  = trung bình 15 vote nhóm MA
oscScore = trung bình 11 vote nhóm Oscillator
score    = (maScore + oscScore) / 2        // -1..+1
```

Bucket theo `TechRatingLabel()`:

| Score | Nhãn |
| :--- | :--- |
| < -0.5 | Strong Sell |
| [-0.5, -0.1) | Sell |
| [-0.1, 0.1] | Neutral |
| (0.1, 0.5] | Buy |
| > 0.5 | Strong Buy |

## Khung Thời Gian & Tần Suất Cập Nhật

`InpTechTF` (mặc định H1) — **độc lập** với `InpSignalTF` của tín hiệu vào lệnh, nên có `hTechIchi` riêng (không dùng chung `hIchi`) để tránh lệch timeframe khi so giá.

Tất cả đọc **bar 0** (giá/chỉ báo hiện tại, nến đang chạy) — khác với các hàm `SignalXXX()` dùng bar đã đóng để tránh repaint tín hiệu vào lệnh. Đồng hồ là một readout trạng thái sống (như widget TradingView thật), không phải một trigger vào lệnh, nên không cần chống repaint.

`UpdateTechnicalRating()` chỉ được gọi từ `UpdateTechnicalsPanel()`, và hàm đó chỉ chạy khi `g_CalExpanded == true` — tính toán 26 chỉ báo mỗi giây (qua `UpdateGUI()`/`OnTimer()`) chỉ khi panel thật sự đang hiển thị, không tốn CPU khi ẩn.

## Vẽ Đồng Hồ Bán Nguyệt — `CCanvas`

Toàn bộ panel còn lại chỉ dùng `OBJ_RECTANGLE_LABEL`/`OBJ_LABEL` (không vẽ được cung tròn/kim xoay). Riêng đồng hồ Technicals dùng `CCanvas` (`<Canvas\Canvas.mqh>`, global `g_TechCanvas`) vẽ lên 1 `OBJ_BITMAP_LABEL` (tên `RTB_TechGauge`):

- **`DrawGaugeRing()`** — tô dải hình khuyên (annulus) bằng cách **quét từng pixel** trong hộp bao rồi kiểm tra khoảng cách/góc so với tâm, thay vì kẻ hàng trăm đoạn thẳng xuyên tâm chồng lên nhau (cách cũ, đã bỏ — để lại viền trong/ngoài răng cưa do các nét AA chồng mép không khớp nhau). Viền trong/ngoài được fade mượt trong khoảng 1px (`cov` — hệ số phủ theo khoảng cách `d` tới `rInner`/`rOuter`) thay vì cắt cứng, cho đường tròn mịn thật sự.
- **`GaugeColorAt()`** — nội suy màu liên tục qua 5 mốc (`LerpARGB`) đặt tại tâm mỗi vùng 36° (162°/126°/90°/54°/18°) — tạo dải chuyển màu mượt kiểu cầu vồng (đỏ đậm→đỏ cam→xám→xanh lá nhạt→xanh lá đậm) thay vì 5 khối màu cứng rời rạc.
- 6 vạch chia nhỏ tại ranh giới các vùng (bán kính `rOuter+2`→`rOuter+7`) — điểm nhấn tinh tế giống đồng hồ đo thật.
- **`DrawLinePx()`** — Bresenham tự viết + `PixelSet()` ghi thẳng ARGB đủ độ đục, **không dùng `LineAA()`**: `LineAA()` pha alpha theo độ phủ pixel, nhưng trên nền trong suốt hoàn toàn (`Erase(0x00000000)`) phép pha đó khiến hầu hết pixel có alpha rất thấp — chỉ điểm phủ gần trọn mới hiện rõ, tạo cảm giác nét đứt/chấm chấm thay vì liền mạch (lỗi đã gặp thực tế, không phải lý thuyết). Dùng cho vạch chia ranh giới vùng.
- Kim: **`DrawThickLine()`** — **không** kẻ nhiều bản sao `DrawLinePx()` song song (thử ban đầu, thất bại: các dải Bresenham lệch vuông góc không khớp pixel-với-pixel ở góc chéo, để lại khe hở giữa các dải → vẫn trông đứt đoạn). Thay bằng quét toàn bộ hộp bao quanh đoạn thẳng, với mỗi pixel tính khoảng cách vuông góc tới điểm gần nhất trên đoạn (kẹp trong [0, len]) — tô nếu khoảng cách ≤ nửa bề dày. Vẽ đúng 1 hình "viên nhộng" (capsule) đặc, không khe hở, hai đầu tự động bo tròn. Kim chạy từ tâm ra theo góc `angle = 90*(1 - score)` (score=-1→180° trái, score=0→90° đỉnh, score=+1→0° phải), dừng trước khi chạm `rInner` (không đâm xuyên qua dải màu) + `FillCircle()` 2 lớp làm trục kim (viền tối + lõi sáng).
- `DrawTechnicalsGauge()` chỉ `CreateBitmapLabel()` **1 lần** (kiểm tra qua `g_TechCanvasReady`), các lần sau chỉ `Erase()` + vẽ lại + `Update()` — tránh recreate object mỗi giây.
- `RemoveTechnicalsPanel()` gọi `g_TechCanvas.Destroy()` đúng API (không tự `ObjectDelete()` object bitmap phía sau lưng CCanvas) để giữ state nội bộ nhất quán — dùng khi đóng lịch (`!g_CalExpanded`) hoặc `RemoveGUI()` lúc EA thoát.

## Vị Trí & Vòng Đời

`UpdateTechnicalsPanel()` được gọi ngay sau `UpdateCalendarPanel()` trong `UpdateGUI()`. Neo tại `g_CalRightEdge + 12` (biến `g_CalRightEdge` được `UpdateCalendarPanel()` ghi lại = `calX + calW` mỗi lần vẽ) theo `InpCalPanelY` — tức luôn bám sát mép phải panel Lịch bất kể panel Lịch co giãn theo tháng. Ẩn/hiện đồng thời với panel Lịch qua `g_CalExpanded` (nút "Xem Lịch »"), không có nút bật/tắt riêng.

## Biên Dịch

MetaEditor có sẵn command-line compile để kiểm tra cú pháp trước khi giao cho người dùng test trên MT5 thật:
```
& "C:\Program Files\MetaTrader 5\MetaEditor64.exe" /compile:"path\to\file.mq5" /log:"path\to\log.txt"
```
File nguồn cần đổi tên/copy tạm thành `.mq5` trước khi biên dịch — dọn `.mq5`/`.ex5`/log tạm sau khi kiểm tra xong, không commit các file build.
