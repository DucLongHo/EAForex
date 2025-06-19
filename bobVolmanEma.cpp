#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;
CChartObjectButton TradeButton;// Nút bật/tắt giao dịch
// Input parameters
input double RiskTrade = 0.01; // Khối lượng giao dịch
input ENUM_TIMEFRAMES TimeFrame = PERIOD_CURRENT;  // Khung thời gian tham chiếu
input double TotalLossPips = 0.0;  // Tổng số pip chấp nhận thua lỗ

// Constant data
const int PERIOD_EMA_BIG = 35;
const int PERIOD_EMA_MEDIUM = 21;
const int PERIOD_EMA_SMALL = 15;

const int ONE = 1;
const int TWO = 2;

const int CANDLES_M5 = 336;

datetime CandleCloseTime;// Biến kiểm tra giá chạy 5p một lần 
bool tradingEnabled = true; // Biến kiểm soát trạng thái giao dịch

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
   EventSetTimer(ONE);

   // Tạo nút và thiết lập thuộc tính
   if(!TradeButton.Create(0, "TradeButton", 0, 100, 200, 150, 50))
      return(INIT_FAILED);

   TradeButton.Description("TRADING: ON");
   TradeButton.Color(clrWhite);
   TradeButton.BackColor(clrGreen); 
   TradeButton.FontSize(12);
   TradeButton.Font("Arial");
   TradeButton.Selectable(true);
   ObjectSetInteger(0, "TradeButton", OBJPROP_ZORDER, 10);
   ChartRedraw(0);
   
   return (INIT_SUCCEEDED);
}

void OnTimer(){
   // Check current time and next M15 candle close time
   datetime currentTime = TimeCurrent();
   datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);

   datetime closeTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);
   string timeString = TimeToString(closeTime - TimeCurrent(), TIME_SECONDS);
   double totalPips = CalculateTotalPips();

   Comment("Đếm ngược thời gian đóng nến: ", timeString, "\n", "\n",
   "Khối lượng giao dịch: ", DoubleToString(RiskTrade, TWO), " Lot", "\n",
   "Khung thời gian tham chiếu: ", EnumToString(TimeFrame), "\n", "\n",
   "Tổng pips chấp nhận thua lỗ: ", DoubleToString(TotalLossPips, 2), "\n",
   "Tổng pips hiện tại: ", DoubleToString(totalPips, 2), "\n");

   if(totalPips <= TotalLossPips){
      closeAllPositions();
   }
   
   bool isRunningEa = false;
   if(currentCandleCloseTime != CandleCloseTime &&
      currentCandleCloseTime - currentTime <= 2 ){
      CandleCloseTime = currentCandleCloseTime;
      isRunningEa = true;
   }
   if(isRunningEa && tradingEnabled){
      // Vẽ tất cả các EMA
      DrawAllEMAs();
      isRunningEa = false;
   }
}

void OnDeinit(const int reason){
   TradeButton.Delete();
   EventKillTimer();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
   // Kiểm tra sự kiện nhấp chuột trên nút
   if(id == CHARTEVENT_OBJECT_CLICK && sparam == "TradeButton") {
      tradingEnabled = !tradingEnabled; // Đảo ngược trạng thái giao dịch
      if (tradingEnabled) {
         TradeButton.Description("TRADING: ON");
         TradeButton.Color(clrWhite);
         TradeButton.BackColor(clrGreen); // Màu xanh lá khi bật
      } else {
         TradeButton.Description("TRADING: OFF");
         TradeButton.Color(clrWhite);
         TradeButton.BackColor(clrRed); // Màu đỏ khi tắt
      }
   }
   // Kiểm tra sự kiện nhấn chuột trái
   if(id == CHARTEVENT_OBJECT_CLICK && TradeButton.State()){
      // Lấy tọa độ chuột
      int x_distance = (int)lparam;
      int y_distance = (int)dparam;
      // Di chuyển nút đến vị trí mới
      TradeButton.X_Distance(x_distance);
      TradeButton.Y_Distance(y_distance);
      // Vẽ lại biểu đồ
      ChartRedraw(0);
   }
}

void DrawAllEMAs(){
   // Xóa các đối tượng cũ nếu tồn tại
   ObjectsDeleteAll(0, -1, OBJ_TREND);
   
   double emaSmall[], emaMedium[], emaBig[];
   int handleSmall = iMA(_Symbol, _Period, PERIOD_EMA_SMALL, 0, MODE_EMA, PRICE_CLOSE);
   int handleMedium = iMA(_Symbol, _Period, PERIOD_EMA_MEDIUM, 0, MODE_EMA, PRICE_CLOSE);
   int handleBig = iMA(_Symbol, _Period, PERIOD_EMA_BIG, 0, MODE_EMA, PRICE_CLOSE);

   if (handleSmall < 0 || handleMedium < 0 || handleBig < 0) return;
   ArraySetAsSeries(emaSmall, true);
   ArraySetAsSeries(emaMedium, true);
   ArraySetAsSeries(emaBig, true);
   if(CopyBuffer(handleBig, 0, 0, CANDLES_M5, emaBig) <= 0 ||
      CopyBuffer(handleMedium, 0, 0, CANDLES_M5, emaMedium) <= 0 || 
      CopyBuffer(handleSmall, 0, 0, CANDLES_M5, emaSmall) <= 0) return;

   string trendNow = DetermineTrend(emaSmall[0], emaMedium[0], emaBig[0]);
   string trendBefore = DetermineTrend(emaSmall[1], emaMedium[1], emaBig[1]);
   if(trendNow != trendBefore){
      closeAllPositions();
      string referenceTrend = getReferenceTrend();
      if(trendNow == "BUY" && 
         (referenceTrend == "BUY" || referenceTrend == "NEUTRAL")){
         double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
         if(!Trade.Buy(RiskTrade, _Symbol, entry)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
         } else {
            Alert("Tham Gia Thị Trường");
         }
         Comment("Trend is BUY");
      } else if(trendNow == "SELL" && 
         (referenceTrend == "SELL" || referenceTrend == "NEUTRAL")){
         double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
         if(!Trade.Sell(RiskTrade, _Symbol, entry)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
         } else {
            Alert("Tham Gia Thị Trường");
         }
         Comment("Trend is SELL");
      } else {
         Comment("Trend is NEUTRAL");
      }
   }
   for(int index = 0; index < CANDLES_M5 - ONE; index++){
      datetime startTime = iTime(_Symbol, _Period, index);
      datetime endTime = iTime(_Symbol, _Period, index + 1);

      double priceStartEmaSmall = emaSmall[index];
      double priceEndEmaSmall = emaSmall[index + 1];

      double priceStartEmaMedium  = emaMedium[index];
      double priceEndEmaMedium = emaMedium[index + 1];

      double priceStartEmaBig = emaBig[index];
      double priceEndEmaBig = emaBig[index + 1];

      if(priceStartEmaSmall == 0 || priceEndEmaSmall == 0 || 
         priceStartEmaMedium == 0 || priceEndEmaMedium == 0 || 
         priceStartEmaBig == 0 || priceEndEmaBig == 0) return;

      color colorEma = DetermineTrendColor(priceStartEmaSmall, priceStartEmaMedium, priceStartEmaBig);
      
      string nameEmaSmall = "EMASmall_"+IntegerToString(index);
      string nameEmaMedium = "EMAMedium_"+IntegerToString(index);
      string nameEmaBig = "EMABig_"+IntegerToString(index);

      ObjectCreate(0, nameEmaSmall, OBJ_TREND, 0, startTime, priceStartEmaSmall, endTime, priceEndEmaSmall);
      ObjectSetInteger(0, nameEmaSmall, OBJPROP_COLOR, colorEma);
      ObjectSetInteger(0, nameEmaSmall, OBJPROP_WIDTH, 2);

      ObjectCreate(0, nameEmaMedium, OBJ_TREND, 0, startTime, priceStartEmaMedium, endTime, priceEndEmaMedium);
      ObjectSetInteger(0, nameEmaMedium, OBJPROP_COLOR, colorEma);
      ObjectSetInteger(0, nameEmaMedium, OBJPROP_WIDTH, 2);

      ObjectCreate(0, nameEmaBig, OBJ_TREND, 0, startTime, priceStartEmaBig, endTime, priceEndEmaBig);
      ObjectSetInteger(0, nameEmaBig, OBJPROP_COLOR, colorEma);
      ObjectSetInteger(0, nameEmaBig, OBJPROP_WIDTH, 2);
   }

   ChartRedraw();
}

string DetermineTrend(double small, double medium, double big){
   //if(small > medium && small > big && medium > big) return "BUY";
   //if(small < medium && small < big && medium < big) return "SELL";
   if(small > medium && small > big) return "BUY";
   if(small < medium && small < big) return "SELL";
   if(small > medium && small < big) return "SELL";
   if(small < medium && small > big) return "BUY";
   return "NEUTRAL";
}

//+------------------------------------------------------------------+
//| Xác định màu theo xu hướng                                      |
//+------------------------------------------------------------------+
color DetermineTrendColor(double small, double medium, double big){
   string trend = DetermineTrend(small, medium, big);
   if(trend == "BUY") return clrGreen;
   if(trend == "SELL") return clrRed;
   return clrGray;
}

void closeAllPositions(){
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

string getReferenceTrend(){
   double emaSmall[], emaMedium[], emaBig[];
   int handleSmall = iMA(_Symbol, TimeFrame, PERIOD_EMA_SMALL, 0, MODE_EMA, PRICE_CLOSE);
   int handleMedium = iMA(_Symbol, TimeFrame, PERIOD_EMA_MEDIUM, 0, MODE_EMA, PRICE_CLOSE);
   int handleBig = iMA(_Symbol, TimeFrame, PERIOD_EMA_BIG, 0, MODE_EMA, PRICE_CLOSE);

   if (handleSmall < 0 || handleMedium < 0 || handleBig < 0) return "NEUTRAL";
   ArraySetAsSeries(emaSmall, true);
   ArraySetAsSeries(emaMedium, true);
   ArraySetAsSeries(emaBig, true);
   if(CopyBuffer(handleBig, 0, 0, ONE, emaBig) <= 0 ||
      CopyBuffer(handleMedium, 0, 0, ONE, emaMedium) <= 0 || 
      CopyBuffer(handleSmall, 0, 0, ONE, emaSmall) <= 0) return "NEUTRAL";

   return DetermineTrend(emaSmall[0], emaMedium[0], emaBig[0]);
}

double CalculateTotalPips(){
   double totalPips = 0.0;
   
   // Lấy tổng số order hiện tại
   int totalOrders = PositionsTotal();
   
   // Lặp qua tất cả các order
   for(int index = 0; index < totalOrders; index++){
      // Lấy thông tin position
      if(PositionGetTicket(index)){
         // Lấy thông tin cần thiết
         string symbol = PositionGetString(POSITION_SYMBOL);
         double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
         double currentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
         long positionType = PositionGetInteger(POSITION_TYPE);
         
         if(positionType == POSITION_TYPE_BUY)
            totalPips += (currentPrice - openPrice) / _Point;
         else if(positionType == POSITION_TYPE_SELL)
            totalPips += (openPrice - currentPrice) / _Point;
      }
   }
   
   return totalPips;
}