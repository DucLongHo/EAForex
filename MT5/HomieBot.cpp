//+------------------------------------------------------------------+
//|                                              HomieBot.mq5        |
//|                             Homie Bot v1.0 (MQL5)                 |
//|   Bản rút gọn — không Pyramiding (Nhồi Dương), không License/Sync |
//+------------------------------------------------------------------+
#property copyright "Homie Bot v1.0"
#property version   "1.00"
#property strict

#include <Trade\Trade.mqh>
#include <Trade\PositionInfo.mqh>

CTrade    Trade;

//+------------------------------------------------------------------+
//| ENUMS                                                            |
//+------------------------------------------------------------------+
enum ENUM_SIGNAL_MODE  { SIG_SIMULATED, SIG_UT_BOT };
enum ENUM_DIRECTION    { DIR_BOTH, DIR_ONLY_BUY, DIR_ONLY_SELL };
enum ENUM_DCA_MODE     { DCA_STOP, DCA_STEP, DCA_STEP_TF };
enum ENUM_TRAIL_MODE   { TRAIL_BASKET, TRAIL_SINGLE };
enum ENUM_BOT_MODE     { MODE_AUTO, MODE_SEMI_AUTO };
enum ENUM_TRIM_MODE    { TRIM_OFF, TRIM_HEDGE, TRIM_HEDGE_PTS };

//+------------------------------------------------------------------+
//| INPUT: BASE SETTINGS                                             |
//+------------------------------------------------------------------+
input group         "══════ CÀI ĐẶT CƠ BẢN ══════"; //
input  ENUM_BOT_MODE InpBotMode = MODE_AUTO;  // Chế độ: Tự động / Bán tự động
input  bool    InpBotEnabled   = true;    // Bật Bot (tắt = đóng toàn bộ lệnh + dừng mọi hoạt động)
input  double  InpLotSize      = 0.01;    // Lots ban đầu
input  bool    InpUseTakeProfit= true;    // Dùng Take Profit (Use_TP)
input  bool    InpUseStopLoss  = false;   // Dùng Stop Loss (Use_SL)
input  bool    InpStealthMode  = false;   // Ẩn TP/SL trên chart (Stealth Mode)
input  int     InpOrderDelay   = 5;       // Độ trễ mở lệnh (giây)
input  ulong   InpMagic        = 202601;  // Magic Number
input  double  InpTP_Points    = 3000.0;  // TP mỗi lệnh (points)
input  double  InpSL_Points    = 0.0;     // SL mỗi lệnh (points, 0=tắt)

//+------------------------------------------------------------------+
//| INPUT: ENTRY SIGNAL                                              |
//+------------------------------------------------------------------+
input group         "══════ TÍN HIỆU VÀO LỆNH ══════"; //
input  ENUM_SIGNAL_MODE InpSignalMode = SIG_UT_BOT;   // Chiến lược tín hiệu
input  ENUM_DIRECTION   InpDirection  = DIR_BOTH;     // Hướng giao dịch
input  ENUM_TIMEFRAMES  InpSignalTF   = PERIOD_H1;    // Khung thời gian tín hiệu

//+------------------------------------------------------------------+
//| INPUT: UT BOT                                                    |
//+------------------------------------------------------------------+
input group         "══════ UT BOT ══════"; //
input  int     InpUTKeyValue  = 1;    // Key Value (độ nhạy ATR)
input  int     InpUTATRPeriod = 10;   // ATR Period

//+------------------------------------------------------------------+
//| INPUT: DCA                                                       |
//+------------------------------------------------------------------+
input group         "══════ DCA - CÀI ĐẶT CHUNG ══════"; //
input  ENUM_DCA_MODE InpDCAMode     = DCA_STEP; // DCA: Chế độ
input  bool          InpDCABuyEnable  = true;   // DCA: Bật DCA chiều Buy
input  bool          InpDCASellEnable = true;   // DCA: Bật DCA chiều Sell
input  bool          InpDCAArithEnable = false; // DCA: Bật Vol Cấp Số Cộng (bỏ qua Hệ số Lot)
input  double        InpDCAArithStep   = 0.01;  // DCA: Cộng thêm Vol mỗi lệnh DCA sau (lots)

input group         "══════ DCA ══════"; //
input  double  InpDCA1Mult = 1.5;    // DCA: Hệ số Lot
input  int     InpDCA1Max  = 2;      // DCA: Max lệnh DCA tối đa
input  double  InpDCA1Dist = 1000.0; // DCA: Khoảng cách (points)
input  double  InpDCA1TP   = 500.0;  // DCA: TP (points)
input  double  InpDCA1SL   = 0.0;    // DCA: SL (points, 0=tắt)

//+------------------------------------------------------------------+
//| INPUT: ORDER TRIMMING                                            |
//+------------------------------------------------------------------+
input group         "══════ TỈA LỆNH (TRIMMING) ══════"; //
input  ENUM_TRIM_MODE InpTrimMode    = TRIM_OFF; // Chế độ (Off/Hedge/Hedge theo điểm)
input  int     InpTrimTrigger    = 5;      // Kích hoạt khi số lệnh >= X
input  double  InpTrimTarget     = 10.0;   // [Hedge/HedgePts] Mục tiêu lợi nhuận sau tỉa ($)
input  int     InpTrimMaxLoss    = 1;      // Số lệnh âm tối đa gộp mỗi lần ghép cặp
input  int     InpTrimMaxWin     = 1;      // [Hedge] Số lệnh dương tối đa gộp mỗi lần ghép cặp
input  int     InpTrimMaxCycles  = 1;      // [Hedge] Số chu kỳ ghép cặp tối đa mỗi lượt tỉa
input  bool    InpTrimIncludeManual = false; // Cho phép lệnh tay (Magic=0) tham gia tỉa kể cả ở chế độ Tự Động

//+------------------------------------------------------------------+
//| INPUT: TRAILING STOP                                             |
//+------------------------------------------------------------------+
input group         "══════ TRAILING STOP ══════"; //
input  bool          InpTrailEnable   = false;        // Bật Trailing
input  ENUM_TRAIL_MODE InpTrailMode   = TRAIL_BASKET; // Basket hoặc Đơn lẻ
input  int           InpTrailMinOrds  = 1;            // Số lệnh tối thiểu kích hoạt
input  double        InpTrailActivate = 500.0;        // Points kích hoạt Trail
input  double        InpTrailStep     = 200.0;        // Bước nhảy SL (points)
input  double        InpTrailInit     = 300.0;        // SL đầu tiên cách giá (points)
input  bool          InpTrailShowLine = true;         // Vẽ đường Trail
input  color         InpTrailBuyColor = clrLimeGreen; // Màu đường Trail Buy
input  color         InpTrailSellColor= clrTomato;    // Màu đường Trail Sell
input  int           InpTrailLineWidth= 2;            // Độ dày đường Trail (1-5)

//+------------------------------------------------------------------+
//| INPUT: EXIT LOGIC                                                |
//+------------------------------------------------------------------+
input group         "══════ ĐÓNG LỆNH TỔNG ══════"; //
input  double  InpCloseProfit  = 0.0;  // Chốt lời khi tổng lãi đạt ($, 0=tắt)
input  double  InpCloseLoss    = 0.0;  // Cắt lỗ khi tổng lỗ đạt ($, 0=tắt)
input  double  InpClosePerPips = 0.0;  // Đóng từng lệnh khi đạt (points, 0=tắt)
input  double  InpDayMaxLoss   = 0.0;  // Dừng bot khi lỗ ngày đạt ($, 0=tắt)
input  double  InpDayMaxProfit = 0.0;  // Dừng bot khi lãi ngày đạt ($, 0=tắt)

//+------------------------------------------------------------------+
//| INPUT: TELEGRAM ALERT                                            |
//+------------------------------------------------------------------+
input group         "══════ TELEGRAM ALERT ══════"; //
input  bool    InpTeleEnable   = false; // Bật báo Telegram
input  string  InpTeleBotToken = "8854647005:AAGgjy92keWNlVs9RhnBp_F_eP6sFxU4qOg";    // Bot Token
input  string  InpTeleChatID   = "-5513792765";    // Chat ID
input  double  InpTeleDDStep   = 20.0;  // Báo khi DD vượt mỗi X% (0=tắt)

//+------------------------------------------------------------------+
//| INPUT: PANEL                                                     |
//+------------------------------------------------------------------+
input group         "══════ PANEL ══════"; //
input  bool    InpShowPanel  = true;  // Hiện panel
input  int     InpPanelX     = 5;     // Panel: tọa độ X
input  int     InpPanelY     = 18;    // Panel: tọa độ Y
input  int     InpPanelWidth = 252;   // Panel: chiều rộng
input  int     InpCalPanelGap = 12;   // Lịch: khoảng cách với panel chính
input  int     InpCalPanelY  = 18;    // Lịch: tọa độ Y

//+------------------------------------------------------------------+
//| GLOBAL STATE                                                     |
//+------------------------------------------------------------------+
int      hATR       = INVALID_HANDLE;

double   g_ats_ut        = 0.0;
int      g_ats_ut_signal = 0;
datetime g_last_bar_ut   = 0;

ENUM_DCA_MODE DCA_Mode;
double        DCA_Mult;
int           DCA_MaxOrd;
double        DCA_Dist;
double        DCA_TP;
double        DCA_SL;

datetime LastOrderTime  = 0;
datetime LastEntryTime  = 0;
double   InitBalance    = 0.0;
double   MaxDrawdownPct = 0.0;
double   DayProfit      = 0.0;
int      LastDay        = -1;
bool     DayLimitHit    = false;

double   TrailBuy  = 0.0;
double   TrailSell = 0.0;

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

const string GUI = "RTB_";

bool     g_BotEnabled           = true;
datetime g_LastBotToggleClick   = 0;

ENUM_SIGNAL_MODE g_SignalMode;
ENUM_DIRECTION   g_Direction;
int              g_UTKeyValue;

bool   g_UseTakeProfit, g_UseStopLoss, g_StealthMode;
int    g_OrderDelay;
double g_TP_Points, g_SL_Points;

bool   g_DCABuyEnable, g_DCASellEnable, g_DCAArithEnable;
double g_DCAArithStep;

ENUM_TRIM_MODE g_TrimMode;
int    g_TrimTrigger, g_TrimMaxLoss, g_TrimMaxWin, g_TrimMaxCycles;
double g_TrimTarget;
bool   g_TrimIncludeManual;

bool             g_TrailEnable;
ENUM_TRAIL_MODE  g_TrailMode;
int              g_TrailMinOrds;
double           g_TrailActivate, g_TrailStep, g_TrailInit;

double g_CloseProfit, g_CloseLoss, g_ClosePerPips, g_DayMaxLoss, g_DayMaxProfit;

bool   g_TeleEnable;
string g_TeleBotToken, g_TeleChatID;
double g_TeleDDStep;
int    g_LastDDAlertLevel = 0;
string g_TeleQueue[];

//+------------------------------------------------------------------+
//| UTILITY FUNCTIONS                                                |
//+------------------------------------------------------------------+

bool IsManaged() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    if(magic == (long)InpMagic) return true;
    if(InpBotMode == MODE_SEMI_AUTO && magic == 0) return true;
    return false;
}

// Dùng riêng cho hệ thống Tỉa Lệnh — khi InpTrimIncludeManual=true, lệnh tay
// (Magic=0) được tính là "quản lý" cho mục đích tỉa NGAY CẢ ở chế độ Tự Động
// (bình thường IsManaged() chỉ chấp nhận Magic=0 ở chế độ Bán Tự Động).
bool IsManagedForTrim() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    if(magic == (long)InpMagic) return true;
    if((InpBotMode == MODE_SEMI_AUTO || g_TrimIncludeManual) && magic == 0) return true;
    return false;
}

// Dùng riêng cho panel GUI — luôn tính cả lệnh tay (Magic=0) bất kể InpBotMode,
// để panel phản ánh đúng toàn bộ vị thế đang có trên symbol này (chỉ hiển thị,
// không ảnh hưởng logic vào/tỉa/đóng lệnh nào cả).
bool IsManagedForDisplay() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    return magic == (long)InpMagic || magic == 0;
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

// Đếm riêng cho Tỉa Lệnh — dùng IsManagedForTrim() thay vì IsManaged(), để lệnh
// tay được tính vào ngưỡng InpTrimTrigger khi InpTrimIncludeManual=true.
int CountAllForTrim() {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForTrim()) continue;
        n++;
    }
    return n;
}
int CountAll()  { return CountBuy() + CountSell(); }

int CountManual(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        n++;
    }
    return n;
}

double FloatProfit(int posType = -1, bool includeManual = false) {
    double p = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        bool managed = includeManual ? IsManagedForTrim() : IsManaged();
        if(!managed) continue;
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
        bool managed = (magic == (long)InpMagic) || (InpBotMode == MODE_SEMI_AUTO && magic == 0);
        if(!managed) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        bool isBuyType  = (ot == ORDER_TYPE_BUY_LIMIT  || ot == ORDER_TYPE_BUY_STOP);
        bool isSellType = (ot == ORDER_TYPE_SELL_LIMIT || ot == ORDER_TYPE_SELL_STOP);
        if(!isBuyType && !isSellType) continue;
        if(posType == POSITION_TYPE_BUY  && !isBuyType)  continue;
        if(posType == POSITION_TYPE_SELL && !isSellType) continue;
        Trade.OrderDelete(otk);
    }

    if(posType < 0 || posType == POSITION_TYPE_BUY) {
        TrailBuy = 0; PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
        OrigBuyPrice = 0;
        ClearSlotPrices(POSITION_TYPE_BUY, ArraySize(DCABuyPrices));
        SavePeak(POSITION_TYPE_BUY, 0);
    }
    if(posType < 0 || posType == POSITION_TYPE_SELL) {
        TrailSell = 0; PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
        OrigSellPrice = 0;
        ClearSlotPrices(POSITION_TYPE_SELL, ArraySize(DCASellPrices));
        SavePeak(POSITION_TYPE_SELL, 0);
    }
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

//+------------------------------------------------------------------+
//| OPEN ORDER                                                       |
//+------------------------------------------------------------------+
bool OpenOrder(int ordType, double lot, double tp_pts = 0, double sl_pts = 0, bool isDCA = false) {
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    double price, tp = 0, sl = 0;

    bool applyTP  = isDCA ? (tp_pts > 0) : (g_UseTakeProfit && tp_pts > 0);
    bool applySL  = isDCA ? (sl_pts > 0) : (g_UseStopLoss   && sl_pts > 0);

    if(ordType == ORDER_TYPE_BUY) {
        price = ask;
        if(applyTP && !g_StealthMode)
            tp = NormalizeDouble(price + tp_pts * point, _Digits);
        if(applySL && !g_StealthMode)
            sl = NormalizeDouble(price - sl_pts * point, _Digits);
    } else {
        price = bid;
        if(applyTP && !g_StealthMode)
            tp = NormalizeDouble(price - tp_pts * point, _Digits);
        if(applySL && !g_StealthMode)
            sl = NormalizeDouble(price + sl_pts * point, _Digits);
    }

    string comment;
    if(isDCA) {
        if(tp_pts == 0 && sl_pts == 0)
            comment = "RTB|0|0|D";
        else
            comment = StringFormat("RTB|%.0f|%.0f", tp_pts, sl_pts);
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

//+------------------------------------------------------------------+
//| ENTRY SIGNALS                                                    |
//+------------------------------------------------------------------+

void WarmupATS(int bars) {
    int need = bars + 5;
    double atr[], cls[];
    ArraySetAsSeries(atr, true);
    ArraySetAsSeries(cls, true);
    if(CopyBuffer(hATR, 0, 0, need, atr) < need ||
       CopyClose(_Symbol, InpSignalTF, 0, need, cls) < need) return;
    double ats = 0.0;
    for(int i = bars; i >= 1; i--) {
        double s=cls[i], sp=cls[i+1], nl=g_UTKeyValue*atr[i];
        if     (s>ats&&sp>ats) ats=MathMax(ats,s-nl);
        else if(s<ats&&sp<ats) ats=MathMin(ats,s+nl);
        else if(s>ats)          ats=s-nl;
        else                    ats=s+nl;
    }
    g_ats_ut = ats;
}

int SignalUTBot() {
    datetime t0 = iTime(_Symbol, InpSignalTF, 0);
    if(t0 == g_last_bar_ut) return g_ats_ut_signal;
    g_last_bar_ut = t0;

    double atr[];
    ArraySetAsSeries(atr, true);
    if(CopyBuffer(hATR, 0, 1, 1, atr) < 1) { g_ats_ut_signal = 0; return 0; }

    double src      = iClose(_Symbol, InpSignalTF, 1);
    double src_prev = iClose(_Symbol, InpSignalTF, 2);
    double nLoss    = g_UTKeyValue * atr[0];
    double ats_prev = g_ats_ut;

    if     (src > ats_prev && src_prev > ats_prev) g_ats_ut = MathMax(ats_prev, src - nLoss);
    else if(src < ats_prev && src_prev < ats_prev) g_ats_ut = MathMin(ats_prev, src + nLoss);
    else if(src > ats_prev)                         g_ats_ut = src - nLoss;
    else                                            g_ats_ut = src + nLoss;

    g_ats_ut_signal = 0;
    if(src > g_ats_ut && src_prev <= ats_prev) g_ats_ut_signal =  1;
    if(src < g_ats_ut && src_prev >= ats_prev) g_ats_ut_signal = -1;
    return g_ats_ut_signal;
}

int SignalSimulated() {
    if(g_Direction == DIR_ONLY_BUY)  return  1;
    if(g_Direction == DIR_ONLY_SELL) return -1;
    return 2;
}

int GetSignal() {
    int sig = 0;
    switch(g_SignalMode) {
        case SIG_SIMULATED: sig = SignalSimulated(); break;
        case SIG_UT_BOT:    sig = SignalUTBot();    break;
    }
    if(g_SignalMode != SIG_SIMULATED) {
        if(g_Direction == DIR_ONLY_BUY  && sig < 0) return 0;
        if(g_Direction == DIR_ONLY_SELL && sig > 0) return 0;
    }
    return sig;
}

//+------------------------------------------------------------------+
//| PEAK PERSISTENCE — sống sót qua restart, tránh mất tầng nếu       |
//| restart rơi đúng lúc 1 tầng vừa đóng nhưng refill chưa kịp đặt   |
//+------------------------------------------------------------------+
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

//+------------------------------------------------------------------+
//| INITIAL ENTRY (OnTick)                                           |
//+------------------------------------------------------------------+
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
        TrailBuy = 0; PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
        ClearSlotPrices(posType, ArraySize(DCABuyPrices));
    } else {
        TrailSell = 0; PeakDCASell = 0;
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
    if(OpenOrder(ORDER_TYPE_BUY, InpLotSize, g_TP_Points, g_SL_Points)) {
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
    if(OpenOrder(ORDER_TYPE_SELL, InpLotSize, g_TP_Points, g_SL_Points)) {
        LastEntryTime = TimeCurrent();
        ulong tk = Trade.ResultOrder();
        OrigSellPrice = (tk > 0 && PositionSelectByTicket(tk))
            ? PositionGetDouble(POSITION_PRICE_OPEN) : SymbolInfoDouble(_Symbol, SYMBOL_BID);
    }
}

void CheckEntry() {
    if(DayLimitHit) return;
    if(!g_BotEnabled) return;
    if(InpBotMode == MODE_SEMI_AUTO) return;
    if(TimeCurrent() - LastEntryTime < g_OrderDelay) return;

    int sig = GetSignal();
    if(sig == 0) return;

    if(sig == 2) {
        TryOpenBuy();
        TryOpenSell();
        return;
    }

    if(sig > 0) TryOpenBuy();
    if(sig < 0) TryOpenSell();
}

//+------------------------------------------------------------------+
//| DCA PRIMARY CHAIN HELPERS (Semi-Auto multi-entry)               |
//+------------------------------------------------------------------+

int CountBotDCA(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        n++;
    }
    if(InpBotMode != MODE_SEMI_AUTO) n = MathMax(0, n - 1);
    return n;
}

double OldestManualPrice(int posType) {
    double   price  = 0;
    datetime oldest = (datetime)0x7FFFFFFF;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t < oldest) { oldest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    return price;
}

double OldestManualLot(int posType) {
    double   lot    = 0;
    datetime oldest = (datetime)0x7FFFFFFF;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t < oldest) { oldest = t; lot = PositionGetDouble(POSITION_VOLUME); }
    }
    return lot;
}

double LastPrimaryPrice(int posType) {
    double   price  = 0;
    datetime latest = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        string parts[];
        if(StringSplit(cmt, '|', parts) < 3) continue;
        if(StringToDouble(parts[1]) == 0 && StringToDouble(parts[2]) == 0) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t > latest) { latest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    if(price > 0) return price;
    return OldestManualPrice(posType);
}

//+------------------------------------------------------------------+
//| DCA LOGIC                                                        |
//+------------------------------------------------------------------+

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


void CheckDCA(int posType) {
    if(posType == POSITION_TYPE_BUY  && !g_DCABuyEnable)  return;
    if(posType == POSITION_TYPE_SELL && !g_DCASellEnable) return;

    int count = CountPos(posType);
    if(count == 0) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    int maxOrds  = DCA_MaxOrd + 1;

    bool multiManual = (InpBotMode == MODE_SEMI_AUTO && CountManual(posType) >= 1);
    if(multiManual) {
        int    dcaCount  = CountBotDCA(posType);
        double lastPrice = LastPrimaryPrice(posType);
        if(lastPrice == 0) return;

        if(dcaCount >= DCA_MaxOrd || DCA_Mode == DCA_STOP) return;
        if(count >= maxOrds) return;

        double dist = DCA_Dist * point;
        bool trigger = (posType == POSITION_TYPE_BUY) ? (lastPrice - bid) >= dist
                                                      : (ask - lastPrice) >= dist;
        if(!trigger) return;
        if(DCA_Mode == DCA_STEP_TF) {
            int sig = GetSignal();
            if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
            if(posType == POSITION_TYPE_SELL && sig != -1) return;
        }
        if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

        double baseLot = OldestManualLot(posType);
        if(baseLot <= 0) baseLot = InpLotSize;
        double lot = DCAOrderLot(baseLot, dcaCount + 1);
        int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
        Print("RTB: DCA triggered dcaCount=", dcaCount, " [primary chain only]");
        OpenOrder(ord, lot, DCA_TP, DCA_SL, true);
        return;
    }

    int peak = (posType == POSITION_TYPE_BUY) ? PeakDCABuy : PeakDCASell;

    for(int slot = 0; slot < peak; slot++) {
        double slotPrice   = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot]   : DCASellPrices[slot];
        bool   slotBounced = (posType == POSITION_TYPE_BUY) ? DCABuyBounced[slot]  : DCASellBounced[slot];
        ulong  limitTk     = (posType == POSITION_TYPE_BUY) ? DCABuyLimitTk[slot]  : DCASellLimitTk[slot];
        if(slotPrice == 0) continue;

        bool isOpen = IsSlotOpen(posType, slot);

        if(!isOpen) {
            if(limitTk > 0) {
                if(PositionSelectByTicket(limitTk)) {
                    if(posType == POSITION_TYPE_BUY) { DCABuyTickets[slot]  = limitTk; DCABuyLimitTk[slot]  = 0; DCABuyBounced[slot]  = false; }
                    else                              { DCASellTickets[slot] = limitTk; DCASellLimitTk[slot] = 0; DCASellBounced[slot] = false; }
                    return;
                }
                if(!OrderSelect(limitTk)) {
                    if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[slot]  = 0;
                    else                              DCASellLimitTk[slot] = 0;
                } else {
                    double cancelTol = 300.0 * point;
                    ENUM_ORDER_TYPE oType = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
                    bool tooFar;
                    if(posType == POSITION_TYPE_BUY)
                        tooFar = (oType == ORDER_TYPE_BUY_STOP) ? (ask > slotPrice + cancelTol)
                                                                 : (ask < slotPrice - cancelTol);
                    else
                        tooFar = (oType == ORDER_TYPE_SELL_STOP) ? (bid < slotPrice - cancelTol)
                                                                  : (bid > slotPrice + cancelTol);
                    if(tooFar) {
                        if(Trade.OrderDelete(limitTk)) {
                            if(posType == POSITION_TYPE_BUY) { DCABuyLimitTk[slot]  = 0; DCABuyBounced[slot]  = false; }
                            else                              { DCASellLimitTk[slot] = 0; DCASellBounced[slot] = false; }
                        } else {
                            Print("RTB: Huỷ lệnh chờ slot ", slot, " thất bại, err=", GetLastError(), " — thử lại lượt sau.");
                        }
                    }
                }
                continue;
            }

            if(count < maxOrds) {
                if(TimeCurrent() - LastOrderTime < g_OrderDelay) continue;
                if(DCA_Mode == DCA_STOP) continue;

                if(DCA_Mode == DCA_STEP_TF) {
                    int sig = GetSignal();
                    if(posType == POSITION_TYPE_BUY  && sig != 1)  continue;
                    if(posType == POSITION_TYPE_SELL && sig != -1) continue;
                }

                double tolDup = 0.5 * point;
                bool duplicateExists = false;
                for(int pi = PositionsTotal()-1; pi >= 0 && !duplicateExists; pi--) {
                    ulong ptk = PositionGetTicket(pi);
                    if(!PositionSelectByTicket(ptk)) continue;
                    if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
                    if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
                    if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
                    if(MathAbs(PositionGetDouble(POSITION_PRICE_OPEN) - slotPrice) < tolDup) duplicateExists = true;
                }
                if(!duplicateExists) {
                    ENUM_ORDER_TYPE dupType1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
                    ENUM_ORDER_TYPE dupType2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
                    for(int oi = OrdersTotal()-1; oi >= 0 && !duplicateExists; oi--) {
                        ulong otk = OrderGetTicket(oi);
                        if(otk == 0 || !OrderSelect(otk)) continue;
                        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
                        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
                        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
                        if(ot != dupType1 && ot != dupType2) continue;
                        if(MathAbs(OrderGetDouble(ORDER_PRICE_OPEN) - slotPrice) < tolDup) duplicateExists = true;
                    }
                }
                if(duplicateExists) {
                    Print("RTB: Slot ", slot, " (", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
                          ") đã có vị thế/lệnh chờ thật ở giá ", slotPrice, " — bỏ qua đặt trùng.");
                    continue;
                }

                double lot = DCAOrderLot(InpLotSize, slot + 1);
                string cmt = (DCA_TP == 0 && DCA_SL == 0)
                    ? "RTB|0|0|D|RF"
                    : "RTB|" + IntegerToString((int)DCA_TP) + "|" + IntegerToString((int)DCA_SL) + "|RF";
                bool ok = false;
                if(posType == POSITION_TYPE_BUY) {
                    double tp_p = DCA_TP > 0 ? NormalizeDouble(slotPrice + DCA_TP * point, _Digits) : 0;
                    double sl_p = DCA_SL > 0 ? NormalizeDouble(slotPrice - DCA_SL * point, _Digits) : 0;
                    if(ask > slotPrice)
                        ok = Trade.BuyLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(ask < slotPrice)
                        ok = Trade.BuyStop(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                } else {
                    double tp_p = DCA_TP > 0 ? NormalizeDouble(slotPrice - DCA_TP * point, _Digits) : 0;
                    double sl_p = DCA_SL > 0 ? NormalizeDouble(slotPrice + DCA_SL * point, _Digits) : 0;
                    if(bid < slotPrice)
                        ok = Trade.SellLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(bid > slotPrice)
                        ok = Trade.SellStop(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                }
                if(ok) {
                    ulong lmtTk = Trade.ResultOrder();
                    if(lmtTk > 0) {
                        Print("RTB: Placed re-fill slot ", slot, " at ", slotPrice);
                        if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[slot]  = lmtTk;
                        else                              DCASellLimitTk[slot] = lmtTk;
                        LastOrderTime = TimeCurrent();
                    }
                }
            }
        } else {
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

    if(peak >= DCA_MaxOrd || DCA_Mode == DCA_STOP) return;

    if(DCA_Mode == DCA_STEP_TF) {
        int sig = GetSignal();
        if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
        if(posType == POSITION_TYPE_SELL && sig != -1) return;
    }

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
        bool ok = OpenOrder(ord, lot, DCA_TP, DCA_SL, true);
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
        double tolDupPeak = 0.5 * point;
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

    string cmt = (DCA_TP == 0 && DCA_SL == 0)
        ? "RTB|0|0|D"
        : "RTB|" + IntegerToString((int)DCA_TP) + "|" + IntegerToString((int)DCA_SL);
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
    if(InpBotMode == MODE_SEMI_AUTO) return;

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
    if(OpenOrder(ord, InpLotSize, g_TP_Points, g_SL_Points)) {
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

//+------------------------------------------------------------------+
//| TELEGRAM ALERT                                                    |
//+------------------------------------------------------------------+
string UrlEncode(string text) {
    uchar arr[];
    int len = StringToCharArray(text, arr, 0, -1, CP_UTF8) - 1;
    string result = "";
    for(int i = 0; i < len; i++) {
        uchar c = arr[i];
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~')
            result += CharToString(c);
        else
            result += StringFormat("%%%02X", c);
    }
    return result;
}

bool SendTelegramMessage(string text) {
    if(!g_TeleEnable || g_TeleBotToken == "" || g_TeleChatID == "") return false;
    string url = "https://api.telegram.org/bot" + g_TeleBotToken +
                 "/sendMessage?chat_id=" + g_TeleChatID + "&text=" + UrlEncode(text);
    char post[]; char result[]; string headers;
    ResetLastError();
    int httpCode = WebRequest("GET", url, "", "", 5000, post, 0, result, headers);
    if(httpCode != 200) {
        Print("RTB: Gửi Telegram lỗi httpCode=", httpCode, " err=", GetLastError());
        return false;
    }
    return true;
}

// Xếp hàng đợi thay vì gửi ngay — WebRequest() chặn đồng bộ, nếu gọi giữa lúc
// đang xử lý DCA/Trim/GUI trong OnTimer() sẽ làm trễ toàn bộ phần còn lại của
// tick đó. FlushTelegramQueue() ở cuối OnTimer() mới thực sự gửi đi.
void QueueTelegramMessage(string text) {
    int n = ArraySize(g_TeleQueue);
    ArrayResize(g_TeleQueue, n + 1);
    g_TeleQueue[n] = text;
}

void FlushTelegramQueue() {
    int n = ArraySize(g_TeleQueue);
    for(int i = 0; i < n; i++) SendTelegramMessage(g_TeleQueue[i]);
    ArrayResize(g_TeleQueue, 0);
}

void CheckDDAlert() {
    if(!g_TeleEnable || g_TeleDDStep <= 0) return;
    double balance = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity  = AccountInfoDouble(ACCOUNT_EQUITY);
    if(balance <= 0) return;
    double ddPct = (equity < balance) ? (balance - equity) / balance * 100.0 : 0;
    int level = (int)(ddPct / g_TeleDDStep);
    if(level > g_LastDDAlertLevel) {
        g_LastDDAlertLevel = level;
        QueueTelegramMessage(StringFormat("⚠ Drawdown vượt mức %.0f%% (hiện tại: %.2f%%)",
                             level * g_TeleDDStep, ddPct));
    } else if(ddPct <= (g_LastDDAlertLevel * g_TeleDDStep) - (g_TeleDDStep / 2.0)) {
        // Chỉ "tháo chốt" mốc đã báo khi DD lùi sâu hơn nửa bước dưới mốc đó — có đệm
        // chống dao động quanh đúng ranh giới (tránh báo lặp lại liên tục nếu DD cứ
        // nhấp nhô ngay sát mốc, ví dụ 19.9% <-> 20.1%).
        g_LastDDAlertLevel = (int)(ddPct / g_TeleDDStep);
    }
}

void NotifyManualTrimClose(ulong ticket, double profit) {
    if(!g_TeleEnable) return;
    QueueTelegramMessage(StringFormat("🔔 Lệnh tay (ticket=%I64u) đã bị đóng do Tỉa Lệnh, P/L=$%.2f",
                         ticket, profit));
}

// Lấy lợi nhuận THỰC TẾ của deal đóng lệnh (profit + swap + commission) từ lịch sử,
// thay vì dùng con số profit nổi đã chụp lúc quét đầu CheckTrimming() — giữa lúc quét
// và lúc lệnh thực sự khớp đóng, giá đã trôi thêm (đặc biệt khi đóng nhiều lệnh liên
// tiếp trong 1 lượt tỉa), nên con số cũ luôn lệch so với giá trị chốt lệnh thật.
double RealizedCloseProfit(double fallback) {
    ulong dealTk = Trade.ResultDeal();
    if(dealTk > 0 && HistoryDealSelect(dealTk))
        return HistoryDealGetDouble(dealTk, DEAL_PROFIT) +
               HistoryDealGetDouble(dealTk, DEAL_SWAP) +
               HistoryDealGetDouble(dealTk, DEAL_COMMISSION);
    return fallback;
}

//+------------------------------------------------------------------+
//| ORDER TRIMMING                                                   |
//+------------------------------------------------------------------+
void CheckTrimming() {
    if(g_TrimMode == TRIM_OFF) return;
    if(CountAllForTrim() < g_TrimTrigger) return;

    switch(g_TrimMode) {
    case TRIM_HEDGE: {
        int    totalPos = PositionsTotal();
        ulong  tks[];
        double profits[];
        long   magics[];
        bool   used[];
        ArrayResize(tks, totalPos);
        ArrayResize(profits, totalPos);
        ArrayResize(magics, totalPos);
        ArrayResize(used, totalPos);
        int cnt = 0;
        for(int i = totalPos - 1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManagedForTrim()) continue;
            tks[cnt]     = tk;
            profits[cnt] = PositionGetDouble(POSITION_PROFIT);
            magics[cnt]  = PositionGetInteger(POSITION_MAGIC);
            used[cnt]    = false;
            cnt++;
        }

        ulong  notifyTk[];
        double notifyProfit[];
        int    notifyCount = 0;
        ArrayResize(notifyTk, cnt);
        ArrayResize(notifyProfit, cnt);

        int closedCycles = 0;
        for(int w = 0; w < g_TrimMaxCycles; w++) {
            int    worstIdx[];
            ArrayResize(worstIdx, g_TrimMaxLoss);
            int    wn = 0;
            double worstSum = 0;
            for(int n = 0; n < g_TrimMaxLoss; n++) {
                int wIdx = -1;
                for(int i = 0; i < cnt; i++) {
                    if(used[i]) continue;
                    if(wIdx < 0 || profits[i] < profits[wIdx]) wIdx = i;
                }
                if(wIdx < 0) break;
                worstIdx[wn] = wIdx;
                worstSum    += profits[wIdx];
                used[wIdx]   = true;
                wn++;
            }
            if(wn == 0) break;

            int    winIdx[];
            ArrayResize(winIdx, g_TrimMaxWin);
            int    wn2 = 0;
            double winSum = 0;
            for(int n = 0; n < g_TrimMaxWin; n++) {
                if(winSum + worstSum >= g_TrimTarget) break;
                int bIdx = -1;
                for(int i = 0; i < cnt; i++) {
                    if(used[i]) continue;
                    if(bIdx < 0 || profits[i] > profits[bIdx]) bIdx = i;
                }
                if(bIdx < 0 || profits[bIdx] <= 0) break;
                winIdx[wn2] = bIdx;
                winSum     += profits[bIdx];
                used[bIdx]  = true;
                wn2++;
            }

            if(winSum + worstSum >= g_TrimTarget) {
                for(int i = 0; i < wn2; i++) {
                    bool okClose = Trade.PositionClose(tks[winIdx[i]]);
                    if(magics[winIdx[i]] == 0) {
                        double realProfit = okClose ? RealizedCloseProfit(profits[winIdx[i]]) : profits[winIdx[i]];
                        notifyTk[notifyCount] = tks[winIdx[i]]; notifyProfit[notifyCount] = realProfit; notifyCount++;
                    }
                }
                for(int i = 0; i < wn;  i++) {
                    bool okClose = Trade.PositionClose(tks[worstIdx[i]]);
                    if(magics[worstIdx[i]] == 0) {
                        double realProfit = okClose ? RealizedCloseProfit(profits[worstIdx[i]]) : profits[worstIdx[i]];
                        notifyTk[notifyCount] = tks[worstIdx[i]]; notifyProfit[notifyCount] = realProfit; notifyCount++;
                    }
                }
                closedCycles++;
            } else break;
        }
        if(closedCycles > 0)
            Print("RTB: Hedge trim cycles=", closedCycles, " x up to ", g_TrimMaxWin, " winners / ", g_TrimMaxLoss, " losers");
        // Gửi Telegram SAU KHI đã đóng xong toàn bộ lệnh của lượt tỉa này — WebRequest()
        // là lệnh gọi mạng đồng bộ, chặn nếu đặt xen giữa các PositionClose() sẽ làm
        // trễ việc đóng lệnh thật khi Telegram phản hồi chậm.
        for(int i = 0; i < notifyCount; i++) NotifyManualTrimClose(notifyTk[i], notifyProfit[i]);
        break;
    }

    case TRIM_HEDGE_PTS: {
        int    totalPos = PositionsTotal();
        ulong  tks[];
        double profits[];
        double pts[];
        long   magics[];
        bool   isOrig[];
        bool   used[];
        ArrayResize(tks, totalPos);
        ArrayResize(profits, totalPos);
        ArrayResize(pts, totalPos);
        ArrayResize(magics, totalPos);
        ArrayResize(isOrig, totalPos);
        ArrayResize(used, totalPos);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        int cnt = 0;
        for(int i = totalPos - 1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManagedForTrim()) continue;
            int    pt  = (int)PositionGetInteger(POSITION_TYPE);
            double opn = PositionGetDouble(POSITION_PRICE_OPEN);
            tks[cnt]     = tk;
            profits[cnt] = PositionGetDouble(POSITION_PROFIT);
            pts[cnt]     = (pt == POSITION_TYPE_BUY) ? (bid - opn) / point : (opn - ask) / point;
            magics[cnt]  = PositionGetInteger(POSITION_MAGIC);
            isOrig[cnt]  = (PositionGetString(POSITION_COMMENT) == "RTB|0|0");
            used[cnt]    = false;
            cnt++;
        }

        ulong  notifyTk[];
        double notifyProfit[];
        int    notifyCount = 0;
        ArrayResize(notifyTk, cnt);
        ArrayResize(notifyProfit, cnt);

        int closedCycles = 0;
        for(int w = 0; w < g_TrimMaxCycles; w++) {
            int    worstIdx[];
            ArrayResize(worstIdx, g_TrimMaxLoss);
            int    wn = 0;
            double worstSum = 0;
            for(int n = 0; n < g_TrimMaxLoss; n++) {
                int wIdx = -1;
                for(int i = 0; i < cnt; i++) {
                    if(used[i] || isOrig[i]) continue;
                    if(wIdx < 0 || pts[i] < pts[wIdx]) wIdx = i;
                }
                if(wIdx < 0) break;
                worstIdx[wn] = wIdx;
                worstSum    += profits[wIdx];
                used[wIdx]   = true;
                wn++;
            }
            if(wn == 0) break;

            int    winIdx[];
            ArrayResize(winIdx, g_TrimMaxWin);
            int    wn2 = 0;
            double winSum = 0;
            for(int n = 0; n < g_TrimMaxWin; n++) {
                if(winSum + worstSum >= g_TrimTarget) break;
                int bIdx = -1;
                for(int i = 0; i < cnt; i++) {
                    if(used[i]) continue;
                    if(bIdx < 0 || profits[i] > profits[bIdx]) bIdx = i;
                }
                if(bIdx < 0 || profits[bIdx] <= 0) break;
                winIdx[wn2] = bIdx;
                winSum     += profits[bIdx];
                used[bIdx]  = true;
                wn2++;
            }

            if(winSum + worstSum >= g_TrimTarget) {
                for(int i = 0; i < wn2; i++) {
                    bool okClose = Trade.PositionClose(tks[winIdx[i]]);
                    if(magics[winIdx[i]] == 0) {
                        double realProfit = okClose ? RealizedCloseProfit(profits[winIdx[i]]) : profits[winIdx[i]];
                        notifyTk[notifyCount] = tks[winIdx[i]]; notifyProfit[notifyCount] = realProfit; notifyCount++;
                    }
                }
                for(int i = 0; i < wn;  i++) {
                    bool okClose = Trade.PositionClose(tks[worstIdx[i]]);
                    if(magics[worstIdx[i]] == 0) {
                        double realProfit = okClose ? RealizedCloseProfit(profits[worstIdx[i]]) : profits[worstIdx[i]];
                        notifyTk[notifyCount] = tks[worstIdx[i]]; notifyProfit[notifyCount] = realProfit; notifyCount++;
                    }
                }
                closedCycles++;
            } else break;
        }
        if(closedCycles > 0)
            Print("RTB: Hedge-by-Points trim cycles=", closedCycles, " x up to ", g_TrimMaxWin, " winners / ", g_TrimMaxLoss, " losers");
        // Gửi Telegram SAU KHI đã đóng xong toàn bộ lệnh của lượt tỉa này — WebRequest()
        // là lệnh gọi mạng đồng bộ, chặn nếu đặt xen giữa các PositionClose() sẽ làm
        // trễ việc đóng lệnh thật khi Telegram phản hồi chậm.
        for(int i = 0; i < notifyCount; i++) NotifyManualTrimClose(notifyTk[i], notifyProfit[i]);
        break;
    }
    }
}

//+------------------------------------------------------------------+
//| TRAILING STOP                                                    |
//+------------------------------------------------------------------+
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
    if(CountAll() < g_TrailMinOrds) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    if(g_TrailMode == TRAIL_BASKET) {
        if(CountBuy() > 0) {
            double avgBuy = AvgOpenPrice(POSITION_TYPE_BUY);
            if(bid - avgBuy >= g_TrailActivate * point) {
                double newSL = bid - g_TrailInit * point;
                if(TrailBuy == 0 || newSL >= TrailBuy + g_TrailStep * point)
                    TrailBuy = newSL;
            }
            if(TrailBuy > 0) {
                for(int i = PositionsTotal()-1; i >= 0; i--) {
                    ulong tk = PositionGetTicket(i);
                    if(!PositionSelectByTicket(tk)) continue;
                    if(!IsManaged()) continue;
                    if((int)PositionGetInteger(POSITION_TYPE) != POSITION_TYPE_BUY) continue;
                    ApplyTrailToPos(tk, POSITION_TYPE_BUY, TrailBuy);
                }
            }
        } else { TrailBuy = 0; }

        if(CountSell() > 0) {
            double avgSell = AvgOpenPrice(POSITION_TYPE_SELL);
            if(avgSell - ask >= g_TrailActivate * point) {
                double newSL = ask + g_TrailInit * point;
                if(TrailSell == 0 || newSL <= TrailSell - g_TrailStep * point)
                    TrailSell = newSL;
            }
            if(TrailSell > 0) {
                for(int i = PositionsTotal()-1; i >= 0; i--) {
                    ulong tk = PositionGetTicket(i);
                    if(!PositionSelectByTicket(tk)) continue;
                    if(!IsManaged()) continue;
                    if((int)PositionGetInteger(POSITION_TYPE) != POSITION_TYPE_SELL) continue;
                    ApplyTrailToPos(tk, POSITION_TYPE_SELL, TrailSell);
                }
            }
        } else { TrailSell = 0; }

        if(InpTrailShowLine) {
            if(TrailBuy  > 0) DrawHLine("TrailBuy",  TrailBuy,  InpTrailBuyColor,  InpTrailLineWidth);
            else ObjectDelete(0, GUI + "TrailBuy");
            if(TrailSell > 0) DrawHLine("TrailSell", TrailSell, InpTrailSellColor, InpTrailLineWidth);
            else ObjectDelete(0, GUI + "TrailSell");
        }

    } else {
        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

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
}

//+------------------------------------------------------------------+
//| EXIT LOGIC                                                       |
//+------------------------------------------------------------------+
void CheckExit() {
    if(TrailBuy > 0 || TrailSell > 0) {
        double _ask = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double _bid = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;
            int pt = (int)PositionGetInteger(POSITION_TYPE);
            if(pt == POSITION_TYPE_BUY  && TrailBuy  > 0 && _bid <= TrailBuy)
                Trade.PositionClose(tk);
            else if(pt == POSITION_TYPE_SELL && TrailSell > 0 && _ask >= TrailSell)
                Trade.PositionClose(tk);
        }
    }

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

    if(g_StealthMode) {
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            string cmt = PositionGetString(POSITION_COMMENT);
            bool isManualPos = (InpBotMode == MODE_SEMI_AUTO &&
                                PositionGetInteger(POSITION_MAGIC) == 0 &&
                                StringFind(cmt, "RTB|") != 0);
            if(cmt != "RTB|0|0" && !isManualPos) continue;

            int    pt  = (int)PositionGetInteger(POSITION_TYPE);
            double opn = PositionGetDouble(POSITION_PRICE_OPEN);

            if(pt == POSITION_TYPE_BUY) {
                if(g_TP_Points > 0 && bid >= opn + g_TP_Points * point)
                    { Trade.PositionClose(tk); continue; }
                if(g_SL_Points > 0 && bid <= opn - g_SL_Points * point)
                    Trade.PositionClose(tk);
            } else {
                if(g_TP_Points > 0 && ask <= opn - g_TP_Points * point)
                    { Trade.PositionClose(tk); continue; }
                if(g_SL_Points > 0 && ask >= opn + g_SL_Points * point)
                    Trade.PositionClose(tk);
            }
        }
    }

    if(g_CloseProfit > 0) {
        if(FloatProfit() >= g_CloseProfit) {
            Print("RTB: CloseProfit target reached. Closing all.");
            CloseAll();
            return;
        }
    }

    if(g_CloseLoss > 0) {
        if(FloatProfit() <= -g_CloseLoss) {
            Print("RTB: CloseLoss limit hit. Closing all.");
            CloseAll();
            return;
        }
    }
}

//+------------------------------------------------------------------+
//| DAY PROFIT TRACKING                                              |
//+------------------------------------------------------------------+
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

//+------------------------------------------------------------------+
//| DAY LIMIT CHECK                                                  |
//+------------------------------------------------------------------+
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

//+------------------------------------------------------------------+
//| GUI                                                              |
//+------------------------------------------------------------------+
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

void CreateChip(string name, string text, int lx, int ly, int lw, int lh, color bg, color fg) {
    CreateRect(name + "Bg", lx, ly, lw, lh, bg);
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_LABEL, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetInteger(0, obj, OBJPROP_ANCHOR,     ANCHOR_LEFT);
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Calibri");
        ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, lx + 6);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, ly + lh/2);
    ObjectSetString(0,  obj, OBJPROP_TEXT,     text);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,    fg);
    ObjectSetInteger(0, obj, OBJPROP_FONTSIZE, 9);
}

void DrawGauge(string name, int lx, int ly, int lw, int lh, double pct, color trackClr, color fillClr) {
    CreateRect(name + "Trk", lx, ly, lw, lh, trackClr);
    int fillW = (int)MathRound(lw * MathMax(0.0, MathMin(100.0, pct)) / 100.0);
    if(fillW < 1 && pct > 0) fillW = 1;
    CreateRect(name + "Fill", lx, ly, fillW, lh, fillClr);
}

int DaysInMonth(int year, int month) {
    int dim[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int d = dim[month-1];
    if(month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) d = 29;
    return d;
}

//+------------------------------------------------------------------+
//| CALENDAR PANEL — bảng thống kê P/L + Lot theo từng ngày trong tháng |
//+------------------------------------------------------------------+
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
    int calY = InpCalPanelY;

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

        if(g_CalCacheLot[slot] <= 0) {
            ObjectSetInteger(0, cellObj, OBJPROP_COLOR, C'40,48,68');
            ObjectDelete(0, GUI + "CalPL" + si);
            ObjectDelete(0, GUI + "CalP" + si);
            ObjectDelete(0, GUI + "CalVL" + si);
            ObjectDelete(0, GUI + "CalL" + si);
            continue;
        }

        color pc = (g_CalCacheProfit[slot] >= 0) ? clrLimeGreen : clrTomato;

        ObjectSetInteger(0, cellObj, OBJPROP_COLOR, g_CalCacheProfit[slot] >= 0 ? clrLimeGreen : C'40,48,68');

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
    int titleOff = 36;

    double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity    = AccountInfoDouble(ACCOUNT_EQUITY);
    double totalProfit = 0, buyProfit = 0, sellProfit = 0;
    int    nBuy = 0, nSell = 0;
    double lotBuy = 0, lotSell = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForDisplay()) continue;
        double p   = PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
        double lot = PositionGetDouble(POSITION_VOLUME);
        int    pt  = (int)PositionGetInteger(POSITION_TYPE);
        totalProfit += p;
        if(pt == POSITION_TYPE_BUY)  { buyProfit  += p; nBuy++;  lotBuy  += lot; }
        else                         { sellProfit += p; nSell++; lotSell += lot; }
    }
    double pnlPct  = (InitBalance > 0) ? totalProfit / InitBalance * 100.0 : 0;
    double ddPct   = (balance > 0 && equity < balance) ? (balance - equity) / balance * 100.0 : 0;
    if(ddPct > MaxDrawdownPct) MaxDrawdownPct = ddPct;

    string sigName = "";
    switch(g_SignalMode) {
        case SIG_SIMULATED: sigName = "Simulated";      break;
        case SIG_UT_BOT:    sigName = "UT Bot";         break;
    }

    string dirName = "";
    color  dirClr  = clrSilver;
    switch(g_Direction) {
        case DIR_BOTH:      dirName = "▲▼ Both";     dirClr = clrDodgerBlue; break;
        case DIR_ONLY_BUY:  dirName = "▲  Buy Only"; dirClr = clrLimeGreen;  break;
        case DIR_ONLY_SELL: dirName = "▼  Sell Only"; dirClr = clrTomato;    break;
    }

    MqlDateTime dt;
    TimeToStruct(TimeLocal(), dt);
    string tStr = StringFormat("%04d/%02d/%02d  %02d:%02d:%02d",
                   dt.year, dt.mon, dt.day, dt.hour, dt.min, dt.sec);

    color cProfit = (totalProfit >= 0) ? clrLimeGreen : clrTomato;
    color cBuyP   = (buyProfit   >= 0) ? clrLimeGreen : clrTomato;
    color cSellP  = (sellProfit  >= 0) ? clrLimeGreen : clrTomato;
    color cDayP   = (DayProfit   >= 0) ? clrLimeGreen : clrTomato;

    int x = PX + 7, s = 18;
    int bh  = 22;
    int bfw = PW - 18;
    int bhw = (PW - 24) / 2;
    int bx2 = PX + 7 + bhw + 4;

    string legacyObjs[] = {"L0","Tim","Sig","Mod","Dir","Sync","L1","Bal","Ini","DayP","FP",
                            "L2","DD","MDD","L3","BuyP","BuyC","SelP","SelC","Tot","TClock",
                            "P3VC1","P3VC2","P3VC3","P3VC4","P3HR0","P3HR1","P3HR2","P3HR3",
                            "TR0S","TR1S","TR2S","TR3S",
                            "TR0BarTrk","TR1BarTrk","TR2BarTrk","TR3BarTrk",
                            "TR0BarFill","TR1BarFill","TR2BarFill","TR3BarFill"};
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

    // ========== Hàng giờ hệ thống — nằm dưới banner tiêu đề, không chen vào header ==========
    int contentX = PX + 7, cardW = PW - 14, rightEdge = contentX + cardW - 8;
    int y2 = PY + titleOff + 6;
    int topRowY = y2;

    if(!g_PanelCollapsed) {
    Lbl("TimeRow", tStr, contentX, y2, C'127,139,163', 10);
    y2 += 16;

    // ========== Chips: Signal / Direction / Mode+Role / Sync ==========
    int chH = 17;

    color dirBg = (g_Direction == DIR_BOTH) ? C'18,50,68' : (g_Direction == DIR_ONLY_BUY ? C'15,36,25' : C'36,18,20');
    color dirFg = (g_Direction == DIR_BOTH) ? C'111,217,238' : (g_Direction == DIR_ONLY_BUY ? C'98,214,150' : C'232,120,120');
    CreateChip("ChipSig", sigName, contentX, y2, bhw, chH, C'24,34,54', C'159,176,201');
    CreateChip("ChipDir", dirName, bx2, y2, bhw, chH, dirBg, dirFg);
    y2 += chH + 4;

    string modeTxt = (InpBotMode == MODE_SEMI_AUTO ? "Bán Tự Động" : "Tự Động");
    color  modeFg  = C'159,176,201';
    if(!g_BotEnabled) { modeTxt = "!! BOT TAT !!"; modeFg = C'239,83,80'; }
    CreateChip("ChipMode", modeTxt, contentX, y2, bfw, chH, C'24,34,54', modeFg);
    y2 += chH + 8;
    } else {
        ObjectDelete(0, GUI + "TimeRow");
        ObjectDelete(0, GUI + "ChipSig");  ObjectDelete(0, GUI + "ChipSigBg");
        ObjectDelete(0, GUI + "ChipDir");  ObjectDelete(0, GUI + "ChipDirBg");
        ObjectDelete(0, GUI + "ChipMode"); ObjectDelete(0, GUI + "ChipModeBg");
    }

    // ========== Card: Tài khoản — LUÔN hiển thị, chứa nút thu gọn/mở rộng toàn panel ==========
    int acctH = 6 + 13 + 4*15 + 6;
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
    Lbl ("IniL", "Initial", contentX + 8, ya, C'127,139,163', 11);
    LblR("IniV", StringFormat("$%.2f", InitBalance), rightEdge, ya, C'231,236,245', 11); ya += 15;
    Lbl ("DayL", "Day P/L", contentX + 8, ya, C'127,139,163', 11);
    LblR("DayV", StringFormat("%s$%.2f", DayProfit >= 0 ? "+" : "-", MathAbs(DayProfit)), rightEdge, ya, cDayP, 11); ya += 15;
    Lbl ("FloL", "Float", contentX + 8, ya, C'127,139,163', 11);
    LblR("FloV", StringFormat("%s$%.2f (%.2f%%)", totalProfit >= 0 ? "+" : "-", MathAbs(totalProfit), pnlPct), rightEdge, ya, cProfit, 11);
    y2 += acctH + 8;

    // ========== Card: Rủi ro (Drawdown) ==========
    color ddColor  = ddPct > 60          ? clrTomato : (ddPct > 20          ? clrOrangeRed : clrSilver);
    color mddColor = MaxDrawdownPct > 60 ? clrTomato : (MaxDrawdownPct > 20 ? clrOrangeRed : clrSilver);
    if(!g_PanelCollapsed) {
    int riskH = 6 + 13 + (13+7+3)*2 + 6;
    CreateRect("CardRisk",    contentX, y2, cardW, riskH, C'20,28,44');
    CreateRect("CardRiskBar", contentX, y2, 2,     riskH, C'240,166,63');
    Lbl("RiskH", "RỦI RO (DRAWDOWN)", contentX + 8, y2 + 5, C'95,108,132', 9);
    ObjectSetString(0, GUI + "RiskH", OBJPROP_FONT, "Calibri Bold");
    int yr = y2 + 6 + 13;
    Lbl ("DDL", "DD Now", contentX + 8, yr, C'127,139,163', 10);
    LblR("DDV", StringFormat("%.2f%%", ddPct), rightEdge, yr, ddColor, 10);
    DrawGauge("DDG", contentX + 8, yr + 14, cardW - 16, 6, ddPct, C'28,36,52', ddColor);
    yr += 13 + 7 + 3;
    Lbl ("MDDL", "DD Max", contentX + 8, yr, C'127,139,163', 10);
    LblR("MDDV", StringFormat("%.2f%%", MaxDrawdownPct), rightEdge, yr, mddColor, 10);
    DrawGauge("MDDG", contentX + 8, yr + 14, cardW - 16, 6, MaxDrawdownPct, C'28,36,52', mddColor);
    ObjectDelete(0, GUI + "HdgS");
    y2 += riskH + 8;

    // ========== Card: Tỉa Lệnh (Trimming) ==========
    string trimModeTxt = (g_TrimMode == TRIM_OFF) ? "Tắt" : (g_TrimMode == TRIM_HEDGE ? "Hedge" : "Hedge Điểm");
    color  trimModeClr = (g_TrimMode == TRIM_OFF) ? C'127,139,163' : C'62,207,142';
    int    trimCount   = CountAllForTrim();
    color  trimCntClr  = (g_TrimMode != TRIM_OFF && trimCount >= g_TrimTrigger) ? clrLimeGreen : C'231,236,245';
    int trimH = 6 + 13 + 15*3 + 6;
    CreateRect("CardTrim",    contentX, y2, cardW, trimH, C'20,28,44');
    CreateRect("CardTrimBar", contentX, y2, 2,     trimH, C'160,110,220');
    Lbl("TrimH", "TỈA LỆNH", contentX + 8, y2 + 5, C'95,108,132', 9);
    ObjectSetString(0, GUI + "TrimH", OBJPROP_FONT, "Calibri Bold");
    LblR("TrimMode", trimModeTxt, rightEdge, y2 + 5, trimModeClr, 9);
    int yt = y2 + 6 + 13;
    Lbl ("TrimCntL", "Số lệnh", contentX + 8, yt, C'127,139,163', 10);
    LblR("TrimCntV", StringFormat("%d / %d", trimCount, g_TrimTrigger), rightEdge, yt, trimCntClr, 10); yt += 15;
    Lbl ("TrimTgtL", "Mục tiêu", contentX + 8, yt, C'127,139,163', 10);
    LblR("TrimTgtV", StringFormat("$%.2f", g_TrimTarget), rightEdge, yt, C'231,236,245', 10); yt += 15;
    Lbl ("TrimManL", "Lệnh tay", contentX + 8, yt, C'127,139,163', 10);
    LblR("TrimManV", g_TrimIncludeManual ? "Có tham gia" : "Không", rightEdge, yt, g_TrimIncludeManual ? C'62,207,142' : C'127,139,163', 10);
    y2 += trimH + 8;

    // ========== Twin cards: Buy / Sell ==========
    int twinH = 6 + 12 + 3 + 17 + 3 + 12 + 6;
    CreateRect("CardBuy",    contentX, y2, bhw, twinH, C'20,28,44');
    CreateRect("CardBuyBar", contentX, y2, bhw, 2,     C'44,107,82');
    Lbl("BuyL",   StringFormat("▲ BUY · %d", nBuy),          contentX + 8, y2 + 8,  C'95,150,125', 10);
    Lbl("BuyV",   StringFormat("%s$%.2f", buyProfit >= 0 ? "+" : "-", MathAbs(buyProfit)), contentX + 8, y2 + 23, cBuyP, 14);
    Lbl("BuyLot", StringFormat("Lot %.2f", lotBuy),           contentX + 8, y2 + 43, C'127,139,163', 10);

    CreateRect("CardSell",    bx2, y2, bhw, twinH, C'20,28,44');
    CreateRect("CardSellBar", bx2, y2, bhw, 2,     C'122,48,51');
    Lbl("SelL",   StringFormat("▼ SELL · %d", nSell),         bx2 + 8, y2 + 8,  C'190,120,120', 10);
    Lbl("SelV",   StringFormat("%s$%.2f", sellProfit >= 0 ? "+" : "-", MathAbs(sellProfit)), bx2 + 8, y2 + 23, cSellP, 14);
    Lbl("SelLot", StringFormat("Lot %.2f", lotSell),          bx2 + 8, y2 + 43, C'127,139,163', 10);
    y2 += twinH + 6;

    // ========== Total ==========
    Lbl ("TotL", "Total", contentX, y2, C'127,139,163', 10);
    LblR("TotV", StringFormat("%d orders", nBuy + nSell), rightEdge, y2, C'231,236,245', 10);
    y2 += 17;
    } else {
        string acctCollapsedObjs[] = {
            "CardRisk", "CardRiskBar", "RiskH", "DDL", "DDV", "DDGTrk", "DDGFill",
            "MDDL", "MDDV", "MDDGTrk", "MDDGFill",
            "CardTrim", "CardTrimBar", "TrimH", "TrimMode", "TrimCntL", "TrimCntV",
            "TrimTgtL", "TrimTgtV", "TrimManL", "TrimManV",
            "CardBuy", "CardBuyBar", "BuyL", "BuyV", "BuyLot",
            "CardSell", "CardSellBar", "SelL", "SelV", "SelLot",
            "TotL", "TotV"
        };
        for(int aci = 0; aci < ArraySize(acctCollapsedObjs); aci++) ObjectDelete(0, GUI + acctCollapsedObjs[aci]);
    }

    int contentBottom = y2 + 6;
    int bg2Y = contentBottom + 8;

    if(!g_PanelCollapsed) {
    int bg3Y = bg2Y + 88 + 8;

    int y = bg2Y + 10;
    Lbl("P2T", "ĐIỀU KHIỂN LỆNH", x + 8, y, C'95,108,132', 9);
    ObjectSetString(0, GUI + "P2T", OBJPROP_FONT, "Calibri Bold"); y += s + 2;

    {
        string botTxt = g_BotEnabled ? "  BOT: Running" : "  BOT: Off";
        color  botBg  = g_BotEnabled ? C'0,90,30'   : C'120,20,20';
        color  botBd  = g_BotEnabled ? C'40,190,90' : C'220,60,60';
        CreateBtn("BtnBotToggle", botTxt, PX+7, y, bfw, bh, botBg, botBd);
        y += bh + 4;
    }

    CreateBtn("BtnCloseAll",    "  Close All",     PX+7, y, bfw, bh, C'20,60,150',  C'80,130,230');

    int y3 = bg3Y + 10;
    Lbl("P3T", "THỐNG KÊ", contentX + 8, y3, C'95,108,132', 9);
    ObjectSetString(0, GUI + "P3T", OBJPROP_FONT, "Calibri Bold");
    CreateBtn("BtnCalToggle", g_CalExpanded ? "« Đóng" : "Xem Lịch »", PX + PW - 82, y3 - 3, 76, 18, C'25,45,85', C'70,110,190');

    int bg3H = 34;

    if(InpBotMode == MODE_SEMI_AUTO) {
        int bg4Y = bg3Y + bg3H + 8;
        int y4 = bg4Y + 10;
        Lbl("P4T", "===  VÀO LỆNH THỦ CÔNG  ===", x, y4, C'230,100,100', 9);
        ObjectSetString(0, GUI + "P4T", OBJPROP_FONT, "Calibri Bold"); y4 += s + 2;
        CreateBtn("BtnOpenBuy",  "▲ Open Buy",  PX+7, y4, bhw, bh, C'0,80,20',  C'30,200,80');
        CreateBtn("BtnOpenSell", "▼ Open Sell", bx2,  y4, bhw, bh, C'100,0,0',  C'220,40,40');
        g_LastPanelBottom = y4 + bh + 8;
    } else {
        ObjectDelete(0, GUI + "P4T");
        ObjectDelete(0, GUI + "BtnOpenBuy");
        ObjectDelete(0, GUI + "BtnOpenSell");
        g_LastPanelBottom = bg3Y + bg3H;
    }

    } else {
        string collapsedObjs[] = {
            "P2T", "BtnBotToggle", "BtnCloseAll",
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

//+------------------------------------------------------------------+
//| CHART COLOR SCHEME                                               |
//+------------------------------------------------------------------+
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

//+------------------------------------------------------------------+
//| INIT DCA ARRAYS                                                  |
//+------------------------------------------------------------------+
void InitDCA() {
    DCA_Mode   = InpDCAMode;
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
    }
    g_BotEnabled = newVal;
}

//+------------------------------------------------------------------+
//| REBUILD DCA STATE FROM LIVE POSITIONS + PENDING ORDERS           |
//+------------------------------------------------------------------+
void RebuildDCAState(int posType) {
    int cap = (posType == POSITION_TYPE_BUY) ? ArraySize(DCABuyPrices) : ArraySize(DCASellPrices);

    double   slotPrices[];
    datetime slotTimes[];
    ulong    slotPosTk[];
    ulong    slotOrderTk[];
    ArrayResize(slotPrices, cap);
    ArrayResize(slotTimes, cap);
    ArrayResize(slotPosTk, cap);
    ArrayResize(slotOrderTk, cap);
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
            count++;
        } else {
            // Lệnh chờ "tầng mới kế tiếp" (chưa từng khớp) — KHÔNG được tính vào peak
            // (peak chỉ đại diện cho các tầng ĐÃ kích hoạt). Gom lại, xử lý sau khi
            // đã biết peak thật, để giữ đúng 1 lệnh và huỷ các lệnh dư (phantom).
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
        int j = i - 1;
        bool outOfOrder = (posType == POSITION_TYPE_BUY) ? (j >= 0 && slotPrices[j] < kp)
                                                          : (j >= 0 && slotPrices[j] > kp);
        while(outOfOrder) {
            slotTimes[j+1] = slotTimes[j]; slotPrices[j+1] = slotPrices[j];
            slotPosTk[j+1] = slotPosTk[j]; slotOrderTk[j+1] = slotOrderTk[j];
            j--;
            outOfOrder = (posType == POSITION_TYPE_BUY) ? (j >= 0 && slotPrices[j] < kp)
                                                         : (j >= 0 && slotPrices[j] > kp);
        }
        slotTimes[j+1] = kt; slotPrices[j+1] = kp; slotPosTk[j+1] = kpTk; slotOrderTk[j+1] = koTk;
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

    // Đối chiếu theo GIÁ đã lưu cho từng slot (SlotPx) thay vì theo vị trí mảng —
    // vì mảng quét từ broker sẽ tự dồn lại (mất chỗ trống) nếu có tầng nào mất dấu
    // ở giữa, khiến so khớp theo index bị lệch hoàn toàn so với tầng thật.
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
        if(expected <= 0) continue;
        int matchIdx = -1;
        for(int b = 0; b < realCount; b++) {
            if(claimed[b]) continue;
            if(MathAbs(slotPrices[b] - expected) < tol) { matchIdx = b; break; }
        }
        if(matchIdx >= 0) {
            finalPrices[slot]  = slotPrices[matchIdx];
            finalPosTk[slot]   = slotPosTk[matchIdx];
            finalOrderTk[slot] = slotOrderTk[matchIdx];
            claimed[matchIdx]  = true;
        } else {
            finalPrices[slot] = expected;
            recoveredGaps++;
        }
    }

    // Dữ liệu cũ (trước khi có SlotPx, hoặc slot chưa từng được lưu) — lấp các ô
    // còn trống theo đúng thứ tự vị trí như hành vi cũ (best-effort).
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
              " khôi phục giá của ", recoveredGaps, " tầng mất dấu trên broker (nhờ GlobalVariable) — sẽ tự đặt lại lệnh refill.");

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
                // Đã dùng hết cap (đủ InpDCA1Max lệnh) — không còn chỗ theo dõi
                // lệnh chờ tầng-kế-tiếp này nữa, huỷ luôn thay vì bỏ mặc mồ côi.
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

//+------------------------------------------------------------------+
//| EVENT HANDLERS                                                   |
//+------------------------------------------------------------------+
int OnInit() {
    g_SignalMode = InpSignalMode; g_Direction = InpDirection; g_UTKeyValue = InpUTKeyValue;
    g_UseTakeProfit = InpUseTakeProfit; g_UseStopLoss = InpUseStopLoss; g_StealthMode = InpStealthMode;
    g_OrderDelay = InpOrderDelay; g_TP_Points = InpTP_Points; g_SL_Points = InpSL_Points;
    g_DCABuyEnable = InpDCABuyEnable; g_DCASellEnable = InpDCASellEnable;
    g_DCAArithEnable = InpDCAArithEnable; g_DCAArithStep = InpDCAArithStep;
    g_TrimMode = InpTrimMode; g_TrimTrigger = InpTrimTrigger;
    g_TrimTarget = InpTrimTarget; g_TrimMaxLoss = InpTrimMaxLoss; g_TrimMaxWin = InpTrimMaxWin;
    g_TrimMaxCycles = InpTrimMaxCycles;
    g_TrimIncludeManual = InpTrimIncludeManual;
    g_TrailEnable = InpTrailEnable; g_TrailMode = InpTrailMode; g_TrailMinOrds = InpTrailMinOrds;
    g_TrailActivate = InpTrailActivate; g_TrailStep = InpTrailStep; g_TrailInit = InpTrailInit;
    g_CloseProfit = InpCloseProfit; g_CloseLoss = InpCloseLoss; g_ClosePerPips = InpClosePerPips;
    g_DayMaxLoss = InpDayMaxLoss; g_DayMaxProfit = InpDayMaxProfit;
    g_TeleEnable = InpTeleEnable; g_TeleBotToken = InpTeleBotToken; g_TeleChatID = InpTeleChatID;
    g_TeleDDStep = InpTeleDDStep;

    g_BotEnabled = InpBotEnabled;

    Trade.SetExpertMagicNumber(InpMagic);
    Trade.SetDeviationInPoints(50);
    Trade.SetTypeFilling(ORDER_FILLING_RETURN);

    SetupChartColors();

    InitDCA();

    hATR     = iATR(_Symbol, InpSignalTF, InpUTATRPeriod);

    if(hATR == INVALID_HANDLE) {
        Print("RTB: ERROR — failed to create indicator handles!");
        return INIT_FAILED;
    }

    WarmupATS(1000);

    InitBalance    = AccountInfoDouble(ACCOUNT_BALANCE);
    MaxDrawdownPct = 0;
    TrailBuy       = 0;
    TrailSell      = 0;
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

    if(g_TeleEnable)
        SendTelegramMessage("✅ Homie Bot đã kết nối Telegram thành công.");

    Print("RTB: Initialized. Magic=", InpMagic, " Signal=", EnumToString(g_SignalMode));
    return INIT_SUCCEEDED;
}

void OnDeinit(const int reason) {
    EventKillTimer();
    RemoveGUI();
    IndicatorRelease(hATR);
}

void OnTick() {
    CheckEntry();
    if(g_StealthMode) CheckExit();
    if(!DayLimitHit) CheckTrailing();
}

void OnTimer() {
    UpdateDayProfit();
    CheckDayLimit();
    CheckDDAlert();

    if(CountBuy()  == 0 && !HasPendingDCA(POSITION_TYPE_BUY))  ResetDCAState(POSITION_TYPE_BUY);
    if(CountSell() == 0 && !HasPendingDCA(POSITION_TYPE_SELL)) ResetDCAState(POSITION_TYPE_SELL);

    if(!g_StealthMode) CheckExit();

    if(!DayLimitHit && g_BotEnabled) {
        CheckOrigRestart(POSITION_TYPE_BUY);
        CheckOrigRestart(POSITION_TYPE_SELL);

        CheckTrimming();

        if(CountBuy()  > 0) CheckDCA(POSITION_TYPE_BUY);
        if(CountSell() > 0) CheckDCA(POSITION_TYPE_SELL);
    }

    UpdateGUI();

    // Gửi Telegram SAU CÙNG — sau khi mọi logic giao dịch + vẽ GUI của tick này
    // đã chạy xong, để WebRequest() (chặn đồng bộ) không trì hoãn bất kỳ phần nào
    // ở trên nếu mạng chậm.
    FlushTelegramQueue();
}

void OnTradeTransaction(const MqlTradeTransaction& trans,
                        const MqlTradeRequest&     req,
                        const MqlTradeResult&      res) {
    if(trans.type == TRADE_TRANSACTION_DEAL_ADD) {
        UpdateDayProfit();
        CheckDayLimit();
        UpdateGUI(true);
    }
}

void OnChartEvent(const int id, const long& lparam, const double& dparam, const string& sparam) {
    if(id != CHARTEVENT_OBJECT_CLICK) return;
    if     (sparam == GUI + "BtnCloseAll")    { ApplyBotEnabled(false); UpdateGUI(); }
    else if(sparam == GUI + "BtnOpenBuy") {
        if(!DayLimitHit && g_BotEnabled && CountBuy() < DCA_MaxOrd + 1) {
            Trade.SetExpertMagicNumber(0);
            OpenOrder(ORDER_TYPE_BUY, InpLotSize, g_TP_Points, g_SL_Points);
            Trade.SetExpertMagicNumber(InpMagic);
        }
    }
    else if(sparam == GUI + "BtnOpenSell") {
        if(!DayLimitHit && g_BotEnabled && CountSell() < DCA_MaxOrd + 1) {
            Trade.SetExpertMagicNumber(0);
            OpenOrder(ORDER_TYPE_SELL, InpLotSize, g_TP_Points, g_SL_Points);
            Trade.SetExpertMagicNumber(InpMagic);
        }
    }
    else if(sparam == GUI + "BtnBotToggle") {
        if(TimeCurrent() - g_LastBotToggleClick < 2) {
            Print("RTB: Bỏ qua click Bot Toggle — bấm quá nhanh (debounce 2 giây).");
        } else {
            g_LastBotToggleClick = TimeCurrent();
            ApplyBotEnabled(!g_BotEnabled);
            UpdateGUI();
        }
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
