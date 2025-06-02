#include <Trade\Trade.mqh>
// Constant data
const int ZERO = 0;
const int ONE = 1;
const int FIVE = 5;
// Biến toàn cục
CTrade Trade;
// Function
void managePositions();
void modifyStopLoss(ulong ticket, double newStopLoss);
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
//---
   
//---
   return(INIT_SUCCEEDED);
}
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason){
//---
   
}

void OnTick(){
   managePositions();
}

void managePositions(){
    // Lặp qua tất cả các vị thế đang mở
    for(int index = PositionsTotal() - ONE; index >= ZERO; index--){
        // Lấy thông tin vị thế
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        // Lấy thông tin chi tiết của vị thế
        double entry = PositionGetDouble(POSITION_PRICE_OPEN);
        double stopLoss = PositionGetDouble(POSITION_SL);
        double currentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket)){
            double stopLossDistance = MathAbs(entry - stopLoss);
            double currentProfit = MathAbs(currentPrice - entry);

            if(type == POSITION_TYPE_BUY && currentPrice > entry){
                if(currentProfit >= stopLossDistance && entry > stopLoss)
                    modifyStopLoss(ticket, entry + FIVE * _Point);
            }else if(type == POSITION_TYPE_SELL && currentPrice < entry){
                if(currentProfit >= stopLossDistance && entry < stopLoss)
                    modifyStopLoss(ticket, entry - FIVE * _Point);
            }
        }
    }
}

// Hàm hỗ trợ để điều chỉnh Stop Loss
void modifyStopLoss(ulong ticket, double newStopLoss){
   newStopLoss = NormalizeDouble(newStopLoss, _Digits);
   if (!Trade.PositionModify(ticket, newStopLoss, ZERO)){
      Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
   }
}