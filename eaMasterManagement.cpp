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
input double   InpCloseAtProfit  = 30.0;     // Mức dương để đóng tất cả ($)

input group "=== Chế độ Auto TP sau mục tiêu ==="
input int      InpAutoTPPoints   = 50;       // Auto TP (50 points = 5 pips)
input bool     InpOnlyTPAfterTarget = true;  // Chỉ Auto TP sau khi đã đạt Target ngày

input group "=== Thông báo ==="
input bool     InpSendAlert      = true;     // Gửi thông báo về điện thoại

//--- Biến toàn cục
double StartDayBalance = 0;
bool   LockTrading = false;
bool   TargetReachedToday = false;

//+------------------------------------------------------------------+
int OnInit()
{
   StartDayBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   LockTrading = false;
   TargetReachedToday = false;
   return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
void OnTick()
{
   double currentEquity = AccountInfoDouble(ACCOUNT_EQUITY);
   double currentBalance = AccountInfoDouble(ACCOUNT_BALANCE);
   double dailyPL = currentEquity - StartDayBalance;

   // 1. KIỂM TRA LỖ NGÀY
   if(dailyPL <= -InpMaxDailyLoss)
   {
      CloseAllOrders("Chạm giới hạn lỗ ngày! Đã khóa giao dịch.");
      LockTrading = true;
   }

   // 2. KIỂM TRA MỤC TIÊU LỢI NHUẬN
   if(dailyPL >= InpTargetProfit)
   {
      TargetReachedToday = true; // Đánh dấu đã đạt mục tiêu trong ngày
   }

   // 3. ĐÓNG TẤT CẢ KHI VỀ MỨC DƯƠNG (Ví dụ đạt 60u nhưng rớt về 30u thì chốt sạch)
   if(TargetReachedToday && dailyPL <= InpCloseAtProfit && dailyPL > 0 && PositionsTotal() > 0)
   {
      CloseAllOrders("Lợi nhuận sụt giảm về mức an toàn. Chốt tất cả.");
   }

   // 4. KHÓA GIAO DỊCH NẾU ĐÃ CHẠM MAX LOSS
   if(LockTrading && PositionsTotal() > 0)
   {
      CloseAllOrders("Lệnh bị đóng ngay lập tức do đã quá giới hạn lỗ.");
   }

   // 5. TỰ ĐỘNG ĐẶT TP (Chỉ kích hoạt sau khi đã đạt Target hoặc nếu cấu hình luôn bật)
   if(!InpOnlyTPAfterTarget || (InpOnlyTPAfterTarget && TargetReachedToday))
   {
      ApplyAutoTP();
   }
}

//+------------------------------------------------------------------+
// Hàm quét và đặt TP cho các lệnh chưa có TP
void ApplyAutoTP()
{
   for(int i = 0; i < PositionsTotal(); i++)
   {
      ulong ticket = PositionGetTicket(i);
      if(PositionSelectByTicket(ticket))
      {
         double tp = PositionGetDouble(POSITION_TP);
         
         // Nếu lệnh chưa có TP, tiến hành đặt TP tự động
         if(tp == 0) 
         {
            double entryPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            string symbol = PositionGetString(POSITION_SYMBOL);
            double point = SymbolInfoDouble(symbol, SYMBOL_POINT);
            double newTP = 0;

            if(type == POSITION_TYPE_BUY)
               newTP = entryPrice + InpAutoTPPoints * point;
            else if(type == POSITION_TYPE_SELL)
               newTP = entryPrice - InpAutoTPPoints * point;

            if(newTP > 0)
            {
               MqlTradeRequest request = {};
               MqlTradeResult  result  = {};
               request.action = TRADE_ACTION_SLTP;
               request.position = ticket;
               request.tp = NormalizeDouble(newTP, (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS));
               request.sl = PositionGetDouble(POSITION_SL);

               if(OrderSend(request, result))
                  Print("Auto TP đã được đặt cho lệnh ", ticket, " sau khi đạt mục tiêu ngày.");
            }
         }
      }
   }
}

//+------------------------------------------------------------------+
// Hàm đóng toàn bộ vị thế đang mở
void CloseAllOrders(string reason)
{
   for(int i = PositionsTotal() - 1; i >= 0; i--)
   {
      ulong ticket = PositionGetTicket(i);
      if(PositionSelectByTicket(ticket))
      {
         MqlTradeRequest request = {};
         MqlTradeResult  result  = {};
         
         request.action       = TRADE_ACTION_DEAL;
         request.position     = ticket;
         request.symbol       = PositionGetString(POSITION_SYMBOL);
         request.volume       = PositionGetDouble(POSITION_VOLUME);
         request.type         = (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) ? ORDER_TYPE_SELL : ORDER_TYPE_BUY;
         request.price        = SymbolInfoDouble(request.symbol, (request.type == ORDER_TYPE_SELL) ? SYMBOL_BID : SYMBOL_ASK);
         request.deviation    = 20;
         
         OrderSend(request, result);
      }
   }
   
   if(InpSendAlert)
      SendNotification("EA Alert: " + reason);
   Print(reason);
}