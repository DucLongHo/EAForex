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
CChartObjectButton MoveAllSL;// Nút dời tất cả SL

// Input parameters
input double LotSize = 0.05; // SL tối thiểu (points) để tính
input int Shift = 200; // Khoảng cách lề phải cho nút (pixels)
// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";
const int PERIOD_EMA = 25;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool TradingEnabled = false; // Biến kiểm soát trạng thái giao dịch
bool CloseAllPositionsEnabled = false; // Biến kiểm soát đóng toàn bộ vị thế
bool MoveAllSlEnabled = false; // Biến kiểm soát dời tất cả SL
string TradingTrend = BUY; // Biến kiểm soát trạng thái xu hướng

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+

int OnInit(){
    EventSetTimer(ONE);

    // Tạo nút và thiết lập thuộc tính
    if(!TradeButton.Create(0, "TradeButton", 0, CalculateButtonX(), 30, 175, 35))
        return(INIT_FAILED);
    
    TradeButton.Description("TRADING: OFF");
    TradeButton.Color(clrWhite);
    TradeButton.BackColor(clrRed); 
    TradeButton.FontSize(12);
    TradeButton.Font("Calibri");
    TradeButton.Selectable(true);

    // Tạo nút và thiết lập thuộc tính
    if(!TrendButton.Create(0, "TrendButton", 0, CalculateButtonX(), 130, 175, 35))
        return(INIT_FAILED);
    
    TrendButton.Description("TREND: BUY");
    TrendButton.Color(clrWhite);
    TrendButton.BackColor(clrGreen); 
    TrendButton.FontSize(12);
    TrendButton.Font("Calibri");
    TrendButton.Selectable(true);

    // Tạo nút và thiết lập thuộc tính
    if(!CloseAllButton.Create(0, "CloseAllButton", 0, CalculateButtonX(), 230, 175, 35))
        return(INIT_FAILED);
    
    CloseAllButton.Description("CLOSE ALL POSITIONS");
    CloseAllButton.Color(clrWhite);
    CloseAllButton.BackColor(clrBlue); 
    CloseAllButton.FontSize(12);
    CloseAllButton.Font("Calibri");
    CloseAllButton.Selectable(true);

    // Tạo nút và thiết lập thuộc tính
    if(!MoveAllSL.Create(0, "MoveAllSL", 0, CalculateButtonX(), 330, 175, 35))
        return(INIT_FAILED);
    
    MoveAllSL.Description("MOVE ALL SL");
    MoveAllSL.Color(clrWhite);
    MoveAllSL.BackColor(clrNavy); 
    MoveAllSL.FontSize(12);
    MoveAllSL.Font("Calibri");
    MoveAllSL.Selectable(true);

    ObjectSetInteger(0, "TradeButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "TrendButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "CloseAllButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "MoveAllSL", OBJPROP_ZORDER, 10);
    
    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, 0) + PeriodSeconds(PERIOD_M1);

    datetime closeTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);
    string timeString = TimeToString(closeTime - TimeCurrent(), TIME_SECONDS);

    Comment("Đếm ngược thời gian đóng nến: ", timeString, "\n", "\n");
    
    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime - currentTime <= TWO ){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
    }

    if(isRunningEa && TradingEnabled){
        Trade();
        isRunningEa = false;
    }
}

void OnTick(){
    if(MoveAllSlEnabled){
        MoveAllSlEnabled = !MoveAllSlEnabled;
        ManagePositions();
    }
    
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
        CloseAllPositionsEnabled = !CloseAllPositionsEnabled;

        // Đóng luôn giao dịch
        TradingEnabled = false;
        TradeButton.Description("TRADING: OFF");
        TradeButton.Color(clrWhite);
        TradeButton.BackColor(clrRed); // Màu đỏ khi tắt
    }
    // Nhấn nút dời SL
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "MoveAllSL"){
        MoveAllSlEnabled = !MoveAllSlEnabled;
    }
}

void Trade(){
    if(!IsCheckCandle()) return;
    
    if(TradingTrend == BUY){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

        if(!Trade.Buy(LotSize, _Symbol, entry)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }

    } else if(TradingTrend == SELL){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);

        if(!Trade.Sell(LotSize, _Symbol, entry)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

void CloseAllPositions(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        string symbol = PositionGetString(POSITION_SYMBOL);
        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}

bool IsCheckCandle(){
    double priceOpen = iOpen(_Symbol, PERIOD_M1, 0);
    double priceClose = iClose(_Symbol, PERIOD_M1, 0);
    double priceHigh = iHigh(_Symbol, PERIOD_M1, 0);
    double priceLow = iLow(_Symbol, PERIOD_M1, 0);

    double emaValue[];
    int handle = iMA(_Symbol, PERIOD_M1, PERIOD_EMA, 0, MODE_EMA, PRICE_CLOSE);;
    if(handle < 0) return false;
    ArraySetAsSeries(emaValue, true);
    if(CopyBuffer(handle, 0, 0, ONE, emaValue) <= 0) return false;

    if(TradingTrend == BUY){
        // Nến tăng thoả mãn
        if(priceClose < priceOpen)
            if((priceClose - priceLow) < (0.75) * (priceHigh - priceLow))
                return false;
        
        // Kiểm tra bấc nến
        if((priceHigh - priceClose) >= (0.5) * (priceHigh - priceLow)) return false;

        // Kiểm tra với ema
        if(priceClose <= emaValue[0]) return false;
    } else if(TradingTrend == SELL){
        // Nến giảm thoả mãn hoặc nến tăng rút râu
        if(priceClose > priceOpen)
            if((priceHigh - priceClose) < (0.75) * (priceHigh - priceLow))
                return false;

        // Kiểm tra bấc nến
        if((priceClose - priceLow) >= (0.5) * (priceHigh - priceLow)) return false;

        // Kiểm tra với ema
        if(priceClose >= emaValue[0]) return false;
    }

    return true;
}

void ManagePositions(){
    // Lấy thông tin vị thế đầu tiên
    if(PositionSelectByTicket(GetFirstPositionTicket())){
        double firstPositionSL = PositionGetDouble(POSITION_SL);  // SL của vị thế đầu tiên

        // Duyệt qua tất cả các vị thế hiện có
        for(int index = PositionsTotal() - ONE; index >= 0; index--){
            ulong ticket = PositionGetTicket(index);
            if(ticket == GetFirstPositionTicket()) continue;  // Bỏ qua vị thế đầu tiên

            if(PositionSelectByTicket(ticket)){
                double currentSL = PositionGetDouble(POSITION_SL);
                double newSL = firstPositionSL;
                if(currentSL != newSL) ModifyStopLoss(ticket, newSL);
            }
        }
    }
}

ulong GetFirstPositionTicket(){
    // Giả sử vị thế đầu tiên là index 0
    if(PositionsTotal() > 0) return PositionGetTicket(0);  
    
    return 0;  // Không có vị thế nào
}

void ModifyStopLoss(ulong ticket, double newStopLoss){
    newStopLoss = NormalizeDouble(newStopLoss, _Digits);
    if (!Trade.PositionModify(ticket, newStopLoss, 0)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
}

int CalculateButtonX(){
    return (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - Shift;;
}