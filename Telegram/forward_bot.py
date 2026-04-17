import asyncio
import pandas as pd
from telethon import TelegramClient, events

print("Reading file Excel...")

# --- ĐỌC DỮ LIỆU TỪ EXCEL ---
try:
    # Đọc file config.xlsx
    df = pd.read_excel('Data.xlsx')
    
    # Lấy dữ liệu từ dòng đầu tiên chứa dữ liệu (index 0)
    api_id = int(df['api_id'].iloc[0])
    api_hash = str(df['api_hash'].iloc[0])
    input_chat_id = int(df['source_id'].iloc[0])
    
    # Xử lý danh sách nhóm nhận (tách bằng dấu phẩy và loại bỏ khoảng trắng)
    dest_str = str(df['destination_ids'].iloc[0])
    output_chat_ids = [int(x.strip()) for x in dest_str.split(',')]
    
except FileNotFoundError:
    print("LỖI: Không tìm thấy file 'Data.xlsx'. Vui lòng tạo file này trong cùng thư mục.")
    exit()
except Exception as e:
    print(f"LỖI KHI ĐỌC EXCEL: {e}")
    print("Vui lòng kiểm tra lại tên cột (api_id, api_hash, source_id, destination_ids) và định dạng dữ liệu.")
    exit()
# -----------------------------

# Khởi tạo Client
client = TelegramClient('my_session', api_id, api_hash)

@client.on(events.NewMessage(chats=input_chat_id))
async def handler(event):
    print(f"\n[+] Có tin nhắn mới: {event.raw_text[:30]}...")
    
    # Lặp qua từng ID trong danh sách nhóm nhận để gửi tin
    for out_id in output_chat_ids:
        try:
            await client.send_message(out_id, event.message)
            print(f"  -> Đã chuyển tiếp đến: {out_id}")
        except Exception as e:
            print(f"  -> LỖI khi gửi đến {out_id}: {e}")

async def main():
    await client.start()
    print("\n==================================================")
    print("BOT ĐÃ KẾT NỐI VÀ ĐANG LẮNG NGHE...")
    print(f"- Nhóm nguồn: {input_chat_id}")
    print(f"- Số nhóm nhận: {len(output_chat_ids)} nhóm")
    print(f"- Danh sách nhóm nhận: {output_chat_ids}")
    print("==================================================")
    await client.run_until_disconnected()

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nĐã dừng bot an toàn.")