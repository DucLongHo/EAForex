//+------------------------------------------------------------------+
//|                                        RichTradingBot.mq5        |
//|                        Rich Trading Bot v1.0 (MQL5)              |
//|                  Spec: Custumer/CLAUDE.md                        |
//+------------------------------------------------------------------+
#property copyright "Rich Trading Bot v1.0"
#property version   "1.00"
#property strict

#include <Trade\Trade.mqh>
#include <Trade\PositionInfo.mqh>

CTrade    Trade;

//+------------------------------------------------------------------+
//| ENUMS                                                            |
//+------------------------------------------------------------------+
enum ENUM_SIGNAL_MODE  { SIG_EMA, SIG_BZ_ZONE, SIG_ICHIMOKU, SIG_BB, SIG_SIMULATED, SIG_UT_BOT };
enum ENUM_DIRECTION    { DIR_BOTH, DIR_ONLY_BUY, DIR_ONLY_SELL };
enum ENUM_DCA_MODE     { DCA_STOP, DCA_STEP, DCA_STEP_TF };
enum ENUM_TRAIL_MODE   { TRAIL_BASKET, TRAIL_SINGLE };
enum ENUM_BOT_MODE     { MODE_AUTO, MODE_SEMI_AUTO };

//+------------------------------------------------------------------+
//| INPUT: BASE SETTINGS                                             |
//+------------------------------------------------------------------+
input group         "══════ CÀI ĐẶT CƠ BẢN ══════"; //
input  ENUM_BOT_MODE InpBotMode = MODE_AUTO;  // Chế độ: Tự động / Bán tự động
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
input  ENUM_SIGNAL_MODE InpSignalMode = SIG_EMA;      // Chiến lược tín hiệu
input  ENUM_DIRECTION   InpDirection  = DIR_BOTH;     // Hướng giao dịch
input  ENUM_TIMEFRAMES  InpSignalTF   = PERIOD_H1;    // Khung thời gian tín hiệu

//+------------------------------------------------------------------+
//| INPUT: EMA                                                       |
//+------------------------------------------------------------------+
input group         "══════ EMA FILTER (34+89) ══════"; //
input  int     InpEMAFast  = 34;   // EMA nhanh
input  int     InpEMASlow  = 89;   // EMA chậm
input  double  InpEMAPullbackPts = 100.0; // Khoảng pullback về EMA34 (points)

//+------------------------------------------------------------------+
//| INPUT: BOLLINGER BANDS                                           |
//+------------------------------------------------------------------+
input group          "══════ BOLLINGER BANDS ══════"; //
input  int     InpBBPeriod = 20;   // BB Period
input  double  InpBBDev    = 2.0;  // BB Deviation

//+------------------------------------------------------------------+
//| INPUT: ICHIMOKU                                                  |
//+------------------------------------------------------------------+
input group         "══════ ICHIMOKU ══════"; //
input  int     InpIchiTenkan = 9;   // Tenkan-sen
input  int     InpIchiKijun  = 26;  // Kijun-sen
input  int     InpIchiSenkou = 52;  // Senkou Span B

//+------------------------------------------------------------------+
//| INPUT: UT BOT                                                    |
//+------------------------------------------------------------------+
input group         "══════ UT BOT ══════"; //
input  int     InpUTKeyValue  = 1;    // Key Value (độ nhạy ATR)
input  int     InpUTATRPeriod = 10;   // ATR Period

//+------------------------------------------------------------------+
//| INPUT: GLOBAL FILTERS                                            |
//+------------------------------------------------------------------+
input group          "══════ BỘ LỌC CHUNG ══════"; //
input  int     InpMaxBuy   = 10;   // Số lệnh Buy tối đa
input  int     InpMaxSell  = 10;   // Số lệnh Sell tối đa

//+------------------------------------------------------------------+
//| INPUT: DCA (8 LEVELS)                                            |
//+------------------------------------------------------------------+
input group         "══════ DCA - CÀI ĐẶT CHUNG ══════"; //
input  ENUM_DCA_MODE InpDCAMode     = DCA_STEP; // DCA: Chế độ (áp dụng cho tất cả tầng)
input  bool          InpDCABuyEnable  = true;   // DCA: Bật DCA chiều Buy
input  bool          InpDCASellEnable = true;   // DCA: Bật DCA chiều Sell
input  bool          InpDCAArithEnable = false; // DCA: Bật Vol Cấp Số Cộng (bỏ qua Hệ số Lot từng tầng)
input  double        InpDCAArithStep   = 0.01;  // DCA: Cộng thêm Vol mỗi lệnh DCA sau (lots)

input group         "══════ DCA - TẦNG 1 ══════"; //
input  double  InpDCA1Mult = 1.5;    // DCA T1: Hệ số Lot
input  int     InpDCA1Max  = 2;      // DCA T1: Max lệnh tổng tại tầng này
input  double  InpDCA1Dist = 1000.0; // DCA T1: Khoảng cách (points)
input  double  InpDCA1TP   = 500.0;  // DCA T1: TP (points)
input  double  InpDCA1SL   = 0.0;    // DCA T1: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 2 ══════"; //
input  double  InpDCA2Mult = 2.0;   // DCA T2: Hệ số Lot
input  int     InpDCA2Max  = 2;     // DCA T2: Max lệnh tổng tại tầng này
input  double  InpDCA2Dist = 1500.0; // DCA T2: Khoảng cách (points)
input  double  InpDCA2TP   = 500.0; // DCA T2: TP (points)
input  double  InpDCA2SL   = 0.0;   // DCA T2: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 3 ══════"; //
input  double  InpDCA3Mult = 2.5;   // DCA T3: Hệ số Lot
input  int     InpDCA3Max  = 2;     // DCA T3: Max lệnh tổng tại tầng này
input  double  InpDCA3Dist = 2000.0;// DCA T3: Khoảng cách (points)
input  double  InpDCA3TP   = 500.0; // DCA T3: TP (points)
input  double  InpDCA3SL   = 0.0;   // DCA T3: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 4 ══════"; //
input  double  InpDCA4Mult = 3.0;   // DCA T4: Hệ số Lot
input  int     InpDCA4Max  = 2;     // DCA T4: Max lệnh tổng tại tầng này
input  double  InpDCA4Dist = 2500.0; // DCA T4: Khoảng cách (points)
input  double  InpDCA4TP   = 500.0; // DCA T4: TP (points)
input  double  InpDCA4SL   = 0.0;   // DCA T4: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 5 ══════"; //
input  double  InpDCA5Mult = 3.5;  // DCA T5: Hệ số Lot
input  int     InpDCA5Max  = 2;    // DCA T5: Max lệnh tổng tại tầng này
input  double  InpDCA5Dist = 3000.0; // DCA T5: Khoảng cách (points)
input  double  InpDCA5TP   = 500.0; // DCA T5: TP (points)
input  double  InpDCA5SL   = 0.0;   // DCA T5: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 6 ══════"; //
input  double  InpDCA6Mult = 4.0; // DCA T6: Hệ số Lot
input  int     InpDCA6Max  = 2;   // DCA T6: Max lệnh tổng tại tầng này
input  double  InpDCA6Dist = 3500.0; // DCA T6: Khoảng cách (points)
input  double  InpDCA6TP   = 500.0; // DCA T6: TP (points)
input  double  InpDCA6SL   = 0.0;  // DCA T6: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 7 ══════"; //
input  double  InpDCA7Mult = 5.0; // DCA T7: Hệ số Lot
input  int     InpDCA7Max  = 2;  // DCA T7: Max lệnh tổng tại tầng này
input  double  InpDCA7Dist = 4000.0; // DCA T7: Khoảng cách (points)
input  double  InpDCA7TP   = 500.0; // DCA T7: TP (points)
input  double  InpDCA7SL   = 0.0;  // DCA T7: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 8 ══════"; //
input  double  InpDCA8Mult = 6.0;    // DCA T8: Hệ số Lot
input  int     InpDCA8Max  = 1;      // DCA T8: Max lệnh tổng tại tầng này
input  double  InpDCA8Dist = 5000.0; // DCA T8: Khoảng cách (points)
input  double  InpDCA8TP   = 500.0;  // DCA T8: TP (points)
input  double  InpDCA8SL   = 0.0;    // DCA T8: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 9 ══════"; //
input  double  InpDCA9Mult = 7.0;    // DCA T9: Hệ số Lot
input  int     InpDCA9Max  = 1;      // DCA T9: Max lệnh tổng tại tầng này
input  double  InpDCA9Dist = 5500.0; // DCA T9: Khoảng cách (points)
input  double  InpDCA9TP   = 500.0;  // DCA T9: TP (points)
input  double  InpDCA9SL   = 0.0;    // DCA T9: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 10 ══════"; //
input  double  InpDCA10Mult = 8.0;    // DCA T10: Hệ số Lot
input  int     InpDCA10Max  = 1;      // DCA T10: Max lệnh tổng tại tầng này
input  double  InpDCA10Dist = 6000.0; // DCA T10: Khoảng cách (points)
input  double  InpDCA10TP   = 500.0;  // DCA T10: TP (points)
input  double  InpDCA10SL   = 0.0;    // DCA T10: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 11 ══════"; //
input  double  InpDCA11Mult = 9.0;    // DCA T11: Hệ số Lot
input  int     InpDCA11Max  = 1;      // DCA T11: Max lệnh tổng tại tầng này
input  double  InpDCA11Dist = 6500.0; // DCA T11: Khoảng cách (points)
input  double  InpDCA11TP   = 500.0;  // DCA T11: TP (points)
input  double  InpDCA11SL   = 0.0;    // DCA T11: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 12 ══════"; //
input  double  InpDCA12Mult = 10.0;   // DCA T12: Hệ số Lot
input  int     InpDCA12Max  = 1;      // DCA T12: Max lệnh tổng tại tầng này
input  double  InpDCA12Dist = 7000.0; // DCA T12: Khoảng cách (points)
input  double  InpDCA12TP   = 500.0;  // DCA T12: TP (points)
input  double  InpDCA12SL   = 0.0;    // DCA T12: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 13 ══════"; //
input  double  InpDCA13Mult = 11.0;   // DCA T13: Hệ số Lot
input  int     InpDCA13Max  = 1;      // DCA T13: Max lệnh tổng tại tầng này
input  double  InpDCA13Dist = 7500.0; // DCA T13: Khoảng cách (points)
input  double  InpDCA13TP   = 500.0;  // DCA T13: TP (points)
input  double  InpDCA13SL   = 0.0;    // DCA T13: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 14 ══════"; //
input  double  InpDCA14Mult = 12.0;   // DCA T14: Hệ số Lot
input  int     InpDCA14Max  = 1;      // DCA T14: Max lệnh tổng tại tầng này
input  double  InpDCA14Dist = 8000.0; // DCA T14: Khoảng cách (points)
input  double  InpDCA14TP   = 500.0;  // DCA T14: TP (points)
input  double  InpDCA14SL   = 0.0;    // DCA T14: SL (points, 0=tắt)

input group         "══════ DCA - TẦNG 15 ══════"; //
input  double  InpDCA15Mult = 13.0;   // DCA T15: Hệ số Lot
input  int     InpDCA15Max  = 1;      // DCA T15: Max lệnh tổng tại tầng này
input  double  InpDCA15Dist = 8500.0; // DCA T15: Khoảng cách (points)
input  double  InpDCA15TP   = 500.0;  // DCA T15: TP (points)
input  double  InpDCA15SL   = 0.0;    // DCA T15: SL (points, 0=tắt)

//+------------------------------------------------------------------+
//| INPUT: PYRAMIDING (NHỒI DƯƠNG)                                   |
//+------------------------------------------------------------------+
input group         "══════ NHỒI DƯƠNG (PYRA) ══════"; //
input  ENUM_DCA_MODE InpPyraMode      = DCA_STEP; // PYRA: Chế độ (áp dụng cho tất cả tầng)
input  bool          InpPyraBuyEnable  = true;    // PYRA: Bật nhồi chiều Buy
input  bool          InpPyraSellEnable = true;    // PYRA: Bật nhồi chiều Sell

input group         "══════ PYRA - TẦNG 1 ══════"; //
input  double  InpPyra1Mult = 1.0;    // PYRA T1: Hệ số Lot
input  int     InpPyra1Max  = 2;      // PYRA T1: Max lệnh tổng tại tầng này
input  double  InpPyra1Dist = 500.0;  // PYRA T1: Khoảng cách (points)
input  double  InpPyra1TP   = 3000.0; // PYRA T1: TP (points)
input  double  InpPyra1SL   = 0.0;    // PYRA T1: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 2 ══════"; //
input  double  InpPyra2Mult = 1.0;    // PYRA T2: Hệ số Lot
input  int     InpPyra2Max  = 2;      // PYRA T2: Max lệnh tổng tại tầng này
input  double  InpPyra2Dist = 500.0;  // PYRA T2: Khoảng cách (points)
input  double  InpPyra2TP   = 3000.0; // PYRA T2: TP (points)
input  double  InpPyra2SL   = 0.0;    // PYRA T2: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 3 ══════"; //
input  double  InpPyra3Mult = 1.0;    // PYRA T3: Hệ số Lot
input  int     InpPyra3Max  = 2;      // PYRA T3: Max lệnh tổng tại tầng này
input  double  InpPyra3Dist = 500.0;  // PYRA T3: Khoảng cách (points)
input  double  InpPyra3TP   = 3000.0; // PYRA T3: TP (points)
input  double  InpPyra3SL   = 0.0;    // PYRA T3: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 4 ══════"; //
input  double  InpPyra4Mult = 1.0;    // PYRA T4: Hệ số Lot
input  int     InpPyra4Max  = 2;      // PYRA T4: Max lệnh tổng tại tầng này
input  double  InpPyra4Dist = 500.0;  // PYRA T4: Khoảng cách (points)
input  double  InpPyra4TP   = 3000.0; // PYRA T4: TP (points)
input  double  InpPyra4SL   = 0.0;    // PYRA T4: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 5 ══════"; //
input  double  InpPyra5Mult = 1.0;    // PYRA T5: Hệ số Lot
input  int     InpPyra5Max  = 2;      // PYRA T5: Max lệnh tổng tại tầng này
input  double  InpPyra5Dist = 500.0;  // PYRA T5: Khoảng cách (points)
input  double  InpPyra5TP   = 3000.0; // PYRA T5: TP (points)
input  double  InpPyra5SL   = 0.0;    // PYRA T5: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 6 ══════"; //
input  double  InpPyra6Mult = 1.0;    // PYRA T6: Hệ số Lot
input  int     InpPyra6Max  = 2;      // PYRA T6: Max lệnh tổng tại tầng này
input  double  InpPyra6Dist = 500.0;  // PYRA T6: Khoảng cách (points)
input  double  InpPyra6TP   = 3000.0; // PYRA T6: TP (points)
input  double  InpPyra6SL   = 0.0;    // PYRA T6: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 7 ══════"; //
input  double  InpPyra7Mult = 1.0;    // PYRA T7: Hệ số Lot
input  int     InpPyra7Max  = 2;      // PYRA T7: Max lệnh tổng tại tầng này
input  double  InpPyra7Dist = 500.0;  // PYRA T7: Khoảng cách (points)
input  double  InpPyra7TP   = 3000.0; // PYRA T7: TP (points)
input  double  InpPyra7SL   = 0.0;    // PYRA T7: SL (points, 0=tắt)

input group         "══════ PYRA - TẦNG 8 ══════"; //
input  double  InpPyra8Mult = 1.0;    // PYRA T8: Hệ số Lot
input  int     InpPyra8Max  = 1;      // PYRA T8: Max lệnh tổng tại tầng này
input  double  InpPyra8Dist = 500.0;  // PYRA T8: Khoảng cách (points)
input  double  InpPyra8TP   = 3000.0; // PYRA T8: TP (points)
input  double  InpPyra8SL   = 0.0;    // PYRA T8: SL (points, 0=tắt)

//+------------------------------------------------------------------+
//| INPUT: ORDER TRIMMING                                            |
//+------------------------------------------------------------------+
input group         "══════ TỈA LỆNH (TRIMMING) ══════"; //
input  bool    InpTrimEnable     = false;  // Bật Tỉa Lệnh
input  bool    InpTrimHedge      = false;  // Tỉa chéo (Hedging mode)
input  int     InpTrimTrigger    = 5;      // Kích hoạt khi số lệnh >= X
input  double  InpTrimTarget     = 10.0;   // Mục tiêu lợi nhuận sau tỉa ($)
input  int     InpTrimMaxLoss    = 1;      // Số lệnh âm tối đa cần tỉa mỗi lần
input  int     InpTrimMaxWin     = 1;      // Số lệnh dương tối đa dùng để tỉa (Hedge)
input  bool    InpPartialTrim    = false;  // Bật Tỉa Một Phần
input  double  InpPartialTrimDD  = 20.0;   // Kích hoạt khi DD% >
input  bool    InpTrimByDayProfit= false;  // Tỉa theo Lãi Ngày

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
//| INPUT: HEDGE FOLLOW WINNER                                       |
//+------------------------------------------------------------------+
input group         "══════ HEDGE FOLLOW WINNER ══════"; //
input  bool    InpHedgeEnable  = false;   // Bật chế độ Hedge Follow Winner
input  double  InpHedgeCutPts  = 3000.0;  // Cắt chiều âm sau X points từ lệnh gốc

//+------------------------------------------------------------------+
//| INPUT: PANEL                                                     |
//+------------------------------------------------------------------+
input group         "══════ PANEL ══════"; //
input  bool    InpShowPanel  = true;  // Hiện panel
input  int     InpPanelX     = 5;     // Panel: tọa độ X
input  int     InpPanelY     = 18;    // Panel: tọa độ Y
input  int     InpPanelWidth = 252;   // Panel: chiều rộng

//+------------------------------------------------------------------+
//| GLOBAL STATE                                                     |
//+------------------------------------------------------------------+
int      hEMAFast   = INVALID_HANDLE;
int      hEMASlow   = INVALID_HANDLE;
int      hBB        = INVALID_HANDLE;
int      hIchi      = INVALID_HANDLE;
int      hATR       = INVALID_HANDLE;
double   g_ats_ut        = 0.0;
int      g_ats_ut_signal = 0;
datetime g_last_bar_ut   = 0;

// DCA config arrays (index 0-7 = level 1-8)
ENUM_DCA_MODE DCA_Mode[15];
double        DCA_Mult[15];
int           DCA_MaxOrd[15];
double        DCA_Dist[15];
double        DCA_TP[15];
double        DCA_SL[15];

// Pyramiding config arrays (index 0-7 = level 1-8)
ENUM_DCA_MODE PYRA_Mode[8];
double        PYRA_Mult[8];
int           PYRA_MaxOrd[8];
double        PYRA_Dist[8];
double        PYRA_TP[8];
double        PYRA_SL[8];

datetime LastOrderTime  = 0;
datetime LastEntryTime  = 0;
double   InitBalance    = 0.0;
double   MaxDrawdownPct = 0.0;
double   DayProfit      = 0.0;
int      LastDay        = -1;
bool     DayLimitHit    = false;

// Thay YOUR_SCRIPT_ID bằng ID thực từ Google Apps Script deploy URL
// Không dùng input để URL không hiển thị trong cài đặt EA
const string LICENSE_URL = "https://script.google.com/macros/s/AKfycbwfKLYX36os92cQqn53XZ1ZKK0BOspHv17C0SeLPPozoF9v0gXIpYTPYOgphkxx1-9kRg/exec";

// Basket trail levels
double   TrailBuy  = 0.0;
double   TrailSell = 0.0;

// Persistent DCA tier counters — only reset when all positions of that side close
int      PeakDCABuy  = 0;
int      PeakDCASell = 0;

// Per-slot DCA price tracking: re-fill a closed slot only when price bounces back to its entry
double   DCABuyPrices[60];    // 15 tiers × max 4 orders each = 60 slots max
double   DCASellPrices[60];
bool     DCABuyBounced[60];   // true after price rose above entry since slot closed (BUY)
bool     DCASellBounced[60];  // true after price fell below entry since slot closed (SELL)
ulong    DCABuyTickets[60];   // position ticket of current open BUY DCA for each slot (0 = empty)
ulong    DCASellTickets[60];  // position ticket of current open SELL DCA for each slot (0 = empty)
ulong    DCABuyLimitTk[60];   // ticket of pending Buy Limit re-fill order for each slot (0 = none)
ulong    DCASellLimitTk[60];  // ticket of pending Sell Limit re-fill order for each slot (0 = none)

// Hedge Follow Winner state
bool   HedgeCutBuy        = false;
bool   HedgeCutSell       = false;
double HedgeInitBuyPrice  = 0.0;
double HedgeInitSellPrice = 0.0;
int    HedgeTrendSide     = -1;   // -1=chưa xác định, 0=BUY(xu hướng), 1=SELL(xu hướng)

// GUI prefix
const string GUI = "RTB_";

//+------------------------------------------------------------------+
//| UTILITY FUNCTIONS                                                |
//+------------------------------------------------------------------+

// Returns true if the currently-selected position should be managed.
// In Semi-Auto mode: also includes manually opened positions (magic == 0).
bool IsManaged() {
    if(PositionGetString(POSITION_SYMBOL) != _Symbol) return false;
    long magic = PositionGetInteger(POSITION_MAGIC);
    if(magic == (long)InpMagic) return true;
    if(InpBotMode == MODE_SEMI_AUTO && magic == 0) return true;
    return false;
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

// Đếm lệnh thủ công (magic=0) cùng chiều trên symbol này
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

// Đếm pyramiding orders theo comment prefix "RTP|" — restart-safe, không lẫn với DCA
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

// Last opened price for a direction (most recently opened position)
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

// Earliest opened price for a direction (lệnh mở cũ nhất — dùng làm giá tham chiếu Hedge)
double FirstOpenPrice(int posType) {
    double   price  = 0;
    datetime oldest = (datetime)0x7FFFFFFF;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t < oldest) { oldest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    return price;
}

// Weighted average open price
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

// Ticket of position with worst (most negative) floating profit
ulong WorstTicket() {
    ulong  tk_worst = 0;
    double worst    = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        double p = PositionGetDouble(POSITION_PROFIT);
        if(p < worst) { worst = p; tk_worst = tk; }
    }
    return tk_worst;
}

// Best (most positive) ticket
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
}

void CloseAllProfit() {
    ulong tickets[];
    int   count = 0;
    ArrayResize(tickets, PositionsTotal());
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if(PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP) > 0)
            tickets[count++] = tk;
    }
    Trade.SetAsyncMode(true);
    for(int i = 0; i < count; i++)
        Trade.PositionClose(tickets[i]);
    Trade.SetAsyncMode(false);
}

void CloseAllLoss() {
    ulong tickets[];
    int   count = 0;
    ArrayResize(tickets, PositionsTotal());
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
        if(PositionGetDouble(POSITION_PROFIT) + PositionGetDouble(POSITION_SWAP) < 0)
            tickets[count++] = tk;
    }
    Trade.SetAsyncMode(true);
    for(int i = 0; i < count; i++)
        Trade.PositionClose(tickets[i]);
    Trade.SetAsyncMode(false);
}

double NormLot(double lot) {
    double minL  = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MIN);
    double maxL  = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_MAX);
    double stepL = SymbolInfoDouble(_Symbol, SYMBOL_VOLUME_STEP);
    lot = MathRound(lot / stepL) * stepL;
    return MathMax(minL, MathMin(maxL, lot));
}

// Lot của lệnh DCA thứ orderIdx1 (1-based: 1=lệnh DCA đầu tiên).
// InpDCAArithEnable=true  → baseLot + orderIdx1 * InpDCAArithStep (bỏ qua hệ số tầng)
// InpDCAArithEnable=false → baseLot * DCA_Mult[lvl] (hệ số tầng như cũ)
double DCAOrderLot(double baseLot, int orderIdx1, int lvl) {
    if(InpDCAArithEnable)
        return NormLot(baseLot + orderIdx1 * InpDCAArithStep);
    return NormLot(baseLot * DCA_Mult[lvl]);
}

//+------------------------------------------------------------------+
//| OPEN ORDER                                                       |
//| isDCA=false, isPyra=false : theo InpUseTakeProfit/SL, comment "RTB|0|0" |
//| isDCA=true  : server TP/SL nếu > 0; comment "RTB|tp|sl"        |
//|               nếu tp=sl=0 → comment "RTB|0|0|D" (phân biệt gốc)|
//| isPyra=true : comment "RTP|tp|sl"; nếu tp=sl=0 → "RTP|0|0|P"  |
//|               fallback về InpTP/SL_Points; section 2a xử lý exit |
//+------------------------------------------------------------------+
bool OpenOrder(int ordType, double lot, double tp_pts = 0, double sl_pts = 0,
               bool isDCA = false, bool isPyra = false) {
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    double price, tp = 0, sl = 0;

    bool autoExit = (isDCA || isPyra);
    bool applyTP  = autoExit ? (tp_pts > 0) : (InpUseTakeProfit && tp_pts > 0);
    bool applySL  = autoExit ? (sl_pts > 0) : (InpUseStopLoss   && sl_pts > 0);

    if(ordType == ORDER_TYPE_BUY) {
        price = ask;
        if(applyTP && !InpStealthMode)
            tp = NormalizeDouble(price + tp_pts * point, _Digits);
        if(applySL && !InpStealthMode)
            sl = NormalizeDouble(price - sl_pts * point, _Digits);
    } else {
        price = bid;
        if(applyTP && !InpStealthMode)
            tp = NormalizeDouble(price - tp_pts * point, _Digits);
        if(applySL && !InpStealthMode)
            sl = NormalizeDouble(price + sl_pts * point, _Digits);
    }

    string comment;
    if(isPyra) {
        if(tp_pts == 0 && sl_pts == 0)
            comment = "RTP|0|0|P";  // Pyramiding không có tier TP/SL — section 2b dùng InpTP_Points
        else
            comment = StringFormat("RTP|%.0f|%.0f", tp_pts, sl_pts);
    } else if(isDCA) {
        if(tp_pts == 0 && sl_pts == 0)
            comment = "RTB|0|0|D";  // DCA không có TP/SL — phân biệt với lệnh gốc "RTB|0|0"
        else
            comment = StringFormat("RTB|%.0f|%.0f", tp_pts, sl_pts);
    }
    else                comment = "RTB|0|0";

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

//+------------------------------------------------------------------+
//| ENTRY SIGNALS                                                    |
//+------------------------------------------------------------------+

// Returns +1 = BUY, -1 = SELL, 0 = no signal
int SignalEMA() {
    double fast[], slow[];
    ArraySetAsSeries(fast, true);
    ArraySetAsSeries(slow, true);
    if(CopyBuffer(hEMAFast, 0, 0, 3, fast) < 3) return 0;
    if(CopyBuffer(hEMASlow, 0, 0, 3, slow) < 3) return 0;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double price = (double)iClose(_Symbol, InpSignalTF, 0);

    // Golden / Death cross
    bool crossUp   = fast[2] < slow[2] && fast[1] > slow[1];
    bool crossDown = fast[2] > slow[2] && fast[1] < slow[1];

    // Pullback to EMA34: price within InpEMAPullbackPts of EMA34
    bool trendUp   = fast[0] > slow[0];
    bool trendDown = fast[0] < slow[0];
    bool pullBuy   = trendUp   && MathAbs(price - fast[0]) <= InpEMAPullbackPts * point;
    bool pullSell  = trendDown && MathAbs(price - fast[0]) <= InpEMAPullbackPts * point;

    if(crossUp  || pullBuy)  return  1;
    if(crossDown || pullSell) return -1;
    return 0;
}

int SignalBZZone() {
    MqlRates r[];
    ArraySetAsSeries(r, true);
    // Read 3 closed candles (index 1-3)
    if(CopyRates(_Symbol, InpSignalTF, 1, 3, r) < 3) return 0;

    bool allGreen = r[0].close > r[0].open && r[1].close > r[1].open && r[2].close > r[2].open;
    bool allRed   = r[0].close < r[0].open && r[1].close < r[1].open && r[2].close < r[2].open;

    if(allGreen) return  1;
    if(allRed)   return -1;
    return 0; // Gray = no signal
}

int SignalIchimoku() {
    double tenkan[], kijun[], spanA[], spanB[], chikou[];
    ArraySetAsSeries(tenkan,  true);
    ArraySetAsSeries(kijun,   true);
    ArraySetAsSeries(spanA,   true);
    ArraySetAsSeries(spanB,   true);
    ArraySetAsSeries(chikou,  true);

    if(CopyBuffer(hIchi, 0, 0, 3, tenkan)  < 3) return 0;
    if(CopyBuffer(hIchi, 1, 0, 3, kijun)   < 3) return 0;
    if(CopyBuffer(hIchi, 2, 0, 3, spanA)   < 3) return 0;
    if(CopyBuffer(hIchi, 3, 0, 3, spanB)   < 3) return 0;
    if(CopyBuffer(hIchi, 4, 0, 3, chikou)  < 3) return 0;

    double price    = (double)iClose(_Symbol, InpSignalTF, 0);
    double pastPrice= (double)iClose(_Symbol, InpSignalTF, InpIchiKijun); // Chikou lag
    double kumoTop  = MathMax(spanA[0], spanB[0]);
    double kumoBot  = MathMin(spanA[0], spanB[0]);

    // Tenkan cross Kijun (golden = buy, death = sell) on bar[2] vs [1]
    bool tkCrossUp  = tenkan[2] < kijun[2] && tenkan[1] > kijun[1];
    bool tkCrossDown= tenkan[2] > kijun[2] && tenkan[1] < kijun[1];

    bool buyOK  = price > kumoTop  && tkCrossUp   && chikou[0] > pastPrice;
    bool sellOK = price < kumoBot  && tkCrossDown  && chikou[0] < pastPrice;

    if(buyOK)  return  1;
    if(sellOK) return -1;
    return 0;
}

int SignalBB() {
    double upper[], lower[];
    ArraySetAsSeries(upper, true);
    ArraySetAsSeries(lower, true);
    if(CopyBuffer(hBB, 1, 0, 2, upper) < 2) return 0;
    if(CopyBuffer(hBB, 2, 0, 2, lower) < 2) return 0;

    double closeBar1 = (double)iClose(_Symbol, InpSignalTF, 1);

    if(closeBar1 <= lower[1]) return  1;  // Touch/breach lower band → BUY
    if(closeBar1 >= upper[1]) return -1;  // Touch/breach upper band → SELL
    return 0;
}

void WarmupATS(int bars) {
    int need = bars + 5;
    double atr[], cls[];
    ArraySetAsSeries(atr, true);
    ArraySetAsSeries(cls, true);
    if(CopyBuffer(hATR, 0, 0, need, atr) < need ||
       CopyClose(_Symbol, InpSignalTF, 0, need, cls) < need) return;
    double ats = 0.0;
    for(int i = bars; i >= 1; i--) {
        double s=cls[i], sp=cls[i+1], nl=InpUTKeyValue*atr[i];
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
    double nLoss    = InpUTKeyValue * atr[0];
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
    if(InpDirection == DIR_ONLY_BUY)  return  1;
    if(InpDirection == DIR_ONLY_SELL) return -1;
    return 2; // Both/Either: signal = 2 → mở cả BUY lẫn SELL
}

int GetSignal() {
    int sig = 0;
    switch(InpSignalMode) {
        case SIG_EMA:       sig = SignalEMA();       break;
        case SIG_BZ_ZONE:   sig = SignalBZZone();    break;
        case SIG_ICHIMOKU:  sig = SignalIchimoku();  break;
        case SIG_BB:        sig = SignalBB();        break;
        case SIG_SIMULATED: sig = SignalSimulated(); break;
        case SIG_UT_BOT:    sig = SignalUTBot();    break;
    }
    // SIG_SIMULATED đã tự filter theo InpDirection — chỉ apply cho các strategy khác
    if(InpSignalMode != SIG_SIMULATED) {
        if(InpDirection == DIR_ONLY_BUY  && sig < 0) return 0;
        if(InpDirection == DIR_ONLY_SELL && sig > 0) return 0;
    }
    return sig;
}

//+------------------------------------------------------------------+
//| INITIAL ENTRY (OnTick)                                           |
//+------------------------------------------------------------------+
void ResetDCAState(int posType) {
    if(posType == POSITION_TYPE_BUY) {
        for(int i = 0; i < 60; i++) { if(DCABuyLimitTk[i] > 0) Trade.OrderDelete(DCABuyLimitTk[i]); }
        TrailBuy = 0; PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
    } else {
        for(int i = 0; i < 60; i++) { if(DCASellLimitTk[i] > 0) Trade.OrderDelete(DCASellLimitTk[i]); }
        TrailSell = 0; PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
    }
}

void TryOpenBuy() {
    if(InpHedgeEnable && HedgeCutBuy) return;
    if(CountBuy() >= InpMaxBuy) return;
    if(CountBuy() > 0) return;
    ResetDCAState(POSITION_TYPE_BUY);
    if(OpenOrder(ORDER_TYPE_BUY, InpLotSize, InpTP_Points, InpSL_Points))
        LastEntryTime = TimeCurrent();
}

void TryOpenSell() {
    if(InpHedgeEnable && HedgeCutSell) return;
    if(CountSell() >= InpMaxSell) return;
    if(CountSell() > 0) return;
    ResetDCAState(POSITION_TYPE_SELL);
    if(OpenOrder(ORDER_TYPE_SELL, InpLotSize, InpTP_Points, InpSL_Points))
        LastEntryTime = TimeCurrent();
}

void CheckEntry() {
    if(DayLimitHit) return;
    if(InpBotMode == MODE_SEMI_AUTO) return;
    if(TimeCurrent() - LastEntryTime < InpOrderDelay) return;

    int sig = GetSignal();
    if(sig == 0) return;

    if(sig == 2) {
        // Simulated Both/Either: mở BUY và SELL độc lập
        // Mỗi hướng tự quản lý DCA/Trail/Trim riêng
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

// Đếm lệnh DCA do bot mở (magic=InpMagic, comment "RTB|X|Y")
// Semi-Auto: lệnh gốc là thủ công (magic=0) nên MỌI lệnh RTB| đều là DCA
// Auto:      lệnh gốc cũng có "RTB|0|0" → đếm tất cả RTB| rồi trừ 1 (lệnh gốc)
//            Không dùng filter TP/SL vì DCA có thể có TP=SL=0 → comment cũng là "RTB|0|0"
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
    // Auto mode: trừ 1 cho lệnh gốc (cũng có prefix "RTB|")
    if(InpBotMode != MODE_SEMI_AUTO) n = MathMax(0, n - 1);
    return n;
}

// Giá mở của lệnh thủ công CŨ NHẤT (lệnh gốc của user) cùng chiều
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

// Lot của lệnh thủ công CŨ NHẤT (lệnh gốc của user) cùng chiều
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

// Giá cực trị trong các lệnh thủ công: BUY → thấp nhất, SELL → cao nhất
double ExtremeManualPrice(int posType) {
    double extreme = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        double price = PositionGetDouble(POSITION_PRICE_OPEN);
        if(posType == POSITION_TYPE_BUY)
            extreme = (extreme == 0 || price < extreme) ? price : extreme;
        else
            extreme = (extreme == 0 || price > extreme) ? price : extreme;
    }
    return extreme;
}

// Giá của lệnh pyramiding (RTP|) gần nhất do bot mở cùng chiều
double LastPyraPrice(int posType) {
    double   price  = 0;
    datetime latest = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") != 0) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t > latest) { latest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    return price;
}

// Lot của lệnh pyramiding (RTP|) gần nhất cùng chiều
double LastPyraLot(int posType) {
    double   lot    = 0;
    datetime latest = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(StringFind(PositionGetString(POSITION_COMMENT), "RTP|") != 0) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t > latest) { latest = t; lot = PositionGetDouble(POSITION_VOLUME); }
    }
    return lot;
}

// Lot của lệnh thủ công có giá cực trị: BUY → thấp nhất, SELL → cao nhất
double ExtremeManualLot(int posType) {
    double extreme = 0, lot = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if(PositionGetInteger(POSITION_MAGIC) != 0) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        double price = PositionGetDouble(POSITION_PRICE_OPEN);
        if(posType == POSITION_TYPE_BUY) {
            if(extreme == 0 || price < extreme) { extreme = price; lot = PositionGetDouble(POSITION_VOLUME); }
        } else {
            if(extreme == 0 || price > extreme) { extreme = price; lot = PositionGetDouble(POSITION_VOLUME); }
        }
    }
    return lot;
}

// Điểm tham chiếu DCA: lệnh DCA bot gần nhất → fallback lệnh thủ công cũ nhất
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
        if(InpBotMode != MODE_SEMI_AUTO) {
            string parts[];
            if(StringSplit(cmt, '|', parts) < 3) continue;
            if(StringToDouble(parts[1]) == 0 && StringToDouble(parts[2]) == 0) continue;
        }
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t > latest) { latest = t; price = PositionGetDouble(POSITION_PRICE_OPEN); }
    }
    if(price > 0) return price;
    return OldestManualPrice(posType);
}

//+------------------------------------------------------------------+
//| HEDGE FOLLOW WINNER — CẮT CHIỀU ÂM                              |
//+------------------------------------------------------------------+
void CheckHedgeCut() {
    if(!InpHedgeEnable) return;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

    int cntBuy  = CountBuy();
    int cntSell = CountSell();

    // Reset hoàn toàn khi không còn lệnh nào (vòng mới)
    if(cntBuy == 0 && cntSell == 0) {
        if(HedgeCutBuy || HedgeCutSell || HedgeTrendSide >= 0) {
            Print("RTB: Hedge — reset state (không còn lệnh)");
            HedgeCutBuy = false; HedgeCutSell = false;
            HedgeTrendSide = -1;
        }
        HedgeInitBuyPrice = 0.0; HedgeInitSellPrice = 0.0;
        return;
    }

    // Luôn cập nhật theo lệnh cũ nhất còn sống — đúng khi lệnh gốc bị SL và pyramided còn mở
    if(cntBuy  > 0) HedgeInitBuyPrice  = FirstOpenPrice(POSITION_TYPE_BUY);
    if(cntSell > 0) HedgeInitSellPrice = FirstOpenPrice(POSITION_TYPE_SELL);

    // Phát hiện đóng bên ngoài (trailing stop / TP / đóng tay)
    // → Chỉ block vào lại, KHÔNG CloseAll() chiều còn lại
    if(!HedgeCutBuy && cntBuy == 0 && HedgeInitBuyPrice > 0) {
        Print("RTB: Hedge — BUY đóng bên ngoài (trailing/TP) → block vào lại BUY");
        HedgeCutBuy = true;  HedgeInitBuyPrice = 0.0;
        return;
    }
    if(!HedgeCutSell && cntSell == 0 && HedgeInitSellPrice > 0) {
        Print("RTB: Hedge — SELL đóng bên ngoài (trailing/TP) → block vào lại SELL");
        HedgeCutSell = true;  HedgeInitSellPrice = 0.0;
        return;
    }

    // Kiểm tra cắt chiều BUY (giá giảm quá X points từ lệnh gốc)
    if(!HedgeCutBuy && HedgeInitBuyPrice > 0 && cntBuy > 0) {
        if((HedgeInitBuyPrice - bid) >= InpHedgeCutPts * point) {
            Print("RTB: Hedge — CẮT tất cả | BUY initPrice=", HedgeInitBuyPrice,
                  " bid=", bid, " loss=", (HedgeInitBuyPrice - bid) / point, "pts");
            CloseAll();  // Cắt toàn bộ BUY + SELL
            HedgeCutBuy  = true;  HedgeInitBuyPrice  = 0.0;
            HedgeCutSell = true;  HedgeInitSellPrice = 0.0;
        }
    }

    // Kiểm tra cắt chiều SELL (giá tăng quá X points từ lệnh gốc)
    if(!HedgeCutSell && HedgeInitSellPrice > 0 && cntSell > 0) {
        if((ask - HedgeInitSellPrice) >= InpHedgeCutPts * point) {
            Print("RTB: Hedge — CẮT tất cả | SELL initPrice=", HedgeInitSellPrice,
                  " ask=", ask, " loss=", (ask - HedgeInitSellPrice) / point, "pts");
            CloseAll();  // Cắt toàn bộ BUY + SELL
            HedgeCutBuy  = true;  HedgeInitBuyPrice  = 0.0;
            HedgeCutSell = true;  HedgeInitSellPrice = 0.0;
        }
    }
}

//+------------------------------------------------------------------+
//| DCA LOGIC                                                        |
//+------------------------------------------------------------------+

// Checks if a DCA slot's position is still open, using ticket (exact) first,
// falling back to price proximity if ticket is unknown (e.g. after EA restart).
bool IsSlotOpen(int posType, int slot) {
    ulong tk = (posType == POSITION_TYPE_BUY) ? DCABuyTickets[slot] : DCASellTickets[slot];
    if(tk > 0) return PositionSelectByTicket(tk);
    // Fallback: price-based with tight tolerance for post-restart recovery
    double slotPrice = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot] : DCASellPrices[slot];
    if(slotPrice == 0) return false;
    double tol = 50.0 * SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    for(int i = 0; i < PositionsTotal(); i++) {
        ulong ptk = PositionGetTicket(i);
        if(!PositionSelectByTicket(ptk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        if(MathAbs(PositionGetDouble(POSITION_PRICE_OPEN) - slotPrice) <= tol) return true;
    }
    return false;
}


void CheckDCA(int posType) {
    if(InpHedgeEnable) return;
    if(posType == POSITION_TYPE_BUY  && !InpDCABuyEnable)  return;
    if(posType == POSITION_TYPE_SELL && !InpDCASellEnable) return;

    int count = CountPos(posType);
    if(count == 0) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    int maxOrds  = (posType == POSITION_TYPE_BUY) ? InpMaxBuy : InpMaxSell;

    // ── SEMI-AUTO (multi-manual): logic cũ, không dùng per-slot tracking ──
    bool multiManual = (InpBotMode == MODE_SEMI_AUTO && CountManual(posType) >= 1);
    if(multiManual) {
        int    dcaCount  = CountBotDCA(posType);
        double lastPrice = LastPrimaryPrice(posType);
        if(lastPrice == 0) return;

        int lvl = -1, cum = 0;
        for(int i = 0; i < 15; i++) {
            int nc = cum + DCA_MaxOrd[i];
            if(dcaCount < nc) { lvl = i; break; }
            cum = nc;
        }
        if(lvl < 0 || DCA_Mode[lvl] == DCA_STOP) return;
        if(count >= maxOrds) return;

        double dist = DCA_Dist[lvl] * point;
        bool trigger = (posType == POSITION_TYPE_BUY) ? (lastPrice - bid) >= dist
                                                      : (ask - lastPrice) >= dist;
        if(!trigger) return;
        if(DCA_Mode[lvl] == DCA_STEP_TF) {
            int sig = GetSignal();
            if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
            if(posType == POSITION_TYPE_SELL && sig != -1) return;
        }
        if(TimeCurrent() - LastOrderTime < InpOrderDelay) return;

        double baseLot = OldestManualLot(posType);
        if(baseLot <= 0) baseLot = InpLotSize;
        double lot = DCAOrderLot(baseLot, dcaCount + 1, lvl);
        int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
        Print("RTB: DCA level ", lvl+1, " triggered dcaCount=", dcaCount, " [primary chain only]");
        OpenOrder(ord, lot, DCA_TP[lvl], DCA_SL[lvl], true);
        return;
    }

    // ── AUTO MODE ──
    int peak = (posType == POSITION_TYPE_BUY) ? PeakDCABuy : PeakDCASell;

    // 1. Re-fill closed slots at exact original entry price via pending limit orders
    for(int slot = 0; slot < peak; slot++) {
        double slotPrice   = (posType == POSITION_TYPE_BUY) ? DCABuyPrices[slot]   : DCASellPrices[slot];
        bool   slotBounced = (posType == POSITION_TYPE_BUY) ? DCABuyBounced[slot]  : DCASellBounced[slot];
        ulong  limitTk     = (posType == POSITION_TYPE_BUY) ? DCABuyLimitTk[slot]  : DCASellLimitTk[slot];
        if(slotPrice == 0) continue;

        bool isOpen = IsSlotOpen(posType, slot);

        if(!isOpen) {
            // ── Pending limit exists: check if filled or cancel if price moved too far ──
            if(limitTk > 0) {
                if(PositionSelectByTicket(limitTk)) {
                    // Filled → slot is open again at the exact original entry price
                    if(posType == POSITION_TYPE_BUY) { DCABuyTickets[slot]  = limitTk; DCABuyLimitTk[slot]  = 0; DCABuyBounced[slot]  = false; }
                    else                              { DCASellTickets[slot] = limitTk; DCASellLimitTk[slot] = 0; DCASellBounced[slot] = false; }
                    return;
                }
                if(!OrderSelect(limitTk)) {
                    // Order gone but no position → cancelled externally, clear so we retry
                    if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[slot]  = 0;
                    else                              DCASellLimitTk[slot] = 0;
                } else {
                    // Still pending — cancel if price moved too far from entry (order no longer useful)
                    double cancelTol = 300.0 * point;
                    ENUM_ORDER_TYPE oType = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
                    bool tooFar;
                    // BuyLimit fills when ASK falls to slotPrice → cancel if ASK too far below
                    // BuyStop  fills when ASK rises to slotPrice → cancel if ASK too far above
                    // SellLimit fills when BID rises to slotPrice → cancel if BID too far above
                    // SellStop  fills when BID falls to slotPrice → cancel if BID too far below
                    if(posType == POSITION_TYPE_BUY)
                        tooFar = (oType == ORDER_TYPE_BUY_STOP) ? (ask > slotPrice + cancelTol)
                                                                 : (ask < slotPrice - cancelTol);
                    else
                        tooFar = (oType == ORDER_TYPE_SELL_STOP) ? (bid < slotPrice - cancelTol)
                                                                  : (bid > slotPrice + cancelTol);
                    if(tooFar) {
                        Trade.OrderDelete(limitTk);
                        if(posType == POSITION_TYPE_BUY) { DCABuyLimitTk[slot]  = 0; DCABuyBounced[slot]  = false; }
                        else                              { DCASellLimitTk[slot] = 0; DCASellBounced[slot] = false; }
                    }
                }
                continue; // pending order management done for this slot
            }

            // ── No pending order yet: place re-fill order at original entry price ──
            // BUY  (fills at ASK): ASK > slotPrice → BuyLimit  | ASK < slotPrice → BuyStop
            // SELL (fills at BID): BID < slotPrice → SellLimit | BID > slotPrice → SellStop
            if(count < maxOrds) {
                if(TimeCurrent() - LastOrderTime < InpOrderDelay) continue;

                int slotLvl = -1, cum = 0;
                for(int i = 0; i < 15; i++) {
                    int nc = cum + DCA_MaxOrd[i];
                    if(slot < nc) { slotLvl = i; break; }
                    cum = nc;
                }
                if(slotLvl < 0 || DCA_Mode[slotLvl] == DCA_STOP) continue;

                if(DCA_Mode[slotLvl] == DCA_STEP_TF) {
                    int sig = GetSignal();
                    if(posType == POSITION_TYPE_BUY  && sig != 1)  continue;
                    if(posType == POSITION_TYPE_SELL && sig != -1) continue;
                }

                double lot = DCAOrderLot(InpLotSize, slot + 1, slotLvl);
                // |RF suffix marks re-fill orders so RebuildDCAState can skip them (they aren't new slots)
                string cmt = (DCA_TP[slotLvl] == 0 && DCA_SL[slotLvl] == 0)
                    ? "RTB|0|0|D|RF"
                    : "RTB|" + IntegerToString((int)DCA_TP[slotLvl]) + "|" + IntegerToString((int)DCA_SL[slotLvl]) + "|RF";
                bool ok = false;
                if(posType == POSITION_TYPE_BUY) {
                    // BUY fills at ASK → slotPrice = ASK at fill = POSITION_PRICE_OPEN
                    // BuyLimit  valid when slotPrice < ask (order fills when ASK falls to slotPrice)
                    // BuyStop   valid when slotPrice > ask (order fills when ASK rises to slotPrice)
                    double tp_p = DCA_TP[slotLvl] > 0 ? NormalizeDouble(slotPrice + DCA_TP[slotLvl] * point, _Digits) : 0;
                    double sl_p = DCA_SL[slotLvl] > 0 ? NormalizeDouble(slotPrice - DCA_SL[slotLvl] * point, _Digits) : 0;
                    if(ask > slotPrice)       // ASK trên entry → BuyLimit (chờ giá giảm về slotPrice)
                        ok = Trade.BuyLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(ask < slotPrice)  // ASK dưới entry → BuyStop (chờ giá tăng về slotPrice)
                        ok = Trade.BuyStop(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                } else {
                    // SELL fills at BID → slotPrice = BID at fill = POSITION_PRICE_OPEN
                    // SellLimit valid when slotPrice > bid (order fills when BID rises to slotPrice)
                    // SellStop  valid when slotPrice < bid (order fills when BID falls to slotPrice)
                    double tp_p = DCA_TP[slotLvl] > 0 ? NormalizeDouble(slotPrice - DCA_TP[slotLvl] * point, _Digits) : 0;
                    double sl_p = DCA_SL[slotLvl] > 0 ? NormalizeDouble(slotPrice + DCA_SL[slotLvl] * point, _Digits) : 0;
                    if(bid < slotPrice)       // BID dưới entry → SellLimit (chờ giá tăng về slotPrice)
                        ok = Trade.SellLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(bid > slotPrice)  // BID trên entry → SellStop (chờ giá giảm về slotPrice)
                        ok = Trade.SellStop(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                }
                if(ok) {
                    ulong lmtTk = Trade.ResultOrder();
                    if(lmtTk > 0) {
                        Print("RTB: Placed re-fill slot ", slot, " (level ", slotLvl+1, ") at ", slotPrice);
                        if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[slot]  = lmtTk;
                        else                              DCASellLimitTk[slot] = lmtTk;
                        LastOrderTime = TimeCurrent();
                    }
                }
            }
        } else {
            // Slot is open — clear bounce and any stale limit ticket
            if(posType == POSITION_TYPE_BUY) { DCABuyBounced[slot] = false; DCABuyLimitTk[slot]  = 0; }
            else                              { DCASellBounced[slot] = false; DCASellLimitTk[slot] = 0; }
        }
    }

    // 2. Open next new DCA slot
    if(count >= maxOrds) return;
    double lastPrice = LastOpenPrice(posType);
    if(lastPrice == 0) return;

    // Block new slot while any previous slot is closed (re-filling or waiting for bounce)
    for(int slot = 0; slot < peak; slot++) {
        if(!IsSlotOpen(posType, slot)) return;
    }

    int lvl = -1, cumulative = 0;
    for(int i = 0; i < 15; i++) {
        int nextCum = cumulative + DCA_MaxOrd[i];
        if(peak < nextCum) { lvl = i; break; }
        cumulative = nextCum;
    }
    if(lvl < 0 || DCA_Mode[lvl] == DCA_STOP) return;

    double dist = DCA_Dist[lvl] * point;
    bool trigger = (posType == POSITION_TYPE_BUY) ? (lastPrice - bid) >= dist
                                                  : (ask - lastPrice) >= dist;
    if(!trigger) return;

    if(DCA_Mode[lvl] == DCA_STEP_TF) {
        int sig = GetSignal();
        if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
        if(posType == POSITION_TYPE_SELL && sig != -1) return;
    }
    if(TimeCurrent() - LastOrderTime < InpOrderDelay) return;

    double lot = DCAOrderLot(InpLotSize, peak + 1, lvl);
    int    ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
    Print("RTB: DCA level ", lvl+1, " triggered. peak=", peak);
    bool ok = OpenOrder(ord, lot, DCA_TP[lvl], DCA_SL[lvl], true);
    ulong newTk = Trade.ResultOrder();
    if(ok && newTk > 0) {
        double fillPrice = 0;
        if(PositionSelectByTicket(newTk))
            fillPrice = PositionGetDouble(POSITION_PRICE_OPEN);
        if(posType == POSITION_TYPE_BUY) {
            if(peak < 60) { DCABuyPrices[peak] = fillPrice > 0 ? fillPrice : ask; DCABuyTickets[peak] = newTk; }
            PeakDCABuy++;
        } else {
            if(peak < 60) { DCASellPrices[peak] = fillPrice > 0 ? fillPrice : bid; DCASellTickets[peak] = newTk; }
            PeakDCASell++;
        }
    }
}

//+------------------------------------------------------------------+
//| PYRAMIDING (NHỒI DƯƠNG)                                          |
//+------------------------------------------------------------------+
void CheckPyramiding(int posType) {
    // Hedge mode: khi đã xác định chiều xu hướng, chỉ pyramid chiều đó
    if(InpHedgeEnable && HedgeTrendSide >= 0 && HedgeTrendSide != posType) return;

    if(posType == POSITION_TYPE_BUY  && !InpPyraBuyEnable)  return;
    if(posType == POSITION_TYPE_SELL && !InpPyraSellEnable) return;

    int count = CountPos(posType);
    if(count == 0) return;

    // Đọc từ broker qua comment "RTP|" — restart-safe, không lẫn với DCA orders
    int pyraCount = CountPyra(posType);

    int lvl = -1;
    int cumulative = 0;
    for(int i = 0; i < 8; i++) {
        int nextCum = cumulative + PYRA_MaxOrd[i];
        if(pyraCount < nextCum) { lvl = i; break; }
        cumulative = nextCum;
    }
    if(lvl < 0) return;
    if(PYRA_Mode[lvl] == DCA_STOP) return;

    int maxOrds = (posType == POSITION_TYPE_BUY) ? InpMaxBuy : InpMaxSell;
    if(count >= maxOrds) return;

    // Semi-Auto: đo từ lệnh pyramiding gần nhất (nếu có) → fallback giá cực trị lệnh tay
    // Auto: đo từ lệnh được mở gần nhất bất kỳ
    double lastPrice;
    if(InpBotMode == MODE_SEMI_AUTO && CountManual(posType) > 0) {
        double pyraRef = LastPyraPrice(posType);
        lastPrice = (pyraRef > 0) ? pyraRef : ExtremeManualPrice(posType);
    } else {
        lastPrice = LastOpenPrice(posType);
    }
    if(lastPrice == 0) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double dist  = PYRA_Dist[lvl] * point;

    bool inProfit = false;
    if(posType == POSITION_TYPE_BUY)
        inProfit = (bid - lastPrice) >= dist;
    else
        inProfit = (lastPrice - ask) >= dist;

    if(!inProfit) return;

    if(PYRA_Mode[lvl] == DCA_STEP_TF) {
        int sig = GetSignal();
        if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
        if(posType == POSITION_TYPE_SELL && sig != -1) return;
    }

    if(TimeCurrent() - LastOrderTime < InpOrderDelay) return;

    double baseLotPyra;
    if(InpBotMode == MODE_SEMI_AUTO && CountManual(posType) >= 1) {
        double pyraRef = LastPyraPrice(posType);
        baseLotPyra = (pyraRef > 0) ? LastPyraLot(posType) : ExtremeManualLot(posType);
        if(baseLotPyra <= 0) baseLotPyra = InpLotSize;
    } else {
        baseLotPyra = InpLotSize;
    }
    double lot = NormLot(baseLotPyra * PYRA_Mult[lvl]);
    int    ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;

    // Luôn isPyra=true để comment luôn là "RTP|..." → CountPyra() đếm đúng
    // Khi PYRA_TP=PYRA_SL=0 (cả hai): fallback toàn bộ sang InpTP/SL_Points
    // Nếu chỉ một trong hai bằng 0: dùng đúng giá trị tier (0 = không đặt)
    bool   noTierExit = (PYRA_TP[lvl] == 0 && PYRA_SL[lvl] == 0);
    double openTP     = noTierExit ? InpTP_Points : PYRA_TP[lvl];
    double openSL     = noTierExit ? InpSL_Points : PYRA_SL[lvl];

    Print("RTB: Pyramiding level ", lvl+1, " triggered. pyraCount=", pyraCount);
    if(OpenOrder(ord, lot, openTP, openSL, false, true)) {
        if(InpHedgeEnable && HedgeTrendSide < 0) {
            HedgeTrendSide = posType;
            Print("RTB: Hedge — xu hướng xác định: ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"));
        }
    }
}

//+------------------------------------------------------------------+
//| ORDER TRIMMING                                                   |
//+------------------------------------------------------------------+
void CheckTrimming() {
    if(!InpTrimEnable) return;
    if(CountAll() < InpTrimTrigger) return;

    double balance = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity  = AccountInfoDouble(ACCOUNT_EQUITY);

    // Partial trim: by drawdown%
    if(InpPartialTrim && balance > 0) {
        double ddPct = (balance - equity) / balance * 100.0;
        if(ddPct > InpPartialTrimDD) {
            int closed = 0;
            for(int n = 0; n < InpTrimMaxLoss; n++) {
                if(CountAll() < InpTrimTrigger) break;
                ulong tk = WorstTicket();
                if(tk == 0) break;
                Trade.PositionClose(tk);
                closed++;
            }
            if(closed > 0) {
                Print("RTB: Partial Trim DD=", ddPct, "% closed=", closed);
                return;
            }
        }
    }

    // Trim by day profit: if today's closed profit > |worst floating loss|
    if(InpTrimByDayProfit) {
        int closed = 0;
        for(int n = 0; n < InpTrimMaxLoss; n++) {
            ulong worstTk = WorstTicket();
            if(worstTk == 0 || !PositionSelectByTicket(worstTk)) break;
            double worstP = PositionGetDouble(POSITION_PROFIT);
            if(DayProfit > MathAbs(worstP) && DayProfit > 0) {
                Trade.PositionClose(worstTk);
                closed++;
            } else break;
        }
        if(closed > 0) {
            Print("RTB: Trim by DayProfit=", DayProfit, " closed=", closed);
            return;
        }
    }

    // Hedging trim: 1 best covers up to InpTrimMaxLoss worst positions per cycle
    // MaxWin controls how many such cycles to attempt per second
    if(InpTrimHedge) {
        int closedCycles = 0;
        for(int w = 0; w < InpTrimMaxWin; w++) {
            ulong bestTk = BestTicket();
            if(bestTk == 0 || !PositionSelectByTicket(bestTk)) break;
            double bestP = PositionGetDouble(POSITION_PROFIT);

            // Collect up to InpTrimMaxLoss worst tickets (excluding bestTk and already picked)
            ulong  worstTks[8];
            int    wn      = 0;
            double worstSum = 0;
            ulong  excluded[9];
            int    exCount = 1;
            excluded[0] = bestTk;

            for(int n = 0; n < InpTrimMaxLoss; n++) {
                ulong  wtk  = 0;
                double wval = 0;
                for(int i = PositionsTotal()-1; i >= 0; i--) {
                    ulong tk = PositionGetTicket(i);
                    if(!PositionSelectByTicket(tk)) continue;
                    if(!IsManaged()) continue;
                    bool skip = false;
                    for(int e = 0; e < exCount; e++) if(tk == excluded[e]) { skip = true; break; }
                    if(skip) continue;
                    double p = PositionGetDouble(POSITION_PROFIT);
                    if(wtk == 0 || p < wval) { wtk = tk; wval = p; }
                }
                if(wtk == 0) break;
                worstTks[wn] = wtk;
                worstSum    += wval;
                excluded[exCount++] = wtk;
                wn++;
            }

            if(wn > 0 && bestP + worstSum >= InpTrimTarget) {
                Trade.PositionClose(bestTk);
                for(int i = 0; i < wn; i++) Trade.PositionClose(worstTks[i]);
                closedCycles++;
            } else break;
        }
        if(closedCycles > 0) {
            Print("RTB: Hedge trim cycles=", closedCycles, " x up to ", InpTrimMaxLoss, " losers");
            return;
        }
    }

    // Same-direction trim: use aggregate floating profit vs target
    if(InpTrimTarget > 0) {
        int closed = 0;
        for(int n = 0; n < InpTrimMaxLoss; n++) {
            double totalProfit = FloatProfit();
            if(totalProfit < InpTrimTarget) break;
            ulong worstTk = WorstTicket();
            if(worstTk == 0 || !PositionSelectByTicket(worstTk)) break;
            double worstP = PositionGetDouble(POSITION_PROFIT);
            if((totalProfit + worstP) >= InpTrimTarget) {
                Trade.PositionClose(worstTk);
                closed++;
            } else break;
        }
        if(closed > 0)
            Print("RTB: Trim target met, closed=", closed);
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
    // Kiểm tra stops level — broker từ chối nếu SL quá gần giá
    if(posType == POSITION_TYPE_BUY  && bid - newSL < minDist) return;
    if(posType == POSITION_TYPE_SELL && newSL - ask < minDist) return;
    // Đặt server SL về đúng mức trailing, kể cả khi phải hạ DCA SL hiện có.
    // DCA SL vẫn được bảo vệ bởi software (CheckExit section 2a).
    // Mục đích: đường SL trên chart khớp với đường trailing, tránh hiển thị 2 đường khác mức.
    if(curSL != normSL)
        Trade.PositionModify(tk, normSL, curTP);
}

void CheckTrailing() {
    bool hedgeTrail = InpHedgeEnable && (HedgeCutBuy || HedgeCutSell);
    if(!InpTrailEnable && !hedgeTrail) return;
    if(CountAll() < InpTrailMinOrds) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    if(InpTrailMode == TRAIL_BASKET) {
        // --- BUY BASKET ---
        if(CountBuy() > 0) {
            double avgBuy = AvgOpenPrice(POSITION_TYPE_BUY);
            if(bid - avgBuy >= InpTrailActivate * point) {
                double newSL = bid - InpTrailInit * point;
                if(TrailBuy == 0 || newSL >= TrailBuy + InpTrailStep * point)
                    TrailBuy = newSL;
            }
            // Apply every second — covers new DCA/Pyra orders opened after trail activated
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

        // --- SELL BASKET ---
        if(CountSell() > 0) {
            double avgSell = AvgOpenPrice(POSITION_TYPE_SELL);
            if(avgSell - ask >= InpTrailActivate * point) {
                double newSL = ask + InpTrailInit * point;
                if(TrailSell == 0 || newSL <= TrailSell - InpTrailStep * point)
                    TrailSell = newSL;
            }
            // Apply every second — covers new DCA/Pyra orders opened after trail activated
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

        // Draw lines
        if(InpTrailShowLine) {
            if(TrailBuy  > 0) DrawHLine("TrailBuy",  TrailBuy,  InpTrailBuyColor,  InpTrailLineWidth);
            else ObjectDelete(0, GUI + "TrailBuy");
            if(TrailSell > 0) DrawHLine("TrailSell", TrailSell, InpTrailSellColor, InpTrailLineWidth);
            else ObjectDelete(0, GUI + "TrailSell");
        }

    } else {
        // --- SINGLE TRAILING PER POSITION ---
        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            int    pt        = (int)PositionGetInteger(POSITION_TYPE);
            double openPrice = PositionGetDouble(POSITION_PRICE_OPEN);

            if(pt == POSITION_TYPE_BUY) {
                double profitPts = (bid - openPrice) / point;
                if(profitPts >= InpTrailActivate) {
                    double newSL = bid - InpTrailInit * point;
                    double curSL = PositionGetDouble(POSITION_SL);
                    if(curSL == 0 || newSL >= curSL + InpTrailStep * point)
                        ApplyTrailToPos(tk, POSITION_TYPE_BUY, newSL);
                }
            } else {
                double profitPts = (openPrice - ask) / point;
                if(profitPts >= InpTrailActivate) {
                    double newSL = ask + InpTrailInit * point;
                    double curSL = PositionGetDouble(POSITION_SL);
                    if(curSL == 0 || newSL <= curSL - InpTrailStep * point)
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
    // 0. Software trailing fallback — đóng khi giá vượt qua TrailBuy/TrailSell
    //    Server SL bị từ chối (invalid stop) khi SL quá gần giá → lệnh mới không có server SL.
    //    Section này đảm bảo thoát đúng dù server SL không được đặt thành công.
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

    // 1. Per-position exit by pips target
    if(InpClosePerPips > 0) {
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

            if(ppts >= InpClosePerPips)
                Trade.PositionClose(tk);
        }
    }

    // 2a. DCA TP/SL — luôn chạy (không cần Stealth Mode)
    //     Đóng lệnh DCA đúng TP/SL của từng tầng dù Use_TP = false
    {
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            string cmt = PositionGetString(POSITION_COMMENT);
            // Xử lý DCA ("RTB|tp|sl") và Pyramiding ("RTP|tp|sl") — bỏ qua "RTB|0|0" (lệnh gốc)
            bool isDCAcmt  = (StringFind(cmt, "RTB|") == 0);
            bool isPyracmt = (StringFind(cmt, "RTP|") == 0);
            if(!isDCAcmt && !isPyracmt) continue;
            string parts[];
            if(StringSplit(cmt, '|', parts) < 3) continue;
            double useTP = StringToDouble(parts[1]);
            double useSL = StringToDouble(parts[2]);
            if(useTP == 0 && useSL == 0) continue; // lệnh gốc "RTB|0|0" hoặc pyra fallback, bỏ qua

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

    // 2b. Stealth TP/SL — lệnh gốc, chỉ chạy khi Stealth Mode bật
    if(InpStealthMode) {
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

        for(int i = PositionsTotal()-1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;

            // Xử lý lệnh gốc ("RTB|0|0") và lệnh thủ công trong Semi-Auto
            // "RTP|..." có TP/SL → section 2a đã xử lý; "RTP|0|0|P" không bao giờ có TP → bỏ qua
            string cmt = PositionGetString(POSITION_COMMENT);
            bool isManualPos = (InpBotMode == MODE_SEMI_AUTO &&
                                PositionGetInteger(POSITION_MAGIC) == 0 &&
                                StringFind(cmt, "RTB|") != 0 &&
                                StringFind(cmt, "RTP|") != 0);
            if(cmt != "RTB|0|0" && !isManualPos) continue;

            int    pt  = (int)PositionGetInteger(POSITION_TYPE);
            double opn = PositionGetDouble(POSITION_PRICE_OPEN);

            // Stealth Mode thay thế hoàn toàn server TP/SL — không phụ thuộc InpUseTakeProfit/SL
            if(pt == POSITION_TYPE_BUY) {
                if(InpTP_Points > 0 && bid >= opn + InpTP_Points * point)
                    { Trade.PositionClose(tk); continue; }
                if(InpSL_Points > 0 && bid <= opn - InpSL_Points * point)
                    Trade.PositionClose(tk);
            } else {
                if(InpTP_Points > 0 && ask <= opn - InpTP_Points * point)
                    { Trade.PositionClose(tk); continue; }
                if(InpSL_Points > 0 && ask >= opn + InpSL_Points * point)
                    Trade.PositionClose(tk);
            }
        }
    }

    // 3. Basket total profit target
    // Bỏ qua khi Hedge mode đã cắt một chiều — trailing stop quản lý thoát chiều dương
    if(InpCloseProfit > 0 && !(InpHedgeEnable && (HedgeCutBuy || HedgeCutSell))) {
        if(FloatProfit() >= InpCloseProfit) {
            Print("RTB: CloseProfit target reached. Closing all.");
            CloseAll();
            return;
        }
    }

    // 4. Basket total loss cut
    // Bỏ qua khi Hedge mode đang chạy chiều dương — tránh cắt nhầm chiều thắng
    if(InpCloseLoss > 0 && !(InpHedgeEnable && (HedgeCutBuy || HedgeCutSell))) {
        if(FloatProfit() <= -InpCloseLoss) {
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
    // Dùng equity = lãi đã chốt hôm nay + floating hiện tại (phản ứng ngay dù chưa đóng lệnh)
    double equity = DayProfit + FloatProfit();
    if(InpDayMaxLoss > 0 && equity <= -InpDayMaxLoss) {
        Print("RTB: Day loss limit $", InpDayMaxLoss, " hit. equity=", equity, ". Closing all.");
        CloseAll();
        DayLimitHit = true;
        return;
    }
    if(InpDayMaxProfit > 0 && equity >= InpDayMaxProfit) {
        Print("RTB: Day profit target $", InpDayMaxProfit, " hit. equity=", equity, ". Closing all.");
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
    // 1 price unit = 10 pips (fixed)
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
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Consolas");
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
        ObjectSetInteger(0, obj, OBJPROP_XSIZE,       lw);
        ObjectSetInteger(0, obj, OBJPROP_YSIZE,       lh);
        ObjectSetInteger(0, obj, OBJPROP_BGCOLOR,     bg);
        ObjectSetInteger(0, obj, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, obj, OBJPROP_COLOR,       bg);
        ObjectSetInteger(0, obj, OBJPROP_WIDTH,       0);
        ObjectSetInteger(0, obj, OBJPROP_BACK,        false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, lx);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, ly);
}

void Lbl(string name, string text, int x, int y, color clr = clrSilver, int sz = 9) {
    string obj = GUI + name;
    if(ObjectFind(0, obj) < 0) {
        ObjectCreate(0, obj, OBJ_LABEL, 0, 0, 0);
        ObjectSetInteger(0, obj, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetString(0,  obj, OBJPROP_FONT,       "Consolas");
        ObjectSetInteger(0, obj, OBJPROP_BACK,       false);
        ObjectSetInteger(0, obj, OBJPROP_SELECTABLE, false);
    }
    ObjectSetInteger(0, obj, OBJPROP_XDISTANCE, x);
    ObjectSetInteger(0, obj, OBJPROP_YDISTANCE, y);
    ObjectSetString(0,  obj, OBJPROP_TEXT,     text);
    ObjectSetInteger(0, obj, OBJPROP_COLOR,    clr);
    ObjectSetInteger(0, obj, OBJPROP_FONTSIZE, sz);
}

void UpdateGUI() {
    if(!InpShowPanel) { RemoveGUI(); return; }
    int PX = InpPanelX;
    int PY = InpPanelY;
    int PW = InpPanelWidth;
    int hOff = InpHedgeEnable ? 16 : 0;  // Thêm 1 dòng Hedge khi bật

    double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity    = AccountInfoDouble(ACCOUNT_EQUITY);
    double spread    = (double)SymbolInfoInteger(_Symbol, SYMBOL_SPREAD);
    double totalProfit = 0, buyProfit = 0, sellProfit = 0;
    int    nBuy = 0, nSell = 0;
    double lotBuy = 0, lotSell = 0;
    for(int i = PositionsTotal()-1; i >= 0; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(!IsManaged()) continue;
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
    switch(InpSignalMode) {
        case SIG_EMA:       sigName = "EMA 34+89";      break;
        case SIG_BZ_ZONE:   sigName = "BZ Zone";        break;
        case SIG_ICHIMOKU:  sigName = "Ichimoku";       break;
        case SIG_BB:        sigName = "Bollinger Band"; break;
        case SIG_SIMULATED: sigName = "Simulated";      break;
        case SIG_UT_BOT:    sigName = "UT Bot";         break;
    }

    string dirName = "";
    color  dirClr  = clrSilver;
    switch(InpDirection) {
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

    // ── PANEL 1: THÔNG TIN ──
    string bg = GUI + "BG";
    if(ObjectFind(0, bg) < 0) {
        ObjectCreate(0, bg, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg, OBJPROP_YSIZE,       362 + hOff);
        ObjectSetInteger(0, bg, OBJPROP_BGCOLOR,     C'14,17,26');
        ObjectSetInteger(0, bg, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg, OBJPROP_COLOR,       C'50,65,120');
        ObjectSetInteger(0, bg, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg, OBJPROP_YDISTANCE, PY);
    ObjectSetInteger(0, bg, OBJPROP_YSIZE,     362 + hOff);

    // ── PANEL 2: ĐIỀU KHIỂN ──
    string bg2 = GUI + "BG2";
    if(ObjectFind(0, bg2) < 0) {
        ObjectCreate(0, bg2, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg2, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg2, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg2, OBJPROP_YSIZE,       110);
        ObjectSetInteger(0, bg2, OBJPROP_BGCOLOR,     C'17,21,32');
        ObjectSetInteger(0, bg2, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg2, OBJPROP_COLOR,       C'65,90,160');
        ObjectSetInteger(0, bg2, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg2, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg2, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg2, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg2, OBJPROP_YDISTANCE, PY + 376 + hOff);

    // ── PANEL 3: THỐNG KÊ ──
    string bg3 = GUI + "BG3";
    if(ObjectFind(0, bg3) < 0) {
        ObjectCreate(0, bg3, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg3, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg3, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg3, OBJPROP_YSIZE,       115);
        ObjectSetInteger(0, bg3, OBJPROP_BGCOLOR,     C'14,19,28');
        ObjectSetInteger(0, bg3, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg3, OBJPROP_COLOR,       C'50,70,130');
        ObjectSetInteger(0, bg3, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg3, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg3, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg3, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg3, OBJPROP_YDISTANCE, PY + 516 + hOff);

    // ── NỘI DUNG PANEL 1 ──
    int x = PX + 7, y = PY + 5, s = 16;
    Lbl("T",    " RICH TRADING BOT  v1.0",   x, y, C'80,160,255', 10); y += s+2;
    Lbl("L0",   "────────────────────────",   x, y, C'45,58,105'  );    y += s-2;
    Lbl("Tim",  "Time   : " + tStr,           x, y, clrSilver     );    y += s;
    Lbl("Sig",  "Signal : " + sigName,        x, y, clrYellow     );    y += s;
    string modeName = (InpBotMode == MODE_SEMI_AUTO) ? "Ban Tu Dong" : "Tu Dong";
    color  modeClr  = (InpBotMode == MODE_SEMI_AUTO) ? clrOrange    : clrLimeGreen;
    Lbl("Mod",  "Mode   : " + modeName,       x, y, modeClr       );    y += s;
    Lbl("Dir",  "Direct : " + dirName,        x, y, dirClr        );    y += s;
    if(InpHedgeEnable) {
        string hedgeText; color hedgeClr;
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        if(HedgeCutBuy && !HedgeCutSell) {
            hedgeText = "Hedge  : ✕BUY  |  ▼SELL Trail";
            hedgeClr  = clrLimeGreen;
        } else if(HedgeCutSell && !HedgeCutBuy) {
            hedgeText = "Hedge  : ▲BUY Trail  |  ✕SELL";
            hedgeClr  = clrLimeGreen;
        } else if(!HedgeCutBuy && !HedgeCutSell) {
            double buyDist  = (HedgeInitBuyPrice  > 0) ? (HedgeInitBuyPrice  - bid) / point : 0;
            double sellDist = (HedgeInitSellPrice > 0) ? (ask - HedgeInitSellPrice) / point : 0;
            if(buyDist > 0 || sellDist > 0)
                hedgeText = StringFormat("Hedge  : ▲%.0f | ▼%.0f pt", buyDist, sellDist);
            else
                hedgeText = "Hedge  : Waiting...";
            hedgeClr = clrSilver;
        } else {
            hedgeText = "Hedge  : Complete"; hedgeClr = C'90,90,90';
        }
        Lbl("HdgS", hedgeText, x, y, hedgeClr); y += s;
    }
    Lbl("L1",   "────────────────────────",   x, y, C'45,58,105'  );    y += s-2;
    Lbl("Bal",  StringFormat("Balance: $%.2f", balance),    x, y, clrSilver); y += s;
    Lbl("Ini",  StringFormat("Initial: $%.2f", InitBalance), x, y, clrSilver); y += s;
    Lbl("DayP", StringFormat("Day P/L: $%.2f", DayProfit),  x, y, cDayP);     y += s;
    {
        string lmtText;
        color  lmtClr;
        if(DayLimitHit) {
            lmtText = "Day Lmt: !! STOP !!";
            lmtClr  = clrTomato;
        } else {
            lmtText = StringFormat("Day Lmt: L$%.0f / P$%.0f", InpDayMaxLoss, InpDayMaxProfit);
            lmtClr  = C'90,90,90';
        }
        Lbl("DayLmt", lmtText, x, y, lmtClr);
    }                                                                           y += s;
    Lbl("FP",   StringFormat("Float  : $%.2f  (%.2f%%)", totalProfit, pnlPct),
                x, y, cProfit);                                                y += s;
    Lbl("L2",   "────────────────────────",   x, y, C'45,58,105'  );    y += s-2;
    Lbl("DD",   StringFormat("DD Now : %.2f%%", ddPct),     x, y,
                ddPct > 15 ? clrOrangeRed : clrSilver);                       y += s;
    Lbl("MDD",  StringFormat("DD Max : %.2f%%", MaxDrawdownPct), x, y,
                MaxDrawdownPct > 25 ? clrTomato : clrSilver);                 y += s;
    Lbl("Sprd", StringFormat("Spread : %.0f pts", spread),  x, y, clrSilver); y += s;
    Lbl("L3",   "────────────────────────",   x, y, C'45,58,105'  );    y += s-2;
    Lbl("BuyP", StringFormat("Buy P/L: $%.2f", buyProfit),  x, y, cBuyP);     y += s;
    Lbl("BuyC", StringFormat("Buy Ord: %d   Lot: %.2f", nBuy,  lotBuy),  x, y, clrSilver); y += s;
    Lbl("SelP", StringFormat("Sel P/L: $%.2f", sellProfit), x, y, cSellP);    y += s;
    Lbl("SelC", StringFormat("Sel Ord: %d   Lot: %.2f", nSell, lotSell), x, y, clrSilver); y += s;
    Lbl("Tot",  StringFormat("Total  : %d orders", nBuy + nSell), x, y, clrSilver);        y += s;

    // ── NỘI DUNG PANEL 2 (Nút điều khiển) ──
    y = PY + 386 + hOff;
    Lbl("P2T", "═══  ĐIỀU KHIỂN LỆNH  ═══", x, y, C'90,140,230', 9); y += s + 2;

    int bh  = 22;
    int bfw = PW - 18;              // full-width button
    int bhw = (PW - 24) / 2;        // half-width button
    int bx2 = PX + 7 + bhw + 4;     // x of second button in a row
    CreateBtn("BtnCloseAll",    "  Close All",     PX+7, y, bfw, bh, C'20,60,150',  C'80,130,230'); y += bh + 4;
    CreateBtn("BtnCloseBuy",    "▲ Close Buy",     PX+7, y, bhw, bh, C'0,105,45',   C'45,185,90' );
    CreateBtn("BtnCloseProfit", "$ Close Profit",  bx2,  y, bhw, bh, C'0,110,100',  C'40,190,170'); y += bh + 4;
    CreateBtn("BtnCloseSell",   "▼ Close Sell",    PX+7, y, bhw, bh, C'145,15,15',  C'230,65,65' );
    CreateBtn("BtnCloseLoss",   "✕ Close Loss",    bx2,  y, bhw, bh, C'140,35,20',  C'210,80,55' );

    // ── NỘI DUNG PANEL 3 (Thống kê) ──
    y = PY + 526 + hOff;
    Lbl("P3T", "═══  THỐNG KÊ  ═══", x, y, C'90,140,230', 9); y += s + 2;

    color sepClr = C'45,65,120';
    int tblTop = y, tblH = 5*(s-2);

    int tw  = PW - 4;
    int vc1 = PX + 2 + tw * 24 / 100;
    int vc2 = PX + 2 + tw * 44 / 100;
    int vc3 = PX + 2 + tw * 65 / 100;
    int vc4 = PX + 2 + tw * 84 / 100;
    CreateRect("P3VC1", vc1, tblTop, 1, tblH, sepClr);
    CreateRect("P3VC2", vc2, tblTop, 1, tblH, sepClr);
    CreateRect("P3VC3", vc3, tblTop, 1, tblH, sepClr);
    CreateRect("P3VC4", vc4, tblTop, 1, tblH, sepClr);
    for(int si = 0; si < 4; si++)
        CreateRect("P3HR"+IntegerToString(si), PX+2, tblTop + (si+1)*(s-2) - 1, PW-4, 1, sepClr);

    int cx0=PX+7, cx1=vc1+2, cx2=vc2+2, cx3=vc3+2, cx4=vc4+2;
    Lbl("TH0", "Date ",  cx0, y, C'100,125,195', 8);
    Lbl("TH1", "Pips ",  cx1, y, C'100,125,195', 8);
    Lbl("TH2", "Profit", cx2, y, C'100,125,195', 8);
    Lbl("TH3", "Gain ",  cx3, y, C'100,125,195', 8);
    Lbl("TH4", "Lot  ",  cx4, y, C'100,125,195', 8);
    y += s - 2;

    TimeToStruct(TimeCurrent(), dt);
    datetime todayStart = StringToTime(StringFormat("%04d.%02d.%02d 00:00:00", dt.year, dt.mon, dt.day));
    int dow = dt.day_of_week; if(dow == 0) dow = 7;
    datetime weekStart  = todayStart - (dow - 1) * 86400;
    datetime monthStart = StringToTime(StringFormat("%04d.%02d.01 00:00:00", dt.year, dt.mon));
    datetime yearStart  = StringToTime(StringFormat("%04d.01.01 00:00:00", dt.year));
    datetime nowT       = TimeCurrent();

    PeriodStats allStats[4];
    allStats[0] = GetPeriodStats(todayStart, nowT);
    allStats[1] = GetPeriodStats(weekStart,  nowT);
    allStats[2] = GetPeriodStats(monthStart, nowT);
    allStats[3] = GetPeriodStats(yearStart,  nowT);

    string rowKeys[4];
    rowKeys[0]="Today"; rowKeys[1]="Week"; rowKeys[2]="Month"; rowKeys[3]="Year";

    for(int r = 0; r < 4; r++) {
        color rc = (allStats[r].profit >= 0) ? clrLimeGreen : clrTomato;
        string ri = IntegerToString(r);
        Lbl("TR"+ri+"L", rowKeys[r],                                cx0, y, clrSilver, 8);
        Lbl("TR"+ri+"P", StringFormat("%.0f",   allStats[r].pips),  cx1, y, rc, 8);
        Lbl("TR"+ri+"$", StringFormat("$%.1f",  allStats[r].profit), cx2, y, rc, 8);
        Lbl("TR"+ri+"G", StringFormat("%.1f%%", allStats[r].gain),  cx3, y, rc, 8);
        Lbl("TR"+ri+"V", StringFormat("%.2f",   allStats[r].lot),   cx4, y, clrSilver, 8);
        y += s - 2;
    }

    // ── PANEL 4: VÀO LỆNH THỦ CÔNG (chỉ hiện khi Bán Tự Động) ──
    if(InpBotMode == MODE_SEMI_AUTO) {
        string bg4 = GUI + "BG4";
        if(ObjectFind(0, bg4) < 0) {
            ObjectCreate(0, bg4, OBJ_RECTANGLE_LABEL, 0, 0, 0);
            ObjectSetInteger(0, bg4, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
            ObjectSetInteger(0, bg4, OBJPROP_XSIZE,       PW);
            ObjectSetInteger(0, bg4, OBJPROP_YSIZE,       58);
            ObjectSetInteger(0, bg4, OBJPROP_BGCOLOR,     C'20,14,14');
            ObjectSetInteger(0, bg4, OBJPROP_BORDER_TYPE, BORDER_FLAT);
            ObjectSetInteger(0, bg4, OBJPROP_COLOR,       C'160,60,60');
            ObjectSetInteger(0, bg4, OBJPROP_WIDTH,       1);
            ObjectSetInteger(0, bg4, OBJPROP_BACK,        false);
            ObjectSetInteger(0, bg4, OBJPROP_SELECTABLE,  false);
        }
        ObjectSetInteger(0, bg4, OBJPROP_XDISTANCE, PX);
        ObjectSetInteger(0, bg4, OBJPROP_YDISTANCE, PY + 673 + hOff);
        int y4 = PY + 683 + hOff;
        Lbl("P4T", "═══  VÀO LỆNH THỦ CÔNG  ═══", x, y4, C'230,100,100', 9); y4 += s + 2;
        CreateBtn("BtnOpenBuy",  "▲ Open Buy",  PX+7, y4, bhw, bh, C'0,80,20',  C'30,200,80');
        CreateBtn("BtnOpenSell", "▼ Open Sell", bx2,  y4, bhw, bh, C'100,0,0',  C'220,40,40');
    } else {
        ObjectDelete(0, GUI + "BG4");
        ObjectDelete(0, GUI + "P4T");
        ObjectDelete(0, GUI + "BtnOpenBuy");
        ObjectDelete(0, GUI + "BtnOpenSell");
    }

    ChartRedraw(0);
}

void RemoveGUI() { ObjectsDeleteAll(0, GUI); }

//+------------------------------------------------------------------+
//| INIT DCA ARRAYS                                                  |
//+------------------------------------------------------------------+
void InitDCA() {
    DCA_Mode[0]=InpDCAMode; DCA_Mult[0]=InpDCA1Mult;  DCA_MaxOrd[0]=InpDCA1Max;
    DCA_Dist[0]=InpDCA1Dist;  DCA_TP[0]=InpDCA1TP;   DCA_SL[0]=InpDCA1SL;

    DCA_Mode[1]=InpDCAMode; DCA_Mult[1]=InpDCA2Mult;  DCA_MaxOrd[1]=InpDCA2Max;
    DCA_Dist[1]=InpDCA2Dist;  DCA_TP[1]=InpDCA2TP;   DCA_SL[1]=InpDCA2SL;

    DCA_Mode[2]=InpDCAMode; DCA_Mult[2]=InpDCA3Mult;  DCA_MaxOrd[2]=InpDCA3Max;
    DCA_Dist[2]=InpDCA3Dist;  DCA_TP[2]=InpDCA3TP;   DCA_SL[2]=InpDCA3SL;

    DCA_Mode[3]=InpDCAMode; DCA_Mult[3]=InpDCA4Mult;  DCA_MaxOrd[3]=InpDCA4Max;
    DCA_Dist[3]=InpDCA4Dist;  DCA_TP[3]=InpDCA4TP;   DCA_SL[3]=InpDCA4SL;

    DCA_Mode[4]=InpDCAMode; DCA_Mult[4]=InpDCA5Mult;  DCA_MaxOrd[4]=InpDCA5Max;
    DCA_Dist[4]=InpDCA5Dist;  DCA_TP[4]=InpDCA5TP;   DCA_SL[4]=InpDCA5SL;

    DCA_Mode[5]=InpDCAMode; DCA_Mult[5]=InpDCA6Mult;  DCA_MaxOrd[5]=InpDCA6Max;
    DCA_Dist[5]=InpDCA6Dist;  DCA_TP[5]=InpDCA6TP;   DCA_SL[5]=InpDCA6SL;

    DCA_Mode[6]=InpDCAMode; DCA_Mult[6]=InpDCA7Mult;  DCA_MaxOrd[6]=InpDCA7Max;
    DCA_Dist[6]=InpDCA7Dist;  DCA_TP[6]=InpDCA7TP;   DCA_SL[6]=InpDCA7SL;

    DCA_Mode[7]=InpDCAMode; DCA_Mult[7]=InpDCA8Mult;  DCA_MaxOrd[7]=InpDCA8Max;
    DCA_Dist[7]=InpDCA8Dist;  DCA_TP[7]=InpDCA8TP;   DCA_SL[7]=InpDCA8SL;

    DCA_Mode[8]=InpDCAMode; DCA_Mult[8]=InpDCA9Mult;  DCA_MaxOrd[8]=InpDCA9Max;
    DCA_Dist[8]=InpDCA9Dist;  DCA_TP[8]=InpDCA9TP;   DCA_SL[8]=InpDCA9SL;

    DCA_Mode[9]=InpDCAMode; DCA_Mult[9]=InpDCA10Mult; DCA_MaxOrd[9]=InpDCA10Max;
    DCA_Dist[9]=InpDCA10Dist; DCA_TP[9]=InpDCA10TP;  DCA_SL[9]=InpDCA10SL;

    DCA_Mode[10]=InpDCAMode; DCA_Mult[10]=InpDCA11Mult; DCA_MaxOrd[10]=InpDCA11Max;
    DCA_Dist[10]=InpDCA11Dist; DCA_TP[10]=InpDCA11TP;  DCA_SL[10]=InpDCA11SL;

    DCA_Mode[11]=InpDCAMode; DCA_Mult[11]=InpDCA12Mult; DCA_MaxOrd[11]=InpDCA12Max;
    DCA_Dist[11]=InpDCA12Dist; DCA_TP[11]=InpDCA12TP;  DCA_SL[11]=InpDCA12SL;

    DCA_Mode[12]=InpDCAMode; DCA_Mult[12]=InpDCA13Mult; DCA_MaxOrd[12]=InpDCA13Max;
    DCA_Dist[12]=InpDCA13Dist; DCA_TP[12]=InpDCA13TP;  DCA_SL[12]=InpDCA13SL;

    DCA_Mode[13]=InpDCAMode; DCA_Mult[13]=InpDCA14Mult; DCA_MaxOrd[13]=InpDCA14Max;
    DCA_Dist[13]=InpDCA14Dist; DCA_TP[13]=InpDCA14TP;  DCA_SL[13]=InpDCA14SL;

    DCA_Mode[14]=InpDCAMode; DCA_Mult[14]=InpDCA15Mult; DCA_MaxOrd[14]=InpDCA15Max;
    DCA_Dist[14]=InpDCA15Dist; DCA_TP[14]=InpDCA15TP;  DCA_SL[14]=InpDCA15SL;
}

void InitPyra() {
    PYRA_Mode[0]=InpPyraMode; PYRA_Mult[0]=InpPyra1Mult; PYRA_MaxOrd[0]=InpPyra1Max;
    PYRA_Dist[0]=InpPyra1Dist; PYRA_TP[0]=InpPyra1TP;   PYRA_SL[0]=InpPyra1SL;

    PYRA_Mode[1]=InpPyraMode; PYRA_Mult[1]=InpPyra2Mult; PYRA_MaxOrd[1]=InpPyra2Max;
    PYRA_Dist[1]=InpPyra2Dist; PYRA_TP[1]=InpPyra2TP;   PYRA_SL[1]=InpPyra2SL;

    PYRA_Mode[2]=InpPyraMode; PYRA_Mult[2]=InpPyra3Mult; PYRA_MaxOrd[2]=InpPyra3Max;
    PYRA_Dist[2]=InpPyra3Dist; PYRA_TP[2]=InpPyra3TP;   PYRA_SL[2]=InpPyra3SL;

    PYRA_Mode[3]=InpPyraMode; PYRA_Mult[3]=InpPyra4Mult; PYRA_MaxOrd[3]=InpPyra4Max;
    PYRA_Dist[3]=InpPyra4Dist; PYRA_TP[3]=InpPyra4TP;   PYRA_SL[3]=InpPyra4SL;

    PYRA_Mode[4]=InpPyraMode; PYRA_Mult[4]=InpPyra5Mult; PYRA_MaxOrd[4]=InpPyra5Max;
    PYRA_Dist[4]=InpPyra5Dist; PYRA_TP[4]=InpPyra5TP;   PYRA_SL[4]=InpPyra5SL;

    PYRA_Mode[5]=InpPyraMode; PYRA_Mult[5]=InpPyra6Mult; PYRA_MaxOrd[5]=InpPyra6Max;
    PYRA_Dist[5]=InpPyra6Dist; PYRA_TP[5]=InpPyra6TP;   PYRA_SL[5]=InpPyra6SL;

    PYRA_Mode[6]=InpPyraMode; PYRA_Mult[6]=InpPyra7Mult; PYRA_MaxOrd[6]=InpPyra7Max;
    PYRA_Dist[6]=InpPyra7Dist; PYRA_TP[6]=InpPyra7TP;   PYRA_SL[6]=InpPyra7SL;

    PYRA_Mode[7]=InpPyraMode; PYRA_Mult[7]=InpPyra8Mult; PYRA_MaxOrd[7]=InpPyra8Max;
    PYRA_Dist[7]=InpPyra8Dist; PYRA_TP[7]=InpPyra8TP;   PYRA_SL[7]=InpPyra8SL;
}

//+------------------------------------------------------------------+
//| LICENSE CHECK                                                    |
//+------------------------------------------------------------------+
bool CheckLicense() {
    if(StringFind(LICENSE_URL, "YOUR_SCRIPT_ID") >= 0) {
        Print("RTB: LICENSE_URL chưa được cấu hình trong source code.");
        return false;
    }

    long   accID = AccountInfoInteger(ACCOUNT_LOGIN);
    string url   = LICENSE_URL + "?id=" + IntegerToString(accID);

    char   post[];
    char   result[];
    string headers;
    ResetLastError();
    int httpCode = WebRequest("GET", url, "", "", 10000, post, 0, result, headers);

    if(httpCode == -1) {
        int err = GetLastError();
        string hint = (err == 4014)
            ? " — Vào MT5: Tools > Options > Expert Advisors > Allow WebRequest, thêm: https://script.google.com"
            : " — Lỗi mạng: " + IntegerToString(err);
        Print("RTB: Không kết nối được server bản quyền", hint);
        return false;
    }

    string response = CharArrayToString(result);
    if(StringFind(response, "\"status\":\"ok\"") >= 0) {
        Print("RTB: License OK — Account ", accID);
        return true;
    }

    // Extract reason from JSON for clearer log
    string reason = "denied";
    int rPos = StringFind(response, "\"reason\":\"");
    if(rPos >= 0) {
        int rStart = rPos + 10;
        int rEnd   = StringFind(response, "\"", rStart);
        if(rEnd > rStart) reason = StringSubstr(response, rStart, rEnd - rStart);
    }
    Print("RTB: License DENIED — Account ", accID, " | Lý do: ", reason);
    Alert("RTB: Bản quyền không hợp lệ. Liên hệ nhà cung cấp.");
    return false;
}

//+------------------------------------------------------------------+
//| REBUILD DCA STATE FROM DEAL HISTORY (after restart)             |
//+------------------------------------------------------------------+
void RebuildDCAState(int posType) {
    // Find oldest managed position open time — marks start of current session
    datetime sessionStart = (datetime)0x7FFFFFFF;
    for(int i = 0; i < PositionsTotal(); i++) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        datetime t = (datetime)PositionGetInteger(POSITION_TIME);
        if(t < sessionStart) sessionStart = t;
    }
    if(sessionStart == (datetime)0x7FFFFFFF) return;

    datetime slotTimes[60];
    double   slotPrices[60];
    ulong    slotPosIds[60];
    int      count = 0;

    if(HistorySelect(sessionStart, TimeCurrent())) {
        for(int i = 0; i < HistoryDealsTotal() && count < 60; i++) {
            ulong dk = HistoryDealGetTicket(i);
            if(HistoryDealGetString(dk, DEAL_SYMBOL) != _Symbol) continue;
            if((long)HistoryDealGetInteger(dk, DEAL_MAGIC) != (long)InpMagic) continue;
            ENUM_DEAL_ENTRY de = (ENUM_DEAL_ENTRY)HistoryDealGetInteger(dk, DEAL_ENTRY);
            if(de != DEAL_ENTRY_IN) continue;
            ENUM_DEAL_TYPE dt = (ENUM_DEAL_TYPE)HistoryDealGetInteger(dk, DEAL_TYPE);
            if(posType == POSITION_TYPE_BUY  && dt != DEAL_TYPE_BUY)  continue;
            if(posType == POSITION_TYPE_SELL && dt != DEAL_TYPE_SELL) continue;
            string cmt = HistoryDealGetString(dk, DEAL_COMMENT);
            if(StringFind(cmt, "RTB|") != 0) continue;
            // Bỏ qua lệnh re-fill (|RF) — chúng không phải slot mới, tránh inflate peak
            if(StringFind(cmt, "|RF") >= 0) continue;
            string parts[];
            int np = StringSplit(cmt, '|', parts);
            // Bỏ qua lệnh gốc "RTB|0|0" (đúng 3 phần, cả hai bằng 0)
            if(np == 3 && parts[1] == "0" && parts[2] == "0") continue;
            slotTimes[count]  = (datetime)HistoryDealGetInteger(dk, DEAL_TIME);
            slotPrices[count] = HistoryDealGetDouble(dk, DEAL_PRICE);
            slotPosIds[count] = (ulong)HistoryDealGetInteger(dk, DEAL_POSITION_ID);
            count++;
        }
    }

    // Sắp xếp theo thời gian mở (insertion sort)
    for(int i = 1; i < count; i++) {
        datetime kt = slotTimes[i]; double kp = slotPrices[i]; ulong ki = slotPosIds[i];
        int j = i - 1;
        while(j >= 0 && slotTimes[j] > kt) {
            slotTimes[j+1] = slotTimes[j]; slotPrices[j+1] = slotPrices[j]; slotPosIds[j+1] = slotPosIds[j];
            j--;
        }
        slotTimes[j+1] = kt; slotPrices[j+1] = kp; slotPosIds[j+1] = ki;
    }

    // Build lookup of open managed positions for matching re-fills by price
    ulong openTks[60];
    double openPrices[60];
    int openCount = 0;
    for(int i = PositionsTotal()-1; i >= 0 && openCount < 60; i--) {
        ulong tk = PositionGetTicket(i);
        if(!PositionSelectByTicket(tk)) continue;
        if(PositionGetString(POSITION_SYMBOL) != _Symbol) continue;
        if((long)PositionGetInteger(POSITION_MAGIC) != (long)InpMagic) continue;
        if((int)PositionGetInteger(POSITION_TYPE) != posType) continue;
        openTks[openCount]    = tk;
        openPrices[openCount] = PositionGetDouble(POSITION_PRICE_OPEN);
        openCount++;
    }

    double priceStep = _Point * 5; // tolerance for price match

    if(posType == POSITION_TYPE_BUY) {
        PeakDCABuy = count;
        for(int s = 0; s < count && s < 60; s++) {
            DCABuyPrices[s] = slotPrices[s];
            // Try original position ticket first
            DCABuyTickets[s] = PositionSelectByTicket(slotPosIds[s]) ? slotPosIds[s] : 0;
            // Fallback: find open position at same price (re-fill already filled)
            if(DCABuyTickets[s] == 0) {
                for(int p = 0; p < openCount; p++) {
                    if(openTks[p] == 0) continue;
                    if(MathAbs(openPrices[p] - slotPrices[s]) < priceStep) {
                        DCABuyTickets[s] = openTks[p];
                        openTks[p] = 0; // mark used — prevent assigning same position to two slots
                        break;
                    }
                }
            } else {
                // Mark original position as used so price-match doesn't reassign it
                for(int p = 0; p < openCount; p++) {
                    if(openTks[p] == DCABuyTickets[s]) { openTks[p] = 0; break; }
                }
            }
        }
    } else {
        PeakDCASell = count;
        for(int s = 0; s < count && s < 60; s++) {
            DCASellPrices[s] = slotPrices[s];
            DCASellTickets[s] = PositionSelectByTicket(slotPosIds[s]) ? slotPosIds[s] : 0;
            if(DCASellTickets[s] == 0) {
                for(int p = 0; p < openCount; p++) {
                    if(openTks[p] == 0) continue;
                    if(MathAbs(openPrices[p] - slotPrices[s]) < priceStep) {
                        DCASellTickets[s] = openTks[p];
                        openTks[p] = 0;
                        break;
                    }
                }
            } else {
                for(int p = 0; p < openCount; p++) {
                    if(openTks[p] == DCASellTickets[s]) { openTks[p] = 0; break; }
                }
            }
        }
    }
    Print("RTB: RebuildDCAState ", (posType==POSITION_TYPE_BUY?"BUY":"SELL"), " peak=", count);
}

//+------------------------------------------------------------------+
//| EVENT HANDLERS                                                   |
//+------------------------------------------------------------------+
int OnInit() {
    if(!CheckLicense()) return INIT_FAILED;

    Trade.SetExpertMagicNumber(InpMagic);
    Trade.SetDeviationInPoints(50);
    Trade.SetTypeFilling(ORDER_FILLING_RETURN);

    InitDCA();
    InitPyra();

    // Create indicator handles
    hEMAFast = iMA(_Symbol, InpSignalTF, InpEMAFast, 0, MODE_EMA, PRICE_CLOSE);
    hEMASlow = iMA(_Symbol, InpSignalTF, InpEMASlow, 0, MODE_EMA, PRICE_CLOSE);
    hBB      = iBands(_Symbol, InpSignalTF, InpBBPeriod, 0, InpBBDev, PRICE_CLOSE);
    hIchi    = iIchimoku(_Symbol, InpSignalTF, InpIchiTenkan, InpIchiKijun, InpIchiSenkou);
    hATR     = iATR(_Symbol, InpSignalTF, InpUTATRPeriod);

    if(hEMAFast == INVALID_HANDLE || hEMASlow == INVALID_HANDLE ||
       hBB == INVALID_HANDLE      || hIchi    == INVALID_HANDLE ||
       hATR == INVALID_HANDLE) {
        Print("RTB: ERROR — failed to create indicator handles!");
        return INIT_FAILED;
    }

    WarmupATS(1000);

    InitBalance    = AccountInfoDouble(ACCOUNT_BALANCE);
    MaxDrawdownPct = 0;
    TrailBuy       = 0;
    TrailSell      = 0;
    ArrayInitialize(DCABuyPrices,   0);
    ArrayInitialize(DCASellPrices,  0);
    ArrayInitialize(DCABuyBounced,  false);
    ArrayInitialize(DCASellBounced, false);
    ArrayInitialize(DCABuyTickets,  0);
    ArrayInitialize(DCASellTickets, 0);
    ArrayInitialize(DCABuyLimitTk,  0);
    ArrayInitialize(DCASellLimitTk, 0);
    // Cancel stale pending DCA re-fill orders (Limit & Stop) from before restart
    for(int i = OrdersTotal()-1; i >= 0; i--) {
        ulong tk = OrderGetTicket(i);
        if(!OrderSelect(tk)) continue;
        if(OrderGetString(ORDER_SYMBOL) != _Symbol) continue;
        if(OrderGetInteger(ORDER_MAGIC) != (long)InpMagic) continue;
        string ocmt = OrderGetString(ORDER_COMMENT);
        if(StringFind(ocmt, "|RF") < 0) continue; // chỉ cancel re-fill orders (có |RF suffix)
        ENUM_ORDER_TYPE ot = (ENUM_ORDER_TYPE)OrderGetInteger(ORDER_TYPE);
        if(ot == ORDER_TYPE_BUY_LIMIT || ot == ORDER_TYPE_SELL_LIMIT ||
           ot == ORDER_TYPE_BUY_STOP  || ot == ORDER_TYPE_SELL_STOP)
            Trade.OrderDelete(tk);
    }
    // Phục hồi trạng thái DCA từ deal history sau khi restart
    PeakDCABuy  = 0;
    PeakDCASell = 0;
    RebuildDCAState(POSITION_TYPE_BUY);
    RebuildDCAState(POSITION_TYPE_SELL);
    LastEntryTime  = 0;
    LastDay        = -1;

    HedgeCutBuy        = false;
    HedgeCutSell       = false;
    HedgeInitBuyPrice  = 0.0;
    HedgeInitSellPrice = 0.0;
    HedgeTrendSide     = -1;
    if(InpHedgeEnable) {
        // Khôi phục sau restart: nếu chỉ còn một chiều → chiều kia đã bị cắt trước đó
        if(CountBuy() > 0 && CountSell() == 0) { HedgeCutSell = true; Print("RTB: Hedge restart — infer SELL was cut"); }
        if(CountSell() > 0 && CountBuy() == 0) { HedgeCutBuy  = true; Print("RTB: Hedge restart — infer BUY was cut"); }
        // Khôi phục trend side từ pyramiding orders còn tồn tại
        if(CountPyra(POSITION_TYPE_BUY) > 0)       { HedgeTrendSide = POSITION_TYPE_BUY;  Print("RTB: Hedge restart — trend=BUY"); }
        else if(CountPyra(POSITION_TYPE_SELL) > 0) { HedgeTrendSide = POSITION_TYPE_SELL; Print("RTB: Hedge restart — trend=SELL"); }
    }

    EventSetTimer(1);
    Print("RTB: Initialized. Magic=", InpMagic, " Signal=", EnumToString(InpSignalMode));
    return INIT_SUCCEEDED;
}

void OnDeinit(const int reason) {
    EventKillTimer();
    RemoveGUI();
    IndicatorRelease(hEMAFast);
    IndicatorRelease(hEMASlow);
    IndicatorRelease(hBB);
    IndicatorRelease(hIchi);
    IndicatorRelease(hATR);
}

void OnTick() {
    // Entry signals only fire when no position exists for that direction
    CheckEntry();
    // Stealth TP/SL check runs on every tick for precision
    if(InpStealthMode) CheckExit();
    // Trailing: chạy mỗi tick để server SL và đường line di chuyển ngay theo giá, không trễ 1 giây
    if(!DayLimitHit) CheckTrailing();
}

void OnTimer() {
    UpdateDayProfit();
    CheckDayLimit();

    if(CountBuy()  == 0) ResetDCAState(POSITION_TYPE_BUY);
    if(CountSell() == 0) ResetDCAState(POSITION_TYPE_SELL);

    // Exit checks (basket close conditions)
    if(!InpStealthMode) CheckExit();

    // Hedge: cắt chiều âm (chạy cả khi DayLimitHit để bảo vệ tài khoản)
    CheckHedgeCut();

    if(!DayLimitHit) {
        // Trimming
        CheckTrimming();

        // DCA scale-in
        if(CountBuy()  > 0) CheckDCA(POSITION_TYPE_BUY);
        if(CountSell() > 0) CheckDCA(POSITION_TYPE_SELL);

        // Pyramiding (add to winners)
        if(CountBuy()  > 0) CheckPyramiding(POSITION_TYPE_BUY);
        if(CountSell() > 0) CheckPyramiding(POSITION_TYPE_SELL);
    }

    // GUI refresh every second
    UpdateGUI();
}

void OnTradeTransaction(const MqlTradeTransaction& trans,
                        const MqlTradeRequest&     req,
                        const MqlTradeResult&      res) {
    // Cập nhật Day P/L ngay khi có deal đóng, không chờ timer 1 giây
    if(trans.type == TRADE_TRANSACTION_DEAL_ADD) {
        UpdateDayProfit();
        CheckDayLimit();
        UpdateGUI();
    }
}

void OnChartEvent(const int id, const long& lparam, const double& dparam, const string& sparam) {
    if(id != CHARTEVENT_OBJECT_CLICK) return;
    if     (sparam == GUI + "BtnCloseAll")    CloseAll();
    else if(sparam == GUI + "BtnCloseBuy")    CloseAll(POSITION_TYPE_BUY);
    else if(sparam == GUI + "BtnCloseSell")   CloseAll(POSITION_TYPE_SELL);
    else if(sparam == GUI + "BtnCloseProfit") CloseAllProfit();
    else if(sparam == GUI + "BtnCloseLoss")   CloseAllLoss();
    else if(sparam == GUI + "BtnOpenBuy")  { if(!DayLimitHit) OpenOrder(ORDER_TYPE_BUY,  InpLotSize, InpTP_Points, InpSL_Points); }
    else if(sparam == GUI + "BtnOpenSell") { if(!DayLimitHit) OpenOrder(ORDER_TYPE_SELL, InpLotSize, InpTP_Points, InpSL_Points); }
    else return;
    ObjectSetInteger(0, sparam, OBJPROP_STATE, false);
    ChartRedraw(0);
}