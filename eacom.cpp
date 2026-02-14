#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;


CChartObjectLabel lblTotalSL, lblTotalTP;

// Input parameters

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 


//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    // Tạo label
    if(!CreateLable(lblTotalSL, "TotalOrderBuy", "Total BUY: 0.00 USD", 8, 30))
        return(INIT_FAILED);

    if(!CreateLable(lblTotalTP, "TotalOrderSell", "Total TP: 0.00 USD", 8, 60))
        return(INIT_FAILED);
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
        
        isRunningEa = false;
    }

    CalculateLable();
}

void OnTick(){

}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){

}

void OnDeinit(const int reason){
    EventKillTimer();
}

bool CreateLable(CChartObjectLabel &lable, string name, string des, int x, int y){
    // Tạo lable và thiết lập thuộc tính
    if(!lable.Create(0, name, 0, CalculateButtonX(), y))
        return false;

    lable.Description(des);
    lable.Color(clrWhite);
    lable.Font("Calibri");
    lable.FontSize(12);

    return true;
}

void CalculateLable(){
    double totalSl = 0;
    double totalTp = 0;
    
    double tickValue = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_VALUE);
    double tickSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_TICK_SIZE);
    double pointValue = tickValue / tickSize;
    
    // Duyệt qua tất cả các vị thế hiện có
    for(int index = PositionsTotal() - ONE; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);

        if(PositionSelectByTicket(ticket)){
            double sl = PositionGetDouble(POSITION_SL);
            double tp = PositionGetDouble(POSITION_TP);

            double price = PositionGetDouble(POSITION_PRICE_OPEN);
            double volume = PositionGetDouble(POSITION_VOLUME);
            ENUM_POSITION_TYPE type = (ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE);
            
            if(sl != 0){
                // Tính khoản lỗ tiềm năng (SL - Price) * Volume
                double diff = 0;
                
                // Tính chênh lệch SL theo hướng lệnh
                if(type == POSITION_TYPE_BUY){
                    diff = sl - price; // BUY: SL > Price = lỗ
                } else diff = price - sl; // SELL: SL < Price = lỗ

                totalSl += diff * volume * pointValue * 100;
            }

            if(tp != 0){
                // Tính khoản lời tiềm năng (TP - Price) * Volume
                double diff = 0;
                
                // Tính chênh lệch TP theo hướng lệnh
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


void CloseAllPositions(){
    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            Trade.SetAsyncMode(true);
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }

    Trade.SetAsyncMode(false);
}