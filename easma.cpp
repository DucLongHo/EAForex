//+------------------------------------------------------------------+
//|                                              TripleSMA_Lines.mq5 |
//|                                  Copyright 2023, Gemini AI Partner|
//+------------------------------------------------------------------+
#property strict

//--- Input parameters
input int      InpMAPeriod   = 20;          // Chu kỳ SMA
input int      InpMAShift    = 0;           // Dịch chuyển ngang
input ENUM_MA_METHOD InpMAMethod = MODE_SMA; // Phương pháp (SMA)
input int      InpDistance   = 200;         // Khoảng cách đường biên (Points)
input color    InpColorMain  = clrYellow;    // Màu đường chính
input color    InpColorUpper = clrAqua;      // Màu đường trên
input color    InpColorLower = clrRed;       // Màu đường dưới

//--- Global variables
int            handleSMA;                   // Handle của chỉ báo SMA
double         bufferSMA[];                 // Mảng lưu giá trị SMA

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit()
{
   // Thiết lập handle cho đường SMA chính
   handleSMA = iMA(_Symbol, _Period, InpMAPeriod, InpMAShift, InpMAMethod, PRICE_CLOSE);
   
   if(handleSMA == INVALID_HANDLE)
   {
      Print("Không thể tạo handle chỉ báo iMA");
      return(INIT_FAILED);
   }
   
   // Đặt mảng là mảng chuỗi thời gian (mới nhất ở chỉ số 0)
   ArraySetAsSeries(bufferSMA, true);
   
   return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
{
   // Xóa các đường đã vẽ khi tắt EA
   ObjectsDeleteAll(0, "SMA_Line_");
   IndicatorRelease(handleSMA);
}

//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick()
{
   // Sao chép dữ liệu SMA vào mảng
   if(CopyBuffer(handleSMA, 0, 0, 2, bufferSMA) < 0) return;

   double mainSMA  = bufferSMA[0];
   double distance = InpDistance * _Point;
   
   double upperSMA = mainSMA + distance;
   double lowerSMA = mainSMA - distance;

   // Vẽ hoặc cập nhật các đường trên Chart
   DrawLine("SMA_Line_Main",  mainSMA,  InpColorMain);
   DrawLine("SMA_Line_Upper", upperSMA, InpColorUpper);
   DrawLine("SMA_Line_Lower", lowerSMA, InpColorLower);
}

//+------------------------------------------------------------------+
//| Hàm vẽ đường ngang (Horizontal Line)                             |
//+------------------------------------------------------------------+
void DrawLine(string name, double price, color clr)
{
   if(ObjectFind(0, name) < 0)
   {
      ObjectCreate(0, name, OBJ_HLINE, 0, 0, price);
      ObjectSetInteger(0, name, OBJPROP_COLOR, clr);
      ObjectSetInteger(0, name, OBJPROP_WIDTH, 2);
      ObjectSetInteger(0, name, OBJPROP_STYLE, STYLE_SOLID);
   }
   else
   {
      ObjectMove(0, name, 0, 0, price);
   }
}