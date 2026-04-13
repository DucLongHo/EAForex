#include <Trade\Trade.mqh>

CTrade Trade;

datetime CandleCloseTime; 
double BreakevenLock = 100;
datetime LastStopDate = 0; // Ngày dừng bot do drawdown
ulong MagicNumber = 123456; // Mã riêng của EA để phân biệt với lệnh tay

sinput string separator1 = "------------------------------------------"; // === THÔNG SỐ VỊ THẾ ===
input double LotSize = 0.01; // Khối lượng giao dịch cố định
input double StopLossPips = 3000; // Khoảng cách Stop Loss (points)
input double TakeProfitPips = 6000; // Khoảng cách Take Profit (points)

sinput string separator2 = "------------------------------------------"; // === QUẢN LÝ RỦI RO ===
input double TrailingStepPips = 2000;  // Bước giá để tiếp tục dời SL (points)
input double MaxDrawdownAccount = 500; // Mức giảm tối đa của EA (USD)

sinput string separator3 = "------------------------------------------"; // === QUẢN LÝ VẬN HÀNH ===
input int TimeCheckCandleClose = 3; // Thời gian (giây) trước khi đóng nến để kiểm tra và mở lệnh mới 

sinput string separator4 = "------------------------------------------"; // === THỜI GIAN TẠM DỪNG ===
input bool EnablePause = true; // Bật tính năng tạm dừng khi kết thúc ngày
input int PauseBeforeCloseMin = 30; // Đóng lệnh & tắt EA trước khi nến đóng (Phút)
input int ResumeAfterOpenMin = 60;  // Mở lại EA sau khi nến mới mở (Phút)

sinput string separator5 = "------------------------------------------"; // === HỆ THỐNG ĐẢO CHIỀU ===
input bool EnableReverse = true;          // Bật tính năng lệnh đảo chiều
input double ReverseTriggerPoints = 1500; // Số Point âm để kích hoạt mở lệnh đảo chiều (VD: 1500)
input double ReverseLotSize = 0.02;       // Khối lượng lệnh đảo chiều
input double ReverseSLPoints = 3000;      // Stop Loss lệnh đảo chiều (points)
input double ReverseTPPoints = 6000;      // Take Profit lệnh đảo chiều (points)
input double TargetBasketProfit = 5.0;    // Tổng lợi nhuận dương (USD) để chốt đóng toàn bộ lệnh

int OnInit(){
    EventSetTimer(1);
    CandleCloseTime = 0;
    
    // Gán Magic Number cho class CTrade để mỗi lệnh mở ra đều có mã này
    Trade.SetExpertMagicNumber(MagicNumber);
    
    return (INIT_SUCCEEDED);
}

void OnDeinit(const int reason){
    EventKillTimer();
}

// Hàm kiểm tra xem hiện tại có đang nằm trong khung giờ nghỉ hay không
bool IsInPauseTime() {
    if(!EnablePause) return false;
    
    datetime currentTime = TimeCurrent();
    datetime pauseCandleOpenTime = iTime(_Symbol, PERIOD_D1, 0);
    datetime pauseCandleCloseTime = pauseCandleOpenTime + PeriodSeconds(PERIOD_D1);
    
    int secondsSinceOpen = (int)(currentTime - pauseCandleOpenTime);
    int secondsToClose = (int)(pauseCandleCloseTime - currentTime);
    
    // Kích hoạt nghỉ nếu nằm trong 30 phút cuối của nến
    if(secondsToClose <= PauseBeforeCloseMin * 60) return true; 
    // Kích hoạt nghỉ nếu nằm trong 60 phút đầu của nến mới
    if(secondsSinceOpen < ResumeAfterOpenMin * 60) return true; 
    
    return false;
}

void OnTimer(){
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);

    // --- LOGIC XỬ LÝ TẠM DỪNG (ĐÓNG EA) ---
    static bool lastPauseState = false;
    bool isCurrentlyPaused = IsInPauseTime();

    // Nếu vừa chạm mốc thời gian nghỉ -> Đóng toàn bộ lệnh của EA
    if(isCurrentlyPaused && !lastPauseState) {
        Print("Đã đến giờ nghỉ (", PauseBeforeCloseMin, " phút trước khi đóng nến). Đóng toàn bộ lệnh EA!");
        CloseAllOrders();
    } 
    // Nếu vừa qua mốc thời gian nghỉ -> Hoạt động lại
    else if (!isCurrentlyPaused && lastPauseState) {
        Print("Đã hết thời gian nghỉ (", ResumeAfterOpenMin, " phút sau khi nến mở). EA hoạt động trở lại.");
    }
    lastPauseState = isCurrentlyPaused;

    // --- LOGIC VÀO LỆNH (CHỈ CHẠY KHI KHÔNG BỊ TẠM DỪNG) ---
    if(currentCandleCloseTime != CandleCloseTime && (currentCandleCloseTime - currentTime <= TimeCheckCandleClose)){
        CandleCloseTime = currentCandleCloseTime;
        
        if(!isCurrentlyPaused){
            MqlRates rates[];
            ArraySetAsSeries(rates, true);
            if(CopyRates(_Symbol, PERIOD_M5, 0, 2, rates) <= 0) return; 

            MqlRates candle = rates[0];

            if(PositionsTotal() == 0){
                if(LastStopDate == 0 || LastStopDate < iTime(_Symbol, PERIOD_D1, 0)){
                    RunningEA(candle);
                } else {
                    Print("Bot is paused due to drawdown. Will resume tomorrow.");
                }
            }
        }

        Comment("Ngày: ", TimeToString(TimeCurrent(), TIME_DATE),
            "\nSố Lot EA đã chạy hôm nay: ", GetDailyTradedLots());
    }
}

void OnTick(){
    if(PositionsTotal() > 0){
        CheckReverseAndBasketExit(); // Kiểm tra nhồi lệnh đảo chiều & chốt lời tổng trước
        ManagePositions();           // Quản lý SL/TP trailing
        CheckDrawdown();             // Quản lý cắt lỗ toàn tài khoản
    }
}

void RunningEA(MqlRates &candle){
    double iClosePrice = candle.close;
    double iOpenPrice = candle.open;
    
    if(iClosePrice > iOpenPrice){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double sl = NormalizeDouble(entry - StopLossPips * _Point, _Digits);
        double tp = NormalizeDouble(entry + TakeProfitPips * _Point, _Digits);
        
        if(!Trade.Buy(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    } else if(iClosePrice < iOpenPrice){
        double entry = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double sl = NormalizeDouble(entry + StopLossPips * _Point, _Digits);
        double tp = NormalizeDouble(entry - TakeProfitPips * _Point, _Digits);

        if(!Trade.Sell(LotSize, _Symbol, entry, sl, tp)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    }
}

void CheckReverseAndBasketExit() {
    if (!EnableReverse) return;

    int eaPositions = 0;
    double totalProfit = 0.0;
    
    // Đếm số lệnh của EA và tính tổng lợi nhuận (bao gồm cả Swap)
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            if(PositionGetInteger(POSITION_MAGIC) == MagicNumber && PositionGetString(POSITION_SYMBOL) == _Symbol) {
                eaPositions++;
                totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
            }
        }
    }

    // 1. CHỐT LỜI TỔNG (Basket Close) khi có từ 2 lệnh trở lên
    if(eaPositions >= 2) {
        if(totalProfit >= TargetBasketProfit) {
            Print("Đã đạt Target Tổng: ", totalProfit, " USD. Đóng toàn bộ lệnh!");
            CloseAllOrders();
        }
        return; // Đã có lệnh nhồi thì ngưng xét tiếp để tránh mở thêm lệnh liên tục
    }

    // 2. KÍCH HOẠT LỆNH ĐẢO CHIỀU khi chỉ có đúng 1 lệnh đang chạy
    if(eaPositions == 1) {
        for(int i = PositionsTotal() - 1; i >= 0; i--) {
            ulong ticket = PositionGetTicket(i);
            if(PositionSelectByTicket(ticket)) {
                if(PositionGetInteger(POSITION_MAGIC) == MagicNumber && PositionGetString(POSITION_SYMBOL) == _Symbol) {
                    
                    double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);
                    long type = PositionGetInteger(POSITION_TYPE);
                    
                    MqlTick last_tick;
                    if(!SymbolInfoTick(_Symbol, last_tick)) return;

                    double currentProfitPoints = 0;

                    // Lệnh ban đầu là BUY
                    if(type == POSITION_TYPE_BUY) {
                        currentProfitPoints = (last_tick.bid - priceOpen) / _Point; // Lợi nhuận = âm thì giá trị sẽ < 0
                        
                        // Nếu âm vượt mức cho phép -> Mở SELL
                        if(currentProfitPoints <= -ReverseTriggerPoints) {
                            double entry = last_tick.bid;
                            double sl = NormalizeDouble(entry + ReverseSLPoints * _Point, _Digits);
                            double tp = NormalizeDouble(entry - ReverseTPPoints * _Point, _Digits);
                            
                            if(Trade.Sell(ReverseLotSize, _Symbol, entry, sl, tp)) {
                                Print("Đã mở Sell đảo chiều do lệnh Buy âm ", currentProfitPoints, " points.");
                            }
                        }
                    } 
                    // Lệnh ban đầu là SELL
                    else if(type == POSITION_TYPE_SELL) {
                        currentProfitPoints = (priceOpen - last_tick.ask) / _Point; 
                        
                        // Nếu âm vượt mức cho phép -> Mở BUY
                        if(currentProfitPoints <= -ReverseTriggerPoints) {
                            double entry = last_tick.ask;
                            double sl = NormalizeDouble(entry - ReverseSLPoints * _Point, _Digits);
                            double tp = NormalizeDouble(entry + ReverseTPPoints * _Point, _Digits);
                            
                            if(Trade.Buy(ReverseLotSize, _Symbol, entry, sl, tp)) {
                                Print("Đã mở Buy đảo chiều do lệnh Sell âm ", currentProfitPoints, " points.");
                            }
                        }
                    }
                }
            }
        }
    }
}

void ManagePositions(){
    MqlTick last_tick;
    if(!SymbolInfoTick(_Symbol, last_tick)) return;

    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            // CHỈ QUẢN LÝ LỆNH CỦA EA VÀ ĐÚNG CẶP TIỀN
            if(PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;
            if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue; 

            double currentSL = PositionGetDouble(POSITION_SL);
            double currentTP = PositionGetDouble(POSITION_TP);
            double priceOpen = PositionGetDouble(POSITION_PRICE_OPEN);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY){
                double profitPoints = (last_tick.bid - priceOpen) / _Point;
                
                if(profitPoints >= TrailingStepPips){
                    if(currentSL < priceOpen){
                        double newSL = NormalizeDouble(priceOpen + BreakevenLock * _Point, _Digits);
                        Trade.PositionModify(ticket, newSL, currentTP);
                    } else {
                        double newSL = NormalizeDouble(last_tick.bid - TrailingStepPips * _Point, _Digits);
                        if(newSL >= currentSL + TrailingStepPips * _Point){
                            Trade.PositionModify(ticket, newSL, currentTP);
                        }
                    }
                }
            } else if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL){
                double profitPoints = (priceOpen - last_tick.ask) / _Point;
                
                if(profitPoints >= TrailingStepPips){
                    if(currentSL > priceOpen || currentSL == 0.0){
                        double newSL = NormalizeDouble(priceOpen - BreakevenLock * _Point, _Digits);
                        Trade.PositionModify(ticket, newSL, currentTP);
                    } else {
                        double newSL = NormalizeDouble(last_tick.ask + TrailingStepPips * _Point, _Digits);
                        if(currentSL >= newSL + TrailingStepPips * _Point){ 
                            Trade.PositionModify(ticket, newSL, currentTP);
                        }
                    }
                }
            }
        }
    }
}

void CheckDrawdown(){
    double equity = AccountInfoDouble(ACCOUNT_EQUITY);
    double balance = AccountInfoDouble(ACCOUNT_BALANCE);
    double drawdown = balance - equity;

    if(drawdown >= MaxDrawdownAccount){        
        CloseAllOrders(); 
        LastStopDate = iTime(_Symbol, PERIOD_D1, 0); 
    }
}

void CloseAllOrders(){
    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            // CHỈ ĐÓNG LỆNH CỦA EA VÀ ĐÚNG CẶP TIỀN
            if(PositionGetInteger(POSITION_MAGIC) != MagicNumber) continue;
            if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
            
            if(Trade.PositionClose(ticket)){
                Print("Closed EA position #", ticket);
            } else Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }
}

// Hàm tính tổng số Lot EA đã mở trong ngày hiện tại
double GetDailyTradedLots() {
    double totalLots = 0.0;
    
    // Lấy thời điểm bắt đầu của ngày hôm nay
    datetime startOfDay = iTime(_Symbol, PERIOD_D1, 0);
    datetime currentTime = TimeCurrent();
    
    // Yêu cầu tải lịch sử giao dịch từ đầu ngày đến hiện tại
    if(HistorySelect(startOfDay, currentTime)) {
        int dealsTotal = HistoryDealsTotal(); 
        
        for(int i = 0; i < dealsTotal; i++) {
            ulong dealTicket = HistoryDealGetTicket(i);
            
            if(dealTicket > 0) {
                long dealMagic = HistoryDealGetInteger(dealTicket, DEAL_MAGIC);
                string dealSymbol = HistoryDealGetString(dealTicket, DEAL_SYMBOL);
                
                if(dealMagic == MagicNumber && dealSymbol == _Symbol) {
                    long dealEntry = HistoryDealGetInteger(dealTicket, DEAL_ENTRY);
                    if(dealEntry == DEAL_ENTRY_IN) {
                        double volume = HistoryDealGetDouble(dealTicket, DEAL_VOLUME);
                        totalLots += volume;
                    }
                }
            }
        }
    }
    
    return NormalizeDouble(totalLots, 3);
}