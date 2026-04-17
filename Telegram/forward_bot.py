import asyncio
import pandas as pd
import sys
from telethon import TelegramClient, events, errors
from deep_translator import GoogleTranslator

# --- HÀM ĐỌC CẤU HÌNH ---
def load_config(file_path='Data.xlsx'):
    try:
        df = pd.read_excel(file_path)
        config = {
            'api_id': int(df['Mã_API'].iloc[0]),
            'api_hash': str(df['Chuỗi_API'].iloc[0]).strip(),
            'source_id': int(df['ID_Nguồn'].iloc[0]),
            'dest_ids': [int(x.strip()) for x in str(df['Danh_Sách_ID_Nhận'].iloc[0]).split(',')]
        }
        return config
    except FileNotFoundError:
        print(f"❌ LỖI: Không tìm thấy file '{file_path}'")
        sys.exit(1)
    except PermissionError:
        print(f"❌ LỖI: File '{file_path}' đang mở trong Excel. Hãy đóng nó lại!")
        sys.exit(1)
    except Exception as e:
        print(f"❌ LỖI CẤU HÌNH: {e}")
        sys.exit(1)

# --- KHỞI TẠO ---
cfg = load_config()
client = TelegramClient('my_session', cfg['api_id'], cfg['api_hash'], 
                        connection_retries=None, # Thử lại vô hạn khi mất mạng
                        auto_reconnect=True)

# --- HÀM DỊCH THUẬT ---
def translate_text(text):
    try:
        if not text or len(text.strip()) == 0:
            return text
        # Dịch từ Tiếng Việt (vi) sang Tiếng Anh (en)
        translated = GoogleTranslator(source='auto', target='en').translate(text)
        return translated
    except Exception as e:
        print(f"⚠️ Lỗi dịch thuật: {e}")
        return text  # Nếu lỗi thì gửi tin gốc để không bị gián đoạn
    
# --- XỬ LÝ CHUYỂN TIẾP ---
@client.on(events.NewMessage(chats=cfg['source_id']))
async def handler(event):
    print(f"\n📩 Tin nhắn mới.")

    original_text = event.raw_text
    
    if original_text:
        translated_text = translate_text(original_text)
    else:
        translated_text = ""
    
    # Tối ưu: Gửi song song đến tất cả các nhóm đích cùng lúc
    tasks = []
    for out_id in cfg['dest_ids']:
        tasks.append(send_to_group(out_id, translated_text, event.media))
    
    await asyncio.gather(*tasks)

async def send_to_group(chat_id, message, media):
    try:
        await client.send_message(chat_id, message, file=media)
        print(f" ✅ Gửi thành công -> {chat_id}")
    except errors.FloodWaitError as e:
        print(f" ⏳ Bị giới hạn tốc độ. Chờ {e.seconds} giây...")
        await asyncio.sleep(e.seconds)
    except Exception as e:
        print(f" ❌ Lỗi tại nhóm {chat_id}: {type(e).__name__}")

# --- CHƯƠNG TRÌNH CHÍNH ---
async def main():
    try:
        # Kiểm tra kết nối
        await client.start()
        
        me = await client.get_me()
        print("\n" + "="*48)
        print(f"✅ BOT ĐANG CHẠY DƯỚI TÊN: {me.first_name}")
        print(f"📢 Nguồn gửi: {cfg['source_id']}")
        print(f"🎯 Đích đến: {len(cfg['dest_ids'])} nhóm")
        print("="*48)
        
        await client.run_until_disconnected()
    except Exception as e:
        print(f"💥 Lỗi hệ thống: {e}")

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Đã dừng bot.")