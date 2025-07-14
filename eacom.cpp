//+------------------------------------------------------------------+
//|                   Expert Advisors                     |
//|                   Copyright 2025                                 |
//|                   Version 1.00                                   |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;
CChartObjectButton TradeButton;// Nút bật/tắt giao dịch
CChartObjectButton TrendButton;// Nút xu hướng giao dịch
CChartObjectButton CloseAllButton;// Nút đóng tất cả giao dịch
CChartObjectButton MoveAllSL;// Nút dời tất cả SL
CChartObjectLabel lblTotalSL, lblTotalTrade;

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
    if(!CreateButton(TradeButton, "TradeButton", "TRADING: OFF", clrRed, 30))
        return(INIT_FAILED);
        
    if(!CreateButton(TrendButton, "TrendButton", "TREND: BUY", clrGreen, 130))
        return(INIT_FAILED);

    if(!CreateButton(CloseAllButton, "CloseAllButton", "CLOSE ALL POSITIONS", clrBlue, 230))
        return(INIT_FAILED);

    // Tạo nút và thiết lập thuộc tính
    if(!CreateButton(MoveAllSL, "MoveAllSL", "MOVE ALL SL", clrNavy, 330))
        return(INIT_FAILED);
    
    // Tạo label cho Total SL
    lblTotalSL.Create(0, "TotalSLLabel", 0, CalculateButtonX(), 370);
    lblTotalSL.Description("Total Stoploss: 0.00");
    lblTotalSL.Color(clrWhite);
    lblTotalSL.Font("Calibri");
    lblTotalSL.FontSize(12);
    
    // Tạo label cho Total Trade
    lblTotalTrade.Create(0, "lblTotalTrade", 0, CalculateButtonX(), 390);
    lblTotalTrade.Description("Total Lotsize: 0.00");
    lblTotalTrade.Color(clrWhite);
    lblTotalTrade.Font("Calibri");
    lblTotalTrade.FontSize(12);
       
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
    CloseAllButton.Delete();
    MoveAllSL.Delete();
    TrendButton.Delete();
    
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

    CalculateTotalVolume();
}

void CloseAllPositions(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            Trade.SetAsyncMode(true);
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
    Trade.SetAsyncMode(false);
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

    CalculateTotalStopLoss();
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

void CalculateTotalVolume(){
    double totalVolume = 0;

    // Duyệt qua tất cả các vị thế hiện có
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);

        if(PositionSelectByTicket(ticket)){
            totalVolume += PositionGetDouble(POSITION_VOLUME);
        }
    }
    // Cập nhật panel
    lblTotalTrade.Description("Total Lotsize: " + DoubleToString(totalVolume, 2)+ "Lot");
}

void CalculateTotalStopLoss(){
    double totalSl = 0;

    // Duyệt qua tất cả các vị thế hiện có
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);

        if(PositionSelectByTicket(ticket)){
            double sl = PositionGetDouble(POSITION_SL);
            double price = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            if(sl != 0){
                // Tính khoản lỗ tiềm năng (SL - Price) * Volume
               double diff = 0;
                
                // Tính chênh lệch SL theo hướng lệnh
                if(type == POSITION_TYPE_BUY){
                    diff = sl - price; // BUY: SL > Price = lỗ
                } else {
                    diff = price - sl; // SELL: SL < Price = lỗ
                }
            
                // Chuyển đổi sang USD
                double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
                double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
                double pointValue = tickValue / tickSize;
                totalSl += diff * volume * pointValue;
            }
        }
    }
   // Cập nhật panel
   lblTotalSL.Description("Total Lotsize: " + DoubleToString(totalSl, 2) + " USD");
}