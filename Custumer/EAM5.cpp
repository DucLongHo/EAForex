#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; 
double BreakevenLock = 100;
datetime LastStopDate = 0; // Ngày dừng bot do drawdown

sinput string separator1 = "------------------------------------------"; // === THÔNG SỐ VỊ THẾ ===
input double LotSize = 0.01; // Khối lượng giao dịch cố định
input double StopLossPips = 3000; // Khoảng cách Stop Loss (points)
input double TakeProfitPips = 6000; // Khoảng cách Take Profit (points)
sinput string separator2 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double TrailingStepPips = 2000;  // Bước giá để tiếp tục dời SL (points)
input double MaxDrawdownAccount = 500; // Mức giảm tối đa của tài khoản (USD)
sinput string separator3 = "------------------------------------------"; // === QUẢN LÝ VẬN HÀNH ===
input int TimeCheckCandleClose = 3; // Thời gian (giây) trước khi đóng nến để kiểm tra và mở lệnh mới 

int OnInit(){
    EventSetTimer(1);
    CandleCloseTime = 0;
    return (INIT_SUCCEEDED);
}

void OnDeinit(const int reason){
    EventKillTimer();
}

void OnTimer(){
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);

    if(currentCandleCloseTime != CandleCloseTime && (currentCandleCloseTime - currentTime <= TimeCheckCandleClose)){
        CandleCloseTime = currentCandleCloseTime;
        MqlRates rates[];
        ArraySetAsSeries(rates, true);
        if(CopyRates(_Symbol, PERIOD_M5, 0, 2, rates) <= 0) return; 

        MqlRates candle = rates[0];

        if(PositionsTotal() == 0){
            if(LastStopDate == 0 || LastStopDate < iTime(_Symbol, PERIOD_D1, 0)){
                RunningEA(candle);
            } else {
                Print("Bot is paused due to drawdown. Will resume tomorrow.");
            }
        }
    }
}

void OnTick(){
    if(PositionsTotal() > 0){
        ManagePositions();
        CheckDrawdown();
    }
}

void RunningEA(MqlRates &candle){
    
    double iClosePrice = candle.close;
    double iOpenPrice = candle.open;
    
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
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

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

void CheckDrawdown(){
    double equity = AccountInfoDouble(ACCOUNT_EQUITY);
    double balance = AccountInfoDouble(ACCOUNT_BALANCE);
    double drawdown = balance - equity;

    if(drawdown >= MaxDrawdownAccount){        
        CloseAllOrders(); 
        
        // Lưu lại ngày hôm nay (dưới dạng 00:00 của ngày hiện tại)
        LastStopDate = iTime(_Symbol, PERIOD_D1, 0); 
    }
}

void CloseAllOrders(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            if(Trade.PositionClose(ticket)){
                Print("Closed position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}