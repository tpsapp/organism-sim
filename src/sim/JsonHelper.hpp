#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace sim {

enum class JsonType { Null, Bool, Number, String, Array, Object };

struct JsonValue {
    JsonType type{JsonType::Null};
    bool bool_val{false};
    double num_val{0.0};
    std::string str_val{};
    std::vector<JsonValue> arr_val{};
    std::unordered_map<std::string, JsonValue> obj_val{};

    [[nodiscard]] bool as_bool() const noexcept { return bool_val; }
    [[nodiscard]] double as_double() const noexcept { return num_val; }
    [[nodiscard]] int as_int() const noexcept { return static_cast<int>(num_val); }
    [[nodiscard]] uint32_t as_uint32() const noexcept { return static_cast<uint32_t>(num_val); }
    [[nodiscard]] uint64_t as_uint64() const noexcept { return static_cast<uint64_t>(num_val); }
    [[nodiscard]] float as_float() const noexcept { return static_cast<float>(num_val); }
    [[nodiscard]] const std::string& as_string() const noexcept { return str_val; }

    [[nodiscard]] bool contains(const std::string& key) const noexcept {
        return type == JsonType::Object && obj_val.find(key) != obj_val.end();
    }

    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue null_val{};
        if (type != JsonType::Object) return null_val;
        auto it = obj_val.find(key);
        if (it != obj_val.end()) return it->second;
        return null_val;
    }

    const JsonValue& operator[](size_t idx) const {
        static const JsonValue null_val{};
        if (type != JsonType::Array || idx >= arr_val.size()) return null_val;
        return arr_val[idx];
    }
};

class JsonParser {
public:
    static bool Parse(const std::string& src, JsonValue& out) {
        size_t pos = 0;
        SkipWhitespace(src, pos);
        return ParseValue(src, pos, out);
    }

private:
    static void SkipWhitespace(const std::string& src, size_t& pos) {
        while (pos < src.size() && (std::isspace(static_cast<unsigned char>(src[pos])) || src[pos] == '\r')) {
            ++pos;
        }
    }

    static bool ParseValue(const std::string& src, size_t& pos, JsonValue& val) {
        SkipWhitespace(src, pos);
        if (pos >= src.size()) return false;

        char c = src[pos];
        if (c == '{') return ParseObject(src, pos, val);
        if (c == '[') return ParseArray(src, pos, val);
        if (c == '"') return ParseString(src, pos, val);
        if (c == 't' || c == 'f') return ParseBool(src, pos, val);
        if (c == 'n') return ParseNull(src, pos, val);
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return ParseNumber(src, pos, val);

        return false;
    }

    static bool ParseObject(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::Object;
        val.obj_val.clear();
        ++pos; // skip '{'

        SkipWhitespace(src, pos);
        if (pos < src.size() && src[pos] == '}') {
            ++pos;
            return true;
        }

        while (pos < src.size()) {
            SkipWhitespace(src, pos);
            if (pos >= src.size() || src[pos] != '"') return false;

            JsonValue key_val;
            if (!ParseString(src, pos, key_val)) return false;

            SkipWhitespace(src, pos);
            if (pos >= src.size() || src[pos] != ':') return false;
            ++pos; // skip ':'

            JsonValue child;
            if (!ParseValue(src, pos, child)) return false;
            val.obj_val[key_val.str_val] = std::move(child);

            SkipWhitespace(src, pos);
            if (pos >= src.size()) return false;
            if (src[pos] == '}') {
                ++pos;
                return true;
            }
            if (src[pos] == ',') {
                ++pos;
                continue;
            }
            return false;
        }
        return false;
    }

    static bool ParseArray(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::Array;
        val.arr_val.clear();
        ++pos; // skip '['

        SkipWhitespace(src, pos);
        if (pos < src.size() && src[pos] == ']') {
            ++pos;
            return true;
        }

        while (pos < src.size()) {
            JsonValue child;
            if (!ParseValue(src, pos, child)) return false;
            val.arr_val.push_back(std::move(child));

            SkipWhitespace(src, pos);
            if (pos >= src.size()) return false;
            if (src[pos] == ']') {
                ++pos;
                return true;
            }
            if (src[pos] == ',') {
                ++pos;
                continue;
            }
            return false;
        }
        return false;
    }

    static bool ParseString(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::String;
        val.str_val.clear();
        ++pos; // skip '"'

        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return true;
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                if (esc == 'n') val.str_val += '\n';
                else if (esc == 't') val.str_val += '\t';
                else if (esc == 'r') val.str_val += '\r';
                else val.str_val += esc;
            } else {
                val.str_val += c;
            }
        }
        return false;
    }

    static bool ParseNumber(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::Number;
        size_t start = pos;
        if (src[pos] == '-') ++pos;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        if (pos < src.size() && src[pos] == '.') {
            ++pos;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            ++pos;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        }
        try {
            val.num_val = std::stod(src.substr(start, pos - start));
            return true;
        } catch (...) {
            return false;
        }
    }

    static bool ParseBool(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::Bool;
        if (src.compare(pos, 4, "true") == 0) {
            val.bool_val = true;
            pos += 4;
            return true;
        }
        if (src.compare(pos, 5, "false") == 0) {
            val.bool_val = false;
            pos += 5;
            return true;
        }
        return false;
    }

    static bool ParseNull(const std::string& src, size_t& pos, JsonValue& val) {
        val.type = JsonType::Null;
        if (src.compare(pos, 4, "null") == 0) {
            pos += 4;
            return true;
        }
        return false;
    }
};

} // namespace sim
