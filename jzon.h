#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace jzon {
enum value_tag {
    number_tag = 0xFFF0,
    null_tag,
    bool_tag,
    string_tag,
    array_tag,
    object_tag,
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

static_assert(sizeof(value) == sizeof(double), "value size must be 8");

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

    static int parse_string(vector<value> &v, const char *s, const char **endptr) {
        value temp;
        size_t n = 0;
#define PUT(c)                            \
    do {                                  \
        temp.s[n++ % sizeof(temp)] = (c); \
        if ((n % sizeof(temp)) == 0)      \
            v.push_back(temp);            \
    } while (0)

        for (; *s; ++s) {
            unsigned int c = *s;
            if (c == '"') {
                *endptr = ++s;
                temp.s[n++ % sizeof(temp)] = '\0';
                v.push_back(temp);
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
            } else if (c < ' ') {
                if (c == '\r' || c == '\n')
                    return missing_terminating_quote;
                break;
            }
            PUT(c);
        }
        *endptr = s;
        return invalid_string_char;
    }

    static vector<value> parse(const char *s) {
        vector<value> result, temp;
        size_t frame = 0;
        while (*s) {
            s = skip_ws(s);
            if (*s == 0) {
                break;
            } else if (*s == '[') {
                temp.push_back({frame, array_tag});
                frame = temp.size();
                ++s;
            } else if (*s == '{') {
                temp.push_back({frame, object_tag});
                frame = temp.size();
                ++s;
            } else if (*s == ']' || *s == '}') {
                size_t offset = result.size();
                size_t length = temp.size() - frame;
                result.push_back({length, number_tag});
                result.append(temp.end() - length, length);
                temp.resize(temp.size() - length);
                frame = temp.back().payload;
                temp.back() = {offset, *s == ']' ? array_tag : object_tag};
                ++s;
            } else if (*s == ',') {
                ++s;
            } else if (*s == ':') {
                ++s;
            } else if (*s == '"') {
                size_t offset = result.size();
                if (parse_string(result, ++s, &s))
                    abort();
                temp.push_back({offset, string_tag});
            } else if (*s >= '0' && *s <= '9') {
                temp.push_back(parse_number(s, &s));
            } else if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') {
                temp.push_back(-parse_number(++s, &s));
            } else if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
                s += 5;
                temp.push_back({false, bool_tag});
            } else if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
                s += 4;
                temp.push_back({true, bool_tag});
            } else if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
                s += 4;
                temp.push_back({0, null_tag});
            } else {
                abort();
            }
        }
        result.push_back(temp.back());
        return result;
    }
};

class view {
    const vector<value> &_data;
    value _value;

public:
    view(const vector<value> &data) : _data(data), _value(data.back()) {}
    view(const vector<value> &data, size_t index) : _data(data), _value(data[index]) {}

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

    view operator[](size_t index) const {
        assert(is_array() || is_object());
        return {_data, _value.payload + index + 1};
    }

    size_t size() const {
        assert(is_array() || is_object());
        return _data[_value.payload].payload;
    }

    void stringify(vector<char> &buffer) const {
        char temp[32];
        switch (tag()) {
        case number_tag:
            buffer.append(temp, snprintf(temp, sizeof(temp), "%.10g", to_number()));
            break;

        case null_tag:
            buffer.append("null", 4);
            break;

        case bool_tag:
            if (to_bool())
                buffer.append("true", 4);
            else
                buffer.append("false", 5);
            break;

        case string_tag:
            buffer.push_back('"');
            for (const char *s = to_string(); *s;) {
                char c = *s++;
                switch (c) {
                case '\b':
                    buffer.append("\\b", 2);
                    break;
                case '\f':
                    buffer.append("\\f", 2);
                    break;
                case '\n':
                    buffer.append("\\n", 2);
                    break;
                case '\r':
                    buffer.append("\\r", 2);
                    break;
                case '\t':
                    buffer.append("\\t", 2);
                    break;
                case '\\':
                    buffer.append("\\\\", 2);
                    break;
                case '"':
                    buffer.append("\\\"", 2);
                    break;
                default:
                    buffer.push_back(c);
                }
            }
            buffer.push_back('"');
            break;

        case array_tag:
            buffer.push_back('[');
            for (size_t i = 0; i < size(); ++i) {
                if (i > 0)
                    buffer.push_back(',');
                operator[](i).stringify(buffer);
            }
            buffer.push_back(']');
            break;

        case object_tag:
            buffer.push_back('{');
            for (size_t i = 0; i < size(); i += 2) {
                if (i > 0)
                    buffer.push_back(',');
                operator[](i).stringify(buffer);
                buffer.append(": ", 2);
                operator[](i + 1).stringify(buffer);
            }
            buffer.push_back('}');
            break;
        }
    }
};
}
