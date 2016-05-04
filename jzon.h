#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace jzon {
template <typename T>
class vector {
    T *_data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;

public:
    vector() = default;

    ~vector() {
        free(_data);
    }

    vector(const vector &x) : _size(x._size), _capacity(x._size) {
        _data = (T *)malloc(sizeof(T) * _size);
        memcpy(_data, x._data, sizeof(T) * _size);
    }

    vector(vector &&x) : _data(x._data), _size(x._size), _capacity(x._capacity) {
        x._data = nullptr;
        x._size = x._capacity = 0;
    }

    vector &operator=(const vector &x) {
        set_capacity(x._size);
        memcpy(_data, x._data, sizeof(T) * x._size);
        _size = x._size;
        return *this;
    }

    vector &operator=(vector &&x) {
        free(_data);

        _data = x._data;
        _size = x._size;
        _capacity = x._capacity;

        x._data = nullptr;
        x._size = x._capacity = 0;

        return *this;
    }

    void resize(size_t n) {
        if (_capacity < n)
            set_capacity(n);
        _size = n;
    }

    void set_capacity(size_t n) {
        if (n < _size) {
            _size = n;
        }
        _data = static_cast<T *>(realloc(_data, sizeof(T) * n));
        _capacity = n;
    }

    void append(const T *x, size_t n) {
        size_t new_size = _size + n;
        if (_capacity < new_size) {
            size_t new_capacity = _capacity * 2 + 8;
            set_capacity(new_capacity < new_size ? new_size : new_capacity);
        }
        memcpy(_data + _size, x, sizeof(T) * n);
        _size = new_size;
    }

    void push_back(const T &x) {
        if (_size == _capacity)
            set_capacity(_capacity * 2 + 8);
        _data[_size++] = x;
    }

    void pop_back() {
        assert(!empty());
        --_size;
    }

    T &operator[](size_t i) {
        assert(i < size());
        return _data[i];
    }
    const T &operator[](size_t i) const {
        assert(i < size());
        return _data[i];
    }
    T &front() {
        assert(!empty());
        return _data[0];
    }
    const T &front() const {
        assert(!empty());
        return _data[0];
    }
    T &back() {
        assert(!empty());
        return _data[_size - 1];
    }
    const T &back() const {
        assert(!empty());
        return _data[_size - 1];
    }

    T *begin() { return _data; }
    const T *begin() const { return _data; }
    T *end() { return _data + _size; }
    const T *end() const { return _data + _size; }

    bool empty() const { return _size == 0; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
};

enum value_tag {
    number_tag = 0xFFF0,
    null_tag,
    bool_tag,
    string_tag,
    array_tag,
    object_tag,
};

enum value_flag {
    prefix_flag = 0x8,
};

union value {
    double number;
    struct {
#if _MSC_VER || __SIZEOF_LONG__ == 4
        unsigned long payload : 32;
        unsigned long padding : 16;
#else
        unsigned long payload : 48;
#endif
        value_tag tag : 16;
    };
    char s[sizeof(double)];

    value() = default;
    constexpr value(double n) : number(n) {}
    constexpr value(unsigned long payload, value_tag tag) : payload(payload), tag(tag) {}
    constexpr bool is_nan() const { return tag > number_tag; }
};

static_assert(sizeof(value) == sizeof(double), "value size != 8");

class view {
    const vector<value> &_data;
    value _value;

public:
    view(const vector<value> &data, value v) : _data(data), _value(v) {}
    view(const vector<value> &data) : view(data, data.back()) {}

    value_tag tag() const {
        return _value.is_nan() ? _value.tag : number_tag;
    }

    bool is_number() const { return !_value.is_nan(); }
    bool is_null() const { return _value.tag == null_tag; }
    bool is_bool() const { return _value.tag == bool_tag; }
    bool is_string() const { return _value.tag == string_tag; }
    bool is_array() const { return _value.tag == array_tag; }
    bool is_object() const { return _value.tag == object_tag; }

    double to_number() const {
        assert(is_number());
        return _value.number;
    }
    bool to_bool() const {
        assert(is_bool());
        return _value.payload ? true : false;
    }
    const char *to_string() const {
        assert(is_string());
        return _data[_value.payload].s;
    }
    size_t size() const {
        assert(is_array() || is_object());
        return _data[_value.payload - 1].payload;
    }
    view operator[](size_t index) const {
        assert(is_array() || is_object());
        return {_data, _data[_value.payload + index]};
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

    static double exp10(size_t n) {
        static constexpr double E[] = {
            1e+0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7,
            1e+8, 1e+9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
            1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23,
        };

        if (n < sizeof(E) / sizeof(E[0]))
            return E[n];

        double x = 1;
        for (double y = 10; n; n >>= 1, y *= y)
            if (n & 1)
                x *= y;

        return x;
    }

    static double parse_number(const char *s, const char **endptr) {
#if 1
        unsigned long significant = 0;
        int power = 0;

        while (*s >= '0' && *s <= '9') {
            significant = (significant * 10) + (*s++ - '0');
        }

        if (*s == '.') {
            ++s;

            while (*s >= '0' && *s <= '9') {
                significant = (significant * 10) + (*s++ - '0');
                --power;
            }
        }

        if (*s == 'e' || *s == 'E') {
            ++s;

            bool negative = false;
            if (*s == '+') {
                ++s;
            } else if (*s == '-') {
                ++s;
                negative = true;
            }

            unsigned int exponent = 0;
            while (*s >= '0' && *s <= '9')
                exponent = (exponent * 10) + (*s++ - '0');

            power = power + (negative ? -exponent : exponent);
        }

        *endptr = s;

        if (power == 0) {
            return significant;
        } else if (power > 0) {
            return significant * exp10(power);
        } else {
            return significant / exp10(-power);
        }
#else
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
#endif
    }

    static int parse_string(vector<value> &v, const char *s, const char **endptr) {
        value buf;
        size_t n = 0;

#define PUT(c)                          \
    do {                                \
        buf.s[n++ % sizeof(buf)] = (c); \
        if ((n % sizeof(buf)) == 0)     \
            v.push_back(buf);           \
    } while (0)

        for (; *s; ++s) {
            unsigned int c = *s;
            if (c < ' ') {
                break;
            } else if (c == '"') {
                *endptr = ++s;

                buf.s[n++ % sizeof(buf)] = '\0';
                v.push_back(buf);

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
                        PUT(cp);
                    } else if (cp < 0x800) {
                        PUT(0xC0 | (cp >> 6));
                        PUT(0x80 | (cp & 0x3F));
                    } else {
                        PUT(0xE0 | (cp >> 12));
                        PUT(0x80 | ((cp >> 6) & 0x3F));
                        PUT(0x80 | (cp & 0x3F));
                    }

                    continue;
                } else {
                    *endptr = s;
                    return invalid_string_escape;
                }
            }
            PUT(c);
        }

#undef PUT

        *endptr = s;
        return (*s == '\r' || *s == '\n') ? missing_terminating_quote : invalid_string_char;
    }

    static vector<value> parse(const char *s) {
        vector<value> backlog;
        vector<value> result;
        size_t frame = 0;
        while (*s) {
            s = skip_ws(s);
            if (*s == 0) {
                break;
            } else if (*s == '[' || *s == '{') {
                backlog.push_back({frame, *s == '[' ? array_tag : object_tag});
                frame = backlog.size();
                ++s;
            } else if (*s == ']' || *s == '}') {
                if (backlog.empty())
                    return {/*stack_underflow*/};

                value saved = backlog[frame - 1];
                if (saved.tag == array_tag && *s != ']')
                    return {/*mismatch_brace*/};

                size_t size = backlog.size() - frame;
                frame = saved.payload;

                result.push_back({size, (value_tag)(saved.tag | prefix_flag)});
                size_t offset = result.size();

                result.append(backlog.end() - size, size);
                backlog.resize(backlog.size() - size);

                backlog.back() = {offset, saved.tag};
                ++s;
            } else if (*s == ',') {
                ++s;
            } else if (*s == ':') {
                ++s;
            } else if (*s == '"') {
                size_t offset = result.size();
                if (parse_string(result, ++s, &s))
                    return {};
                backlog.push_back({offset, string_tag});
            } else if (*s >= '0' && *s <= '9') {
                backlog.push_back(parse_number(s, &s));
            } else if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') {
                backlog.push_back(-parse_number(++s, &s));
            } else if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
                s += 5;
                backlog.push_back({false, bool_tag});
            } else if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
                s += 4;
                backlog.push_back({true, bool_tag});
            } else if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
                s += 4;
                backlog.push_back({0, null_tag});
            } else {
                return {};
            }
        }
        result.push_back(backlog.back());
        return result;
    }
};

inline void stringify(vector<char> &s, view v, size_t depth = 0) {
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
            for (size_t i = 0; i < v.size(); ++i, comma = ',') {
                s.push_back(comma);
                stringify(s, v[i], depth + 1);
            }
        } else {
            s.push_back('[');
        }
        s.push_back(']');
        break;

    case object_tag:
        if (v.size()) {
            char comma = '{';
            for (size_t i = 0; i < v.size(); i += 2, comma = ',') {
                s.push_back(comma);
                stringify(s, v[i], depth + 1);
                s.push_back(':');
                stringify(s, v[i + 1], depth + 1);
            }
        } else {
            s.push_back('{');
        }
        s.push_back('}');
        break;
    }
}

inline void indent(vector<char> &s, size_t depth) {
    if (s.size())
        s.push_back('\n');
    while (depth--)
        s.append("\x20\x20\x20\x20", 4);
}

inline void prettify(vector<char> &s, view v, size_t depth = 0) {
    switch (v.tag()) {
    case array_tag:
        if (v.size()) {
            char comma = '[';
            for (size_t i = 0; i < v.size(); ++i, comma = ',') {
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
            for (size_t i = 0; i < v.size(); i += 2, comma = ',') {
                s.push_back(comma);
                indent(s, depth + 1);
                stringify(s, v[i], depth + 1);
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
}
