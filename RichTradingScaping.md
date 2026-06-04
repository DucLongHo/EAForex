# Điều Kiện Tín Hiệu — Rich Tool Scaping

---

## 1. Điều Kiện EMA

### Khung M1 — EMA 9 / 21 / 50

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA9 cắt lên EMA21, đồng thời cả hai nằm trên EMA50 |
| **Sell** | EMA9 cắt xuống EMA21, đồng thời cả hai nằm dưới EMA50 |

### Khung M5 — EMA 13 / 34 / 89

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA13 cắt lên EMA34, đồng thời cả hai nằm trên EMA89 |
| **Sell** | EMA13 cắt xuống EMA34, đồng thời cả hai nằm dưới EMA89 |

### Khung M15 — EMA 21 / 50 / 200

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA21 cắt lên EMA50, cả hai cắt hoặc nằm trên EMA200 |
| **Sell** | EMA21 cắt xuống EMA50, cả hai cắt hoặc nằm dưới EMA200 |

### Khung M30 — EMA 34 / 89 / 200

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA34 cắt lên EMA89, cả hai cắt hoặc nằm trên EMA200 |
| **Sell** | EMA34 cắt xuống EMA89, cả hai cắt hoặc nằm dưới EMA200 |

### Khung H1 — EMA 50 / 200 / 365

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA50 cắt lên EMA200, cả hai cắt hoặc nằm trên EMA365 |
| **Sell** | EMA50 cắt xuống EMA200, cả hai cắt hoặc nằm dưới EMA365 |

### Khung H4 — EMA 50 / 200 / 500

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA50 cắt lên EMA200, cả hai cắt hoặc nằm trên EMA500 |
| **Sell** | EMA50 cắt xuống EMA200, cả hai cắt hoặc nằm dưới EMA500 |

### Khung D1 — EMA 20 / 50 / 200

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA20 cắt lên EMA50, cả hai cắt hoặc nằm trên EMA200 |
| **Sell** | EMA20 cắt xuống EMA50, cả hai cắt hoặc nằm dưới EMA200 |

### Khung W1 — EMA 39 / 89 / 200

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | EMA39 cắt lên EMA89, cả hai cắt hoặc nằm trên EMA200 |
| **Sell** | EMA39 cắt xuống EMA89, cả hai cắt hoặc nằm dưới EMA200 |

### Bảng Tính Điểm EMA

| Khung | Điểm |
| :--- | :---: |
| M1 | 4 |
| M5 | 6 |
| M15 | 8 |
| M30 | 10 |
| H1 | 12 |
| H4 | 15 |
| D1 | 20 |
| W1 | 25 |
| **Tổng tối đa** | **100** |

### Thang Đánh Giá EMA

| Điểm | Mức độ |
| :--- | :--- |
| 90 – 100 | Xu hướng cực mạnh |
| 75 – 90 | Xu hướng mạnh |
| 60 – 75 | Xu hướng trung bình |
| 40 – 60 | Sideway / điều chỉnh — hạn chế giao dịch |
| < 40 | Tín hiệu yếu — không giao dịch |

---

## 2. Điều Kiện RSI

### Khung M1 — Period 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI rơi xuống dưới 25 **và** xuất hiện phân kỳ tăng (đáy giá thấp hơn, đáy RSI cao hơn) |
| **Sell** | RSI vượt trên 75 **và** xuất hiện phân kỳ giảm (đỉnh giá cao hơn, đỉnh RSI thấp hơn) |

### Khung M5 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI < 30 **và** nến đảo chiều tăng |
| **Sell** | RSI > 70 **và** nến đảo chiều giảm |

### Khung M15 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI vừa cắt lên mức 50 |
| **Sell** | RSI vừa cắt xuống dưới mức 50 |

### Khung M30 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI nằm trong khoảng 35–40 rồi bật tăng |
| **Sell** | RSI nằm trong khoảng 60–65 rồi quay đầu giảm |

### Khung H1 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI từ dưới 45 lên trên 45 **và** giá tạo đáy cao hơn |
| **Sell** | RSI từ trên 55 xuống dưới 55 **và** giá tạo đỉnh thấp hơn |

### Khung H4 — Period 21

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI < 30 **và** phân kỳ tăng rõ ràng **và** nến xác nhận đóng cửa trên vùng Order Block hỗ trợ |
| **Sell** | RSI > 70 **và** phân kỳ giảm rõ ràng **và** nến xác nhận đóng cửa dưới vùng Order Block kháng cự |

### Khung D1 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI < 30 **và** phân kỳ tăng trên khung tuần **và** khối lượng giảm dần ở đáy |
| **Sell** | RSI > 70 **và** phân kỳ giảm trên khung tuần **và** khối lượng giảm dần ở đỉnh |

### Khung W1 — Period 14

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | RSI thoát vùng quá bán (từ dưới 30 lên trên 30), tuần sau vẫn đóng cửa trên 30 |
| **Sell** | RSI thoát vùng quá mua (từ trên 70 xuống dưới 70), tuần sau vẫn đóng cửa dưới 70 |

### Bảng Tính Điểm RSI

| Khung | Điểm |
| :--- | :---: |
| M1 | 5 |
| M5 | 10 |
| M15 | 10 |
| M30 | 10 |
| H1 | 10 |
| H4 | 15 |
| D1 | 20 |
| W1 | 20 |
| **Tổng tối đa** | **100** |

### Thang Đánh Giá RSI

| Điểm | Mức độ |
| :--- | :--- |
| 90 – 100 | Xu hướng cực mạnh |
| 75 – 90 | Xu hướng mạnh |
| 60 – 75 | Xu hướng trung bình |
| 40 – 60 | Sideway / điều chỉnh — hạn chế giao dịch |
| < 40 | Tín hiệu yếu — không giao dịch |

---

## 3. Điều Kiện MACD

### Khung M1 — Fast EMA 8 / Slow EMA 21 / Signal SMA 5

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line cắt lên Signal line; Histogram chuyển từ âm → dương, 3 nến tăng dần |
| **Sell** | MACD line cắt xuống Signal line; Histogram chuyển từ dương → âm và giảm sâu 3 nến |

### Khung M5 — Fast EMA 8 / Slow EMA 21 / Signal SMA 5

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD cắt lên, Histogram dương 2 cây liên tiếp |
| **Sell** | MACD cắt xuống, Histogram âm 2 cây liên tiếp |

### Khung M15 — Fast EMA 12 / Slow EMA 26 / Signal SMA 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD cắt lên, Histogram > 0 và mở rộng |
| **Sell** | MACD cắt xuống, Histogram < 0 và mở rộng âm |

### Khung M30 — Fast EMA 12 / Slow EMA 26 / Signal SMA 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line > Signal line; Histogram dương liên tiếp 2–3 cây |
| **Sell** | MACD line < Signal line; Histogram âm liên tiếp 2–3 cây |

### Khung H1 — Fast EMA 12 / Slow EMA 26 / Signal SMA 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line cắt lên Signal line (dưới 0: bắt đáy; trên 0: xu hướng mạnh); Histogram chuyển dương rõ ràng |
| **Sell** | MACD line cắt xuống Signal line (trên 0: bắt đỉnh; dưới 0: xu hướng giảm mạnh); Histogram chuyển âm rõ ràng |

### Khung H4 — Fast EMA 12 / Slow EMA 26 / Signal SMA 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line trên Signal line và trên 0 (xu hướng tăng xác nhận); Histogram dương và tăng dần |
| **Sell** | MACD line dưới Signal line và dưới 0 (xu hướng giảm xác nhận); Histogram âm và giảm sâu |

### Khung D1 — Fast EMA 12 / Slow EMA 26 / Signal SMA 9

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line cắt lên Signal line; Histogram dương ít nhất 1 ngày |
| **Sell** | MACD line cắt xuống Signal line; Histogram âm ít nhất 1 ngày |

### Khung W1 — Fast EMA 5 / Slow EMA 34 / Signal SMA 5

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | MACD line trên Signal line, liên tục trên 0; Histogram dương nhiều tuần |
| **Sell** | MACD line dưới Signal line, liên tục dưới 0; Histogram âm nhiều tuần |

### Bảng Tính Điểm MACD

| Khung | Điểm |
| :--- | :---: |
| M1 | 4 |
| M5 | 6 |
| M15 | 8 |
| M30 | 10 |
| H1 | 12 |
| H4 | 15 |
| D1 | 20 |
| W1 | 25 |
| **Tổng tối đa** | **100** |

### Thang Đánh Giá MACD

| Điểm | Mức độ |
| :--- | :--- |
| 90 – 100 | Xu hướng cực mạnh |
| 75 – 90 | Xu hướng mạnh |
| 60 – 75 | Xu hướng trung bình |
| 40 – 60 | Sideway / điều chỉnh — hạn chế giao dịch |
| < 40 | Tín hiệu yếu — không giao dịch |

---

## 4. Điều Kiện Ichimoku

### Khung M1 — Tenkan 6 / Kijun 13 / Senkou B 26

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá phá vỡ trên đám mây (Kumo) và đóng cửa xác nhận; Tenkan cắt lên Kijun |
| **Sell** | Giá phá vỡ dưới đám mây (Kumo) và đóng cửa xác nhận; Tenkan cắt xuống Kijun |

### Khung M5 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá trên đám mây (mây xanh); Tenkan cắt lên Kijun; Chikou Span trên giá |
| **Sell** | Giá dưới đám mây (mây đỏ); Tenkan cắt xuống Kijun; Chikou Span dưới giá |

### Khung M15 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá phá vỡ đám mây từ dưới lên; đám mây chuyển xanh (Span A > Span B); chờ 1–2 nến đóng trên mây |
| **Sell** | Giá phá vỡ đám mây từ trên xuống; đám mây chuyển đỏ (Span A < Span B); chờ 1–2 nến đóng dưới mây |

### Khung M30 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Tenkan > Kijun và cả hai dốc lên; Chikou Span trên giá và hướng lên; giá đóng cửa trên Kijun |
| **Sell** | Tenkan < Kijun và cả hai dốc xuống; Chikou Span dưới giá và hướng xuống; giá đóng cửa dưới Kijun |

### Khung H1 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá trên đám mây (màu xanh); Tenkan trên Kijun |
| **Sell** | Giá dưới đám mây (màu đỏ); Tenkan dưới Kijun |

### Khung H4 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá phá vỡ toàn bộ đám mây từ dưới lên; mây chuyển xanh (Span A > Span B); Future Cloud cũng xanh |
| **Sell** | Giá phá vỡ toàn bộ đám mây từ trên xuống; mây chuyển đỏ (Span A < Span B); Future Cloud cũng đỏ |

### Khung D1 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Giá trên đám mây D1; Tenkan trên Kijun; Chikou Span trên giá và hướng lên |
| **Sell** | Giá dưới đám mây D1; Tenkan dưới Kijun; Chikou Span dưới giá và hướng xuống |

### Khung W1 — Tenkan 9 / Kijun 26 / Senkou B 52

| Lệnh | Điều kiện |
| :--- | :--- |
| **Buy** | Chikou Span cắt lên trên giá; giá phá vỡ và đóng cửa trên đám mây; Tenkan cắt lên Kijun |
| **Sell** | Chikou Span cắt xuống dưới giá; giá phá vỡ và đóng cửa dưới đám mây; Tenkan cắt xuống Kijun |

### Bảng Tính Điểm Ichimoku

| Khung | Điểm |
| :--- | :---: |
| M1 | 4 |
| M5 | 6 |
| M15 | 8 |
| M30 | 10 |
| H1 | 12 |
| H4 | 15 |
| D1 | 20 |
| W1 | 25 |
| **Tổng tối đa** | **100** |

### Thang Đánh Giá Ichimoku

| Điểm | Mức độ |
| :--- | :--- |
| 90 – 100 | Xu hướng cực mạnh |
| 80 – 90 | Xu hướng mạnh |
| 70 – 79 | Xu hướng trung bình |
| 50 – 60 | Sideway / điều chỉnh — hạn chế giao dịch |
| < 50 | Tín hiệu yếu — không giao dịch |
