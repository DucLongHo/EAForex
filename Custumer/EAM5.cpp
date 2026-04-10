#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; 
double BreakevenLock = 100;

sinput string separator1 = "------------------------------------------"; // === THÔNG SỐ VỊ THẾ ===
input double LotSize = 0.01; // Khối lượng giao dịch cố định
input double StopLossPips = 3000; // Khoảng cách Stop Loss (points)
input double TakeProfitPips = 1500; // Khoảng cách Take Profit (points)
sinput string separator2 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double TrailingStepPips = 2000;  // Bước giá để tiếp tục dời SL (points)

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
    double iClosePrice = iClose(_Symbol, PERIOD_M5, 0);
    double iOpenPrice = iOpen(_Symbol, PERIOD_M5, 0);
    
    if(iClosePrice > iOpenPrice){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double sl = NormalizeDouble(entry - StopLossPips * _Point, _Digits);
        double tp = NormalizeDouble(entry + TakeProfitPips * _Point, _Digits);
        
        if(!Trade.Buy(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    } else if(iClosePrice < iOpenPrice){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double sl = NormalizeDouble(entry + StopLossPips * _Point, _Digits);
        double tp = NormalizeDouble(entry - TakeProfitPips * _Point, _Digits);

        if(!Trade.Sell(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

void ManagePositions(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return; // Cần gọi dòng này để lấy bid/ask hiện tại

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            // Chỉ quản lý lệnh của symbol hiện tại
            if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue; 

            double currentSL = PositionGetDouble(POSITION_SL);
            double currentTP = PositionGetDouble(POSITION_TP);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                double profitPoints = (last_tick.bid - priceOpen) / _Point;
                
                if(profitPoints >= TrailingStepPips){
                    if(currentSL < priceOpen){
                        double newSL = NormalizeDouble(priceOpen + BreakevenLock * _Point, _Digits);
                        Trade.PositionModify(ticket, newSL, currentTP);
                    } else {
                        double newSL = NormalizeDouble(last_tick.bid - TrailingStepPips * _Point, _Digits);
                        
                        if(newSL >= currentSL + TrailingStepPips * _Point){
                            Trade.PositionModify(ticket, newSL, currentTP);
                        }
                    }
                }
            } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                double profitPoints = (priceOpen - last_tick.ask) / _Point;
                
                if(profitPoints >= TrailingStepPips){
                    if(currentSL > priceOpen || currentSL == 0.0){
                        double newSL = NormalizeDouble(priceOpen - BreakevenLock * _Point, _Digits);
                        Trade.PositionModify(ticket, newSL, currentTP);
                    } else {
                        double newSL = NormalizeDouble(last_tick.ask + TrailingStepPips * _Point, _Digits);
                        
                        if(currentSL >= newSL + TrailingStepPips * _Point){ 
                            Trade.PositionModify(ticket, newSL, currentTP);
                        }
                    }
                }
            }
        }
    }
}