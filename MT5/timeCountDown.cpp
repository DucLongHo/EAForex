//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
const int ONE = 1;

int OnInit(){
   EventSetTimer(ONE);
   return (INIT_SUCCEEDED);
}

void OnTimer(){

   datetime closeTime = iTime(_Symbol, PERIOD_CURRENT, 0) + PeriodSeconds(PERIOD_CURRENT);
   string timeString = TimeToString(closeTime - TimeCurrent(), TIME_SECONDS);

   Comment("Đếm ngược thời gian đóng nến: ", timeString, "\n", "\n");
}

void OnDeinit(const int reason){
   EventKillTimer();
}
