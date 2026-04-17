import asyncio
import pandas as pd
import sys
from telethon import TelegramClient, events, errors

# --- HÀM ĐỌC CẤU HÌNH ---
def load_config(file_path='Data.xlsx'):
    try:
        df = pd.read_excel(file_path)
        config = {
            'api_id': int(df['api_id'].iloc[0]),
            'api_hash': str(df['api_hash'].iloc[0]).strip(),
            'source_id': int(df['source_id'].iloc[0]),
            'dest_ids': [int(x.strip()) for x in str(df['destination_ids'].iloc[0]).split(',')]
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

# --- XỬ LÝ CHUYỂN TIẾP ---
@client.on(events.NewMessage(chats=cfg['source_id']))
async def handler(event):
    content = event.raw_text[:50].replace('\n', ' ')
    print(f"\n📩 Tin nhắn mới: {content}...")

    # Tối ưu: Gửi song song đến tất cả các nhóm đích cùng lúc
    tasks = []
    for out_id in cfg['dest_ids']:
        tasks.append(send_to_group(out_id, event.message))
    
    await asyncio.gather(*tasks)

async def send_to_group(chat_id, message):
    try:
        await client.send_message(chat_id, message)
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
        print("\n" + "="*50)
        print(f"✅ BOT ĐANG CHẠY DƯỚI TÊN: {me.first_name}")
        print(f"📢 Nguồn gửi: {cfg['source_id']}")
        print(f"🎯 Đích đến: {len(cfg['dest_ids'])} nhóm")
        print("="*50)
        
        await client.run_until_disconnected()
    except Exception as e:
        print(f"💥 Lỗi hệ thống: {e}")

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n🛑 Đã dừng bot.")