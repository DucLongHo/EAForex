#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; 

sinput string separator1 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double LotSize = 0.01; // Khối lượng giao dịch cố định
input double StopLossPips = 3000; // Khoảng cách Stop Loss (pips)
input double TakeProfitPips = 1500; // Khoảng cách Take Profit (pips)
input double TrailingStartPips = 2000; // Khoảng cách bắt đầu trailing (pips)

int OnInit(){
    EventSetTimer(1);
    
    return (INIT_SUCCEEDED);
}

void OnDeinit(const int reason){
    EventKillTimer();
}

void OnTimer(){
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M5, 0) + PeriodSeconds(PERIOD_M5);

    if(currentCandleCloseTime != CandleCloseTime && (currentCandleCloseTime - currentTime <= 2)){
        CandleCloseTime = currentCandleCloseTime;
        if(PositionsTotal() == 0){
            RunningEA();
        }
    }
}

void OnTick(){
    if(PositionsTotal() > 0){
        ManagePositions();
    }
}

void RunningEA(){
    double iClose = iClose(_Symbol, PERIOD_M5, 0);
    double iOpen = iOpen(_Symbol, PERIOD_M5, 0);
    
    if(iClose > iOpen){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double sl = entry - StopLossPips * _Point;
        double tp = entry + TakeProfitPips * _Point;
        
        if(!Trade.Buy(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    } else if(iClose < iOpen){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double sl = entry + StopLossPips * _Point;
        double tp = entry - TakeProfitPips * _Point;

        if(!Trade.Sell(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

void ManagePositions(){
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){

            double currentSL = PositionGetDouble(POSITION_SL);
            double currentTP = PositionGetDouble(POSITION_TP);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                double profit = last_tick.bid - priceOpen;
                
                if(currentTP > 0 && profit > TrailingStartPips * _Point){
                    double newSL = priceOpen + 50 * _Point;;

                    Trade.PositionModify(ticket, newSL, currentTP);
                }
            } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                double profit = priceOpen - last_tick.ask;
                
                if(currentTP > 0 && profit > TrailingStartPips * _Point){
                    double newSL = priceOpen - 50 * _Point;
                    if(newSL <= currentSL - trailingStep && newSL > last_tick.ask){
                        Trade.PositionModify(ticket, newSL, currentTP);
                    }
                }
            }
        }
    }
}