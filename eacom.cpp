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
input double TrailingStartProfit = 5.0;  // Có lời 5 USD thì bắt đầu kích hoạt trailing stop
input double TakeProfitHedge = 5.0; // Mỗi khi lời 5 USD thì căn cắt lệnh cân
input int TrailingStepPips = 100;   // Khoảng cách duy trì
input int TakeProfitUSD = 1; // Mức lợi nhuận mục tiêu để đóng lệnh (đơn vị: USD)

int ProfitHedge = -20; // Mức chênh lệch lợi nhuận để thực hiện vào lệnh cân bằng (đơn vị: USD)
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
    if(GetTotalBuyProfit() + GetTotalSellProfit() >= TakeProfitUSD){
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
            if(!Trade.Sell(LotSize, _Symbol)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }

            if(!Trade.Buy(LotSize, _Symbol)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
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

    // Reset cờ nếu tài khoản dương trở lại hoặc hết lệnh (để chuẩn bị cho chu kỳ mới)
    if(PositionsTotal() == 0){
        ProfitHedge = -20;
    }

    if(totalProfit <= ProfitHedge){
        ProfitHedge -= 20; // Giảm thêm 5 USD cho lần tiếp theo nếu vẫn chưa cân bằng được
        ExecuteHedge(buyLots, sellLots);
    }

    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        
        if(PositionSelectByTicket(ticket)){
            if(PositionGetDouble(POSITION_VOLUME) > LotSize && PositionGetDouble(POSITION_PROFIT) >= TakeProfitHedge){
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                    if(buyLots - sellLots >= PositionGetDouble(POSITION_VOLUME)){
                        if(!Trade.PositionClose(ticket))
                            Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
                    }
                } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                    if(sellLots - buyLots >= PositionGetDouble(POSITION_VOLUME)){
                        if(!Trade.PositionClose(ticket))
                            Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
                    }
                }
            }    
        }
    }
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return; // Lấy giá Bid/Ask nhanh nhất

    for(int i = PositionsTotal() - 1; i >= 0; i--){
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)){
            double profit = PositionGetDouble(POSITION_PROFIT);
            double currentSL = PositionGetDouble(POSITION_SL);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);
            if(volume == LotSize){
                continue;
            }
            // Công thức: (Lợi nhuận mục tiêu / (Khối lượng * Giá trị 1 tick)) * Kích thước 1 tick
            double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
            double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
                
            double pointsFor5USD = (TrailingStartProfit / (volume * tickValue)) * tickSize;
            double step = NormalizeDouble(pointsFor5USD, _Digits);

            // --- XỬ LÝ LỆNH BUY ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                if(profit >= TrailingStartProfit){
                    // SL mới duy trì khoảng cách 'step' so với giá hiện tại
                    double newSL = NormalizeDouble(last_tick.bid - step, _Digits);
                        
                    // Điều kiện 1: Nếu chưa có SL hoặc SL đang dưới Entry -> Đưa về Entry
                    if(currentSL < priceOpen){
                        Trade.PositionModify(ticket, priceOpen, 0);
                    } else if(newSL > currentSL + _Point){
                        Trade.PositionModify(ticket, newSL, 0);
                    }
                }
            }

            // --- XỬ LÝ LỆNH SELL ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                if(profit >= TrailingStartProfit){
                    // SL mới duy trì khoảng cách 'step' so với giá hiện tại
                    double newSL = NormalizeDouble(last_tick.ask + step, _Digits);
                    
                    if(currentSL > priceOpen || currentSL == 0){
                        Trade.PositionModify(ticket, priceOpen, 0);
                    } else if(newSL < currentSL - _Point){
                        Trade.PositionModify(ticket, newSL, 0);
                    }
                }
            }
        }
    }
}