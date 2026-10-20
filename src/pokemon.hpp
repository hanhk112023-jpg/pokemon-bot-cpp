#pragma once

#include <string>
#include <vector>
#include <map>
#include <random>

namespace pkmn {

enum class Rarity {
    Common,
    Rare,
    Epic,
    Legendary,
    Mythic
};

inline std::string RarityToString(Rarity r) {
    switch (r) {
        case Rarity::Mythic: return "✨ Thần Thoại";
        case Rarity::Legendary: return "👑 Huyền Thoại";
        case Rarity::Epic: return "🔥 Sử Thi";
        case Rarity::Rare: return "💎 Hiếm";
        default: return "⚪ Thường";
    }
}

inline std::string TypeToEmoji(const std::string& type) {
    if (type == "electric") return "⚡ Điện";
    if (type == "fire") return "🔥 Lửa";
    if (type == "water") return "💧 Nước";
    if (type == "grass") return "🌿 Cỏ";
    if (type == "psychic") return "🔮 Tâm Linh";
    if (type == "dragon") return "🐉 Rồng";
    if (type == "steel") return "🛡️ Thép";
    if (type == "ghost") return "👻 Ma";
    if (type == "rock") return "🪨 Đá";
    if (type == "fighting") return "🥊 Giác Đấu";
    return "⚪ Thường";
}

struct Move {
    std::string name;
    int power;
    int energy; // 0 for basic, 2-3 for special
    std::string type;
};

struct Card {
    int id;
    std::string name;
    std::string type;
    Rarity rarity;
    int hp;
    int atk;
    int def;
    int spd;
    std::vector<Move> moves;
    std::string flavor;

    std::string getImageUrl() const {
        return "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/other/official-artwork/" + std::to_string(id) + ".png";
    }
};

inline const std::vector<Card>& GetMasterDatabase() {
    static const std::vector<Card> database = {
        // Gen 1
        {25, "Pikachu", "electric", Rarity::Rare, 420, 88, 55, 110,
            {{"Tia Điện (Quick Spark)", 45, 0, "electric"}, {"Sét 10 Vạn Volt (Volt Tackle)", 125, 2, "electric"}},
            "Chuột điện với hai túi má phát ra tia chớp khổng lồ khi nổi giận."},
        {6, "Charizard", "fire", Rarity::Legendary, 680, 135, 82, 100,
            {{"Cào Lửa (Dragon Claw)", 55, 0, "fire"}, {"Bão Lửa Thiêu Rụi (Blast Burn)", 165, 3, "fire"}},
            "Rồng lửa bay lượn trên bầu trời, hơi thở thiêu rụi cả đá tảng."},
        {9, "Blastoise", "water", Rarity::Epic, 720, 95, 115, 78,
            {{"Bắn Nước (Water Gun)", 45, 0, "water"}, {"Đại Pháo Thủy Lực (Hydro Cannon)", 150, 3, "water"}},
            "Trang bị hai nòng súng nước khổng lồ sau mai, bắn thủng thép dày."},
        {3, "Venusaur", "grass", Rarity::Epic, 710, 100, 105, 80,
            {{"Roi Mây (Vine Whip)", 50, 0, "grass"}, {"Bão Phấn Hoa (Frenzy Plant)", 150, 3, "grass"}},
            "Hoa nở rực rỡ sau lưng tỏa ra hương thơm làm dịu tinh thần chiến đấu."},
        {150, "Mewtwo", "psychic", Rarity::Mythic, 850, 160, 95, 130,
            {{"Chưởng Tâm Linh (Confusion)", 60, 0, "psychic"}, {"Sóng Não Phá Hủy (Psystrike)", 185, 3, "psychic"}},
            "Sinh vật nhân tạo mạnh nhất, sở hữu năng lực tâm linh bóp méo không gian."},
        {151, "Mew", "psychic", Rarity::Mythic, 750, 120, 120, 120,
            {{"Tia Sáng Bí Ẩn (Swift)", 50, 0, "psychic"}, {"Biến Ảo Vô Cực (Genesis Nova)", 170, 3, "psychic"}},
            "Tổ tiên của loài Pokémon mang trong mình DNA của toàn bộ muôn loài."},
        {94, "Gengar", "ghost", Rarity::Epic, 580, 130, 75, 110,
            {{"Liếm Bóng Đêm (Shadow Lick)", 45, 0, "ghost"}, {"Quả Cầu Bóng Ma (Shadow Ball)", 140, 2, "ghost"}},
            "Ẩn mình trong bóng tối chờ đợi cơ hội cướp lấy hơi ấm của đối thủ."},
        {149, "Dragonite", "dragon", Rarity::Legendary, 760, 134, 95, 80,
            {{"Cánh Gió (Wing Attack)", 50, 0, "dragon"}, {"Long Hống Phẫn Nộ (Outrage)", 160, 3, "dragon"}},
            "Thần hộ mệnh biển cả với lòng tốt cứu giúp các con thuyền gặp nạn."},
        {143, "Snorlax", "normal", Rarity::Rare, 890, 110, 90, 30,
            {{"Đấm Thẳng (Body Slam)", 55, 0, "normal"}, {"Ngủ Say Đè Bẹp (Giga Impact)", 150, 3, "normal"}},
            "Ăn cả tạ thức ăn mỗi ngày rồi chìm sâu vào giấc ngủ không ai đánh thức nổi."},
        {130, "Gyarados", "water", Rarity::Rare, 650, 130, 85, 81,
            {{"Cắn Xé (Bite)", 48, 0, "water"}, {"Sóng Thần Thủy Triều (Hydro Pump)", 145, 3, "water"}},
            "Tiến hóa từ cá chép yếu ớt trở thành quái thú tàn phá bờ cõi khi giận dữ."},

        // Gen 2-4
        {249, "Lugia", "psychic", Rarity::Legendary, 820, 110, 130, 110,
            {{"Gió Cực Lớn (Gust)", 50, 0, "psychic"}, {"Vụ Nổ Khí Áp (Aeroblast)", 165, 3, "psychic"}},
            "Thủ lĩnh ba loài chim huyền thoại ngủ sâu nơi đáy đại dương sâu thẳm."},
        {250, "Ho-Oh", "fire", Rarity::Legendary, 810, 130, 110, 90,
            {{"Đốt Cháy (Ember)", 52, 0, "fire"}, {"Ngọn Lửa Thánh Diệu (Sacred Fire)", 170, 3, "fire"}},
            "Bay lượn trên bầu trời để lại dải cầu vồng rực rỡ và hồi sinh vạn vật."},
        {384, "Rayquaza", "dragon", Rarity::Mythic, 880, 165, 90, 115,
            {{"Móng Rồng (Dragon Claw)", 60, 0, "dragon"}, {"Hơi Thở Rồng Vũ Trụ (Draco Meteor)", 195, 3, "dragon"}},
            "Thần rồng cai quản tầng bình lưu, hấp thụ thiên thạch làm nguồn năng lượng."},
        {448, "Lucario", "fighting", Rarity::Epic, 600, 125, 75, 95,
            {{"Quyền Lực (Force Palm)", 55, 0, "fighting"}, {"Chưởng Ba Động (Aura Sphere)", 150, 2, "fighting"}},
            "Cảm nhận sóng hào quang để phán đoán suy nghĩ và hướng di chuyển của kẻ địch."},
        {493, "Arceus", "normal", Rarity::Mythic, 999, 170, 130, 120,
            {{"Thẩm Phán Tinh Tú (Judgment)", 70, 0, "normal"}, {"Sáng Tạo Không Gian (Creation Burst)", 210, 3, "normal"}},
            "Đấng sáng tạo tối cao đã tạo nên toàn bộ vũ trụ Pokémon từ thuở hỗn mang."},
        
        // Commons & Starters
        {1, "Bulbasaur", "grass", Rarity::Common, 380, 55, 60, 50,
            {{"Húc Đầu (Tackle)", 35, 0, "grass"}, {"Hạt Ký Sinh (Leech Seed)", 85, 2, "grass"}},
            "Cõng nụ hoa trên lưng từ khi mới sinh."},
        {4, "Charmander", "fire", Rarity::Common, 360, 65, 45, 65,
            {{"Cào Cấu (Scratch)", 35, 0, "fire"}, {"Hơi Thở Lửa (Ember)", 90, 2, "fire"}},
            "Đuôi cháy sáng biểu hiện sức sống mãnh liệt."},
        {7, "Squirtle", "water", Rarity::Common, 400, 52, 70, 45,
            {{"Húc Mai (Tackle)", 35, 0, "water"}, {"Súng Nước (Water Gun)", 85, 2, "water"}},
            "Thu mình vào mai cứng để phòng thủ và phản công bằng áp lực nước."},
        {74, "Geodude", "rock", Rarity::Common, 420, 65, 90, 25,
            {{"Ném Đá (Rock Throw)", 40, 0, "rock"}, {"Đá Lăn Đè Bẹp (Rock Slide)", 95, 2, "rock"}},
            "Tảng đá biết đi thường bị nhầm lẫn trên các con dốc hiểm trở."},
        {16, "Pidgey", "normal", Rarity::Common, 340, 48, 42, 60,
            {{"Mổ Gió (Gust)", 35, 0, "normal"}, {"Gió Lốc Bụi Cát (Sand Attack)", 80, 2, "normal"}},
            "Chim nhỏ hiền lành bay lượn khắp các ngọn đồi Kanto."}
    };
    return database;
}

// Elemental Effectiveness Matrix
inline double GetTypeMultiplier(const std::string& atkType, const std::string& defType) {
    if (atkType == "water" && defType == "fire") return 2.0;
    if (atkType == "water" && defType == "rock") return 2.0;
    if (atkType == "fire" && defType == "grass") return 2.0;
    if (atkType == "fire" && defType == "steel") return 2.0;
    if (atkType == "grass" && defType == "water") return 2.0;
    if (atkType == "grass" && defType == "rock") return 2.0;
    if (atkType == "electric" && defType == "water") return 2.0;
    if (atkType == "psychic" && defType == "fighting") return 2.0;
    if (atkType == "dragon" && defType == "dragon") return 2.0;
    if (atkType == "fighting" && defType == "normal") return 2.0;
    if (atkType == "fighting" && defType == "rock") return 2.0;

    // Resisted
    if (atkType == "fire" && defType == "water") return 0.5;
    if (atkType == "water" && defType == "grass") return 0.5;
    if (atkType == "grass" && defType == "fire") return 0.5;
    if (atkType == "electric" && defType == "grass") return 0.5;

    return 1.0;
}

// Gym Leader Definition
struct GymLeader {
    std::string id;
    std::string name;
    std::string title;
    int pokemonId;
    std::string difficulty;
    int rewardCoins;
    int rewardExp;
};

inline const std::vector<GymLeader>& GetGymLeaders() {
    static const std::vector<GymLeader> leaders = {
        {"brock", "Brock", "Thủ Lĩnh Đá Cứng (Pewter Gym)", 74, "Dễ", 100, 60},
        {"misty", "Misty", "Nàng Tiên Nước (Cerulean Gym)", 9, "Trung Bình", 180, 100},
        {"surge", "Lt. Surge", "Sĩ Quan Tia Chớp (Vermilion Gym)", 25, "Khó", 250, 150},
        {"sabrina", "Sabrina", "Bậc Thầy Siêu Năng (Saffron Gym)", 150, "Cực Khó", 400, 220},
        {"blaine", "Blaine", "Nhà Đố Vui Núi Lửa (Cinnabar Gym)", 6, "Khó", 320, 180},
        {"red", "Quán Quân Red", "Huyền Thoại Núi Bạc (Champion Red)", 384, "Huyền Thoại", 700, 500}
    };
    return leaders;
}

} // namespace pkmn
