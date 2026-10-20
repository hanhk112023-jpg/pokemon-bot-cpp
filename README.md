# ⚡ Pokémon Card Battle — C++ Telegram Bot on GitHub Actions

Game thẻ bài Pokémon hoàn chỉnh viết bằng **C++17**, tương tác qua **Telegram Bot** với Inline Keyboards, ảnh minh họa chính hãng và chạy tự động hoàn toàn 24/7 trên **GitHub Actions**.

🔗 **Kho mã nguồn:** [https://github.com/hanhk112023-jpg/pokemon-bot-cpp](https://github.com/hanhk112023-jpg/pokemon-bot-cpp)

---

## 🌟 Tính Năng Trò Chơi

1. **📦 Mở Thẻ Bài (Gacha System):**
   - **Poké Pack (100 Xu):** 1 thẻ ngẫu nhiên, tỷ lệ Common cao.
   - **Great Pack (250 Xu):** 3 thẻ, đảm bảo Rare hoặc Epic trở lên.
   - **Master Mythic Pack (500 Xu):** 5 thẻ, tỷ lệ mở Legendary & Mythic (Mewtwo, Rayquaza, Arceus, Ho-Oh...).
   - Bot tự động gửi ảnh thẻ bài chính thức chất lượng cao từ PokeAPI kèm khung thẻ, chỉ số HP/ATK/DEF/SPD, hệ và kỹ năng.

2. **⚔️ Đấu Trường Chiến Đấu Theo Lượt (Turn-Based Battle):**
   - Đấu trường tương tác trực tiếp trên tin nhắn Telegram bằng Inline Buttons (tự cập nhật tin nhắn không bị spam chat).
   - Thanh máu ASCII trực quan: `[████████░░] 80/100 HP`.
   - Các hành động trong trận:
     - ⚔️ **Tấn Công:** Gây sát thương vật lý và nạp +1 EP (Năng lượng).
     - ⚡ **Tuyệt Kỹ Hệ (2 EP):** Tung đòn bộc phá x2 sát thương nếu khắc hệ.
     - 🛡️ **Phòng Thủ:** Giảm 50% sát thương nhận vào ở lượt kế tiếp và hồi +1 EP.
     - 🧪 **Dược Phẩm:** Hồi phục 35% HP (2 lần dùng mỗi trận).
     - 🏃 **Bỏ Chạy.**
   - Thủ lĩnh Nhà thi đấu Kanto: Brock, Misty, Lt. Surge, Sabrina, Blaine và Quán Quân Red.
   - Thắng trận nhận Xu thưởng và EXP thăng cấp thẻ bài.

3. **🎒 Túi Thẻ Bài & Đội Hình:**
   - Xem toàn bộ thẻ bài đang sở hữu kèm cấp độ và phẩm cấp.
   - Đổi thẻ Pokémon làm Đội Trưởng Xuất Trận chính.

4. **🎁 Điểm Danh Hằng Ngày & Hồ Sơ:**
   - `/daily`: Nhận 150 Xu miễn phí sau mỗi 12 giờ.
   - `/profile`: Thống kê tỷ lệ thắng/thua, tổng trận đấu, số gói đã mở.

---

## 🚀 Cơ Chế Chạy 24/7 Trên GitHub Actions

GitHub Actions giới hạn tối đa 6 giờ cho mỗi lần chạy workflow:
- Bot C++ được biên dịch trực tiếp trên Ubuntu runner với `libcurl` và `nlohmann-json`.
- Tham số `--max-seconds 18000` (5 giờ): Bot chạy liên tục trong 5 giờ.
- Khi hết chu kỳ 5 giờ: Bot thoát an toàn, tự động `git commit` và `git push` file `data/players.json` lên repository để **lưu trữ vĩnh viễn tiến độ của người chơi**.
- Bước cuối workflow tự động gọi `gh workflow run bot.yml` để **kích hoạt chu kỳ 5 giờ tiếp theo**, tạo thành một vòng lặp vĩnh cửu không bao giờ tắt.

---

## 🛠️ Hướng Dẫn Kích Hoạt Bot Trên GitHub Actions

### Bước 1: Lấy Token Bot Telegram
1. Mở bot **[@BotFather](https://t.me/BotFather)** trên Telegram.
2. Gửi lệnh `/newbot`, đặt tên và username cho bot.
3. Sao chép đoạn `HTTP API Token` (dạng `123456789:ABCdefGhI...`).

### Bước 2: Cài Đặt Secret Trên GitHub
1. Truy cập repository: [https://github.com/hanhk112023-jpg/pokemon-bot-cpp](https://github.com/hanhk112023-jpg/pokemon-bot-cpp)
2. Chọn tab **Settings** -> **Secrets and variables** -> **Actions**.
3. Nhấp nút **New repository secret**:
   - **Name:** `TELEGRAM_BOT_TOKEN`
   - **Secret:** *(Dán token từ BotFather vào đây)*
4. Nhấp **Add secret**.

### Bước 3: Cho Phép Workflow Ghi Dữ Liệu & Kích Hoạt
1. Trong tab **Settings** -> **Actions** -> **General** -> mục **Workflow permissions**:
   - Chọn **Read and write permissions**.
   - Tích chọn **Allow GitHub Actions to create and approve pull requests**.
   - Bấm **Save**.
2. Chuyển sang tab **Actions** trên GitHub.
3. Chọn workflow **Run Pokémon C++ Telegram Bot** ở cột bên trái.
4. Bấm nút **Run workflow** -> **Run workflow**.

Bot sẽ tự động build C++, khởi động và hoạt động 24/7 trên Telegram của bạn!
