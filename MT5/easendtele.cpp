//+------------------------------------------------------------------+
//|                                                  TelegramBot.mq5 |
//|                        Copyright 2024, MetaQuotes Software Corp. |
//|                                                                  |
//+------------------------------------------------------------------+
#property copyright "Copyright 2024"
#property version   "1.00"

//--- Input parameters
input string   TelegramToken = "7996115077:AAEdVxsgNOvpTVIBmW8yZY7ZYAMBEXlipUs"; // Telegram Bot Token
input string   ChatID = "-4700690816"; // Chat ID
input bool     EnableNotifications = true; // Bật thông báo
input int      DelayBetweenMessages = 1000; // Độ trễ giữa các tin nhắn (ms)
input bool     NotifyOnTrade = true; // Thông báo khi có giao dịch
input bool     NotifyOnError = true; // Thông báo khi có lỗi

//--- Global variables
int lastOrdersCount = 0;
ulong lastDealTicket = 0;

//+------------------------------------------------------------------+
const int TIME_VN = 7*3600; // Giờ Việt Nam
//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit(){
    // Kiểm tra kết nối Internet
    if(!TerminalInfoInteger(TERMINAL_CONNECTED)){
        Print("Dont connect internet!");
        return(INIT_FAILED);
    }
    
    // Lấy deal cuối cùng để tránh thông báo lịch sử cũ
    if(HistorySelect(0, TimeCurrent())){
        int totalDeals = HistoryDealsTotal();
        if(totalDeals > 0){
            lastDealTicket = HistoryDealGetTicket(totalDeals - 1);
        }
    }
    
    // Gửi thông báo khởi động
    if(EnableNotifications){
        string startMessage = "🤖 EA ĐÃ KHỞI ĐỘNG\n";
        startMessage += "💻 Server: " + AccountInfoString(ACCOUNT_SERVER) + "\n";
        startMessage += "👤 Tài khoản: " + IntegerToString(AccountInfoInteger(ACCOUNT_LOGIN)) + "\n";
        startMessage += "⏰ Timeframe: " + TimeToString(TimeCurrent() + TIME_VN, TIME_DATE|TIME_SECONDS) + "\n";
        
        SendTelegramMessage(startMessage);
    }
    
    lastOrdersCount = PositionsTotal();
    
    return(INIT_SUCCEEDED);
}

//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick(){
    static datetime lastCheck = 0;
    
    // Kiểm tra mỗi 1 giây
    if(TimeCurrent() - lastCheck >= 1){
        if(NotifyOnTrade){
            CheckForNewPositions();
            CheckForNewDeals();
        }
        lastCheck = TimeCurrent();
    }
}

//+------------------------------------------------------------------+
//| Hàm kiểm tra position mới                                       |
//+------------------------------------------------------------------+
void CheckForNewPositions(){
    int currentPositions = PositionsTotal();
    
    if(currentPositions != lastOrdersCount){
        if(currentPositions > lastOrdersCount){
            // Có position mới được mở
            CheckNewOpenedPositions();
        }
        
        lastOrdersCount = currentPositions;
    }
}

//+------------------------------------------------------------------+
//| Kiểm tra position mới mở                                        |
//+------------------------------------------------------------------+
void CheckNewOpenedPositions(){
    for(int index = PositionsTotal() - 1; index >= 0; index--){
        ulong ticket = PositionGetTicket(index);
        if(PositionSelectByTicket(ticket)){
            string symbol = PositionGetString(POSITION_SYMBOL);
            long positionTime = PositionGetInteger(POSITION_TIME);
            
            // Chỉ thông báo position mở trong 10 giây gần nhất
            if(TimeCurrent() - positionTime <= 10){
                string message = "🎯 POSITION MỚI\n";
                message += "Cặp: " + symbol + "\n";
                message += "Loại: " + GetPositionTypeString((ENUM_ORDER_TYPE)PositionGetInteger(POSITION_TYPE)) + "\n";
                message += "Khối lượng: " + DoubleToString(PositionGetDouble(POSITION_VOLUME), 2) + "\n";
                message += "Giá vào: " + DoubleToString(PositionGetDouble(POSITION_PRICE_OPEN), (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS)) + "\n";
                message += "SL: " + FormatPriceLevel(symbol, PositionGetDouble(POSITION_SL)) + "\n";
                message += "TP: " + FormatPriceLevel(symbol, PositionGetDouble(POSITION_TP)) + "\n";
                message += "Thời gian: " + TimeToString(positionTime + TIME_VN, TIME_DATE|TIME_MINUTES);
                SendTelegramMessage(message);
                Sleep(DelayBetweenMessages);
            }
        }
    }
}

//+------------------------------------------------------------------+
//| Kiểm tra deal mới                                               |
//+------------------------------------------------------------------+
void CheckForNewDeals(){
    if(HistorySelect(0, TimeCurrent())){
        int totalDeals = HistoryDealsTotal();

        for(int index = 0; index < totalDeals; index++){
            ulong dealTicket = HistoryDealGetTicket(index);
            
            // Chỉ xử lý deal mới
            if(dealTicket > lastDealTicket){
                ProcessNewDeal(dealTicket);
                lastDealTicket = dealTicket;
            }
        }
    }
}

//+------------------------------------------------------------------+
//| Xử lý deal mới                                                  |
//+------------------------------------------------------------------+
void ProcessNewDeal(ulong dealTicket){
    if(HistoryDealGetInteger(dealTicket, DEAL_ENTRY) == DEAL_ENTRY_OUT){
        string message = "✅ POSITION ĐÓNG\n";
        message += "Cặp: " + HistoryDealGetString(dealTicket, DEAL_SYMBOL) + "\n";
        message += "Khối lượng: " + DoubleToString(HistoryDealGetDouble(dealTicket, DEAL_VOLUME), 2) + "\n";
        message += "Lợi nhuận: " + DoubleToString(HistoryDealGetDouble(dealTicket, DEAL_PROFIT), 2) + " " + AccountInfoString(ACCOUNT_CURRENCY) + "\n";
        message += "Thời gian: " + TimeToString(HistoryDealGetInteger(dealTicket, DEAL_TIME) + TIME_VN, TIME_DATE|TIME_MINUTES);
        
        SendTelegramMessage(message);
        Sleep(DelayBetweenMessages);
    }
}

//+------------------------------------------------------------------+
//| Hàm gửi tin nhắn Telegram                                       |
//+------------------------------------------------------------------+
bool SendTelegramMessage(string message){
    string headers = "Content-Type: application/x-www-form-urlencoded";
    char data[], result[];
    string url = "https://api.telegram.org/bot" + TelegramToken + "/sendMessage";
    
    string request = "chat_id=" + ChatID + "&text=" + message;
    
    // Reset last error
    ResetLastError();
    
    // Chuyển đổi string sang mảng char
    int datalen = StringToCharArray(request, data, 0, WHOLE_ARRAY, CP_UTF8);
    if(datalen <= 0){
        if(NotifyOnError) Print("Lỗi chuyển đổi string to char array");
        return false;
    }
    
    // Gửi request POST
    int res = WebRequest("POST", url, headers, 5000, data, result, headers);
    
    if(res == -1){
        int errorCode = GetLastError();
        
        if(NotifyOnError){
            string errorMessage = "❌ Lỗi gửi Telegram: " + IntegerToString(errorCode) + "\n";
            errorMessage += "Mã lỗi: " + GetWebErrorDescription(errorCode);
            Print(errorMessage);
        }
        return false;
    }
    
    return true;
}

//+------------------------------------------------------------------+
//| Hàm lấy mô tả lỗi WebRequest                                    |
//+------------------------------------------------------------------+
string GetWebErrorDescription(int errorCode){
    switch(errorCode)
    {
        case 4001: return "Lỗi kết nối - không thể kết nối đến server";
        case 4002: return "Lỗi kết nối - không thể gửi dữ liệu";
        case 4003: return "Lỗi kết nối - không thể nhận dữ liệu";
        case 4004: return "Lỗi kết nối - timeout";
        case 4010: return "URL không được phép (chưa thêm vào danh sách cho phép)";
        case 4012: return "Không thể mở file";
        case 4013: return "Lỗi ghi file";
        case 4014: return "Lỗi đọc file";
        case 4015: return "Lỗi chuyển đổi dữ liệu";
        case 4016: return "Lỗi kết nối - không thể xác thực server";
        default: return "Lỗi không xác định";
    }
}

//+------------------------------------------------------------------+
//| Hàm chuyển đổi loại position thành string                       |
//+------------------------------------------------------------------+
string GetPositionTypeString(ENUM_ORDER_TYPE orderType){

    // Xác định loại lệnh
    switch(orderType){
        case ORDER_TYPE_BUY: return "LỆNH MUA";
        case ORDER_TYPE_SELL: return "LỆNH BÁN";
        case ORDER_TYPE_BUY_LIMIT: return "LỆNH MUA LIMIT";
        case ORDER_TYPE_SELL_LIMIT: return "LỆNH BÁN LIMIT";
        case ORDER_TYPE_BUY_STOP: return "LỆNH MUA STOP";
        case ORDER_TYPE_SELL_STOP: return "LỆNH BÁN STOP";
        default: return "LỆNH KHÁC";
    }

    return "";
}

//+------------------------------------------------------------------+
//| Định dạng mức giá (SL/TP)                                       |
//+------------------------------------------------------------------+
string FormatPriceLevel(string symbol, double price){
    if(price == 0) return "Chưa đặt";
    
    int digits = (int)SymbolInfoInteger(symbol, SYMBOL_DIGITS);
    return DoubleToString(price, digits);
}

//+------------------------------------------------------------------+
//| Hàm deinitialization                                            |
//+------------------------------------------------------------------+
void OnDeinit(const int reason){
    if(EnableNotifications)
    {
        string stopMessage = "🛑 EA Telegram Bot MT5 ĐÃ DỪNG\n";
        stopMessage += "Lý do: " + GetUninitReasonText(reason) + "\n";
        stopMessage += "Cặp tiền: " + Symbol() + "\n";
        stopMessage += "Thời gian: " + TimeToString(TimeCurrent(), TIME_DATE|TIME_MINUTES);
        
        SendTelegramMessage(stopMessage);
    }
}

//+------------------------------------------------------------------+
//| Hàm lấy lý do deinit                                            |
//+------------------------------------------------------------------+
string GetUninitReasonText(int reasonCode){
    switch(reasonCode)
    {
        case REASON_ACCOUNT:     return "Tài khoản thay đổi";
        case REASON_CHARTCHANGE: return "Chart thay đổi";
        case REASON_CHARTCLOSE:  return "Chart đóng";
        case REASON_PARAMETERS:  return "Tham số thay đổi";
        case REASON_RECOMPILE:   return "EA được biên dịch lại";
        case REASON_REMOVE:      return "EA bị xóa";
        case REASON_TEMPLATE:    return "Template thay đổi";
        case REASON_INITFAILED:  return "Khởi tạo thất bại";
        case REASON_CLOSE:       return "Platform đóng";
        default:                 return "Lý do khác";
    }
}

//+------------------------------------------------------------------+
//| Hàm TradeEvent (xử lý sự kiện giao dịch)                       |
//+------------------------------------------------------------------+
void OnTrade(){
    if(NotifyOnTrade)
    {
        // Xử lý real-time trade events
        CheckForNewPositions();
        CheckForNewDeals();
    }
}