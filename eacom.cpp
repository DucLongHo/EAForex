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
CChartObjectButton CloseAllButton;// Nút đóng tất cả giao dịch

// Input parameters
input double LotSize = 0.05; // SL tối thiểu (points) để tính
// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool TradingEnabled = true; // Biến kiểm soát trạng thái giao dịch
bool CloseAllPositionsEnabled = true; // Biến kiểm soát đóng toàn bộ vị thế
string TradingTrend = BUY; // Biến kiểm soát trạng thái xu hướng

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+

int OnInit(){
    EventSetTimer(ONE);

    // Tạo nút và thiết lập thuộc tính
    if(!TradeButton.Create(0, "TradeButton", 0, 400, 10, 150, 35))
        return(INIT_FAILED);
    
    TradeButton.Description("TRADING: ON");
    TradeButton.Color(clrWhite);
    TradeButton.BackColor(clrGreen); 
    TradeButton.FontSize(12);
    TradeButton.Font("Calibri");
    TradeButton.Selectable(true);

    // Tạo nút và thiết lập thuộc tính
    if(!TrendButton.Create(0, "TrendButton", 0, 600, 10, 150, 35))
        return(INIT_FAILED);
    
    TrendButton.Description("TREND: BUY");
    TrendButton.Color(clrWhite);
    TrendButton.BackColor(clrGreen); 
    TrendButton.FontSize(12);
    TrendButton.Font("Calibri");
    TrendButton.Selectable(true);

    // Tạo nút và thiết lập thuộc tính
    if(!CloseAllButton.Create(0, "CloseAllButton", 0, 800, 10, 175, 35))
        return(INIT_FAILED);
    
    CloseAllButton.Description("CLOSE ALL POSITIONS");
    CloseAllButton.Color(clrWhite);
    CloseAllButton.BackColor(clrBlue); 
    CloseAllButton.FontSize(12);
    CloseAllButton.Font("Calibri");
    CloseAllButton.Selectable(true);

    ObjectSetInteger(0, "TradeButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "TrendButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "CloseAllButton", OBJPROP_ZORDER, 10);

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
        Trade();
        isRunningEa = false;
    }
}

void OnTick(){
    // Kiểm tra xem có bật giao dịch không
    ManagePositions();

    if(CloseAllPositionsEnabled){
        CloseAllPositionsEnabled = !CloseAllPositionsEnabled;
        CloseAllPositions();
    }
}

void OnDeinit(const int reason){
    TradeButton.Delete();
    EventKillTimer();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Kiểm tra sự kiện nhấp chuột trên nút
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton"){
        TradingEnabled = !TradingEnabled; // Đảo ngược trạng thái giao dịch
        if(TradingEnabled){
            TradeButton.Description("TRADING: ON");
            TradeButton.Color(clrWhite);
            TradeButton.BackColor(clrGreen); // Màu xanh lá khi bật
        } else{
            TradeButton.Description("TRADING: OFF");
            TradeButton.Color(clrWhite);
            TradeButton.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    // Nhấn nút trend
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TrendButton"){
        // Đảo ngược trạng thái xu hướng
        TradingTrend = TradingTrend == BUY ? SELL : BUY; 
        if(TradingTrend == BUY){
            TrendButton.Description("TREND: BUY");
            TrendButton.Color(clrWhite);
            TrendButton.BackColor(clrGreen); // Màu xanh lá khi bật
        } else if(TradingTrend == SELL){
            TrendButton.Description("TREND: SELL");
            TrendButton.Color(clrWhite);
            TrendButton.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    // Nhấn nút close all
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "CloseAllButton"){
        if(!CloseAllPositionsEnabled){
            CloseAllPositionsEnabled = !CloseAllPositionsEnabled;
        }
    }
}

void Trade(){

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

void CloseAllPositions(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        string symbol = PositionGetString(POSITION_SYMBOL);
        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            if(Trade.PositionClose(ticket)){
                Print("Closed position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}