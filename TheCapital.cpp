#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
input double RiskTrade = 50; // Rủi ro long trade (USD)
input double MinDistanceSL = 1000; // Stop loss tối thiểu (Points)
input int TimeLeniency = 2; // Độ trễ cho việc kiểm tra thời gian đóng nến (giây)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(1);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M5 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M5, 0) + PeriodSeconds(PERIOD_M5);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime && 
        currentCandleCloseTime - currentTime <= 2){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;

        RunningEA();
    }
    
    if(isRunningEa) isRunningEa = false;
}

void OnTick(){
    if(PositionsTotal() > 0){
        TrailingByProfitUSD();
    }
}

void OnDeinit(const int reason){
    EventKillTimer();
}

void RunningEA(){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    int copied = CopyRates(_Symbol, PERIOD_M5, 0, 10, rates);
    if(copied <= 0) return;
    
    Trading(rates);
}

void Trading(const MqlRates &rates[]){
    MqlRates candle = rates[0], secondCandle = rates[1], thirdCandle = rates[2];
    
    double upperShadow = candle.high - MathMax(candle.open, candle.close);
    double lowerShadow = MathMin(candle.open, candle.close) - candle.low;

    if(candle.close > candle.open){
        if(secondCandle.close < secondCandle.open  
            && candle.close > secondCandle.open
            && MathAbs(candle.open - secondCandle.close) >= 100 * _Point
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
            BUY(candle, true); // Mazubozu
        }
    } else if(candle.close < candle.open){
        if(secondCandle.close > secondCandle.open 
            && candle.close < secondCandle.open
            && MathAbs(candle.open - secondCandle.close) >= 100 * _Point 
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
            SELL(candle, true); // Mazubozu
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

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    double trailingStep = 10 * _Point; 

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            double currentSL = PositionGetDouble(POSITION_SL);
            double currentTP = PositionGetDouble(POSITION_TP);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double newSL = 0;
            double distanceFromOpen = MathAbs(currentTP - priceOpen)/1.5;

            // --- XỬ LÝ LỆNH BUY ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                newSL = NormalizeDouble(last_tick.bid - distanceFromOpen, _Digits);

                if(newSL >= currentSL + trailingStep){
                    Trade.PositionModify(ticket, newSL, currentTP);
                }
            }

            // --- XỬ LÝ LỆNH SELL ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                newSL = NormalizeDouble(last_tick.ask + distanceFromOpen, _Digits);
                
                if(newSL <= currentSL - trailingStep) {
                    Trade.PositionModify(ticket, newSL, currentTP);
                }
            }
        }
    }
}

void BUY(MqlRates &candle, bool hasTakeProfit = false){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double sl = candle.low - 200 * _Point;
    double lotSize = GetLotSize(MathAbs(entry - sl), MqlRates());
    
    if(!isOpenOrder(entry, sl))
        return;
    
    if(CountPositions("BUY") > 1 || checkEmaConditions("BUY", entry)){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2.0) * lotStep;
    }

    if(hasTakeProfit){
        double takeProfit = entry + 1.5 * MathAbs(entry - sl);
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
    
    if(!isOpenOrder(entry, sl))
        return;
    
    if(CountPositions("SELL") > 1 || checkEmaConditions("SELL", entry)){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2.0) * lotStep;
    }

    if(hasTakeProfit){
        double takeProfit = entry - 1.5 * MathAbs(entry - sl);
        if(!Trade.Sell(lotSize, _Symbol, entry, sl, takeProfit)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    } else {
        if(!Trade.Sell(lotSize, _Symbol, entry, sl)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

bool isOpenOrder(double entry, double sl){
    if(MathAbs(entry - sl) < MinDistanceSL * _Point) return false;
    return true;
}

int CountPositions(string type) {
    int count = 0;
   
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            if(type == "BUY" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                count++;
            } else if(type == "SELL" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                count++;
            }
        }
    }
    return count;
}

bool checkEmaConditions(string trend, double price){
    int emaHandle = iMA(_Symbol, PERIOD_M5, 25, 0, MODE_EMA, PRICE_CLOSE);
    if(emaHandle < 0) return false;
    
    double ema[];
    ArraySetAsSeries(ema, true);
    if(CopyBuffer(emaHandle, 0, 1, 0, ema) <= 0) return false;

    if(trend == "BUY" && price < ema[0]) return true;
    if(trend == "SELL" && price > ema[0]) return true;

    return false;
}