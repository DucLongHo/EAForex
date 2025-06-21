//+------------------------------------------------------------------+
//|                   Ema50Trend Expert Advisors                     |
//|                   Copyright 2025                                 |
//|                   Version 1.00                                   |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;
CChartObjectButton TradeButton;// Nút bật/tắt giao dịch

// Input parameters
input double DrawDownBalance = 0.5;  // Tỷ lệ drawdown chấp nhận (%)
input double DrawDownBalanceMax = 5;  // Tỷ lệ drawdown tối đa chấp nhận (%)
input bool BEManage = true;  // Kích hoạt quản lý điểm hòa vốn
input bool SendEmail = true; // Kích hoạt gửi email thông báo
// Constant data
const int PERIOD_EMA50 = 50;
const int PERIOD_EMA100 = 100;
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const int CANDLES_M30 = 336;

datetime CandleCloseTime;// Biến kiểm tra giá chạy 30p một lần 
bool tradingEnabled = true; // Biến kiểm soát trạng thái giao dịch
double initialBalance = 0; // Biến toàn cục lưu balance ban đầu
bool IsRunning = true; // Biến kiểm soát trạng thái EA đang chạy
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+

int OnInit(){
   EventSetTimer(ONE);
   initialBalance = AccountInfoDouble(ACCOUNT_BALANCE);

   // Tạo nút và thiết lập thuộc tính
   if(!TradeButton.Create(0, "TradeButton", 0, 100, 200, 150, 50))
      return(INIT_FAILED);
   
   TradeButton.Description("TRADING: ON");
   TradeButton.Color(clrWhite);
   TradeButton.BackColor(clrGreen); 
   TradeButton.FontSize(12);
   TradeButton.Font("Arial");
   TradeButton.Selectable(true);
   ObjectSetInteger(0, "TradeButton", OBJPROP_ZORDER, 10);
   ChartRedraw(0);
   
   return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M15 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);

    datetime closeTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);
    string timeString = TimeToString(closeTime - TimeCurrent(), TIME_SECONDS);

    Comment("Đếm ngược thời gian đóng nến: ", timeString, "\n", "\n");
    
    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime - currentTime <= 2 ){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
    }

    if(isRunningEa && tradingEnabled && IsRunning){
        TradeEMAs();
        isRunningEa = false;
    }
}

void OnTick(){
    if(IsAccountOverInitialDrawdown(DrawDownBalanceMax)){
        // Đóng toàn bộ lệnh nếu vượt drawdown
        CloseAllPositions();
        IsRunning = false; // Dừng EA
    }
    // Kiểm tra xem có bật giao dịch không
   if(BEManage) ManagePositions();
}

void OnDeinit(const int reason){
    TradeButton.Delete();
    EventKillTimer();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Kiểm tra sự kiện nhấp chuột trên nút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton") {
        tradingEnabled = !tradingEnabled; // Đảo ngược trạng thái giao dịch
        if (tradingEnabled) {
            TradeButton.Description("TRADING: ON");
            TradeButton.Color(clrWhite);
            TradeButton.BackColor(clrGreen); // Màu xanh lá khi bật
        } else {
            TradeButton.Description("TRADING: OFF");
            TradeButton.Color(clrWhite);
            TradeButton.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    // Kiểm tra sự kiện nhấn chuột trái
    if(id == CHARTEVENT_OBJECT_CLICK && TradeButton.State()){
        // Lấy tọa độ chuột
        int x_distance = (int)lparam;
        int y_distance = (int)dparam;
        // Di chuyển nút đến vị trí mới
        TradeButton.X_Distance(x_distance);
        TradeButton.Y_Distance(y_distance);
        // Vẽ lại biểu đồ
        ChartRedraw(0);
    }
}

void TradeEMAs(){
    double ema50[], ema100[];
    int handle50 = iMA(_Symbol, _Period, PERIOD_EMA50, 0, MODE_EMA, PRICE_CLOSE);
    int handle100 = iMA(_Symbol, _Period, PERIOD_EMA100, 0, MODE_EMA, PRICE_CLOSE);

    if (handle50 < 0 || handle100 < 0) return;
    ArraySetAsSeries(ema50, true);
    ArraySetAsSeries(ema100, true);
    if(CopyBuffer(handle50, 0, 0, CANDLES_M30, ema50) <= 0 || 
        CopyBuffer(handle100, 0, 0, CANDLES_M30, ema100) <= 0) return;

    string trendNow = DetermineTrend(ema50[0], ema100[0]);
    string trendBefore = DetermineTrend(ema50[1], ema100[1]);
    //Trade
    if(trendNow != trendBefore){
        CloseAllPositions();
        if(trendNow == "BUY"){
            // Mở lệnh BUY
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
            double stopLoss = GetStopLossM15("BUY", ema50[0], ema100[0]);
            double lotSize = GetLotSize(entry - stopLoss);
            if(!Trade.Buy(lotSize, _Symbol, entry, stopLoss)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            } else {
                if(SendEmail) CheckNewOrdersAndSendEmail("Mở lệnh BUY, ema50 cắt lên ema100");
                Alert("Tham Gia Thị Trường");
            }
        } else if(trendNow == "SELL"){
            // Mở lệnh SELL
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
            double stopLoss = GetStopLossM15("SELL", ema50[0], ema100[0]);
            double lotSize = GetLotSize(stopLoss - entry);
            if(!Trade.Sell(lotSize, _Symbol, entry, stopLoss)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            } else {
                if(SendEmail) CheckNewOrdersAndSendEmail("Mở lệnh SELL, ema50 cắt xuống ema100");
                Alert("Tham Gia Thị Trường");
            }
        }
    }

    //Vẽ đường EMA50
    ObjectsDeleteAll(0, -1, OBJ_TREND);// Xóa các đối tượng cũ nếu tồn tại
    for(int index = 0; index < CANDLES_M30 - ONE; index++){
        datetime startTime = iTime(_Symbol, _Period, index);
        datetime endTime = iTime(_Symbol, _Period, index + 1);

        double priceStartEma50 = ema50[index];
        double priceEndEma50 = ema50[index + 1];

        double priceStartEma100 = ema100[index];
        double priceEndEma100 = ema100[index + 1];

        if(priceStartEma50 == 0 || priceEndEma50 == 0 ) return;

        color colorEma = DetermineTrendColor(priceStartEma50, priceStartEma100);

        string nameEma50 = "EMA50"+IntegerToString(index);
        string nameEma100 = "EMA100"+IntegerToString(index);

        ObjectCreate(0, nameEma50, OBJ_TREND, 0, startTime, priceStartEma50, endTime, priceEndEma50);
        ObjectSetInteger(0, nameEma50, OBJPROP_COLOR, colorEma);
        ObjectSetInteger(0, nameEma50, OBJPROP_WIDTH, 2);

        ObjectCreate(0, nameEma100, OBJ_TREND, 0, startTime, priceStartEma100, endTime, priceEndEma100);
        ObjectSetInteger(0, nameEma100, OBJPROP_COLOR, colorEma);
        ObjectSetInteger(0, nameEma100, OBJPROP_WIDTH, 2);
    }

    ChartRedraw(0);
}

double GetLotSize(double stopLossDistance){
    // Tính toán kích thước lô dựa trên rủi ro và khoảng cách stop loss
    double accountBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    double riskAmounts = accountBalance * DrawDownBalance / 100.0; // Rủi ro theo phần trăm tài khoản

    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    
    // Kiểm tra giá trị hợp lệ
    if (tickValue <= ZERO || tickSize <= ZERO || _Point <= ZERO) return 0.0;
    // Tính số pips tương ứng với stopLossDistance
    double stopLossPips = stopLossDistance / _Point;

    // Tính giá trị pip
    double pipValue = tickValue / (tickSize / _Point);

    // Tính toán kích thước lô
    double lotSize = riskAmounts / (stopLossPips * pipValue);

    // Đảm bảo rằng kích thước lô nằm trong phạm vi cho phép của sàn giao dịch
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
    
    // Làm tròn kích thước lô đến số thập phân phù hợp
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lotSize = MathFloor(lotSize / lotStep) * lotStep;

    return lotSize;
}

double GetStopLossM15(string trend, double ema50, double ema100){
    double stopLoss = 0.0;
    if(trend == "BUY") {
        // Lấy giá thấp nhất trong ba nến gần nhất
        stopLoss = MathMin(iLow(_Symbol, PERIOD_M30, 0), MathMin(iLow(_Symbol, PERIOD_M30, 1), iLow(_Symbol, PERIOD_M30, 2)));
        // So sánh với giá trị EMA
        if(ema50 > 0) stopLoss = MathMin(stopLoss, ema50);
        if(ema100 > 0) stopLoss = MathMin(stopLoss, ema100);
    } else if (trend == "SELL") {
        // Lấy giá cao nhất trong ba nến gần nhất
        stopLoss = MathMax(iHigh(_Symbol, PERIOD_M30, 0), MathMax(iHigh(_Symbol, PERIOD_M30, 1), iHigh(_Symbol, PERIOD_M30, 2)));
        // So sánh với giá trị EMA
        if(ema50 > 0) stopLoss = MathMax(stopLoss, ema50);
        if(ema100 > 0) stopLoss = MathMax(stopLoss, ema100);
    }

    return stopLoss;
}

void ManagePositions(){
    // Lặp qua tất cả các vị thế đang mở
    for(int index = PositionsTotal() - 1; index >= ZERO; index--){
        // Lấy thông tin vị thế
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        // Lấy thông tin chi tiết của vị thế
        double entry = PositionGetDouble(POSITION_PRICE_OPEN);
        double stopLoss = PositionGetDouble(POSITION_SL);
        double takeProfit = PositionGetDouble(POSITION_TP);
        double currentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
        string symbol = PositionGetString(POSITION_SYMBOL);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            double stopLossDistance = MathAbs(entry - stopLoss);
            double takeProfitDistance = MathAbs(entry - takeProfit);

            if(type == POSITION_TYPE_BUY && currentPrice > entry){
                double currentProfit = currentPrice - entry;
                if(currentProfit >= stopLossDistance && entry > stopLoss){
                    ModifyStopLoss(ticket, entry + 5 * _Point, takeProfit);
                }
            } else if(type == POSITION_TYPE_SELL && currentPrice < entry){
                double currentProfit = entry - currentPrice;
                if(currentProfit >= stopLossDistance && entry < stopLoss) {
                    ModifyStopLoss(ticket, entry - 5 * _Point, takeProfit);
                }
            }
        }
    }
}

// Hàm hỗ trợ để điều chỉnh Stop Loss
void ModifyStopLoss(ulong ticket, double newStopLoss, double takeProfit){
    newStopLoss = NormalizeDouble(newStopLoss, _Digits);
    if (!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
}

void CloseAllPositions(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        string symbol = PositionGetString(POSITION_SYMBOL);
        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            double currentProfit = PositionGetDouble(POSITION_PROFIT);
            // Đóng lệnh ngay mà không cần kiểm tra Magic Number
            if(Trade.PositionClose(ticket)){
                Print("Closed position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}

string DetermineTrend(double ema50, double ema100){
    if(ema50 > ema100) return "BUY";
    if(ema50 < ema100) return "SELL";
    return "NEUTRAL";
}

color DetermineTrendColor(double ema50, double ema100){
    string trend = DetermineTrend(ema50, ema100);
    if(trend == "BUY") return clrGreen;
    if(trend == "SELL") return clrRed;
    return clrGray;
}

//+------------------------------------------------------------------+
//| Hàm gửi email thông báo khi có lệnh                              |
//+------------------------------------------------------------------+
void SendTradeEmail(string symbol, double price, double volume, ENUM_ORDER_TYPE orderType, double sl, string comment){
    // Thông tin email
    string subject = "Thông báo giao dịch - " + AccountInfoString(ACCOUNT_COMPANY);
    string body = "";
    
    // Xác định loại lệnh
    string orderTypeStr = "";
    switch(orderType){
        case ORDER_TYPE_BUY: orderTypeStr = "LỆNH MUA"; break;
        case ORDER_TYPE_SELL: orderTypeStr = "LỆNH BÁN"; break;
        case ORDER_TYPE_BUY_LIMIT: orderTypeStr = "LỆNH MUA LIMIT"; break;
        case ORDER_TYPE_SELL_LIMIT: orderTypeStr = "LỆNH BÁN LIMIT"; break;
        case ORDER_TYPE_BUY_STOP: orderTypeStr = "LỆNH MUA STOP"; break;
        case ORDER_TYPE_SELL_STOP: orderTypeStr = "LỆNH BÁN STOP"; break;
        default: orderTypeStr = "LỆNH KHÁC"; break;
    }
    
    // Tạo nội dung email
    body += "THÔNG BÁO GIAO DỊCH\n";
    body += "------------------\n";
    body += "Tài khoản: " + IntegerToString(AccountInfoInteger(ACCOUNT_LOGIN)) + "\n";
    body += "Sản phẩm: " + symbol + "\n";
    body += "Loại lệnh: " + orderTypeStr + "\n";
    body += "Giá: " + DoubleToString(price, (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS)) + "\n";
    body += "Khối lượng: " + DoubleToString(volume, 2) + "\n";
    body += "Stop Loss: " + (sl > 0 ? DoubleToString(sl, (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS)) : "Không") + "\n";
    body += "Thời gian: " + TimeToString(TimeCurrent(), TIME_DATE|TIME_SECONDS) + "\n";
    body += "Ghi chú: " + comment + "\n";
    body += "\nTự động gửi từ EA giao dịch";
    
    // Gửi email
    if(!SendMail(subject, body)){
        Print("Không thể gửi email thông báo! Lỗi: ", GetLastError());
    } else Print("Đã gửi email thông báo giao dịch thành công!");
}

//+------------------------------------------------------------------+
//| Hàm kiểm tra lệnh mới và gửi email                               |
//+------------------------------------------------------------------+
void CheckNewOrdersAndSendEmail(string comment){
    // Lấy lịch sử lệnh
    HistorySelect(TimeCurrent() - 60, TimeCurrent()); // Kiểm tra lệnh trong 1 phút qua
    
    int totalOrders = HistoryOrdersTotal();
    for(int i = 0; i < totalOrders; i++){
        ulong orderTicket = HistoryOrderGetTicket(i);
        if(orderTicket > 0){
            // Kiểm tra trạng thái lệnh
            ENUM_ORDER_STATE state = (ENUM_ORDER_STATE)HistoryOrderGetInteger(orderTicket, ORDER_STATE);
            if(state == ORDER_STATE_FILLED || state == ORDER_STATE_PARTIAL){
                // Lấy thông tin lệnh
                string symbol = HistoryOrderGetString(orderTicket, ORDER_SYMBOL);
                double price = HistoryOrderGetDouble(orderTicket, ORDER_PRICE_OPEN);
                double volume = HistoryOrderGetDouble(orderTicket, ORDER_VOLUME_CURRENT);
                ENUM_ORDER_TYPE orderType = (ENUM_ORDER_TYPE)HistoryOrderGetInteger(orderTicket, ORDER_TYPE);
                double sl = HistoryOrderGetDouble(orderTicket, ORDER_SL);
                double tp = HistoryOrderGetDouble(orderTicket, ORDER_TP);
                
                // Gửi email thông báo
                SendTradeEmail(symbol, price, volume, orderType, sl, comment);
            }
        }
    }
}

bool IsAccountOverInitialDrawdown(double maxAllowedDrawdownPercent){
   // Kiểm tra tham số hợp lệ
   if(DrawDownBalanceMax <= 0 || DrawDownBalanceMax >= 100){
      Alert("Giới hạn thâm hụt phải >0% và <100%");
      return false;
   }

   // Lấy equity hiện tại
   double currentEquity = AccountInfoDouble(ACCOUNT_EQUITY);
   
   // Tính % thâm hụt so với balance ban đầu
   double drawdownPercent = 0;
   if(initialBalance > 0){
      drawdownPercent = ((initialBalance - currentEquity) / initialBalance) * 100;
      drawdownPercent = NormalizeDouble(drawdownPercent, 2);
   }

   // Debug thông tin
   Print("Balance ban đầu: ", initialBalance, 
         " | Equity hiện tại: ", currentEquity,
         " | Thâm hụt: ", drawdownPercent, "%");

   // Kiểm tra ngưỡng
   if(drawdownPercent >= DrawDownBalanceMax){
      Alert("VƯỢT NGƯỠNG THÂM HỤT: ", drawdownPercent, "% (Giới hạn: ", DrawDownBalanceMax, "%)");
      return true;
   }
   
   return false;
}