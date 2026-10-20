#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tg {

// Helper to write HTTP response chunks
inline size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* mem = static_cast<std::string*>(userp);
    mem->append(static_cast<char*>(contents), total);
    return total;
}

class Bot {
private:
    std::string token_;
    std::string apiBase_;

public:
    Bot(const std::string& token) : token_(token) {
        apiBase_ = "https://api.telegram.org/bot" + token_ + "/";
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~Bot() {
        curl_global_cleanup();
    }

    // Generic JSON POST Request to Telegram Bot API
    json makeRequest(const std::string& method, const json& payload) {
        CURL* curl = curl_easy_init();
        if (!curl) return {{"ok", false}, {"description", "curl init failed"}};

        std::string url = apiBase_ + method;
        std::string postFields = payload.dump();
        std::string responseString;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return {{"ok", false}, {"description", curl_easy_strerror(res)}};
        }

        try {
            return json::parse(responseString);
        } catch (const std::exception& e) {
            return {{"ok", false}, {"description", e.what()}};
        }
    }

    // Long polling getUpdates
    json getUpdates(long long offset, int timeoutSeconds = 25) {
        json payload = {
            {"offset", offset},
            {"timeout", timeoutSeconds},
            {"allowed_updates", {"message", "callback_query"}}
        };
        return makeRequest("getUpdates", payload);
    }

    // Send Text Message with optional Inline Keyboard
    json sendMessage(long long chatId, const std::string& text, const json& replyMarkup = nullptr, const std::string& parseMode = "Markdown") {
        json payload = {
            {"chat_id", chatId},
            {"text", text},
            {"parse_mode", parseMode}
        };
        if (!replyMarkup.is_null()) {
            payload["reply_markup"] = replyMarkup;
        }
        return makeRequest("sendMessage", payload);
    }

    // Edit Existing Message Text
    json editMessageText(long long chatId, long long messageId, const std::string& text, const json& replyMarkup = nullptr, const std::string& parseMode = "Markdown") {
        json payload = {
            {"chat_id", chatId},
            {"message_id", messageId},
            {"text", text},
            {"parse_mode", parseMode}
        };
        if (!replyMarkup.is_null()) {
            payload["reply_markup"] = replyMarkup;
        }
        return makeRequest("editMessageText", payload);
    }

    // Send Photo with Caption
    json sendPhoto(long long chatId, const std::string& photoUrl, const std::string& caption, const json& replyMarkup = nullptr, const std::string& parseMode = "Markdown") {
        json payload = {
            {"chat_id", chatId},
            {"photo", photoUrl},
            {"caption", caption},
            {"parse_mode", parseMode}
        };
        if (!replyMarkup.is_null()) {
            payload["reply_markup"] = replyMarkup;
        }
        return makeRequest("sendPhoto", payload);
    }

    // Answer Callback Query (Acknowledge inline button tap)
    json answerCallbackQuery(const std::string& callbackQueryId, const std::string& text = "", bool showAlert = false) {
        json payload = {
            {"callback_query_id", callbackQueryId},
            {"show_alert", showAlert}
        };
        if (!text.empty()) {
            payload["text"] = text;
        }
        return makeRequest("answerCallbackQuery", payload);
    }
};

} // namespace tg
