#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;


CChartObjectLabel lblTotalBuyProfit, lblTotalSellProfit;

// Input parameters

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

input double LotSize = 0.01; // Khối lượng từng lệnh

int ProfitHedge = -5; // Mức chênh lệch lợi nhuận để thực hiện vào lệnh cân bằng (đơn vị: USD)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    // Tạo label
    if(!CreateLable(lblTotalBuyProfit, "lblTotalBuyProfit", "Total BUY: 0.00 USD", 30))
        return(INIT_FAILED);

    if(!CreateLable(lblTotalSellProfit, "lblTotalSellProfit", "Total TP: 0.00 USD", 60))
        return(INIT_FAILED);
    
    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, ONE) + PeriodSeconds(PERIOD_M1);

    bool isRunningEa = false;
    if(currentCandleCloseTime != CandleCloseTime &&
        currentCandleCloseTime <= currentTime){
        CandleCloseTime = currentCandleCloseTime;
        isRunningEa = true;

        TradeCom();
    }

    if(isRunningEa) isRunningEa = false;

    // Cập nhật lên Panel (Giả định bạn đã đổi tên label)
    lblTotalBuyProfit.Description("Buy Profit: " + DoubleToString(GetTotalBuyProfit(), 2) + " USD");
    lblTotalSellProfit.Description("Sell Profit: " + DoubleToString(GetTotalSellProfit(), 2) + " USD");
}

void OnTick(){
    if(GetTotalBuyProfit() + GetTotalSellProfit() >= 1){
        CloseAllPositions();
    }
    HedgePositions();
}

void OnChartEvent(const int id, const long &lparam, const double &dparam, const string &sparam){

}

void OnDeinit(const int reason){
    EventKillTimer();
}

void TradeCom(){
    if(PositionsTotal() == 0){
        Trade.SetAsyncMode(true);

        if(!Trade.Sell(LotSize, _Symbol)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }

        if(!Trade.Buy(LotSize, _Symbol)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }

        Trade.SetAsyncMode(false);
    } else {
        if(GetTotalBuyProfit() >= GetTotalSellProfit()){
            if(!Trade.Buy(LotSize, _Symbol)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        } else { 
            if(!Trade.Sell(LotSize, _Symbol)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
        }     
    }
}

bool CreateLable(CChartObjectLabel &lable, string name, string des, int y){
    int xPos = (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
    // Tạo lable và thiết lập thuộc tính
    if(!lable.Create(0, name, 0, xPos, y))
        return false;

    lable.Description(des);
    lable.Color(clrWhite);
    lable.Font("Calibri");
    lable.FontSize(12);

    return true;
}


double GetTotalBuyProfit() {
    double totalProfit = 0;
    
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            // Kiểm tra nếu là lệnh BUY
            if((ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) {
                double profit = PositionGetDouble(POSITION_PROFIT);
                double swap = PositionGetDouble(POSITION_SWAP);
                
                totalProfit += (profit + swap);
            }
        }
    }
    return totalProfit;
}

double GetTotalSellProfit() {
    double totalProfit = 0;
    
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            // Kiểm tra nếu là lệnh SELL
            if((ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) {
                double profit = PositionGetDouble(POSITION_PROFIT);
                double swap = PositionGetDouble(POSITION_SWAP);
                
                totalProfit += (profit + swap);
            }
        }
    }
    return totalProfit;
}

void CloseAllPositions(){
    Trade.SetAsyncMode(true);

    for(int index = PositionsTotal() - ONE; index >= 0 && !IsStopped(); index--){
        ulong ticket = PositionGetTicket(index);
        if(ticket <= 0) continue;
        
        if(PositionSelectByTicket(ticket)){
            if(!Trade.PositionClose(ticket))
                Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
        }
    }

    Trade.SetAsyncMode(false);
}

// Hàm phụ trợ để thực hiện vào lệnh cân bằng khối lượng
void ExecuteHedge(double bLots, double sLots) {
    double diff = (bLots - sLots);
    
    if(NormalizeDouble(diff, 2) > 0) {
        if(!Trade.Sell(MathAbs(diff), _Symbol)){
            Print("Error placing Sell Order: ", Trade.ResultRetcode());
        }
    } 
    else if(NormalizeDouble(diff, 2) < 0) {
        if(!Trade.Buy(MathAbs(diff), _Symbol)){
            Print("Error placing Buy Order: ", Trade.ResultRetcode());
        }
    }
}

void HedgePositions() {
    double totalProfit = 0;
    double buyLots = 0;
    double sellLots = 0;

    // 1. Tính toán trạng thái hiện tại
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) 
                buyLots += PositionGetDouble(POSITION_VOLUME);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) 
                sellLots += PositionGetDouble(POSITION_VOLUME);
        }
    }

    // Reset cờ nếu tài khoản dương trở lại hoặc hết lệnh (để chuẩn bị cho chu kỳ mới)
    if(PositionsTotal() == 0){
        ProfitHedge = -5;
    }

    if(totalProfit <= ProfitHedge){
        ProfitHedge -= 10; // Giảm thêm 5 USD cho lần tiếp theo nếu vẫn chưa cân bằng được
        ExecuteHedge(buyLots, sellLots);
    }
}