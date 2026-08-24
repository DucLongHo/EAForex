#property copyright "Homie Bot Candle v1.0"
#property version   "1.00"
#property strict

#include <Trade\Trade.mqh>
#include <Trade\PositionInfo.mqh>

CTrade    Trade;

enum ENUM_DIRECTION    { DIR_BOTH, DIR_ONLY_BUY, DIR_ONLY_SELL };

input group         "══════ CÀI ĐẶT CƠ BẢN ══════"; //
input  bool    InpBotEnabled   = true;    // Bật Bot (tắt = đóng toàn bộ lệnh + dừng mọi hoạt động)
input  double  InpLotSize      = 0.01;    // Lots ban đầu
input  int     InpOrderDelay   = 5;       // Độ trễ mở lệnh (giây)
input  ulong   InpMagic        = 202601;  // Magic Number

input group         "══════ TÍN HIỆU VÀO LỆNH ══════"; //
input  ENUM_DIRECTION   InpDirection  = DIR_BOTH;     // Hướng giao dịch
input  ENUM_TIMEFRAMES  InpSignalTF   = PERIOD_H1;    // Khung thời gian đọc màu nến tín hiệu

input group         "══════ LỆNH TAY - AUTO SL (EMA) ══════"; //
input  int     InpManualSLEMAPeriod = 25;    // EMA lọc: giá dưới/trên EMA (period)
input  double  InpManualSL_Points   = 0.0;   // SL tự động cho lệnh tay (points, 0=tắt)
input  double  InpManualLotSize     = 0.01;  // Lots cho nút Buy/Sell Tay trên panel

input group         "══════ HEDGE & PAUSE ══════"; //
input  bool    InpDDPauseEnable    = false; // Bật Hedge & Pause khi DD đạt ngưỡng
input  double  InpDDPauseThreshold = -50.0; // Ngưỡng DD ($, số âm cố định) để cân lệnh + tạm dừng EA

input group         "══════ DCA - CÀI ĐẶT CHUNG ══════"; //
input  bool          InpDCAArithEnable = false; // DCA: Bật Vol Cấp Số Cộng (bỏ qua Hệ số Lot)
input  double        InpDCAArithStep   = 0.01;  // DCA: Cộng thêm Vol mỗi lệnh DCA sau (lots)
input  int           InpMaxPendingDCA  = 20;    // DCA: Trần lệnh chờ sống cùng lúc mỗi chiều (0=không giới hạn)

input group         "══════ DCA ══════"; //
input  double  InpDCA1Mult = 1.5;    // DCA: Hệ số Lot
input  int     InpDCA1Max  = 2;      // DCA: Max lệnh DCA tối đa
input  double  InpDCA1Dist = 1000.0; // DCA: Khoảng cách (points)
input  double  InpDCA1TP   = 500.0;  // DCA: TP (points)
input  double  InpDCA1SL   = 0.0;    // DCA: SL (points, 0=tắt)

input group         "══════ TRAILING STOP ══════"; //
input  bool          InpTrailEnable   = false;        // Bật Trailing
input  int           InpTrailMinOrds  = 1;            // Số lệnh tối thiểu kích hoạt
input  double        InpTrailActivate = 500.0;        // Points kích hoạt Trail
input  double        InpTrailStep     = 200.0;        // Bước nhảy SL (points)
input  double        InpTrailInit     = 300.0;        // SL đầu tiên cách giá (points)

input group         "══════ LỆNH TAY - TRAILING RIÊNG ══════"; //
input  bool          InpManualTrailEnable   = false;        // Bật Trailing riêng cho lệnh tay
input  int           InpManualTrailMinOrds  = 1;            // Số lệnh tay tối thiểu kích hoạt
input  double        InpManualTrailActivate = 500.0;        // Points kích hoạt Trail
input  double        InpManualTrailStep     = 200.0;        // Bước nhảy SL (points)
input  double        InpManualTrailInit     = 300.0;        // SL đầu tiên cách giá (points)

input group         "══════ ĐÓNG LỆNH TỔNG ══════"; //
input  double  InpCloseProfit  = 0.0;  // Chốt lời khi tổng lãi đạt ($, 0=tắt)
input  double  InpCloseLoss    = 0.0;  // Cắt lỗ khi tổng lỗ đạt ($, 0=tắt)
input  double  InpClosePerPips = 0.0;  // Đóng từng lệnh khi đạt (points, 0=tắt)
input  double  InpDayMaxLoss   = 0.0;  // Dừng bot khi lỗ ngày đạt ($, 0=tắt)
input  double  InpDayMaxProfit = 0.0;  // Dừng bot khi lãi ngày đạt ($, 0=tắt)

input group         "══════ PANEL ══════"; //
input  bool    InpShowPanel  = true;  // Hiện panel
input  int     InpPanelX     = 5;     // Panel: tọa độ X
input  int     InpPanelY     = 18;    // Panel: tọa độ Y
input  int     InpPanelWidth = 252;   // Panel: chiều rộng
input  int     InpCalPanelGap = 12;   // Lịch: khoảng cách với panel chính
input  int     InpCalPanelY  = 18;    // Lịch: tọa độ Y

input group         "══════ QUẢN LÝ LỊCH ══════"; //
input  bool    InpCalShowProfitDays = true;  // Hiện ngày có lãi
input  bool    InpCalShowLossDays  = true;   // Hiện ngày lỗ

double        DCA_Mult;
int           DCA_MaxOrd;
double        DCA_Dist;
double        DCA_TP;
double        DCA_SL;

datetime LastOrderTime  = 0;
datetime LastEntryTime  = 0;
double   InitBalance    = 0.0;
double   DayProfit      = 0.0;
int      LastDay        = -1;
bool     DayLimitHit    = false;

double   OrigBuyPrice   = 0.0;
double   OrigSellPrice  = 0.0;

int      PeakDCABuy  = 0;
int      PeakDCASell = 0;

double   DCABuyPrices[];
double   DCASellPrices[];
bool     DCABuyBounced[];
bool     DCASellBounced[];
ulong    DCABuyTickets[];
ulong    DCASellTickets[];
ulong    DCABuyLimitTk[];
ulong    DCASellLimitTk[];

bool g_CalExpanded = false;
bool g_PanelCollapsed = false;
int  g_LastPanelBottom = 0;
int  g_CalYear     = 0;
int  g_CalMonth    = 0;

int    g_CalCacheYear  = 0;
int    g_CalCacheMonth = 0;
bool   g_CalCacheDone[42];
double g_CalCacheProfit[42];
double g_CalCacheLot[42];
datetime g_CalLastTodayCalc = 0;
#define RTB_CAL_TODAY_THROTTLE_SEC 3
#define RTB_TITLEBAR_H 36

const string GUI = "RTB_";
const string HEDGE_ALL_CMT = "HEDGE ALL";

bool     g_BotEnabled           = true;
datetime g_LastBotToggleClick   = 0;

ENUM_DIRECTION   g_Direction;

double g_ManualSL_Points;
bool   g_ManualAutoSLEnable = true;   // bật/tắt Auto SL (EMA) cho lệnh tay qua nút panel
int    hManualSLEMA = INVALID_HANDLE;
ulong  g_ManualSLSeen[];   // ticket lệnh tay đã được gán SL (EMA) 1 lần lúc mới mở, không xét lại (nhường quyền cho Trailing)

bool   g_CloseIncludeManual = false; // bật/tắt tính lệnh tay vào Float khi xét Đóng Lệnh Tổng (InpCloseProfit/InpCloseLoss)

int    g_OrderDelay;

bool   g_DCAArithEnable;
double g_DCAArithStep;

bool             g_TrailEnable;
int              g_TrailMinOrds;
double           g_TrailActivate, g_TrailStep, g_TrailInit;

bool             g_ManualTrailEnable;
int              g_ManualTrailMinOrds;
double           g_ManualTrailActivate, g_ManualTrailStep, g_ManualTrailInit;

double g_CloseProfit, g_CloseLoss, g_ClosePerPips, g_DayMaxLoss, g_DayMaxProfit;

bool   g_DDPauseEnable;
double g_DDPauseThreshold;
bool   g_PauseEA = false;


bool IsManaged() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    return magic == (long)InpMagic;
}

bool IsManagedForDisplay() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    return magic == (long)InpMagic || magic == 0;
}

bool IsManagedForTrail() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    return PositionGetInteger(POSITION_MAGIC) == (long)InpMagic;
}

bool IsManualManagedForTrail() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    if(PositionGetInteger(POSITION_MAGIC) != 0) return false;
    if(PositionGetString(POSITION_COMMENT) == HEDGE_ALL_CMT) return false;
    return true;
}

int CountPos(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) == posType) n++;
    }
    return n;
}

int CountBuy()  { return CountPos(POSITION_TYPE_BUY);  }
int CountSell() { return CountPos(POSITION_TYPE_SELL); }

int CountAll()  { return CountBuy() + CountSell(); }

double FloatProfit(int posType = -1) {
    double p = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if(posType >= 0 && (int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        p += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
    }
    return p;
}

double TotalLot(int posType) {
    double lot = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        lot += PositionGetDouble(POSITION_VOLUME);
    }
    return lot;
}

double LastOpenPrice(int posType) {
    double   price = 0;
    datetime latest = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t > latest) { latest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    return price;
}

double AvgOpenPrice(int posType) {
    double totalLot = 0, totalCost = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        double lot   = PositionGetDouble(POSITION_VOLUME);
        double price = PositionGetDouble(POSITION_PRICE_OPEN);
        totalLot  += lot;
        totalCost += lot * price;
    }
    return (totalLot > 0) ? totalCost / totalLot : 0;
}

int CountPosForTrail(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForTrail()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) == posType) n++;
    }
    return n;
}

int CountManualForTrail(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManualManagedForTrail()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) == posType) n++;
    }
    return n;
}

double ManualFloatProfit(int posType = -1) {
    double p = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManualManagedForTrail()) continue;
        if(posType >= 0 && (int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        p += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
    }
    return p;
}

ulong BestTicket() {
    ulong  tk_best = 0;
    double best    = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        double p = PositionGetDouble(POSITION_PROFIT);
        if(p > best) { best = p; tk_best = tk; }
    }
    return tk_best;
}

ulong WorstTicketByPoints() {
    ulong  tk_worst  = 0;
    double worstPts  = 0;
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        int    pt  = (int)PositionGetInteger(POSITION_TYPE);
        double opn = PositionGetDouble(POSITION_PRICE_OPEN);
        double pts = (pt == POSITION_TYPE_BUY) ? (bid - opn) / point : (opn - ask) / point;
        if(pts < worstPts) { worstPts = pts; tk_worst = tk; }
    }
    return tk_worst;
}

void CloseAll(int posType = -1) {
    ulong tickets[];
    int   count = 0;
    ArrayResize(tickets, PositionsTotal());
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if(posType >= 0 && (int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        tickets[count++] = tk;
    }
    Trade.SetAsyncMode(true);
    for(int i = 0; i < count; i++)
        Trade.PositionClose(tickets[i]);
    Trade.SetAsyncMode(false);

    for(int i = OrdersTotal()-1; i >= 0; i--) {
        ulong otk = OrderGetTicket(i);
        if(otk == 0 || !OrderSelect(otk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        long magic = OrderGetInteger(ORDER_MAGIC);
        if(magic != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        bool isBuyType  = (ot == ORDER_TYPE_BUY_LIMIT  || ot == ORDER_TYPE_BUY_STOP);
        bool isSellType = (ot == ORDER_TYPE_SELL_LIMIT || ot == ORDER_TYPE_SELL_STOP);
        if(!isBuyType && !isSellType) continue;
        if(posType == POSITION_TYPE_BUY  && !isBuyType)  continue;
        if(posType == POSITION_TYPE_SELL && !isSellType) continue;
        Trade.OrderDelete(otk);
    }

    if(posType < 0 || posType == POSITION_TYPE_BUY) {
        PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
        OrigBuyPrice = 0;
        ClearSlotPrices(POSITION_TYPE_BUY, ArraySize(DCABuyPrices));
        SavePeak(POSITION_TYPE_BUY, 0);
    }
    if(posType < 0 || posType == POSITION_TYPE_SELL) {
        PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
        OrigSellPrice = 0;
        ClearSlotPrices(POSITION_TYPE_SELL, ArraySize(DCASellPrices));
        SavePeak(POSITION_TYPE_SELL, 0);
    }
}

void CloseAllManual() {
    ulong tickets[];
    int   count = 0;
    ArrayResize(tickets, PositionsTotal());
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        tickets[count++] = tk;
    }
    if(count > 0) {
        Trade.SetAsyncMode(true);
        for(int i = 0; i < count; i++)
            Trade.PositionClose(tickets[i]);
        Trade.SetAsyncMode(false);

        Print("RTB: Đóng toàn bộ lệnh tay (", count, " lệnh).");
    }

    for(int i = OrdersTotal()-1; i >= 0; i--) {
        ulong otk = OrderGetTicket(i);
        if(otk == 0 || !OrderSelect(otk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if(OrderGetInteger(ORDER_MAGIC) != 0) continue;
        Trade.OrderDelete(otk);
    }
}

void BalanceHedgeAccount() {
    double buyLot = 0, sellLot = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        double lot = PositionGetDouble(POSITION_VOLUME);
        if((int)PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY) buyLot += lot;
        else sellLot += lot;
    }

    double netLot = buyLot - sellLot;
    double minLot = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    if(MathAbs(netLot) < minLot) {
        Print("RTB: Cân lệnh — tài khoản đã cân bằng (buy=", buyLot, " sell=", sellLot, "), không cần hedge.");
        return;
    }

    double hedgeLot = NormLot(MathAbs(netLot));
    Trade.SetExpertMagicNumber(0);
    bool ok;
    if(netLot > 0)
        ok = Trade.Sell(hedgeLot, _Symbol, SymbolInfoDouble(_Symbol, SYMBOL_BID), 0, 0, HEDGE_ALL_CMT);
    else
        ok = Trade.Buy(hedgeLot, _Symbol, SymbolInfoDouble(_Symbol, SYMBOL_ASK), 0, 0, HEDGE_ALL_CMT);
    Trade.SetExpertMagicNumber(InpMagic);

    if(ok)
        Print("RTB: Cân lệnh — mở lệnh Hedge ", (netLot > 0 ? "SELL" : "BUY"), " lot=", hedgeLot,
              " để cân bằng toàn tài khoản (buy=", buyLot, " sell=", sellLot, ")");
    else
        Print("RTB: Cân lệnh thất bại, err=", GetLastError());
}

void CancelAllPendingOrders() {
    int cancelled = 0, failed = 0;
    for(int i = OrdersTotal() - 1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        if(Trade.OrderDelete(tk)) {
            cancelled++;
        } else {
            failed++;
            Print("RTB: Pause — huỷ lệnh chờ ticket=", tk, " thất bại, err=", GetLastError());
        }
    }
    if(cancelled > 0 || failed > 0)
        Print("RTB: Pause — đã huỷ ", cancelled, " lệnh chờ", (failed > 0 ? (" (" + IntegerToString(failed) + " thất bại — vẫn còn sống trên sàn)") : "."));
}

void CheckDDPause() {
    if(!g_DDPauseEnable) return;

    double botFloat = FloatProfit();

    if(botFloat > g_DDPauseThreshold) return;

    Print("RTB: DD $", DoubleToString(botFloat, 2), " <= ngưỡng $", DoubleToString(g_DDPauseThreshold, 2), " — cân lệnh & pause EA.");
    BalanceHedgeAccount();
    CancelAllPendingOrders();
    g_PauseEA       = true;
    g_DDPauseEnable = false;
    Print("RTB: Đã cân lệnh xong — tự động tắt Cân Lệnh (bật lại thủ công nếu muốn theo dõi DD tiếp). EA vẫn đang Pause — tắt tay ở nút Pause EA khi sẵn sàng giao dịch lại.");
}

double NormLot(double lot) {
    double minL  = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxL  = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    double stepL = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lot = MathRound(lot / stepL) * stepL;
    return MathMax(minL, MathMin(maxL, lot));
}

double DCAOrderLot(double baseLot, int orderIdx1) {
    if(g_DCAArithEnable)
        return NormLot(baseLot + orderIdx1 * g_DCAArithStep);
    return NormLot(baseLot * DCA_Mult);
}

bool OpenOrder(int ordType, double lot, double tp_pts = 0, double sl_pts = 0, bool isDCA = false, int slotIdx = -1) {
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    double price, tp = 0, sl = 0;

    bool applyTP = tp_pts > 0;
    bool applySL = sl_pts > 0;

    if(ordType == ORDER_TYPE_BUY) {
        price = ask;
        if(applyTP) tp = NormalizeDouble(price + tp_pts * point, _Digits);
        if(applySL) sl = NormalizeDouble(price - sl_pts * point, _Digits);
    } else {
        price = bid;
        if(applyTP) tp = NormalizeDouble(price - tp_pts * point, _Digits);
        if(applySL) sl = NormalizeDouble(price + sl_pts * point, _Digits);
    }

    string comment;
    if(isDCA) {
        string base = (tp_pts == 0 && sl_pts == 0) ? "RTB|0|0" : StringFormat("RTB|%.0f|%.0f", tp_pts, sl_pts);
        if(slotIdx >= 0)
            comment = base + "|S" + IntegerToString(slotIdx);
        else
            comment = (tp_pts == 0 && sl_pts == 0) ? base + "|D" : base;
    }
    else comment = "RTB|0|0";

    lot = NormLot(lot);
    bool ok;
    if(ordType == ORDER_TYPE_BUY)
        ok = Trade.Buy(lot, _Symbol, price, sl, tp, comment);
    else
        ok = Trade.Sell(lot, _Symbol, price, sl, tp, comment);

    if(ok) {
        LastOrderTime = TimeCurrent();
        string tag = isDCA ? " [DCA]" : " [Entry]";
        Print("RTB: Open ", (ordType == ORDER_TYPE_BUY ? "BUY" : "SELL"),
              " lot=", lot, " tp=", tp, " sl=", sl, tag);
    } else {
        Print("RTB: OpenOrder FAILED type=", ordType, " err=", GetLastError());
    }
    return ok;
}

void OpenManualOrder(int ordType) {
    double lot = NormLot(InpManualLotSize);
    double ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    Trade.SetExpertMagicNumber(0);
    bool ok = (ordType == ORDER_TYPE_BUY)
        ? Trade.Buy(lot, _Symbol, ask, 0, 0, "")
        : Trade.Sell(lot, _Symbol, bid, 0, 0, "");
    Trade.SetExpertMagicNumber(InpMagic);
    if(ok)
        Print("RTB: Mở lệnh tay ", (ordType == ORDER_TYPE_BUY ? "BUY" : "SELL"), " lot=", lot);
    else
        Print("RTB: Mở lệnh tay thất bại, err=", GetLastError());
}

int SignalCandle() {
    // Cố ý đọc bar 0 (nến đang chạy dở, chưa đóng) thay vì bar đã đóng — tín hiệu đổi liên tục theo
    // màu nến hiện tại cho tới khi nến đóng hẳn. Repaint có chủ đích (xem rtb-entry-signals.md SIG_CANDLE).
    MqlRates r[];
    ArraySetAsSeries(r, true);
    if(CopyRates(_Symbol, InpSignalTF, 0, 1, r) < 1) return 0;

    if(r[0].close > r[0].open) return  1;
    if(r[0].close < r[0].open) return -1;
    return 0;
}

int GetSignal() {
    int sig = SignalCandle();
    if(g_Direction == DIR_ONLY_BUY  && sig < 0) return 0;
    if(g_Direction == DIR_ONLY_SELL && sig > 0) return 0;
    return sig;
}

string PeakGVName(int posType) {
    return "RTB_Peak_" + _Symbol + "_" + IntegerToString(InpMagic) + "_" + (posType == POSITION_TYPE_BUY ? "BUY" : "SELL");
}

void SavePeak(int posType, int val) {
    GlobalVariableSet(PeakGVName(posType), (double)val);
    GlobalVariablesFlush();
}

int LoadPeak(int posType) {
    string name = PeakGVName(posType);
    if(!GlobalVariableCheck(name)) return 0;
    return (int)GlobalVariableGet(name);
}

string SlotPxGVName(int posType, int slot) {
    return "RTB_SlotPx_" + _Symbol + "_" + IntegerToString(InpMagic) + "_" +
           (posType == POSITION_TYPE_BUY ? "BUY" : "SELL") + "_" + IntegerToString(slot);
}

void SaveSlotPrice(int posType, int slot, double price) {
    GlobalVariableSet(SlotPxGVName(posType, slot), price);
}

double LoadSlotPrice(int posType, int slot) {
    string name = SlotPxGVName(posType, slot);
    if(!GlobalVariableCheck(name)) return 0;
    return GlobalVariableGet(name);
}

void ClearSlotPrices(int posType, int cap) {
    for(int i = 0; i < cap; i++)
        GlobalVariableDel(SlotPxGVName(posType, i));
}

void ResetDCAState(int posType) {
    ENUM_ORDER_TYPE pendType1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE pendType2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    for(int i = OrdersTotal() - 1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot != pendType1 && ot != pendType2) continue;
        string cmt = OrderGetString(ORDER_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        string parts[];
        int np = StringSplit(cmt, '|', parts);
        if(np == 3 && parts[1] == "0" && parts[2] == "0") continue;
        if(!Trade.OrderDelete(tk))
            Print("RTB: ResetDCAState huỷ lệnh chờ ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
                  " ticket=", tk, " thất bại, err=", GetLastError());
    }

    if(posType == POSITION_TYPE_BUY) {
        PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
        ClearSlotPrices(posType, ArraySize(DCABuyPrices));
    } else {
        PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
        ClearSlotPrices(posType, ArraySize(DCASellPrices));
    }
    SavePeak(posType, 0);
}

bool HasPendingDCA(int posType) {
    int peak = (posType == POSITION_TYPE_BUY) ? PeakDCABuy : PeakDCASell;
    for(int i = 0; i < peak; i++) {
        ulong tk = (posType == POSITION_TYPE_BUY) ? DCABuyLimitTk[i] : DCASellLimitTk[i];
        if(tk > 0) return true;
    }
    return false;
}

void TryOpenBuy() {
    if(CountBuy() >= DCA_MaxOrd + 1) return;
    if(CountBuy() > 0) return;
    if(HasPendingDCA(POSITION_TYPE_BUY)) return;
    ResetDCAState(POSITION_TYPE_BUY);
    if(OpenOrder(ORDER_TYPE_BUY, InpLotSize)) {
        LastEntryTime = TimeCurrent();
        ulong tk = Trade.ResultOrder();
        OrigBuyPrice = (tk > 0 && PositionSelectByTicket(tk))
            ? PositionGetDouble(POSITION_PRICE_OPEN) : SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    }
}

void TryOpenSell() {
    if(CountSell() >= DCA_MaxOrd + 1) return;
    if(CountSell() > 0) return;
    if(HasPendingDCA(POSITION_TYPE_SELL)) return;
    ResetDCAState(POSITION_TYPE_SELL);
    if(OpenOrder(ORDER_TYPE_SELL, InpLotSize)) {
        LastEntryTime = TimeCurrent();
        ulong tk = Trade.ResultOrder();
        OrigSellPrice = (tk > 0 && PositionSelectByTicket(tk))
            ? PositionGetDouble(POSITION_PRICE_OPEN) : SymbolInfoDouble(_Symbol, SYMBOL_BID);
    }
}

void CheckEntry() {
    if(DayLimitHit) return;
    if(!g_BotEnabled) return;

    // Khi sạch lệnh (CountAll()==0): bỏ qua InpOrderDelay để vào lệnh gốc mới NGAY khi nến cho tín
    // hiệu, không chờ hết độ trễ còn lại từ lệnh trước đó. Delay vẫn áp dụng bình thường khi đang có
    // lệnh (DCA/CheckDCA() tự tôn trọng InpOrderDelay riêng qua LastOrderTime).
    bool skipDelay = (CountAll() == 0);
    if(!skipDelay && TimeCurrent() - LastEntryTime < g_OrderDelay) return;

    int sig = GetSignal();
    if(sig == 0) return;

    // Candle phát tín hiệu mới mỗi khi nến đang chạy đổi màu — nếu không chặn, nến lật màu trong lúc
    // 1 chiều đang chạy DCA sẽ mở thêm lệnh gốc hedge ở chiều đối diện dù InpDirection=Both. Chỉ cho
    // phép lệnh gốc ĐẦU TIÊN (cả 2 chiều đang trống) — sau đó DCA quản lý tiếp đúng 1 chuỗi.
    if(CountAll() > 0) return;

    if(sig > 0) TryOpenBuy();
    if(sig < 0) TryOpenSell();
}


bool IsSlotOpen(int posType, int slot) {
    ulong tk = (posType == POSITION_TYPE_BUY) ? DCABuyTickets[slot] : DCASellTickets[slot];
    if(tk > 0 && PositionSelectByTicket(tk)) return true;
    double slotPrice = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot] : DCASellPrices[slot];
    if(slotPrice == 0) return false;
    double tol = 50.0 * SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    for(int i = 0; i < PositionsTotal(); i++) {
        ulong ptk = PositionGetTicket(i);
        if(!PositionSelectByTicket(ptk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(MathAbs(PositionGetDouble(POSITION_PRICE_OPEN) - slotPrice) <= tol) {
            if(posType == POSITION_TYPE_BUY) DCABuyTickets[slot]  = ptk;
            else                              DCASellTickets[slot] = ptk;
            return true;
        }
    }
    return false;
}


// Đếm số lệnh chờ DCA đang sống thật trên sàn theo chiều posType (đọc trực tiếp OrdersTotal(), không dựa
// vào mảng ticket nội bộ — an toàn kể cả khi ticket bị lệch do EA restart/desync).
int CountLivePendingDCA(int posType) {
    int cnt = 0;
    ENUM_ORDER_TYPE t1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE t2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    for(int i = OrdersTotal() - 1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot != t1 && ot != t2) continue;
        cnt++;
    }
    return cnt;
}

// Tìm lệnh chờ DCA cách xa giá hiện tại nhất theo chiều posType — dùng để nhường chỗ cho lệnh chờ gần
// giá hơn khi đã chạm trần InpMaxPendingDCA.
bool FarthestPendingDCA(int posType, ulong &farTk, double &farDist) {
    double refPrice = (posType == POSITION_TYPE_BUY) ? SymbolInfoDouble(_Symbol, SYMBOL_ASK) : SymbolInfoDouble(_Symbol, SYMBOL_BID);
    ENUM_ORDER_TYPE t1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE t2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    farTk = 0; farDist = -1;
    for(int i = OrdersTotal() - 1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot != t1 && ot != t2) continue;
        double dist = MathAbs(OrderGetDouble(ORDER_PRICE_OPEN) - refPrice);
        if(dist > farDist) { farDist = dist; farTk = tk; }
    }
    return farTk > 0;
}

// Giữ trần InpMaxPendingDCA lệnh chờ DCA sống cùng lúc mỗi chiều (một số sàn giới hạn tổng số lệnh chờ,
// vượt trần sẽ bị từ chối) — ưu tiên giữ các lệnh GẦN giá hiện tại nhất. Gọi ngay trước khi đặt 1 lệnh
// chờ DCA mới tại newPrice: nếu chưa đầy trần, cho đặt luôn; nếu đầy trần mà newPrice gần giá hơn lệnh
// chờ xa nhất đang có, huỷ lệnh xa nhất để nhường chỗ; nếu newPrice xa hơn mọi lệnh đang có thì bỏ qua,
// CheckDCA() sẽ tự thử lại các lượt sau (khi giá đổi hoặc lệnh chờ khác biến mất).
bool MakeRoomForPendingDCA(int posType, double newPrice) {
    if(InpMaxPendingDCA <= 0) return true; // 0 = không giới hạn
    if(CountLivePendingDCA(posType) < InpMaxPendingDCA) return true;

    double refPrice = (posType == POSITION_TYPE_BUY) ? SymbolInfoDouble(_Symbol, SYMBOL_ASK) : SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double newDist  = MathAbs(newPrice - refPrice);

    ulong  farTk; double farDist;
    if(!FarthestPendingDCA(posType, farTk, farDist)) return true;

    if(newDist >= farDist) return false;

    if(Trade.OrderDelete(farTk)) {
        Print("RTB: Trần lệnh chờ DCA (", InpMaxPendingDCA, ") đầy phía ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
              " — huỷ lệnh chờ xa nhất (ticket=", farTk, ", cách ", DoubleToString(farDist, _Digits),
              ") để nhường chỗ cho lệnh gần giá hơn.");
        return true;
    }
    Print("RTB: Huỷ lệnh chờ xa nhất (ticket=", farTk, ") để nhường trần lệnh chờ DCA thất bại, err=", GetLastError());
    return false;
}

// Cưỡng chế trần InpMaxPendingDCA — khác MakeRoomForPendingDCA() (chỉ nhường chỗ khi có 1 lệnh MỚI sắp
// đặt), hàm này chủ động huỷ bớt lệnh chờ XA giá nhất cho tới khi về đúng trần, chạy mỗi tick bất kể có
// đặt lệnh mới hay không. Cần thiết cho trường hợp số lệnh chờ đã vượt trần từ trước (tích luỹ từ bản EA
// cũ chưa có giới hạn, hoặc InpMaxPendingDCA vừa bị hạ thấp hơn số đang có) — nếu không có hàm này, trần
// chỉ ngăn được lệnh MỚI vượt thêm chứ không tự rút gọn số dư thừa đã có sẵn về đúng trần.
void EnforcePendingDCACap(int posType) {
    if(InpMaxPendingDCA <= 0) return;
    int guard = 0;
    while(CountLivePendingDCA(posType) > InpMaxPendingDCA && guard < 200) {
        guard++;
        ulong farTk; double farDist;
        if(!FarthestPendingDCA(posType, farTk, farDist)) break;
        if(Trade.OrderDelete(farTk)) {
            Print("RTB: Vượt trần lệnh chờ DCA (", InpMaxPendingDCA, ") phía ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
                  " — huỷ bớt lệnh chờ xa nhất (ticket=", farTk, ", cách ", DoubleToString(farDist, _Digits), ").");
        } else {
            Print("RTB: Huỷ lệnh chờ xa nhất (ticket=", farTk, ") để ép trần lệnh chờ DCA thất bại, err=", GetLastError());
            break;
        }
    }
}

void CheckDCA(int posType) {
    EnforcePendingDCACap(posType);

    int count = CountPos(posType);
    if(count == 0) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    int maxOrds  = DCA_MaxOrd + 1;

    int peak = (posType == POSITION_TYPE_BUY) ? PeakDCABuy : PeakDCASell;

    // Refill đã bị tắt: khi 1 slot đã đóng lệnh (chạm TP/SL hoặc bị đóng cách nào khác), KHÔNG đặt lại
    // lệnh chờ ở giá cũ nữa — số lệnh DCA đang mở chỉ giảm dần theo số tầng đã chốt, không tự bơm lại.
    // Rổ chỉ đóng sạch 1 lần khi FloatProfit() chạm InpCloseProfit (CheckExit() — Basket Profit Target).
    // Vòng lặp dưới đây chỉ còn tác dụng dọn dẹp: khi 1 slot đang có vị thế sống, xoá sạch LimitTk/Bounced
    // còn sót (ví dụ từ RebuildDCAState() phục hồi 1 lệnh chờ refill cũ trên broker trước khi tắt cơ chế này).
    for(int slot = 0; slot < peak; slot++) {
        double slotPrice = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot] : DCASellPrices[slot];
        if(slotPrice == 0) continue;

        if(IsSlotOpen(posType, slot)) {
            if(posType == POSITION_TYPE_BUY) { DCABuyBounced[slot] = false; DCABuyLimitTk[slot]  = 0; }
            else                              { DCASellBounced[slot] = false; DCASellLimitTk[slot] = 0; }
        }
    }

    if(count >= maxOrds) return;
    if(peak >= maxOrds) return;
    double lastPrice = (peak > 0)
        ? ((posType == POSITION_TYPE_BUY) ? DCABuyPrices[peak-1] : DCASellPrices[peak-1])
        : LastOpenPrice(posType);
    if(lastPrice == 0) return;

    if(peak >= DCA_MaxOrd) return;

    double dist   = DCA_Dist * point;
    double target = (posType == POSITION_TYPE_BUY) ? NormalizeDouble(lastPrice - dist, _Digits)
                                                    : NormalizeDouble(lastPrice + dist, _Digits);
    ulong  nextTk = (posType == POSITION_TYPE_BUY) ? DCABuyLimitTk[peak] : DCASellLimitTk[peak];
    bool   goMarket = false;

    if(nextTk > 0) {
        if(PositionSelectByTicket(nextTk)) {
            double fillPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            SaveSlotPrice(posType, peak, fillPrice);
            if(posType == POSITION_TYPE_BUY) { DCABuyPrices[peak] = fillPrice; DCABuyTickets[peak] = nextTk; DCABuyLimitTk[peak] = 0; PeakDCABuy++; SavePeak(posType, PeakDCABuy); }
            else                              { DCASellPrices[peak] = fillPrice; DCASellTickets[peak] = nextTk; DCASellLimitTk[peak] = 0; PeakDCASell++; SavePeak(posType, PeakDCASell); }
            Print("RTB: DCA khớp bằng lệnh chờ tại ", fillPrice, " peak=", peak);
            return;
        }
        if(!OrderSelect(nextTk)) {
            if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[peak] = 0;
            else                              DCASellLimitTk[peak] = 0;
            return;
        }
        double overshootTol = 300.0 * point;
        bool overshot = (posType == POSITION_TYPE_BUY) ? (target - ask) >= overshootTol
                                                        : (bid - target) >= overshootTol;
        if(!overshot) return;
        if(!Trade.OrderDelete(nextTk)) {
            Print("RTB: Huỷ lệnh chờ tầng mới (peak=", peak, ") thất bại, err=", GetLastError(), " — thử lại lượt sau.");
            return;
        }
        if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[peak] = 0;
        else                              DCASellLimitTk[peak] = 0;
        Print("RTB: DCA giá gapped qua mục tiêu ", target, " — huỷ lệnh chờ, vào market lấy giá tốt hơn.");
        goMarket = true;
    } else {
        goMarket = (posType == POSITION_TYPE_BUY) ? (ask <= target) : (bid >= target);
    }

    if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

    double lot = DCAOrderLot(InpLotSize, peak + 1);

    if(goMarket) {
        int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
        Print("RTB: DCA vào market. peak=", peak);
        bool ok = OpenOrder(ord, lot, DCA_TP, DCA_SL, true, peak);
        ulong newTk = Trade.ResultOrder();
        if(ok && newTk > 0) {
            double fillPrice = 0;
            if(PositionSelectByTicket(newTk))
                fillPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            double usedPrice = (posType == POSITION_TYPE_BUY) ? (fillPrice > 0 ? fillPrice : ask) : (fillPrice > 0 ? fillPrice : bid);
            SaveSlotPrice(posType, peak, usedPrice);
            if(posType == POSITION_TYPE_BUY) { DCABuyPrices[peak] = usedPrice; DCABuyTickets[peak] = newTk; PeakDCABuy++; SavePeak(posType, PeakDCABuy); }
            else                              { DCASellPrices[peak] = usedPrice; DCASellTickets[peak] = newTk; PeakDCASell++; SavePeak(posType, PeakDCASell); }
        }
        return;
    }

    {
        double tolDupPeak = 50.0 * point;
        bool duplicateAtTarget = false;
        for(int pi = PositionsTotal()-1; pi >= 0 && !duplicateAtTarget; pi--) {
            ulong ptk = PositionGetTicket(pi);
            if(!PositionSelectByTicket(ptk)) continue;
            if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
            if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
            if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
            if(MathAbs(PositionGetDouble(POSITION_PRICE_OPEN) - target) < tolDupPeak) duplicateAtTarget = true;
        }
        if(!duplicateAtTarget) {
            ENUM_ORDER_TYPE dupType1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
            ENUM_ORDER_TYPE dupType2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
            for(int oi = OrdersTotal()-1; oi >= 0 && !duplicateAtTarget; oi--) {
                ulong otk = OrderGetTicket(oi);
                if(otk == 0 || !OrderSelect(otk)) continue;
                if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
                if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
                ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
                if(ot != dupType1 && ot != dupType2) continue;
                if(MathAbs(OrderGetDouble(ORDER_PRICE_OPEN) - target) < tolDupPeak) duplicateAtTarget = true;
            }
        }
        if(duplicateAtTarget) {
            Print("RTB: (", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
                  ") đã có vị thế/lệnh chờ thật ở giá ", target, " — bỏ qua đặt trùng.");
            return;
        }
    }

    if(!MakeRoomForPendingDCA(posType, target)) return;

    string cmt = ((DCA_TP == 0 && DCA_SL == 0)
        ? "RTB|0|0"
        : "RTB|" + IntegerToString((int)DCA_TP) + "|" + IntegerToString((int)DCA_SL))
        + "|S" + IntegerToString(peak);
    bool placed;
    if(posType == POSITION_TYPE_BUY) {
        double tp_p = DCA_TP > 0 ? NormalizeDouble(target + DCA_TP * point, _Digits) : 0;
        double sl_p = DCA_SL > 0 ? NormalizeDouble(target - DCA_SL * point, _Digits) : 0;
        placed = Trade.BuyLimit(lot, target, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
    } else {
        double tp_p = DCA_TP > 0 ? NormalizeDouble(target - DCA_TP * point, _Digits) : 0;
        double sl_p = DCA_SL > 0 ? NormalizeDouble(target + DCA_SL * point, _Digits) : 0;
        placed = Trade.SellLimit(lot, target, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
    }
    if(placed) {
        ulong tk = Trade.ResultOrder();
        if(tk > 0) {
            if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[peak] = tk;
            else                              DCASellLimitTk[peak] = tk;
            LastOrderTime = TimeCurrent();
            Print("RTB: Đặt lệnh chờ DCA tại ", target, " (peak=", peak, ")");
        }
    }
}

void CheckOrigRestart(int posType) {
    double origPrice = (posType == POSITION_TYPE_BUY) ? OrigBuyPrice : OrigSellPrice;
    if(origPrice == 0) return;

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(PositionGetString(POSITION_COMMENT) == "RTB|0|0") return;
    }

    int count   = CountPos(posType);
    int maxOrds = DCA_MaxOrd + 1;
    if(count >= maxOrds) return;
    if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

    ResetDCAState(posType);

    int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
    if(OpenOrder(ord, InpLotSize)) {
        LastEntryTime = TimeCurrent();
        ulong tk = Trade.ResultOrder();
        double fillPrice = (tk > 0 && PositionSelectByTicket(tk))
            ? PositionGetDouble(POSITION_PRICE_OPEN)
            : ((posType == POSITION_TYPE_BUY) ? SymbolInfoDouble(_Symbol, SYMBOL_ASK) : SymbolInfoDouble(_Symbol, SYMBOL_BID));
        if(posType == POSITION_TYPE_BUY) OrigBuyPrice  = fillPrice;
        else                              OrigSellPrice = fillPrice;
        Print("RTB: Lệnh gốc (", (posType==POSITION_TYPE_BUY?"BUY":"SELL"),
              ") đã bị cắt — mở lại NGAY tại ", fillPrice, ", huỷ sạch lệnh chờ và reset chuỗi DCA.");
    }
}

void ApplyTrailToPos(ulong tk, int posType, double newSL) {
    if(!PositionSelectByTicket(tk)) return;
    double curSL   = PositionGetDouble(POSITION_SL);
    double curTP   = PositionGetDouble(POSITION_TP);
    double bid     = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask     = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double point   = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double minDist = (double)SymbolInfoInteger(_Symbol, SYMBOL_TRADE_STOPS_LEVEL) * point;
    double normSL  = NormalizeDouble(newSL, _Digits);
    if(posType == POSITION_TYPE_BUY  && bid - newSL < minDist) return;
    if(posType == POSITION_TYPE_SELL && newSL - ask < minDist) return;
    if(curSL != normSL)
        Trade.PositionModify(tk, normSL, curTP);
}

void CheckTrailing() {
    if(!g_TrailEnable) return;
    if(CountPosForTrail(POSITION_TYPE_BUY) + CountPosForTrail(POSITION_TYPE_SELL) < g_TrailMinOrds) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForTrail()) continue;

        int    pt        = (int)PositionGetInteger(POSITION_TYPE);
        double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);

        if(pt == POSITION_TYPE_BUY) {
            double profitPts = (bid - openPrice) / point;
            if(profitPts >= g_TrailActivate) {
                double newSL = bid - g_TrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL >= curSL + g_TrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_BUY, newSL);
            }
        } else {
            double profitPts = (openPrice - ask) / point;
            if(profitPts >= g_TrailActivate) {
                double newSL = ask + g_TrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL <= curSL - g_TrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_SELL, newSL);
            }
        }
    }
}

void CheckManualTrailing() {
    if(!g_ManualTrailEnable) return;
    if(CountManualForTrail(POSITION_TYPE_BUY) + CountManualForTrail(POSITION_TYPE_SELL) < g_ManualTrailMinOrds) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManualManagedForTrail()) continue;

        int    pt        = (int)PositionGetInteger(POSITION_TYPE);
        double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);

        if(pt == POSITION_TYPE_BUY) {
            double profitPts = (bid - openPrice) / point;
            if(profitPts >= g_ManualTrailActivate) {
                double newSL = bid - g_ManualTrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL >= curSL + g_ManualTrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_BUY, newSL);
            }
        } else {
            double profitPts = (openPrice - ask) / point;
            if(profitPts >= g_ManualTrailActivate) {
                double newSL = ask + g_ManualTrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL <= curSL - g_ManualTrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_SELL, newSL);
            }
        }
    }
}

double GetManualSLEMA() {
    if(hManualSLEMA == INVALID_HANDLE) return 0;
    double buf[1];
    if(CopyBuffer(hManualSLEMA, 0, 0, 1, buf) <= 0) return 0;
    return buf[0];
}

bool ManualSLAlreadySeen(ulong tk) {
    int n = ArraySize(g_ManualSLSeen);
    for(int i = 0; i < n; i++)
        if(g_ManualSLSeen[i] == tk) return true;
    return false;
}

void MarkManualSLSeen(ulong tk) {
    int n = ArraySize(g_ManualSLSeen);
    ArrayResize(g_ManualSLSeen, n + 1);
    g_ManualSLSeen[n] = tk;
}

void PruneManualSLSeen() {
    int n = ArraySize(g_ManualSLSeen);
    int k = 0;
    for(int i = 0; i < n; i++) {
        if(PositionSelectByTicket(g_ManualSLSeen[i]))
            g_ManualSLSeen[k++] = g_ManualSLSeen[i];
    }
    ArrayResize(g_ManualSLSeen, k);
}

void CheckManualAutoSL() {
    if(!g_ManualAutoSLEnable) return;
    if(g_ManualSL_Points <= 0) return;

    PruneManualSLSeen();

    double emaVal = GetManualSLEMA();
    if(emaVal <= 0) return;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") == 0) continue;
        if(cmt == HEDGE_ALL_CMT) continue;

        if(ManualSLAlreadySeen(tk)) continue;

        double curSL = PositionGetDouble(POSITION_SL);
        if(curSL != 0) { MarkManualSLSeen(tk); continue; } // đã có SL (Trailing/người dùng đặt) — không đụng vào

        int    pt    = (int)PositionGetInteger(POSITION_TYPE);
        double opn   = PositionGetDouble(POSITION_PRICE_OPEN);
        double curTP = PositionGetDouble(POSITION_TP);

        // Gán 1 lần duy nhất lúc mới phát hiện lệnh, sau đó nhường quyền cho Trailing lệnh tay (tránh 2 cơ chế
        // cùng ghi đè SL liên tục gây xung đột). BUY: giá trên EMA -> cùng chiều -> SL xa gấp đôi; giá dưới EMA
        // -> ngược chiều -> SL gần (1x). SELL: ngược lại.
        bool   sameDir = (pt == POSITION_TYPE_BUY) ? (bid > emaVal) : (ask < emaVal);
        double slPts   = sameDir ? g_ManualSL_Points * 2.0 : g_ManualSL_Points;

        double sl = (pt == POSITION_TYPE_BUY) ? NormalizeDouble(opn - slPts * point, _Digits)
                                               : NormalizeDouble(opn + slPts * point, _Digits);

        if(Trade.PositionModify(tk, sl, curTP))
            Print("RTB: EMA lọc — gán SL 1 lần lúc vào lệnh (", (sameDir ? "cùng chiều x2" : "ngược chiều x1"),
                  ") cho lệnh tay ticket=", tk, " sl=", sl);
        else
            Print("RTB: EMA lọc — gán SL lệnh tay ticket=", tk, " thất bại, err=", GetLastError());

        MarkManualSLSeen(tk);
    }
}

void CheckExit() {
    if(g_ClosePerPips > 0) {
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            int    pt    = (int)PositionGetInteger(POSITION_TYPE);
            double opn   = PositionGetDouble(POSITION_PRICE_OPEN);
            double ppts  = (pt == POSITION_TYPE_BUY) ? (bid - opn) / point : (opn - ask) / point;

            if(ppts >= g_ClosePerPips)
                Trade.PositionClose(tk);
        }
    }

    {
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            string cmt = PositionGetString(POSITION_COMMENT);
            if(StringFind(cmt, "RTB|") != 0) continue;
            string parts[];
            if(StringSplit(cmt, '|', parts) < 3) continue;
            double useTP = StringToDouble(parts[1]);
            double useSL = StringToDouble(parts[2]);
            if(useTP == 0 && useSL == 0) continue;

            int    pt  = (int)PositionGetInteger(POSITION_TYPE);
            double opn = PositionGetDouble(POSITION_PRICE_OPEN);

            if(pt == POSITION_TYPE_BUY) {
                if(useTP > 0 && bid >= opn + useTP * point) { Trade.PositionClose(tk); continue; }
                if(useSL > 0 && bid <= opn - useSL * point)   Trade.PositionClose(tk);
            } else {
                if(useTP > 0 && ask <= opn - useTP * point) { Trade.PositionClose(tk); continue; }
                if(useSL > 0 && ask >= opn + useSL * point)   Trade.PositionClose(tk);
            }
        }
    }

    // g_CloseIncludeManual (nút "Tính lệnh tay" trên panel Manual Trade): khi bật, lệnh tay (Magic=0)
    // cũng được cộng vào Float để xét ngưỡng InpCloseProfit/InpCloseLoss, và khi ngưỡng chạm thì lệnh
    // tay cũng bị đóng theo cùng lúc với lệnh bot (CloseAllManual()) — vì đã tính chung vào quyết định.
    double basketFloat = FloatProfit() + (g_CloseIncludeManual ? ManualFloatProfit() : 0.0);

    if(g_CloseProfit > 0) {
        if(basketFloat >= g_CloseProfit) {
            Print("RTB: CloseProfit target reached. Closing all.");
            CloseAll();
            if(g_CloseIncludeManual) CloseAllManual();
            return;
        }
    }

    if(g_CloseLoss > 0) {
        if(basketFloat <= -g_CloseLoss) {
            Print("RTB: CloseLoss limit hit. Closing all.");
            CloseAll();
            if(g_CloseIncludeManual) CloseAllManual();
            return;
        }
    }
}

void UpdateDayProfit() {
    MqlDateTime dt;
    TimeToStruct(TimeCurrent(), dt);
    if(dt.day != LastDay) {
        DayProfit    = 0;
        DayLimitHit  = false;
        LastDay      = dt.day;
    }

    datetime dayStart = StringToTime(StringFormat("%04d.%02d.%02d 00:00:00",
                         dt.year, dt.mon, dt.day));
    if(!HistorySelect(dayStart, TimeCurrent())) return;

    double closed = 0;
    for(int i = 0; i < HistoryDealsTotal(); i++) {
        ulong dTk = HistoryDealGetTicket(i);
        if(HistoryDealGetString(dTk, DEAL_SYMBOL) != _Symbol) continue;
        ENUM_DEAL_ENTRY de = (ENUM_DEAL_ENTRY)HistoryDealGetInteger(dTk, DEAL_ENTRY);
        if(de == DEAL_ENTRY_OUT || de == DEAL_ENTRY_OUT_BY)
            closed += HistoryDealGetDouble(dTk, DEAL_PROFIT) +
                      HistoryDealGetDouble(dTk, DEAL_SWAP);
    }
    DayProfit = closed;
}

void CheckDayLimit() {
    if(DayLimitHit) return;
    double equity = DayProfit + FloatProfit();
    if(g_DayMaxLoss > 0 && equity <= -g_DayMaxLoss) {
        Print("RTB: Day loss limit $", g_DayMaxLoss, " hit. equity=", equity, ". Closing all.");
        CloseAll();
        DayLimitHit = true;
        return;
    }
    if(g_DayMaxProfit > 0 && equity >= g_DayMaxProfit) {
        Print("RTB: Day profit target $", g_DayMaxProfit, " hit. equity=", equity, ". Closing all.");
        CloseAll();
        DayLimitHit = true;
    }
}

struct PeriodStats { double pips, profit, gain, lot; };

PeriodStats GetPeriodStats(datetime from, datetime to) {
    PeriodStats s;
    s.pips = s.profit = s.gain = s.lot = 0;
    if(!HistorySelect(from, to)) return s;
    double contractSize = SymbolInfoDouble(_Symbol, SYMBOL_TRADE_CONTRACT_SIZE);
    for(int i = 0; i < HistoryDealsTotal(); i++) {
        ulong dk = HistoryDealGetTicket(i);
        if(HistoryDealGetString(dk, DEAL_SYMBOL) != _Symbol) continue;
        ENUM_DEAL_ENTRY de = (ENUM_DEAL_ENTRY)HistoryDealGetInteger(dk, DEAL_ENTRY);
        if(de != DEAL_ENTRY_OUT && de != DEAL_ENTRY_OUT_BY) continue;
        double dp = HistoryDealGetDouble(dk, DEAL_PROFIT) + HistoryDealGetDouble(dk, DEAL_SWAP);
        double dv = HistoryDealGetDouble(dk, DEAL_VOLUME);
        s.profit += dp;
        s.lot    += dv;
    }
    if(contractSize > 0 && s.lot > 0)
        s.pips = s.profit / (s.lot * contractSize) * 10.0;
    s.gain = (InitBalance > 0) ? s.profit / InitBalance * 100.0 : 0;
    return s;
}

void CreateBtn(string name, string text, int x, int y, int w, int h, color bgClr, color borderClr = clrSilver) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_BUTTON, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetInteger(0, obj, OBJPROP_XSIZE,      w);
        ObjectSetInteger(0, obj, OBJPROP_YSIZE,      h);
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Tahoma");
        ObjectSetInteger(0, obj, OBJPROP_FONTSIZE,   8);
        ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE,  x);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE,  y);
    ObjectSetString(0,  obj, OBJPROP_TEXT,         text);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,        clrWhite);
    ObjectSetInteger(0, obj, OBJPROP_BGCOLOR,      bgClr);
    ObjectSetInteger(0, obj, OBJPROP_BORDER_COLOR, borderClr);
    ObjectSetInteger(0, obj, OBJPROP_STATE,        false);
}

void DrawHLine(string name, double price, color clr, int width = 1) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0)
        ObjectCreate(0, obj, OBJ_HLINE, 0, 0, price);
    ObjectSetDouble(0,  obj, OBJPROP_PRICE, price);
    ObjectSetInteger(0, obj, OBJPROP_COLOR, clr);
    ObjectSetInteger(0, obj, OBJPROP_STYLE, STYLE_SOLID);
    ObjectSetInteger(0, obj, OBJPROP_WIDTH, width);
}

void CreateRect(string name, int lx, int ly, int lw, int lh, color bg) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, obj, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, obj, OBJPROP_WIDTH,       0);
        ObjectSetInteger(0, obj, OBJPROP_BACK,        false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, lx);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, ly);
    ObjectSetInteger(0, obj, OBJPROP_XSIZE,   lw);
    ObjectSetInteger(0, obj, OBJPROP_YSIZE,   lh);
    ObjectSetInteger(0, obj, OBJPROP_BGCOLOR, bg);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,   bg);
}

void Lbl(string name, string text, int x, int y, color clr = clrSilver, int sz = 9) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_LABEL, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Calibri");
        ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, x);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, y);
    ObjectSetString(0,  obj, OBJPROP_TEXT,     text);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,    clr);
    ObjectSetInteger(0, obj, OBJPROP_FONTSIZE, sz);
}

void LblR(string name, string text, int xRight, int y, color clr = clrSilver, int sz = 9) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_LABEL, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetInteger(0, obj, OBJPROP_ANCHOR,     ANCHOR_RIGHT_UPPER);
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Consolas");
        ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, xRight);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, y);
    ObjectSetString(0,  obj, OBJPROP_TEXT,     text);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,    clr);
    ObjectSetInteger(0, obj, OBJPROP_FONTSIZE, sz);
}

int DaysInMonth(int year, int month) {
    int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = dim[month-1];
    if(month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) d = 29;
    return d;
}

void UpdateCalendarPanel(bool forceRecalc = false) {
    if(!g_CalExpanded) {
        ObjectDelete(0, GUI + "CalBG");
        ObjectDelete(0, GUI + "CalTitle");
        ObjectDelete(0, GUI + "BtnCalPrev");
        ObjectDelete(0, GUI + "BtnCalNext");
        ObjectDelete(0, GUI + "CalMY");
        ObjectDelete(0, GUI + "CalWDBg");
        for(int i = 0; i < 7; i++) ObjectDelete(0, GUI + "CalWD" + IntegerToString(i));
        for(int i = 0; i < 42; i++) {
            string si = IntegerToString(i);
            ObjectDelete(0, GUI + "CalC" + si);
            ObjectDelete(0, GUI + "CalD" + si);
            ObjectDelete(0, GUI + "CalPL" + si);
            ObjectDelete(0, GUI + "CalP" + si);
            ObjectDelete(0, GUI + "CalVL" + si);
            ObjectDelete(0, GUI + "CalL" + si);
        }
        return;
    }

    int titleH = 28, navH = 26, wdH = 22;
    int calX = InpPanelX + InpPanelWidth + InpCalPanelGap;
    int calY = InpCalPanelY + RTB_TITLEBAR_H;

    int dim = DaysInMonth(g_CalYear, g_CalMonth);
    datetime firstDay = StringToTime(StringFormat("%04d.%02d.01 00:00:00", g_CalYear, g_CalMonth));
    MqlDateTime fdt;
    TimeToStruct(firstDay, fdt);
    int firstCol = (fdt.day_of_week + 6) % 7;
    int rows = (int)MathCeil((firstCol + dim) / 7.0);
    if(rows < 1) rows = 1;

    if(g_CalCacheYear != g_CalYear || g_CalCacheMonth != g_CalMonth) {
        g_CalCacheYear  = g_CalYear;
        g_CalCacheMonth = g_CalMonth;
        ArrayInitialize(g_CalCacheDone, false);
    }
    MqlDateTime nowDt;
    TimeToStruct(TimeCurrent(), nowDt);
    bool viewingCurrentMonth = (nowDt.year == g_CalYear && nowDt.mon == g_CalMonth);
    bool throttleOk = forceRecalc || (TimeCurrent() - g_CalLastTodayCalc >= RTB_CAL_TODAY_THROTTLE_SEC);

    int colW;
    {
        uint maxContentW = 0;
        for(int d = 1; d <= dim; d++) {
            int si = firstCol + d - 1;
            if(!g_CalCacheDone[si]) {
                datetime dS = firstDay + (d - 1) * 86400;
                datetime dE = dS + 86400;
                PeriodStats ds = GetPeriodStats(dS, dE);
                g_CalCacheProfit[si] = ds.profit;
                g_CalCacheLot[si]    = ds.lot;
                g_CalCacheDone[si]   = true;
            }
            if(g_CalCacheLot[si] <= 0) continue;
            string pnlTxt = StringFormat("%s%.1f$", g_CalCacheProfit[si] >= 0 ? "+" : "", g_CalCacheProfit[si]);
            string volTxt = StringFormat("%.2f Lot", g_CalCacheLot[si]);
            uint pw = 0, ph = 0, vw = 0, vh = 0;
            TextSetFont("Calibri", 12);
            TextGetSize(pnlTxt, pw, ph);
            TextSetFont("Calibri", 11);
            TextGetSize(volTxt, vw, vh);
            uint contentW = MathMax(pw, vw);
            if(contentW > maxContentW) maxContentW = contentW;
        }
        colW = MathMax(92, 6 + (int)maxContentW + 8);
    }
    int calW = colW * 7;

    int cellH;
    {
        int availH = (g_LastPanelBottom - calY) - titleH - navH - wdH;
        cellH = MathMax(64, availH / rows);
    }
    int gridTop = calY + titleH + navH + wdH;
    int calH = titleH + navH + wdH + rows * cellH;

    string bgc = GUI + "CalBG";
    if(ObjectFind(0, bgc) < 0) {
        ObjectCreate(0, bgc, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bgc, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bgc, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bgc, OBJPROP_COLOR,       C'60,80,140');
        ObjectSetInteger(0, bgc, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bgc, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bgc, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bgc, OBJPROP_XDISTANCE, calX);
    ObjectSetInteger(0, bgc, OBJPROP_YDISTANCE, calY);
    ObjectSetInteger(0, bgc, OBJPROP_XSIZE,     calW);
    ObjectSetInteger(0, bgc, OBJPROP_YSIZE,     calH);
    ObjectSetInteger(0, bgc, OBJPROP_BGCOLOR,   C'10,13,20');

    Lbl("CalTitle", "THỐNG KÊ THEO NGÀY", calX + calW/2 - 75, calY + 7, C'90,160,255', 10);
    ObjectSetString(0, GUI + "CalTitle", OBJPROP_FONT, "Calibri Bold");

    int navY = calY + titleH;
    CreateBtn("BtnCalPrev", "<", calX + calW/2 - 90, navY, 26, navH - 2, C'30,40,70', C'70,100,170');
    Lbl("CalMY", StringFormat("Tháng %02d / %04d", g_CalMonth, g_CalYear), calX + calW/2 - 54, navY + 4, clrWhite, 10);
    CreateBtn("BtnCalNext", ">", calX + calW/2 + 64, navY, 26, navH - 2, C'30,40,70', C'70,100,170');

    string wdNames[7] = {"Thứ 2","Thứ 3","Thứ 4","Thứ 5","Thứ 6","Thứ 7","CN"};
    int wdY = calY + titleH + navH;
    CreateRect("CalWDBg", calX, wdY, calW, wdH, C'35,50,90');
    for(int c = 0; c < 7; c++) {
        string wdObj = "CalWD" + IntegerToString(c);
        Lbl(wdObj, wdNames[c], calX + c*colW + 6, wdY + 3, C'230,140,60', 9);
        ObjectSetString(0, GUI + wdObj, OBJPROP_FONT, "Calibri Bold");
    }

    for(int slot = 0; slot < 42; slot++) {
        int row = slot / 7, col = slot % 7;
        string si = IntegerToString(slot);

        if(row >= rows) {
            ObjectDelete(0, GUI + "CalC" + si);
            ObjectDelete(0, GUI + "CalD" + si);
            ObjectDelete(0, GUI + "CalPL" + si);
            ObjectDelete(0, GUI + "CalP" + si);
            ObjectDelete(0, GUI + "CalVL" + si);
            ObjectDelete(0, GUI + "CalL" + si);
            continue;
        }

        int cx = calX + col*colW;
        int cy = gridTop + row*cellH;

        int day = slot - firstCol + 1;
        bool valid = (day >= 1 && day <= dim);

        string cellObj = GUI + "CalC" + si;
        if(ObjectFind(0, cellObj) < 0) {
            ObjectCreate(0, cellObj, OBJ_RECTANGLE_LABEL, 0, 0, 0);
            ObjectSetInteger(0, cellObj, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
            ObjectSetInteger(0, cellObj, OBJPROP_BORDER_TYPE, BORDER_FLAT);
            ObjectSetInteger(0, cellObj, OBJPROP_COLOR,       C'40,48,68');
            ObjectSetInteger(0, cellObj, OBJPROP_WIDTH,       1);
            ObjectSetInteger(0, cellObj, OBJPROP_BACK,        false);
            ObjectSetInteger(0, cellObj, OBJPROP_SELECTABLE,  false);
        }
        ObjectSetInteger(0, cellObj, OBJPROP_XDISTANCE, cx + 1);
        ObjectSetInteger(0, cellObj, OBJPROP_YDISTANCE, cy);
        ObjectSetInteger(0, cellObj, OBJPROP_XSIZE,     colW - 2);
        ObjectSetInteger(0, cellObj, OBJPROP_YSIZE,     cellH - 2);
        ObjectSetInteger(0, cellObj, OBJPROP_BGCOLOR,   valid ? C'16,19,28' : C'8,9,12');

        if(!valid) {
            ObjectDelete(0, GUI + "CalD" + si);
            ObjectDelete(0, GUI + "CalPL" + si);
            ObjectDelete(0, GUI + "CalP" + si);
            ObjectDelete(0, GUI + "CalVL" + si);
            ObjectDelete(0, GUI + "CalL" + si);
            continue;
        }

        bool isToday = viewingCurrentMonth && (day == nowDt.day);
        if(!g_CalCacheDone[slot] || (isToday && throttleOk)) {
            datetime dStart = firstDay + (day - 1) * 86400;
            datetime dEnd   = dStart + 86400;
            PeriodStats ds  = GetPeriodStats(dStart, dEnd);
            g_CalCacheProfit[slot] = ds.profit;
            g_CalCacheLot[slot]    = ds.lot;
            g_CalCacheDone[slot]   = true;
            if(isToday) g_CalLastTodayCalc = TimeCurrent();
        }
        int cellCenterX = cx + colW / 2;
        int dateY       = cy + 4;

        Lbl("CalD" + si, StringFormat("%02d/%02d", day, g_CalMonth), cellCenterX, dateY, clrSilver, 8);
        ObjectSetInteger(0, GUI + "CalD" + si, OBJPROP_ANCHOR, ANCHOR_UPPER);

        bool isProfitDay = g_CalCacheProfit[slot] >= 0;
        bool dayVisible  = (g_CalCacheLot[slot] > 0) && (isProfitDay ? InpCalShowProfitDays : InpCalShowLossDays);

        if(!dayVisible) {
            ObjectSetInteger(0, cellObj, OBJPROP_COLOR, C'40,48,68');
            ObjectDelete(0, GUI + "CalPL" + si);
            ObjectDelete(0, GUI + "CalP" + si);
            ObjectDelete(0, GUI + "CalVL" + si);
            ObjectDelete(0, GUI + "CalL" + si);
            continue;
        }

        color pc = isProfitDay ? clrLimeGreen : clrTomato;

        ObjectSetInteger(0, cellObj, OBJPROP_COLOR, pc);

        int lineH       = 16;
        int contentTop  = dateY + 16;
        int freeSpace   = (cy + cellH) - contentTop;
        int padTop      = MathMax(0, (freeSpace - lineH - 12) / 2);
        int pnlY        = contentTop + padTop;
        int volY        = pnlY + lineH;

        Lbl("CalP" + si, StringFormat("%s%.1f$", g_CalCacheProfit[slot] >= 0 ? "+" : "", g_CalCacheProfit[slot]), cellCenterX, pnlY, pc, 12);
        ObjectSetInteger(0, GUI + "CalP" + si, OBJPROP_ANCHOR, ANCHOR_UPPER);
        Lbl("CalL" + si, StringFormat("%.2f Lot", g_CalCacheLot[slot]), cellCenterX, volY, clrWhite, 11);
        ObjectSetInteger(0, GUI + "CalL" + si, OBJPROP_ANCHOR, ANCHOR_UPPER);
    }
}

void UpdateGUI(bool forceCalRefresh = false) {
    if(!InpShowPanel) { RemoveGUI(); return; }
    int PX = InpPanelX;
    int PY = InpPanelY;
    int PW = InpPanelWidth;
    int titleOff = RTB_TITLEBAR_H;

    double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
    double totalProfit = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForDisplay()) continue;
        totalProfit += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
    }
    double pnlPct  = (InitBalance > 0) ? totalProfit / InitBalance * 100.0 : 0;

    MqlDateTime dt;
    TimeToStruct(TimeLocal(), dt);
    string tStr = StringFormat("%04d/%02d/%02d  %02d:%02d:%02d",
                   dt.year, dt.mon, dt.day, dt.hour, dt.min, dt.sec);

    color cProfit = (totalProfit >= 0) ? clrLimeGreen : clrTomato;

    int x = PX + 7, s = 18;
    int bh  = 22;
    int bfw = PW - 18;
    int bhw = (PW - 24) / 2;
    int bx2 = PX + 7 + bhw + 4;

    string legacyObjs[] = {"L0","Tim","Sig","Mod","Dir","Sync","L1","Bal","Ini","DayP","FP",
                            "IniL","IniV","DayL","DayV","TotL","TotV","TotAllL","TotAllV",
                            "L2","DD","MDD","L3","BuyP","BuyC","SelP","SelC","Tot","TClock","BtnCloseManual",
                            "CardRisk","CardRiskBar","RiskH","DDL","DDV","DDGTrk","DDGFill",
                            "MDDL","MDDV","MDDGTrk","MDDGFill","HdgS",
                            "CardBuy","CardBuyBar","BuyL","BuyV","BuyLot",
                            "CardSell","CardSellBar","SelL","SelV","SelLot",
                            "P3VC1","P3VC2","P3VC3","P3VC4","P3HR0","P3HR1","P3HR2","P3HR3",
                            "TR0S","TR1S","TR2S","TR3S",
                            "TR0BarTrk","TR1BarTrk","TR2BarTrk","TR3BarTrk",
                            "TR0BarFill","TR1BarFill","TR2BarFill","TR3BarFill",
                            "CardTrim","CardTrimBar","TrimH",
                            "TrimModeL","TrimMode","TrimManL","TrimManV",
                            "SideBG","SideH",
                            "TrimSideBG","TrimSideH","TrimStatsCard","TrimStatsCardBar",
                            "TrimCntL","TrimCntV","TrimWorstL","TrimWorstV","TrimSLL","TrimSLV","BtnTrimToggle",
                            "ReserveCard","ReserveCardBar","ReserveL","ReserveV","BtnResetReserve",
                            "ManualSubCloseProfit","ManualCPTargetL","ManualCPTargetV","BtnManualCloseProfitToggle",
                            "EditManualCP","BtnManualCPEdit","BtnManualCPCancel","BtnManualCPSave",
                            "ManTrailBuyL","ManTrailBuyV","ManTrailSellL","ManTrailSellV",
                            "ManualTPV","ManTrailModeL","ManTrailModeV",
                            "ManualEMAL","ManualEMAV",
                            "ManualSubStats","ManualBuyCntL","ManualSellCntL","ManualFloatL","ManualFloatV",
                            "ChipMode","ChipModeBg",
                            "ChipSig","ChipSigBg","ChipDir","ChipDirBg",
                            "DcaDirCard","DcaDirCardBar","DcaDirH","DcaLvlBuyL","DcaLvlSellL",
                            "DcaLvlBuyV","DcaLvlSellV",
                            "DcaPanelBG","DcaPanelH",
                            "DcaParamCard","DcaParamCardBar","DcaParamH",
                            "DcaModeL","DcaModeV","DcaMultL","DcaMultV","DcaMaxL","DcaMaxV",
                            "DcaDistL","DcaDistV","DcaTPL","DcaTPV","DcaSLL","DcaSLV",
                            "CloseCard","CloseCardBar","CloseH",
                            "CloseProfitL","CloseProfitV","CloseLossL","CloseLossV",
                            "ClosePipsL","ClosePipsV","CloseDMLossL","CloseDMLossV",
                            "CloseDMProfitL","CloseDMProfitV","CloseStL","CloseStV"};
    for(int li = 0; li < ArraySize(legacyObjs); li++) ObjectDelete(0, GUI + legacyObjs[li]);

    string bgt = GUI + "BGTitle";
    if(ObjectFind(0, bgt) < 0) {
        ObjectCreate(0, bgt, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bgt, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bgt, OBJPROP_BGCOLOR,     C'14,11,7');
        ObjectSetInteger(0, bgt, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bgt, OBJPROP_COLOR,       C'220,175,60');
        ObjectSetInteger(0, bgt, OBJPROP_WIDTH,       2);
        ObjectSetInteger(0, bgt, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bgt, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bgt, OBJPROP_XSIZE,     PW);
    ObjectSetInteger(0, bgt, OBJPROP_YSIZE,     titleOff - 4);
    ObjectSetInteger(0, bgt, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bgt, OBJPROP_YDISTANCE, PY);

    string titleObj = GUI + "T";
    if(ObjectFind(0, titleObj) < 0) {
        ObjectCreate(0, titleObj, OBJ_LABEL, 0, 0, 0);
        ObjectSetInteger(0, titleObj, OBJPROP_CORNER, CORNER_LEFT_UPPER);
        ObjectSetInteger(0, titleObj, OBJPROP_ANCHOR, ANCHOR_CENTER);
        ObjectSetString(0,  titleObj, OBJPROP_FONT,       "Arial Black");
        ObjectSetInteger(0, titleObj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, titleObj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, titleObj, OBJPROP_XDISTANCE, PX + PW/2);
    ObjectSetInteger(0, titleObj, OBJPROP_YDISTANCE, PY + (titleOff - 4) / 2);
    ObjectSetString(0,  titleObj, OBJPROP_TEXT,      "★ HOMIE BOT ★");
    ObjectSetInteger(0, titleObj, OBJPROP_COLOR,     C'255,200,60');
    ObjectSetInteger(0, titleObj, OBJPROP_FONTSIZE,  12);

    string bg = GUI + "BG";
    if(ObjectFind(0, bg) < 0) {
        ObjectCreate(0, bg, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg, OBJPROP_BGCOLOR,     C'14,20,32');
        ObjectSetInteger(0, bg, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg, OBJPROP_COLOR,       C'38,50,72');
        ObjectSetInteger(0, bg, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg, OBJPROP_YDISTANCE, PY + titleOff);

    int contentX = PX + 7, cardW = PW - 14, rightEdge = contentX + cardW - 8;
    int y2 = PY + titleOff + 6;
    int topRowY = y2;

    if(!g_PanelCollapsed) {
    y2 += 4;
    Lbl("TimeRow", tStr, contentX, y2, clrWhite, 12);
    ObjectSetString(0, GUI + "TimeRow", OBJPROP_FONT, "Consolas Bold");
    y2 += 22 + 8;
    } else {
        ObjectDelete(0, GUI + "TimeRow");
    }

    int acctH = 6 + 13 + 2*15 + 6;
    CreateRect("CardAcct",    contentX, y2, cardW, acctH, C'20,28,44');
    CreateRect("CardAcctBar", contentX, y2, 2,     acctH, C'79,195,217');
    Lbl("AcctH", "TÀI KHOẢN", contentX + 8, y2 + 5, C'95,108,132', 9);
    ObjectSetString(0, GUI + "AcctH", OBJPROP_FONT, "Calibri Bold");
    {
        bool expanded = !g_PanelCollapsed;
        color pcBg = expanded ? C'10,70,35'   : C'45,18,18';
        color pcBd = expanded ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnPanelToggle", g_PanelCollapsed ? " + " : " - ", rightEdge - 18, topRowY + 1, 18, 14, pcBg, pcBd);
        ObjectSetInteger(0, GUI + "BtnPanelToggle", OBJPROP_COLOR,    expanded ? clrWhite : C'160,160,160');
        ObjectSetString(0,  GUI + "BtnPanelToggle", OBJPROP_FONT,     "Consolas");
        ObjectSetInteger(0, GUI + "BtnPanelToggle", OBJPROP_FONTSIZE, 8);
    }
    int ya = y2 + 6 + 13;
    Lbl ("BalL", "Balance", contentX + 8, ya, C'127,139,163', 11);
    LblR("BalV", StringFormat("$%.2f", balance), rightEdge, ya, C'231,236,245', 11); ya += 15;
    Lbl ("FloL", "Float", contentX + 8, ya, C'127,139,163', 11);
    LblR("FloV", StringFormat("%s$%.2f (%.2f%%)", totalProfit >= 0 ? "+" : "-", MathAbs(totalProfit), pnlPct), rightEdge, ya, cProfit, 11);
    y2 += acctH + 8;

    if(!g_PanelCollapsed) {
    int rowH   = 15;
    int hdrGap = 6;
    int subH   = 14;
    color cardAccent = C'90,160,255';

    double ddPauseBotFloat = FloatProfit();
    color  ddPauseNowClr   = (ddPauseBotFloat <= g_DDPauseThreshold) ? clrTomato : C'231,236,245';
    string ddPauseStTxt    = g_PauseEA ? "ĐANG PAUSE" : "Bình thường";
    color  ddPauseStClr    = g_PauseEA ? clrTomato : clrLimeGreen;

    int ddPauseCardH = 6 + 13 + hdrGap + (18+4) + rowH*2 + 6;
    CreateRect("DDPauseCard",    contentX, y2, cardW, ddPauseCardH, C'20,28,44');
    CreateRect("DDPauseCardBar", contentX, y2, 2,     ddPauseCardH, cardAccent);
    Lbl("DDPauseH", "HEDGE & PAUSE", contentX + 8, y2 + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "DDPauseH", OBJPROP_FONT, "Calibri Bold");
    {
        int yDP = y2 + 6 + 13 + hdrGap;
        int    ddpBtnW  = (cardW - 16 - 6) / 2;
        string cbBtnTxt = g_DDPauseEnable ? "Cân Lệnh: On" : "Cân Lệnh: Off";
        color  cbBtnBg  = g_DDPauseEnable ? C'10,70,35'   : C'45,18,18';
        color  cbBtnBd  = g_DDPauseEnable ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnDDPauseToggle", cbBtnTxt, contentX + 8, yDP, ddpBtnW, 18, cbBtnBg, cbBtnBd);

        string peBtnTxt = g_PauseEA ? "Pause EA: On" : "Pause EA: Off";
        color  peBtnBg  = g_PauseEA ? C'10,70,35'   : C'45,18,18';
        color  peBtnBd  = g_PauseEA ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnPauseEAToggle", peBtnTxt, contentX + 8 + ddpBtnW + 6, yDP, ddpBtnW, 18, peBtnBg, peBtnBd);
        yDP += 18 + 4;

        Lbl ("DDPauseDDL", "DD hiện tại", contentX + 8, yDP, C'127,139,163', 10);
        LblR("DDPauseDDV", StringFormat("%s$%.2f / %s$%.2f",
                 ddPauseBotFloat    >= 0 ? "+" : "-", MathAbs(ddPauseBotFloat),
                 g_DDPauseThreshold >= 0 ? "+" : "-", MathAbs(g_DDPauseThreshold)),
             rightEdge, yDP, ddPauseNowClr, 10);
        yDP += rowH;

        Lbl ("DDPauseStL", "Trạng thái", contentX + 8, yDP, C'127,139,163', 10);
        LblR("DDPauseStV", ddPauseStTxt, rightEdge, yDP, ddPauseStClr, 10);
    }
    y2 += ddPauseCardH + 8;

    int manualCardH = 6 + 13 + hdrGap
                    + subH + rowH*2 + 18 + 4
                    + subH + 5 + 18 + 4
                    + subH + 5 + 18
                    + 6;
    CreateRect("ManualCard",    contentX, y2, cardW, manualCardH, C'20,28,44');
    CreateRect("ManualCardBar", contentX, y2, 2,     manualCardH, cardAccent);
    Lbl("ManualH", "MANUAL TRADE", contentX + 8, y2 + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "ManualH", OBJPROP_FONT, "Calibri Bold");
    {
        int yT = y2 + 6 + 13 + hdrGap;

        Lbl("ManualSubSLTP", StringFormat("THÔNG SỐ & SL (EMA %d)", InpManualSLEMAPeriod), contentX + 8, yT, C'90,102,127', 9);
        yT += subH;

        Lbl ("ManualLotL", "Lot lệnh tay", contentX + 8, yT, C'127,139,163', 10);
        LblR("ManualLotV", DoubleToString(InpManualLotSize, 2), rightEdge, yT, C'231,236,245', 10); yT += rowH;

        bool   slAvailable = g_ManualSL_Points > 0 && g_ManualAutoSLEnable;

        Lbl ("ManualSLL", "SL", contentX + 8, yT, slAvailable ? clrTomato : C'127,139,163', 10);
        LblR("ManualSLV", slAvailable ? StringFormat("%.0f pts", g_ManualSL_Points) : "Tắt", rightEdge, yT, slAvailable ? clrTomato : C'127,139,163', 10);
        yT += rowH;

        string manSLBtnTxt = g_ManualAutoSLEnable ? "Auto SL: On" : "Auto SL: Off";
        color  manSLBtnBg  = g_ManualAutoSLEnable ? C'10,70,35'   : C'45,18,18';
        color  manSLBtnBd  = g_ManualAutoSLEnable ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnManualSLToggle", manSLBtnTxt, contentX + 8, yT, cardW - 16, 18, manSLBtnBg, manSLBtnBd);
        yT += 18 + 4;

        Lbl("ManualSubTrail", "TRAILING STOP", contentX + 8, yT, C'90,102,127', 9);
        yT += subH + 5;

        string manTrailBtnTxt = g_ManualTrailEnable ? "Trailing: On" : "Trailing: Off";
        color  manTrailBtnBg  = g_ManualTrailEnable ? C'10,70,35'   : C'45,18,18';
        color  manTrailBtnBd  = g_ManualTrailEnable ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnManTrailToggle", manTrailBtnTxt, contentX + 8, yT, cardW - 16, 18, manTrailBtnBg, manTrailBtnBd);
        yT += 18 + 4;

        Lbl("ManualSubCloseAll", "TÍNH VÀO ĐÓNG TỔNG", contentX + 8, yT, C'90,102,127', 9);
        yT += subH + 5;

        string manCIMBtnTxt = g_CloseIncludeManual ? "Tính lệnh tay: On" : "Tính lệnh tay: Off";
        color  manCIMBtnBg  = g_CloseIncludeManual ? C'10,70,35'   : C'45,18,18';
        color  manCIMBtnBd  = g_CloseIncludeManual ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnCloseIncludeManual", manCIMBtnTxt, contentX + 8, yT, cardW - 16, 18, manCIMBtnBg, manCIMBtnBd);
    }
    y2 += manualCardH + 8;
    } else {
        string acctCollapsedObjs[] = {
            "DDPauseCard", "DDPauseCardBar", "DDPauseH", "BtnDDPauseToggle", "BtnPauseEAToggle",
            "DDPauseDDL", "DDPauseDDV", "DDPauseStL", "DDPauseStV",
            "ManualCard", "ManualCardBar", "ManualH", "ManualSubSLTP",
            "ManualLotL", "ManualLotV", "ManualSLL", "ManualSLV",
            "BtnManualSLToggle",
            "ManualSubTrail", "BtnManTrailToggle",
            "ManualSubCloseAll", "BtnCloseIncludeManual"
        };
        for(int aci = 0; aci < ArraySize(acctCollapsedObjs); aci++) ObjectDelete(0, GUI + acctCollapsedObjs[aci]);
    }

    int contentBottom = y2 + 3;
    int bg2Y = contentBottom + 4;

    if(!g_PanelCollapsed) {
    int bg3Y = bg2Y + 92 + 2;

    int y = bg2Y + 5;
    Lbl("P2T", "ĐIỀU KHIỂN LỆNH", x + 8, y, clrWhite, 9);
    ObjectSetString(0, GUI + "P2T", OBJPROP_FONT, "Calibri Bold"); y += s + 2;

    {
        string botTxt = g_BotEnabled ? "  EA Running" : "  EA Off";
        color  botBg  = g_BotEnabled ? C'0,90,30'   : C'120,20,20';
        color  botBd  = g_BotEnabled ? C'40,190,90' : C'220,60,60';
        CreateBtn("BtnBotToggle", botTxt, PX+7, y, bfw, bh, botBg, botBd);
        y += bh + 4;

        CreateBtn("BtnManualBuy",  "▲ BUY",  PX+7, y, bhw, bh, C'0,60,80',  C'40,150,200');
        CreateBtn("BtnManualSell", "▼ SELL", bx2,  y, bhw, bh, C'80,50,0',  C'200,140,40');
        y += bh + 4;
    }

    int y3 = bg3Y + 2;
    Lbl("P3T", "THỐNG KÊ", contentX + 8, y3, clrWhite, 9);
    ObjectSetString(0, GUI + "P3T", OBJPROP_FONT, "Calibri Bold");
    CreateBtn("BtnCalToggle", g_CalExpanded ? "« Đóng" : "Xem Lịch »", PX + PW - 82, y3 - 3, 76, 18, C'25,45,85', C'70,110,190');

    int bg3H = 34;

    ObjectDelete(0, GUI + "P4T");
    ObjectDelete(0, GUI + "BtnOpenBuy");
    ObjectDelete(0, GUI + "BtnOpenSell");
    g_LastPanelBottom = bg3Y + bg3H;

    } else {
        string collapsedObjs[] = {
            "P2T", "BtnBotToggle", "BtnManualBuy", "BtnManualSell",
            "P3T", "BtnCalToggle",
            "P4T", "BtnOpenBuy", "BtnOpenSell"
        };
        for(int ci = 0; ci < ArraySize(collapsedObjs); ci++) ObjectDelete(0, GUI + collapsedObjs[ci]);
        g_LastPanelBottom = contentBottom;
    }

    ObjectSetInteger(0, bg, OBJPROP_YSIZE, g_LastPanelBottom - (PY + titleOff));

    UpdateCalendarPanel(forceCalRefresh);

    ChartRedraw(0);
}

void RemoveGUI() {
    ObjectsDeleteAll(0, GUI);
}

void SetupChartColors() {
    ChartSetInteger(0, CHART_COLOR_BACKGROUND,  C'11,11,11');
    ChartSetInteger(0, CHART_COLOR_FOREGROUND,  clrMistyRose);
    ChartSetInteger(0, CHART_COLOR_GRID,        clrWhite);
    ChartSetInteger(0, CHART_COLOR_CHART_UP,    clrSeaGreen);
    ChartSetInteger(0, CHART_COLOR_CHART_DOWN,  clrYellow);
    ChartSetInteger(0, CHART_COLOR_CANDLE_BULL, clrSeaGreen);
    ChartSetInteger(0, CHART_COLOR_CANDLE_BEAR, clrYellow);
    ChartSetInteger(0, CHART_COLOR_CHART_LINE,  clrLime);
    ChartSetInteger(0, CHART_COLOR_VOLUME,      clrLimeGreen);
    ChartSetInteger(0, CHART_COLOR_BID,         clrLightSlateGray);
    ChartSetInteger(0, CHART_COLOR_ASK,         clrRed);
    ChartSetInteger(0, CHART_COLOR_LAST,        C'0,192,0');
    ChartSetInteger(0, CHART_COLOR_STOP_LEVEL,  clrRed);
    ChartSetInteger(0, CHART_SHOW_GRID,    false);
    ChartSetInteger(0, CHART_SHOW_VOLUMES, false);
    ChartRedraw(0);
}

void InitDCA() {
    DCA_Mult   = InpDCA1Mult;
    DCA_MaxOrd = InpDCA1Max;
    DCA_Dist   = InpDCA1Dist;
    DCA_TP     = InpDCA1TP;
    DCA_SL     = InpDCA1SL;
}

void ApplyBotEnabled(bool newVal) {
    if(g_BotEnabled && !newVal) {
        Print("RTB: Bot bị TẮT (BotEnabled=false) — đóng toàn bộ lệnh.");
        CloseAll();
        CloseAllManual();
    }
    g_BotEnabled = newVal;
}

int ParseSlotTag(string cmt) {
    string parts[];
    int np = StringSplit(cmt, '|', parts);
    for(int i = 0; i < np; i++) {
        if(StringLen(parts[i]) < 2 || StringGetCharacter(parts[i], 0) != 'S') continue;
        string digits = StringSubstr(parts[i], 1);
        bool allDigits = StringLen(digits) > 0;
        for(int c = 0; c < StringLen(digits) && allDigits; c++) {
            ushort ch = StringGetCharacter(digits, c);
            if(ch < '0' || ch > '9') allDigits = false;
        }
        if(allDigits) return (int)StringToInteger(digits);
    }
    return -1;
}

void RebuildDCAState(int posType) {
    int cap = (posType == POSITION_TYPE_BUY) ? ArraySize(DCABuyPrices) : ArraySize(DCASellPrices);

    double   slotPrices[];
    datetime slotTimes[];
    ulong    slotPosTk[];
    ulong    slotOrderTk[];
    int      slotTag[];
    ArrayResize(slotPrices, cap);
    ArrayResize(slotTimes, cap);
    ArrayResize(slotPosTk, cap);
    ArrayResize(slotOrderTk, cap);
    ArrayResize(slotTag, cap);
    int      count = 0;
    double   tol = 5.0 * _Point;

    for(int i = 0; i < PositionsTotal() && count < cap; i++) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        string parts[];
        int np = StringSplit(cmt, '|', parts);
        if(np == 3 && parts[1] == "0" && parts[2] == "0") {
            if(posType == POSITION_TYPE_BUY) OrigBuyPrice  = PositionGetDouble(POSITION_PRICE_OPEN);
            else                              OrigSellPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            continue;
        }

        slotPrices[count]  = PositionGetDouble(POSITION_PRICE_OPEN);
        slotTimes[count]   = (datetime)PositionGetInteger(POSITION_TIME);
        slotPosTk[count]   = tk;
        slotOrderTk[count] = 0;
        slotTag[count]     = ParseSlotTag(cmt);
        count++;
    }

    ulong  candTk[];
    double candPx[];
    ArrayResize(candTk, 0);
    ArrayResize(candPx, 0);

    ENUM_ORDER_TYPE pendType1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE pendType2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    for(int i = 0; i < OrdersTotal(); i++) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot != pendType1 && ot != pendType2) continue;
        string cmt = OrderGetString(ORDER_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        string oparts[];
        int onp = StringSplit(cmt, '|', oparts);
        if(onp == 3 && oparts[1] == "0" && oparts[2] == "0") continue;

        double oPrice = OrderGetDouble(ORDER_PRICE_OPEN);
        bool   dupOfOpenSlot = false;
        for(int s = 0; s < count; s++) {
            if(MathAbs(slotPrices[s] - oPrice) < tol) { dupOfOpenSlot = true; break; }
        }
        if(dupOfOpenSlot) {
            Trade.OrderDelete(tk);
            Print("RTB: RebuildDCAState huỷ pending dư thừa ticket=", tk, " giá=", oPrice);
            continue;
        }

        bool isRefill = (onp >= 1 && oparts[onp-1] == "RF");
        if(isRefill) {
            if(count >= cap) continue;
            slotPrices[count]  = oPrice;
            slotTimes[count]   = (datetime)OrderGetInteger(ORDER_TIME_SETUP);
            slotPosTk[count]   = 0;
            slotOrderTk[count] = tk;
            slotTag[count]     = ParseSlotTag(cmt);
            count++;
        } else {
            int nc = ArraySize(candTk);
            ArrayResize(candTk, nc + 1);
            ArrayResize(candPx, nc + 1);
            candTk[nc] = tk;
            candPx[nc] = oPrice;
        }
    }

    for(int i = 1; i < count; i++) {
        datetime kt = slotTimes[i]; double kp = slotPrices[i];
        ulong kpTk = slotPosTk[i];  ulong koTk = slotOrderTk[i];
        int kTag = slotTag[i];
        int j = i - 1;
        bool outOfOrder = (posType == POSITION_TYPE_BUY) ? (j >= 0 && slotPrices[j] < kp)
                                                          : (j >= 0 && slotPrices[j] > kp);
        while(outOfOrder) {
            slotTimes[j+1] = slotTimes[j]; slotPrices[j+1] = slotPrices[j];
            slotPosTk[j+1] = slotPosTk[j]; slotOrderTk[j+1] = slotOrderTk[j];
            slotTag[j+1] = slotTag[j];
            j--;
            outOfOrder = (posType == POSITION_TYPE_BUY) ? (j >= 0 && slotPrices[j] < kp)
                                                         : (j >= 0 && slotPrices[j] > kp);
        }
        slotTimes[j+1] = kt; slotPrices[j+1] = kp; slotPosTk[j+1] = kpTk; slotOrderTk[j+1] = koTk;
        slotTag[j+1] = kTag;
    }

    int realCount = count;
    int persisted = LoadPeak(posType);
    int finalCount = realCount;
    if(persisted > finalCount) finalCount = persisted;
    if(finalCount > cap) {
        Print("RTB: RebuildDCAState ", (posType==POSITION_TYPE_BUY?"BUY":"SELL"),
              " peak lưu trữ (", persisted, ") vượt cap mảng (", cap, ") — có thể do giảm InpDCA1Max, dùng cap.");
        finalCount = cap;
    }

    double   finalPrices[];
    ulong    finalPosTk[];
    ulong    finalOrderTk[];
    ArrayResize(finalPrices, finalCount);
    ArrayResize(finalPosTk, finalCount);
    ArrayResize(finalOrderTk, finalCount);
    ArrayInitialize(finalPrices, 0);
    ArrayInitialize(finalPosTk, 0);
    ArrayInitialize(finalOrderTk, 0);

    bool claimed[];
    ArrayResize(claimed, realCount);
    ArrayInitialize(claimed, false);

    int recoveredGaps = 0;
    for(int slot = 0; slot < finalCount; slot++) {
        double expected = LoadSlotPrice(posType, slot);
        int matchIdx = -1;
        for(int b = 0; b < realCount; b++) {
            if(claimed[b]) continue;
            if(slotTag[b] == slot) { matchIdx = b; break; }
        }
        if(matchIdx < 0 && expected > 0) {
            for(int b = 0; b < realCount; b++) {
                if(claimed[b]) continue;
                if(MathAbs(slotPrices[b] - expected) < tol) { matchIdx = b; break; }
            }
        }
        if(matchIdx >= 0) {
            finalPrices[slot]  = slotPrices[matchIdx];
            finalPosTk[slot]   = slotPosTk[matchIdx];
            finalOrderTk[slot] = slotOrderTk[matchIdx];
            claimed[matchIdx]  = true;
        } else if(expected > 0) {
            finalPrices[slot] = expected;
            recoveredGaps++;
        }
    }

    int nextUnclaimed = 0;
    for(int slot = 0; slot < finalCount; slot++) {
        if(finalPrices[slot] != 0) continue;
        while(nextUnclaimed < realCount && claimed[nextUnclaimed]) nextUnclaimed++;
        if(nextUnclaimed >= realCount) break;
        finalPrices[slot]  = slotPrices[nextUnclaimed];
        finalPosTk[slot]   = slotPosTk[nextUnclaimed];
        finalOrderTk[slot] = slotOrderTk[nextUnclaimed];
        claimed[nextUnclaimed] = true;
        nextUnclaimed++;
    }

    if(recoveredGaps > 0)
        Print("RTB: RebuildDCAState ", (posType==POSITION_TYPE_BUY?"BUY":"SELL"),
              " khôi phục giá của ", recoveredGaps, " tầng mất dấu trên broker (nhờ GlobalVariable) — refill đã tắt nên sẽ không tự đặt lại lệnh.");

    ulong  frontierTk = 0;
    double frontierPx = 0;
    int    nCand = ArraySize(candTk);
    if(nCand > 0) {
        double refPrice = (realCount > 0) ? slotPrices[realCount-1] : LastOpenPrice(posType);
        if(refPrice == 0)
            refPrice = (posType == POSITION_TYPE_BUY) ? SymbolInfoDouble(_Symbol, SYMBOL_ASK)
                                                       : SymbolInfoDouble(_Symbol, SYMBOL_BID);
        int    best     = -1;
        double bestDist = 0;
        for(int c = 0; c < nCand; c++) {
            double d = MathAbs(candPx[c] - refPrice);
            if(best < 0 || d < bestDist) { best = c; bestDist = d; }
        }
        for(int c = 0; c < nCand; c++) {
            if(c == best) continue;
            if(Trade.OrderDelete(candTk[c]))
                Print("RTB: RebuildDCAState huỷ lệnh chờ tầng-kế-tiếp DƯ THỪA (phantom) ticket=", candTk[c], " giá=", candPx[c]);
            else
                Print("RTB: RebuildDCAState không huỷ được lệnh chờ dư thừa ticket=", candTk[c], ", err=", GetLastError());
        }
        if(best >= 0) {
            if(finalCount < cap) { frontierTk = candTk[best]; frontierPx = candPx[best]; }
            else {
                if(Trade.OrderDelete(candTk[best]))
                    Print("RTB: RebuildDCAState huỷ lệnh chờ tầng-kế-tiếp (đã đạt cap=", cap, ") ticket=", candTk[best], " giá=", candPx[best]);
                else
                    Print("RTB: RebuildDCAState không huỷ được lệnh chờ tầng-kế-tiếp (đã đạt cap) ticket=", candTk[best], ", err=", GetLastError());
            }
        }
    }

    if(posType == POSITION_TYPE_BUY) {
        PeakDCABuy = finalCount;
        for(int s = 0; s < finalCount; s++) {
            DCABuyPrices[s]    = finalPrices[s];
            DCABuyTickets[s]   = finalPosTk[s];
            DCABuyLimitTk[s]   = finalOrderTk[s];
            DCABuyBounced[s]   = false;
        }
        if(frontierTk > 0) { DCABuyPrices[finalCount] = frontierPx; DCABuyLimitTk[finalCount] = frontierTk; }
    } else {
        PeakDCASell = finalCount;
        for(int s = 0; s < finalCount; s++) {
            DCASellPrices[s]    = finalPrices[s];
            DCASellTickets[s]   = finalPosTk[s];
            DCASellLimitTk[s]   = finalOrderTk[s];
            DCASellBounced[s]   = false;
        }
        if(frontierTk > 0) { DCASellPrices[finalCount] = frontierPx; DCASellLimitTk[finalCount] = frontierTk; }
    }
    for(int s = 0; s < finalCount; s++) SaveSlotPrice(posType, s, finalPrices[s]);
    SavePeak(posType, finalCount);
    Print("RTB: RebuildDCAState ", (posType==POSITION_TYPE_BUY?"BUY":"SELL"), " peak=", finalCount,
          (frontierTk > 0 ? " (+1 lệnh chờ tầng kế tiếp)" : ""));
}

int OnInit() {
    g_Direction = InpDirection;

    hManualSLEMA = iMA(_Symbol, PERIOD_CURRENT, InpManualSLEMAPeriod, 0, MODE_EMA, PRICE_CLOSE);
    if(hManualSLEMA == INVALID_HANDLE) {
        Print("RTB: Không tạo được handle EMA lọc Auto SL lệnh tay (period=", InpManualSLEMAPeriod, ").");
        return INIT_FAILED;
    }

    g_ManualSL_Points = InpManualSL_Points;
    g_OrderDelay = InpOrderDelay;
    g_DCAArithEnable = InpDCAArithEnable; g_DCAArithStep = InpDCAArithStep;
    g_TrailEnable = InpTrailEnable; g_TrailMinOrds = InpTrailMinOrds;
    g_TrailActivate = InpTrailActivate; g_TrailStep = InpTrailStep; g_TrailInit = InpTrailInit;
    g_ManualTrailEnable = InpManualTrailEnable; g_ManualTrailMinOrds = InpManualTrailMinOrds;
    g_ManualTrailActivate = InpManualTrailActivate; g_ManualTrailStep = InpManualTrailStep; g_ManualTrailInit = InpManualTrailInit;
    g_CloseProfit = InpCloseProfit; g_CloseLoss = InpCloseLoss; g_ClosePerPips = InpClosePerPips;
    g_DayMaxLoss = InpDayMaxLoss; g_DayMaxProfit = InpDayMaxProfit;

    g_DDPauseEnable    = InpDDPauseEnable;
    g_DDPauseThreshold = InpDDPauseThreshold;
    g_PauseEA          = false;

    g_BotEnabled = InpBotEnabled;

    Trade.SetExpertMagicNumber(InpMagic);
    Trade.SetDeviationInPoints(50);
    Trade.SetTypeFilling(ORDER_FILLING_RETURN);

    SetupChartColors();

    InitDCA();

    InitBalance    = AccountInfoDouble(ACCOUNT_BALANCE);
    {
        MqlDateTime nowDt;
        TimeToStruct(TimeCurrent(), nowDt);
        g_CalYear  = nowDt.year;
        g_CalMonth = nowDt.mon;
    }
    ArrayResize(DCABuyPrices,    DCA_MaxOrd);
    ArrayResize(DCABuyBounced,   DCA_MaxOrd);
    ArrayResize(DCABuyTickets,   DCA_MaxOrd);
    ArrayResize(DCABuyLimitTk,   DCA_MaxOrd);
    ArrayResize(DCASellPrices,    DCA_MaxOrd);
    ArrayResize(DCASellBounced,   DCA_MaxOrd);
    ArrayResize(DCASellTickets,   DCA_MaxOrd);
    ArrayResize(DCASellLimitTk,   DCA_MaxOrd);
    ArrayInitialize(DCABuyPrices,   0);
    ArrayInitialize(DCASellPrices,  0);
    ArrayInitialize(DCABuyBounced,  false);
    ArrayInitialize(DCASellBounced, false);
    ArrayInitialize(DCABuyTickets,  0);
    ArrayInitialize(DCASellTickets, 0);
    ArrayInitialize(DCABuyLimitTk,  0);
    ArrayInitialize(DCASellLimitTk, 0);
    PeakDCABuy  = 0;
    PeakDCASell = 0;
    OrigBuyPrice = 0; OrigSellPrice = 0;
    RebuildDCAState(POSITION_TYPE_BUY);
    RebuildDCAState(POSITION_TYPE_SELL);
    LastEntryTime  = 0;
    LastDay        = -1;

    EventSetTimer(1);

    Print("RTB: Initialized. Magic=", InpMagic);
    return INIT_SUCCEEDED;
}

void OnDeinit(const int reason) {
    EventKillTimer();
    RemoveGUI();
    if(hManualSLEMA != INVALID_HANDLE) IndicatorRelease(hManualSLEMA);
}

void OnTick() {
    if(!g_PauseEA) CheckEntry();
    if(!DayLimitHit) {
        if(!g_PauseEA) CheckTrailing();
        CheckManualTrailing();
    }
}

void OnTimer() {
    UpdateDayProfit();
    CheckDayLimit();
    CheckDDPause();

    if(CountBuy()  == 0 && !HasPendingDCA(POSITION_TYPE_BUY))  ResetDCAState(POSITION_TYPE_BUY);
    if(CountSell() == 0 && !HasPendingDCA(POSITION_TYPE_SELL)) ResetDCAState(POSITION_TYPE_SELL);

    CheckManualAutoSL();

    CheckExit();

    if(!DayLimitHit && g_BotEnabled) {
        if(!g_PauseEA) {
            CheckOrigRestart(POSITION_TYPE_BUY);
            CheckOrigRestart(POSITION_TYPE_SELL);
        }

        if(!g_PauseEA) {
            if(CountBuy()  > 0) CheckDCA(POSITION_TYPE_BUY);
            if(CountSell() > 0) CheckDCA(POSITION_TYPE_SELL);
        }
    }

    UpdateGUI();
}

void OnTradeTransaction(const MqlTradeTransaction& trans,
                        const MqlTradeRequest&     req,
                        const MqlTradeResult&      res) {
    if(trans.type == TRADE_TRANSACTION_DEAL_ADD) {
        UpdateDayProfit();
        CheckDayLimit();
        CheckManualAutoSL();
        UpdateGUI(true);
    }
}

void OnChartEvent(const int id, const long& lparam, const double& dparam, const string& sparam) {
    if(id != CHARTEVENT_OBJECT_CLICK) return;
    if(sparam == GUI + "BtnBotToggle") {
        if(TimeCurrent() - g_LastBotToggleClick < 2) {
            Print("RTB: Bỏ qua click Bot Toggle — bấm quá nhanh (debounce 2 giây).");
        } else {
            g_LastBotToggleClick = TimeCurrent();
            ApplyBotEnabled(!g_BotEnabled);
            UpdateGUI();
        }
    }
    else if(sparam == GUI + "BtnManTrailToggle") {
        g_ManualTrailEnable = !g_ManualTrailEnable;
        Print("RTB: Trailing lệnh tay ", (g_ManualTrailEnable ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnManualSLToggle") {
        g_ManualAutoSLEnable = !g_ManualAutoSLEnable;
        Print("RTB: Auto SL (EMA) lệnh tay ", (g_ManualAutoSLEnable ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnDDPauseToggle") {
        g_DDPauseEnable = !g_DDPauseEnable;
        Print("RTB: Cân Lệnh theo DD ", (g_DDPauseEnable ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnPauseEAToggle") {
        g_PauseEA = !g_PauseEA;
        Print("RTB: Pause EA ", (g_PauseEA ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnCloseIncludeManual") {
        g_CloseIncludeManual = !g_CloseIncludeManual;
        Print("RTB: Tính lệnh tay vào Float Đóng Lệnh Tổng ", (g_CloseIncludeManual ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnManualBuy") {
        if(!DayLimitHit && g_BotEnabled) OpenManualOrder(ORDER_TYPE_BUY);
    }
    else if(sparam == GUI + "BtnManualSell") {
        if(!DayLimitHit && g_BotEnabled) OpenManualOrder(ORDER_TYPE_SELL);
    }
    else if(sparam == GUI + "BtnPanelToggle") {
        g_PanelCollapsed = !g_PanelCollapsed;
        if(g_PanelCollapsed) g_CalExpanded = false;
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnCalToggle") {
        g_CalExpanded = !g_CalExpanded;
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnCalPrev") {
        g_CalMonth--;
        if(g_CalMonth < 1) { g_CalMonth = 12; g_CalYear--; }
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnCalNext") {
        g_CalMonth++;
        if(g_CalMonth > 12) { g_CalMonth = 1; g_CalYear++; }
        UpdateGUI();
    }
    else return;
    ObjectSetInteger(0, sparam, OBJPROP_STATE, false);
    ChartRedraw(0);
}
