//+------------------------------------------------------------------+
//|                   Expert Advisors                     |
//|                   Copyright 2025                                 |
//|                   Version 1.00                                   |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>
#include <Trade\DealInfo.mqh>  

CTrade Trade;
CChartObjectButton TradeButton;// Nút bật/tắt giao dịch
CChartObjectButton TrendButton;// Nút xu hướng giao dịch

// Input parameters
input double LotSize = 0.05; // SL tối thiểu (points) để tính
// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 
bool TradingEnabled = true; // Biến kiểm soát trạng thái giao dịch
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+

int OnInit(){
    EventSetTimer(ONE);

    // Tạo nút và thiết lập thuộc tính
    if(!TradeButton.Create(0, "TradeButton", 0, 400, 10, 150, 50))
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
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, ONE) + PeriodSeconds(PERIOD_M1);

    datetime closeTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);
    string timeString = TimeToString(closeTime - TimeCurrent(), TIME_SECONDS);

    Comment("Đếm ngược thời gian đóng nến: ", timeString, "\n", "\n");
    
    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime - currentTime <= 2 ){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
    }

    if(isRunningEa && TradingEnabled){
        TradeEMAs();
        isRunningEa = false;
    }
}

void OnTick(){
    // Kiểm tra xem có bật giao dịch không
    ManagePositions();
}

void OnDeinit(const int reason){
    TradeButton.Delete();
    EventKillTimer();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Kiểm tra sự kiện nhấp chuột trên nút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton") {
        TradingEnabled = !TradingEnabled; // Đảo ngược trạng thái giao dịch
        if(TradingEnabled) {
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
        string symbol = PositionGetString(POSITION_SYMBOL);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
           
        }
    }
}
