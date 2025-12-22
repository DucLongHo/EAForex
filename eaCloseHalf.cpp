//+------------------------------------------------------------------+
//|                   Expert Advisors                     |
//|                   Copyright 2025                                 |
//|                   Version 1.00                                   |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton MoveAllSLButton;// Nút dời tất cả SL
CChartObjectButton CloseAllButton;// Nút đóng tất cả giao dịch
CChartObjectLabel lblTotalSL, lblTotalPips;

// Input parameters
input int Shift = 200; // Khoảng cách lề phải cho nút (pixels)
// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;
const int FIVE = 5;

const string BUY = "BUY";
const string SELL = "SELL";
const int PERIOD_EMA = 25;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool CloseAllPositionsEnabled = false; // Biến kiểm soát đóng toàn bộ vị thế
bool MoveAllSlEnabled = false; // Biến kiểm soát dời tất cả SL

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    if(!CreateButton(CloseAllButton, "CloseAllButton", "HALF CLOSE", clrBlue, CalculateButtonY()))
        return(INIT_FAILED);

    if(!CreateButton(MoveAllSLButton, "MoveAllSLButton", "MOVE ALL SL", clrNavy, CalculateButtonY() - 50))
        return(INIT_FAILED);
        
    // Tạo label
    if(!CreateLable(lblTotalSL, "TotalSLLabel", "Total SL: 0.00 USD", 8, 30))
        return(INIT_FAILED);

    if(!CreateLable(lblTotalPips, "TotalPips", "Pips: 0 pips", 8, 60))
        return(INIT_FAILED);
        
    ObjectSetInteger(0, "CloseAllButton", OBJPROP_ZORDER, 10);
    
    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, 0) + PeriodSeconds(PERIOD_M1);

    
    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime - currentTime <= TWO ){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
    }

    if(isRunningEa){
        Draw();
        
        if(PositionsTotal() > 0){
            CalculateTotalStopLoss();
        } else {
            lblTotalSL.Description("Total SL: 0.00 USD");
        }
        isRunningEa = false;
    }
}

void OnTick(){
    if(CloseAllPositionsEnabled){
        CloseAllPositionsEnabled = !CloseAllPositionsEnabled;
        CloseHalfVolume();
    }
    
    if(MoveAllSlEnabled){
        MoveAllSlEnabled = !MoveAllSlEnabled;
        ManagePositions();
    }
    CalculateTotalPips();
    CalculateTotalStopLoss();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Nhấn nút close all
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "CloseAllButton"){
        CloseAllPositionsEnabled = !CloseAllPositionsEnabled;
    }
    
    // Nhấn nút dời SL
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "MoveAllSLButton"){
        MoveAllSlEnabled = !MoveAllSlEnabled;
    }
}
void OnDeinit(const int reason){
    CloseAllButton.Delete();
    MoveAllSLButton.Delete();
    EventKillTimer();
}

void CloseHalfVolume(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            double currentVolume = PositionGetDouble(POSITION_VOLUME);
            string symbol = PositionGetString(POSITION_SYMBOL);
            double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            
            double halfVolume = NormalizeDouble(currentVolume / 2.0, 2);
            
            MqlTradeRequest request = {};
            MqlTradeResult result = {};
             
            request.action = TRADE_ACTION_DEAL;
            request.symbol = symbol;
            request.volume = halfVolume;
            request.type = (type == POSITION_TYPE_BUY) ? ORDER_TYPE_SELL : ORDER_TYPE_BUY;
            request.price = (type == POSITION_TYPE_BUY) ? SymbolInfoDouble(symbol, SYMBOL_BID) : SymbolInfoDouble(symbol, SYMBOL_ASK);
            request.position = ticket;
            request.deviation = 10;
            
            if(!OrderSend(request, result)){
                Print("Failed to close half position. Error: ", GetLastError());
            } else {
               double newStoploss = (type == POSITION_TYPE_BUY) ? openPrice + FIVE * _Point : openPrice - FIVE * _Point;
               if(!Trade.PositionModify(ticket, newStoploss, 0)){
                    Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
               }
            }
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

bool CreateLable(CChartObjectLabel &lable, string name, string des, int x, int y){
    // Tạo lable và thiết lập thuộc tính
    if(!lable.Create(0, name, 0, CalculateButtonX(), y))
        return false;

    lable.Description(des);
    lable.Color(clrWhite);
    lable.Font("Calibri");
    lable.FontSize(12);

    return true;
}


void CalculateTotalPips(){
    double totalPips = 0;
    if(PositionsTotal() - ONE < 0){
        lblTotalPips.Description("Pips: 0 pips");
        return;
    }
    
    // Duyệt qua tất cả các vị thế hiện có
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);

        if(PositionSelectByTicket(ticket)){
            double currentPrice = 0;
            double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            
            // Lấy giá hiện tại tùy theo loại lệnh
            if(type == POSITION_TYPE_BUY){
                currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_BID);
            } else {
                currentPrice = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
            }
            
            // Tính số pip hiện tại
            double pipDiff = 0;
            if(type == POSITION_TYPE_BUY){
                pipDiff = currentPrice - openPrice; // BUY: giá tăng = lợi nhuận dương
            } else {
                pipDiff = openPrice - currentPrice; // SELL: giá giảm = lợi nhuận dương
            }

            totalPips = pipDiff; // Tính theo khối lượng
        }
    }
    
    // Cập nhật panel
    lblTotalPips.Description("Pips: " + DoubleToString(totalPips, TWO) + " pips");
}

void DrawMarkerPrice(ENUM_TIMEFRAMES timeframe, color lineColor){
    double emaValue[];
    int handle = iMA(_Symbol, timeframe, PERIOD_EMA, 0, MODE_EMA, PRICE_CLOSE);;
    if(handle < 0) return ;

    ArraySetAsSeries(emaValue, true);
    if(CopyBuffer(handle, 0, 0, ONE, emaValue) <= 0) return;

    double price = emaValue[0];
    if(price == 0) return;

    datetime currentTime = iTime(_Symbol, PERIOD_CURRENT, 0);
    datetime start = currentTime + PeriodSeconds(PERIOD_CURRENT) * 10;
    datetime end = currentTime + PeriodSeconds(PERIOD_CURRENT) * 2;
    
    string lineName = "Price " + DoubleToString(price);
    string textName = "TimeframeLabel_" + IntegerToString(timeframe);

    ObjectCreate(0, lineName, OBJ_TREND, 0, start, price, end, price);
    ObjectSetInteger(0, lineName, OBJPROP_COLOR, lineColor);
    ObjectSetInteger(0, lineName, OBJPROP_WIDTH, 2);
    
    ObjectCreate(0, textName, OBJ_TEXT, 0, start, price + 52 * _Point);
    ObjectSetString(0, textName, OBJPROP_TEXT, StringSubstr(EnumToString(timeframe), 7));
    ObjectSetInteger(0, textName, OBJPROP_COLOR, lineColor);
    ObjectSetInteger(0, textName, OBJPROP_ANCHOR, ANCHOR_LEFT_UPPER);
    ObjectSetInteger(0, textName, OBJPROP_FONTSIZE, 10);
}

void Draw(){
   ObjectsDeleteAll(0, -1, OBJ_TREND);
   ObjectsDeleteAll(0, -1, OBJ_TEXT);

   DrawMarkerPrice(PERIOD_H1, clrGray);
   DrawMarkerPrice(PERIOD_M15, clrTeal);
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
                } else diff = price - sl; // SELL: SL < Price = lỗ
            
                // Chuyển đổi sang USD
                double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
                double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
                double pointValue = tickValue / tickSize;
                //THE5ER
                totalSl += diff * volume * pointValue * 100;
                //EXNESS
                // totalSl += diff * volume * pointValue; 
            }
        }
    }
   // Cập nhật panel
   lblTotalSL.Description("Total Stoploss: " + DoubleToString(totalSl, 2) + " USD");
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