//--- BỘ KHUNG KHỞI TẠO ---
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>
#include <Trade\DealInfo.mqh>  

CTrade Trade;

int OnInit(){
    EventSetTimer(1);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M5 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M5, 0) + PeriodSeconds(PERIOD_M5);
    
    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime && 
        currentCandleCloseTime - currentTime <= 2){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
        // Thực hiện các hành động cần thiết khi đóng nến M5

    }
    
    if(isRunningEa) isRunningEa = false;
}

void OnTick(){

}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Nhấn nút đóng EA
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "OnOffButton"){
        OnOffEnabled = !OnOffEnabled;
        
        if(OnOffEnabled){
            OnOffButton.Description("ON");
            OnOffButton.Color(clrWhite);
            OnOffButton.BackColor(clrGreen);
        } else {
            OnOffButton.Description("OFF");
            OnOffButton.Color(clrWhite);
            OnOffButton.BackColor(clrRed);
        }
    }
}

void OnDeinit(const int reason){
    EventKillTimer();
}
//--- LẤY CÁC THÔNG SỐ ---
double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);
long accID = AccountInfoInteger(ACCOUNT_LOGIN);
double currentEquity  = AccountInfoDouble(ACCOUNT_EQUITY);
double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);

//--- CÁC HÀM HỖ TRỢ ---

// Hàm hỗ trợ để điều chỉnh Stop Loss
void ModifyStopLoss(ulong ticket, double newStopLoss = 0, double takeProfit = 0){

    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
}

// Hàm đóng tất cả các vị thế đang mở
void CloseAllPositions(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            // Đóng lệnh ngay mà không cần kiểm tra Magic Number
            if(Trade.PositionClose(ticket)){
                Print("Closed position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}


// Hàm tính toán kích thước lot dựa trên khoảng cách Stop Loss và mức rủi ro đã định trước
input double RiskTrade = 100; // Rủi ro long trade (USD)
double GetLotSize(double stopLossDistance){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    double stopLossPips = stopLossDistance / _Point;

    double pipValue = tickValue / (tickSize / _Point);
   
    double lotSize = RiskTrade / (stopLossPips * pipValue);
   
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
   
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lotSize = MathFloor(lotSize / lotStep) * lotStep;

    return lotSize;
}

// Hàm quản lý vị thế
void ManagePositions(){
    // Lặp qua tất cả các vị thế đang mở
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        // Lấy thông tin chi tiết của vị thế
        double entry = PositionGetDouble(POSITION_PRICE_OPEN);
        double stopLoss = PositionGetDouble(POSITION_SL);
        double takeProfit = PositionGetDouble(POSITION_TP);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
           
        }
    }
}

// Hàm setup thời gian trade
input string TradingTimeStart = "00:00";  // Thời gian VN bắt đầu buổi sáng (HH:MM)
input string TradingTimeEnd = "18:00";    // Thời gian VN kết thúc buổi sáng (HH:MM)

bool IsTradingTime(){
    datetime brokerTime = TimeCurrent(); // Thời gian hiện tại theo broker
    // Chuyển sang giờ Việt Nam
    datetime vietnamTime = brokerTime + GetBrokerTimezoneOffset() * 3600;

    MqlDateTime timeStruct;
    TimeToStruct(vietnamTime, timeStruct);
    int vnHour = timeStruct.hour;
    int vnMinute = timeStruct.min;
    
    // Phân tích thời gian nhập vào
    string startParts[];
    if(StringSplit(TradingTimeStart, ':', startParts) != 2) return false;
    int startHour = (int)StringToInteger(startParts[0]);
    int startMinute = (int)StringToInteger(startParts[1]);
    
    string endParts[];
    if(StringSplit(TradingTimeEnd, ':', endParts) != 2) return false;
    int endHour = (int)StringToInteger(endParts[0]);
    int endMinute = (int)StringToInteger(endParts[1]);
    
    // So sánh thời gian
    int currentTotal = vnHour * 60 + vnMinute;
    int startTotal = startHour * 60 + startMinute;
    int endTotal = endHour * 60 + endMinute;
    
    // Xử lý trường hợp qua đêm (ví dụ: 22:00 tối đến 05:00 sáng)
    if (startTotal > endTotal) {
        // Khoảng thời gian qua đêm
        return (currentTotal >= startTotal || currentTotal <= endTotal);
    } else {
        // Khoảng thời gian trong cùng một ngày
        return (currentTotal >= startTotal && currentTotal <= endTotal);
    }
}

int GetBrokerTimezoneOffset(){
   // Lấy thời gian local và server, tính chênh lệch
   datetime serverTime = TimeCurrent();
   datetime localTime = TimeLocal();
   
   int diff = int((localTime - serverTime) / 3600);
   return diff;
}

// Hàm quản lý số lượng lệnh mở trong ngày
input int MaxOrdersPerDay = 3; // Số lượng lệnh tối đa mỗi
bool IsDailyOrderLimit(){
    int dailyOrders = 0;
    datetime currentDay = iTime(_Symbol, PERIOD_D1, 0); // Thời gian bắt đầu của ngày hiện tại
   
    // Tạo mảng lưu trữ các lệnh đã kiểm tra để tránh trùng lặp
    ulong checkedOrders[10];
    int countCheckedOrders = 0;
    // 1. Đếm các vị thế đang mở (positions)
    for(int index = 0; index < PositionsTotal(); index++){
        ulong ticket = PositionGetTicket(index);
        if(ticket > 0 && PositionSelectByTicket(ticket)){
            if(PositionGetString(POSITION_SYMBOL) == _Symbol && 
                PositionGetInteger(POSITION_TIME) >= currentDay){
                dailyOrders++;
                countCheckedOrders++;
                checkedOrders[countCheckedOrders - 1] = ticket;
            }
        }
    }
   
    // 2. Đếm các lệnh đang chờ (pending orders)
    for(int index = 0; index < OrdersTotal(); index++){
        ulong ticket = OrderGetTicket(index);
        if(ticket > 0){
            if(OrderGetString(ORDER_SYMBOL) == _Symbol && 
                OrderGetInteger(ORDER_TIME_SETUP) >= currentDay)
                dailyOrders++;
        }
    }
   
    // 3. Đếm lịch sử lệnh (history orders), bỏ qua các lệnh đã có position tương ứng
    if(HistorySelect(currentDay, TimeCurrent())){
        int totalHistoryOrders = HistoryOrdersTotal();
        for(int index = 0; index < totalHistoryOrders; index++){
            ulong ticket = HistoryOrderGetTicket(index);
            // Kiểm tra xem lệnh này đã được tính qua position chưa
            bool alreadyCounted = false;
            for(int indexCheckedOrders = 0; indexCheckedOrders < 10; indexCheckedOrders++){
                if(ticket == checkedOrders[indexCheckedOrders]){
                    alreadyCounted = true;
                    break;
                }
            }  
         
            if(!alreadyCounted && 
                HistoryOrderGetString(ticket, ORDER_SYMBOL) == _Symbol && 
                HistoryOrderGetInteger(ticket, ORDER_TIME_DONE) >= currentDay){
                if(HistoryDealGetInteger(ticket, DEAL_ENTRY) == DEAL_ENTRY_OUT)
                    dailyOrders++;
            }
        }
    }
   
    if(dailyOrders >= MaxOrdersPerDay){
        Print("Đã đạt giới hạn ", MaxOrdersPerDay, " lệnh trong ngày. Không thể mở thêm lệnh.");
        return false;
    }
   
    return true;
}

//--- SETUP EMA ---
double ema[];

int handle = iMA(_Symbol, _Period, 25, 0, MODE_EMA, PRICE_CLOSE);
if(handle < 0) return;

ArraySetAsSeries(ema, true);
if(CopyBuffer(handle, 0, 0, 1, ema) <= 0) return; // Lấy giá trị EMA mới nhất

//--- DRAW ---
ObjectsDeleteAll(0, -1, OBJ_TREND);// Xóa các đối tượng trend

void DrawFiboInChart(double startPoint, double endPoint){
    // Xóa tất cả các đường xu hướng
    ObjectsDeleteAll(0, -1, OBJ_TREND);
    
    datetime startTime = iTime(NULL, 0, 0);
    datetime endTime = iTime(NULL, 0, TWENTY_CANDLES);

    double priceStart = startPoint;
    double priceEnd = endPoint;
    double price618 = GetFibo618Data(startPoint, endPoint);

    if (priceStart == 0 || priceEnd == 0) return;

    string nameStart = "Start Fibo ";
    string nameEnd = "End Fibo ";

    // Tạo và thiết lập thuộc tính của đường xu hướng bắt đầu
    ObjectCreate(0, nameStart, OBJ_TREND, 0, startTime, priceStart, endTime, priceStart);
    ObjectSetInteger(0, nameStart, OBJPROP_COLOR, clrGreen);
    ObjectSetInteger(0, nameStart, OBJPROP_WIDTH, 2);

    // Tạo và thiết lập thuộc tính của đường xu hướng kết thúc
    ObjectCreate(0, nameEnd, OBJ_TREND, 0, startTime, priceEnd, endTime, priceEnd);
    ObjectSetInteger(0, nameEnd, OBJPROP_COLOR, clrRed);
    ObjectSetInteger(0, nameEnd, OBJPROP_WIDTH, 2);
}

int CalculateButtonX(){
    return (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
}

int CalculateButtonY(){
    return (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 50;
}

bool CreateButton(CChartObjectButton &button, string name, string des, color bgColor,){
    // Tạo nút và thiết lập thuộc tính
    if(!button.Create(0, name, 0, CalculateButtonX(), CalculateButtonY(), 175, 35))
        return false;
    
    button.Description(des);
    button.Color(clrWhite);
    button.BackColor(bgColor); 
    button.FontSize(12);
    button.Font("Calibri");
    button.Selectable(true);
    
    return true;
}

//--- GỬI THÔNG TIN ---

// Hàm gửi email thông báo giao dịch
void SendTradeEmail(double price, double volume, ENUM_ORDER_TYPE orderType, double StopLoss, string comment){
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
    body += "Sản phẩm: " + _Symbol + "\n";
    body += "Loại lệnh: " + orderTypeStr + "\n";
    body += "Giá: " + DoubleToString(price, (int)SymbolInfoInteger(_Symbol, SYMBOL_DIGITS)) + "\n";
    body += "Khối lượng: " + DoubleToString(volume, 2) + "\n";
    body += "Stop Loss: " + (StopLoss > 0 ? DoubleToString(StopLoss, (int)SymbolInfoInteger(_Symbol, SYMBOL_DIGITS)) : "Không") + "\n";
    body += "Thời gian: " + TimeToString(TimeCurrent(), TIME_DATE|TIME_SECONDS) + "\n";
    body += "Ghi chú: " + comment + "\n";
    body += "\nTự động gửi từ EA giao dịch";
    
    // Gửi email
    if(!SendMail(subject, body)){
        Print("Không thể gửi email thông báo! Lỗi: ", GetLastError());
    } else Print("Đã gửi email thông báo giao dịch thành công!");
}

// Hàm gửi tin nhắn về Telegram
string botToken = ""; // Token bot
string chatId = ""; // ID chat

void SendTelegram(string text) {
    string url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + text;
    char post[], result[];
    string headers;
    WebRequest("GET", url, headers, 1000, post, result, headers);
}