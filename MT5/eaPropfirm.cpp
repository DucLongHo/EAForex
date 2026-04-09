#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton SLBE;// Nút dời SL về điểm hòa vốn

CChartObjectLabel lblTotalSL, lblTotalTP, lblLengthCandle, lblTimeCountDown;

// Input parameters
input double   InpMaxLossAmount  = 15.0;    // Số tiền rủi ro tối đa trên mỗi lệnh (Ví dụ: 100$)
input int FIVE = 50; // Số pip để đặt SL/TP cách điểm hòa vốn

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";
const int PERIOD_EMA = 25;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool SlBeEnabled = false; // Biến kiểm soát dời SL về điểm hòa vốn

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
   EventSetTimer(ONE);

   int x = (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
   int y =  (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 50;

   if(!CreateButton(SLBE, "SLBEButton", "BREAK EVEN", clrGreen, x, y - 100))
      return(INIT_FAILED);
      
   // Tạo label
   if(!CreateLable(lblTotalSL, "TotalSLLabel", "Total SL: 0.00 USD", x, 30))
      return(INIT_FAILED);

   if(!CreateLable(lblTotalTP, "TotalTP", "Total TP: 0.00 USD", x, 60))
      return(INIT_FAILED);

   if(!CreateLable(lblLengthCandle, "LengthCandle", "Length: ", x, 90))
      return(INIT_FAILED);
   
   if(!CreateLable(lblTimeCountDown, "TimeCountDown", "Countdown:  s", x, 120))
      return(INIT_FAILED);
    
    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
   // Check current time and next M1 candle close time
   datetime currentTime = TimeCurrent();
   datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, 0) + PeriodSeconds(PERIOD_M1);

   
   bool isRunningEa = false;
   if(currentCandleCloseTime != CandleCloseTime &&
      currentCandleCloseTime - currentTime <= TWO ){
      CandleCloseTime = currentCandleCloseTime;
      isRunningEa = true;
   }

   if(isRunningEa){
      Draw();
      
      isRunningEa = false;
   }

   CalculateLable();
   CheckAndAdjustStopLoss();
}

void OnTick(){
   if(SlBeEnabled){
      SlBeEnabled = !SlBeEnabled;
      MoveSLBE();
   }
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){
   // Nhấn nút dời SL về điểm hòa vốn
   if(id == CHARTEVENT_OBJECT_CLICK && sparam == "SLBEButton"){
      SlBeEnabled = !SlBeEnabled;
   }
}
void OnDeinit(const int reason){
   SLBE.Delete();
   EventKillTimer();
}

void MoveSLBE(){
   // Code dời SL về điểm hòa vốn
   for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
      ulong ticket = PositionGetTicket(index);
      if(ticket <= 0) continue;
      
      if(PositionSelectByTicket(ticket)){
         double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);
         double currentTP = PositionGetDouble(POSITION_TP);
         double currentSL = PositionGetDouble(POSITION_SL);
         double profit = PositionGetDouble(POSITION_PROFIT);
         ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
         
         double newPosition = (type == POSITION_TYPE_BUY) ? openPrice + FIVE * _Point : openPrice - FIVE * _Point;

         if(profit > 0){
            if(!Trade.PositionModify(ticket, newPosition, currentTP)){
                  Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
            }
         } else {
            if(!Trade.PositionModify(ticket, currentSL, newPosition)){
                  Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
            }         
         }
      }    
   }
}

bool CreateButton(CChartObjectButton &button, string name, string des, color bgColor,int x, int y){
   // Tạo nút và thiết lập thuộc tính
   if(!button.Create(0, name, 0, x, y, 175, 35))
      return false;
   
   button.Description(des);
   button.Color(clrWhite);
   button.BackColor(bgColor); 
   button.FontSize(12);
   button.Font("Calibri");
   button.Selectable(true);
   button.BorderColor(clrWhite);
   
   return true;
}

bool CreateLable(CChartObjectLabel &lable, string name, string des, int x, int y){
   // Tạo lable và thiết lập thuộc tính
   if(!lable.Create(0, name, 0, x, y))
      return false;

   lable.Description(des);
   lable.Color(clrWhite);
   lable.Font("Calibri");
   lable.FontSize(12);

   return true;
}


void DrawMarkerPrice(ENUM_TIMEFRAMES timeframe, color lineColor){
   double emaValue[];
   int handle = iMA(_Symbol, timeframe, PERIOD_EMA, 0, MODE_EMA, PRICE_CLOSE);;
   if(handle < 0) return ;

   ArraySetAsSeries(emaValue, true);
   if(CopyBuffer(handle, 0, 0, ONE, emaValue) <= 0) return;

   double price = emaValue[0];
   if(price == 0) return;

   datetime currentTime = iTime(_Symbol, PERIOD_CURRENT, 0);
   datetime start = currentTime + PeriodSeconds(PERIOD_CURRENT) * 10;
   datetime end = currentTime + PeriodSeconds(PERIOD_CURRENT) * 2;
   
   string lineName = "Price " + DoubleToString(price);
   string textName = "TimeframeLabel_" + IntegerToString(timeframe);

   ObjectCreate(0, lineName, OBJ_TREND, 0, start, price, end, price);
   ObjectSetInteger(0, lineName, OBJPROP_COLOR, lineColor);
   ObjectSetInteger(0, lineName, OBJPROP_WIDTH, 2);
   
   ObjectCreate(0, textName, OBJ_TEXT, 0, start, price + 52 * _Point);
   ObjectSetString(0, textName, OBJPROP_TEXT, StringSubstr(EnumToString(timeframe), 7));
   ObjectSetInteger(0, textName, OBJPROP_COLOR, lineColor);
   ObjectSetInteger(0, textName, OBJPROP_ANCHOR, ANCHOR_LEFT_UPPER);
   ObjectSetInteger(0, textName, OBJPROP_FONTSIZE, 10);
}

void Draw(){
   ObjectsDeleteAll(0, -1, OBJ_TREND);
   ObjectsDeleteAll(0, -1, OBJ_TEXT);

   DrawMarkerPrice(PERIOD_H1, clrGray);
   DrawMarkerPrice(PERIOD_M15, clrTeal);
}

void CalculateLable(){
   datetime currentTime = TimeCurrent();
   datetime nextCandleTime = iTime(_Symbol, PERIOD_M1, 0) + PeriodSeconds(PERIOD_M1);
   int secondsToNextCandle = (int)(nextCandleTime - currentTime - 1);
   
   lblTimeCountDown.Description("Countdown: " + IntegerToString(secondsToNextCandle) + " s");

   double high = iHigh(_Symbol, PERIOD_M1, 0);
   double low = iLow(_Symbol, PERIOD_M1, 0);
   double open = iOpen(_Symbol, PERIOD_M1, 0);
   double close = iClose(_Symbol, PERIOD_M1, 0);

   if(open > close){
      lblLengthCandle.Description("Length: " + DoubleToString(high - close, 2) + " pips");
   } else {
      lblLengthCandle.Description("Length: " + DoubleToString(close - low, 2) + " pips");
   }
   
   double totalSl = 0, totalTp = 0;
   
   double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
   double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
   double pointValue = tickValue / tickSize;
   
   for(int index = PositionsTotal() - ONE; index >= 0; index--) {
      ulong ticket = PositionGetTicket(index);

      if(PositionSelectByTicket(ticket)){
         double sl = PositionGetDouble(POSITION_SL);
         double tp = PositionGetDouble(POSITION_TP);

         double price = PositionGetDouble(POSITION_PRICE_OPEN);
         double volume = PositionGetDouble(POSITION_VOLUME);
         ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
         
         if(sl != 0) {
            double diff = 0;
            
            if(type == POSITION_TYPE_BUY){
               diff = sl - price; // BUY: SL > Price = lỗ
            } else diff = price - sl; // SELL: SL < Price = lỗ

            totalSl += diff * volume * pointValue * 100;
         }

         if(tp != 0) {
            double diff = 0;
            
            if(type == POSITION_TYPE_BUY){
               diff = tp - price; // BUY: TP > Price = lời
            } else diff = price - tp; // SELL: TP < Price = lời

            totalTp += diff * volume * pointValue * 100;
         }
      }
   }
    
   // Cập nhật panel
   lblTotalSL.Description("Total SL: " + DoubleToString(totalSl, 2) + " USD");
   lblTotalTP.Description("Total TP: " + DoubleToString(totalTp, 2) + " USD");

   
}


void ModifyStopLoss(ulong ticket, double newStopLoss, double currentTP){
   newStopLoss = NormalizeDouble(newStopLoss, _Digits);
   if (!Trade.PositionModify(ticket, newStopLoss, currentTP)){
      Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
   }
}

void CheckAndAdjustStopLoss(){
   // Duyệt qua tất cả các lệnh đang mở
   for(int index = PositionsTotal() - ONE; index >= 0; index--){
      ulong ticket = PositionGetTicket(index);
      
      if(PositionSelectByTicket(ticket)){

         double lot      = PositionGetDouble(POSITION_VOLUME);
         long type       = PositionGetInteger(POSITION_TYPE);
         double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
         double currentSL = PositionGetDouble(POSITION_SL);
         double curentTP = PositionGetDouble(POSITION_TP);
         
         double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
         double tickSize  = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
         
         if(tickValue <= 0) continue; // Tránh lỗi chia cho 0

         double slDistance = (InpMaxLossAmount / (lot * tickValue * 100)) * tickSize;
         double targetSL = 0;

         if(type == POSITION_TYPE_BUY){
            targetSL = NormalizeDouble(priceOpen - slDistance, _Digits);
            
            if(currentSL == 0 || currentSL < targetSL){
               ModifyStopLoss(ticket, targetSL, curentTP);
            }
         } else if(type == POSITION_TYPE_SELL){
            targetSL = NormalizeDouble(priceOpen + slDistance, _Digits);
            
            if(currentSL == 0 || currentSL > targetSL){
               ModifyStopLoss(ticket, targetSL, curentTP);
            }
         }
      }
   }
}