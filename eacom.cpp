#include <Trade\Trade.mqh>
#include <ChartObjects\ChartObjectsTxtControls.mqh>

CTrade Trade;

CChartObjectButton OnOffButton;// Nút bật tắt EA

// Constant data
const int ZERO = 0;
const int ONE = 1;
const int TWO = 2;

const string BUY = "BUY";
const string SELL = "SELL";

datetime CandleCloseTime; // Biến kiểm tra giá chạy 1p một lần 

bool OnOffEnabled = false; // Biến kiểm soát bật tắt EA

// Input parameters
input double LotSize = 0.01; // Khối lượng từng lệnh
input double TrailingStartProfit = 5.0;  // Có lời 5 USD thì bắt đầu kích hoạt trailing stop
input int TrailingStepPips = 50;   // Khoảng cách duy trì (ví dụ 50 pips)

int ProfitHedge = -20; // Mức chênh lệch lợi nhuận để thực hiện vào lệnh cân bằng (đơn vị: USD)
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    EventSetTimer(ONE);

    if(!CreateButton(OnOffButton, "OnOffButton", "OFF", clrRed, CalculateButtonY() - 30))
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
    // Nhấn nút đóng EA
    if(id == CHARTEVENT_OBJECT_CLICK && sparam == "OnOffButton"){
        OnOffEnabled = !OnOffEnabled;
        
        if(OnOffEnabled){
            OnOffButton.Description("ON");
            OnOffButton.Color(clrWhite);
            OnOffButton.BackColor(clrGreen);
        } else {
            OnOffButton.Description("OFF");
            OnOffButton.Color(clrWhite);
            OnOffButton.BackColor(clrRed);
        }
    }
}

void OnDeinit(const int reason){
    OnOffButton.Delete();
    EventKillTimer();
}

void TradeCom(){
    if(PositionsTotal() == 0){
        
        if(OnOffEnabled){
            Trade.SetAsyncMode(true);

            if(!Trade.Sell(LotSize, _Symbol)){
                Print("Error placing Sell Order: ", Trade.ResultRetcode());
            }

            if(!Trade.Buy(LotSize, _Symbol)){
                Print("Error placing Buy Order: ", Trade.ResultRetcode());
            }

            Trade.SetAsyncMode(false);
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

int CalculateButtonX(){
    return (int)ChartGetInteger(0, CHART_WIDTH_IN_PIXELS) - 200;
}

int CalculateButtonY(){
    return (int)ChartGetInteger(0, CHART_HEIGHT_IN_PIXELS) - 50;
}

bool CreateButton(CChartObjectButton &button, string name, string des, color bgColor, int y){
    // Tạo nút và thiết lập thuộc tính
    if(!button.Create(0, name, 0, CalculateButtonX(), y, 175, 35))
        return false;
    
    button.Description(des);
    button.Color(clrWhite);
    button.BackColor(bgColor); 
    button.FontSize(12);
    button.Font("Calibri");
    button.Selectable(true);
    
    return true;
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
        if(PositionSelectByTicket(ticket)){
            if(PositionGetDouble(POSITION_PROFIT) > 5 &&  PositionGetDouble(POSITION_VOLUME) > 0.01){
                if(!Trade.PositionClose(ticket))
                    Print("Close failed #", ticket, " - Error: ", Trade.ResultComment());
                    
                continue;
            }
            totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) 
                buyLots += PositionGetDouble(POSITION_VOLUME);

            if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL) 
                sellLots += PositionGetDouble(POSITION_VOLUME);
        }
    }

    // Reset cờ nếu tài khoản dương trở lại hoặc hết lệnh (để chuẩn bị cho chu kỳ mới)
    if(PositionsTotal() == 0){
        ProfitHedge = -20;
    }

    if(totalProfit <= ProfitHedge){
        ProfitHedge -= 20; // Giảm thêm 5 USD cho lần tiếp theo nếu vẫn chưa cân bằng được
        ExecuteHedge(buyLots, sellLots);
    }
}

void TrailingByProfitUSD(){
    // Duyệt qua tất cả các lệnh đang mở trên tài khoản
    for(int i = PositionsTotal() - 1; i >= 0; i--){
        ulong ticket = PositionGetTicket(i);

        if(PositionSelectByTicket(ticket)){
            // Kiểm tra xem có đúng ký hiệu (Symbol) của biểu đồ hiện tại không
            if(PositionGetString(POSITION_SYMBOL) == _Symbol){
                double profit = PositionGetDouble(POSITION_PROFIT); // Lấy lợi nhuận hiện tại (USD)
                double currentSL = PositionGetDouble(POSITION_SL);
                double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);
                double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

                // 1. Nếu là lệnh BUY và lợi nhuận > 5 USD
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY && profit >= TrailingStartProfit){
                    double newSL = bid - (TrailingStepPips * _Point);
                    // Chỉ dời SL lên cao hơn mức SL cũ
                    if(newSL > currentSL || currentSL == 0){
                        Trade.PositionModify(ticket, newSL, 0);
                    }
                }

                // 2. Nếu là lệnh SELL và lợi nhuận > 5 USD
                if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL && profit >= TrailingStartProfit){
                    double newSL = ask + (TrailingStepPips * point);
                    // Chỉ dời SL xuống thấp hơn mức SL cũ
                    if(newSL < currentSL || currentSL == 0){
                        Trade.PositionModify(ticket, newSL, 0);
                    }
                }
            }
        }
    }
}