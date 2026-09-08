#property copyright "Homie Bot v1.0"
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

input group         "══════ TIN TỨC (AUTO MT5 CALENDAR) ══════"; //
input  bool    InpNewsPauseEnabled = false;                    // Bật tự động dừng vào lệnh mới khi có tin
input  int     InpNewsPauseBefore  = 10;                       // Dừng trước tin (phút)
input  int     InpNewsResumeAfter  = 10;                       // Mở lại sau tin (phút)
input  string  InpNewsCurrencies   = "USD,XAU";                // Tiền tệ cần lọc (cách nhau bởi dấu ,)
input  int     InpNewsScanHours    = 24;                       // Quét lịch tin trước X giờ
input  int     InpNewsUpdateSec    = 60;                       // Cập nhật lịch mỗi X giây

input group         "══════ DCA - CÀI ĐẶT CHUNG ══════"; //
input  bool          InpDCAArithEnable = false; // DCA: Bật Vol Cấp Số Cộng (bỏ qua Hệ số Lot)
input  double        InpDCAArithStep   = 0.01;  // DCA: Cộng thêm Vol mỗi lệnh DCA sau (lots)
input  int           InpMaxPendingDCA  = 20;    // DCA: Trần lệnh chờ sống cùng lúc mỗi chiều (0=không giới hạn)

input group         "══════ DCA ══════"; //
input  double  InpDCA1Mult = 1.5;    // DCA: Hệ số Lot
input  int     InpDCA1Max  = 2;      // DCA: Max lệnh DCA tối đa
input  double  InpDCA1Dist = 1000.0; // DCA: Khoảng cách (points)
input  double  InpDCA1SL   = 0.0;    // DCA: SL (points, 0=tắt)

input group         "══════ NHỒI DƯƠNG (PYRA) ══════"; //
input  double  InpPyra1Mult = 1.0;    // PYRA: Hệ số Lot
input  int     InpPyra1Max  = 2;      // PYRA: Số lệnh nhồi dương tối đa (0=tắt)
input  double  InpPyra1Dist = 500.0;  // PYRA: Khoảng cách (points)
input  double  InpPyra1SL   = 0.0;    // PYRA: SL (points, 0=tắt)

input group         "══════ TỈA LỆNH (TRIMMING) ══════"; //
input  int     InpTrimTrigger    = 5;      // Kích hoạt khi số lệnh >= X
input  double  InpTrimTarget     = 10.0;   // Mục tiêu lợi nhuận sau tỉa ($)
input  int     InpTrimMaxLoss    = 1;      // Số lệnh âm tối đa gộp mỗi lần ghép cặp
input  int     InpTrimMaxWin     = 1;      // Số lệnh dương tối đa gộp mỗi lần ghép cặp
input  int     InpTrimMaxCycles  = 1;      // Số chu kỳ ghép cặp tối đa mỗi lượt tỉa

input group         "══════ TRAILING STOP ══════"; //
input  bool          InpTrailEnable   = false;        // Bật Trailing
input  int           InpTrailMinOrds  = 1;            // Số lệnh tối thiểu kích hoạt
input  double        InpTrailActivate = 500.0;        // Points kích hoạt Trail
input  double        InpTrailStep     = 200.0;        // Bước nhảy SL (points)
input  double        InpTrailInit     = 300.0;        // SL đầu tiên cách giá (points)

input group         "══════ NHỒI DƯƠNG - TRAILING RIÊNG ══════"; //
input  bool          InpPyraTrailEnable   = false;        // Bật Trailing riêng cho lệnh nhồi dương
input  int           InpPyraTrailMinOrds  = 1;            // Số lệnh nhồi dương tối thiểu kích hoạt
input  double        InpPyraTrailActivate = 500.0;        // Points kích hoạt Trail
input  double        InpPyraTrailStep     = 200.0;        // Bước nhảy SL (points)
input  double        InpPyraTrailInit     = 300.0;        // SL đầu tiên cách giá (points)

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
double        DCA_SL;

double        PYRA_Mult;
int           PYRA_MaxOrd;
double        PYRA_Dist;
double        PYRA_SL;

datetime LastOrderTime  = 0;
datetime LastEntryTime  = 0;
double   InitBalance    = 0.0;
double   MaxDrawdownPct = 0.0;
double   DayProfit      = 0.0;
double   DayProfitBuy   = 0.0;
double   DayProfitSell  = 0.0;
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

// Ticket lệnh chờ (Stop/Limit) đang neo cho tầng nhồi dương (Pyramiding) KẾ TIẾP theo từng chiều — khác
// CheckDCA() (mảng nhiều slot, có persist qua GlobalVariable): Pyramiding chỉ 1 tầng đang chờ tại 1 thời
// điểm nên chỉ cần 1 ticket/chiều, không cần persist vì mốc neo được PyraAnchorPrice() tính lại động mỗi
// tick từ vị thế lot lớn nhất đang sống — không phụ thuộc lịch sử giá cũ như DCA.
ulong    PyraBuyLimitTk  = 0;
ulong    PyraSellLimitTk = 0;

bool g_CalExpanded = false;
bool g_PanelCollapsed = false;
int  g_LastPanelBottom = 0;
int  g_CalRightEdge    = 0;
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

bool     g_BotEnabled           = true;
datetime g_LastBotToggleClick   = 0;

ENUM_DIRECTION   g_Direction;

int    g_OrderDelay;

bool   g_DCAArithEnable;
double g_DCAArithStep;

int    g_TrimTrigger, g_TrimMaxLoss, g_TrimMaxWin, g_TrimMaxCycles;
double g_TrimTarget;
bool   g_TrimEnabled = true;

bool             g_TrailEnable;
int              g_TrailMinOrds;
double           g_TrailActivate, g_TrailStep, g_TrailInit;

bool             g_PyraTrailEnable;
int              g_PyraTrailMinOrds;
double           g_PyraTrailActivate, g_PyraTrailStep, g_PyraTrailInit;

double g_CloseProfit, g_CloseLoss, g_ClosePerPips, g_DayMaxLoss, g_DayMaxProfit;

struct NewsItem { datetime time; string currency; };
NewsItem g_newsCache[];
datetime g_lastNewsUpdate = 0;
// true nếu EA đang thực sự bị chặn vào lệnh mới do tin (IsInNewsZone() tại tick gần nhất) — ghi lại ở
// đây (thay vì chỉ là biến cục bộ trong OnTick()) để UpdateGUI() đọc được và hiện lên panel.
bool g_NewsBlock = false;


bool IsManaged() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    return magic == (long)InpMagic;
}

bool IsManagedForTrim() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    return PositionGetInteger(POSITION_MAGIC) == (long)InpMagic;
}

bool IsManagedForDisplay() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    return PositionGetInteger(POSITION_MAGIC) == (long)InpMagic;
}

// Trailing chung (CheckTrailing()) chỉ quản lý lệnh gốc/DCA ("RTB|") — loại trừ lệnh nhồi dương ("RTP|")
// vì nhồi dương có bộ Trailing riêng (CheckPyraTrailing()), tránh 2 cơ chế cùng ghi đè SL 1 lệnh.
bool IsManagedForTrail() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) return false;
    return StringFind(PositionGetString(POSITION_COMMENT), "RTP|") != 0;
}

bool IsPyraManagedForTrail() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) return false;
    return StringFind(PositionGetString(POSITION_COMMENT), "RTP|") == 0;
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

// Đếm lệnh nhồi dương (Pyramiding) đang sống theo chiều posType — nhận diện qua prefix comment "RTP|"
// (khác "RTB|" của lệnh gốc/DCA). Đếm trực tiếp mỗi lần gọi, không dựa vào biến đếm runtime — an toàn
// kể cả khi EA restart giữa chừng.
int CountPyra(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") == 0) n++;
    }
    return n;
}

// Tổng số lệnh để so với ngưỡng InpTrimTrigger — LOẠI TRỪ lệnh nhồi dương đã khớp (comment "RTP|"), chỉ
// đếm lệnh gốc/DCA ("RTB|"). Tránh trường hợp tự kích hoạt vòng lặp: nhồi dương mở thêm vị thế → tổng
// lệnh tăng → chạm InpTrimTrigger → CheckPyramiding() lại được phép mở thêm tầng kế tiếp, tự đẩy ngưỡng
// lên bằng chính lệnh nó vừa mở. Lệnh RTP vẫn là ứng viên hợp lệ để CheckTrimming() chọn tỉa một khi
// ngưỡng đã đạt bằng lệnh gốc/DCA — chỉ không được tính vào phép so sánh ngưỡng này.
// Đếm lệnh DCA (âm) đang sống theo chiều posType — nhận diện qua prefix "RTB|" nhưng KHÁC "RTB|0|0"
// (đó là lệnh gốc, không phải tầng DCA). Dùng làm điều kiện phụ cho CheckPyramiding(): riêng CHIỀU đang
// xét cũng phải tự "chín" (đủ sâu DCA riêng của nó), tránh trường hợp chiều còn non chỉ nhờ chiều kia
// gánh đủ InpTrimTrigger tổng mà đã được nhồi dương ăn theo.
int CountDCA(int posType) {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        if(cmt == "RTB|0|0") continue; // loại lệnh gốc
        n++;
    }
    return n;
}

int CountAllForTrim() {
    int n = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForTrim()) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") == 0) continue;
        n++;
    }
    return n;
}
int CountAll()  { return CountBuy() + CountSell(); }

// Đếm lệnh chờ (Stop/Limit) đang neo theo chiều posType, lọc đúng theo prefix comment — dùng riêng cho
// hiển thị panel để tách bạch lệnh chờ DCA ("RTB|") với lệnh chờ Pyramiding ("RTP|"), vì cả hai đều dùng
// chung 4 loại lệnh Stop/Limit nên không thể phân biệt chỉ bằng ORDER_TYPE.
int CountPendingByPrefix(int posType, string prefix) {
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
        if(StringFind(OrderGetString(ORDER_COMMENT), prefix) != 0) continue;
        cnt++;
    }
    return cnt;
}
int CountPendingDCA(int posType)  { return CountPendingByPrefix(posType, "RTB|"); }
int CountPendingPyra(int posType) { return CountPendingByPrefix(posType, "RTP|"); }

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

// Tổng floating (profit+swap) gộp CẢ 2 chiều Buy+Sell của riêng lệnh DCA (nhồi âm) — "RTB|" nhưng loại
// lệnh gốc "RTB|0|0" — và riêng lệnh Pyramiding (nhồi dương) — "RTP|". Dùng cho panel, tách biệt hiệu
// suất đang trôi nổi của 2 cơ chế khỏi FloatProfit() tổng (gồm cả lệnh gốc).
double FloatDCA() {
    double p = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        string cmt = PositionGetString(POSITION_COMMENT);
        if(StringFind(cmt, "RTB|") != 0) continue;
        if(cmt == "RTB|0|0") continue; // loại lệnh gốc
        p += PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP);
    }
    return p;
}

double FloatPyra() {
    double p = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") != 0) continue;
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

// Mốc neo khoảng cách cho Pyramiding (nhồi dương) — KHÔNG neo theo lệnh gốc ("RTB|0|0") và KHÔNG neo
// theo lệnh mở gần nhất (khác LastOpenPrice()). Neo theo lệnh TỆ GIÁ NHẤT còn lại trong giỏ cùng chiều:
// BUY lấy giá THẤP NHẤT, SELL lấy giá CAO NHẤT (thường chính là tầng DCA sâu nhất). Nếu giỏ chỉ có mỗi
// lệnh gốc (chưa có DCA/Pyra nào khác) → trả về 0, CheckPyramiding() sẽ không kích hoạt.
double PyraAnchorPrice(int posType) {
    double anchor = 0;
    bool   found  = false;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(PositionGetString(POSITION_COMMENT) == "RTB|0|0") continue; // loại lệnh gốc

        double opn = PositionGetDouble(POSITION_PRICE_OPEN);
        if(!found) { anchor = opn; found = true; }
        else if(posType == POSITION_TYPE_BUY) { if(opn < anchor) anchor = opn; }
        else                                   { if(opn > anchor) anchor = opn; }
    }
    return anchor;
}

// Giá tham chiếu để tính target cho TẦNG PYRA KẾ TIẾP — KHÁC PyraAnchorPrice() (chỉ dùng làm điều kiện
// gác, luôn là giá TỆ NHẤT trong giỏ, gần như đứng yên). Frontier là điểm XA NHẤT mà chuỗi Pyra đã thực
// sự vươn tới: MAX(anchor, giá lệnh Pyra tốt nhất hiện có) cho BUY / MIN(...) cho SELL.
//
// Lý do tách riêng: nếu dùng thẳng anchor để tính target, mọi tầng Pyra đều nhắm về đúng 1 mức cố định
// "anchor ± dist" — tầng 1 khớp xong, tầng 2 vẫn tính lại đúng mức đó (anchor không đổi), giá chỉ cần dao
// động qua lại quanh mức này là chồng lệnh Pyra liên tục tại cùng 1 điểm (bug thực tế quan sát được: một
// chuỗi lệnh 0.03 lot mở sát nhau chỉ cách vài chục point thay vì cách đều PYRA_Dist). Frontier khắc phục
// bằng cách LUÔN tính tầng kế tiếp từ tầng Pyra gần nhất đã khớp (nếu có), buộc mỗi tầng phải tiến xa hơn
// tầng trước đúng 1 khoảng dist — giá quay lại mức tầng trước sẽ không kích hoạt gì thêm.
double PyraFrontierPrice(int posType, double anchor) {
    double frontier = anchor;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") != 0) continue;
        double opn = PositionGetDouble(POSITION_PRICE_OPEN);
        if(posType == POSITION_TYPE_BUY) { if(opn > frontier) frontier = opn; }
        else                              { if(opn < frontier) frontier = opn; }
    }
    return frontier;
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

bool WorstTrimCandidate(double &outProfit, double &outPts, double &outLot, double &outPrice, int &outType) {
    outProfit = 0; outPts = 0; outLot = 0; outPrice = 0; outType = -1;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

    bool   found      = false;
    double bestMetric = 0;

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManagedForTrim()) continue;
        if(PositionGetString(POSITION_COMMENT) == "RTB|0|0") continue;

        double profit = PositionGetDouble(POSITION_PROFIT);
        int    pt  = (int)PositionGetInteger(POSITION_TYPE);
        double opn = PositionGetDouble(POSITION_PRICE_OPEN);
        double pts = (pt == POSITION_TYPE_BUY) ? (bid - opn) / point : (opn - ask) / point;

        if(!found || pts < bestMetric) {
            found      = true;
            bestMetric = pts;
            outProfit  = profit;
            outPts     = pts;
            outLot     = PositionGetDouble(POSITION_VOLUME);
            outPrice   = opn;
            outType    = pt;
        }
    }
    return found;
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
        PyraBuyLimitTk = 0;
        ClearSlotPrices(POSITION_TYPE_BUY, ArraySize(DCABuyPrices));
        SavePeak(POSITION_TYPE_BUY, 0);
    }
    if(posType < 0 || posType == POSITION_TYPE_SELL) {
        PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
        OrigSellPrice = 0;
        PyraSellLimitTk = 0;
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

bool OpenOrder(int ordType, double lot, double tp_pts = 0, double sl_pts = 0, bool isDCA = false, int slotIdx = -1, bool isPyra = false) {
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
    if(isPyra) {
        comment = (tp_pts == 0 && sl_pts == 0) ? "RTP|0|0" : StringFormat("RTP|%.0f|%.0f", tp_pts, sl_pts);
    }
    else if(isDCA) {
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
        string tag = isPyra ? " [PYRA]" : (isDCA ? " [DCA]" : " [Entry]");
        Print("RTB: Open ", (ordType == ORDER_TYPE_BUY ? "BUY" : "SELL"),
              " lot=", lot, " tp=", tp, " sl=", sl, tag);
    } else {
        Print("RTB: OpenOrder FAILED type=", ordType, " err=", GetLastError());
    }
    return ok;
}


int SignalSimulated() {
    if(g_Direction == DIR_ONLY_BUY)  return  1;
    if(g_Direction == DIR_ONLY_SELL) return -1;
    return 2;
}

int GetSignal() {
    return SignalSimulated();
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

// Nạp lại cache lịch tin High Importance trong khung [now - InpNewsPauseBefore phút, now + InpNewsScanHours
// giờ], lọc theo danh sách tiền tệ InpNewsCurrencies — throttle bằng InpNewsUpdateSec để không gọi
// CalendarValueHistory() mỗi tick (tốn CPU không cần thiết).
void UpdateNewsCache() {
    if(TimeCurrent() - g_lastNewsUpdate < InpNewsUpdateSec) return;
    g_lastNewsUpdate = TimeCurrent();
    ArrayResize(g_newsCache, 0);

    datetime from = TimeCurrent() - (datetime)(InpNewsPauseBefore * 60);
    datetime to   = TimeCurrent() + (datetime)(InpNewsScanHours   * 3600);

    MqlCalendarValue values[];
    int total = CalendarValueHistory(values, from, to, NULL, NULL);
    if(total <= 0) return;

    string currencies[];
    int n = StringSplit(InpNewsCurrencies, StringGetCharacter(",", 0), currencies);
    for(int i = 0; i < n; i++) { StringTrimLeft(currencies[i]); StringTrimRight(currencies[i]); }

    for(int i = 0; i < total; i++) {
        MqlCalendarEvent  event;
        MqlCalendarCountry country;
        if(!CalendarEventById(values[i].event_id, event))   continue;
        if(event.importance != CALENDAR_IMPORTANCE_HIGH)    continue;
        if(!CalendarCountryById(event.country_id, country)) continue;

        bool found = false;
        for(int j = 0; j < n; j++)
            if(currencies[j] == country.currency) { found = true; break; }
        if(!found) continue;

        int idx = ArraySize(g_newsCache);
        ArrayResize(g_newsCache, idx + 1);
        g_newsCache[idx].time     = values[i].time;
        g_newsCache[idx].currency = country.currency;
    }
}

// true nếu hiện đang trong khung [tin - InpNewsPauseBefore phút, tin + InpNewsResumeAfter phút] của bất kỳ
// tin nào trong cache.
bool IsInNewsZone() {
    datetime now    = TimeCurrent();
    datetime before = (datetime)(InpNewsPauseBefore * 60);
    datetime after  = (datetime)(InpNewsResumeAfter  * 60);
    for(int i = 0; i < ArraySize(g_newsCache); i++) {
        datetime t = g_newsCache[i].time;
        if(now >= t - before && now <= t + after) return true;
    }
    return false;
}

// Tin sắp tới TRONG HÔM NAY (theo giờ server) mà chưa xảy ra — dùng để hiện lên panel "còn bao lâu nữa
// thì có tin". Chỉ tìm được tin nằm trong cửa sổ đã quét (g_newsCache quét tới InpNewsScanHours giờ tới
// kể từ lần UpdateNewsCache() gần nhất) — nếu InpNewsScanHours đặt ngắn hơn phần còn lại của hôm nay, tin
// muộn hơn trong ngày có thể chưa xuất hiện trong cache dù thực tế vẫn có.
bool GetNextNewsToday(datetime &outTime, string &outCurrency) {
    MqlDateTime nowDt;
    TimeToStruct(TimeCurrent(), nowDt);
    datetime todayStart = StringToTime(StringFormat("%04d.%02d.%02d 00:00:00", nowDt.year, nowDt.mon, nowDt.day));
    datetime todayEnd   = todayStart + 86400;
    datetime now        = TimeCurrent();

    bool found = false;
    datetime best = 0;
    string   bestCcy = "";
    for(int i = 0; i < ArraySize(g_newsCache); i++) {
        datetime t = g_newsCache[i].time;
        if(t < todayStart || t >= todayEnd) continue; // chỉ tính tin trong hôm nay
        if(t < now) continue;                          // tin đã xảy ra — không tính "còn bao lâu"
        if(!found || t < best) { found = true; best = t; bestCcy = g_newsCache[i].currency; }
    }
    outTime     = best;
    outCurrency = bestCcy;
    return found;
}

// Định dạng khoảng thời gian (giây) thành chuỗi ngắn gọn kiểu "2g 15p" / "45p" / "1ngày 3g" cho panel.
string FormatDuration(long secs) {
    if(secs < 0) secs = 0;
    long mins = secs / 60;
    long hrs  = mins / 60;
    long days = hrs  / 24;
    mins %= 60;
    hrs  %= 24;
    if(days > 0) return StringFormat("%dngày %dg", days, hrs);
    if(hrs  > 0) return StringFormat("%dg %02dp", hrs, mins);
    return StringFormat("%dp", mins);
}

void CheckEntry() {
    if(DayLimitHit) return;
    if(!g_BotEnabled) return;
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

    // Ứng viên refill GẦN giá hiện tại nhất trong toàn bộ các slot đang cần đặt lại lệnh chờ — chỉ ghi
    // nhận trong vòng lặp bên dưới, đặt lệnh thật SAU vòng lặp (xem lý do ở comment gần chỗ dùng).
    int    bestSlot = -1;
    double bestDist = -1;

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
                // KHÔNG đặt lệnh ngay khi gặp slot đầu tiên cần refill theo thứ tự index tăng dần — với 1
                // chuỗi DCA giá chạy liên tục 1 hướng, index tăng dần lại là thứ tự XA→GẦN giá hiện tại, nên
                // vòng lặp sẽ ưu tiên nhồi các slot xa trước; đến lượt slot gần hơn, trần InpMaxPendingDCA
                // đã đầy nên MakeRoomForPendingDCA() lại huỷ đúng slot xa vừa đặt ở lượt trước để nhường chỗ
                // — tự dẫm chân lên nhau, đặt-rồi-huỷ liên tục ("kéo lệnh xuống dần" từng slot một, quan sát
                // thực tế qua log) thay vì hội tụ thẳng về đúng 20 slot gần giá nhất. Sửa bằng cách chỉ GHI
                // NHẬN slot này là ứng viên ở đây, rồi chọn đúng 1 ứng viên GẦN GIÁ NHẤT trong toàn bộ slot
                // đang cần refill để đặt lệnh thật, sau khi vòng lặp quét hết (xem khối code ngay sau vòng lặp).
                double distNow = MathAbs(slotPrice - ((posType == POSITION_TYPE_BUY) ? ask : bid));
                if(bestSlot < 0 || distNow < bestDist) { bestSlot = slot; bestDist = distNow; }
            }
        } else {
            if(posType == POSITION_TYPE_BUY) { DCABuyBounced[slot] = false; DCABuyLimitTk[slot]  = 0; }
            else                              { DCASellBounced[slot] = false; DCASellLimitTk[slot] = 0; }
        }
    }

    if(bestSlot >= 0 && TimeCurrent() - LastOrderTime >= g_OrderDelay) {
        int    slot      = bestSlot;
        double slotPrice = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot] : DCASellPrices[slot];

        double tolDup = 50.0 * point;
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
        } else if(MakeRoomForPendingDCA(posType, slotPrice)) {
            double lot = DCAOrderLot(InpLotSize, slot + 1);
            string cmt = (DCA_SL == 0
                ? "RTB|0|0"
                : "RTB|0|" + IntegerToString((int)DCA_SL))
                + "|S" + IntegerToString(slot) + "|RF";
            bool ok = false;
            if(posType == POSITION_TYPE_BUY) {
                double sl_p = DCA_SL > 0 ? NormalizeDouble(slotPrice - DCA_SL * point, _Digits) : 0;
                if(ask > slotPrice)
                    ok = Trade.BuyLimit(lot, slotPrice, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
                else if(ask < slotPrice)
                    ok = Trade.BuyStop(lot, slotPrice, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
            } else {
                double sl_p = DCA_SL > 0 ? NormalizeDouble(slotPrice + DCA_SL * point, _Digits) : 0;
                if(bid < slotPrice)
                    ok = Trade.SellLimit(lot, slotPrice, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
                else if(bid > slotPrice)
                    ok = Trade.SellStop(lot, slotPrice, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
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
        bool ok = OpenOrder(ord, lot, 0, DCA_SL, true, peak);
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

    string cmt = (DCA_SL == 0
        ? "RTB|0|0"
        : "RTB|0|" + IntegerToString((int)DCA_SL))
        + "|S" + IntegerToString(peak);
    bool placed;
    if(posType == POSITION_TYPE_BUY) {
        double sl_p = DCA_SL > 0 ? NormalizeDouble(target - DCA_SL * point, _Digits) : 0;
        placed = Trade.BuyLimit(lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
    } else {
        double sl_p = DCA_SL > 0 ? NormalizeDouble(target + DCA_SL * point, _Digits) : 0;
        placed = Trade.SellLimit(lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt);
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

//+------------------------------------------------------------------+
//| NHỒI DƯƠNG (PYRAMIDING) — gói gọn 1 tầng, đặt LỆNH CHỜ tránh trượt giá |
//+------------------------------------------------------------------+

ulong FindLivePyraOrder(int posType) {
    ENUM_ORDER_TYPE t1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE t2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    ulong found = 0;
    for(int i = OrdersTotal() - 1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(tk == 0 || !OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if((long)OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot != t1 && ot != t2) continue;
        if(StringFind(OrderGetString(ORDER_COMMENT), "RTP|") != 0) continue;
        if(found == 0) found = tk;
        else Trade.OrderDelete(tk);
    }
    return found;
}

// Huỷ lệnh chờ Pyramiding đang neo (nếu có) và xoá ticket runtime — gọi khi điều kiện nhồi dương không
// còn thoả (basket rỗng, Tỉa Lệnh tắt/chưa đạt ngưỡng, đã đạt PYRA_MaxOrd, hoặc mất mốc neo hợp lệ).
void ResetPyraPending(int posType) {
    ulong tk = (posType == POSITION_TYPE_BUY) ? PyraBuyLimitTk : PyraSellLimitTk;
    if(tk == 0) tk = FindLivePyraOrder(posType);
    if(tk > 0 && OrderSelect(tk)) Trade.OrderDelete(tk);
    if(posType == POSITION_TYPE_BUY) PyraBuyLimitTk = 0; else PyraSellLimitTk = 0;
}

datetime g_PyraDiagLast[2] = {0, 0}; // throttle log chẩn đoán (index 0=BUY, 1=SELL) — tránh spam mỗi giây

// In lý do KHÔNG kích hoạt nhồi dương ra log Experts, throttle 30 giây/chiều — các early-return trong
// CheckPyramiding() vốn im lặng hoàn toàn, khiến không cách nào biết đang bị chặn bởi điều kiện nào khi
// quan sát từ panel (chỉ thấy "Limit 0 lệnh" mãi không đổi) mà không có log kèm theo.
void PyraDiagLog(int posType, string reason) {
    int idx = (posType == POSITION_TYPE_BUY) ? 0 : 1;
    if(TimeCurrent() - g_PyraDiagLast[idx] < 30) return;
    g_PyraDiagLast[idx] = TimeCurrent();
    Print("RTB: Pyramiding (nhồi dương) ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
          " KHÔNG kích hoạt — ", reason);
}

void CheckPyramiding(int posType) {
    if(!g_TrimEnabled) {
        PyraDiagLog(posType, "Tỉa Lệnh (InpTrimMode/nút Trim) đang TẮT");
        ResetPyraPending(posType); return;
    }
    int allCnt = CountAllForTrim();
    if(allCnt < g_TrimTrigger) {
        PyraDiagLog(posType, StringFormat("chưa đạt InpTrimTrigger (%d / %d)", allCnt, g_TrimTrigger));
        ResetPyraPending(posType); return;
    }
    // Riêng chiều đang xét phải tự có đủ độ sâu DCA (> nửa ngưỡng) — không để chiều còn non ăn theo vì
    // chiều kia đã gánh đủ InpTrimTrigger tổng.
    int dcaCnt  = CountDCA(posType);
    int dcaNeed = g_TrimTrigger / 2;
    if(dcaCnt <= dcaNeed) {
        PyraDiagLog(posType, StringFormat("DCA riêng chiều này chưa đủ sâu (%d, cần > %d)", dcaCnt, dcaNeed));
        ResetPyraPending(posType); return;
    }

    int count = CountPos(posType);
    if(count == 0) { ResetPyraPending(posType); return; }

    int pyraCount = CountPyra(posType);
    if(pyraCount >= PYRA_MaxOrd) {
        PyraDiagLog(posType, StringFormat("đã đạt PYRA_MaxOrd (%d / %d)", pyraCount, PYRA_MaxOrd));
        ResetPyraPending(posType); return;
    }

    double lastPrice = PyraAnchorPrice(posType);
    if(lastPrice == 0) {
        PyraDiagLog(posType, "chưa có mốc neo hợp lệ — cần ít nhất 1 lệnh DCA/Pyra khác lệnh gốc");
        ResetPyraPending(posType); return;
    }

    double frontier = PyraFrontierPrice(posType, lastPrice);

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double dist  = PYRA_Dist * point;
    double target = (posType == POSITION_TYPE_BUY) ? NormalizeDouble(frontier + dist, _Digits)
                                                    : NormalizeDouble(frontier - dist, _Digits);

    ulong pendTk = (posType == POSITION_TYPE_BUY) ? PyraBuyLimitTk : PyraSellLimitTk;
    if(pendTk == 0) pendTk = FindLivePyraOrder(posType);

    if(pendTk > 0) {
        if(!OrderSelect(pendTk)) {
            // Lệnh chờ đã khớp thành vị thế (hoặc bị huỷ từ bên ngoài) — dọn ticket, để logic bên dưới
            // tự đặt lệnh chờ tầng kế tiếp nếu pyraCount vẫn còn dư quota.
            pendTk = 0;
        } else {
            double curPrice   = OrderGetDouble(ORDER_PRICE_OPEN);
            double repriceTol = 50.0 * point;
            if(MathAbs(curPrice - target) > repriceTol) {
                // Mốc neo (lệnh giá tệ nhất) đã đổi kể từ lúc đặt — huỷ lệnh chờ cũ để đặt lại đúng mục
                // tiêu mới ở nhánh bên dưới, thay vì để lệnh chờ neo sai giá.
                if(!Trade.OrderDelete(pendTk)) {
                    Print("RTB: Huỷ lệnh chờ Pyra cũ để reprice thất bại, err=", GetLastError());
                    if(posType == POSITION_TYPE_BUY) PyraBuyLimitTk = pendTk; else PyraSellLimitTk = pendTk;
                    return;
                }
                pendTk = 0;
            } else {
                if(posType == POSITION_TYPE_BUY) PyraBuyLimitTk = pendTk; else PyraSellLimitTk = pendTk;
                return; // lệnh chờ vẫn đúng mục tiêu — chờ khớp, không làm gì thêm
            }
        }
    }
    if(posType == POSITION_TYPE_BUY) PyraBuyLimitTk = 0; else PyraSellLimitTk = 0;

    if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

    double lot  = NormLot(InpLotSize * PYRA_Mult);
    string cmt  = (PYRA_SL == 0) ? "RTP|0|0" : StringFormat("RTP|0|%.0f", PYRA_SL);
    bool   ok   = false;
    string ordKind = "";
    // Dùng if/else (không phải if / else-if) để LUÔN gọi đúng 1 lệnh Trade.* — nếu ask/bid trùng khít
    // target (hiếm nhưng có thể xảy ra), nhánh else-if cũ để cả 2 điều kiện cùng false, ok vẫn false mà
    // KHÔNG hề gọi Trade.*, khiến GetLastError() bên dưới trả về lỗi CŨ không liên quan lần thử này.
    if(posType == POSITION_TYPE_BUY) {
        double sl_p = PYRA_SL > 0 ? NormalizeDouble(target - PYRA_SL * point, _Digits) : 0;
        if(ask > target) { ordKind = "BuyLimit"; ok = Trade.BuyLimit(lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt); }
        else              { ordKind = "BuyStop";  ok = Trade.BuyStop (lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt); }
    } else {
        double sl_p = PYRA_SL > 0 ? NormalizeDouble(target + PYRA_SL * point, _Digits) : 0;
        if(bid < target) { ordKind = "SellLimit"; ok = Trade.SellLimit(lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt); }
        else              { ordKind = "SellStop";  ok = Trade.SellStop (lot, target, _Symbol, sl_p, 0, ORDER_TIME_GTC, 0, cmt); }
    }
    if(ok) {
        ulong tk = Trade.ResultOrder();
        if(tk > 0) {
            if(posType == POSITION_TYPE_BUY) PyraBuyLimitTk = tk; else PyraSellLimitTk = tk;
            LastOrderTime = TimeCurrent();
            Print("RTB: Đặt lệnh chờ Pyramiding (nhồi dương) ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
                  " (", ordKind, ") tại ", target, " pyraCount=", pyraCount, "/", PYRA_MaxOrd,
                  " anchor=", lastPrice, " frontier=", frontier);
        }
    } else {
        Print("RTB: Đặt lệnh chờ Pyramiding (", ordKind, ") ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"),
              " thất bại tại target=", target, " (ask=", ask, " bid=", bid, "), err=", GetLastError());
    }
}

void CheckTrimming() {
    if(!g_TrimEnabled) return;
    if(CountAllForTrim() < g_TrimTrigger) return;

    int    totalPos = PositionsTotal();
    ulong  tks[];
    double profits[];
    double pts[];
    bool   isOrig[];
    bool   used[];
    ArrayResize(tks, totalPos);
    ArrayResize(profits, totalPos);
    ArrayResize(pts, totalPos);
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
        isOrig[cnt]  = (PositionGetString(POSITION_COMMENT) == "RTB|0|0");
        used[cnt]    = false;
        cnt++;
    }

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
                Trade.PositionClose(tks[winIdx[i]]);
                Print("RTB: Trim đóng ticket=", tks[winIdx[i]], " (thắng) profit=", profits[winIdx[i]]);
            }
            for(int i = 0; i < wn;  i++) {
                Trade.PositionClose(tks[worstIdx[i]]);
                Print("RTB: Trim đóng ticket=", tks[worstIdx[i]], " (thua) profit=", profits[worstIdx[i]]);
            }
            closedCycles++;
        } else break;
    }
    if(closedCycles > 0)
        Print("RTB: Hedge-by-Points trim cycles=", closedCycles, " x up to ", g_TrimMaxWin, " winners / ", g_TrimMaxLoss, " losers");
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

// Trailing riêng cho lệnh nhồi dương ("RTP|") — độc lập hoàn toàn với CheckTrailing() (lệnh gốc/DCA),
// tham số kích hoạt/bước nhảy/SL đầu tiên riêng (g_PyraTrail*), không dùng chung ngưỡng với Trailing chung.
void CheckPyraTrailing() {
    if(!g_PyraTrailEnable) return;
    if(CountPyra(POSITION_TYPE_BUY) + CountPyra(POSITION_TYPE_SELL) < g_PyraTrailMinOrds) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsPyraManagedForTrail()) continue;

        int    pt        = (int)PositionGetInteger(POSITION_TYPE);
        double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);

        if(pt == POSITION_TYPE_BUY) {
            double profitPts = (bid - openPrice) / point;
            if(profitPts >= g_PyraTrailActivate) {
                double newSL = bid - g_PyraTrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL >= curSL + g_PyraTrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_BUY, newSL);
            }
        } else {
            double profitPts = (openPrice - ask) / point;
            if(profitPts >= g_PyraTrailActivate) {
                double newSL = ask + g_PyraTrailInit * point;
                double curSL = PositionGetDouble(POSITION_SL);
                if(curSL == 0 || newSL <= curSL - g_PyraTrailStep * point)
                    ApplyTrailToPos(tk, POSITION_TYPE_SELL, newSL);
            }
        }
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
            if(StringFind(cmt, "RTB|") != 0 && StringFind(cmt, "RTP|") != 0) continue;
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

void UpdateDayProfit() {
    MqlDateTime dt;
    TimeToStruct(TimeCurrent(), dt);
    if(dt.day != LastDay) {
        DayProfit     = 0;
        DayProfitBuy  = 0;
        DayProfitSell = 0;
        DayLimitHit   = false;
        LastDay       = dt.day;
    }

    datetime dayStart = StringToTime(StringFormat("%04d.%02d.%02d 00:00:00",
                         dt.year, dt.mon, dt.day));
    if(!HistorySelect(dayStart, TimeCurrent())) return;

    double closed = 0, closedBuy = 0, closedSell = 0;
    for(int i = 0; i < HistoryDealsTotal(); i++) {
        ulong dTk = HistoryDealGetTicket(i);
        if(HistoryDealGetString(dTk, DEAL_SYMBOL) != _Symbol) continue;
        ENUM_DEAL_ENTRY de = (ENUM_DEAL_ENTRY)HistoryDealGetInteger(dTk, DEAL_ENTRY);
        if(de != DEAL_ENTRY_OUT && de != DEAL_ENTRY_OUT_BY) continue;
        double p = HistoryDealGetDouble(dTk, DEAL_PROFIT) + HistoryDealGetDouble(dTk, DEAL_SWAP);
        closed += p;
        // Deal ĐÓNG vị thế mang loại NGƯỢC hướng gốc: đóng 1 lệnh BUY thực hiện bằng deal SELL và
        // ngược lại — dùng DEAL_TYPE của chính deal OUT này để suy ra chiều của vị thế vừa đóng.
        ENUM_DEAL_TYPE dType = (ENUM_DEAL_TYPE)HistoryDealGetInteger(dTk, DEAL_TYPE);
        if(dType == DEAL_TYPE_SELL)      closedBuy  += p; // deal SELL đóng 1 lệnh BUY
        else if(dType == DEAL_TYPE_BUY)  closedSell += p; // deal BUY đóng 1 lệnh SELL
    }
    DayProfit     = closed;
    DayProfitBuy  = closedBuy;
    DayProfitSell = closedSell;
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

// Ô nhập liệu (OBJ_EDIT) — CHỈ gán OBJPROP_TEXT lúc mới tạo object (khác Lbl/CreateBtn luôn ghi đè text
// mỗi lần gọi). UpdateGUI() chạy lại mỗi giây qua OnTimer(); nếu ghi đè text mỗi lần vẽ, chữ người dùng
// đang gõ dở sẽ bị xoá mất trước khi họ kịp nhấn Enter để xác nhận.
void CreateEdit(string name, string initText, int x, int y, int w, int h) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_EDIT, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,       CORNER_LEFT_UPPER);
        ObjectSetString(0,  obj, OBJPROP_FONT,         "Consolas");
        ObjectSetInteger(0, obj, OBJPROP_FONTSIZE,     10);
        ObjectSetInteger(0, obj, OBJPROP_ALIGN,        ALIGN_RIGHT);
        ObjectSetInteger(0, obj, OBJPROP_COLOR,        clrWhite);
        ObjectSetInteger(0, obj, OBJPROP_BGCOLOR,      C'30,38,58');
        ObjectSetInteger(0, obj, OBJPROP_BORDER_COLOR, C'90,160,255');
        ObjectSetInteger(0, obj, OBJPROP_BACK,         false);
        // SELECTABLE=false, khớp với mọi object tương tác khác trong file (xem CreateBtn) — với OBJ_EDIT,
        // SELECTABLE=true khiến click vào ô bị MT5 hiểu là "chọn object" (chế độ kéo/di chuyển) thay vì
        // đặt con trỏ gõ chữ, nên trước đó bấm "Sửa" xong không gõ được số vào ô.
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE,   false);
        ObjectSetInteger(0, obj, OBJPROP_READONLY,     false);
        ObjectSetString(0,  obj, OBJPROP_TEXT,         initText);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, x);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, y);
    ObjectSetInteger(0, obj, OBJPROP_XSIZE,     w);
    ObjectSetInteger(0, obj, OBJPROP_YSIZE,     h);
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
        // Lề rộng + hệ số an toàn 15% — TextGetSize() đo theo canvas/bitmap, còn OBJ_LABEL do chart tự
        // render (font hinting/DPI màn hình có thể khác), nên bề rộng đo được không khớp tuyệt đối bề
        // rộng thật hiển thị. Lề cố định nhỏ (14px) từng khiến số dài (âm lớn, lot cộng dồn nhiều chữ số)
        // chạm/tràn viền ô — nới lề theo tỉ lệ % để tự co giãn khi số càng dài thay vì hằng số cố định.
        colW = MathMax(92, (int)(maxContentW * 1.15) + 24);
    }
    int calW = colW * 7;
    g_CalRightEdge = calX + calW;

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

// Chiều cao "tự nhiên" (chưa co giãn) của panel Trim/DCA bên phải — hằng số thuần (không phụ
// thuộc dữ liệu runtime), dùng chung bởi UpdateGUI() (để so khớp chiều cao 2 panel) và UpdateTrimSidePanel()
// (để vẽ từng card) — tránh 2 nơi tính công thức trùng lặp rồi lệch nhau khi sửa sau này.
int SidePanelCardH(int &trimStatsCardH, int &trailCardH, int &closeCardH, int &dcaDirCardH) {
    int rowH   = 15;
    int titleH = 24;
    int hdrGap = 6;

    trimStatsCardH = 6 + 13 + hdrGap + rowH*4 + 4 + 18 + 6;
    trailCardH     = 6 + 13 + hdrGap + (18+4) + rowH*2 + 6;
    closeCardH     = 6 + 13 + hdrGap + rowH*5 + 6;
    dcaDirCardH    = 6 + 13 + hdrGap + rowH*4 + 4 + rowH*4 + 6;
    return titleH + trimStatsCardH + 8 + trailCardH + 8 + closeCardH + 8 + dcaDirCardH + 8;
}

void UpdateTrimSidePanel() {
    if(g_PanelCollapsed) {
        string delObjs[] = {
            "TrimSideBG", "TrimSideH",
            "TrimStatsCard", "TrimStatsCardBar", "TrimStatsH",
            "TrimCntL", "TrimCntV",
            "TrimWorstL", "TrimWorstV",
            "TrimWorstDetailL", "TrimWorstDetailV",
            "BtnTrimToggle",
            "TrailCard", "TrailCardBar", "TrailH",
            "BtnTrailToggle", "BtnPyraTrailToggle",
            "TrailCntL", "TrailCntV", "PyraTrailCntL", "PyraTrailCntV",
            "CloseCard", "CloseCardBar", "CloseH",
            "CloseProfitL", "CloseProfitV",
            "CloseLossL", "CloseLossV", "ClosePipsL", "ClosePipsV",
            "CloseDayTgtL", "CloseDayTgtV", "CloseStL", "CloseStV",
            "DcaDirCard", "DcaDirCardBar", "DcaDirH", "BtnDCABuyToggle", "BtnDCASellToggle",
            "DcaLvlBuyL", "DcaLvlBuyV", "DcaLvlSellL", "DcaLvlSellV",
            "PyraLvlBuyL", "PyraLvlBuyV", "PyraLvlSellL", "PyraLvlSellV",
            "DcaBuyTodayL", "DcaBuyTodayV", "DcaSellTodayL", "DcaSellTodayV",
            "DcaFloatL", "DcaFloatV", "PyraFloatL", "PyraFloatV"
        };
        for(int i = 0; i < ArraySize(delObjs); i++) ObjectDelete(0, GUI + delObjs[i]);
        return;
    }

    int sideX     = g_CalExpanded ? (g_CalRightEdge + 12) : (InpPanelX + InpPanelWidth + InpCalPanelGap);
    int sideY     = InpCalPanelY + RTB_TITLEBAR_H;
    int sideW     = 230;
    int rightEdge = sideX + sideW - 8;
    int rowH      = 15;
    int titleH    = 24;
    int hdrGap    = 6;

    int    trimCount   = CountAllForTrim();
    color  trimCntClr  = (g_TrimEnabled && trimCount >= g_TrimTrigger) ? clrLimeGreen : C'231,236,245';

    double wProfit, wPts, wLot, wPrice;
    int    wType;
    bool   hasWorst = WorstTrimCandidate(wProfit, wPts, wLot, wPrice, wType);
    string worstTxt = hasWorst ? StringFormat("%s$%.2f", wProfit >= 0 ? "+" : "-", MathAbs(wProfit)) : "—";
    color  worstClr = hasWorst ? ((wProfit < 0) ? clrTomato : clrLimeGreen) : C'127,139,163';
    string worstDetailTxt = hasWorst
        ? StringFormat("%s %.2f @ %s", (wType == POSITION_TYPE_BUY ? "Buy" : "Sell"), wLot, DoubleToString(wPrice, _Digits))
        : "—";

    color  cardAccent = C'90,160,255';

    int trimStatsCardH, trailCardH, closeCardH, dcaDirCardH;
    int cardH = SidePanelCardH(trimStatsCardH, trailCardH, closeCardH, dcaDirCardH);

    int extraH = MathMax(0, (g_LastPanelBottom - sideY) - cardH);
    dcaDirCardH += extraH;
    cardH       += extraH;

    string bgc = GUI + "TrimSideBG";
    if(ObjectFind(0, bgc) < 0) {
        ObjectCreate(0, bgc, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bgc, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bgc, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bgc, OBJPROP_COLOR,       C'60,80,140');
        ObjectSetInteger(0, bgc, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bgc, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bgc, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bgc, OBJPROP_XDISTANCE, sideX);
    ObjectSetInteger(0, bgc, OBJPROP_YDISTANCE, sideY);
    ObjectSetInteger(0, bgc, OBJPROP_XSIZE,     sideW);
    ObjectSetInteger(0, bgc, OBJPROP_YSIZE,     cardH);
    ObjectSetInteger(0, bgc, OBJPROP_BGCOLOR,   C'10,13,20');

    Lbl("TrimSideH", "THÔNG TIN", sideX + 8, sideY + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "TrimSideH", OBJPROP_FONT, "Calibri Bold");

    int y = sideY + titleH;

    CreateRect("TrimStatsCard",    sideX, y, sideW, trimStatsCardH, C'20,28,44');
    CreateRect("TrimStatsCardBar", sideX, y, 2,     trimStatsCardH, cardAccent);
    Lbl("TrimStatsH", "TỈA LỆNH", sideX + 8, y + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "TrimStatsH", OBJPROP_FONT, "Calibri Bold");
    {
        int yS = y + 6 + 13 + hdrGap;
        Lbl ("TrimCntL", "Số lệnh", sideX + 8, yS, C'127,139,163', 10);
        LblR("TrimCntV", StringFormat("%d / %d", trimCount, g_TrimTrigger), rightEdge, yS, trimCntClr, 10); yS += rowH;

        Lbl ("TrimWorstL", "Lệnh phải tỉa", sideX + 8, yS, C'127,139,163', 10);
        LblR("TrimWorstV", worstTxt, rightEdge, yS, worstClr, 10); yS += rowH;

        // Nhãn + giá trị TÁCH RIÊNG 2 hàng (giống "Tin tiếp theo") thay vì cùng hàng — giá trị dạng
        // "Buy 0.09 @ 4416.957" đủ dài để chồng chữ lên nhãn bên trái nếu dùng chung 1 hàng trong card hẹp.
        Lbl ("TrimWorstDetailL", "Khối lượng / Giá", sideX + 8, yS, C'127,139,163', 10); yS += rowH;
        LblR("TrimWorstDetailV", worstDetailTxt, rightEdge, yS, hasWorst ? C'231,236,245' : C'127,139,163', 10); yS += rowH + 4;

        string trimBtnTxt = g_TrimEnabled ? "Trim: On" : "Trim: Off";
        color  trimBtnBg  = g_TrimEnabled ? C'10,70,35'   : C'45,18,18';
        color  trimBtnBd  = g_TrimEnabled ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnTrimToggle", trimBtnTxt, sideX + 8, yS, sideW - 16, 18, trimBtnBg, trimBtnBd);
    }
    y += trimStatsCardH + 8;

    CreateRect("TrailCard",    sideX, y, sideW, trailCardH, C'20,28,44');
    CreateRect("TrailCardBar", sideX, y, 2,     trailCardH, cardAccent);
    Lbl("TrailH", "TRAILING STOP", sideX + 8, y + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "TrailH", OBJPROP_FONT, "Calibri Bold");
    {
        int yT = y + 6 + 13 + hdrGap;
        int trailBtnW = (sideW - 16 - 6) / 2;

        string trailBtnTxt = g_TrailEnable ? "Trailing: On" : "Trailing: Off";
        color  trailBtnBg  = g_TrailEnable ? C'10,70,35'   : C'45,18,18';
        color  trailBtnBd  = g_TrailEnable ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnTrailToggle", trailBtnTxt, sideX + 8, yT, trailBtnW, 18, trailBtnBg, trailBtnBd);

        string pyraTrailBtnTxt = g_PyraTrailEnable ? "Pyra Trail: On" : "Pyra Trail: Off";
        color  pyraTrailBtnBg  = g_PyraTrailEnable ? C'10,70,35'   : C'45,18,18';
        color  pyraTrailBtnBd  = g_PyraTrailEnable ? C'55,200,110' : C'130,50,50';
        CreateBtn("BtnPyraTrailToggle", pyraTrailBtnTxt, sideX + 8 + trailBtnW + 6, yT, trailBtnW, 18, pyraTrailBtnBg, pyraTrailBtnBd);
        yT += 18 + 4;

        int   trailTotal    = CountPosForTrail(POSITION_TYPE_BUY) + CountPosForTrail(POSITION_TYPE_SELL);
        color trailCntClr   = (g_TrailEnable && trailTotal >= g_TrailMinOrds) ? clrLimeGreen : C'127,139,163';
        Lbl ("TrailCntL", "Số lệnh DCA", sideX + 8, yT, C'127,139,163', 10);
        LblR("TrailCntV", StringFormat("%d / %d", trailTotal, g_TrailMinOrds), rightEdge, yT, trailCntClr, 10);
        yT += rowH;

        int   pyraTrailTotal  = CountPyra(POSITION_TYPE_BUY) + CountPyra(POSITION_TYPE_SELL);
        color pyraTrailCntClr = (g_PyraTrailEnable && pyraTrailTotal >= g_PyraTrailMinOrds) ? clrLimeGreen : C'127,139,163';
        Lbl ("PyraTrailCntL", "Số lệnh Pyra", sideX + 8, yT, C'127,139,163', 10);
        LblR("PyraTrailCntV", StringFormat("%d / %d", pyraTrailTotal, g_PyraTrailMinOrds), rightEdge, yT, pyraTrailCntClr, 10);
    }
    y += trailCardH + 8;

    CreateRect("CloseCard",    sideX, y, sideW, closeCardH, C'20,28,44');
    CreateRect("CloseCardBar", sideX, y, 2,     closeCardH, cardAccent);
    Lbl("CloseH", "ĐÓNG LỆNH TỔNG", sideX + 8, y + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "CloseH", OBJPROP_FONT, "Calibri Bold");
    {
        int yC = y + 6 + 13 + hdrGap;

        Lbl ("CloseProfitL", "Chốt lời tổng", sideX + 8, yC, C'127,139,163', 10);
        LblR("CloseProfitV", g_CloseProfit > 0 ? StringFormat("$%.2f", g_CloseProfit) : "Tắt", rightEdge, yC, g_CloseProfit > 0 ? clrLimeGreen : C'127,139,163', 10);
        yC += rowH;

        Lbl ("CloseLossL", "Cắt lỗ tổng", sideX + 8, yC, C'127,139,163', 10);
        LblR("CloseLossV", g_CloseLoss > 0 ? StringFormat("$%.2f", g_CloseLoss) : "Tắt", rightEdge, yC, g_CloseLoss > 0 ? clrTomato : C'127,139,163', 10);
        yC += rowH;

        Lbl ("ClosePipsL", "Đóng theo pips", sideX + 8, yC, C'127,139,163', 10);
        LblR("ClosePipsV", g_ClosePerPips > 0 ? StringFormat("%.0f pts", g_ClosePerPips) : "Tắt", rightEdge, yC, g_ClosePerPips > 0 ? C'231,236,245' : C'127,139,163', 10);
        yC += rowH;

        bool   hasDayTgt   = (g_DayMaxProfit > 0 || g_DayMaxLoss > 0);
        string dayTgtTxt   = hasDayTgt ? StringFormat("+$%.0f / -$%.0f", g_DayMaxProfit, g_DayMaxLoss) : "Tắt";
        Lbl ("CloseDayTgtL", "Mục tiêu ngày", sideX + 8, yC, C'127,139,163', 10);
        LblR("CloseDayTgtV", dayTgtTxt, rightEdge, yC, hasDayTgt ? C'231,236,245' : C'127,139,163', 10);
        yC += rowH;

        string stTxt = DayLimitHit ? "ĐÃ DỪNG (Day Limit)" : "Bình thường";
        color  stClr = DayLimitHit ? clrTomato : clrLimeGreen;
        Lbl ("CloseStL", "Trạng thái ngày", sideX + 8, yC, C'127,139,163', 10);
        LblR("CloseStV", stTxt, rightEdge, yC, stClr, 10);
    }
    y += closeCardH + 8;

    CreateRect("DcaDirCard",    sideX, y, sideW, dcaDirCardH, C'20,28,44');
    CreateRect("DcaDirCardBar", sideX, y, 2,     dcaDirCardH, cardAccent);
    Lbl("DcaDirH", "CHIỀU & TẦNG DCA/PYRA", sideX + 8, y + 5, clrWhite, 9);
    ObjectSetString(0, GUI + "DcaDirH", OBJPROP_FONT, "Calibri Bold");
    {
        int yD = y + 6 + 13 + hdrGap;

        // "Doing" phải là số lệnh DCA ĐANG SỐNG THẬT (CountDCA(), đếm trực tiếp mỗi lần gọi) — KHÔNG dùng
        // PeakDCABuy/PeakDCASell: đó là mốc "đã từng nhồi tới tầng thứ mấy" (chỉ tăng, không giảm khi 1
        // tầng đóng vì TP/SL/Trim/tay), nên luôn ≥ số lệnh thật sự còn mở, gây hiển thị sai (VD 17 dù chỉ
        // còn vài lệnh đang chạy).
        int dcaBuyDoing  = CountDCA(POSITION_TYPE_BUY);
        int dcaSellDoing = CountDCA(POSITION_TYPE_SELL);
        int dcaBuyPend   = CountPendingDCA(POSITION_TYPE_BUY);
        int dcaSellPend  = CountPendingDCA(POSITION_TYPE_SELL);
        string dcaBuyTxt  = (dcaBuyPend  > 0) ? StringFormat("%d (+%d chờ)", dcaBuyDoing,  dcaBuyPend)  : IntegerToString(dcaBuyDoing);
        string dcaSellTxt = (dcaSellPend > 0) ? StringFormat("%d (+%d chờ)", dcaSellDoing, dcaSellPend) : IntegerToString(dcaSellDoing);
        Lbl ("DcaLvlBuyL",  "DCA Buy",  sideX + 8, yD, C'127,139,163', 10);
        LblR("DcaLvlBuyV",  dcaBuyTxt,  rightEdge, yD, dcaBuyDoing  > 0 ? clrLimeGreen : C'127,139,163', 10);
        yD += rowH;
        Lbl ("DcaLvlSellL", "DCA Sell", sideX + 8, yD, C'127,139,163', 10);
        LblR("DcaLvlSellV", dcaSellTxt, rightEdge, yD, dcaSellDoing > 0 ? clrTomato : C'127,139,163', 10);
        yD += rowH;

        int pyraBuyCount  = CountPyra(POSITION_TYPE_BUY);
        int pyraSellCount = CountPyra(POSITION_TYPE_SELL);
        int pyraBuyPend   = CountPendingPyra(POSITION_TYPE_BUY);
        int pyraSellPend  = CountPendingPyra(POSITION_TYPE_SELL);
        string pyraBuyTxt  = (pyraBuyPend  > 0) ? StringFormat("%d (+%d chờ)", pyraBuyCount,  pyraBuyPend)  : IntegerToString(pyraBuyCount);
        string pyraSellTxt = (pyraSellPend > 0) ? StringFormat("%d (+%d chờ)", pyraSellCount, pyraSellPend) : IntegerToString(pyraSellCount);
        Lbl ("PyraLvlBuyL",  "Pyra Buy",  sideX + 8, yD, C'127,139,163', 10);
        LblR("PyraLvlBuyV",  pyraBuyTxt,  rightEdge, yD, pyraBuyCount  > 0 ? clrLimeGreen : C'127,139,163', 10);
        yD += rowH;
        Lbl ("PyraLvlSellL", "Pyra Sell", sideX + 8, yD, C'127,139,163', 10);
        LblR("PyraLvlSellV", pyraSellTxt, rightEdge, yD, pyraSellCount > 0 ? clrTomato : C'127,139,163', 10);
        yD += rowH + 4;

        // Hiệu suất theo chiều (gồm cả lệnh gốc + DCA + Pyra) và theo cơ chế (chỉ DCA / chỉ Pyra) — tách
        // biệt khỏi 4 hàng đếm tầng phía trên bằng khoảng cách +4 vừa cộng.
        string buyTodayTxt  = StringFormat("%s$%.2f", DayProfitBuy  >= 0 ? "+" : "-", MathAbs(DayProfitBuy));
        string sellTodayTxt = StringFormat("%s$%.2f", DayProfitSell >= 0 ? "+" : "-", MathAbs(DayProfitSell));
        Lbl ("DcaBuyTodayL",  "Buy hôm nay",  sideX + 8, yD, C'127,139,163', 10);
        LblR("DcaBuyTodayV",  buyTodayTxt,  rightEdge, yD, DayProfitBuy  >= 0 ? clrLimeGreen : clrTomato, 10);
        yD += rowH;
        Lbl ("DcaSellTodayL", "Sell hôm nay", sideX + 8, yD, C'127,139,163', 10);
        LblR("DcaSellTodayV", sellTodayTxt, rightEdge, yD, DayProfitSell >= 0 ? clrLimeGreen : clrTomato, 10);
        yD += rowH;

        double floatDCA  = FloatDCA();
        double floatPyra = FloatPyra();
        string floatDCATxt  = StringFormat("%s$%.2f", floatDCA  >= 0 ? "+" : "-", MathAbs(floatDCA));
        string floatPyraTxt = StringFormat("%s$%.2f", floatPyra >= 0 ? "+" : "-", MathAbs(floatPyra));
        Lbl ("DcaFloatL",  "Float DCA (âm)", sideX + 8, yD, C'127,139,163', 10);
        LblR("DcaFloatV",  floatDCATxt,  rightEdge, yD, floatDCA  >= 0 ? clrLimeGreen : clrTomato, 10);
        yD += rowH;
        Lbl ("PyraFloatL", "Float Pyra (dương)", sideX + 8, yD, C'127,139,163', 10);
        LblR("PyraFloatV", floatPyraTxt, rightEdge, yD, floatPyra >= 0 ? clrLimeGreen : clrTomato, 10);
    }
    y += dcaDirCardH + 8;
}

void UpdateGUI(bool forceCalRefresh = false) {
    if(!InpShowPanel) { RemoveGUI(); return; }
    int PX = InpPanelX;
    int PY = InpPanelY;
    int PW = InpPanelWidth;
    int titleOff = RTB_TITLEBAR_H;

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

    string sigName = "Simulated";

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
                            "TR0BarFill","TR1BarFill","TR2BarFill","TR3BarFill",
                            "CardTrim","CardTrimBar","TrimH",
                            "TrimModeL","TrimMode","TrimManL","TrimManV",
                            "ManTrailBuyL","ManTrailBuyV","ManTrailSellL","ManTrailSellV",
                            "ManualTPV","ManTrailModeL","ManTrailModeV",
                            "ManualEMAL","ManualEMAV",
                            "ChipMode","ChipModeBg",
                            "DcaPanelBG","DcaPanelH",
                            "DcaParamCard","DcaParamCardBar","DcaParamH",
                            "DcaModeL","DcaModeV","DcaMultL","DcaMultV","DcaMaxL","DcaMaxV",
                            "DcaDistL","DcaDistV","DcaTPL","DcaTPV","DcaSLL","DcaSLV",
                            "CloseDMLossL","CloseDMLossV",
                            "CloseDMProfitL","CloseDMProfitV",
                            "CloseFloatL","CloseFloatV"};
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
    y2 += 22;

    int chH = 17;

    color dirBg = (g_Direction == DIR_BOTH) ? C'18,50,68' : (g_Direction == DIR_ONLY_BUY ? C'15,36,25' : C'36,18,20');
    color dirFg = (g_Direction == DIR_BOTH) ? C'111,217,238' : (g_Direction == DIR_ONLY_BUY ? C'98,214,150' : C'232,120,120');
    CreateChip("ChipSig", sigName, contentX, y2, bhw, chH, C'24,34,54', C'159,176,201');
    CreateChip("ChipDir", dirName, bx2, y2, bhw, chH, dirBg, dirFg);
    y2 += chH + 8;
    } else {
        ObjectDelete(0, GUI + "TimeRow");
        ObjectDelete(0, GUI + "ChipSig");  ObjectDelete(0, GUI + "ChipSigBg");
        ObjectDelete(0, GUI + "ChipDir");  ObjectDelete(0, GUI + "ChipDirBg");
    }

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

    color ddColor  = ddPct > 60          ? clrTomato : (ddPct > 20          ? clrOrangeRed : clrSilver);
    color mddColor = MaxDrawdownPct > 60 ? clrTomato : (MaxDrawdownPct > 20 ? clrOrangeRed : clrSilver);
    if(!g_PanelCollapsed) {
    {
        datetime nextNewsTime = 0;
        string   nextNewsCcy  = "";
        bool     hasNextNews  = InpNewsPauseEnabled && GetNextNewsToday(nextNewsTime, nextNewsCcy);

        string checkTxt = InpNewsPauseEnabled ? "Bật" : "Tắt";
        color  checkClr = InpNewsPauseEnabled ? clrLimeGreen : C'127,139,163';

        string nextTxt; color nextClr;
        if(!InpNewsPauseEnabled)  { nextTxt = "—"; nextClr = C'127,139,163'; }
        else if(hasNextNews)      { nextTxt = StringFormat("%s · còn %s", nextNewsCcy, FormatDuration((long)(nextNewsTime - TimeCurrent()))); nextClr = C'231,236,245'; }
        else                      { nextTxt = "Không có (hôm nay)"; nextClr = C'127,139,163'; }

        string stTxt; color stClr;
        if(!InpNewsPauseEnabled) { stTxt = "—";               stClr = C'127,139,163'; }
        else if(g_NewsBlock)     { stTxt = "ĐANG DỪNG DO TIN"; stClr = clrTomato; }
        else                     { stTxt = "Bình thường";     stClr = clrLimeGreen; }

        // "Tin tiếp theo" tách thành 2 dòng riêng (nhãn 1 dòng, giá trị 1 dòng) thay vì cùng hàng với
        // LblR() — giá trị dạng "Mã tiền · còn Xg Yp" hoặc "Không có (hôm nay)" đủ dài để chồng chữ lên
        // nhãn bên trái khi cùng 1 hàng trong card hẹp (bài học từ lỗi chồng chữ ở card Thống Kê).
        int newsH = 6 + 13 + 4*15 + 6;
        CreateRect("CardNews",    contentX, y2, cardW, newsH, C'20,28,44');
        CreateRect("CardNewsBar", contentX, y2, 2,     newsH, C'160,120,220');
        Lbl("NewsH", "TIN TỨC", contentX + 8, y2 + 5, clrWhite, 9);
        ObjectSetString(0, GUI + "NewsH", OBJPROP_FONT, "Calibri Bold");
        int yn = y2 + 6 + 13;
        Lbl ("NewsCheckL", "Check tin", contentX + 8, yn, C'127,139,163', 11);
        LblR("NewsCheckV", checkTxt, rightEdge, yn, checkClr, 11); yn += 15;
        Lbl ("NewsNextL", "Tin tiếp theo", contentX + 8, yn, C'127,139,163', 11); yn += 15;
        LblR("NewsNextV", nextTxt, rightEdge, yn, nextClr, 11); yn += 15;
        Lbl ("NewsStL", "Trạng thái", contentX + 8, yn, C'127,139,163', 11);
        LblR("NewsStV", stTxt, rightEdge, yn, stClr, 11);
        y2 += newsH + 8;
    }

    int riskH = 6 + 13 + (13+7+3)*2 + 6;
    CreateRect("CardRisk",    contentX, y2, cardW, riskH, C'20,28,44');
    CreateRect("CardRiskBar", contentX, y2, 2,     riskH, C'240,166,63');
    Lbl("RiskH", "RỦI RO (DRAWDOWN)", contentX + 8, y2 + 5, clrWhite, 9);
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

    Lbl ("TotL", "Total", contentX, y2, C'127,139,163', 10);
    LblR("TotV", StringFormat("%d orders", nBuy + nSell), rightEdge, y2, C'231,236,245', 10);
    y2 += 17;
    } else {
        string acctCollapsedObjs[] = {
            "CardNews", "CardNewsBar", "NewsH", "NewsCheckL", "NewsCheckV",
            "NewsNextL", "NewsNextV", "NewsStL", "NewsStV",
            "CardRisk", "CardRiskBar", "RiskH", "DDL", "DDV", "DDGTrk", "DDGFill",
            "MDDL", "MDDV", "MDDGTrk", "MDDGFill",
            "CardBuy", "CardBuyBar", "BuyL", "BuyV", "BuyLot",
            "CardSell", "CardSellBar", "SelL", "SelV", "SelLot",
            "TotL", "TotV"
        };
        for(int aci = 0; aci < ArraySize(acctCollapsedObjs); aci++) ObjectDelete(0, GUI + acctCollapsedObjs[aci]);
    }

    int contentBottom = y2 + 3;
    int bg2Y = contentBottom + 4;

    if(!g_PanelCollapsed) {
    int bg3Y = bg2Y + 66 + 2;

    int y = bg2Y + 5;
    Lbl("P2T", "ĐIỀU KHIỂN LỆNH", x + 8, y, clrWhite, 9);
    ObjectSetString(0, GUI + "P2T", OBJPROP_FONT, "Calibri Bold"); y += s + 2;

    {
        string botTxt = g_BotEnabled ? "  EA Running" : "  EA Off";
        color  botBg  = g_BotEnabled ? C'0,90,30'   : C'120,20,20';
        color  botBd  = g_BotEnabled ? C'40,190,90' : C'220,60,60';
        CreateBtn("BtnBotToggle", botTxt, PX+7, y, bfw, bh, botBg, botBd);
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

    // So khớp chiều cao với panel Trim/DCA bên phải — lấy đáy thấp hơn trong 2 panel rồi kéo lên
    // bằng đáy thấp hơn kia, để cả 2 luôn cùng chiều cao (panel phải tự kéo giãn card cuối qua extraH
    // trong UpdateTrimSidePanel(), dựa trên g_LastPanelBottom này).
    {
        int t1, t2, t3, t4;
        int sideNaturalBottom = (InpCalPanelY + RTB_TITLEBAR_H) + SidePanelCardH(t1, t2, t3, t4);
        if(sideNaturalBottom > g_LastPanelBottom) g_LastPanelBottom = sideNaturalBottom;
    }

    } else {
        string collapsedObjs[] = {
            "P2T", "BtnBotToggle",
            "P3T", "BtnCalToggle",
            "P4T", "BtnOpenBuy", "BtnOpenSell"
        };
        for(int ci = 0; ci < ArraySize(collapsedObjs); ci++) ObjectDelete(0, GUI + collapsedObjs[ci]);
        g_LastPanelBottom = contentBottom;
    }

    ObjectSetInteger(0, bg, OBJPROP_YSIZE, g_LastPanelBottom - (PY + titleOff));

    UpdateCalendarPanel(forceCalRefresh);
    UpdateTrimSidePanel();

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
    DCA_SL     = InpDCA1SL;
}

void InitPyra() {
    PYRA_Mult   = InpPyra1Mult;
    PYRA_MaxOrd = InpPyra1Max;
    PYRA_Dist   = InpPyra1Dist;
    PYRA_SL     = InpPyra1SL;
}

void ApplyBotEnabled(bool newVal) {
    if(g_BotEnabled && !newVal) {
        Print("RTB: Bot bị TẮT (BotEnabled=false) — đóng toàn bộ lệnh.");
        CloseAll();
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

    g_OrderDelay = InpOrderDelay;
    g_DCAArithEnable = InpDCAArithEnable; g_DCAArithStep = InpDCAArithStep;
    g_TrimTrigger = InpTrimTrigger;
    g_TrimTarget = InpTrimTarget; g_TrimMaxLoss = InpTrimMaxLoss; g_TrimMaxWin = InpTrimMaxWin;
    g_TrimMaxCycles = InpTrimMaxCycles;
    g_TrailEnable = InpTrailEnable; g_TrailMinOrds = InpTrailMinOrds;
    g_TrailActivate = InpTrailActivate; g_TrailStep = InpTrailStep; g_TrailInit = InpTrailInit;
    g_PyraTrailEnable = InpPyraTrailEnable; g_PyraTrailMinOrds = InpPyraTrailMinOrds;
    g_PyraTrailActivate = InpPyraTrailActivate; g_PyraTrailStep = InpPyraTrailStep; g_PyraTrailInit = InpPyraTrailInit;
    g_CloseProfit = InpCloseProfit; g_CloseLoss = InpCloseLoss; g_ClosePerPips = InpClosePerPips;
    g_DayMaxLoss = InpDayMaxLoss; g_DayMaxProfit = InpDayMaxProfit;

    g_BotEnabled = InpBotEnabled;

    Trade.SetExpertMagicNumber(InpMagic);
    Trade.SetDeviationInPoints(50);
    Trade.SetTypeFilling(ORDER_FILLING_RETURN);

    SetupChartColors();

    InitDCA();
    InitPyra();

    InitBalance    = AccountInfoDouble(ACCOUNT_BALANCE);
    MaxDrawdownPct = 0;
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
}

void OnTick() {
    bool newsBlock = false;
    if(InpNewsPauseEnabled) {
        UpdateNewsCache();
        newsBlock = IsInNewsZone();
    }
    g_NewsBlock = newsBlock;

    if(!newsBlock) CheckEntry();
    if(!DayLimitHit) {
        CheckTrailing();
        CheckPyraTrailing();
    }
}

void OnTimer() {
    UpdateDayProfit();
    CheckDayLimit();

    if(CountBuy()  == 0 && !HasPendingDCA(POSITION_TYPE_BUY))  ResetDCAState(POSITION_TYPE_BUY);
    if(CountSell() == 0 && !HasPendingDCA(POSITION_TYPE_SELL)) ResetDCAState(POSITION_TYPE_SELL);
    if(CountBuy()  == 0) ResetPyraPending(POSITION_TYPE_BUY);
    if(CountSell() == 0) ResetPyraPending(POSITION_TYPE_SELL);

    CheckExit();

    if(!DayLimitHit && g_BotEnabled) {
        CheckOrigRestart(POSITION_TYPE_BUY);
        CheckOrigRestart(POSITION_TYPE_SELL);

        CheckTrimming();

        if(CountBuy()  > 0) CheckDCA(POSITION_TYPE_BUY);
        if(CountSell() > 0) CheckDCA(POSITION_TYPE_SELL);
        if(CountBuy()  > 0) CheckPyramiding(POSITION_TYPE_BUY);
        if(CountSell() > 0) CheckPyramiding(POSITION_TYPE_SELL);
    }

    UpdateGUI();
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
    if(sparam == GUI + "BtnBotToggle") {
        if(TimeCurrent() - g_LastBotToggleClick < 2) {
            Print("RTB: Bỏ qua click Bot Toggle — bấm quá nhanh (debounce 2 giây).");
        } else {
            g_LastBotToggleClick = TimeCurrent();
            ApplyBotEnabled(!g_BotEnabled);
            UpdateGUI();
        }
    }
    else if(sparam == GUI + "BtnTrimToggle") {
        g_TrimEnabled = !g_TrimEnabled;
        Print("RTB: Tỉa lệnh ", (g_TrimEnabled ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnTrailToggle") {
        g_TrailEnable = !g_TrailEnable;
        Print("RTB: Trailing (gốc/DCA) ", (g_TrailEnable ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
    }
    else if(sparam == GUI + "BtnPyraTrailToggle") {
        g_PyraTrailEnable = !g_PyraTrailEnable;
        Print("RTB: Trailing nhồi dương ", (g_PyraTrailEnable ? "BẬT" : "TẮT"), " (thủ công qua panel).");
        UpdateGUI();
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
