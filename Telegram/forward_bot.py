import asyncio
from telethon import TelegramClient, events

# --- CẤU HÌNH ---
api_id = '26845740'          # api_id của bạn
api_hash = 'a6e00e485b0c1ddf905b13f7d9fe1b3e' # api_hash của bạn

# ID của nhóm nguồn (nơi lấy tin) và nhóm đích (nơi gửi đến)
# Mẹo: Để lấy ID, bạn có thể forward 1 tin nhắn vào bot @userinfobot
input_chat_id = -1003844825335   # ID nhóm nguồn (phải có dấu -100 ở đầu nếu là Supergroup)
output_chat_id = -1003836056810   # ID nhóm đích
# ----------------

client = TelegramClient('my_session', api_id, api_hash)

@client.on(events.NewMessage(chats=input_chat_id))
async def handler(event):
    try:
        # Gửi tin nhắn sang nhóm đích (dạng copy, không nhãn Forward)
        await client.send_message(output_chat_id, event.message)
        print(f"Đã copy tin nhắn mới: {event.raw_text[:30]}...")
    except Exception as e:
        print(f"Lỗi khi gửi: {e}")

async def main():
    # Khởi động client và đăng nhập nếu cần
    await client.start()
    print("Bot đã kết nối thành công và đang lắng nghe...")
    # Giữ cho script chạy liên tục
    await client.run_until_disconnected()

if __name__ == '__main__':
    try:
        # Cách chạy hiện đại để tránh lỗi 'no current event loop'
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nĐã dừng bot.")