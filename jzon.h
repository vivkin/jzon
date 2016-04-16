#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

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

    value() = default;
    constexpr value(double n) : number(n) {}
    constexpr value(unsigned long payload, value_tag tag) : payload(payload), tag(tag) {}
    constexpr bool is_nan() const { return tag > number_tag; }
};

static_assert(sizeof(value) == sizeof(double), "value size must be 8");

struct stack {
    value *_data = nullptr;
    size_t _size = 0;
    size_t _capacity = 0;

    stack() = default;
    stack(const stack &) = delete;
    stack &operator=(const stack &) = delete;
    stack(stack &&x) : _data(x._data), _size(x._size), _capacity(x._capacity) {
        x._data = nullptr;
        x._size = x._capacity = 0;
    }
    stack &operator=(stack &&x) {
        free(_data);
        _data = x._data;
        _size = x._size;
        _capacity = x._capacity;
        x._data = nullptr;
        x._size = x._capacity = 0;
        return *this;
    }
    ~stack() {
        free(_data);
    }
    void grow(size_t n = 1) {
        _capacity = _capacity * 2 + 8;
        if (_capacity < n)
            _capacity = n;
        _data = static_cast<value *>(realloc(_data, _capacity * sizeof(value)));
    }
    void push(value x) {
        if (_size == _capacity)
            grow(_size + 1);
        _data[_size++] = x;
    }
    value &top() {
        return _data[_size - 1];
    }
    void push(const value *p, size_t n) {
        if (_capacity < _size + n)
            grow(_size + n);
        while (n--)
            _data[_size++] = *p++;
    }
    const value *pop(size_t n) {
        _size -= n;
        return _data + _size;
    }
    char &put(size_t index) {
        index += _size * sizeof(value);
        if (index >= _capacity * sizeof(value))
            grow(_size + 1);
        return ((char *)_data)[index];
    }
    void add(size_t length) {
        _size += (length + sizeof(value) - 1) / sizeof(value);
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
                temp.push({frame, array_tag});
                frame = temp._size;
                ++s;
            } else if (*s == '{') {
                temp.push({frame, object_tag});
                frame = temp._size;
                ++s;
            } else if (*s == ']' || *s == '}') {
                size_t offset = result._size;
                size_t length = temp._size - frame;
                result.push({length, number_tag});
                result.push(temp.pop(length), length);
                frame = temp.top().payload;
                temp.top() = {offset, *s == ']' ? array_tag : object_tag};
                ++s;
            } else if (*s == ',') {
                ++s;
            } else if (*s == ':') {
                ++s;
            } else if (*s == '"') {
                size_t offset = result._size;
                if (parse_string(result, ++s, &s))
                    abort();
                temp.push({offset, string_tag});
            } else if (*s >= '0' && *s <= '9') {
                temp.push(parse_number(s, &s));
            } else if (s[0] == '-' && s[1] >= '0' && s[1] <= '9') {
                temp.push(-parse_number(++s, &s));
            } else if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
                s += 5;
                temp.push({false, bool_tag});
            } else if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
                s += 4;
                temp.push({true, bool_tag});
            } else if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
                s += 4;
                temp.push({0, null_tag});
            } else {
                abort();
            }
        }
        result.push(temp.top());
        return result;
    }
};

struct view {
    const value *_data;
    value _value;

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
        return (const char *)(_data + _value.payload);
    }

    view operator[](size_t index) const {
        assert(is_array() || is_object());
        return {_data, _data[_value.payload + index + 1]};
    }

    size_t size() const {
        assert(is_array() || is_object());
        return _data[_value.payload].payload;
    }
};
}
