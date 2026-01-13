//+------------------------------------------------------------------+
//|                                         DailySafeManager.mq5     |
//|                                  Copyright 2024, AI Assistant    |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>

CTrade Trade;

#property strict

//--- Input Parameters
input group "=== Quản trị rủi ro ==="
input double   InpMaxDailyLoss   = 100.0;    // Lỗ tối đa trong ngày ($) - Tính theo Equity
input double   InpTargetProfit   = 60.0;     // Mục tiêu lợi nhuận ngày ($) - Tính theo Balance
input double   InpCloseAtProfit  = 30.0;     // Mức dương bảo vệ lợi nhuận ($) - Tính theo Equity

input group "=== Chế độ Auto TP sau mục tiêu ==="
input int      InpAutoTPPoints   = 50;       // Auto TP (50 points = 5 pips)
input bool     InpOnlyTPAfterTarget = true;  // Chỉ Auto TP sau khi đã đạt Target ngày (Balance)

input group "=== Thông báo ==="
input bool     InpSendAlert      = true;     // Gửi thông báo về điện thoại

//--- Biến toàn cục
double StartDayBalance = 0;
bool   LockTrading = false;
bool   TargetReachedToday = false;
int    LastDay = -1; // Lưu ngày gần nhất để kiểm tra chuyển giao ngày mới

//+------------------------------------------------------------------+
int OnInit(){
   ResetDailyStats();
   return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
void OnTick()
{
    // 1. Tự động Reset khi sang ngày mới
    MqlDateTime dt;
    TimeCurrent(dt);
    if(dt.day != LastDay){
        ResetDailyStats();
        Print("--- Ngày mới: Cập nhật Balance mốc: ", StartDayBalance);
    }

    double currentEquity  = AccountInfoDouble(ACCOUNT_EQUITY);
    double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    
    // Lợi nhuận thực tế (lệnh đã đóng)
    double closedProfit = currentBalance - StartDayBalance;
    // Lợi nhuận tạm thời (lệnh đang chạy)
    double floatingPL   = currentEquity - StartDayBalance;

    // 2. KIỂM TRA LỖ NGÀY (Dùng Equity để cắt lỗ kịp thời)
    if(floatingPL <= -InpMaxDailyLoss){
        if(!LockTrading) {
            CloseAllOrders();
            Print("Chạm giới hạn lỗ ngày! Đã khóa giao dịch.");
            LockTrading = true;
            if(InpSendAlert) SendNotification("EA: Đã chạm Max Loss ngày. Khóa giao dịch.");
        } else if(PositionsTotal() > 0){
            CloseAllOrders();
        }
    }

    // 3. KIỂM TRA MỤC TIÊU LỢI NHUẬN (Dùng Balance theo yêu cầu của bạn)
    if(closedProfit >= InpTargetProfit){
        if(!TargetReachedToday) {
            TargetReachedToday = true;
            Print("Đã đạt mục tiêu Balance: +", closedProfit);
            if(InpSendAlert) SendNotification("Chúc mừng! Balance đạt mục tiêu. Kích hoạt Auto TP.");
        }
    }

    // 4. ĐÓNG TẤT CẢ KHI VỀ MỨC DƯƠNG (Bảo vệ lợi nhuận dựa trên Equity)
    // Nếu đã đạt Target Balance nhưng lệnh đang chạy làm tổng Equity sụt về mức InpCloseAtProfit
    if(TargetReachedToday && floatingPL <= InpCloseAtProfit && PositionsTotal() > 0){
        Print("Equity sụt giảm về mức bảo vệ (", InpCloseAtProfit, "). Chốt sạch.");
        CloseAllOrders();
    }

    // 5. TỰ ĐỘNG ĐẶT TP (Chỉ kích hoạt khi TargetReachedToday = true)
    if(!InpOnlyTPAfterTarget || (InpOnlyTPAfterTarget && TargetReachedToday)){
        ApplyAutoTP();
    }
}

//+------------------------------------------------------------------+
void ResetDailyStats(){
    MqlDateTime dt;
    TimeCurrent(dt);
    LastDay = dt.day;
    StartDayBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    LockTrading = false;
    TargetReachedToday = false;
}

//+------------------------------------------------------------------+
void ApplyAutoTP(){
    for(int i = 0; i < PositionsTotal(); i++){
        ulong ticket = PositionGetTicket(i);
        if(ticket <= 0) continue;

        if(PositionSelectByTicket(ticket)){
            double tp = PositionGetDouble(POSITION_TP);
            // Chỉ đặt nếu lệnh chưa có TP
            if(tp == 0){
                double entry = PositionGetDouble(POSITION_PRICE_OPEN);
                double sl = PositionGetDouble(POSITION_SL);
                string symbol = PositionGetString(POSITION_SYMBOL);
                int digits = (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS);
                double point = SymbolInfoDouble(symbol, SYMBOL_POINT);
                ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

                double newTP = 0;
                if(type == POSITION_TYPE_BUY) 
                    newTP = NormalizeDouble(entry + InpAutoTPPoints * point, digits);
                else 
                    newTP = NormalizeDouble(entry - InpAutoTPPoints * point, digits);

                if(!Trade.PositionModify(ticket, NormalizeDouble(sl, digits), newTP)){
                    Print("Lỗi đặt Auto TP cho #", ticket, ": ", GetLastError());
                } else {
                    Print("Đã gán Auto TP cho lệnh #", ticket);
                }
            }
        }
    }
}

//+------------------------------------------------------------------+
void CloseAllOrders(){
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        
        if(ticket > 0){
            Trade.SetAsyncMode(true);
            Trade.PositionClose(ticket);
        } 
    }
    Trade.SetAsyncMode(false);
}