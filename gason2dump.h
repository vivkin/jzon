#pragma once

#include "gason2.h"
#include <stdio.h>

namespace gason2 {
struct dump {
    static void stringify(vector<char> &s, value v) {
        char buf[32];

        switch (v.type()) {
        case type::number:
            s.append(buf, snprintf(buf, sizeof(buf), "%.10g", v.to_number()));
            break;

        case type::null:
            s.append("null", 4);
            break;

        case type::boolean:
            if (v.to_bool())
                s.append("true", 4);
            else
                s.append("false", 5);
            break;

        case type::string:
            s.push_back('"');
            for (const char *p = v.to_string(); *p;) {
                char c = *p++;
                switch (c) {
                case '\b':
                    s.append("\\b", 2);
                    break;
                case '\f':
                    s.append("\\f", 2);
                    break;
                case '\n':
                    s.append("\\n", 2);
                    break;
                case '\r':
                    s.append("\\r", 2);
                    break;
                case '\t':
                    s.append("\\t", 2);
                    break;
                case '\\':
                    s.append("\\\\", 2);
                    break;
                case '"':
                    s.append("\\\"", 2);
                    break;
                default:
                    s.push_back(c);
                }
            }
            s.push_back('"');
            break;

        case type::array:
            if (v.size()) {
                char comma = '[';
                for (size_t i = 0, i_end = v.size(); i < i_end; ++i, comma = ',') {
                    s.push_back(comma);
                    stringify(s, v[i]);
                }
            } else {
                s.push_back('[');
            }
            s.push_back(']');
            break;

        case type::object:
            if (v.size()) {
                char comma = '{';
                for (size_t i = 0, i_end = v.size(); i < i_end; i += 2, comma = ',') {
                    s.push_back(comma);
                    stringify(s, v[i]);
                    s.push_back(':');
                    stringify(s, v[i + 1]);
                }
            } else {
                s.push_back('{');
            }
            s.push_back('}');
            break;
        default:
            break;
        }
    }

    static void indent(vector<char> &s, size_t depth) {
        if (s.size())
            s.push_back('\n');
        while (depth--)
            s.append("\x20\x20\x20\x20", 4);
    }

    static void prettify(vector<char> &s, value v, size_t depth = 0) {
        switch (v.type()) {
        case type::array:
            if (v.size()) {
                char comma = '[';
                for (size_t i = 0, i_end = v.size(); i < i_end; ++i, comma = ',') {
                    s.push_back(comma);
                    indent(s, depth + 1);
                    prettify(s, v[i], depth + 1);
                }
                indent(s, depth);
            } else {
                s.push_back('[');
            }
            s.push_back(']');
            break;

        case type::object:
            if (v.size()) {
                char comma = '{';
                for (size_t i = 0, i_end = v.size(); i < i_end; i += 2, comma = ',') {
                    s.push_back(comma);
                    indent(s, depth + 1);
                    stringify(s, v[i]);
                    s.append(": ", 2);
                    prettify(s, v[i + 1], depth + 1);
                }
                indent(s, depth);
            } else {
                s.push_back('{');
            }
            s.push_back('}');
            break;

        default:
            stringify(s, v);
        }
    }

    static int print_error(const char *filename, const char *json, const document &doc) {
        int lineno = 1;
        const char *left = json;
        const char *right = json;
        const char *endptr = json + doc.error_offset();
        while (*right)
            if (*right++ == '\n') {
                if (endptr < right)
                    break;
                left = right;
                ++lineno;
            }

        int column = endptr - left;
        if (column > 80)
            left = endptr - 40;
        if (right - left > 80)
            right = endptr + 80 - (endptr - left);

        const char *str = "";
        switch (doc.error_code()) {
        case error::expecting_string: str = "expecting string"; break;
        case error::expecting_value: str = "expecting value"; break;
        case error::invalid_literal_name: str = "invalid literal name"; break;
        case error::invalid_number: str = "invalid number"; break;
        case error::invalid_string_char: str = "invalid string char"; break;
        case error::invalid_string_escape: str = "invalid string escape"; break;
        case error::invalid_surrogate_pair: str = "invalid surrogate pair"; break;
        case error::missing_colon: str = "missing colon"; break;
        case error::missing_comma_or_bracket: str = "missing comma or bracket"; break;
        case error::unexpected_character: str = "unexpected character"; break;
        }

        return fprintf(stderr, "%s:%d:%d: error: %s\n%.*s\n%*s\n", filename, lineno, column, str, int(right - left), left, int(endptr - left), "^");
    }
};
}
