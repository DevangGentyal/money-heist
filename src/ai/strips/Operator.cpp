#include "Operator.h"
#include <algorithm>
#include <cctype>

namespace ai {
namespace strips {

static std::string trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        start++;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        end--;
    }
    return value.substr(start, end - start);
}

std::string Goal::toString() const {
    std::string result = predicate;
    if (!args.empty()) {
        result += "(";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) result += ",";
            result += args[i];
        }
        result += ")";
    }
    return result;
}

Goal Goal::parse(const std::string& expression) {
    std::string trimmed = trim(expression);
    size_t openParen = trimmed.find('(');
    if (openParen == std::string::npos) {
        return Goal(trimmed, {});
    }

    std::string pred = trim(trimmed.substr(0, openParen));
    size_t closeParen = trimmed.rfind(')');
    if (closeParen == std::string::npos || closeParen <= openParen) {
        return Goal(pred, {});
    }

    std::string argText = trimmed.substr(openParen + 1, closeParen - openParen - 1);
    std::vector<std::string> args;
    size_t current = 0;
    while (current < argText.size()) {
        size_t comma = argText.find(',', current);
        if (comma == std::string::npos) comma = argText.size();
        std::string token = trim(argText.substr(current, comma - current));
        if (!token.empty()) {
            args.push_back(token);
        }
        current = comma + 1;
    }
    return Goal(pred, args);
}

} // namespace strips
} // namespace ai
