#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton MoveAllSLButton;// Nút dời tất cả SL
CChartObjectButton CloseHalfButton;// Nút đóng một nửa khối lương tất cả giao dịch
CChartObjectButton CloseAllButton;// Nút đóng tất cả giao dịch
CChartObjectButton SLBE;// Nút dời SL về điểm hòa vốn
CChartObjectButton TPBE;// Nút dời TP về điểm hòa vốn

CChartObjectLabel lblTotalSL, lblTotalTP;

// Input parameters
input double   InpMaxLossAmount  = 15.0;    // Số tiền rủi ro tối đa trên mỗi lệnh (Ví dụ: 100$)
input int FIVE = 50; // Số pip để đặt SL/TP cách điểm hòa vốn

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";
const int PERIOD_EMA = 25;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool CloseHalfEnabled = false; // Biến kiểm soát đóng một nửa khối lương tất cả giao dịch
bool CloseAllEnabled = false; // Biến kiểm soát đóng toàn bộ vị thế
bool MoveAllSlEnabled = false; // Biến kiểm soát dời tất cả SL
bool SlBeEnabled = false; // Biến kiểm soát dời SL về điểm hòa vốn
bool TpBeEnabled = false; // Biến kiểm soát dời TP về điểm hòa

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    if(!CreateButton(CloseHalfButton, "CloseHalfButton", "HALF CLOSE", clrBlue, CalculateButtonY() - 100))
        return(INIT_FAILED);
    
    if(!CreateButton(CloseAllButton, "CloseAllButton", "CLOSE ALL", clrRed, CalculateButtonY()))
        return(INIT_FAILED);

    if(!CreateButton(MoveAllSLButton, "MoveAllSLButton", "MOVE ALL SL", clrNavy, CalculateButtonY() - 150))
        return(INIT_FAILED);

    if(!CreateButton(SLBE, "SLBEButton", "SL BE", clrSilver, CalculateButtonY() - 250))
        return(INIT_FAILED);
    
    if(!CreateButton(TPBE, "TPBEButton", "TP BE", clrGreen, CalculateButtonY() - 200))
        return(INIT_FAILED);
        
    // Tạo label
    if(!CreateLable(lblTotalSL, "TotalSLLabel", "Total SL: 0.00 USD", 8, 30))
        return(INIT_FAILED);

    if(!CreateLable(lblTotalTP, "TotalTP", "Total TP: 0.00 USD", 8, 60))
        return(INIT_FAILED);
        
    ObjectSetInteger(0, "CloseHalfButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "CloseAllButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "MoveAllSLButton", OBJPROP_ZORDER, 10);
    
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
        
        isRunningEa = false;
    }

    CalculateLable();
    CheckAndAdjustStopLoss();
}

void OnTick(){
    if(CloseHalfEnabled){
        CloseHalfEnabled = !CloseHalfEnabled;
        CloseHalfVolume();
    }
    
    if(MoveAllSlEnabled){
        MoveAllSlEnabled = !MoveAllSlEnabled;
        ManagePositions();
    }

    if(CloseAllEnabled){
        CloseAllEnabled = !CloseAllEnabled;
        CloseAllPositions();
    }

    if(SlBeEnabled){
        SlBeEnabled = !SlBeEnabled;
        MoveSLBE();
    }

    if(TpBeEnabled){
        TpBeEnabled = !TpBeEnabled;
        MoveTPBE();
    }
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
    // Nhấn nút đóng một nửa khối lượng
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "CloseHalfButton"){
        CloseHalfEnabled = !CloseHalfEnabled;
    }
    
    // Nhấn nút đóng tất cả
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "CloseAllButton"){
        CloseAllEnabled = !CloseAllEnabled;
    }
    
    // Nhấn nút dời SL
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "MoveAllSLButton"){
        MoveAllSlEnabled = !MoveAllSlEnabled;
    }

    // Nhấn nút dời SL về điểm hòa vốn
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "SLBEButton"){
        SlBeEnabled = !SlBeEnabled;
    }

    // Nhấn nút dời TP về điểm hòa vốn
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TPBEButton"){
        TpBeEnabled = !TpBeEnabled;
    }
}
void OnDeinit(const int reason){
    CloseHalfButton.Delete();
    CloseAllButton.Delete();
    MoveAllSLButton.Delete();
    SLBE.Delete();
    TPBE.Delete();
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
            double currentTP = PositionGetDouble(POSITION_TP);
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

                if(!Trade.PositionModify(ticket, newStoploss, currentTP)){
                        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
                }
            }
        }
    }
}

void MoveSLBE(){
    // Code dời SL về điểm hòa vốn
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            double currentTP = PositionGetDouble(POSITION_TP);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            
            double newStoploss = (type == POSITION_TYPE_BUY) ? openPrice + FIVE * _Point : openPrice - FIVE * _Point;
            
            if(!Trade.PositionModify(ticket, newStoploss, currentTP)){
                Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
            }
        }    
    }
}

void MoveTPBE(){
    // Code dời TP về điểm hòa vốn
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            double currentSL = PositionGetDouble(POSITION_SL);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            
            double newTakeProfit = (type == POSITION_TYPE_BUY) ? openPrice + FIVE * _Point : openPrice - FIVE * _Point;
            
            if(!Trade.PositionModify(ticket, currentSL, newTakeProfit)){
                Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
            }
        }
    }
}

int CalculateButtonX(){
    return (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
}

int CalculateButtonY(){
    return (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 50;
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

void CalculateLable(){
    double totalSl = 0;
    double totalTp = 0;
    
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    double pointValue = tickValue / tickSize;
    
    // Duyệt qua tất cả các vị thế hiện có
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);

        if(PositionSelectByTicket(ticket)){
            double sl = PositionGetDouble(POSITION_SL);
            double tp = PositionGetDouble(POSITION_TP);

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

                totalSl += diff * volume * pointValue * 100;
            }

            if(tp != 0){
                // Tính khoản lời tiềm năng (TP - Price) * Volume
                double diff = 0;
                
                // Tính chênh lệch TP theo hướng lệnh
                if(type == POSITION_TYPE_BUY){
                    diff = tp - price; // BUY: TP > Price = lời
                } else diff = price - tp; // SELL: TP < Price = lời

                totalTp += diff * volume * pointValue * 100;
            }
        }
    }
    
    // Cập nhật panel
    lblTotalSL.Description("Total SL: " + DoubleToString(totalSl, 2) + " USD");
    lblTotalTP.Description("Total TP: " + DoubleToString(totalTp, 2) + " USD");
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
                double currentTP = PositionGetDouble(POSITION_TP);
                double newSL = firstPositionSL;
                if(currentSL != newSL) ModifyStopLoss(ticket, newSL, currentTP);
            }
        }
    }
}

ulong GetFirstPositionTicket(){
    // Giả sử vị thế đầu tiên là index 0
    if(PositionsTotal() > 0) return PositionGetTicket(0);  
    
    return 0;  // Không có vị thế nào
}

void ModifyStopLoss(ulong ticket, double newStopLoss, double currentTP){
    newStopLoss = NormalizeDouble(newStopLoss, _Digits);
    if (!Trade.PositionModify(ticket, newStopLoss, currentTP)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
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

void CheckAndAdjustStopLoss(){
    // Duyệt qua tất cả các lệnh đang mở
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        
        if(PositionSelectByTicket(ticket)){

            double lot      = PositionGetDouble(POSITION_VOLUME);
            long type       = PositionGetInteger(POSITION_TYPE);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double currentSL = PositionGetDouble(POSITION_SL);
            double curentTP = PositionGetDouble(POSITION_TP);
            
            // 1. Tính toán khoảng cách SL (theo giá) tương ứng với số tiền mất
            // Công thức: Khoảng cách SL = Số tiền / (Khối lượng * TickValue / TickSize)
            double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
            double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
            
            if(tickValue <= 0) continue; // Tránh lỗi chia cho 0

            double slDistance = (InpMaxLossAmount / (lot * tickValue)) * tickSize;
            double targetSL = 0;

            // 2. Xác định mức giá SL mục tiêu
            if(type == POSITION_TYPE_BUY){
                targetSL = NormalizeDouble(priceOpen - slDistance, _Digits);
                
                // Nếu chưa có SL HOẶC SL đang đặt xa hơn mức tiền cho phép (thấp hơn targetSL)
                if(currentSL == 0 || currentSL < targetSL){
                    ModifyStopLoss(ticket, targetSL, curentTP);
                }
            } else if(type == POSITION_TYPE_SELL){
                targetSL = NormalizeDouble(priceOpen + slDistance, _Digits);
                
                // Nếu chưa có SL HOẶC SL đang đặt xa hơn mức tiền cho phép (cao hơn targetSL)
                if(currentSL == 0 || currentSL > targetSL){
                    ModifyStopLoss(ticket, targetSL, curentTP);
                }
            }
        }
    }
}