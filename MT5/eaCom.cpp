#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;
CChartObjectLabel lblTotalBuyLot, lblTotalSellLot, lblTotalLots;

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

// Input parameters
input double LotSize = 0.01; // Khối lượng từng lệnh
input double TakeProfitUSD = 1.5; // Mức lợi nhuận mục tiêu để đóng lệnh (đơn vị: USD)
input double MaxDrawdownUSD = 50.0; // Mức thua lỗ tối đa để đóng lệnh (đơn vị: USD)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(1);
    
    int x = (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
    int y =  (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 50;

    if(!CreateLable(lblTotalBuyLot, "TotalBuyLot", "Total Buy Lots: 0.00", x, 30))
      return(INIT_FAILED);

    if(!CreateLable(lblTotalSellLot, "TotalSellLot", "Total Sell Lots: 0.00", x, 60))
      return(INIT_FAILED);
    
    if(!CreateLable(lblTotalLots, "TotalLots", "Total Lots: 0.00", x, 90))
      return(INIT_FAILED);

    ChartRedraw(0);
    
    return (INIT_SUCCEEDED);
}

void OnTimer(){
    // Check current time and next M1 candle close time
    datetime currentTime = TimeCurrent();
    datetime currentCandleCloseTime = iTime(_Symbol, PERIOD_M1, 1) + PeriodSeconds(PERIOD_M1);

    if(currentCandleCloseTime != CandleCloseTime && currentCandleCloseTime <= currentTime){
        CandleCloseTime = currentCandleCloseTime;

        TradeCom();

        lblTotalLots.Description("Total Lots: "  + DoubleToString(GetDailyTradedLots(), 2));
    }
    
    CalculateLable();

}

void OnTick(){
    if(GetTotalBuyProfit() + GetTotalSellProfit() >= TakeProfitUSD){
        CloseAllPositions();
    }

    if(GetTotalBuyProfit() + GetTotalSellProfit() <= -MaxDrawdownUSD){
        CloseAllPositions();
    }

    HedgePositions();
}

void OnDeinit(const int reason){
    EventKillTimer();
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

void TradeCom(){
    if(PositionsTotal() == 0){
        double preCandleOpen = iOpen(_Symbol, PERIOD_M1, 1);
        double preCandleClose = iClose(_Symbol, PERIOD_M1, 1);
        
        if(preCandleClose < preCandleOpen) {
            if(!Trade.Sell(LotSize, _Symbol)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }
        } else {
            if(!Trade.Buy(LotSize, _Symbol)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }
        }
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


double GetTotalBuyProfit() {
    double totalProfit = 0;
    
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)) {
            // Kiểm tra nếu là lệnh BUY
            if((ENUM_POSITION_TYPE)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) {
                totalProfit += PositionGetDouble(POSITION_PROFIT);
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
                totalProfit += PositionGetDouble(POSITION_PROFIT);
            }
        }
    }
    return totalProfit;
}

void CloseAllPositions(){
    Trade.SetAsyncMode(true);

    for(int index = PositionsTotal() - 1; index >= 0 && !IsStopped(); index--){
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
        if(PositionSelectByTicket(ticket)){
            totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) 
                buyLots += PositionGetDouble(POSITION_VOLUME);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) 
                sellLots += PositionGetDouble(POSITION_VOLUME);
        }
    }


    if(MathAbs(sellLots - buyLots) == LotSize){
        ulong ticket = PositionGetTicket(PositionsTotal() - 1);

        if(PositionSelectByTicket(ticket)){
            if(PositionGetDouble(POSITION_PROFIT) <= -3){
                ExecuteHedge(buyLots, sellLots);
            }    
        }
    }
}

void CalculateLable(){
    double totalBuyLots = 0;
    double totalSellLots = 0;
    for(int i = PositionsTotal() - 1; i >= 0; i--) {
        ulong ticket = PositionGetTicket(i);
        if(PositionSelectByTicket(ticket)){
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) 
                totalBuyLots += PositionGetDouble(POSITION_VOLUME);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) 
                totalSellLots += PositionGetDouble(POSITION_VOLUME);
        }
    }
    lblTotalBuyLot.Description("Total Buy Lots: " + DoubleToString(totalBuyLots, 2));
    lblTotalSellLot.Description("Total Sell Lots: " + DoubleToString(totalSellLots, 2));
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
                string dealSymbol = HistoryDealGetString(dealTicket, DEAL_SYMBOL);
                
                if(dealSymbol == _Symbol) {
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