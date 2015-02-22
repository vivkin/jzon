#pragma once

#include <stddef.h>
#include <stdlib.h>

namespace jzon {
enum {
    number_tag,
    string_tag,
    object_tag,
    array_tag,
    false_tag,
    true_tag,
    null_tag,
};
union value {
    struct {
#if __SIZEOF_LONG__ == 4
        unsigned long integer : 32;
        unsigned long _dummy : 16;
#else
        unsigned long integer : 48;
#endif
        unsigned long _tag : 16;
    };
    double number;

    constexpr value(unsigned long x, int tag, bool flag)
        : integer(x), _tag(tag | (flag ? 0xFFF8 : 0xFFF0)) {
    }
    constexpr value(double x)
        : number(x) {
    }
    constexpr value(bool x)
        : value(0, x ? true_tag : false_tag, false) {
    }
    constexpr value(decltype(nullptr))
        : value(0, null_tag, false) {
    }
    value() = default;
    value(const void *x, int tag)
        : value(reinterpret_cast<unsigned long>(x), tag, true) {
    }
    constexpr int tag() const {
        return _tag > 0xFFF0 ? _tag & 7 : number_tag;
    }
    constexpr bool flag() const {
        return _tag > 0xFFF8;
    }
};
struct array {
    const value *_begin, *_end;

    array(const value *data, value x) : _begin(data + x.integer + 1), _end(_begin + _begin[-1].integer) {}
    value operator[](size_t index) const { return _begin[index]; }
    size_t size() const { return _end - _begin; }
    const value *begin() const { return _begin; }
    const value *end() const { return _end; }
};
struct stack {
    value *data = nullptr;
    size_t size = 0, capacity = 0;

    stack() = default;
    stack(const stack &) = delete;
    stack &operator=(const stack &) = delete;
    stack(stack &&x): data(x.data), size(x.size), capacity(x.capacity) {
        x.data = nullptr;
        x.size = x.capacity = 0;
    }
    stack &operator=(stack &&x) {
        free(data);
        data = x.data;
        size = x.size;
        capacity = x.capacity;
        x.data = nullptr;
        x.size = x.capacity = 0;
        return *this;
    }
    ~stack() {
        free(data);
    }
    void grow(size_t n = 1) {
        capacity = capacity ? capacity * 2 : 16 / sizeof(value);
        while (size + n >= capacity)
            capacity *= 2;
        data = static_cast<value *>(realloc(data, capacity * sizeof(value)));
    }
    void push(value x) {
        if (size == capacity)
            grow();
        data[size++] = x;
    }
    value &top() {
        return data[size - 1];
    }
    void push(const value *p, size_t n) {
        if (size + n >= capacity)
            grow(n);
        while (n--)
            data[size++] = *p++;
    }
    const value *pop(size_t n) {
        size -= n;
        return data + size;
    }
    char &put(size_t index) {
        index += size * sizeof(value);
        if (index >= capacity * sizeof(value))
            grow();
        return ((char *)data)[index];
    }
    void add(size_t length) {
        size += (length + sizeof(value) - 1) / sizeof(value);
    }
};
struct parser {
    enum {
        invalid_number = 1,
        invalid_string_char,
        invalid_string_escape,
        missing_terminating_quote,
        missing_comma,
        missing_colon,
        document_empty,
        stack_underflow,
        unexpected_character,
    };

    static inline const char *skip_ws(const char *s) {
        while (*s == '\x20' || *s == '\x9' || *s == '\xD' || *s == '\xA')
            ++s;
        return s;
    }

    static double parse_number(const char *s, const char **endptr) {
        double n = 0;
        while (*s >= '0' && *s <= '9')
            n = (n * 10) + (*s++ - '0');

        if (*s == '.')
            for (double base = 0.1; *++s >= '0' && *s <= '9'; base *= 0.1)
                n += (*s - '0') * base;

        if (*s == 'e' || *s == 'E') {
            int minus;
            if ((minus = *++s == '-') || *s == '+')
                ++s;

            int exponent = 0;
            while (*s >= '0' && *s <= '9')
                exponent = (exponent * 10) + (*s++ - '0');

            for (double base = minus ? 0.1 : 10; exponent; exponent >>= 1, base *= base)
                if (exponent & 1)
                    n *= base;
        }

        *endptr = s;
        return n;
    }

    static int parse_string(stack &v, const char *s, const char **endptr) {
        for (size_t n = 0; *s; ++s) {
            unsigned int c = *s;
            if (c == '"') {
                *endptr = ++s;
                v.put(n++) = 0;
                v.add(n);
                return 0;
            } else if (c == '\\') {
                c = *++s;
                if (c == 'b')
                    c = '\b';
                else if (c == 't')
                    c = '\t';
                else if (c == 'n')
                    c = '\n';
                else if (c == 'f')
                    c = '\f';
                else if (c == 'r')
                    c = '\r';
                else if (c == '/' || c == '"' || c == '\\')
                    ;
                else if (c == 'u') {
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        c = *++s;
                        cp = cp * 16 + c;
                        if (c >= '0' && c <= '9')
                            cp -= '0';
                        else if (c >= 'A' && c <= 'F')
                            cp -= 'A' - 10;
                        else if (c >= 'a' && c <= 'f')
                            cp -= 'a' - 10;
                        else {
                            *endptr = s;
                            return invalid_string_escape;
                        }
                    }
                    if (cp < 0x80) {
                        v.put(n++) = cp;
                    } else if (cp < 0x800) {
                        v.put(n++) = 0xC0 | (cp >> 6);
                        v.put(n++) = 0x80 | (cp & 0x3F);
                    } else {
                        v.put(n++) = 0xE0 | (cp >> 12);
                        v.put(n++) = 0x80 | ((cp >> 6) & 0x3F);
                        v.put(n++) = 0x80 | (cp & 0x3F);
                    }
                    continue;
                } else {
                    *endptr = s;
                    return invalid_string_escape;
                }
            } else if (c < ' ') {
                if (c == '\r' || c == '\n')
                    return missing_terminating_quote;
                break;
            }
            v.put(n++) = c;
        }
        *endptr = s;
        return invalid_string_char;
    }

    static stack parse(const char *s) {
        stack result, temp;
        size_t frame = 0;
        while (*s) {
            s = skip_ws(s);
            if (*s == 0) {
                break;
            } else if (*s == '[') {
                temp.push({frame, array_tag, false});
                frame = temp.size;
                ++s;
            } else if (*s == '{') {
                temp.push({frame, object_tag, false});
                frame = temp.size;
                ++s;
            } else if (*s == ']' || *s == '}') {
                size_t offset = result.size;
                size_t length = temp.size - frame;
                result.push({length, number_tag, true});
                result.push(temp.pop(length), length);
                frame = temp.top().integer;
                temp.top() = {offset, *s == ']' ? array_tag : object_tag, false};
                ++s;
            } else if (*s == ',') {
                ++s;
            } else if (*s == ':') {
                ++s;
            } else if (*s == '"') {
                size_t offset = result.size;
                if (parse_string(result, ++s, &s))
                    abort();
                temp.push({offset, string_tag, false});
            } else if (*s >= '0' && *s <= '9') {
                temp.push(parse_number(s, &s));
            } else if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') {
                temp.push(-parse_number(++s, &s));
            } else if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
                s += 5;
                temp.push(false);
            } else if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
                s += 4;
                temp.push(true);
            } else if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
                s += 4;
                temp.push(nullptr);
            } else {
                abort();
            }
        }
        result.push(temp.top());
        return result;
    }
};
}
