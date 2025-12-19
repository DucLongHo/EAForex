//+------------------------------------------------------------------+
//|                                                  SafeTraderEA.mq5 |
//|                        Copyright 2023, MetaQuotes Software Corp. |
//|                                             https://www.mql5.com |
//+------------------------------------------------------------------+
#property copyright "Copyright 2023, MetaQuotes Software Corp."
#property link      "https://www.mql5.com"
#property version   "1.00"

input double MaxDailyDrawdownPercent = 10.0; // Drawdown tối đa cho phép (%)

// Biến toàn cục
double dailyBalance;    // Balance đầu ngày
bool   isNewDay = true; // Cờ kiểm tra ngày mới

// Constant data
const int ONE = 1;

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);
    Print("EA SafeTrader khởi động - Kiểm soát drawdown hàng ngày");
    return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason){
    Print("EA SafeTrader dừng hoạt động");
}

//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick(){

}

void OnTimer(){
    // Kiểm tra ngày mới
    CheckNewDay();
    
    // Kiểm tra drawdown trước khi vào lệnh
    if(IsDrawdownExceeded()){
        Print("Tạm dừng giao dịch: Drawdown vượt quá ", MaxDailyDrawdownPercent, "% so với balance đầu ngày");
        CloseAllPositions();
    }
}
//+------------------------------------------------------------------+
//| Kiểm tra ngày mới và cập nhật balance đầu ngày                  |
//+------------------------------------------------------------------+
void CheckNewDay(){
    MqlDateTime currentTime;
    TimeCurrent(currentTime);
    
    static int lastDay = -1;
    
    if(currentTime.day != lastDay){
        // Ngày mới - cập nhật balance đầu ngày
        dailyBalance = AccountInfoDouble(ACCOUNT_BALANCE);
        lastDay = currentTime.day;
        isNewDay = true;
        
        Print("Ngày mới: ", TimeToString(TimeCurrent(), TIME_DATE), 
              " - Balance đầu ngày: ", dailyBalance);
    } else {
        isNewDay = false;
    }
}

//+------------------------------------------------------------------+
//| Kiểm tra xem drawdown có vượt quá giới hạn cho phép không       |
//+------------------------------------------------------------------+
bool IsDrawdownExceeded(){
    double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    double drawdown = dailyBalance - currentBalance;
    double drawdownPercent = (drawdown / dailyBalance) * 100.0;
    
    // In thông tin drawdown để debug
    if(isNewDay){
        Print("Drawdown hiện tại: ", DoubleToString(drawdownPercent, 2), "%");
        isNewDay = false;
    }
    
    return (drawdownPercent >= MaxDailyDrawdownPercent);
}

//+------------------------------------------------------------------+
//| Hàm tính toán drawdown hiện tại                                 |
//+------------------------------------------------------------------+
double GetCurrentDrawdownPercent(){
    double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    double drawdown = dailyBalance - currentBalance;
    return (drawdown / dailyBalance) * 100.0;
}

void CloseAllPositions(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        string symbol = PositionGetString(POSITION_SYMBOL);
        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            double currentProfit = PositionGetDouble(POSITION_PROFIT);
            // Đóng lệnh ngay mà không cần kiểm tra Magic Number
            if(Trade.PositionClose(ticket)){
                Print("Closed position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}