// Thêm thư viện cần thiết
#include <Files\File.mqh>

// Hàm kiểm tra giới hạn thời gian
bool IsTrialExpired(){
    string filename = "License_Data.bin"; // Tên file lưu trữ ẩn
    datetime start_time = 0;
    int days_limit = 5; // Số ngày cho phép dùng thử
    long seconds_limit = days_limit * 24 * 60 * 60; // Đổi ra giây
   
    // Kiểm tra file lưu trữ
    int handle = FileOpen(filename, FILE_READ|FILE_BIN);
   
    if(handle != INVALID_HANDLE) {
        // Nếu file tồn tại, đọc ngày bắt đầu đã lưu
        start_time = FileReadLong(handle);
        FileClose(handle);
    } else {
        // Nếu file chưa có, đây là lần đầu chạy bot -> Tạo file mới
        handle = FileOpen(filename, FILE_WRITE|FILE_BIN);
        if(handle != INVALID_HANDLE) {
            start_time = TimeCurrent(); // Lưu thời điểm hiện tại
            FileWriteLong(handle, start_time);
            FileClose(handle);
            Print("Bot đã kích hoạt chế độ Trial 5 ngày.");
        }
    }
   
    // Kiểm tra logic thời gian
    if(TimeCurrent() - start_time > seconds_limit) {
        Alert("Giấy phép dùng thử đã hết hạn. Vui lòng liên hệ tác giả.");
        return true; // Đã hết hạn
    }
   
    return false; // Chưa hết hạn
}

// Cách gọi trong hàm OnInit của EA
int OnInit() {
    if(IsTrialExpired()) {
        ExpertRemove(); // Tự động tắt bot nếu hết hạn
        return(INIT_FAILED);
    }
   
   // Các code khác của bạn...
   return(INIT_SUCCEEDED);
}