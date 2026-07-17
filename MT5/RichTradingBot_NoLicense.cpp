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
#include <Canvas\Canvas.mqh>

#define RTB_PI 3.14159265358979323846

CTrade    Trade;
CCanvas   g_TechCanvas;

//+------------------------------------------------------------------+
//| ENUMS                                                            |
//+------------------------------------------------------------------+
enum ENUM_SIGNAL_MODE  { SIG_EMA, SIG_BZ_ZONE, SIG_ICHIMOKU, SIG_BB, SIG_SIMULATED, SIG_UT_BOT };
enum ENUM_DIRECTION    { DIR_BOTH, DIR_ONLY_BUY, DIR_ONLY_SELL };
enum ENUM_DCA_MODE     { DCA_STOP, DCA_STEP, DCA_STEP_TF };
enum ENUM_TRAIL_MODE   { TRAIL_BASKET, TRAIL_SINGLE };
enum ENUM_BOT_MODE     { MODE_AUTO, MODE_SEMI_AUTO };
enum ENUM_TRIM_MODE    { TRIM_OFF, TRIM_TARGET, TRIM_PARTIAL_DD, TRIM_DAY_PROFIT, TRIM_HEDGE, TRIM_HEDGE_PTS };

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
input  ENUM_TRIM_MODE InpTrimMode    = TRIM_OFF; // Chế độ (Off/Target/PartialDD/DayProfit/Hedge/Hedge theo điểm)
input  int     InpTrimTrigger    = 5;      // Kích hoạt khi số lệnh >= X
input  double  InpTrimTarget     = 10.0;   // [Target/Hedge/HedgePts] Mục tiêu lợi nhuận sau tỉa ($)
input  double  InpPartialTrimDD  = 20.0;   // [Partial DD] Kích hoạt khi DD% >
input  int     InpTrimMaxLoss    = 1;      // Số lệnh âm tối đa gộp mỗi lần ghép cặp (Hedge) / đóng mỗi lượt (mode khác)
input  int     InpTrimMaxWin     = 1;      // [Hedge] Số lệnh dương tối đa gộp mỗi lần ghép cặp
input  int     InpTrimMaxCycles  = 1;      // [Hedge] Số chu kỳ ghép cặp tối đa mỗi lượt tỉa

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
//| INPUT: TECHNICALS (GAUGE)                                        |
//+------------------------------------------------------------------+
input group         "══════ TECHNICALS (GAUGE) ══════"; //
input  ENUM_TIMEFRAMES InpTechTF = PERIOD_H1; // Khung thời gian tính Technicals

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
int      hEMAFast   = INVALID_HANDLE;
int      hEMASlow   = INVALID_HANDLE;
int      hBB        = INVALID_HANDLE;
int      hIchi      = INVALID_HANDLE;
int      hATR       = INVALID_HANDLE;

int TechMAPeriods[6] = {10, 20, 30, 50, 100, 200};
int hTechMASMA[6];
int hTechMAEMA[6];
int hTechIchi  = INVALID_HANDLE;
int hTechRSI   = INVALID_HANDLE;
int hTechStoch = INVALID_HANDLE;
int hTechCCI   = INVALID_HANDLE;
int hTechADX   = INVALID_HANDLE;
int hTechAO    = INVALID_HANDLE;
int hTechMom   = INVALID_HANDLE;
int hTechMACD  = INVALID_HANDLE;
int hTechWPR   = INVALID_HANDLE;
int hTechBulls = INVALID_HANDLE;
int hTechBears = INVALID_HANDLE;

double   g_TechRating    = 0.0;
string   g_TechLabel     = "Neutral";
int      g_CalRightEdge  = 0;
bool     g_TechCanvasReady = false;
double   g_ats_ut        = 0.0;
int      g_ats_ut_signal = 0;
datetime g_last_bar_ut   = 0;

ENUM_DCA_MODE DCA_Mode[15];
double        DCA_Mult[15];
int           DCA_MaxOrd[15];
double        DCA_Dist[15];
double        DCA_TP[15];
double        DCA_SL[15];

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

bool   HedgeCutBuy        = false;
bool   HedgeCutSell       = false;
double HedgeInitBuyPrice  = 0.0;
double HedgeInitSellPrice = 0.0;
int    HedgeTrendSide     = -1;

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

bool   g_BotEnabled      = true;
datetime g_LastBotToggleClick = 0;

ENUM_SIGNAL_MODE g_SignalMode;
ENUM_DIRECTION   g_Direction;
int              g_UTKeyValue;

bool   g_UseTakeProfit, g_UseStopLoss, g_StealthMode;
int    g_OrderDelay;
double g_TP_Points, g_SL_Points;

bool   g_DCABuyEnable, g_DCASellEnable, g_DCAArithEnable;
double g_DCAArithStep;
bool   g_PyraBuyEnable, g_PyraSellEnable;

ENUM_TRIM_MODE g_TrimMode;
int    g_TrimTrigger, g_TrimMaxLoss, g_TrimMaxWin, g_TrimMaxCycles;
double g_TrimTarget, g_PartialTrimDD;

bool             g_TrailEnable;
ENUM_TRAIL_MODE  g_TrailMode;
int              g_TrailMinOrds;
double           g_TrailActivate, g_TrailStep, g_TrailInit;

double g_CloseProfit, g_CloseLoss, g_ClosePerPips, g_DayMaxLoss, g_DayMaxProfit;

bool   g_HedgeEnable;
double g_HedgeCutPts;

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
    }
    if(posType < 0 || posType == POSITION_TYPE_SELL) {
        TrailSell = 0; PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
        OrigSellPrice = 0;
    }
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

double DCAOrderLot(double baseLot, int orderIdx1, int lvl) {
    if(g_DCAArithEnable)
        return NormLot(baseLot + orderIdx1 * g_DCAArithStep);
    return NormLot(baseLot * DCA_Mult[lvl]);
}

//+------------------------------------------------------------------+
//| OPEN ORDER                                                       |
//+------------------------------------------------------------------+
bool OpenOrder(int ordType, double lot, double tp_pts = 0, double sl_pts = 0,
               bool isDCA = false, bool isPyra = false) {
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);

    double price, tp = 0, sl = 0;

    bool autoExit = (isDCA || isPyra);
    bool applyTP  = autoExit ? (tp_pts > 0) : (g_UseTakeProfit && tp_pts > 0);
    bool applySL  = autoExit ? (sl_pts > 0) : (g_UseStopLoss   && sl_pts > 0);

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
    if(isPyra) {
        if(tp_pts == 0 && sl_pts == 0)
            comment = "RTP|0|0|P";
        else
            comment = StringFormat("RTP|%.0f|%.0f", tp_pts, sl_pts);
    } else if(isDCA) {
        if(tp_pts == 0 && sl_pts == 0)
            comment = "RTB|0|0|D";
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

int SignalEMA() {
    double fast[], slow[];
    ArraySetAsSeries(fast, true);
    ArraySetAsSeries(slow, true);
    if(CopyBuffer(hEMAFast, 0, 0, 3, fast) < 3) return 0;
    if(CopyBuffer(hEMASlow, 0, 0, 3, slow) < 3) return 0;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double price = (double)iClose(_Symbol, InpSignalTF, 0);

    bool crossUp   = fast[2] < slow[2] && fast[1] > slow[1];
    bool crossDown = fast[2] > slow[2] && fast[1] < slow[1];

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
    if(CopyRates(_Symbol, InpSignalTF, 1, 3, r) < 3) return 0;

    bool allGreen = r[0].close > r[0].open && r[1].close > r[1].open && r[2].close > r[2].open;
    bool allRed   = r[0].close < r[0].open && r[1].close < r[1].open && r[2].close < r[2].open;

    if(allGreen) return  1;
    if(allRed)   return -1;
    return 0;
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
    double pastPrice= (double)iClose(_Symbol, InpSignalTF, InpIchiKijun);
    double kumoTop  = MathMax(spanA[0], spanB[0]);
    double kumoBot  = MathMin(spanA[0], spanB[0]);

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

    if(closeBar1 <= lower[1]) return  1;
    if(closeBar1 >= upper[1]) return -1;
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
        case SIG_EMA:       sig = SignalEMA();       break;
        case SIG_BZ_ZONE:   sig = SignalBZZone();    break;
        case SIG_ICHIMOKU:  sig = SignalIchimoku();  break;
        case SIG_BB:        sig = SignalBB();        break;
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
//| TECHNICALS RATING — đồng hồ Strong Sell..Strong Buy kiểu TradingView |
//+------------------------------------------------------------------+

double LWMA(const double &price[], int shift, int period) {
    double sum = 0, wsum = 0;
    for(int i = 0; i < period; i++) {
        double w = period - i;
        sum  += price[shift + i] * w;
        wsum += w;
    }
    return (wsum > 0) ? sum / wsum : 0;
}

double HullMA(const double &price[], int shift, int period) {
    int half   = period / 2;
    int sqrtN  = (int)MathRound(MathSqrt(period));
    if(sqrtN < 1) sqrtN = 1;
    double raw[];
    ArrayResize(raw, sqrtN);
    for(int s = 0; s < sqrtN; s++)
        raw[s] = 2.0 * LWMA(price, shift + s, half) - LWMA(price, shift + s, period);
    return LWMA(raw, 0, sqrtN);
}

double VWMA(int period, ENUM_TIMEFRAMES tf) {
    double close[]; long vol[];
    ArraySetAsSeries(close, true);
    ArraySetAsSeries(vol, true);
    if(CopyClose(_Symbol, tf, 0, period, close) < period) return 0;
    if(CopyTickVolume(_Symbol, tf, 0, period, vol) < period) return 0;
    double num = 0, den = 0;
    for(int i = 0; i < period; i++) { num += close[i] * (double)vol[i]; den += (double)vol[i]; }
    return (den > 0) ? num / den : 0;
}

double UltimateOscillator(ENUM_TIMEFRAMES tf) {
    int need = 29;
    double high[], low[], close[];
    ArraySetAsSeries(high, true); ArraySetAsSeries(low, true); ArraySetAsSeries(close, true);
    if(CopyHigh(_Symbol, tf, 0, need, high)   < need) return 50;
    if(CopyLow(_Symbol, tf, 0, need, low)     < need) return 50;
    if(CopyClose(_Symbol, tf, 0, need, close) < need) return 50;

    double bp[], tr[];
    ArrayResize(bp, need - 1);
    ArrayResize(tr, need - 1);
    for(int i = 0; i < need - 1; i++) {
        double priorClose = close[i + 1];
        bp[i] = close[i] - MathMin(low[i], priorClose);
        tr[i] = MathMax(high[i], priorClose) - MathMin(low[i], priorClose);
    }
    double sumBP7 = 0, sumTR7 = 0, sumBP14 = 0, sumTR14 = 0, sumBP28 = 0, sumTR28 = 0;
    for(int i = 0; i < 28; i++) {
        sumBP28 += bp[i]; sumTR28 += tr[i];
        if(i < 14) { sumBP14 += bp[i]; sumTR14 += tr[i]; }
        if(i < 7)  { sumBP7  += bp[i]; sumTR7  += tr[i]; }
    }
    double avg7  = (sumTR7  > 0) ? sumBP7  / sumTR7  : 0;
    double avg14 = (sumTR14 > 0) ? sumBP14 / sumTR14 : 0;
    double avg28 = (sumTR28 > 0) ? sumBP28 / sumTR28 : 0;
    return 100.0 * (4.0 * avg7 + 2.0 * avg14 + avg28) / 7.0;
}

void StochRSICalc(double &kOut, double &dOut) {
    int rsiLen = 14, smoothK = 3, smoothD = 3;
    int rawCount = smoothK + smoothD;
    int need = rsiLen + rawCount;
    double rsi[];
    ArraySetAsSeries(rsi, true);
    if(CopyBuffer(hTechRSI, 0, 0, need, rsi) < need) { kOut = 50; dOut = 50; return; }

    double rawK[];
    ArrayResize(rawK, rawCount);
    for(int i = 0; i < rawCount; i++) {
        double hi = rsi[i], lo = rsi[i];
        for(int j = 0; j < rsiLen; j++) {
            hi = MathMax(hi, rsi[i + j]);
            lo = MathMin(lo, rsi[i + j]);
        }
        rawK[i] = (hi - lo > 0) ? (rsi[i] - lo) / (hi - lo) * 100.0 : 0;
    }

    double kArr[];
    ArrayResize(kArr, smoothD);
    for(int i = 0; i < smoothD; i++) {
        double s = 0;
        for(int j = 0; j < smoothK; j++) s += rawK[i + j];
        kArr[i] = s / smoothK;
    }
    kOut = kArr[0];
    double s2 = 0;
    for(int i = 0; i < smoothD; i++) s2 += kArr[i];
    dOut = s2 / smoothD;
}

int VoteMA(double maVal, double price) {
    if(price > maVal) return 1;
    if(price < maVal) return -1;
    return 0;
}

int VoteIchimoku() {
    double tenkan[], kijun[], spanA[], spanB[];
    ArraySetAsSeries(tenkan, true); ArraySetAsSeries(kijun, true);
    ArraySetAsSeries(spanA, true);  ArraySetAsSeries(spanB, true);
    if(CopyBuffer(hTechIchi, 0, 0, 1, tenkan) < 1) return 0;
    if(CopyBuffer(hTechIchi, 1, 0, 1, kijun)  < 1) return 0;
    if(CopyBuffer(hTechIchi, 2, 0, 1, spanA)  < 1) return 0;
    if(CopyBuffer(hTechIchi, 3, 0, 1, spanB)  < 1) return 0;
    double price = iClose(_Symbol, InpTechTF, 0);
    bool buyOK  = spanA[0] > spanB[0] && kijun[0] > spanA[0] && tenkan[0] > kijun[0] && price > tenkan[0];
    bool sellOK = spanA[0] < spanB[0] && kijun[0] < spanA[0] && tenkan[0] < kijun[0] && price < tenkan[0];
    if(buyOK)  return 1;
    if(sellOK) return -1;
    return 0;
}

int VoteRSI() {
    double buf[]; ArraySetAsSeries(buf, true);
    if(CopyBuffer(hTechRSI, 0, 0, 2, buf) < 2) return 0;
    if(buf[0] < 30 && buf[0] > buf[1]) return 1;
    if(buf[0] > 70 && buf[0] < buf[1]) return -1;
    return 0;
}

int VoteStoch() {
    double k[], d[];
    ArraySetAsSeries(k, true); ArraySetAsSeries(d, true);
    if(CopyBuffer(hTechStoch, 0, 0, 1, k) < 1) return 0;
    if(CopyBuffer(hTechStoch, 1, 0, 1, d) < 1) return 0;
    if(k[0] < 20 && d[0] < 20 && k[0] > d[0]) return 1;
    if(k[0] > 80 && d[0] > 80 && k[0] < d[0]) return -1;
    return 0;
}

int VoteCCI() {
    double buf[]; ArraySetAsSeries(buf, true);
    if(CopyBuffer(hTechCCI, 0, 0, 2, buf) < 2) return 0;
    if(buf[0] < -100 && buf[0] > buf[1]) return 1;
    if(buf[0] >  100 && buf[0] < buf[1]) return -1;
    return 0;
}

int VoteADX() {
    double adx[], plus[], minus[];
    ArraySetAsSeries(adx, true); ArraySetAsSeries(plus, true); ArraySetAsSeries(minus, true);
    if(CopyBuffer(hTechADX, 0, 0, 2, adx)   < 2) return 0;
    if(CopyBuffer(hTechADX, 1, 0, 1, plus)  < 1) return 0;
    if(CopyBuffer(hTechADX, 2, 0, 1, minus) < 1) return 0;
    if(plus[0] > minus[0] && adx[0] > 20 && adx[0] > adx[1]) return 1;
    if(plus[0] < minus[0] && adx[0] > 20 && adx[0] < adx[1]) return -1;
    return 0;
}

int VoteAO() {
    double ao[]; ArraySetAsSeries(ao, true);
    if(CopyBuffer(hTechAO, 0, 0, 3, ao) < 3) return 0;
    bool crossUp     = ao[1] <= 0 && ao[0] > 0;
    bool crossDown   = ao[1] >= 0 && ao[0] < 0;
    bool saucerUp    = ao[2] > 0 && ao[1] > 0 && ao[0] > 0 && ao[1] < ao[2] && ao[0] > ao[1];
    bool saucerDown  = ao[2] < 0 && ao[1] < 0 && ao[0] < 0 && ao[1] > ao[2] && ao[0] < ao[1];
    if(crossUp   || saucerUp)   return 1;
    if(crossDown || saucerDown) return -1;
    return 0;
}

int VoteMomentum() {
    double buf[]; ArraySetAsSeries(buf, true);
    if(CopyBuffer(hTechMom, 0, 0, 2, buf) < 2) return 0;
    if(buf[0] > buf[1]) return 1;
    if(buf[0] < buf[1]) return -1;
    return 0;
}

int VoteMACD() {
    double macd[], sig[];
    ArraySetAsSeries(macd, true); ArraySetAsSeries(sig, true);
    if(CopyBuffer(hTechMACD, 0, 0, 1, macd) < 1) return 0;
    if(CopyBuffer(hTechMACD, 1, 0, 1, sig)  < 1) return 0;
    if(macd[0] > sig[0]) return 1;
    if(macd[0] < sig[0]) return -1;
    return 0;
}

bool TechUptrend() {
    double sma[]; ArraySetAsSeries(sma, true);
    if(CopyBuffer(hTechMASMA[3], 0, 0, 1, sma) < 1) return true;
    double price = iClose(_Symbol, InpTechTF, 0);
    return price > sma[0];
}

int VoteStochRSI() {
    double k, d;
    StochRSICalc(k, d);
    bool uptrend = TechUptrend();
    if(!uptrend && k < 20 && d < 20 && k > d) return 1;
    if(uptrend  && k > 80 && d > 80 && k < d) return -1;
    return 0;
}

int VoteWPR() {
    double buf[]; ArraySetAsSeries(buf, true);
    if(CopyBuffer(hTechWPR, 0, 0, 2, buf) < 2) return 0;
    if(buf[0] < -80 && buf[0] > buf[1]) return 1;
    if(buf[0] > -20 && buf[0] < buf[1]) return -1;
    return 0;
}

int VoteBullBearPower() {
    double bull[], bear[];
    ArraySetAsSeries(bull, true); ArraySetAsSeries(bear, true);
    if(CopyBuffer(hTechBulls, 0, 0, 2, bull) < 2) return 0;
    if(CopyBuffer(hTechBears, 0, 0, 2, bear) < 2) return 0;
    bool uptrend = TechUptrend();
    if(uptrend  && bear[0] < 0 && bear[0] > bear[1]) return 1;
    if(!uptrend && bull[0] > 0 && bull[0] < bull[1]) return -1;
    return 0;
}

int VoteUltimateOsc() {
    double uo = UltimateOscillator(InpTechTF);
    if(uo > 70) return 1;
    if(uo < 30) return -1;
    return 0;
}

string TechRatingLabel(double score) {
    if(score < -0.5) return "Strong Sell";
    if(score < -0.1) return "Sell";
    if(score <=  0.1) return "Neutral";
    if(score <=  0.5) return "Buy";
    return "Strong Buy";
}

double GetTechnicalRatingScore() {
    double price = iClose(_Symbol, InpTechTF, 0);

    double maSum = 0; int maCount = 0;
    double buf1[]; ArraySetAsSeries(buf1, true);
    for(int i = 0; i < 6; i++) {
        if(CopyBuffer(hTechMASMA[i], 0, 0, 1, buf1) == 1) { maSum += VoteMA(buf1[0], price); maCount++; }
        if(CopyBuffer(hTechMAEMA[i], 0, 0, 1, buf1) == 1) { maSum += VoteMA(buf1[0], price); maCount++; }
    }

    double closeArr[]; ArraySetAsSeries(closeArr, true);
    if(CopyClose(_Symbol, InpTechTF, 0, 30, closeArr) == 30) {
        maSum += VoteMA(HullMA(closeArr, 0, 9), price); maCount++;
        maSum += VoteMA(VWMA(20, InpTechTF), price);    maCount++;
    }

    maSum += VoteIchimoku(); maCount++;
    double maScore = (maCount > 0) ? maSum / maCount : 0;

    double oscSum = 0; int oscCount = 0;
    oscSum += VoteRSI();           oscCount++;
    oscSum += VoteStoch();         oscCount++;
    oscSum += VoteCCI();           oscCount++;
    oscSum += VoteADX();           oscCount++;
    oscSum += VoteAO();            oscCount++;
    oscSum += VoteMomentum();      oscCount++;
    oscSum += VoteMACD();          oscCount++;
    oscSum += VoteStochRSI();      oscCount++;
    oscSum += VoteWPR();           oscCount++;
    oscSum += VoteBullBearPower(); oscCount++;
    oscSum += VoteUltimateOsc();   oscCount++;
    double oscScore = (oscCount > 0) ? oscSum / oscCount : 0;

    return (maScore + oscScore) / 2.0;
}

void UpdateTechnicalRating() {
    g_TechRating = GetTechnicalRatingScore();
    g_TechLabel  = TechRatingLabel(g_TechRating);
}

//+------------------------------------------------------------------+
//| INITIAL ENTRY (OnTick)                                           |
//+------------------------------------------------------------------+
void ResetDCAState(int posType) {
    if(posType == POSITION_TYPE_BUY) {
        for(int i = 0; i < ArraySize(DCABuyLimitTk); i++) {
            if(DCABuyLimitTk[i] > 0 && !Trade.OrderDelete(DCABuyLimitTk[i]))
                Print("RTB: ResetDCAState huỷ lệnh chờ BUY ticket=", DCABuyLimitTk[i], " thất bại, err=", GetLastError());
        }
        TrailBuy = 0; PeakDCABuy = 0;
        ArrayInitialize(DCABuyPrices, 0); ArrayInitialize(DCABuyBounced, false);
        ArrayInitialize(DCABuyTickets, 0); ArrayInitialize(DCABuyLimitTk, 0);
    } else {
        for(int i = 0; i < ArraySize(DCASellLimitTk); i++) {
            if(DCASellLimitTk[i] > 0 && !Trade.OrderDelete(DCASellLimitTk[i]))
                Print("RTB: ResetDCAState huỷ lệnh chờ SELL ticket=", DCASellLimitTk[i], " thất bại, err=", GetLastError());
        }
        TrailSell = 0; PeakDCASell = 0;
        ArrayInitialize(DCASellPrices, 0); ArrayInitialize(DCASellBounced, false);
        ArrayInitialize(DCASellTickets, 0); ArrayInitialize(DCASellLimitTk, 0);
    }
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
    if(g_HedgeEnable && HedgeCutBuy) return;
    if(CountBuy() >= InpMaxBuy) return;
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
    if(g_HedgeEnable && HedgeCutSell) return;
    if(CountSell() >= InpMaxSell) return;
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
//| HEDGE FOLLOW WINNER — CẮT CHIỀU ÂM                              |
//+------------------------------------------------------------------+
void CheckHedgeCut() {
    if(!g_HedgeEnable) return;

    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);

    int cntBuy  = CountBuy();
    int cntSell = CountSell();

    if(cntBuy == 0 && cntSell == 0) {
        if(HedgeCutBuy || HedgeCutSell || HedgeTrendSide >= 0) {
            Print("RTB: Hedge — reset state (không còn lệnh)");
            HedgeCutBuy = false; HedgeCutSell = false;
            HedgeTrendSide = -1;
        }
        HedgeInitBuyPrice = 0.0; HedgeInitSellPrice = 0.0;
        return;
    }

    if(cntBuy  > 0) HedgeInitBuyPrice  = FirstOpenPrice(POSITION_TYPE_BUY);
    if(cntSell > 0) HedgeInitSellPrice = FirstOpenPrice(POSITION_TYPE_SELL);

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

    if(!HedgeCutBuy && HedgeInitBuyPrice > 0 && cntBuy > 0) {
        if((HedgeInitBuyPrice - bid) >= g_HedgeCutPts * point) {
            Print("RTB: Hedge — CẮT tất cả | BUY initPrice=", HedgeInitBuyPrice,
                  " bid=", bid, " loss=", (HedgeInitBuyPrice - bid) / point, "pts");
            CloseAll();
            HedgeCutBuy  = true;  HedgeInitBuyPrice  = 0.0;
            HedgeCutSell = true;  HedgeInitSellPrice = 0.0;
        }
    }

    if(!HedgeCutSell && HedgeInitSellPrice > 0 && cntSell > 0) {
        if((ask - HedgeInitSellPrice) >= g_HedgeCutPts * point) {
            Print("RTB: Hedge — CẮT tất cả | SELL initPrice=", HedgeInitSellPrice,
                  " ask=", ask, " loss=", (ask - HedgeInitSellPrice) / point, "pts");
            CloseAll();
            HedgeCutBuy  = true;  HedgeInitBuyPrice  = 0.0;
            HedgeCutSell = true;  HedgeInitSellPrice = 0.0;
        }
    }
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
    if(g_HedgeEnable) return;
    if(posType == POSITION_TYPE_BUY  && !g_DCABuyEnable)  return;
    if(posType == POSITION_TYPE_SELL && !g_DCASellEnable) return;

    int count = CountPos(posType);
    if(count == 0) return;

    double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
    double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
    double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
    int maxOrds  = (posType == POSITION_TYPE_BUY) ? InpMaxBuy : InpMaxSell;

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
        if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

        double baseLot = OldestManualLot(posType);
        if(baseLot <= 0) baseLot = InpLotSize;
        double lot = DCAOrderLot(baseLot, dcaCount + 1, lvl);
        int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
        Print("RTB: DCA level ", lvl+1, " triggered dcaCount=", dcaCount, " [primary chain only]");
        OpenOrder(ord, lot, DCA_TP[lvl], DCA_SL[lvl], true);
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
            int slotLvl = -1, cumS = 0;
            for(int i = 0; i < 15; i++) {
                int ncS = cumS + DCA_MaxOrd[i];
                if(slot < ncS) { slotLvl = i; break; }
                cumS = ncS;
            }


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
                if(slotLvl < 0 || DCA_Mode[slotLvl] == DCA_STOP) continue;

                if(DCA_Mode[slotLvl] == DCA_STEP_TF) {
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

                double lot = DCAOrderLot(InpLotSize, slot + 1, slotLvl);
                string cmt = (DCA_TP[slotLvl] == 0 && DCA_SL[slotLvl] == 0)
                    ? "RTB|0|0|D|RF"
                    : "RTB|" + IntegerToString((int)DCA_TP[slotLvl]) + "|" + IntegerToString((int)DCA_SL[slotLvl]) + "|RF";
                bool ok = false;
                if(posType == POSITION_TYPE_BUY) {
                    double tp_p = DCA_TP[slotLvl] > 0 ? NormalizeDouble(slotPrice + DCA_TP[slotLvl] * point, _Digits) : 0;
                    double sl_p = DCA_SL[slotLvl] > 0 ? NormalizeDouble(slotPrice - DCA_SL[slotLvl] * point, _Digits) : 0;
                    if(ask > slotPrice)
                        ok = Trade.BuyLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(ask < slotPrice)
                        ok = Trade.BuyStop(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                } else {
                    double tp_p = DCA_TP[slotLvl] > 0 ? NormalizeDouble(slotPrice - DCA_TP[slotLvl] * point, _Digits) : 0;
                    double sl_p = DCA_SL[slotLvl] > 0 ? NormalizeDouble(slotPrice + DCA_SL[slotLvl] * point, _Digits) : 0;
                    if(bid < slotPrice)
                        ok = Trade.SellLimit(lot, slotPrice, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
                    else if(bid > slotPrice)
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

    int lvl = -1, cumulative = 0;
    for(int i = 0; i < 15; i++) {
        int nextCum = cumulative + DCA_MaxOrd[i];
        if(peak < nextCum) { lvl = i; break; }
        cumulative = nextCum;
    }
    if(lvl < 0 || DCA_Mode[lvl] == DCA_STOP) return;

    if(DCA_Mode[lvl] == DCA_STEP_TF) {
        int sig = GetSignal();
        if(posType == POSITION_TYPE_BUY  && sig != 1)  return;
        if(posType == POSITION_TYPE_SELL && sig != -1) return;
    }

    double dist   = DCA_Dist[lvl] * point;
    double target = (posType == POSITION_TYPE_BUY) ? NormalizeDouble(lastPrice - dist, _Digits)
                                                    : NormalizeDouble(lastPrice + dist, _Digits);
    ulong  nextTk = (posType == POSITION_TYPE_BUY) ? DCABuyLimitTk[peak] : DCASellLimitTk[peak];
    bool   goMarket = false;

    if(nextTk > 0) {
        if(PositionSelectByTicket(nextTk)) {
            double fillPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            if(posType == POSITION_TYPE_BUY) { DCABuyPrices[peak] = fillPrice; DCABuyTickets[peak] = nextTk; DCABuyLimitTk[peak] = 0; PeakDCABuy++; }
            else                              { DCASellPrices[peak] = fillPrice; DCASellTickets[peak] = nextTk; DCASellLimitTk[peak] = 0; PeakDCASell++; }
            Print("RTB: DCA level ", lvl+1, " khớp bằng lệnh chờ tại ", fillPrice, " peak=", peak);
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
        Print("RTB: DCA level ", lvl+1, " giá gapped qua mục tiêu ", target, " — huỷ lệnh chờ, vào market lấy giá tốt hơn.");
        goMarket = true;
    } else {
        goMarket = (posType == POSITION_TYPE_BUY) ? (ask <= target) : (bid >= target);
    }

    if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

    double lot = DCAOrderLot(InpLotSize, peak + 1, lvl);

    if(goMarket) {
        int ord = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY : ORDER_TYPE_SELL;
        Print("RTB: DCA level ", lvl+1, " vào market. peak=", peak);
        bool ok = OpenOrder(ord, lot, DCA_TP[lvl], DCA_SL[lvl], true);
        ulong newTk = Trade.ResultOrder();
        if(ok && newTk > 0) {
            double fillPrice = 0;
            if(PositionSelectByTicket(newTk))
                fillPrice = PositionGetDouble(POSITION_PRICE_OPEN);
            if(posType == POSITION_TYPE_BUY) { DCABuyPrices[peak] = fillPrice > 0 ? fillPrice : ask; DCABuyTickets[peak] = newTk; PeakDCABuy++; }
            else                              { DCASellPrices[peak] = fillPrice > 0 ? fillPrice : bid; DCASellTickets[peak] = newTk; PeakDCASell++; }
        }
        return;
    }

    string cmt = (DCA_TP[lvl] == 0 && DCA_SL[lvl] == 0)
        ? "RTB|0|0|D"
        : "RTB|" + IntegerToString((int)DCA_TP[lvl]) + "|" + IntegerToString((int)DCA_SL[lvl]);
    bool placed;
    if(posType == POSITION_TYPE_BUY) {
        double tp_p = DCA_TP[lvl] > 0 ? NormalizeDouble(target + DCA_TP[lvl] * point, _Digits) : 0;
        double sl_p = DCA_SL[lvl] > 0 ? NormalizeDouble(target - DCA_SL[lvl] * point, _Digits) : 0;
        placed = Trade.BuyLimit(lot, target, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
    } else {
        double tp_p = DCA_TP[lvl] > 0 ? NormalizeDouble(target - DCA_TP[lvl] * point, _Digits) : 0;
        double sl_p = DCA_SL[lvl] > 0 ? NormalizeDouble(target + DCA_SL[lvl] * point, _Digits) : 0;
        placed = Trade.SellLimit(lot, target, _Symbol, sl_p, tp_p, ORDER_TIME_GTC, 0, cmt);
    }
    if(placed) {
        ulong tk = Trade.ResultOrder();
        if(tk > 0) {
            if(posType == POSITION_TYPE_BUY) DCABuyLimitTk[peak] = tk;
            else                              DCASellLimitTk[peak] = tk;
            LastOrderTime = TimeCurrent();
            Print("RTB: Đặt lệnh chờ tầng ", lvl+1, " tại ", target, " (peak=", peak, ")");
        }
    }
}

void CheckOrigRestart(int posType) {
    if(InpBotMode == MODE_SEMI_AUTO) return;
    if(g_HedgeEnable && ((posType == POSITION_TYPE_BUY && HedgeCutBuy) ||
                          (posType == POSITION_TYPE_SELL && HedgeCutSell))) return;

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
    int maxOrds = (posType == POSITION_TYPE_BUY) ? InpMaxBuy : InpMaxSell;
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
//| PYRAMIDING (NHỒI DƯƠNG)                                          |
//+------------------------------------------------------------------+
void CheckPyramiding(int posType) {
    if(g_HedgeEnable && HedgeTrendSide >= 0 && HedgeTrendSide != posType) return;

    if(posType == POSITION_TYPE_BUY  && !g_PyraBuyEnable)  return;
    if(posType == POSITION_TYPE_SELL && !g_PyraSellEnable) return;

    int count = CountPos(posType);
    if(count == 0) return;

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

    if(TimeCurrent() - LastOrderTime < g_OrderDelay) return;

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

    bool   noTierExit = (PYRA_TP[lvl] == 0 && PYRA_SL[lvl] == 0);
    double openTP     = noTierExit ? g_TP_Points : PYRA_TP[lvl];
    double openSL     = noTierExit ? g_SL_Points : PYRA_SL[lvl];

    Print("RTB: Pyramiding level ", lvl+1, " triggered. pyraCount=", pyraCount);
    if(OpenOrder(ord, lot, openTP, openSL, false, true)) {
        if(g_HedgeEnable && HedgeTrendSide < 0) {
            HedgeTrendSide = posType;
            Print("RTB: Hedge — xu hướng xác định: ", (posType == POSITION_TYPE_BUY ? "BUY" : "SELL"));
        }
    }
}

//+------------------------------------------------------------------+
//| ORDER TRIMMING                                                   |
//+------------------------------------------------------------------+
void CheckTrimming() {
    if(g_TrimMode == TRIM_OFF) return;
    if(CountAll() < g_TrimTrigger) return;

    switch(g_TrimMode) {
    case TRIM_PARTIAL_DD: {
        double balance = AccountInfoDouble(ACCOUNT_BALANCE);
        double equity  = AccountInfoDouble(ACCOUNT_EQUITY);
        if(balance <= 0) return;
        double ddPct = (balance - equity) / balance * 100.0;
        if(ddPct <= g_PartialTrimDD) return;

        int closed = 0;
        for(int n = 0; n < g_TrimMaxLoss; n++) {
            if(CountAll() < g_TrimTrigger) break;
            ulong tk = WorstTicket();
            if(tk == 0) break;
            Trade.PositionClose(tk);
            closed++;
        }
        if(closed > 0) Print("RTB: Partial Trim DD=", ddPct, "% closed=", closed);
        break;
    }

    case TRIM_DAY_PROFIT: {
        int closed = 0;
        for(int n = 0; n < g_TrimMaxLoss; n++) {
            ulong worstTk = WorstTicket();
            if(worstTk == 0 || !PositionSelectByTicket(worstTk)) break;
            double worstP = PositionGetDouble(POSITION_PROFIT);
            if(DayProfit > MathAbs(worstP) && DayProfit > 0) {
                Trade.PositionClose(worstTk);
                closed++;
            } else break;
        }
        if(closed > 0) Print("RTB: Trim by DayProfit=", DayProfit, " closed=", closed);
        break;
    }

    case TRIM_HEDGE: {
        int    totalPos = PositionsTotal();
        ulong  tks[];
        double profits[];
        bool   used[];
        ArrayResize(tks, totalPos);
        ArrayResize(profits, totalPos);
        ArrayResize(used, totalPos);
        int cnt = 0;
        for(int i = totalPos - 1; i >= 0; i--) {
            ulong tk = PositionGetTicket(i);
            if(!PositionSelectByTicket(tk)) continue;
            if(!IsManaged()) continue;
            tks[cnt]     = tk;
            profits[cnt] = PositionGetDouble(POSITION_PROFIT);
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
                for(int i = 0; i < wn2; i++) Trade.PositionClose(tks[winIdx[i]]);
                for(int i = 0; i < wn;  i++) Trade.PositionClose(tks[worstIdx[i]]);
                closedCycles++;
            } else break;
        }
        if(closedCycles > 0)
            Print("RTB: Hedge trim cycles=", closedCycles, " x up to ", g_TrimMaxWin, " winners / ", g_TrimMaxLoss, " losers");
        break;
    }

    case TRIM_HEDGE_PTS: {
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
            if(!IsManaged()) continue;
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
                for(int i = 0; i < wn2; i++) Trade.PositionClose(tks[winIdx[i]]);
                for(int i = 0; i < wn;  i++) Trade.PositionClose(tks[worstIdx[i]]);
                closedCycles++;
            } else break;
        }
        if(closedCycles > 0)
            Print("RTB: Hedge-by-Points trim cycles=", closedCycles, " x up to ", g_TrimMaxWin, " winners / ", g_TrimMaxLoss, " losers");
        break;
    }

    case TRIM_TARGET: {
        if(g_TrimTarget <= 0) return;
        int closed = 0;
        for(int n = 0; n < g_TrimMaxLoss; n++) {
            double totalProfit = FloatProfit();
            if(totalProfit < g_TrimTarget) break;
            ulong worstTk = WorstTicket();
            if(worstTk == 0 || !PositionSelectByTicket(worstTk)) break;
            double worstP = PositionGetDouble(POSITION_PROFIT);
            if((totalProfit + worstP) >= g_TrimTarget) {
                Trade.PositionClose(worstTk);
                closed++;
            } else break;
        }
        if(closed > 0) Print("RTB: Trim target met, closed=", closed);
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
    bool hedgeTrail = g_HedgeEnable && (HedgeCutBuy || HedgeCutSell);
    if(!g_TrailEnable && !hedgeTrail) return;
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
            bool isDCAcmt  = (StringFind(cmt, "RTB|") == 0);
            bool isPyracmt = (StringFind(cmt, "RTP|") == 0);
            if(!isDCAcmt && !isPyracmt) continue;
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
                                StringFind(cmt, "RTB|") != 0 &&
                                StringFind(cmt, "RTP|") != 0);
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

    if(g_CloseProfit > 0 && !(g_HedgeEnable && (HedgeCutBuy || HedgeCutSell))) {
        if(FloatProfit() >= g_CloseProfit) {
            Print("RTB: CloseProfit target reached. Closing all.");
            CloseAll();
            return;
        }
    }

    if(g_CloseLoss > 0 && !(g_HedgeEnable && (HedgeCutBuy || HedgeCutSell))) {
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

//+------------------------------------------------------------------+
//| TECHNICALS PANEL — đồng hồ bán nguyệt vẽ bằng CCanvas, cạnh Lịch  |
//+------------------------------------------------------------------+
uint LerpARGB(uint c1, uint c2, double t) {
    t = MathMax(0.0, MathMin(1.0, t));
    double a1 = (double)((c1 >> 24) & 0xFF), r1 = (double)((c1 >> 16) & 0xFF),
           g1 = (double)((c1 >> 8)  & 0xFF), b1 = (double)(c1 & 0xFF);
    double a2 = (double)((c2 >> 24) & 0xFF), r2 = (double)((c2 >> 16) & 0xFF),
           g2 = (double)((c2 >> 8)  & 0xFF), b2 = (double)(c2 & 0xFF);
    uchar a = (uchar)MathRound(a1 + (a2 - a1) * t);
    uchar r = (uchar)MathRound(r1 + (r2 - r1) * t);
    uchar g = (uchar)MathRound(g1 + (g2 - g1) * t);
    uchar b = (uchar)MathRound(b1 + (b2 - b1) * t);
    return ((uint)a << 24) | ((uint)r << 16) | ((uint)g << 8) | (uint)b;
}

uint GaugeColorAt(double angle) {
    double stopAngles[5] = {162, 126, 90, 54, 18};
    uint   stopColors[5]  = {0xFFB91C1C, 0xFFE0653A, 0xFF5B6472, 0xFF3FAE72, 0xFF16A34A};
    if(angle >= stopAngles[0]) return stopColors[0];
    if(angle <= stopAngles[4]) return stopColors[4];
    for(int i = 0; i < 4; i++) {
        if(angle <= stopAngles[i] && angle >= stopAngles[i + 1]) {
            double t = (stopAngles[i] - angle) / (stopAngles[i] - stopAngles[i + 1]);
            return LerpARGB(stopColors[i], stopColors[i + 1], t);
        }
    }
    return stopColors[2];
}

void DrawGaugeRing(int cx, int cy, int rInner, int rOuter) {
    int boundOuter2 = (rOuter + 1) * (rOuter + 1);
    int boundInner2 = MathMax(0, (rInner - 1) * (rInner - 1));
    for(int y = -rOuter - 1; y <= 0; y++) {
        int yy = y * y;
        for(int x = -rOuter - 1; x <= rOuter + 1; x++) {
            int d2 = x * x + yy;
            if(d2 < boundInner2 || d2 > boundOuter2) continue;
            double d = MathSqrt((double)d2);
            if(d < rInner - 1.0 || d > rOuter + 1.0) continue;
            double angle = MathArctan2((double)(-y), (double)x) * 180.0 / RTB_PI;
            if(angle < -0.01 || angle > 180.01) continue;
            double cov = 1.0;
            if(d < rInner) cov *= MathMax(0.0, MathMin(1.0, 1.0 - (rInner - d)));
            if(d > rOuter) cov *= MathMax(0.0, MathMin(1.0, 1.0 - (d - rOuter)));
            if(cov <= 0.0) continue;
            uint clr  = GaugeColorAt(angle);
            uchar a   = (uchar)MathRound(((clr >> 24) & 0xFF) * cov);
            uint  px  = ((uint)a << 24) | (clr & 0x00FFFFFF);
            g_TechCanvas.PixelSet(cx + x, cy + y, px);
        }
    }
}

void DrawLinePx(int x1, int y1, int x2, int y2, uint argb) {
    int dx = MathAbs(x2 - x1), dy = -MathAbs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;
    int x = x1, y = y1;
    while(true) {
        g_TechCanvas.PixelSet(x, y, argb);
        if(x == x2 && y == y2) break;
        int e2 = 2 * err;
        if(e2 >= dy) { err += dy; x += sx; }
        if(e2 <= dx) { err += dx; y += sy; }
    }
}

void DrawThickLine(int x1, int y1, int x2, int y2, int width, uint argb) {
    double dx = x2 - x1, dy = y2 - y1;
    double len = MathSqrt(dx * dx + dy * dy);
    double hw  = width / 2.0;
    if(len < 0.001) {
        int r = (int)MathCeil(hw);
        for(int oy = -r; oy <= r; oy++)
            for(int ox = -r; ox <= r; ox++)
                if(ox * ox + oy * oy <= hw * hw) g_TechCanvas.PixelSet(x1 + ox, y1 + oy, argb);
        return;
    }
    double ux = dx / len, uy = dy / len;
    int minX = (int)MathFloor(MathMin(x1, x2) - hw) - 1;
    int maxX = (int)MathCeil (MathMax(x1, x2) + hw) + 1;
    int minY = (int)MathFloor(MathMin(y1, y2) - hw) - 1;
    int maxY = (int)MathCeil (MathMax(y1, y2) + hw) + 1;
    for(int y = minY; y <= maxY; y++) {
        for(int x = minX; x <= maxX; x++) {
            double px = x - x1, py = y - y1;
            double t = px * ux + py * uy;
            t = MathMax(0.0, MathMin(len, t));
            double cx = x1 + ux * t, cy = y1 + uy * t;
            double ddx = x - cx, ddy = y - cy;
            if(ddx * ddx + ddy * ddy <= hw * hw) g_TechCanvas.PixelSet(x, y, argb);
        }
    }
}

void DrawTechnicalsGauge(int px, int py, int width, int height) {
    string objName = GUI + "TechGauge";
    if(!g_TechCanvasReady) {
        if(!g_TechCanvas.CreateBitmapLabel(objName, px, py, width, height, COLOR_FORMAT_ARGB_NORMALIZE)) {
            Print("RTB: Không tạo được canvas Technicals, err=", GetLastError());
            return;
        }
        ObjectSetInteger(0, objName, OBJPROP_CORNER,     CORNER_LEFT_UPPER);
        ObjectSetInteger(0, objName, OBJPROP_BACK,       false);
        ObjectSetInteger(0, objName, OBJPROP_SELECTABLE, false);
        g_TechCanvasReady = true;
    }
    ObjectSetInteger(0, objName, OBJPROP_XDISTANCE, px);
    ObjectSetInteger(0, objName, OBJPROP_YDISTANCE, py);

    g_TechCanvas.Erase(0x00000000);

    int cx = width / 2;
    int cy = height - 6;
    int R  = MathMin(width / 2, height - 6) - 12;
    int thickness = 20;
    int rInner = R - thickness / 2, rOuter = R + thickness / 2;

    DrawGaugeRing(cx, cy, rInner, rOuter);

    for(int t = 0; t <= 5; t++) {
        double a   = 180.0 - t * 36.0;
        double rad = a * RTB_PI / 180.0;
        int tx1 = cx + (int)MathRound((rOuter + 2) * MathCos(rad));
        int ty1 = cy - (int)MathRound((rOuter + 2) * MathSin(rad));
        int tx2 = cx + (int)MathRound((rOuter + 7) * MathCos(rad));
        int ty2 = cy - (int)MathRound((rOuter + 7) * MathSin(rad));
        DrawLinePx(tx1, ty1, tx2, ty2, 0xFF6B7280);
    }

    double angle = 90.0 * (1.0 - g_TechRating);
    double rad   = angle * RTB_PI / 180.0;
    int nx = cx + (int)MathRound((rInner - 4) * MathCos(rad));
    int ny = cy - (int)MathRound((rInner - 4) * MathSin(rad));
    DrawThickLine(cx, cy, nx, ny, 3, 0xFFEEF1F6);
    g_TechCanvas.FillCircle(cx, cy, 6, 0xFF14192A);
    g_TechCanvas.FillCircle(cx, cy, 4, 0xFFEEF1F6);

    g_TechCanvas.Update();
}

void RemoveTechnicalsPanel() {
    string names[] = {"TechCardBg", "TechCardBar", "TechEB", "TechSrc", "TechVerdict", "TechFoot",
                       "TechZ0", "TechZ1", "TechZ2", "TechZ3", "TechZ4"};
    for(int i = 0; i < ArraySize(names); i++) ObjectDelete(0, GUI + names[i]);
    if(g_TechCanvasReady) { g_TechCanvas.Destroy(); g_TechCanvasReady = false; }
}

void UpdateTechnicalsPanel() {
    if(!g_CalExpanded) { RemoveTechnicalsPanel(); return; }

    UpdateTechnicalRating();

    int px = g_CalRightEdge + 12;
    int py = InpCalPanelY;
    int cardW = 250, cardH = 224;

    CreateRect("TechCardBg",  px, py, cardW, cardH, C'20,28,44');
    CreateRect("TechCardBar", px, py, 2,     cardH, C'79,195,217');
    Lbl("TechEB", "CHỈ BÁO KỸ THUẬT", px + 10, py + 6, C'95,108,132', 9);
    ObjectSetString(0, GUI + "TechEB", OBJPROP_FONT, "Calibri Bold");
    LblR("TechSrc", EnumToString(InpTechTF), px + cardW - 10, py + 6, C'95,108,132', 8);

    DrawTechnicalsGauge(px + 8, py + 24, cardW - 16, 130);

    string zoneNames[5] = {"Strong Sell", "Sell", "Neutral", "Buy", "Strong Buy"};
    int zoneIdx = 2;
    if(g_TechRating < -0.5)      zoneIdx = 0;
    else if(g_TechRating < -0.1) zoneIdx = 1;
    else if(g_TechRating <= 0.1) zoneIdx = 2;
    else if(g_TechRating <= 0.5) zoneIdx = 3;
    else                          zoneIdx = 4;

    int zoneY  = py + 24 + 130 + 10;
    int slotW  = (cardW - 16) / 5;
    for(int z = 0; z < 5; z++) {
        int zx = px + 8 + slotW * z + slotW / 2;
        color zc = (z == zoneIdx) ? C'231,236,245' : C'95,108,132';
        string objName = "TechZ" + IntegerToString(z);
        Lbl(objName, zoneNames[z], zx, zoneY, zc, 7);
        ObjectSetInteger(0, GUI + objName, OBJPROP_ANCHOR, ANCHOR_UPPER);
    }

    color vClr = (g_TechRating > 0.1) ? clrLimeGreen : (g_TechRating < -0.1 ? clrTomato : clrSilver);
    string vTxt = zoneNames[zoneIdx];
    StringToUpper(vTxt);
    Lbl("TechVerdict", vTxt, px + cardW / 2, py + 192, vClr, 15);
    ObjectSetInteger(0, GUI + "TechVerdict", OBJPROP_ANCHOR, ANCHOR_CENTER);
    ObjectSetString(0, GUI + "TechVerdict", OBJPROP_FONT, "Calibri Bold");

    Lbl("TechFoot", StringFormat("Score %.2f", g_TechRating), px + 10, py + cardH - 16, C'95,108,132', 8);
}

void UpdateGUI(bool forceCalRefresh = false) {
    if(!InpShowPanel) { RemoveGUI(); return; }
    int PX = InpPanelX;
    int PY = InpPanelY;
    int PW = InpPanelWidth;
    int botOff = 26;
    int titleOff = 36;

    double balance   = AccountInfoDouble(ACCOUNT_BALANCE);
    double equity    = AccountInfoDouble(ACCOUNT_EQUITY);
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
    switch(g_SignalMode) {
        case SIG_EMA:       sigName = "EMA 34+89";      break;
        case SIG_BZ_ZONE:   sigName = "BZ Zone";        break;
        case SIG_ICHIMOKU:  sigName = "Ichimoku";       break;
        case SIG_BB:        sigName = "Bollinger Band"; break;
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
    ObjectSetString(0,  titleObj, OBJPROP_TEXT,      "★ RICH TRADING BOT ★");
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

    if(!g_PanelCollapsed) {
    Lbl("TimeRow", tStr, contentX, y2, C'127,139,163', 10);
    y2 += 16;

    // ========== Chips: Signal / Direction / Mode ==========
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
        CreateBtn("BtnPanelToggle", g_PanelCollapsed ? " + " : " - ", rightEdge - 18, y2 + 3, 18, 14, pcBg, pcBd);
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
    string hedgeText = ""; color hedgeClr = clrSilver;
    if(g_HedgeEnable) {
        double point = SymbolInfoDouble(_Symbol, SYMBOL_POINT);
        double bid   = SymbolInfoDouble(_Symbol, SYMBOL_BID);
        double ask   = SymbolInfoDouble(_Symbol, SYMBOL_ASK);
        if(HedgeCutBuy && !HedgeCutSell) {
            hedgeText = "Hedge : ✕BUY | ▼SELL Trail"; hedgeClr = clrLimeGreen;
        } else if(HedgeCutSell && !HedgeCutBuy) {
            hedgeText = "Hedge : ▲BUY Trail | ✕SELL"; hedgeClr = clrLimeGreen;
        } else if(!HedgeCutBuy && !HedgeCutSell) {
            double buyDist  = (HedgeInitBuyPrice  > 0) ? (HedgeInitBuyPrice  - bid) / point : 0;
            double sellDist = (HedgeInitSellPrice > 0) ? (ask - HedgeInitSellPrice) / point : 0;
            hedgeText = (buyDist > 0 || sellDist > 0)
                ? StringFormat("Hedge : ▲%.0f | ▼%.0f pt", buyDist, sellDist) : "Hedge : Waiting...";
            hedgeClr = C'127,139,163';
        } else {
            hedgeText = "Hedge : Complete"; hedgeClr = C'90,90,90';
        }
    }
    if(!g_PanelCollapsed) {
    int riskH = 6 + 13 + (13+7+3)*2 + (g_HedgeEnable ? 15 : 0) + 6;
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
    yr += 13 + 7 + 3;
    if(g_HedgeEnable) Lbl("HdgS", hedgeText, contentX + 8, yr, hedgeClr, 10);
    else ObjectDelete(0, GUI + "HdgS");
    y2 += riskH + 8;

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
            "MDDL", "MDDV", "MDDGTrk", "MDDGFill", "HdgS",
            "CardBuy", "CardBuyBar", "BuyL", "BuyV", "BuyLot",
            "CardSell", "CardSellBar", "SelL", "SelV", "SelLot",
            "TotL", "TotV"
        };
        for(int aci = 0; aci < ArraySize(acctCollapsedObjs); aci++) ObjectDelete(0, GUI + acctCollapsedObjs[aci]);
    }

    int contentBottom = y2 + 6;
    int bgH  = contentBottom - (PY + titleOff);
    int bg2Y = contentBottom + 14;
    ObjectSetInteger(0, bg, OBJPROP_YSIZE, bgH);

    if(!g_PanelCollapsed) {
    string bg2 = GUI + "BG2";
    if(ObjectFind(0, bg2) < 0) {
        ObjectCreate(0, bg2, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg2, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg2, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg2, OBJPROP_YSIZE,       110 + botOff);
        ObjectSetInteger(0, bg2, OBJPROP_BGCOLOR,     C'17,21,32');
        ObjectSetInteger(0, bg2, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg2, OBJPROP_COLOR,       C'65,90,160');
        ObjectSetInteger(0, bg2, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg2, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg2, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg2, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg2, OBJPROP_YDISTANCE, bg2Y);

    int bg3Y = bg2Y + 110 + botOff + 14;
    string bg3 = GUI + "BG3";
    if(ObjectFind(0, bg3) < 0) {
        ObjectCreate(0, bg3, OBJ_RECTANGLE_LABEL, 0, 0, 0);
        ObjectSetInteger(0, bg3, OBJPROP_CORNER,      CORNER_LEFT_UPPER);
        ObjectSetInteger(0, bg3, OBJPROP_XSIZE,       PW);
        ObjectSetInteger(0, bg3, OBJPROP_YSIZE,       115);
        ObjectSetInteger(0, bg3, OBJPROP_BGCOLOR,     C'20,28,44');
        ObjectSetInteger(0, bg3, OBJPROP_BORDER_TYPE, BORDER_FLAT);
        ObjectSetInteger(0, bg3, OBJPROP_COLOR,       C'38,50,72');
        ObjectSetInteger(0, bg3, OBJPROP_WIDTH,       1);
        ObjectSetInteger(0, bg3, OBJPROP_BACK,        false);
        ObjectSetInteger(0, bg3, OBJPROP_SELECTABLE,  false);
    }
    ObjectSetInteger(0, bg3, OBJPROP_XDISTANCE, PX);
    ObjectSetInteger(0, bg3, OBJPROP_YDISTANCE, bg3Y);

    int y = bg2Y + 10;
    Lbl("P2T", "===  ĐIỀU KHIỂN LỆNH  ===", x, y, C'90,140,230', 9);
    ObjectSetString(0, GUI + "P2T", OBJPROP_FONT, "Calibri Bold"); y += s + 2;

    {
        string botTxt = g_BotEnabled ? "  Bot: ON" : "  Bot: OFF";
        color  botBg  = g_BotEnabled ? C'0,90,30'   : C'120,20,20';
        color  botBd  = g_BotEnabled ? C'40,190,90' : C'220,60,60';
        CreateBtn("BtnBotToggle", botTxt, PX+7, y, bfw, bh, botBg, botBd);
        y += bh + 4;
    }

    CreateBtn("BtnCloseAll",    "  Close All",     PX+7, y, bfw, bh, C'20,60,150',  C'80,130,230'); y += bh + 4;
    CreateBtn("BtnCloseBuy",    "▲ Close Buy",     PX+7, y, bhw, bh, C'0,105,45',   C'45,185,90' );
    CreateBtn("BtnCloseProfit", "$ Close Profit",  bx2,  y, bhw, bh, C'0,110,100',  C'40,190,170'); y += bh + 4;
    CreateBtn("BtnCloseSell",   "▼ Close Sell",    PX+7, y, bhw, bh, C'145,15,15',  C'230,65,65' );
    CreateBtn("BtnCloseLoss",   "✕ Close Loss",    bx2,  y, bhw, bh, C'140,35,20',  C'210,80,55' );

    int y3 = bg3Y + 10;
    Lbl("P3T", "THỐNG KÊ", contentX + 8, y3, C'95,108,132', 9);
    ObjectSetString(0, GUI + "P3T", OBJPROP_FONT, "Calibri Bold");
    CreateBtn("BtnCalToggle", g_CalExpanded ? "« Đóng" : "Xem Lịch »", PX + PW - 82, y3 - 3, 76, 18, C'25,45,85', C'70,110,190');
    y3 += 20;

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

    string pipsTxt[4], profitTxt[4], gainTxt[4], lotTxt[4];
    for(int r = 0; r < 4; r++) {
        pipsTxt[r]   = StringFormat("%.0f",   allStats[r].pips);
        profitTxt[r] = StringFormat("%s$%.1f", allStats[r].profit >= 0 ? "+" : "", allStats[r].profit);
        gainTxt[r]   = StringFormat("%s%.1f%%", allStats[r].gain >= 0 ? "+" : "", allStats[r].gain);
        lotTxt[r]    = StringFormat("%.2f",   allStats[r].lot);
    }

    int maxLen0 = StringLen("Date"), maxLen1 = StringLen("Pips"), maxLen2 = StringLen("Profit"),
        maxLen3 = StringLen("Gain");
    for(int r = 0; r < 4; r++) {
        maxLen0 = MathMax(maxLen0, StringLen(rowKeys[r]));
        maxLen1 = MathMax(maxLen1, StringLen(pipsTxt[r]));
        maxLen2 = MathMax(maxLen2, StringLen(profitTxt[r]));
        maxLen3 = MathMax(maxLen3, StringLen(gainTxt[r]));
    }
    int charPx = 7;
    int colW0 = maxLen0 * charPx, colW1 = maxLen1 * charPx, colW2 = maxLen2 * charPx, colW3 = maxLen3 * charPx;

    int cx0 = contentX + 8;
    int cx1 = cx0 + colW0 + 6;
    int cx2 = cx1 + colW1 + 6;
    int cx3 = cx2 + colW2 + 6;
    int cx4 = cx3 + colW3 + 6;

    Lbl("TH0", "Date",   cx0, y3, C'111,163,201', 9);
    Lbl("TH1", "Pips",   cx1, y3, C'111,163,201', 9);
    Lbl("TH2", "Profit", cx2, y3, C'111,163,201', 9);
    Lbl("TH3", "Gain",   cx3, y3, C'111,163,201', 9);
    Lbl("TH4", "Lot",    cx4, y3, C'111,163,201', 9);
    y3 += 16;

    int rowH = 16;
    CreateRect("RowStripe0", contentX + 2, y3,           cardW - 4, rowH, C'26,36,56');
    CreateRect("RowStripe1", contentX + 2, y3 + rowH * 2, cardW - 4, rowH, C'26,36,56');

    for(int r = 0; r < 4; r++) {
        color  rc = (allStats[r].profit >= 0) ? clrLimeGreen : clrTomato;
        string ri = IntegerToString(r);
        Lbl("TR"+ri+"L", rowKeys[r],   cx0, y3, C'159,176,201', 9);
        Lbl("TR"+ri+"P", pipsTxt[r],   cx1, y3, rc, 9);
        Lbl("TR"+ri+"$", profitTxt[r], cx2, y3, rc, 9);
        Lbl("TR"+ri+"G", gainTxt[r],   cx3, y3, rc, 9);
        Lbl("TR"+ri+"V", lotTxt[r],    cx4, y3, C'159,176,201', 9);
        y3 += rowH;
    }

    int bg3H = (y3 - bg3Y) + 6;
    ObjectSetInteger(0, bg3, OBJPROP_YSIZE, bg3H);
    CreateRect("StatsBar", PX, bg3Y, 2, bg3H, C'79,195,217');

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
        int bg4Y = bg3Y + bg3H + 14;
        ObjectSetInteger(0, bg4, OBJPROP_XDISTANCE, PX);
        ObjectSetInteger(0, bg4, OBJPROP_YDISTANCE, bg4Y);
        int y4 = bg4Y + 10;
        Lbl("P4T", "===  VÀO LỆNH THỦ CÔNG  ===", x, y4, C'230,100,100', 9);
        ObjectSetString(0, GUI + "P4T", OBJPROP_FONT, "Calibri Bold"); y4 += s + 2;
        CreateBtn("BtnOpenBuy",  "▲ Open Buy",  PX+7, y4, bhw, bh, C'0,80,20',  C'30,200,80');
        CreateBtn("BtnOpenSell", "▼ Open Sell", bx2,  y4, bhw, bh, C'100,0,0',  C'220,40,40');
        g_LastPanelBottom = bg4Y + 58;
    } else {
        ObjectDelete(0, GUI + "BG4");
        ObjectDelete(0, GUI + "P4T");
        ObjectDelete(0, GUI + "BtnOpenBuy");
        ObjectDelete(0, GUI + "BtnOpenSell");
        g_LastPanelBottom = bg3Y + bg3H;
    }

    } else {
        string collapsedObjs[] = {
            "BG2", "P2T", "BtnBotToggle", "BtnCloseAll", "BtnCloseBuy", "BtnCloseProfit", "BtnCloseSell", "BtnCloseLoss",
            "BG3", "P3T", "BtnCalToggle",
            "TH0", "TH1", "TH2", "TH3", "TH4", "RowStripe0", "RowStripe1", "StatsBar",
            "TR0L","TR0P","TR0$","TR0G","TR0V", "TR1L","TR1P","TR1$","TR1G","TR1V",
            "TR2L","TR2P","TR2$","TR2G","TR2V", "TR3L","TR3P","TR3$","TR3G","TR3V",
            "BG4", "P4T", "BtnOpenBuy", "BtnOpenSell"
        };
        for(int ci = 0; ci < ArraySize(collapsedObjs); ci++) ObjectDelete(0, GUI + collapsedObjs[ci]);
        g_LastPanelBottom = contentBottom;
    }

    UpdateCalendarPanel(forceCalRefresh);
    UpdateTechnicalsPanel();

    ChartRedraw(0);
}

void RemoveGUI() {
    if(g_TechCanvasReady) { g_TechCanvas.Destroy(); g_TechCanvasReady = false; }
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
//| BOT ENABLE / DISABLE                                             |
//+------------------------------------------------------------------+
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

    ENUM_ORDER_TYPE pendType1 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_STOP  : ORDER_TYPE_SELL_STOP;
    ENUM_ORDER_TYPE pendType2 = (posType == POSITION_TYPE_BUY) ? ORDER_TYPE_BUY_LIMIT : ORDER_TYPE_SELL_LIMIT;
    for(int i = 0; i < OrdersTotal() && count < cap; i++) {
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

        slotPrices[count]  = oPrice;
        slotTimes[count]   = (datetime)OrderGetInteger(ORDER_TIME_SETUP);
        slotPosTk[count]   = 0;
        slotOrderTk[count] = tk;
        count++;
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

    if(posType == POSITION_TYPE_BUY) {
        PeakDCABuy = count;
        for(int s = 0; s < count; s++) {
            DCABuyPrices[s]    = slotPrices[s];
            DCABuyTickets[s]   = slotPosTk[s];
            DCABuyLimitTk[s]   = slotOrderTk[s];
            DCABuyBounced[s]   = false;
        }
    } else {
        PeakDCASell = count;
        for(int s = 0; s < count; s++) {
            DCASellPrices[s]    = slotPrices[s];
            DCASellTickets[s]   = slotPosTk[s];
            DCASellLimitTk[s]   = slotOrderTk[s];
            DCASellBounced[s]   = false;
        }
    }
    Print("RTB: RebuildDCAState ", (posType==POSITION_TYPE_BUY?"BUY":"SELL"), " peak=", count);
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
    g_PyraBuyEnable = InpPyraBuyEnable; g_PyraSellEnable = InpPyraSellEnable;
    g_TrimMode = InpTrimMode; g_TrimTrigger = InpTrimTrigger;
    g_TrimTarget = InpTrimTarget; g_TrimMaxLoss = InpTrimMaxLoss; g_TrimMaxWin = InpTrimMaxWin;
    g_TrimMaxCycles = InpTrimMaxCycles;
    g_PartialTrimDD = InpPartialTrimDD;
    g_TrailEnable = InpTrailEnable; g_TrailMode = InpTrailMode; g_TrailMinOrds = InpTrailMinOrds;
    g_TrailActivate = InpTrailActivate; g_TrailStep = InpTrailStep; g_TrailInit = InpTrailInit;
    g_CloseProfit = InpCloseProfit; g_CloseLoss = InpCloseLoss; g_ClosePerPips = InpClosePerPips;
    g_DayMaxLoss = InpDayMaxLoss; g_DayMaxProfit = InpDayMaxProfit;
    g_HedgeEnable = InpHedgeEnable; g_HedgeCutPts = InpHedgeCutPts;

    ApplyBotEnabled(InpBotEnabled);

    Trade.SetExpertMagicNumber(InpMagic);
    Trade.SetDeviationInPoints(50);
    Trade.SetTypeFilling(ORDER_FILLING_RETURN);

    SetupChartColors();

    InitDCA();
    InitPyra();

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

    for(int i = 0; i < 6; i++) {
        hTechMASMA[i] = iMA(_Symbol, InpTechTF, TechMAPeriods[i], 0, MODE_SMA, PRICE_CLOSE);
        hTechMAEMA[i] = iMA(_Symbol, InpTechTF, TechMAPeriods[i], 0, MODE_EMA, PRICE_CLOSE);
    }
    hTechIchi  = iIchimoku(_Symbol, InpTechTF, InpIchiTenkan, InpIchiKijun, InpIchiSenkou);
    hTechRSI   = iRSI(_Symbol, InpTechTF, 14, PRICE_CLOSE);
    hTechStoch = iStochastic(_Symbol, InpTechTF, 14, 3, 3, MODE_SMA, STO_LOWHIGH);
    hTechCCI   = iCCI(_Symbol, InpTechTF, 20, PRICE_TYPICAL);
    hTechADX   = iADX(_Symbol, InpTechTF, 14);
    hTechAO    = iAO(_Symbol, InpTechTF);
    hTechMom   = iMomentum(_Symbol, InpTechTF, 10, PRICE_CLOSE);
    hTechMACD  = iMACD(_Symbol, InpTechTF, 12, 26, 9, PRICE_CLOSE);
    hTechWPR   = iWPR(_Symbol, InpTechTF, 14);
    hTechBulls = iBullsPower(_Symbol, InpTechTF, 13);
    hTechBears = iBearsPower(_Symbol, InpTechTF, 13);
    if(hTechIchi == INVALID_HANDLE || hTechRSI == INVALID_HANDLE || hTechStoch == INVALID_HANDLE ||
       hTechCCI == INVALID_HANDLE  || hTechADX == INVALID_HANDLE || hTechAO == INVALID_HANDLE     ||
       hTechMom == INVALID_HANDLE  || hTechMACD == INVALID_HANDLE || hTechWPR == INVALID_HANDLE   ||
       hTechBulls == INVALID_HANDLE || hTechBears == INVALID_HANDLE)
        Print("RTB: CẢNH BÁO — một số indicator Technicals tạo lỗi, panel Technicals có thể thiếu dữ liệu.");

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
    ArrayResize(DCABuyPrices,    InpMaxBuy);
    ArrayResize(DCABuyBounced,   InpMaxBuy);
    ArrayResize(DCABuyTickets,   InpMaxBuy);
    ArrayResize(DCABuyLimitTk,   InpMaxBuy);
    ArrayResize(DCASellPrices,    InpMaxSell);
    ArrayResize(DCASellBounced,   InpMaxSell);
    ArrayResize(DCASellTickets,   InpMaxSell);
    ArrayResize(DCASellLimitTk,   InpMaxSell);
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

    HedgeCutBuy        = false;
    HedgeCutSell       = false;
    HedgeInitBuyPrice  = 0.0;
    HedgeInitSellPrice = 0.0;
    HedgeTrendSide     = -1;
    if(g_HedgeEnable) {
        if(CountBuy() > 0 && CountSell() == 0) { HedgeCutSell = true; Print("RTB: Hedge restart — infer SELL was cut"); }
        if(CountSell() > 0 && CountBuy() == 0) { HedgeCutBuy  = true; Print("RTB: Hedge restart — infer BUY was cut"); }
        if(CountPyra(POSITION_TYPE_BUY) > 0)       { HedgeTrendSide = POSITION_TYPE_BUY;  Print("RTB: Hedge restart — trend=BUY"); }
        else if(CountPyra(POSITION_TYPE_SELL) > 0) { HedgeTrendSide = POSITION_TYPE_SELL; Print("RTB: Hedge restart — trend=SELL"); }
    }

    EventSetTimer(1);

    Print("RTB: Initialized. Magic=", InpMagic, " Signal=", EnumToString(g_SignalMode));
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
    for(int i = 0; i < 6; i++) { IndicatorRelease(hTechMASMA[i]); IndicatorRelease(hTechMAEMA[i]); }
    IndicatorRelease(hTechIchi);
    IndicatorRelease(hTechRSI);
    IndicatorRelease(hTechStoch);
    IndicatorRelease(hTechCCI);
    IndicatorRelease(hTechADX);
    IndicatorRelease(hTechAO);
    IndicatorRelease(hTechMom);
    IndicatorRelease(hTechMACD);
    IndicatorRelease(hTechWPR);
    IndicatorRelease(hTechBulls);
    IndicatorRelease(hTechBears);
}

void OnTick() {
    CheckEntry();
    if(g_StealthMode) CheckExit();
    if(!DayLimitHit) CheckTrailing();
}

void OnTimer() {
    UpdateDayProfit();
    CheckDayLimit();

    if(CountBuy()  == 0 && !HasPendingDCA(POSITION_TYPE_BUY))  ResetDCAState(POSITION_TYPE_BUY);
    if(CountSell() == 0 && !HasPendingDCA(POSITION_TYPE_SELL)) ResetDCAState(POSITION_TYPE_SELL);

    if(!g_StealthMode) CheckExit();

    CheckHedgeCut();

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
    if     (sparam == GUI + "BtnCloseAll")    CloseAll();
    else if(sparam == GUI + "BtnCloseBuy")    CloseAll(POSITION_TYPE_BUY);
    else if(sparam == GUI + "BtnCloseSell")   CloseAll(POSITION_TYPE_SELL);
    else if(sparam == GUI + "BtnCloseProfit") CloseAllProfit();
    else if(sparam == GUI + "BtnCloseLoss")   CloseAllLoss();
    else if(sparam == GUI + "BtnOpenBuy") {
        if(!DayLimitHit && g_BotEnabled && CountBuy() < InpMaxBuy) {
            Trade.SetExpertMagicNumber(0);
            OpenOrder(ORDER_TYPE_BUY, InpLotSize, g_TP_Points, g_SL_Points);
            Trade.SetExpertMagicNumber(InpMagic);
        }
    }
    else if(sparam == GUI + "BtnOpenSell") {
        if(!DayLimitHit && g_BotEnabled && CountSell() < InpMaxSell) {
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
