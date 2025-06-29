#include <Trade\Trade.mqh>

//enum
enum StructType {
   SPH, // Small Pivot High
   SPL, // Small Pivot Low
   LPH, // Large Pivot High
   LPL, // Large Pivot Low
   NONE
};

enum PriceType {
   HIGH,
   CLOSE,
   LOW
};

// Struct
struct StructPoint {
   int index;
   double point;
   StructType type;
};

// Constant data
const int ONE = 1;
const int TWO = 2;
const int THREE = 3;
const string BUY = "BUY";
const string SELL = "SELL";
const double FIBO_618 = 0.618;

const int CANDLES_H1_15_DAYS = 360; // Số lượng nến H1 trong 15 ngày

// Biến toàn cục
CTrade Trade;
datetime CandleCloseTime;// Biến kiểm tra giá chạy 15p một lần 

// Input data
input double RiskTrade = 50; // Rủi ro short trade (USD)
input bool DrawStruct = true; // Vẽ cấu trúc thị trường
input bool DrawFibo = true; // Vẽ Fibo
input int PeriodADX = 14; // Chu kỳ tính toán ADX
// Input trading time
input string TradingTimeStart = "00:00";  // Thời gian VN bắt đầu buổi sáng (HH:MM)
input string TradingTimeEnd = "18:00";    // Thời gian VN kết thúc buổi sáng (HH:MM)
input int MaxOrdersPerDay = 3; // Số lượng lệnh tối đa mỗi ngày
int OnInit(){
   EventSetTimer(ONE);
   return (INIT_SUCCEEDED);
}

void OnDeinit(const int reason){
   EventKillTimer();
}

void OnTimer(){
    // Check current time and next M15 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M15, 1) + PeriodSeconds(PERIOD_M15);
   
    bool isRunningEa = false; 
    
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime <= currentTime ){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;
    }

    if(isRunningEa && IsTradingTime()){
        if(DrawStruct){
            DrawStruct();
            ChartRedraw(0);
        }

        //Print("Current ADX: ", GetCurrentADX());
    }
}

void DrawStruct(){
    ObjectsDeleteAll(0, -1, OBJ_TEXT);
    
    StructPoint pointArray[];
    SetStructPointArray(pointArray);

    for(int index = 0; index < ArraySize(pointArray); index++){
        string name = DoubleToString(pointArray[index].point);
        datetime time = iTime(_Symbol, _Period, pointArray[index].index);
        
        if(pointArray[index].type == SPH || pointArray[index].type == LPH){
            ObjectCreate(0, name, OBJ_TEXT, 0, time, pointArray[index].point);
            string text = pointArray[index].type == SPH ? "SPH" : "LPH";
            ObjectSetString(0, name, OBJPROP_TEXT, text);
            color colorText = pointArray[index].type == SPH ? clrCyan : clrGreen;
            ObjectSetInteger(0, name, OBJPROP_COLOR, colorText);
            ObjectSetInteger(0, name, OBJPROP_ANCHOR, ANCHOR_LOWER);
            int sizeText = pointArray[index].type == SPH ? 6 : 10;
            ObjectSetInteger(0, name, OBJPROP_FONTSIZE, sizeText);
        } 
        if(pointArray[index].type == SPL || pointArray[index].type == LPL){
            ObjectCreate(0, name, OBJ_TEXT, 0, time, pointArray[index].point);
            string text = pointArray[index].type == SPL ? "SPL" : "LPL";
            ObjectSetString(0, name, OBJPROP_TEXT, text);
            color colorText = pointArray[index].type == SPL ? clrTomato : clrRed;
            ObjectSetInteger(0, name, OBJPROP_COLOR, colorText);
            ObjectSetInteger(0, name, OBJPROP_ANCHOR, ANCHOR_UPPER);
            int sizeText = pointArray[index].type == SPL ? 6 : 10;
            ObjectSetInteger(0, name, OBJPROP_FONTSIZE, sizeText);
        }  
    }
}

string GetTypeTrend(){
    int pointBullishIndex = 0, pointBearishIndex = 0;
    double lowestPoint = iLow(_Symbol, PERIOD_H1, 0), highestPoint = iHigh(_Symbol, PERIOD_H1, 0);

    for(int index = ONE; index < CANDLES_H1_15_DAYS; index++){
        // Kiểm tra nếu cây nến tăng giá (giá đóng cửa > giá mở cửa)
        if(lowestPoint >= iLow(_Symbol, PERIOD_H1, index)){
            lowestPoint = iLow(_Symbol, PERIOD_H1, index);
            pointBullishIndex = index;
        } 
        if(highestPoint <= iHigh(_Symbol, PERIOD_H1, index)){
            highestPoint = iHigh(_Symbol, PERIOD_H1, index);
            pointBearishIndex = index;
        } 
    }

    // So sánh chỉ số của điểm tăng và giảm
    if(pointBearishIndex > pointBullishIndex){
        return SELL;
    } else return BUY;
}

double GetPriceHighest(int start, int end, PriceType priceType, bool isGetIndex){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    int copied = CopyRates(Symbol(), PERIOD_M15, 0, start + 1, rates);
    
    double priceHighest = (priceType == HIGH) ? rates[start].high : rates[start].close;
    int indexResult = start;
    
    for(int index = start; index >= end; index--){ 
        double currentPrice = (priceType == HIGH) ? rates[index].high : rates[index].close;
        if(currentPrice > priceHighest){
            priceHighest = currentPrice;
            indexResult = index;
        }
    }
    
    return isGetIndex ? indexResult : priceHighest;
}

double GetPriceLowest(int start, int end, PriceType priceType, bool isGetIndex){
   MqlRates rates[];
   ArraySetAsSeries(rates, true);
   int copied = CopyRates(Symbol(), PERIOD_H1, 0, start + 1, rates);
   
   double priceLowest = (priceType == LOW) ? rates[start].low : rates[start].close;
   int indexResult = start;
   
   for(int index = start; index >= end; index--){ 
      double currentPrice = (priceType == LOW) ? rates[index].low : rates[index].close;
      if(currentPrice < priceLowest){
         priceLowest = currentPrice;
         indexResult = index;
      }
   }
   
   return isGetIndex ? indexResult : priceLowest;
}

int GetIndexPointHigh(StructPoint &structPointArray[], int start, int end){
   double high = 0.0;
   int result = 0;
   for(int index = start + 1; index < end; index++){
      if(structPointArray[index].type == SPH && structPointArray[index].point > high){
         high = structPointArray[index].point;
         result = index;
      }
   }
   
   return result;
}

int GetIndexPointLow(StructPoint &structPointArray[], int start, int end){
   double low = structPointArray[start].point;
   int result = 0;
   for(int index = start + 1; index < end; index++){
      if(structPointArray[index].type == SPL && structPointArray[index].point < low){
         low = structPointArray[index].point;
         result = index;
      }
   }
   
   return result;
}

void SetStructPointArray(StructPoint &myArray[]){
    MqlRates ratesH1[];
    ArraySetAsSeries(ratesH1, true);
    int copied = CopyRates(_Symbol, PERIOD_H1, 0, CANDLES_H1_15_DAYS, ratesH1);
    if(copied < 0) return;
    
    StructPoint structPointArray[];
    ArrayResize(structPointArray, 1000);
    int indexStructPointArray = 0;
    
    StructPoint tempSpl, tempSph;
    // Cập nhập cấu trúc
    bool checkLPL = false, checkLPH = false;
    int start = 0;
    double temp = 0.0;

    int startIndexPoint = 0;
    if(GetTypeTrend() == BUY){
        tempSpl.index = (int)GetPriceLowest(copied - 1, 0, LOW, true);
        tempSpl.point = GetPriceLowest(copied - 1, 0, LOW, false);
        tempSpl.type = SPL;
        tempSph.type = NONE;
        startIndexPoint = tempSpl.index;
        checkLPL = true;
    } else if (GetTypeTrend()== SELL){
        tempSph.index = (int)GetPriceHighest(copied - 1, 0, HIGH, true);
        tempSph.point = GetPriceHighest(copied - 1, 0, HIGH, false);
        tempSph.type = SPH;
        tempSpl.type = NONE;
        startIndexPoint = tempSph.index;
        checkLPH = true;
    }

    while(startIndexPoint >= 0){
        // Kiểm tra khi có đáy tạm
        if(tempSpl.type == SPL){
            if(ratesH1[startIndexPoint].low < tempSpl.point){
                // Cập nhật đáy tạm
                tempSpl.index = startIndexPoint;
                tempSpl.point = ratesH1[startIndexPoint].low;
            }

            if(ratesH1[startIndexPoint].close > ratesH1[tempSpl.index].high){
                // Từ đáy tạm trở thành đáy thực
                structPointArray[indexStructPointArray].index = tempSpl.index;
                structPointArray[indexStructPointArray].type = tempSpl.type;
                structPointArray[indexStructPointArray].point = tempSpl.point;
        
                indexStructPointArray++;
                // Khởi tạo đỉnh tạm
                tempSph.index = tempSpl.index;
                tempSph.point = ratesH1[tempSpl.index].high;
                tempSph.type = SPH;
                // Xóa bỏ đáy tạm
                tempSpl.type = NONE;
                
                startIndexPoint = tempSpl.index;
            } 
        } else if(tempSph.type == SPH){
            if(ratesH1[startIndexPoint].high > tempSph.point){
                // Cập nhật đỉnh tạm
                tempSph.index = startIndexPoint;
                tempSph.point = ratesH1[startIndexPoint].high;
            }

            if(ratesH1[startIndexPoint].close < ratesH1[tempSph.index].low){
                // Từ đỉnh tạm trở thành đỉnh thực
                structPointArray[indexStructPointArray].index = tempSph.index;
                structPointArray[indexStructPointArray].type = tempSph.type;
                structPointArray[indexStructPointArray].point = tempSph.point;
                indexStructPointArray++;
                // Khởi tạo đáy tạm
                tempSpl.index = tempSph.index;
                tempSpl.point = ratesH1[tempSph.index].low;
                tempSpl.type = SPL;
                // Xóa bỏ đỉnh tạm
                tempSph.type = NONE;

                startIndexPoint = tempSph.index;
            }
        }

        startIndexPoint--;
    }
    // Sao chép dữ liệu từ mảng tạm vào mảng kết quả
    ArrayResize(myArray, indexStructPointArray);
    for(int index = 0; index < indexStructPointArray; index++){
        myArray[index] = structPointArray[index];
    }
    
    int index = 0;
    while(index < indexStructPointArray){
        if(checkLPH && myArray[index].type == SPL){
            if(temp != 0.0){
                if(temp <= myArray[index].point){
                    temp = myArray[index].point;
                } else {
                    int result = GetIndexPointHigh(myArray, start, index);
                    myArray[result].type = LPH;
                    temp = myArray[result].point;
                    checkLPL = true;
                    checkLPH = false;
                    start = result;
                    
                    if(index < indexStructPointArray){
                        index = result;
                        continue;
                    }
                }
            } else temp = myArray[index].point;
        } else if(checkLPL && myArray[index].type == SPH){
            if(temp != 0.0){
                if(temp >= myArray[index].point){
                    temp = myArray[index].point;
                } else {
                    int result = GetIndexPointLow(myArray, start, index);
                    myArray[result].type = LPL;

                    temp = myArray[result].point;
                    checkLPL = false;
                    checkLPH = true;
                    start = result;
                    
                    if(index < indexStructPointArray){
                      index = result;
                        continue;
                    }
                }
            } else temp = myArray[index].point;
        }
        index++;
    }
}

double GetCurrentADX(){
    // Lấy handle của chỉ báo ADX
    int adxHandle = iADX(_Symbol, PERIOD_M15, PeriodADX);

    if(adxHandle == INVALID_HANDLE){
        Print("Không thể tạo handle cho ADX. Lỗi: ", GetLastError());
        return -1;
    }
    
    // Sao chép dữ liệu ADX
    double adxValues[];
    if(CopyBuffer(adxHandle, 0, 0, ONE, adxValues) <= 0){
        Print("Không thể sao chép dữ liệu ADX. Lỗi: ", GetLastError());
        IndicatorRelease(adxHandle);
        return -1;
    }
    
    // Giải phóng handle
    IndicatorRelease(adxHandle);

    return adxValues[0];
}

double GetLotSize(double stopLossDistance){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    
    // Kiểm tra giá trị hợp lệ
    if (tickValue <= 0 || tickSize <= 0) return 0.0;
   
    // Tính số pips tương ứng với stopLossDistance
    double stopLossPips = stopLossDistance / _Point;
    
    // Tính giá trị pip
    double pipValue = tickValue / (tickSize / _Point);
    
    // Tính toán kích thước lô
    double lotSize = RiskTrade / (stopLossPips * pipValue);
    
    // Đảm bảo rằng kích thước lô nằm trong phạm vi cho phép của sàn giao dịch
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
   
    // Làm tròn kích thước lô đến số thập phân phù hợp
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lotSize = MathFloor(lotSize / lotStep) * lotStep;

    return lotSize;
}

double GetFibo618Data(double start, double end){   
   if(start == 0 && end == 0) return 0;
      
   return NormalizeDouble((start - end) * FIBO_618 + end, _Digits);
}

bool IsTradingTime(){
    datetime brokerTime = TimeCurrent(); // Thời gian hiện tại theo broker
    // Chuyển sang giờ Việt Nam
    datetime vietnamTime = brokerTime + GetBrokerTimezoneOffset() * 3600;

    MqlDateTime timeStruct;
    TimeToStruct(vietnamTime, timeStruct);
    int vnHour = timeStruct.hour;
    int vnMinute = timeStruct.min;
    
    // Phân tích thời gian nhập vào
    string startParts[];
    if(StringSplit(TradingTimeStart, ':', startParts) != 2) return false;
    int startHour = (int)StringToInteger(startParts[0]);
    int startMinute = (int)StringToInteger(startParts[1]);
    
    string endParts[];
    if(StringSplit(TradingTimeEnd, ':', endParts) != 2) return false;
    int endHour = (int)StringToInteger(endParts[0]);
    int endMinute = (int)StringToInteger(endParts[1]);
    
    // So sánh thời gian
    int currentTotal = vnHour * 60 + vnMinute;
    int startTotal = startHour * 60 + startMinute;
    int endTotal = endHour * 60 + endMinute;
    
    // Xử lý trường hợp qua đêm (ví dụ: 22:00 tối đến 05:00 sáng)
    if (startTotal > endTotal) {
        // Khoảng thời gian qua đêm
        return (currentTotal >= startTotal || currentTotal <= endTotal);
    } else {
        // Khoảng thời gian trong cùng một ngày
        return (currentTotal >= startTotal && currentTotal <= endTotal);
    }
}

int GetBrokerTimezoneOffset(){
   // Lấy thời gian local và server, tính chênh lệch
   datetime serverTime = TimeCurrent();
   datetime localTime = TimeLocal();
   
   int diff = int((localTime - serverTime) / 3600);
   return diff;
}

void managePositions(){
    // Lặp qua tất cả các vị thế đang mở
    for(int index = 0; index < PositionsTotal(); index++){
        // Lấy thông tin vị thế
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        // Lấy thông tin chi tiết của vị thế
        double entry = PositionGetDouble(POSITION_PRICE_OPEN);
        double stopLoss = PositionGetDouble(POSITION_SL);
        double takeProfit = PositionGetDouble(POSITION_TP);
        double currentPrice = PositionGetDouble(POSITION_PRICE_CURRENT);
        string symbol = PositionGetString(POSITION_SYMBOL);
        ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);

        if(PositionSelectByTicket(ticket) && symbol == _Symbol){
            double stopLossDistance = MathAbs(entry - stopLoss);
            double takeProfitDistance = MathAbs(entry - takeProfit);

            if(type == POSITION_TYPE_BUY && currentPrice > entry){
                // Dời BE khi được 1R
                if(currentPrice - entry >= stopLossDistance && entry > stopLoss) 
                    ModifyStopLoss(ticket, entry, takeProfit);
                // Dời Stop Loss khi đạt 1.3R
                if(currentPrice > entry + stopLossDistance * 1.3 && !stopLossDistance){
                    double newStopLoss = entry + stopLossDistance * 0.5; // Dời Stop Loss về 0.5R
                    ModifyStopLoss(ticket, newStopLoss, takeProfit);
                }
                // Dời Stop Loss khi đạt 1.6R
                if(currentPrice > entry + stopLossDistance * 1.6 && entry < stopLoss){
                    double newStopLoss = entry + stopLossDistance; // Dời Stop Loss về 1R
                    ModifyStopLoss(ticket, newStopLoss, takeProfit);
                }
            }else if(type == POSITION_TYPE_SELL && currentPrice < entry){
                // Dời BE khi được 1R
                if(entry - currentPrice >= stopLossDistance && entry < stopLoss)
                    ModifyStopLoss(ticket, entry, takeProfit);
                // Dời Stop Loss khi đạt 1.3R
                if(currentPrice < entry - stopLossDistance * 1.3 && !stopLossDistance){
                    double newStopLoss = entry - stopLossDistance * 0.5; // Dời Stop Loss về 0.5R
                    ModifyStopLoss(ticket, newStopLoss, takeProfit);
                }
                // Dời Stop Loss khi đạt 1.6R
                if(currentPrice < entry - stopLossDistance * 1.6 && entry > stopLoss){
                    double newStopLoss = entry - stopLossDistance; // Dời Stop Loss về 1R
                    ModifyStopLoss(ticket, newStopLoss, takeProfit);
                }
            }
        }
    }
}

// Hàm hỗ trợ để điều chỉnh Stop Loss
void ModifyStopLoss(ulong ticket, double newStopLoss, double takeProfit){
    newStopLoss = NormalizeDouble(newStopLoss, _Digits);
    if(!Trade.PositionModify(ticket, newStopLoss, takeProfit)){
        Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
    }
}

bool CheckDailyOrderLimit(){
    int dailyOrders = 0;
    datetime currentDay = iTime(_Symbol, PERIOD_D1, 0); // Thời gian bắt đầu của ngày hiện tại
   
    // Kiểm tra lệnh đang hoạt động (pending và positions)
    if(OrdersTotal() > 0 || PositionsTotal() > 0){
        for(int index = 0; index < OrdersTotal(); index++){
            if(OrderGetTicket(index) > 0){
                if(OrderGetString(ORDER_SYMBOL) == _Symbol && 
                OrderGetInteger(ORDER_TIME_SETUP) >= currentDay)
                    dailyOrders++;
            }
        }
      
        for(int index = 0; index < PositionsTotal(); index++){
            if(PositionSelectByTicket(PositionGetTicket(index))){
                if(PositionGetString(POSITION_SYMBOL) == _Symbol && 
                    PositionGetInteger(POSITION_TIME) >= currentDay)
                    dailyOrders++;
            }
        }
    }    
   
    // Kiểm tra lịch sử lệnh trong ngày
    if(HistorySelect(currentDay, TimeCurrent())){
        int totalHistoryOrders = HistoryOrdersTotal();
        for(int index = 0; index < totalHistoryOrders; index++){
            ulong ticket = HistoryOrderGetTicket(index);
            if(HistoryOrderGetString(ticket, ORDER_SYMBOL) == _Symbol && 
                HistoryOrderGetInteger(ticket, ORDER_TIME_DONE) >= currentDay){
                dailyOrders++;
            }
        }
    }
   
    if(dailyOrders >= MaxOrdersPerDay){
        Print("Đã đạt giới hạn ", MaxOrdersPerDay, " lệnh trong ngày. Không thể mở thêm lệnh.");
        return false;
    }
   
    return true;
}