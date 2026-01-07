//+------------------------------------------------------------------+
//|                                         DailySafeManager.mq5     |
//|                                  Copyright 2024, AI Assistant    |
//+------------------------------------------------------------------+
#include <Trade\Trade.mqh>

CTrade Trade;

#property strict

//--- Input Parameters
input group "=== Quản trị rủi ro ==="
input double   InpMaxDailyLoss   = 100.0;    // Lỗ tối đa trong ngày ($)
input double   InpTargetProfit   = 60.0;     // Mục tiêu lợi nhuận ngày ($)
input double   InpCloseAtProfit  = 30.0;     // Mức dương bảo vệ lợi nhuận ($)

input group "=== Chế độ Auto TP sau mục tiêu ==="
input int      InpAutoTPPoints   = 50;       // Auto TP (50 points = 5 pips)
input bool     InpOnlyTPAfterTarget = true;  // Chỉ Auto TP sau khi đã đạt Target ngày

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
    // Tự động Reset khi sang ngày mới
    MqlDateTime dt;
    TimeCurrent(dt);
    if(dt.day != LastDay){
        ResetDailyStats();
        Print("--- Ngày mới bắt đầu. Đã cập nhật lại Balance mục tiêu ---");
    }

    double currentEquity = AccountInfoDouble(ACCOUNT_EQUITY);
    double dailyPL = currentEquity - StartDayBalance;

    // 1. KIỂM TRA LỖ NGÀY
    if(dailyPL <= -InpMaxDailyLoss){
        // Chỉ gửi thông báo một lần khi bắt đầu khóa
        if(!LockTrading) {
            CloseAllOrders();
            Print("Chạm giới hạn lỗ ngày! Đã khóa giao dịch.");
            LockTrading = true;
        } else if(PositionsTotal() > 0){
            // Nếu có lệnh mới mở thêm khi đang khóa, đóng ngay lập tức
            CloseAllOrders();
            Print("Khóa giao dịch: Đóng lệnh mở thêm.");
        }
    }

    // 2. KIỂM TRA MỤC TIÊU LỢI NHUẬN
    if(dailyPL >= InpTargetProfit){
        if(!TargetReachedToday) {
            TargetReachedToday = true;
            if(InpSendAlert) SendNotification("Chúc mừng! Đã đạt mục tiêu lợi nhuận ngày. Kích hoạt Auto TP.");
        }
    }

    // 3. ĐÓNG TẤT CẢ KHI VỀ MỨC DƯƠNG (Bảo vệ lợi nhuận)
    if(TargetReachedToday && dailyPL <= InpCloseAtProfit && dailyPL > 0 && PositionsTotal() > 0){
        Print("Lợi nhuận sụt giảm về mức dương an toàn. Chốt tất cả.");
        CloseAllOrders();
    }

    // 4. TỰ ĐỘNG ĐẶT TP (Chỉ kích hoạt sau khi đã đạt Target)
    if(!InpOnlyTPAfterTarget || (InpOnlyTPAfterTarget && TargetReachedToday)){
        ApplyAutoTP();
    }
}

//+------------------------------------------------------------------+
// Hàm Reset thông số theo ngày
void ResetDailyStats(){
    MqlDateTime dt;
    TimeCurrent(dt);

    LastDay = dt.day;
    StartDayBalance = AccountInfoDouble(ACCOUNT_BALANCE);
    LockTrading = false;
    TargetReachedToday = false;
}

//+------------------------------------------------------------------+
// Hàm quét và đặt TP cho các lệnh chưa có TP
void ApplyAutoTP(){
    for(int i = 0; i < PositionsTotal(); i++){
        ulong ticket = PositionGetTicket(i);
        if(ticket <= 0) continue;

        // Lấy thông tin chi tiết của vị thế
        double entry = PositionGetDouble(POSITION_PRICE_OPEN);
        double stopLoss = PositionGetDouble(POSITION_SL);
        string symbol = PositionGetString(POSITION_SYMBOL);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            if(type == POSITION_TYPE_BUY){
                double newTP = entry + InpAutoTPPoints * _Point;
                ModifyTakeProfit(ticket, stopLoss, newTP);
            } else if(type == POSITION_TYPE_SELL){
                double newTP = entry - InpAutoTPPoints * _Point;
                ModifyTakeProfit(ticket, stopLoss, newTP);
            }
        }
   }
}

//+------------------------------------------------------------------+
// Hàm đóng toàn bộ vị thế đang mở
void CloseAllOrders(){
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

// Hàm hỗ trợ để điều chỉnh Stop Loss
void ModifyTakeProfit(ulong ticket, double stopLoss, double newTP){
    stopLoss = NormalizeDouble(stopLoss, _Digits);
    if (!Trade.PositionModify(ticket, stopLoss, newTP)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
}