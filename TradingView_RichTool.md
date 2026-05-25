# Rich Trading Tool — TradingView Indicator (Pine Script v6)

Indicator tổng hợp 5 thành phần phân tích kỹ thuật trên một script duy nhất.

---

## Tính Năng

| # | Tên | Mô tả |
|---|-----|-------|
| 1 | **OP** | Giá mở cửa phiên hiện tại — phân định xu hướng ngày |
| 2 | **MLP** | Trung điểm phiên hôm qua (High+Low)/2 — ngưỡng xoay |
| 3 | **RD+ / RD−** | Phân kỳ Cumulative Volume Delta (CVD) lọc qua EMA 50 |
| 4 | **VP** | Volume Profile đơn giản: POC / VAH / VAL |
| 5 | **KCX** | VWAP + dải ±1σ / ±2σ / ±3σ |

---

**Code:** [RichTradingTool.pine](RichTradingTool.pine)

---

## Cách Dùng

### Cài đặt
1. Mở TradingView → **Pine Editor** (tab dưới cùng)
2. Xóa code mặc định, dán toàn bộ code trên vào
3. Nhấn **"Add to chart"**

### Logic Giao Dịch

| Điều kiện | Hành động |
|-----------|-----------|
| Giá **trên OP** | Ưu tiên **BUY** |
| Giá **dưới OP** | Ưu tiên **SELL** |
| Giá **trên MLP** | Lực mua chiếm ưu thế trong ngày |
| Giá **dưới MLP** | Lực bán chiếm ưu thế trong ngày |
| Giá **trên POC** | Xu hướng tăng (volume xác nhận) |
| Giá **dưới POC** | Xu hướng giảm (volume xác nhận) |
| Giá chạm **VAH** | Kháng cự volume — cân nhắc SELL |
| Giá chạm **VAL** | Hỗ trợ volume — cân nhắc BUY |
| Giá chạm dải **đỏ +2σ/+3σ** | Vùng quá mua — cân nhắc SELL |
| Giá chạm dải **xanh -2σ/-3σ** | Vùng quá bán — cân nhắc BUY |
| Tín hiệu **RD−** xuất hiện | Cảnh báo đảo chiều giảm |
| Tín hiệu **RD+** xuất hiện | Cảnh báo đảo chiều tăng |

### Bộ lọc tín hiệu mạnh nhất (hội tụ)

**BUY chất lượng cao:**
- Giá trên OP + trên MLP + trên POC
- RD+ xuất hiện (RSI đáy 2 < 30, giá trên EMA 200)
- Giá đang ở dải xanh -2σ / -3σ (VWAP quá bán)

**SELL chất lượng cao:**
- Giá dưới OP + dưới MLP + dưới POC
- RD− xuất hiện (RSI đỉnh 2 > 70, giá dưới EMA 200)
- Giá đang ở dải đỏ +2σ / +3σ (VWAP quá mua)

---

## Ghi Chú Kỹ Thuật

| Thành phần | Lưu ý |
|-----------|-------|
| **OP / MLP** | Dùng `request.security` D timeframe — chính xác trên mọi khung giờ |
| **Volume Profile** | Đây là xấp xỉ Pine Script, không phải VP chuyên dụng. Nên kết hợp thêm công cụ **Volume Profile Fixed Range** tích hợp sẵn của TradingView để có kết quả chính xác hơn |
| **Divergence** | Tín hiệu bị trễ `swingLen` nến do cần xác nhận swing. Đây là tính năng, không phải lỗi — giúp tránh tín hiệu giả |
| **VWAP** | Reset mỗi ngày giao dịch. Trên khung D1 trở lên, VWAP sẽ không có ý nghĩa thống kê — dùng trên M15/H1 |
| **RD+ từ code `0us9LJIi`** | Indicator phân kỳ tham chiếu — có thể thêm riêng qua **Indicators → Community Scripts → nhập mã** để so sánh tín hiệu |
