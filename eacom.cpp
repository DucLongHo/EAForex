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
CChartObjectButton MoveAllSLButton;// Nút dời tất cả SL
CChartObjectButton AlertButton;// Nút cảnh báo

CChartObjectLabel lblTotalSL, lblTotalTrade, lblSetup, lblFunctions;

// Input parameters
input double LotSize = 0.05; // SL tối thiểu (points) để tính
input double RiskFirstPos = 20; // Sl của vị thế đầu tiên (Usd)
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
bool AlertEnabled = false; // Biến kiểm soát bật/tắt cảnh báo
string TradingTrend = BUY; // Biến kiểm soát trạng thái xu hướng

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+

int OnInit(){
    EventSetTimer(ONE);
    // Tiêu đề các nút setup
    if(!CreateLable(lblSetup, "lblSetup", "// SETUP BUTTON //", CalculateButtonX(), 30))
        return(INIT_FAILED);
    // Tạo nút và thiết lập thuộc tính
    if(!CreateButton(TradeButton, "TradeButton", "TRADING: OFF", clrRed, 60))
        return(INIT_FAILED);
        
    if(!CreateButton(TrendButton, "TrendButton", "TREND: BUY", clrGreen, 140))
        return(INIT_FAILED);

    // Tiêu đề các nút chức năng
    if(!CreateLable(lblFunctions, "lblFunctions", "// FUNCTION BUTTONS //", CalculateButtonX(), 190))
        return(INIT_FAILED);

    if(!CreateButton(CloseAllButton, "CloseAllButton", "CLOSE ALL POSITIONS", clrBlue, CalculateButtonY()))
        return(INIT_FAILED);

    // Tạo nút và thiết lập thuộc tính
    if(!CreateButton(MoveAllSLButton, "MoveAllSLButton", "MOVE ALL SL", clrNavy, 220))
        return(INIT_FAILED);

    if(!CreateButton(AlertButton, "AlertButton", "ALERT PRICE EMA: OFF", clrRed, 300))
        return(INIT_FAILED);

    // Tạo label cho Total SL
    if(!CreateLable(lblTotalSL, "TotalSLLabel", "Total Stoploss: 0.00 USD", 5, 30))
        return(INIT_FAILED);

    // Tạo label cho Total Trade
    if(!CreateLable(lblTotalTrade, "TotalTradeLable", "Total Lotsize: 0.00 Lot", 5, 60))
        return(INIT_FAILED);

    ObjectSetInteger(0, "TradeButton", OBJPROP_ZORDER, 10);
    ObjectSetInteger(0, "TrendButton", OBJPROP_ZORDER, 10);
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
        if(TradingEnabled){
            Trade();
            
            if(CheckBreakEma()){
               CloseAllPositions();
               CloseEA();
            }
        }
        
        if(AlertEnabled){
            CheckPriceAlert();
        }

        if(PositionsTotal() > 0){
            CalculateTotalStopLoss();
            CalculateTotalVolume();
        }
        
        Draw();
        
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
        // Cập nhật panel
        lblTotalTrade.Description("Total Lotsize: 0.00 Lot");
        lblTotalSL.Description("Total Stoploss: 0.00 USD");
    }
}

void OnDeinit(const int reason){
    TradeButton.Delete();
    CloseAllButton.Delete();
    MoveAllSLButton.Delete();
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
    // Nhấn nút cảnh báo
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "AlertButton"){
        AlertEnabled = !AlertEnabled; // Đảo ngược trạng thái cảnh báo
        if(AlertEnabled){
            AlertButton.Description("ALERT PRICE EMA: ON");
            AlertButton.Color(clrWhite);
            AlertButton.BackColor(clrGreen); // Màu xanh lá khi bật
        } else{
            AlertButton.Description("ALERT PRICE EMA: OFF");
            AlertButton.Color(clrWhite);
            AlertButton.BackColor(clrRed); // Màu đỏ khi tắt
        }
    }
    // Nhấn nút close all
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "CloseAllButton"){
        CloseAllPositionsEnabled = !CloseAllPositionsEnabled;

        CloseEA();
    }
    // Nhấn nút dời SL
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "MoveAllSLButton"){
        MoveAllSlEnabled = !MoveAllSlEnabled;
    }
}

void Trade(){
    if(!IsCheckCandle()) return;

    double firstPositionSL = 0, firstPositionEntry = 0;
    if(PositionSelectByTicket(GetFirstPositionTicket())){
        firstPositionSL = PositionGetDouble(POSITION_SL);  // SL của vị thế đầu tiên
        firstPositionEntry = PositionGetDouble(POSITION_PRICE_OPEN); // Giá mở của vị thế đầu tiên
    }

    if(TradingTrend == BUY){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        if(GetFirstPositionTicket() == 0){
            double diffSl = CalculateStopLossPoints();
            if(!Trade.Buy(LotSize, _Symbol, entry, entry - diffSl)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        } else if(firstPositionEntry <= entry){
            if(!Trade.Buy(LotSize, _Symbol, entry, firstPositionSL)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        }
    } else if(TradingTrend == SELL){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        if(GetFirstPositionTicket() == 0){
            double diffSl = CalculateStopLossPoints();
            if(!Trade.Sell(LotSize, _Symbol, entry, entry + diffSl)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
        } else  if(firstPositionEntry >= entry){
            if(!Trade.Sell(LotSize, _Symbol, entry, firstPositionSL)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
        }
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
    if(!lable.Create(0, name, 0, x, y))
        return false;

    lable.Description(des);
    lable.Color(clrWhite);
    lable.Font("Calibri");
    lable.FontSize(12);

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
    lblTotalTrade.Description("Total Lotsize: " + DoubleToString(totalVolume, 2)+ " Lot");
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
                totalSl += diff * volume * pointValue;
            }
        }
    }
   // Cập nhật panel
   lblTotalSL.Description("Total Stoploss: " + DoubleToString(totalSl, 2) + " USD");
}

double CalculateStopLossPoints(){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    // Kiểm tra giá trị hợp lệ
    if (tickValue <= 0 || tickSize <= 0) {
        return 0.0;
    }

    // Tính giá trị pip
    double pipValue = tickValue / tickSize;

    // Tính stop loss theo points
    double stopLossPoints = RiskFirstPos / (LotSize * pipValue);

    // Đảm bảo stop loss không âm và làm tròn đến số nguyên
    stopLossPoints = MathMax(stopLossPoints, 0);
    stopLossPoints = MathRound(stopLossPoints);

    return stopLossPoints;
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

   DrawMarkerPrice(PERIOD_M5, clrGray);
   DrawMarkerPrice(PERIOD_M15, clrTeal);
}

bool CheckBreakEma(){
    if(PositionsTotal() == 0) return false;

    double emaValue[];
    int handle = iMA(_Symbol, PERIOD_M1, PERIOD_EMA, 0, MODE_EMA, PRICE_CLOSE);;
    if(handle < 0) return false;

    ArraySetAsSeries(emaValue, true);
    if(CopyBuffer(handle, 0, 0, TWO, emaValue) <= 0) return false;

    double currentEma = emaValue[0], preEma = emaValue[ONE];
    if(currentEma == 0 || preEma == 0) return false;
   
    double currentClose = iClose(_Symbol, PERIOD_M1, 0);
    double currentOpen = iOpen(_Symbol, PERIOD_M1, 0);
    double preClose = iClose(_Symbol, PERIOD_M1, ONE);
    double preOpen = iOpen(_Symbol, PERIOD_M1, ONE);

    if(TradingTrend == BUY){
        if(currentClose < currentEma && preClose < preEma)
            return true;        
    } else if(TradingTrend == SELL){
        if(currentClose > currentEma && preClose > preEma)
            return true;  
    }
    
    return false;
}

void CloseEA(){
    // Đóng luôn giao dịch
    TradingEnabled = false;
    TradeButton.Description("TRADING: OFF");
    TradeButton.Color(clrWhite);
    TradeButton.BackColor(clrRed); // Màu đỏ khi tắt
}

void CheckPriceAlert(){
    double emaValue[];
    int handle = iMA(_Symbol, PERIOD_M1, PERIOD_EMA, 0, MODE_EMA, PRICE_CLOSE);;
    if(handle < 0) return;

    ArraySetAsSeries(emaValue, true);
    if(CopyBuffer(handle, 0, 0, ONE, emaValue) <= 0) return;

    double currentEma = emaValue[0];
    if(currentEma == 0) return;

    double currentHigh = iHigh(_Symbol, PERIOD_M1, 0);
    double currentLow = iLow(_Symbol, PERIOD_M1, 0);

    if(TradingTrend == SELL && currentHigh >= currentEma){
        Alert("Price crossed above EMA");
    } else if(TradingTrend == BUY && currentLow <= currentEma){
        Alert("Price crossed below EMA");
    }

    // Cập nhật trạng thái cảnh báo
    AlertEnabled = false;
    AlertButton.Description("ALERT PRICE EMA: OFF");
    AlertButton.Color(clrWhite);
    AlertButton.BackColor(clrRed); // Màu đỏ khi tắt
}