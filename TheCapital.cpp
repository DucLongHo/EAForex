#include <Trade\Trade.mqh>

CTrade Trade;

// Constant data

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
input double RiskTrade = 15; // Rủi ro long trade (USD)
input double ProfitBreakEvent = 5; // Lợi nhuận để BE (USD)
input double TrailingStartPoint = 500;  // Mốc lợi nhuận trailing stop

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
    if(PositionsTotal() > 0){
        TrailingByProfitUSD();
    }
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
    
    Trading(rates);
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

void Trading(const MqlRates &rates[]){
    MqlRates candle = rates[0], secondCandle = rates[1], thirdCandle = rates[2];
    
    double upperShadow = candle.high - MathMax(candle.open, candle.close);
    double lowerShadow = MathMin(candle.open, candle.close) - candle.low;

    if(candle.close > candle.open){
        if(secondCandle.close < secondCandle.open  
            && candle.close > secondCandle.open
            && candle.open >= secondCandle.close
            && candle.high > secondCandle.high
            && candle.low < secondCandle.low
            && candle.low < thirdCandle.low
            && candle.close - candle.open > upperShadow
        ){
            BUY(candle, true); // NoSD
        }

        if(upperShadow <= 0.15 * (candle.close - candle.open)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            BUY(candle); // Mazubozu
        }
    } else if(candle.close < candle.open){
        if(secondCandle.close > secondCandle.open 
            && candle.close < secondCandle.open
            && candle.open <= secondCandle.close 
            && candle.low < secondCandle.low
            && candle.high > secondCandle.high
            && candle.high > thirdCandle.high
            && candle.open - candle.close > lowerShadow
        ){
            SELL(candle, true); // NoSD
        }

        if(lowerShadow <= 0.15 * (candle.open - candle.close)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            SELL(candle); // Mazubozu
        }
    }
}

double GetLotSize(double stopLossDistance, MqlRates &candle){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    double stopLossPips = stopLossDistance / _Point;

    double pipValue = tickValue / (tickSize / _Point);
   
    double lotSize = RiskTrade / (stopLossPips * pipValue);
   
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
   
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
            double currentSL = PositionGetDouble(POSITION_SL);
            double profit = PositionGetDouble(POSITION_PROFIT);

            if(profit >= ProfitBreakEvent && takeProfit > 0){
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY && currentSL < entryPrice){
                    double newStopLoss = entryPrice + 100 * _Point; // Đặt stop loss về giá entry
                    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
                        Print("Error modifying Buy Order #", ticket, " - Error: ", Trade.ResultComment());
                    }
                } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL && currentSL > entryPrice){
                    double newStopLoss = entryPrice - 100 * _Point; // Đặt stop loss về giá entry
                    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
                        Print("Error modifying Sell Order #", ticket, " - Error: ", Trade.ResultComment());
                    }
                }
            }
        }
    }
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            double currentSL = PositionGetDouble(POSITION_SL);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double takeProfit = PositionGetDouble(POSITION_TP);
            double volume = PositionGetDouble(POSITION_VOLUME);
        

            if(takeProfit < 0.00001){
                // --- XỬ LÝ LỆNH BUY ---
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                    if(last_tick.bid - priceOpen >= TrailingStartPoint * _Point){
                        double newSL = NormalizeDouble(last_tick.bid - TrailingStartPoint * _Point, _Digits);
                            
                        if(currentSL < priceOpen || currentSL == 0){
                            Trade.PositionModify(ticket, priceOpen, 0);
                        } else if(newSL > currentSL + _Point){
                            Trade.PositionModify(ticket, newSL, 0);
                        }
                    }
                }

                // --- XỬ LÝ LỆNH SELL ---
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                    if(priceOpen - last_tick.ask >= TrailingStartPoint * _Point){
                        double newSL = NormalizeDouble(last_tick.ask + TrailingStartPoint * _Point, _Digits);
                        
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
    
}

void BUY(MqlRates &candle, bool hasTakeProfit = false){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double sl = candle.low - 200 * _Point;
    double lotSize = GetLotSize(MathAbs(entry - sl), MqlRates());

    if(hasTakeProfit){
        double takeProfit = entry + 2 * MathAbs(entry - sl);
        if(!Trade.Buy(lotSize, _Symbol, entry, sl, takeProfit)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    } else {
        if(!Trade.Buy(lotSize, _Symbol, entry, sl)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    }
}

void SELL(MqlRates &candle, bool hasTakeProfit = false){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double sl = candle.high + 200 * _Point;
    double lotSize = GetLotSize(MathAbs(entry - sl), MqlRates());

    if(hasTakeProfit){
        double takeProfit = entry + 2 * MathAbs(entry - sl);
        if(!Trade.Sell(lotSize, _Symbol, entry, sl, takeProfit)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    } else {
        if(!Trade.Sell(lotSize, _Symbol, entry, sl)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}