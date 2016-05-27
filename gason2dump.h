#pragma once

#include "gason2.h"
#include <stdio.h>

namespace gason2 {
struct dump {
    static void stringify(vector<char> &s, view v) {
        char buf[32];

        switch (v.tag()) {
        case number_tag:
            s.append(buf, snprintf(buf, sizeof(buf), "%.10g", v.to_number()));
            break;

        case null_tag:
            s.append("null", 4);
            break;

        case bool_tag:
            if (v.to_bool())
                s.append("true", 4);
            else
                s.append("false", 5);
            break;

        case string_tag:
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

        case array_tag:
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

        case object_tag:
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

    static void prettify(vector<char> &s, view v, size_t depth = 0) {
        switch (v.tag()) {
        case array_tag:
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

        case object_tag:
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

    static const char *error_string(value_tag tag) {
        static const char *err2str[] = {
            "expecting string",
            "expecting value",
            "invalid literal name",
            "invalid number",
            "invalid string char",
            "invalid string escape",
            "invalid surrogate pair",
            "missing colon",
            "missing comma or bracket",
            "second root",
            "unexpected character",
        };
        if (tag > error_tag && tag - error_expecting_string < sizeof(err2str) / sizeof(err2str[0]))
            return err2str[tag - error_expecting_string];
        return "not an error";
    }

    static int print_error(const char *filename, const char *json, const document &doc) {
        int lineno = 1;
        const char *left = json;
        const char *right = json;
        const char *endptr = json + doc.error_offset();
        while (*right) {
            if (*right++ == '\n') {
                if (endptr < right)
                    break;
                left = right;
                ++lineno;
            }
        }

        int column = endptr - left;
        if (column > 80)
            left = endptr - 40;
        if (right - left > 80)
            right = endptr + 80 - (endptr - left);

        return fprintf(stderr, "%s:%d:%d: error: %s\n%.*s\n%*s\n", filename, lineno, column, error_string(doc.tag()), int(right - left), left, int(endptr - left), "^");
    }
};
}
