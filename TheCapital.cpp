#include <Trade\Trade.mqh>

CTrade Trade;

// Constant data

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
input double RiskTrade = 15; // Rủi ro long trade (USD)
input double ProfitBreakEvent = 5; // Lợi nhuận để BE (USD)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(1);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, 0) + PeriodSeconds(PERIOD_M1);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime && 
        currentCandleCloseTime - currentTime <= 2){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;

        RunningEA();
    }
    
    if(isRunningEa) isRunningEa = false;

    ManageBreakEven();
}

void OnTick(){

}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){

}

void OnDeinit(const int reason){
    EventKillTimer();
}

void RunningEA(){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    int copied = CopyRates(_Symbol, _Period, 0, 10, rates);
    if(copied <= 0) return;
    
    TradeNosdCandle(rates);
}
void CloseAllPositions(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}

void TradeNosdCandle(const MqlRates &rates[]){
    MqlRates candle = rates[0], secondCandle = rates[1], thirdCandle = rates[2];

    if(candle.close > candle.open){
        if(secondCandle.close < secondCandle.open  
            && candle.close > secondCandle.open
            && candle.high > secondCandle.high
            && candle.low < secondCandle.low
            && candle.low < thirdCandle.low
        ){
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
            double stopLossDistance = MathAbs(entry - candle.low);
            double lotSize = GetLotSize(stopLossDistance, candle);
            double takeProfit = entry + 2 * stopLossDistance;

            if(!Trade.Buy(lotSize, _Symbol, entry, candle.low, takeProfit)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        }
    } else if(candle.close < candle.open){
        if(secondCandle.close > secondCandle.open 
            && candle.close < secondCandle.open
            && candle.low < secondCandle.low
            && candle.high > secondCandle.high
            && candle.high > thirdCandle.high
        ){
            double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
            double stopLossDistance = MathAbs(entry - candle.high);
            double lotSize = GetLotSize(stopLossDistance, candle);
            double takeProfit = entry - 2 * stopLossDistance;

            if(!Trade.Sell(lotSize, _Symbol, entry, candle.high, takeProfit)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
        }
    }
}

double GetLotSize(double stopLossDistance, MqlRates &candle){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    // Tính số pips tương ứng với stopLossDistance
    double stopLossPips = stopLossDistance / _Point;

    // Tính giá trị pip
    double pipValue = tickValue / (tickSize / _Point);
   
    // Tính toán kích thước lô
    double lotSize = RiskTrade / (stopLossPips * pipValue);
   
    // Đảm bảo rằng kích thước lô nằm trong phạm vi cho phép của sàn giao dịch
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
   
    // Làm tròn kích thước lô đến số thập phân phù hợp
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lotSize = MathFloor(lotSize / lotStep) * lotStep;

    return lotSize;
}

void ManageBreakEven(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            double entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            double takeProfit = PositionGetDouble(POSITION_TP);
            double profit = PositionGetDouble(POSITION_PROFIT);

            if(profit >= ProfitBreakEvent){
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                    double newStopLoss = entryPrice + 100 * _Point; // Đặt stop loss về giá entry
                    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
                        Print("Error modifying Buy Order #", ticket, " - Error: ", Trade.ResultComment());
                    }
                } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                    double newStopLoss = entryPrice - 100 * _Point; // Đặt stop loss về giá entry
                    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
                        Print("Error modifying Sell Order #", ticket, " - Error: ", Trade.ResultComment());
                    }
                }
            }
        }
    }
}