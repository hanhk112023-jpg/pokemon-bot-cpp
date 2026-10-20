#include "bot.hpp"
#include "game.hpp"
#include <csignal>
#include <chrono>
#include <thread>
#include <cstdlib>

using json = nlohmann::json;

static bool g_running = true;

void SignalHandler(int signum) {
    std::cout << "\n[SIGNAL] Received signal " << signum << ", shutting down gracefully...\n";
    g_running = false;
}

// Helper to generate a text HP progress bar
std::string GenerateHpBar(int current, int maxHp) {
    int totalBars = 10;
    int filled = std::max(0, std::min(totalBars, (int)std::round(((double)current / maxHp) * totalBars)));
    std::string bar = "";
    for (int i = 0; i < filled; ++i) bar += "█";
    for (int i = filled; i < totalBars; ++i) bar += "░";
    return "[" + bar + "] " + std::to_string(current) + "/" + std::to_string(maxHp);
}

std::string RepeatString(const std::string& str, int count) {
    std::string res = "";
    for (int i = 0; i < count; ++i) res += str;
    return res;
}

// Generate Main Menu Markup
json CreateMainMenuMarkup() {
    return {
        {"inline_keyboard", {
            {
                {{"text", "🎮 Chơi Trong Mini App (Đồ Họa 3D)", {"web_app", {{"url", "https://hanhk112023-jpg.github.io/pokemon-twa/"}}}}}
            },
            {
                {{"text", "📦 Mở Thẻ Bài (Gacha)", {"callback_data", "menu_gacha"}}},
                {{"text", "⚔️ Đấu Trường (Battle)", {"callback_data", "menu_battle"}}}
            },
            {
                {{"text", "🎒 Túi Thẻ & Đội Hình", {"callback_data", "menu_bag"}}},
                {{"text", "🎁 Điểm Danh Nhận Xu", {"callback_data", "menu_daily"}}}
            },
            {
                {{"text", "📊 Hồ Sơ Huấn Luyện Viên", {"callback_data", "menu_profile"}}}
            }
        }}
    };
}

// Generate Battle Action Buttons
json CreateBattleButtons(const pkmn::BattleSession& b) {
    return {
        {"inline_keyboard", {
            {
                {{"text", "⚔️ Tấn Công", {"callback_data", "action:attack"}}},
                {{"text", "⚡ Tuyệt Kỹ (2 EP)", {"callback_data", "action:skill"}}}
            },
            {
                {{"text", "🛡️ Phòng Thủ (+1 EP)", {"callback_data", "action:shield"}}},
                {{"text", "🧪 Dược Phẩm (" + std::to_string(b.potionCharges) + ")", {"callback_data", "action:potion"}}}
            },
            {
                {{"text", "🏃 Bỏ Chạy", {"callback_data", "action:flee"}}}
            }
        }}
    };
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Retrieve Token
    const char* tokenEnv = std::getenv("TELEGRAM_BOT_TOKEN");
    std::string token = tokenEnv ? tokenEnv : "";

    int maxRunSeconds = 0; // 0 = run forever
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--token" && i + 1 < argc) {
            token = argv[++i];
        } else if (arg == "--max-seconds" && i + 1 < argc) {
            maxRunSeconds = std::stoi(argv[++i]);
        }
    }

    if (token.empty()) {
        std::cerr << "Lỗi: Chưa cung cấp TELEGRAM_BOT_TOKEN! Hãy đặt biến môi trường hoặc dùng --token <TOKEN>\n";
        return 1;
    }

    std::cout << "========================================\n";
    std::cout << " ⚡ POKÉMON C++ TELEGRAM BOT STARTING ⚡\n";
    std::cout << "========================================\n";

    tg::Bot bot(token);
    pkmn::GameManager game;

    std::string dataPath = "data/players.json";
    if (game.loadFromFile(dataPath)) {
        std::cout << "[INFO] Loaded player data from " << dataPath << "\n";
    } else {
        std::cout << "[INFO] Initialized new player data repository.\n";
    }

    long long lastUpdateId = 0;
    auto startTime = std::chrono::steady_clock::now();
    auto lastSaveTime = startTime;

    std::cout << "[READY] Bot is listening for updates...\n";

    while (g_running) {
        // Auto exit after maxRunSeconds (for GitHub Actions runner rotation)
        if (maxRunSeconds > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime).count();
            if (elapsed >= maxRunSeconds) {
                std::cout << "[TIMEOUT] Reached max run duration (" << maxRunSeconds << "s). Saving and exiting cleanly for next workflow cycle.\n";
                break;
            }
        }

        // Periodic auto save every 60s
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSaveTime).count() >= 60) {
            game.saveToFile(dataPath);
            lastSaveTime = now;
        }

        json res = bot.getUpdates(lastUpdateId + 1, 20);
        if (!res.value("ok", false)) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        const auto& updates = res["result"];
        for (const auto& update : updates) {
            long long updateId = update.value("update_id", 0LL);
            if (updateId > lastUpdateId) lastUpdateId = updateId;

            // 1. Process Messages (Commands)
            if (update.contains("message")) {
                const auto& msg = update["message"];
                if (!msg.contains("from") || !msg.contains("text")) continue;

                long long chatId = msg["chat"]["id"];
                long long userId = msg["from"]["id"];
                std::string firstName = msg["from"].value("first_name", "Trainer");
                std::string username = msg["from"].value("username", "");
                std::string text = msg.value("text", "");

                auto& player = game.getOrCreatePlayer(userId, firstName, username);

                if (text == "/start" || text == "/menu") {
                    std::string menuText = "⚡ *CHÀO MỪNG ĐẾN VỚI POKÉMON CARD BATTLE* ⚡\n\n"
                                           "Xin chào *" + firstName + "*!\n"
                                           "🪙 *Số Dư:* `" + std::to_string(player.coins) + " Xu`\n"
                                           "🃏 *Bộ Sưu Tập:* `" + std::to_string(player.cards.size()) + " Thẻ bài`\n"
                                           "🏆 *Thành Tích:* `" + std::to_string(player.battlesWon) + " Thắng / " + std::to_string(player.battlesLost) + " Thua`\n\n"
                                           "Hãy chọn tính năng bạn muốn tham gia bên dưới:";
                    bot.sendMessage(chatId, menuText, CreateMainMenuMarkup());
                } else if (text == "/daily") {
                    auto resClaim = game.claimDaily(userId);
                    bot.sendMessage(chatId, resClaim.second);
                } else if (text == "/gacha") {
                    std::string gachaText = "📦 *CỬA HÀNG GÓI THẺ BOOSTER PACK* 📦\n\n"
                                            "🪙 Số dư của bạn: `" + std::to_string(player.coins) + " Xu`\n\n"
                                            "1️⃣ *Poké Pack (100 Xu)*: 1 thẻ bài ngẫu nhiên (Tỷ lệ Common cao, cơ hội mở Rare).\n"
                                            "2️⃣ *Great Pack (250 Xu)*: 3 thẻ bài (Đảm bảo ít nhất 1 Rare hoặc Epic trở lên).\n"
                                            "3️⃣ *Master Mythic Pack (500 Xu)*: 5 thẻ bài (Tỷ lệ cao mở Legendary & Mythic!).";
                    json markup = {
                        {"inline_keyboard", {
                            {
                                {{"text", "🥉 Mở Poké Pack (100🪙)", {"callback_data", "pack:poke_pack"}}}
                            },
                            {
                                {{"text", "🥈 Mở Great Pack (250🪙)", {"callback_data", "pack:great_pack"}}}
                            },
                            {
                                {{"text", "🥇 Mở Master Pack (500🪙)", {"callback_data", "pack:master_pack"}}}
                            },
                            {
                                {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                            }
                        }}
                    };
                    bot.sendMessage(chatId, gachaText, markup);
                } else if (text == "/battle") {
                    const auto& leaders = pkmn::GetGymLeaders();
                    std::string arenaText = "🏟️ *ĐẤU TRƯỜNG KANTO - THỦ LĨNH NHÀ THI ĐẤU* 🏟️\n\n"
                                            "Hãy chọn đối thủ để khiêu chiến và nhận Xu thưởng + EXP:";
                    json kb = json::array();
                    for (const auto& l : leaders) {
                        kb.push_back({
                            {{"text", l.name + " (" + l.difficulty + " - Thưởng " + std::to_string(l.rewardCoins) + "🪙)", {"callback_data", "battle_start:" + l.id}}}
                        });
                    }
                    kb.push_back({
                        {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                    });
                    bot.sendMessage(chatId, arenaText, {{"inline_keyboard", kb}});
                } else if (text == "/bag") {
                    std::string bagText = "🎒 *TÚI THẺ BÀI CỦA BẠN* 🎒\n\n"
                                          "Tổng số thẻ: *" + std::to_string(player.cards.size()) + "*\n\n";

                    json kb = json::array();
                    for (size_t i = 0; i < player.cards.size() && i < 8; ++i) {
                        const auto& pc = player.cards[i];
                        const auto* base = game.findBaseCard(pc.pokemonId);
                        if (!base) continue;

                        bool isActive = (pc.instanceId == player.activeInstanceId);
                        bagText += (isActive ? "⭐ *[ĐỘI TRƯỞNG]* " : "▫️ ") + base->name + " (Lv." + std::to_string(pc.level) + ") - " + pkmn::RarityToString(base->rarity) + " | HP: " + std::to_string(base->hp) + " | ATK: " + std::to_string(base->atk) + "\n";

                        if (!isActive) {
                            kb.push_back({
                                {{"text", "⚔️ Chọn " + base->name + " Xuất Trận", {"callback_data", "set_active:" + pc.instanceId}}}
                            });
                        }
                    }
                    kb.push_back({
                        {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                    });
                    bot.sendMessage(chatId, bagText, {{"inline_keyboard", kb}});
                }
            }

            // 2. Process Callback Queries (Inline Button Clicks)
            if (update.contains("callback_query")) {
                const auto& cb = update["callback_query"];
                std::string cbId = cb.value("id", "");
                std::string data = cb.value("data", "");
                long long userId = cb["from"]["id"];
                std::string firstName = cb["from"].value("first_name", "Trainer");
                std::string username = cb["from"].value("username", "");
                long long chatId = cb["message"]["chat"]["id"];
                long long messageId = cb["message"]["message_id"];

                auto& player = game.getOrCreatePlayer(userId, firstName, username);
                bot.answerCallbackQuery(cbId);

                if (data == "main_menu") {
                    std::string menuText = "⚡ *CHÀO MỪNG ĐẾN VỚI POKÉMON CARD BATTLE* ⚡\n\n"
                                           "Xin chào *" + firstName + "*!\n"
                                           "🪙 *Số Dư:* `" + std::to_string(player.coins) + " Xu`\n"
                                           "🃏 *Bộ Sưu Tập:* `" + std::to_string(player.cards.size()) + " Thẻ bài`\n"
                                           "🏆 *Thành Tích:* `" + std::to_string(player.battlesWon) + " Thắng / " + std::to_string(player.battlesLost) + " Thua`\n\n"
                                           "Hãy chọn tính năng bạn muốn tham gia bên dưới:";
                    bot.editMessageText(chatId, messageId, menuText, CreateMainMenuMarkup());
                } else if (data == "menu_gacha") {
                    std::string gachaText = "📦 *CỬA HÀNG GÓI THẺ BOOSTER PACK* 📦\n\n"
                                            "🪙 Số dư của bạn: `" + std::to_string(player.coins) + " Xu`\n\n"
                                            "1️⃣ *Poké Pack (100 Xu)*: 1 thẻ bài ngẫu nhiên.\n"
                                            "2️⃣ *Great Pack (250 Xu)*: 3 thẻ bài (Đảm bảo Rare/Epic trở lên).\n"
                                            "3️⃣ *Master Mythic Pack (500 Xu)*: 5 thẻ bài (Tỷ lệ cao mở Legendary & Mythic!).";
                    json markup = {
                        {"inline_keyboard", {
                            {
                                {{"text", "🥉 Mở Poké Pack (100🪙)", {"callback_data", "pack:poke_pack"}}}
                            },
                            {
                                {{"text", "🥈 Mở Great Pack (250🪙)", {"callback_data", "pack:great_pack"}}}
                            },
                            {
                                {{"text", "🥇 Mở Master Pack (500🪙)", {"callback_data", "pack:master_pack"}}}
                            },
                            {
                                {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                            }
                        }}
                    };
                    bot.editMessageText(chatId, messageId, gachaText, markup);
                } else if (data.rfind("pack:", 0) == 0) {
                    std::string packType = data.substr(5);
                    auto gachaRes = game.openGacha(userId, packType);

                    if (!gachaRes.success) {
                        bot.sendMessage(chatId, "❌ " + gachaRes.message, CreateMainMenuMarkup());
                    } else {
                        // Send last card as full photo preview
                        const auto* lastCard = gachaRes.cardsPulled.back();
                        std::string caption = "✨ *BẠN VỪA MỞ ĐƯỢC THẺ BÀI:* ✨\n\n"
                                              "🎴 *" + lastCard->name + "* - `" + pkmn::RarityToString(lastCard->rarity) + "`\n"
                                              "🔹 *Hệ:* " + pkmn::TypeToEmoji(lastCard->type) + "\n"
                                              "❤️ *Máu (HP):* `" + std::to_string(lastCard->hp) + "` | ⚔️ *Tấn Công:* `" + std::to_string(lastCard->atk) + "`\n"
                                              "🛡️ *Phòng Thủ:* `" + std::to_string(lastCard->def) + "` | ⚡ *Tốc Độ:* `" + std::to_string(lastCard->spd) + "`\n\n"
                                              "💥 *Kỹ Năng:* " + lastCard->moves[0].name + " (Sát thương " + std::to_string(lastCard->moves[0].power) + ")\n"
                                              "🌟 *Tuyệt Kỹ:* " + lastCard->moves[1].name + " (Sát thương " + std::to_string(lastCard->moves[1].power) + ")\n\n"
                                              "📖 _\"" + lastCard->flavor + "\"_\n\n"
                                              "🪙 Số dư còn lại: `" + std::to_string(player.coins) + " Xu`";

                        json postGachaButtons = {
                            {"inline_keyboard", {
                                {
                                    {{"text", "📦 Mở Thêm Gói Này", {"callback_data", "pack:" + packType}}},
                                    {{"text", "🎒 Xem Bộ Sưu Tập", {"callback_data", "menu_bag"}}}
                                },
                                {
                                    {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                                }
                            }}
                        };
                        bot.sendPhoto(chatId, lastCard->getImageUrl(), caption, postGachaButtons);
                    }
                } else if (data == "menu_battle") {
                    const auto& leaders = pkmn::GetGymLeaders();
                    std::string arenaText = "🏟️ *ĐẤU TRƯỜNG KANTO - THỦ LĨNH NHÀ THI ĐẤU* 🏟️\n\n"
                                            "Chọn đối thủ bạn muốn khiêu chiến:";
                    json kb = json::array();
                    for (const auto& l : leaders) {
                        kb.push_back({
                            {{"text", l.name + " (" + l.difficulty + " - " + std::to_string(l.rewardCoins) + "🪙)", {"callback_data", "battle_start:" + l.id}}}
                        });
                    }
                    kb.push_back({
                        {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                    });
                    bot.editMessageText(chatId, messageId, arenaText, {{"inline_keyboard", kb}});
                } else if (data.rfind("battle_start:", 0) == 0) {
                    std::string leaderId = data.substr(13);
                    auto bStart = game.startBattle(userId, leaderId);
                    if (!bStart.first) {
                        bot.sendMessage(chatId, "❌ " + bStart.second, CreateMainMenuMarkup());
                    } else {
                        const auto& b = player.currentBattle;
                        std::string battleText = "🏟️ *TRẬN ĐẤU BẮT ĐẦU:* 🏟️\n\n"
                                                 "🔴 *ĐỐI THỦ:* `" + b.enemyName + "`\n"
                                                 "HP: `" + GenerateHpBar(b.enemyHp, b.enemyMaxHp) + "`\n\n"
                                                 "-----------------------------------\n\n"
                                                 "🔵 *BẠN:* `" + b.playerName + "`\n"
                                                 "HP: `" + GenerateHpBar(b.playerHp, b.playerMaxHp) + "`\n"
                                                 "⚡ Năng lượng: `" + RepeatString("⚡", b.playerEnergy) + "` | 🧪 Dược phẩm: `" + std::to_string(b.potionCharges) + "`\n\n"
                                                 "📝 *Diễn biến:* _" + b.log + "_";
                        bot.editMessageText(chatId, messageId, battleText, CreateBattleButtons(b));
                    }
                } else if (data.rfind("action:", 0) == 0) {
                    std::string action = data.substr(7);
                    auto actRes = game.executeBattleAction(userId, action);

                    const auto& b = player.currentBattle;
                    if (actRes.finished) {
                        json endButtons = {
                            {"inline_keyboard", {
                                {
                                    {{"text", "⚔️ Đấu Trận Khác", {"callback_data", "menu_battle"}}},
                                    {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                                }
                            }}
                        };
                        bot.editMessageText(chatId, messageId, actRes.summary, endButtons);
                    } else {
                        std::string battleText = "🏟️ *ĐẤU TRƯỜNG KANTO ĐANG DIỄN RA* 🏟️\n\n"
                                                 "🔴 *ĐỐI THỦ:* `" + b.enemyName + "`\n"
                                                 "HP: `" + GenerateHpBar(b.enemyHp, b.enemyMaxHp) + "`\n\n"
                                                 "-----------------------------------\n\n"
                                                 "🔵 *BẠN:* `" + b.playerName + "`\n"
                                                 "HP: `" + GenerateHpBar(b.playerHp, b.playerMaxHp) + "`\n"
                                                 "⚡ Năng lượng: `" + (b.playerEnergy > 0 ? RepeatString("⚡", b.playerEnergy) : "Trống") + "` | 🧪 Dược phẩm: `" + std::to_string(b.potionCharges) + "`\n\n"
                                                 "📝 *Diễn biến mới nhất:*\n_" + b.log + "_";
                        bot.editMessageText(chatId, messageId, battleText, CreateBattleButtons(b));
                    }
                } else if (data.rfind("set_active:", 0) == 0) {
                    std::string cardId = data.substr(11);
                    player.activeInstanceId = cardId;
                    bot.answerCallbackQuery(cbId, "Đã chọn Pokémon vào đội hình chính!", false);

                    // Re-render bag
                    std::string bagText = "🎒 *TÚI THẺ BÀI CỦA BẠN* 🎒\n\n"
                                          "Đã cập nhật Đội Trưởng Xuất Trận!\n\n";
                    json kb = json::array();
                    for (size_t i = 0; i < player.cards.size() && i < 8; ++i) {
                        const auto& pc = player.cards[i];
                        const auto* base = game.findBaseCard(pc.pokemonId);
                        if (!base) continue;

                        bool isActive = (pc.instanceId == player.activeInstanceId);
                        bagText += (isActive ? "⭐ *[ĐỘI TRƯỞNG]* " : "▫️ ") + base->name + " (Lv." + std::to_string(pc.level) + ") - " + pkmn::RarityToString(base->rarity) + "\n";
                        if (!isActive) {
                            kb.push_back({
                                {{"text", "⚔️ Chọn " + base->name + " Xuất Trận", {"callback_data", "set_active:" + pc.instanceId}}}
                            });
                        }
                    }
                    kb.push_back({
                        {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                    });
                    bot.editMessageText(chatId, messageId, bagText, {{"inline_keyboard", kb}});
                } else if (data == "menu_bag") {
                    std::string bagText = "🎒 *TÚI THẺ BÀI CỦA BẠN* 🎒\n\n"
                                          "Tổng số thẻ: *" + std::to_string(player.cards.size()) + "*\n\n";

                    json kb = json::array();
                    for (size_t i = 0; i < player.cards.size() && i < 8; ++i) {
                        const auto& pc = player.cards[i];
                        const auto* base = game.findBaseCard(pc.pokemonId);
                        if (!base) continue;

                        bool isActive = (pc.instanceId == player.activeInstanceId);
                        bagText += (isActive ? "⭐ *[ĐỘI TRƯỞNG]* " : "▫️ ") + base->name + " (Lv." + std::to_string(pc.level) + ") - " + pkmn::RarityToString(base->rarity) + " | HP: " + std::to_string(base->hp) + " | ATK: " + std::to_string(base->atk) + "\n";

                        if (!isActive) {
                            kb.push_back({
                                {{"text", "⚔️ Chọn " + base->name + " Xuất Trận", {"callback_data", "set_active:" + pc.instanceId}}}
                            });
                        }
                    }
                    kb.push_back({
                        {{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}
                    });
                    bot.editMessageText(chatId, messageId, bagText, {{"inline_keyboard", kb}});
                } else if (data == "menu_daily") {
                    auto resClaim = game.claimDaily(userId);
                    json kb = {
                        {"inline_keyboard", {
                            {{{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}}
                        }}
                    };
                    bot.editMessageText(chatId, messageId, resClaim.second, kb);
                } else if (data == "menu_profile") {
                    int totalBattles = player.battlesWon + player.battlesLost;
                    int winRate = totalBattles > 0 ? (player.battlesWon * 100) / totalBattles : 0;

                    std::string profileText = "📊 *HỒ SƠ HUẤN LUYỆN VIÊN* 📊\n\n"
                                              "👤 *Tên:* `" + firstName + "`\n"
                                              "🆔 *Telegram ID:* `" + std::to_string(userId) + "`\n"
                                              "🪙 *Xu:* `" + std::to_string(player.coins) + " 🪙`\n"
                                              "🃏 *Số thẻ bài sở hữu:* `" + std::to_string(player.cards.size()) + "`\n"
                                              "📦 *Gói thẻ đã mở:* `" + std::to_string(player.packsOpened) + "`\n\n"
                                              "🏆 *Trận thắng:* `" + std::to_string(player.battlesWon) + "`\n"
                                              "💀 *Trận thua:* `" + std::to_string(player.battlesLost) + "`\n"
                                              "📈 *Tỷ lệ thắng:* `" + std::to_string(winRate) + "%`";
                    json kb = {
                        {"inline_keyboard", {
                            {{{"text", "🔙 Về Menu Chính", {"callback_data", "main_menu"}}}}
                        }}
                    };
                    bot.editMessageText(chatId, messageId, profileText, kb);
                }
            }
        }
    }

    std::cout << "[SHUTDOWN] Saving player state to " << dataPath << "...\n";
    game.saveToFile(dataPath);
    std::cout << "[DONE] Player state saved. Bye!\n";
    return 0;
}
