#pragma once

#include "pokemon.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace pkmn {

struct PlayerCard {
    std::string instanceId;
    int pokemonId;
    int level;
    int exp;
    std::string pulledAt;

    json toJson() const {
        return {
            {"instanceId", instanceId},
            {"pokemonId", pokemonId},
            {"level", level},
            {"exp", exp},
            {"pulledAt", pulledAt}
        };
    }

    static PlayerCard fromJson(const json& j) {
        PlayerCard c;
        c.instanceId = j.value("instanceId", "c_0");
        c.pokemonId = j.value("pokemonId", 25);
        c.level = j.value("level", 1);
        c.exp = j.value("exp", 0);
        c.pulledAt = j.value("pulledAt", "");
        return c;
    }
};

struct BattleSession {
    bool active = false;
    std::string leaderId;
    int enemyPokemonId = 0;
    std::string enemyName;
    std::string enemyType;
    int enemyHp = 0;
    int enemyMaxHp = 0;
    int enemyAtk = 0;
    int enemyDef = 0;
    bool enemyShield = false;

    std::string playerInstanceId;
    std::string playerName;
    std::string playerType;
    int playerHp = 0;
    int playerMaxHp = 0;
    int playerAtk = 0;
    int playerDef = 0;
    int playerEnergy = 1;
    bool playerShield = false;
    int potionCharges = 2;
    std::string turn = "player";
    std::string log;
};

struct Player {
    long long userId;
    std::string firstName;
    std::string username;
    int coins = 300;
    std::string activeInstanceId;
    std::vector<PlayerCard> cards;
    int battlesWon = 0;
    int battlesLost = 0;
    int packsOpened = 0;
    long long lastDailyClaim = 0;
    BattleSession currentBattle;

    json toJson() const {
        json cardArr = json::array();
        for (const auto& c : cards) cardArr.push_back(c.toJson());

        return {
            {"userId", userId},
            {"firstName", firstName},
            {"username", username},
            {"coins", coins},
            {"activeInstanceId", activeInstanceId},
            {"cards", cardArr},
            {"battlesWon", battlesWon},
            {"battlesLost", battlesLost},
            {"packsOpened", packsOpened},
            {"lastDailyClaim", lastDailyClaim}
        };
    }

    static Player fromJson(const json& j) {
        Player p;
        p.userId = j.value("userId", 0LL);
        p.firstName = j.value("firstName", "Trainer");
        p.username = j.value("username", "");
        p.coins = j.value("coins", 300);
        p.activeInstanceId = j.value("activeInstanceId", "");
        p.battlesWon = j.value("battlesWon", 0);
        p.battlesLost = j.value("battlesLost", 0);
        p.packsOpened = j.value("packsOpened", 0);
        p.lastDailyClaim = j.value("lastDailyClaim", 0LL);

        if (j.contains("cards") && j["cards"].is_array()) {
            for (const auto& cj : j["cards"]) {
                p.cards.push_back(PlayerCard::fromJson(cj));
            }
        }
        return p;
    }
};

class GameManager {
private:
    std::map<long long, Player> players_;
    std::mt19937 rng_;

public:
    GameManager() {
        rng_.seed(std::chrono::system_clock::now().time_since_epoch().count());
    }

    const Card* findBaseCard(int id) {
        const auto& db = GetMasterDatabase();
        for (const auto& c : db) {
            if (c.id == id) return &c;
        }
        return nullptr;
    }

    Player& getOrCreatePlayer(long long userId, const std::string& firstName, const std::string& username) {
        auto it = players_.find(userId);
        if (it != players_.end()) {
            it->second.firstName = firstName;
            it->second.username = username;
            return it->second;
        }

        Player p;
        p.userId = userId;
        p.firstName = firstName;
        p.username = username;
        p.coins = 300;

        // Give starter card (Pikachu)
        PlayerCard starter;
        starter.instanceId = "card_" + std::to_string(userId) + "_starter";
        starter.pokemonId = 25;
        starter.level = 1;
        starter.exp = 0;
        starter.pulledAt = "starter";
        p.cards.push_back(starter);
        p.activeInstanceId = starter.instanceId;

        players_[userId] = p;
        return players_[userId];
    }

    // Gacha Opening
    struct GachaResult {
        bool success = false;
        std::string message;
        std::vector<const Card*> cardsPulled;
        int cost = 0;
    };

    GachaResult openGacha(long long userId, const std::string& packType) {
        auto it = players_.find(userId);
        if (it == players_.end()) return {false, "Người chơi không tồn tại!", {}, 0};

        Player& p = it->second;
        int cost = 100;
        int count = 1;
        std::vector<std::pair<Rarity, double>> rates;

        if (packType == "poke_pack") {
            cost = 100;
            count = 1;
            rates = {{Rarity::Common, 0.65}, {Rarity::Rare, 0.25}, {Rarity::Epic, 0.08}, {Rarity::Legendary, 0.02}};
        } else if (packType == "great_pack") {
            cost = 250;
            count = 3;
            rates = {{Rarity::Common, 0.35}, {Rarity::Rare, 0.45}, {Rarity::Epic, 0.16}, {Rarity::Legendary, 0.04}};
        } else if (packType == "master_pack") {
            cost = 500;
            count = 5;
            rates = {{Rarity::Rare, 0.40}, {Rarity::Epic, 0.35}, {Rarity::Legendary, 0.20}, {Rarity::Mythic, 0.05}};
        } else {
            return {false, "Gói thẻ không hợp lệ!", {}, 0};
        }

        if (p.coins < cost) {
            return {false, "Bạn không đủ Xu! Cần " + std::to_string(cost) + " Xu để mở gói này. Hãy khiêu chiến Đấu Trường để kiếm thêm!", {}, cost};
        }

        p.coins -= cost;
        p.packsOpened += count;

        std::uniform_real_distribution<double> dist(0.0, 1.0);
        const auto& db = GetMasterDatabase();
        GachaResult res;
        res.success = true;
        res.cost = cost;

        for (int i = 0; i < count; ++i) {
            double roll = dist(rng_);
            double acc = 0.0;
            Rarity chosenRarity = Rarity::Common;
            for (const auto& r : rates) {
                acc += r.second;
                if (roll <= acc) {
                    chosenRarity = r.first;
                    break;
                }
            }

            std::vector<const Card*> pool;
            for (const auto& c : db) {
                if (c.rarity == chosenRarity) pool.push_back(&c);
            }
            if (pool.empty()) pool.push_back(&db[0]);

            std::uniform_int_distribution<size_t> poolDist(0, pool.size() - 1);
            const Card* pulled = pool[poolDist(rng_)];

            // Add to player cards
            PlayerCard pc;
            pc.instanceId = "card_" + std::to_string(userId) + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + "_" + std::to_string(i);
            pc.pokemonId = pulled->id;
            pc.level = 1;
            pc.exp = 0;
            pc.pulledAt = "gacha";
            p.cards.push_back(pc);

            res.cardsPulled.push_back(pulled);
        }

        return res;
    }

    // Daily Gift
    std::pair<bool, std::string> claimDaily(long long userId) {
        auto it = players_.find(userId);
        if (it == players_.end()) return {false, "Người chơi không tồn tại!"};

        Player& p = it->second;
        long long now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        long long cooldown = 12 * 3600; // 12 hours

        if (p.lastDailyClaim > 0 && (now - p.lastDailyClaim < cooldown)) {
            long long remaining = cooldown - (now - p.lastDailyClaim);
            int hours = remaining / 3600;
            int mins = (remaining % 3600) / 60;
            return {false, "Bạn đã nhận quà hôm nay rồi! Vui lòng quay lại sau " + std::to_string(hours) + "h " + std::to_string(mins) + "p."};
        }

        p.lastDailyClaim = now;
        p.coins += 150;
        return {true, "🎉 Chúc mừng! Bạn vừa nhận được +150 Xu miễn phí! Tổng số dư hiện tại: " + std::to_string(p.coins) + " 🪙"};
    }

    // Start Battle with Gym Leader
    std::pair<bool, std::string> startBattle(long long userId, const std::string& leaderId) {
        auto it = players_.find(userId);
        if (it == players_.end()) return {false, "Người chơi không tồn tại!"};

        Player& p = it->second;
        if (p.cards.empty()) return {false, "Bạn chưa có thẻ bài nào!"};

        // Find leader
        const auto& leaders = GetGymLeaders();
        const GymLeader* leader = nullptr;
        for (const auto& l : leaders) {
            if (l.id == leaderId) { leader = &l; break; }
        }
        if (!leader) leader = &leaders[0];

        // Active Player card
        PlayerCard* pCard = nullptr;
        for (auto& c : p.cards) {
            if (c.instanceId == p.activeInstanceId) { pCard = &c; break; }
        }
        if (!pCard) {
            pCard = &p.cards[0];
            p.activeInstanceId = pCard->instanceId;
        }

        const Card* pBase = findBaseCard(pCard->pokemonId);
        const Card* eBase = findBaseCard(leader->pokemonId);
        if (!pBase || !eBase) return {false, "Lỗi dữ liệu thẻ bài!"};

        // Scale stats by level
        double lvlMult = 1.0 + (pCard->level - 1) * 0.08;
        int playerHp = std::floor(pBase->hp * lvlMult);
        int playerAtk = std::floor(pBase->atk * lvlMult);
        int playerDef = std::floor(pBase->def * lvlMult);

        // Scale enemy by difficulty
        double eMult = 1.0;
        if (leader->difficulty == "Trung Bình") eMult = 1.15;
        if (leader->difficulty == "Khó") eMult = 1.30;
        if (leader->difficulty == "Cực Khó") eMult = 1.50;
        if (leader->difficulty == "Huyền Thoại") eMult = 1.85;

        int enemyHp = std::floor(eBase->hp * eMult);
        int enemyAtk = std::floor(eBase->atk * eMult);
        int enemyDef = std::floor(eBase->def * eMult);

        // Setup session
        BattleSession b;
        b.active = true;
        b.leaderId = leader->id;
        b.enemyPokemonId = eBase->id;
        b.enemyName = leader->name + " (" + eBase->name + ")";
        b.enemyType = eBase->type;
        b.enemyHp = enemyHp;
        b.enemyMaxHp = enemyHp;
        b.enemyAtk = enemyAtk;
        b.enemyDef = enemyDef;
        b.enemyShield = false;

        b.playerInstanceId = pCard->instanceId;
        b.playerName = pBase->name;
        b.playerType = pBase->type;
        b.playerHp = playerHp;
        b.playerMaxHp = playerHp;
        b.playerAtk = playerAtk;
        b.playerDef = playerDef;
        b.playerEnergy = 1;
        b.playerShield = false;
        b.potionCharges = 2;
        b.turn = "player";
        b.log = "⚔️ Trận đấu bắt đầu! Đối đầu với " + leader->name + "!";

        p.currentBattle = b;
        return {true, "OK"};
    }

    // Execute Turn Action in Battle
    struct BattleActionResult {
        bool finished = false;
        bool victory = false;
        std::string summary;
        int coinsEarned = 0;
        int expEarned = 0;
    };

    BattleActionResult executeBattleAction(long long userId, const std::string& action) {
        auto it = players_.find(userId);
        if (it == players_.end()) return {true, false, "Người chơi không tồn tại!", 0, 0};

        Player& p = it->second;
        BattleSession& b = p.currentBattle;
        if (!b.active) return {true, false, "Trận đấu đã kết thúc!", 0, 0};

        if (action == "flee") {
            b.active = false;
            p.battlesLost++;
            return {true, false, "🏃 Bạn đã bỏ chạy khỏi trận đấu!", 0, 0};
        }

        std::string turnLog;

        if (action == "attack") {
            double mult = GetTypeMultiplier(b.playerType, b.enemyType);
            int baseDmg = std::max(20, (int)(b.playerAtk * 0.5 * mult));
            if (b.enemyShield) { baseDmg = std::max(10, baseDmg / 2); b.enemyShield = false; }
            b.enemyHp = std::max(0, b.enemyHp - baseDmg);
            b.playerEnergy = std::min(3, b.playerEnergy + 1);
            turnLog = "💥 " + b.playerName + " tấn công thường gây " + std::to_string(baseDmg) + " sát thương! (Nạp +1 EP)";
        } else if (action == "skill") {
            if (b.playerEnergy < 2) {
                b.log = "⚠️ Cần ít nhất 2 EP để dùng Tuyệt Kỹ Hệ! Hãy dùng tấn công thường để nạp EP.";
                return {false, false, b.log, 0, 0};
            }
            b.playerEnergy -= 2;
            double mult = GetTypeMultiplier(b.playerType, b.enemyType);
            int baseDmg = std::max(40, (int)(b.playerAtk * 1.2 * mult));
            if (b.enemyShield) { baseDmg = std::max(15, baseDmg / 2); b.enemyShield = false; }
            b.enemyHp = std::max(0, b.enemyHp - baseDmg);
            turnLog = "⚡ " + b.playerName + " tung TUYỆT KỸ BỘC PHÁ gây " + std::to_string(baseDmg) + " sát thương!";
        } else if (action == "shield") {
            b.playerShield = true;
            b.playerEnergy = std::min(3, b.playerEnergy + 1);
            turnLog = "🛡️ " + b.playerName + " vào thế phòng thủ! (Giảm 50% đòn đánh tới & +1 EP)";
        } else if (action == "potion") {
            if (b.potionCharges <= 0) {
                b.log = "⚠️ Đã dùng hết dược phẩm trong trận đấu này!";
                return {false, false, b.log, 0, 0};
            }
            b.potionCharges--;
            int heal = std::floor(b.playerMaxHp * 0.35);
            b.playerHp = std::min(b.playerMaxHp, b.playerHp + heal);
            turnLog = "🧪 Dùng dược phẩm hồi phục +" + std::to_string(heal) + " HP! (Còn " + std::to_string(b.potionCharges) + " bình)";
        }

        // Check Victory
        if (b.enemyHp <= 0) {
            b.active = false;
            p.battlesWon++;

            // Rewards
            const auto& leaders = GetGymLeaders();
            int coins = 100, exp = 60;
            for (const auto& l : leaders) {
                if (l.id == b.leaderId) { coins = l.rewardCoins; exp = l.rewardExp; break; }
            }
            p.coins += coins;

            // EXP to active card
            for (auto& c : p.cards) {
                if (c.instanceId == b.playerInstanceId) {
                    c.exp += exp;
                    if (c.exp >= c.level * 100) {
                        c.level++;
                        turnLog += "\n🌟 Thẻ bài thăng lên Cấp " + std::to_string(c.level) + "! (+8% HP & ATK)";
                    }
                    break;
                }
            }

            return {true, true, "🏆 CHIẾN THẮNG!\n" + turnLog + "\n\n🎁 Phần thưởng: +" + std::to_string(coins) + " 🪙 | +" + std::to_string(exp) + " EXP!", coins, exp};
        }

        // Enemy Turn
        int enemyDmg = std::max(15, (int)(b.enemyAtk * 0.45));
        if (b.playerShield) {
            enemyDmg = std::max(8, enemyDmg / 2);
            b.playerShield = false;
        }
        b.playerHp = std::max(0, b.playerHp - enemyDmg);
        turnLog += "\n⚠️ " + b.enemyName + " phản kích gây " + std::to_string(enemyDmg) + " sát thương!";

        // Check Defeat
        if (b.playerHp <= 0) {
            b.active = false;
            p.battlesLost++;
            return {true, false, "💀 THẤT BẠI!\n" + turnLog + "\n\nPokémon của bạn đã kiệt sức. Hãy nâng cấp thẻ và thử lại!", 0, 0};
        }

        b.log = turnLog;
        return {false, false, turnLog, 0, 0};
    }

    // Save & Load
    bool saveToFile(const std::string& path) {
        json j = json::object();
        json pArr = json::array();
        for (const auto& pair : players_) {
            pArr.push_back(pair.second.toJson());
        }
        j["players"] = pArr;

        std::ofstream out(path);
        if (!out.is_open()) return false;
        out << j.dump(2);
        return true;
    }

    bool loadFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in.is_open()) return false;

        try {
            json j = json::parse(in);
            if (j.contains("players") && j["players"].is_array()) {
                players_.clear();
                for (const auto& pj : j["players"]) {
                    Player p = Player::fromJson(pj);
                    players_[p.userId] = p;
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }
};

} // namespace pkmn
