#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; 

sinput string separator0 = "------------------------------------------"; // === CÀI ĐẶT BẢN QUYỀN ===
input string Input_LicenseKey = ""; // NHẬP KEY ADMIN CUNG CẤP
sinput string separator1 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double RiskTrade = 100; // Rủi ro cho mỗi lệnh (USD)
ulong  MagicNumber = 123456; // ID định danh của Bot 

//--- CẤU HÌNH GIAO DỊCH ---
double RiskRewardRatio = 1; 
double MinDistanceSL = 2500; 
double RatioSLDistance = 0.5; 

//--- CẤU HÌNH THỜI GIAN SỬ DỤNG BOT ---
const int DAYS30 = 2592000; 

// --- ENUM LOẠI NẾN ---
enum CandleType {
    Bollinger, 
    Normal 
};

// --- CẤU HÌNH BẢO MẬT & TELEGRAM ---
string SecretSalt = "20042000";
string botToken = "8520319257:AAEEh_J2dEUtd-S7CVhf9BIsA_9CFhPu0Kk"; 
string chatId = "8385086008"; 
bool isTelegramSent = false; // Biến chống spam tin nhắn

// --- BIẾN GLOBAL HANDLE CHỈ BÁO ---
int bbHandle = INVALID_HANDLE;

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    // Cài đặt Magic Number cho EA
    Trade.SetExpertMagicNumber(MagicNumber);
    
    // Khởi tạo Handle
    bbHandle  = iBands(_Symbol, PERIOD_M5, 20, 0, 2.0, PRICE_CLOSE);
    
    if(bbHandle == INVALID_HANDLE){
        Print("Lỗi khởi tạo Indicator!");
        return(INIT_FAILED);
    }
    
    EventSetTimer(1);
    return (INIT_SUCCEEDED);
}

void OnDeinit(const int reason){
    EventKillTimer();
    // Giải phóng bộ nhớ chỉ báo
    IndicatorRelease(bbHandle);
}

void OnTimer(){
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M5, 1) + PeriodSeconds(PERIOD_M5);

    if(currentCandleCloseTime != CandleCloseTime && currentCandleCloseTime <= currentTime){
        CandleCloseTime = currentCandleCloseTime;
        
        if (!CheckLicense()){
            ExpertRemove();
            return;        
        }

        RunningEA();
    }
}

void OnTick(){
    if(PositionsTotal() > 0){
        TrailingByProfitUSD();
    }
}

void RunningEA(){
    MqlRates rates[];
    ArraySetAsSeries(rates, true);
    if(CopyRates(_Symbol, PERIOD_M5, 0, 4, rates) <= 0) return; // Chỉ cần lấy 4 nến là đủ
    
    Trading(rates);
}

void Trading(const MqlRates &rates[]){
    MqlRates candle = rates[1], secondCandle = rates[2], thirdCandle = rates[3];
    
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
            BUY(candle); 
        } else if(upperShadow <= 0.15 * (candle.close - candle.open)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            if(!checkBollingerConditions("BUY", candle)){
                BUY(candle); 
            } else {
                BUY(candle, Bollinger); 
            }
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
            SELL(candle); 
        } else if(lowerShadow <= 0.15 * (candle.open - candle.close)
            && (candle.high > secondCandle.high || candle.low < secondCandle.low)
        ){
            if(!checkBollingerConditions("SELL", candle)){
                SELL(candle); 
            } else {
                SELL(candle, Bollinger); 
            }
        }
    }
}

double GetLotSize(double stopLossDistance){
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    if(tickSize == 0 || tickValue == 0) return 0.01;

    double stopLossPips = stopLossDistance / _Point;
    if(stopLossPips == 0) return 0.01;

    double pipValue = tickValue / (tickSize / _Point);
    double lotSize = RiskTrade / (stopLossPips * pipValue);
    
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    lotSize = MathMin(MathMax(lotSize, minLot), maxLot);
    
    double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    return MathFloor(lotSize / lotStep) * lotStep;
}

void TrailingByProfitUSD(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    double trailingStep = 10 * _Point; 

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            if(PositionGetString(POSITION_SYMBOL) != _Symbol || PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;

            double currentSL = PositionGetDouble(POSITION_SL);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
            string comment = PositionGetString(POSITION_COMMENT);
            
            double distanceFromOpen = StringToDouble(comment);
            if(distanceFromOpen <= 0) continue; // Tránh chia 0 hoặc lỗi logic

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                double profit = last_tick.bid - priceOpen;
                double initialSL = priceOpen - distanceFromOpen;

                // Nếu lãi được 0.5R thì BE
                if(profit >= distanceFromOpen/2 && currentSL < priceOpen){
                    double newSL = priceOpen + 50 * _Point;
                    Trade.PositionModify(ticket, newSL);                
                }
                // Nếu lãi được 1R thì chốt một nửa và giữ phần còn lại chạy tiếp
                if(profit >= distanceFromOpen && currentSL > priceOpen){
                    CloseHalfVolume(ticket, currentSL, last_tick.bid);
                }
                // Sau khi chốt một nửa thì bắt đầu trailing stop với khoảng cách 0.5R
                if(profit >= distanceFromOpen && currentSL > priceOpen){
                    double newSL = last_tick.bid - distanceFromOpen/2;
                    if(newSL > currentSL + trailingStep && newSL < last_tick.bid){
                        Trade.PositionModify(ticket, newSL);
                    }
                }
            } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                double profit = priceOpen - last_tick.ask;
                double initialSL = priceOpen + distanceFromOpen;
                // Nếu lãi được 0.5R thì BE
                if(profit >= distanceFromOpen/2 && currentSL > priceOpen){
                    double newSL = priceOpen - 50 * _Point;
                    Trade.PositionModify(ticket, newSL);
                }
                // Nếu lãi được 1R thì chốt một nửa và giữ phần còn lại chạy tiếp
                if(profit >= distanceFromOpen && currentSL < priceOpen){
                    CloseHalfVolume(ticket, currentSL, last_tick.ask);
                }
                // Sau khi chốt một nửa thì bắt đầu trailing stop với khoảng cách 0.5R
                if(profit >= distanceFromOpen && currentSL < priceOpen){
                    double newSL = last_tick.ask + distanceFromOpen/2;
                    if(newSL < currentSL - trailingStep && newSL > last_tick.ask){
                        Trade.PositionModify(ticket, newSL);
                    }
                }
            }
        }
    }
}

void BUY(MqlRates &candle, CandleType candleType = Normal){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double slDistance = (entry - candle.low);
    if(candleType == Normal) slDistance *= RatioSLDistance;

    double sl = entry - slDistance;
    if(!isOpenOrder(entry, sl)) return;

    double lotSize = GetLotSize(MathAbs(entry - sl));
    
    if(CountPositions("BUY") > 1){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2) * lotStep;
    }

    
    if(!Trade.Buy(lotSize, _Symbol, entry, sl, 0, DoubleToString(slDistance))){
        Print("Error placing Buy Order: ", Trade.ResultRetcode());
    }
}

void SELL(MqlRates &candle, CandleType candleType = Normal){
    double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double slDistance = (candle.high - entry);
    if(candleType == Normal) slDistance *= RatioSLDistance;

    double sl = entry + slDistance;
    if(!isOpenOrder(entry, sl)) return;

    double lotSize = GetLotSize(MathAbs(entry - sl));
    
    if(CountPositions("SELL") > 1){
        double lotStep = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
        lotSize = MathFloor((lotSize / lotStep) / 2) * lotStep;
    }
    
    if(!Trade.Sell(lotSize, _Symbol, entry, sl, 0, DoubleToString(slDistance))){
        Print("Error placing Sell Order: ", Trade.ResultRetcode());
    }
}

bool isOpenOrder(double entry, double sl){
    return (MathAbs(entry - sl) >= MinDistanceSL * _Point);
}

int CountPositions(string type) {
    int count = 0;
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            if(PositionGetString(POSITION_SYMBOL) != _Symbol || PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;

            if(type == "BUY" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) count++;
            else if(type == "SELL" && PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) count++;
        }
    }
    return count;
}

bool checkBollingerConditions(string trend, MqlRates &candle){
    double upperBand[], lowerBand[], middleBand[];
    ArraySetAsSeries(upperBand, true);
    ArraySetAsSeries(lowerBand, true);
    ArraySetAsSeries(middleBand, true);

    if(CopyBuffer(bbHandle, 1, 0, 1, upperBand) <= 0) return false;
    if(CopyBuffer(bbHandle, 2, 0, 1, lowerBand) <= 0) return false;
    if(CopyBuffer(bbHandle, 0, 0, 1, middleBand) <= 0) return false;

    if(trend == "BUY" && candle.low < lowerBand[0] && candle.high < middleBand[0]) return true;
    if(trend == "SELL" && candle.high > upperBand[0] && candle.low > middleBand[0]) return true;

    return false;
}

void SendTelegram(string text) {
    string url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + text;
    char post[], result[];
    string headers;
    WebRequest("GET", url, headers, 1000, post, result, headers);
}

string HashEngine(string data) {
    uint hash = 5381;
    for(int i = 0; i < StringLen(data); i++)
        hash = ((hash << 5) + hash) + StringGetCharacter(data, i);
    return StringFormat("%08X", hash);
}

bool CheckLicense() {
    long accID = AccountInfoInteger(ACCOUNT_LOGIN);
    datetime now = TimeCurrent();
    MqlDateTime mqlNow;
    TimeToStruct(now, mqlNow); 

    string timeData = IntegerToString(accID) + SecretSalt + 
                      IntegerToString(mqlNow.mon) + 
                      IntegerToString(mqlNow.year);
    string expectedKey = HashEngine(timeData);

    if (Input_LicenseKey != expectedKey) {
        Alert("Mã Key không đúng! Vui lòng liên hệ Admin. SĐT/Zalo: 0866797299");
        
        if(!isTelegramSent){
            string msg = "🔑 YEU CAU KEY MOI!%0A" + 
                         "ID: " + (string)accID + "%0A" +
                         "Key: " + expectedKey;
            SendTelegram(msg);
            isTelegramSent = true; 
        }
        return false;
    }

    if (!GlobalVariableCheck(IntegerToString(accID))) {
        GlobalVariableSet(IntegerToString(accID), (double)now);
        return true;
    }

    datetime activationDate = (datetime)GlobalVariableGet(IntegerToString(accID));
    if(activationDate + DAYS30 < TimeCurrent()) {
        Alert("Thời gian sử dụng bot đã hết hạn! Vui lòng liên hệ SĐT/Zalo: 0866797299");
        GlobalVariableDel(IntegerToString(accID));
        return false;
    }

    return true;
}

void CloseHalfVolume(long ticket, double stoploss, double price) {
    if(PositionSelectByTicket(ticket)){
        double currentVolume = PositionGetDouble(POSITION_VOLUME);
        
        double halfVolume = NormalizeDouble(currentVolume / 2.0, 2);
        
        MqlTradeRequest request = {};
        MqlTradeResult result = {};
            
        request.action = TRADE_ACTION_DEAL;
        request.symbol = _Symbol;
        request.volume = halfVolume;
        request.type = (PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) ? ORDER_TYPE_SELL : ORDER_TYPE_BUY;
        request.price = price;
        request.position = ticket;
        request.deviation = 10;
        
        if(!OrderSend(request, result)) {
            Print("Failed to close half position. Error: ", GetLastError());
        } else if(!Trade.PositionModify(ticket, stoploss)) {
                Print("Failed to modify position #", ticket, ". Error: ", GetLastError());
        }
    }
}

bool IsPartiallyClosed(ulong position_id) {
    if(HistorySelectByPosition(position_id)) {
        int deals = HistoryDealsTotal();
        for(int i = 0; i < deals; i++) {
            ulong deal_ticket = HistoryDealGetTicket(i);
            long entry_type = HistoryDealGetInteger(deal_ticket, DEAL_ENTRY);
            
            if(entry_type == DEAL_ENTRY_OUT || entry_type == DEAL_ENTRY_INOUT) {
                return true;
            }
        }
    }
    return false;
}