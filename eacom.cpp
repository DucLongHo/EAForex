#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton OnOffButton;// Nút bật tắt EA

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool OnOffEnabled = false; // Biến kiểm soát bật tắt EA

// Input parameters
input double LotSize = 0.01; // Khối lượng từng lệnh
input double TrailingStartProfit = 5.0;  // Mốc lợi nhuận lệnh cân trailing stop
input double TakeProfitHedge = 10.0; // Mốc lợi nhuận để đóng lệnh cân bằng
input int TrailingStepPips = 200;   // Khoảng cách duy trì
input double TakeProfitUSD = 1; // Mức lợi nhuận đóng lệnh (đơn vị: USD)
input double DrawdownLimitUSD = -50; // Mức thua lỗ tối đa (đơn vị: USD)
input double TakeProfitSLEntry = 5; // Mức lợi nhuận để BE (đơn vị: USD)
input double TakeProfitHedgeEntryFirst = -2; // Mức lợi nhuận để hedge cho lệnh đầu tiên (đơn vị: USD)

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    if(!CreateButton(OnOffButton, "OnOffButton", "OFF", clrRed, CalculateButtonY() - 30))
        return(INIT_FAILED);
    
    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, ONE) + PeriodSeconds(PERIOD_M1);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime <= currentTime){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;

        TradeCom();
    }
    
    if(isRunningEa) isRunningEa = false;
}

void OnTick(){
    if(GetTotalBuyProfit() + GetTotalSellProfit() >= TakeProfitUSD || GetTotalBuyProfit() + GetTotalSellProfit() <= DrawdownLimitUSD){
        CloseAllPositions();
    }

    if(PositionsTotal() > 0){
        TrailingByProfitUSD();
    }

    HedgePositions();
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
    OnOffButton.Delete();
    EventKillTimer();
}

void TradeCom(){
    if(PositionsTotal() == 0){
        if(OnOffEnabled){
            double openPrice = iOpen(_Symbol, _Period, ONE);
            double closePrice = iClose(_Symbol, _Period, ONE);
            
            if(closePrice > openPrice) {
                if(!Trade.Buy(LotSize, _Symbol)){
                    Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
            } else if(closePrice < openPrice) {
                if(!Trade.Sell(LotSize, _Symbol)){
                    Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
            }
        }
    } else {
        if(GetTotalBuyProfit() >= GetTotalSellProfit()){
            if(!Trade.Buy(LotSize, _Symbol)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        } else { 
            if(!Trade.Sell(LotSize, _Symbol)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
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

double GetTotalBuyProfit() {
    double totalProfit = 0;
    
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            // Kiểm tra nếu là lệnh BUY
            if((ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) {
                totalProfit += PositionGetDouble(POSITION_PROFIT);
            }
        }
    }
    return totalProfit;
}

double GetTotalSellProfit() {
    double totalProfit = 0;
    
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            // Kiểm tra nếu là lệnh SELL
            if((ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) {
                totalProfit += PositionGetDouble(POSITION_PROFIT);
            }
        }
    }
    return totalProfit;
}

void CloseAllPositions(){
    Trade.SetAsyncMode(true);

    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }

    Trade.SetAsyncMode(false);
}

// Hàm phụ trợ để thực hiện vào lệnh cân bằng khối lượng
void ExecuteHedge(double bLots, double sLots) {
    double diff = (bLots - sLots);
    
    if(NormalizeDouble(diff, 2) > 0) {
        if(!Trade.Sell(MathAbs(diff), _Symbol)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    } 
    else if(NormalizeDouble(diff, 2) < 0) {
        if(!Trade.Buy(MathAbs(diff), _Symbol)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    }
}

void HedgePositions() {
    double totalProfit = 0;
    double buyLots = 0;
    double sellLots = 0;

    // 1. Tính toán trạng thái hiện tại
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)){
            totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) 
                buyLots += PositionGetDouble(POSITION_VOLUME);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) 
                sellLots += PositionGetDouble(POSITION_VOLUME);
        }
    }

    // Cân lệnh lẻ
    if(NormalizeDouble(MathAbs(sellLots - buyLots), TWO) == LotSize){
        ulong ticket = PositionGetTicket(PositionsTotal() - ONE);

        if(PositionSelectByTicket(ticket)){
            if(PositionGetDouble(POSITION_PROFIT) <= -3 && totalProfit <= 0){ //Cần sửa check cùng chiều chứ
                ExecuteHedge(buyLots, sellLots);
            }
            
            if(PositionsTotal() == ONE && PositionGetDouble(POSITION_PROFIT) <= TakeProfitHedgeEntryFirst){
                ExecuteHedge(buyLots, sellLots);
            }
        }
    }
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return; // Lấy giá Bid/Ask nhanh nhất

    // Công thức: (Lợi nhuận mục tiêu / (Khối lượng * Giá trị 1 tick)) * Kích thước 1 tick
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    
    for(int i = PositionsTotal() - 1; i >= 0; i--){
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)){
            double profit = PositionGetDouble(POSITION_PROFIT);
            double currentSL = PositionGetDouble(POSITION_SL);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);
            double entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

            if(profit >= TakeProfitSLEntry && PositionsTotal() > 25){
                if(type == POSITION_TYPE_BUY){
                    double newSL = entryPrice + 500 * _Point; // Cách Entry 50 pips
                    Trade.PositionModify(ticket, newSL, 0);
                } else if(type == POSITION_TYPE_SELL){
                    double newSL = entryPrice - 500 * _Point; // Cách Entry 50 pips
                    Trade.PositionModify(ticket, newSL, 0);
                }
            }
        }
    }        
}