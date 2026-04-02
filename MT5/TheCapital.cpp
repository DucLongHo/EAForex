#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
sinput string separator0 = "------------------------------------------"; // === CÀI ĐẶT BẢN QUYỀN ===
input string Input_LicenseKey = ""; // NHẬP KEY ADMIN CUNG CẤP
sinput string separator1 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double RiskTrade = 100; // Rủi ro long trade (USD)

//--- CẤU HÌNH GIAO DỊCH ---
double RiskRewardRatio = 0.5; // Tỷ lệ Risk:Reward
double MinDistanceSL = 2500; // Stop loss tối thiểu (Points)
double RatioSLDistance = 0.5; // Tỷ lệ khoảng cách SL so với điểm vào lệnh

int TimeLeniency = 1; // Độ trễ cho việc kiểm tra thời gian đóng nến (giây)
//--- CẤU HÌNH THỜI GIAN SỬ DỤNG BOT ---
const int DAYS30 = 2592000; // 30 ngày tính bằng giây
datetime StartTimeBot = 0; // Biến lưu thời điểm bắt đầu chạy bot

// --- CẤU HÌNH BẢO MẬT ---
string SecretSalt = "20042000";

// --- THÔNG TIN BOT TELEGRAM ---
string botToken = "8520319257:AAEEh_J2dEUtd-S7CVhf9BIsA_9CFhPu0Kk"; // Token bot
string chatId = "8385086008"; // ID chat

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(1);
    
    if (!CheckLicense()) {
        return(INIT_FAILED);
    }
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M5 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M5, 0) + PeriodSeconds(PERIOD_M5);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime && 
        currentCandleCloseTime - currentTime <= 2){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;

        if(StartTimeBot + DAYS30 < TimeCurrent()) {
            Alert("Thời gian sử dụng bot đã hết hạn! Vui lòng liên hệ SĐT/Zalo: 0866797299");
            ExpertRemove();
        } else {
            RunningEA();
        }
    }
    
    if(isRunningEa) isRunningEa = false;
}

void OnTick(){
    if(PositionsTotal() > 0){
        TrailingByProfitUSD();
    }
}

void OnDeinit(const int reason){
    EventKillTimer();
}

void RunningEA(){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    int copied = CopyRates(_Symbol, PERIOD_M5, 0, 10, rates);
    if(copied <= 0) return;
    
    Trading(rates);
}

void Trading(const MqlRates &rates[]){
    MqlRates candle = rates[0], secondCandle = rates[1], thirdCandle = rates[2];
    
    double upperShadow = candle.high - MathMax(candle.open, candle.close);
    double lowerShadow = MathMin(candle.open, candle.close) - candle.low;

    if(candle.close > candle.open){
        if(secondCandle.close < secondCandle.open  
            && candle.close > secondCandle.open
            && MathAbs(candle.open - secondCandle.close) <= 100 * _Point
            && candle.high > secondCandle.high
            && candle.low < secondCandle.low
            && candle.low < thirdCandle.low
            && (candle.close - candle.open) * 0.3 > upperShadow
        ){
            BUY(candle, true); // NoSD
        }

        if(upperShadow <= 0.15 * (candle.close - candle.open)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            BUY(candle, true); // Mazubozu
        }
    } else if(candle.close < candle.open){
        if(secondCandle.close > secondCandle.open 
            && candle.close < secondCandle.open
            && MathAbs(candle.open - secondCandle.close) <= 100 * _Point 
            && candle.low < secondCandle.low
            && candle.high > secondCandle.high
            && candle.high > thirdCandle.high
            && (candle.open - candle.close) * 0.3 > lowerShadow
        ){
            SELL(candle, true); // NoSD
        }

        if(lowerShadow <= 0.15 * (candle.open - candle.close)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            SELL(candle, true); // Mazubozu
        }
    }
}

double GetLotSize(double stopLossDistance){
    // Lấy thông tin về công cụ giao dịch
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);

    double stopLossPips = stopLossDistance / _Point;

    double pipValue = tickValue / (tickSize / _Point);
   
    double lotSize = RiskTrade / (stopLossPips * pipValue);
   
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
   
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lotSize = MathFloor(lotSize / lotStep) * lotStep;

    return lotSize;
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    double trailingStep = 10 * _Point; 

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            double currentSL = PositionGetDouble(POSITION_SL);
            double currentTP = PositionGetDouble(POSITION_TP);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            double newSL = 0;
            
            double distanceFromOpen = MathAbs(currentTP - priceOpen) / RiskRewardRatio;

            // --- XỬ LÝ LỆNH BUY ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                double profit = last_tick.bid - priceOpen;
                double initialSL = priceOpen - distanceFromOpen;
                
                newSL = NormalizeDouble(initialSL + (profit * 1.5), _Digits);

                if(newSL >= currentSL + trailingStep && newSL < last_tick.bid){
                    Trade.PositionModify(ticket, newSL, currentTP);
                }
            }

            // --- XỬ LÝ LỆNH SELL ---
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                double profit = priceOpen - last_tick.ask;
                double initialSL = priceOpen + distanceFromOpen;
                
                newSL = NormalizeDouble(initialSL - (profit * 1.5), _Digits);
                

                if(newSL <= currentSL - trailingStep && newSL > last_tick.ask) {
                    Trade.PositionModify(ticket, newSL, currentTP);
                }
            }
        }
    }
}

void BUY(MqlRates &candle, bool hasTakeProfit = false){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double slDistance = (entry - candle.low) * RatioSLDistance;

    double sl = entry - slDistance;
    double lotSize = GetLotSize(MathAbs(entry - sl));
    
    if(!isOpenOrder(entry, sl))
        return;
    
    if(CountPositions("BUY") > 1 || checkEmaConditions("BUY", entry)){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2) * lotStep;
    }

    if(hasTakeProfit){
        double takeProfit = entry + RiskRewardRatio * MathAbs(entry - sl);
        if(!Trade.Buy(lotSize, _Symbol, entry, sl, takeProfit)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    } else {
        if(!Trade.Buy(lotSize, _Symbol, entry, sl)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    }
}

void SELL(MqlRates &candle, bool hasTakeProfit = false){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double slDistance = (candle.high - entry) * RatioSLDistance;
    double sl = entry + slDistance;
    double lotSize = GetLotSize(MathAbs(entry - sl));
    
    if(!isOpenOrder(entry, sl))
        return;
    
    if(CountPositions("SELL") > 1 || checkEmaConditions("SELL", entry)){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2) * lotStep;
    }

    if(hasTakeProfit){
        double takeProfit = entry - RiskRewardRatio * MathAbs(entry - sl);
        if(!Trade.Sell(lotSize, _Symbol, entry, sl, takeProfit)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    } else {
        if(!Trade.Sell(lotSize, _Symbol, entry, sl)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

bool isOpenOrder(double entry, double sl){
    if(MathAbs(entry - sl) < MinDistanceSL * _Point) return false;
    return true;
}

int CountPositions(string type) {
    int count = 0;
   
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            if(type == "BUY" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                count++;
            } else if(type == "SELL" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                count++;
            }
        }
    }
    return count;
}

bool checkEmaConditions(string trend, double price){
    int emaHandle = iMA(_Symbol, PERIOD_M5, 25, 0, MODE_EMA, PRICE_CLOSE);
    if(emaHandle < 0) return false;
    
    double ema[];
    ArraySetAsSeries(ema, true);
    if(CopyBuffer(emaHandle, 0, 0, 1, ema) <= 0) return false;

    if(trend == "BUY" && price < ema[0]) return true;
    if(trend == "SELL" && price > ema[0]) return true;

    return false;
}

// Hàm gửi tin nhắn về Telegram
void SendTelegram(string text) {
    string url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + text;
    char post[], result[];
    string headers;
    WebRequest("GET", url, headers, 1000, post, result, headers);
}

// Hàm băm dữ liệu thành chuỗi Hex 8 ký tự
string HashEngine(string data) {
    uint hash = 5381;
    for(int i = 0; i < StringLen(data); i++)
        hash = ((hash << 5) + hash) + StringGetCharacter(data, i);
    return StringFormat("%08X", hash);
}

bool CheckLicense() {
    long accID = AccountInfoInteger(ACCOUNT_LOGIN);
    datetime now = TimeCurrent();
    datetime vnTime = now + 7 * 3600; // Chuyển sang giờ Việt Nam (GMT+7)
    MqlDateTime mqlNow;
    TimeToStruct(now, mqlNow); // Lấy thông tin tháng/năm hiện tại

    // Key này sẽ thay đổi ngay khi bước sang ngày 1 của tháng kế tiếp
    string timeData = IntegerToString(accID) + SecretSalt + 
                      IntegerToString(mqlNow.mon) + 
                      IntegerToString(mqlNow.year);
    string expectedKey = HashEngine(timeData);

    if (Input_LicenseKey != expectedKey) {
        Alert("Mã Key không đúng! Vui lòng liên hệ Admin để nhận Key mới. SĐT/Zalo: 0866797299");
        
        string msg = "🔑 YEU CAU KEY MOI!%0A" + 
                     "ID: " + (string)accID + "%0A" +
                     "Vào lúc: " + TimeToString(vnTime) + "%0A" +
                     "Đến ngày: " + TimeToString(now + DAYS30) + "%0A" +
                     "Key: " + expectedKey;

        SendTelegram(msg);
        
        return false;
    }

    StartTimeBot = now;
    return true;
}