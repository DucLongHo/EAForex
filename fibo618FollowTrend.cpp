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
const int FIBO_618 = 618;

const int CANDLES_H1_15_DAYS = 360; // Số lượng nến H1 trong 15 ngày

// Biến toàn cục
CTrade Trade;
datetime CandleCloseTime;// Biến kiểm tra giá chạy 15p một lần 

// Input data
input double RiskTrade = 50; // Rủi ro short trade (USD)
input bool DrawStruct = true; // Vẽ cấu trúc thị trường
input bool DrawFibo = true; // Vẽ Fibo
input int PeriodADX = 14; // Period for ADX calculation
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

    if (isRunningEa){
        if(DrawStruct) DrawStruct();
        ChartRedraw(0);
        Print("Current ADX: ", GetCurrentADX());
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
    
    int i = 0;
    while(i < indexStructPointArray){
        if(checkLPH && myArray[i].type == SPL){
            if(temp != 0.0){
                if(temp <= myArray[i].point){
                    temp = myArray[i].point;
                } else {
                    int result = GetIndexPointHigh(myArray, start, i);
                    myArray[result].type = LPH;
                    temp = myArray[result].point;
                    checkLPL = true;
                    checkLPH = false;
                    start = result;
                    
                    if(i < indexStructPointArray){
                        i = result;
                        continue;
                    }
                }
            } else temp = myArray[i].point;
        } else if(checkLPL && myArray[i].type == SPH){
            if(temp != 0.0){
                if(temp >= myArray[i].point){
                    temp = myArray[i].point;
                } else {
                    int result = GetIndexPointLow(myArray, start, i);
                    myArray[result].type = LPL;

                    temp = myArray[result].point;
                    checkLPL = false;
                    checkLPH = true;
                    start = result;
                    
                    if(i < indexStructPointArray){
                        i = result;
                        continue;
                    }
                }
            } else temp = myArray[i].point;
        }
        i++;
    }
}

double GetCurrentADX(){
    // Lấy handle của chỉ báo ADX
    int adx_handle = iADX(_Symbol, PERIOD_M15, PeriodADX);
    
    if(adx_handle == INVALID_HANDLE){
        Print("Không thể tạo handle cho ADX. Lỗi: ", GetLastError());
        return -1;
    }
    
    // Sao chép dữ liệu ADX
    double adx_values[];
    if(CopyBuffer(adx_handle, 0, 0, ONE, adx_values) <= 0){
        Print("Không thể sao chép dữ liệu ADX. Lỗi: ", GetLastError());
        IndicatorRelease(adx_handle);
        return -1;
    }
    
    // Giải phóng handle
    IndicatorRelease(adx_handle);
    
    return adx_values[0];
}