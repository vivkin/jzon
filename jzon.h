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

    void set_capacity(size_t n) {
        if (n < _size) {
            _size = n;
        }
        _data = static_cast<T *>(realloc(_data, sizeof(T) * n));
        _capacity = n;
    }

    void grow(size_t n) {
        if (_capacity < n) {
            size_t new_capacity = _capacity * 2 + 8;
            set_capacity(new_capacity < n ? n : new_capacity);
        }
    }

    void resize(size_t n) {
        grow(n);
        _size = n;
    }

    void append(const T *x, size_t n) {
        size_t new_size = _size + n;
        grow(new_size);
        memcpy(_data + _size, x, sizeof(T) * n);
        _size = new_size;
    }

    void push_back(const T &x) {
        grow(_size + 1);
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
    enum error_code {
        invalid_number = 1,
        invalid_exponent,
        invalid_string_char,
        invalid_string_escape,
        invalid_surrogate_pair,
        missing_terminating_quote,
        missing_comma,
        missing_colon,
        mismatch_brace,
        document_empty,
        stack_underflow,
        unexpected_character,
    };

    static inline const char *skip_ws(const char *s) {
        while (*s == '\x20' || *s == '\x9' || *s == '\xD' || *s == '\xA')
            ++s;
        return s;
    }

    static int parse_number(value *dst, const char **src) {
        const char *s = *src;
        bool negative = *s == '-';
        double significant = 0;
        int num_digits = 0;
        int fraction = 0;
        int exponent = 0;

        if (negative)
            ++s;

        if (*s == '0' && (s[1] >= '0' && s[1] <= '9'))
            return invalid_number;

        for (; *s >= '0' && *s <= '9'; ++s, ++num_digits)
            significant = (significant * 10) + (*s - '0');

        if (*s == '.') {
            ++s;

            for (; *s >= '0' && *s <= '9'; ++s)
                if (num_digits < 17) {
                    significant = (significant * 10) + (*s - '0');
                    ++num_digits;
                    --fraction;
                }
        }

        if (*s == 'e' || *s == 'E') {
            ++s;

            if (*s == '-') {
                ++s;

                if (*s < '0' || *s > '9')
                    return invalid_exponent;

                while (*s >= '0' && *s <= '9')
                    exponent = (exponent * 10) - (*s++ - '0');
            } else {
                if (*s == '+')
                    ++s;

                if (*s < '0' || *s > '9')
                    return invalid_exponent;

                while (*s >= '0' && *s <= '9')
                    exponent = (exponent * 10) + (*s++ - '0');
            }
        }

        static constexpr double exp10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39, 1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47, 1e48, 1e49, 1e50, 1e51, 1e52, 1e53, 1e54, 1e55, 1e56, 1e57, 1e58, 1e59, 1e60, 1e61, 1e62, 1e63, 1e64, 1e65, 1e66, 1e67, 1e68, 1e69, 1e70, 1e71, 1e72, 1e73, 1e74, 1e75, 1e76, 1e77, 1e78, 1e79, 1e80, 1e81, 1e82, 1e83, 1e84, 1e85, 1e86, 1e87, 1e88, 1e89, 1e90, 1e91, 1e92, 1e93, 1e94, 1e95, 1e96, 1e97, 1e98, 1e99, 1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109, 1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219, 1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308};
        exponent += fraction;
        if (exponent < 0) {
            while (exponent < -308) {
                significant /= exp10[308];
                exponent += 308;
            }
            significant /= exp10[-exponent];
        } else {
            significant *= exp10[exponent < 308 ? exponent : 308];
        }

        *dst = negative ? -significant : significant;
        *src = s;
        return 0;
    }

    static int parse_string(vector<value> &v, value *dst, const char **src) {
        const char *s = *src;
        while (*s >= ' ' && *s != '"' && *s != '\\')
            ++s;

        size_t offset = v.size();
        size_t n = s - *src;
        if (n) {
            v.resize(offset + (n + 7) / 8);
            memcpy(v[offset].s, *src, n);
        }

#define PUT(c) v[offset].s[n++] = (c)

        for (;;) {
            if (n % sizeof(value) == 0)
                v.push_back(value());

            unsigned int cp, c = *s++;

            if (c < ' ')
                return (c == '\r' || c == '\n') ? missing_terminating_quote : invalid_string_char;

            if (c == '"') {
                PUT('\0');
                *dst = {offset, string_tag};
                *src = s;
                return 0;
            }

            if (c == '\\') {
                c = *s++;
                switch (c) {
                case 'b':
                    PUT('\b');
                    continue;
                case 't':
                    PUT('\t');
                    continue;
                case 'n':
                    PUT('\n');
                    continue;
                case 'f':
                    PUT('\f');
                    continue;
                case 'r':
                    PUT('\r');
                    continue;
                case '/':
                case '"':
                case '\\':
                    PUT(c);
                    continue;
                case 'u':
                    static constexpr unsigned char hex2dec[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};

                    cp = 0;
                    for (int i = 0; i < 4; ++i, ++s)
                        cp = cp * 16 + hex2dec[((unsigned char)*s & 0x7F) - '0'];

                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (s[0] != '\\' && s[1] != 'u')
                            return invalid_surrogate_pair;
                        s += 2;

                        unsigned int high = cp;
                        cp = 0;
                        for (int i = 0; i < 4; ++i, ++s)
                            cp = cp * 16 + hex2dec[((unsigned char)*s & 0x7F) - '0'];

                        if (cp < 0xDC00 && cp > 0xDFFF)
                            return invalid_surrogate_pair;

                        cp = 0x10000 + ((high & 0x3FF) << 10) + (cp & 0x3FF);
                    }

                    if (cp < 0x80) {
                        PUT(cp);
                    } else if (cp < 0x800) {
                        if (n % sizeof(value) > 6)
                            v.push_back(value());
                        PUT(0xC0 | (cp >> 6));
                        PUT(0x80 | (cp & 0x3F));
                    } else if (cp < 0xFFFF) {
                        if (n % sizeof(value) > 5)
                            v.push_back(value());
                        PUT(0xE0 | (cp >> 12));
                        PUT(0x80 | ((cp >> 6) & 0x3F));
                        PUT(0x80 | (cp & 0x3F));
                    } else {
                        if (n % sizeof(value) > 4)
                            v.push_back(value());
                        PUT(0xF0 | (cp >> 18));
                        PUT(0x80 | ((cp >> 12) & 0x3F));
                        PUT(0x80 | ((cp >> 6) & 0x3F));
                        PUT(0x80 | (cp & 0x3F));
                    }
                    continue;
                default:
                    return invalid_string_escape;
                }
            }

            PUT(c);
        }
#undef PUT
    }

    static int parse(vector<value> &v, const char *s) {
        vector<value> backlog;
        vector<value> &result = v;
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
                if (frame == 0)
                    return stack_underflow;

                value saved = backlog[frame - 1];
                if (saved.tag == array_tag && *s != ']')
                    return mismatch_brace;

                size_t size = backlog.size() - frame;
                result.push_back({size, (value_tag)(saved.tag | prefix_flag)});
                size_t offset = result.size();
                result.append(backlog.end() - size, size);

                backlog.resize(frame);
                backlog.back() = {offset, saved.tag};
                frame = saved.payload;

                ++s;
            } else if (*s == ',') {
                ++s;
            } else if (*s == ':') {
                ++s;
            } else if (*s == '"') {
                ++s;
                value d;
                int res = parse_string(result, &d, &s);
                if (res)
                    return res;
                backlog.push_back(d);
            } else if ((*s >= '0' && *s <= '9') || *s == '-') {
                value d;
                int res = parse_number(&d, &s);
                if (res)
                    return res;
                backlog.push_back(d);
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
                return unexpected_character;
            }
        }
        result.push_back(backlog.back());
        return 0;
    }

    static vector<value> parse(const char *s) {
        vector<value> data;
        parse(data, s);
        return data;
    }
};

inline void stringify(vector<char> &s, view v) {
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
            for (size_t i = 0; i < v.size(); i += 2, comma = ',') {
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
}
