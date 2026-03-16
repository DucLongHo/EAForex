#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton OnOffButton;// Nút bật tắt EA

// Constant data
const int ZERO = 0, ONE = 1, TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, TEN = 10;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
input double TrailingStartProfit = 2.0;  // Mốc lợi nhuận trailing stop
input double RiskTrade = 15; // Rủi ro long trade (USD)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, ZERO) + PeriodSeconds(PERIOD_M1);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime && 
        currentCandleCloseTime - currentTime <= TWO){
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

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){

}

void OnDeinit(const int reason){
    EventKillTimer();
}

void RunningEA(){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    int copied = CopyRates(_Symbol, _Period, ZERO, TEN, rates);
    if(copied <= 0) return;
    
    TradeNosdCandle(rates);
}
void CloseAllPositions(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            double profit = PositionGetDouble(POSITION_PROFIT);
            double currentSL = PositionGetDouble(POSITION_SL);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);

            double pointsD = (TrailingStartProfit / (volume * tickValue)) * tickSize;
            double step = NormalizeDouble(pointsD, _Digits);

            // --- XỬ LÝ LỆNH BUY ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                if(profit >= TrailingStartProfit){
                    double newSL = NormalizeDouble(last_tick.bid - step, _Digits);
                        
                    if(currentSL < priceOpen || currentSL == 0){
                        Trade.PositionModify(ticket, priceOpen, 0);
                    } else if(newSL > currentSL + _Point){
                        Trade.PositionModify(ticket, newSL, 0);
                    }
                }
            }

            // --- XỬ LÝ LỆNH SELL ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                if(profit >= TrailingStartProfit){
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

void TradeNosdCandle(const MqlRates &rates[]){
    MqlRates candle = rates[ONE], secondCandle = rates[TWO], thirdCandle = rates[THREE];

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

            if(!Trade.Buy(lotSize, _Symbol, entry, candle.low)){
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

            if(!Trade.Sell(lotSize, _Symbol, entry, candle.high)){
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