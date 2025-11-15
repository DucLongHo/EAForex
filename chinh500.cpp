//+------------------------------------------------------------------+
//|                   BobVolmanEma Expert Advisors                   |
//|                   Copyright 2025, DucLong                        |
//|                   Version 1.03 - Thêm điều kiện chốt lãi         |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton ButtonSetup50;// Nút đóng mở setup 50 phút
CChartObjectButton ButtonSetup30;// Nút đóng mở setup 30 phút
CChartObjectButton ButtonSetup5; // Nút đóng mở setup 5 phút

// Input parameters
input double RiskTrade = 0.01; // Khối lượng giao dịch
input string TypeOrder = "BUY"; // Tham gia BUY/SELL thị trường(Chỉ điền "BUY", :"SELL")
input string TradingTimeStartAM = "09:00"; // Thời gian VN bắt đầu buổi sáng (HH:MM)
input string TradingTimeEndAM = "16:30"; // Thời gian VN kết thúc buổi sáng (HH:MM)
input int Shift = 200; // Khoảng cách lề phải cho nút (pixels)

// Constant data
const int ONE = 1;
const int TWO = 2;
const int TEN = 10;

datetime LastCheckTime; // Biến kiểm tra giá chạy 5p một lần 
bool IsTradeFirst = true;
bool IsTradeSecond = true;

bool TradingEnabledM50 = true; // Biến kiểm soát trạng thái giao dịch
bool TradingEnabledM30 = true; // Biến kiểm soát trạng thái giao dịch
bool TradingEnabledM5 = true; // Biến kiểm soát trạng thái giao dịch

int OnInit(){
    EventSetTimer(ONE);

    //Mở nút setup 50 phút
    if(!CreateButton(ButtonSetup50, "TradeButton50", "TRADING 50Minute: ON", clrGreen, 60))
        return(INIT_FAILED);
    //Mở nút setup 30 phút
    if(!CreateButton(ButtonSetup30, "TradeButton30", "TRADING 30Minute: ON", clrGreen, 140))
        return(INIT_FAILED);
    //Mở nút setup 5 phút
    if(!CreateButton(ButtonSetup5, "TradeButton5", "TRADING 5Minute: ON", clrGreen, 220))
        return(INIT_FAILED);

    ObjectSetInteger(0, "TradeButton50", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "TradeButton30", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "TradeButton5", OBJPROP_ZORDER, 10);

    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    bool isRunningEa = false;
    datetime currentTime = TimeCurrent();
    
    if(currentTime - LastCheckTime > TEN || !LastCheckTime){
        LastCheckTime = LastCheckTime + TEN;
        
        isRunningEa = true;
    }
    
    if(isRunningEa && IsTradingTime(TradingTimeStartAM, TradingTimeEndAM)){
        if(TradingEnabledM50) TradeMarketMin50();
        if(TradingEnabledM30) TradeMarketMin30();
        if(TradingEnabledM5) TradeMarketMin5();
        
        isRunningEa = false;
    }
}

void OnDeinit(const int reason){
    ButtonSetup50.Delete();
    ButtonSetup30.Delete();
    ButtonSetup5.Delete();
    
    EventKillTimer();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Kiểm tra sự kiện nhấp chuột trên nút 50 phút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton50"){
        TradingEnabledM50 = !TradingEnabledM50; // Đảo ngược trạng thái giao dịch
        if(TradingEnabledM50){
            ButtonSetup50.Description("TRADING 50Minute: ON");
            ButtonSetup50.Color(clrWhite);
            ButtonSetup50.BackColor(clrGreen); // Màu xanh lá khi bật
        } else {
            ButtonSetup50.Description("TRADING 50Minute: OFF");
            ButtonSetup50.Color(clrWhite);
            ButtonSetup50.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    
    // Kiểm tra sự kiện nhấp chuột trên nút 30 phút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton30"){
        TradingEnabledM30 = !TradingEnabledM30; // Đảo ngược trạng thái giao dịch
        if(TradingEnabledM30){
            ButtonSetup30.Description("TRADING 30Minute: ON");
            ButtonSetup30.Color(clrWhite);
            ButtonSetup30.BackColor(clrGreen); // Màu xanh lá khi bật
        } else {
            ButtonSetup30.Description("TRADING 30Minute: OFF");
            ButtonSetup30.Color(clrWhite);
            ButtonSetup30.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    
    // Kiểm tra sự kiện nhấp chuột trên nút 5 phút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton5"){
        TradingEnabledM5 = !TradingEnabledM5; // Đảo ngược trạng thái giao dịch
        if(TradingEnabledM5){
            ButtonSetup5.Description("TRADING 5Minute: ON");
            ButtonSetup5.Color(clrWhite);
            ButtonSetup5.BackColor(clrGreen); // Màu xanh lá khi bật
        } else {
            ButtonSetup5.Description("TRADING 5Minute: OFF");
            ButtonSetup5.Color(clrWhite);
            ButtonSetup5.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    ChartRedraw(0);
}


bool IsTradingTime(string TradingTimeStart, string TradingTimeEnd){
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
    if(startTotal > endTotal) {
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

void TradeMarketMin50(){
    MqlDateTime currentTime;
    TimeCurrent(currentTime);
    
    TradeFirstOrder(50, 30, currentTime);
    TradeSecondOrder(51, 10, currentTime);
    
    if(currentTime.min == 52 && currentTime.sec == 30){
        IsTradeSecond = true;
        closeAllPositions();
    }
}

void TradeMarketMin5(){
    MqlDateTime currentTime;
    TimeCurrent(currentTime);
    
    TradeFirstOrder(5, 30, currentTime);
    TradeSecondOrder(6, 10, currentTime);
    
    if(currentTime.min == 7 && currentTime.sec == 30){
        IsTradeSecond = true;
        closeAllPositions();
    }
}

void TradeMarketMin30(){
    MqlDateTime currentTime;
    TimeCurrent(currentTime);
    
    TradeFirstOrder(30, 30, currentTime);
    TradeSecondOrder(31, 10, currentTime);
    
    if(currentTime.min == 32 && currentTime.sec == 30){
        IsTradeSecond = true;
        closeAllPositions();
    }
}

void TradeFirstOrder(int minute, int section, MqlDateTime &currentTime){
    if(currentTime.min == minute && currentTime.sec == section && IsTradeFirst){
        IsTradeFirst = false;
        if(TypeOrder == "BUY"){
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
            
            if(!Trade.Buy(RiskTrade, _Symbol, entry)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            } else {
                Alert("Tham Gia Thị Trường");
            }
        } else if(TypeOrder == "SELL"){
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
            
            if(!Trade.Sell(RiskTrade, _Symbol, entry)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            } else {
                Alert("Tham Gia Thị Trường");
            }
        }
    }
}

void TradeSecondOrder(int minute, int section, MqlDateTime &currentTime){
   if(currentTime.min == minute && currentTime.sec == section && IsTradeSecond){
        IsTradeFirst = true;
        IsTradeSecond = false;

        if(TypeOrder == "BUY"){
            double entry = GetCurrentPositionEntryPrice();
            double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

            if(currentPrice < entry){
                if(!Trade.Buy(RiskTrade, _Symbol, currentPrice)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
                } else {
                Alert("Tham Gia Thị Trường");
                }
            }
        } else if(TypeOrder == "SELL"){
            double entry = GetCurrentPositionEntryPrice();
            double currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
            
            if(currentPrice > entry){
                if(!Trade.Sell(RiskTrade, _Symbol, currentPrice)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
                } else {
                Alert("Tham Gia Thị Trường");
                }
            }
        }
   }
}

double GetCurrentPositionEntryPrice(){
   double entryPrice = 0;
   
   if(PositionSelect(_Symbol)){
      entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
      Print("Entry price position: ", entryPrice);
   } else Print("Không có position cho ", _Symbol);
   
   return entryPrice;
}

void closeAllPositions(){
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

int CalculateButtonX(){
    return (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - Shift;
}

int CalculateButtonY(){
    return (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 100;
}

bool CreateButton(CChartObjectButton &button, string name, string des, color bgColor, int y){
    // Tạo nút và thiết lập thuộc tính
    if(!button.Create(0, name, 0, CalculateButtonX(), y, 175, 35))
        return false;
    
    button.Description(des);
    button.Color(clrWhite);
    button.BackColor(bgColor); 
    button.FontSize(12);
    button.Font("Calibri");
    button.Selectable(true);
    
    return true;
}