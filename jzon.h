#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

    vector(const vector &x) {
        *this = x;
    }

    vector(vector &&x) : _data(x._data), _size(x._size), _capacity(x._capacity) {
        x._data = nullptr;
        x._size = x._capacity = 0;
    }

    vector &operator=(const vector &x) {
        resize(x._size);
        memcpy(_data, x._data, sizeof(T) * x._size);
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

    void reserve(size_t n) {
        if (_capacity < n)
            set_capacity(_capacity * 2 < n ? n : _capacity * 2);
    }

    void resize(size_t n) {
        reserve(n);
        _size = n;
    }

    void append(const T *x, size_t n) {
        resize(_size + n);
        memcpy(_data + _size - n, x, sizeof(T) * n);
    }

    void push_back(const T &x) {
        reserve(_size + 1);
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

    T *data() { return _data; }
    const T *data() const { return _data; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    bool empty() const { return _size == 0; }
};

enum value_tag {
    number_tag = 0xFFF00000,
    null_tag,
    bool_tag,
    string_tag,
    array_tag,
    object_tag,

    prefix_tag,

    token_end_array,
    token_end_object,
    token_name_separator,
    token_value_separator,

    error_tag,
    error_expecting_string,
    error_expecting_value,
    error_invalid_literal_name,
    error_invalid_number,
    error_invalid_string_char,
    error_invalid_string_escape,
    error_invalid_surrogate_pair,
    error_missing_colon,
    error_missing_comma_or_bracket,
    error_second_root,
    error_unexpected_character,
};

union value {
    char s[8];
    double number;
    struct {
        unsigned int payload;
        unsigned int tag;
    };

    value() = default;
    constexpr value(double n) : number(n) {}
    constexpr value(value_tag tag) : payload(0), tag(tag) {}
    constexpr value(size_t payload, value_tag tag) : payload(payload), tag(tag) {}
    constexpr bool is_nan() const { return tag > number_tag; }
};

class view {
    const vector<value> &_data;
    value _value;

public:
    view(const vector<value> &data, value v) : _data(data), _value(v) {}
    view(const vector<value> &data) : view(data, data.back()) {}

    value_tag tag() const {
        return _value.is_nan() ? value_tag(_value.tag) : number_tag;
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
    struct stream {
        const char *_s;

        char skipws() {
            while (*_s == '\x20' || *_s == '\x9' || *_s == '\xD' || *_s == '\xA')
                ++_s;
            return *_s;
        }
        char getch() { return *_s++; }
        char peek() const { return *_s; }
    };

    static inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

    static value parse_number(stream &s) {
        unsigned long long integer = 0;
        double significand = 0;
        int fraction = 0;
        int exponent = 0;

        integer = s.getch() - '0';
        if (integer)
            while (is_digit(s.peek()) && integer < 0x1999999999999999ull)
                integer = (integer * 10) + (s.getch() - '0');

        if (integer >= 0x1999999999999999ull) {
            significand = integer;
            while (is_digit(s.peek()))
                significand = (significand * 10) + (s.getch() - '0');
        }

        if (s.peek() == '.') {
            s.getch();

            while (is_digit(s.peek()) && integer < 0x1fffffffffffffull) {
                integer = (integer * 10) + (s.getch() - '0');
                --fraction;
            }

            while (is_digit(s.peek()))
                s.getch();
        }

        if (integer < 0x1999999999999999ull)
            significand = integer;

        if ((s.peek() | 0x20) == 'e') {
            s.getch();
            if (s.peek() == '-') {
                s.getch();
                if (!is_digit(s.peek()))
                    return error_invalid_number;
                while (is_digit(s.peek()))
                    exponent = (exponent * 10) - (s.getch() - '0');
            } else {
                if (s.peek() == '+')
                    s.getch();
                if (!is_digit(s.peek()))
                    return error_invalid_number;
                while (is_digit(s.peek()))
                    exponent = (exponent * 10) + (s.getch() - '0');
            }
        }

        static constexpr double exp10[] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39, 1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47, 1e48, 1e49, 1e50, 1e51, 1e52, 1e53, 1e54, 1e55, 1e56, 1e57, 1e58, 1e59, 1e60, 1e61, 1e62, 1e63, 1e64, 1e65, 1e66, 1e67, 1e68, 1e69, 1e70, 1e71, 1e72, 1e73, 1e74, 1e75, 1e76, 1e77, 1e78, 1e79, 1e80, 1e81, 1e82, 1e83, 1e84, 1e85, 1e86, 1e87, 1e88, 1e89, 1e90, 1e91, 1e92, 1e93, 1e94, 1e95, 1e96, 1e97, 1e98, 1e99, 1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109, 1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219, 1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308};
        exponent += fraction;
        if (exponent < 0) {
            if (exponent < -308) {
                significand /= exp10[308];
                exponent += 308;
            }
            significand = exponent < -308 ? 0.0 : significand / exp10[-exponent];
        } else {
            significand *= exp10[exponent < 308 ? exponent : 308];
        }

        return significand;
    }

    static int unescape(stream &s) {
        switch (s.getch()) {
        case '\x22':
            return '"';
        case '\x2F':
            return '/';
        case '\x5C':
            return '\\';
        case '\x62':
            return '\b';
        case '\x66':
            return '\f';
        case '\x6E':
            return '\n';
        case '\x72':
            return '\r';
        case '\x74':
            return '\t';
        case '\x75': {
            static constexpr char hex2dec[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15};
            unsigned int cp = 0;
            for (int i = 0; i < 4; ++i) {
                unsigned int c = s.getch() - '0';
                if (c < sizeof(hex2dec) && hex2dec[c] >= 0)
                    cp = cp * 16 + hex2dec[c];
                else
                    return -1;
            }
            return cp;
        }
        default:
            return -1;
        }
    }

    static value parse_string(vector<value> &v, stream &s) {
        size_t offset = v.size();
        size_t size = 0;
        for (;;) {
            v.resize(v.size() + 8);
            char *span = v[offset].s;

            for (size_t size_end = size + sizeof(value) * 7; size < size_end; ++size) {
                span[size] = s.getch();
                switch (span[size]) {
                case '"':
                    span[size] = '\0';
                    v.resize(offset + (size + 8) / 8);
                    return {offset, string_tag};

                case '\\': {
                    int cp = unescape(s);
                    if (cp >= 0xD800 && cp <= 0xDBFF) {
                        if (s.getch() != '\\')
                            return error_invalid_surrogate_pair;
                        int low = unescape(s);
                        if (low < 0xDC00 && low > 0xDFFF)
                            return error_invalid_surrogate_pair;
                        cp = 0x10000 + ((cp & 0x3FF) << 10) + (low & 0x3FF);
                    }

                    if (cp < 0)
                        return error_invalid_string_escape;
                    else if (cp < 0x80) {
                        span[size] = cp;
                    } else if (cp < 0x800) {
                        span[size] = 0xC0 | (cp >> 6);
                        span[++size] = 0x80 | (cp & 0x3F);
                    } else if (cp < 0xFFFF) {
                        span[size] = 0xE0 | (cp >> 12);
                        span[++size] = 0x80 | ((cp >> 6) & 0x3F);
                        span[++size] = 0x80 | (cp & 0x3F);
                    } else {
                        span[size] = 0xF0 | (cp >> 18);
                        span[++size] = 0x80 | ((cp >> 12) & 0x3F);
                        span[++size] = 0x80 | ((cp >> 6) & 0x3F);
                        span[++size] = 0x80 | (cp & 0x3F);
                    }
                    continue;
                }

                default:
                    if ((unsigned int)span[size] < ' ')
                        return error_invalid_string_char;
                }
            }
        }
    }

    vector<value> _backlog;
    vector<value> _stack;

    value parse_value(stream &s) {
        switch (s.skipws()) {
        case '"':
            s.getch();
            return parse_string(_stack, s);
        case ',':
            s.getch();
            return token_value_separator;
        case ':':
            s.getch();
            return token_name_separator;
        case 'f':
            s.getch();
            if (s.getch() == 'a' && s.getch() == 'l' && s.getch() == 's' && s.getch() == 'e')
                return {false, bool_tag};
            return error_invalid_literal_name;
        case 't':
            s.getch();
            if (s.getch() == 'r' && s.getch() == 'u' && s.getch() == 'e')
                return {true, bool_tag};
            return error_invalid_literal_name;
        case 'n':
            s.getch();
            if (s.getch() == 'u' && s.getch() == 'l' && s.getch() == 'l')
                return {0, null_tag};
            return error_invalid_literal_name;
        case '[':
            s.getch();
            return parse_array(s);
        case ']':
            s.getch();
            return token_end_array;
        case '{':
            s.getch();
            return parse_object(s);
        case '}':
            s.getch();
            return token_end_object;
        case '-':
            s.getch();
            if (is_digit(s.peek()))
                return -parse_number(s).number;
            break;
        default:
            if (is_digit(s.peek()))
                return parse_number(s);
            break;
        }
        return error_unexpected_character;
    }

    value parse_array(stream &s) {
        size_t frame = _backlog.size();

        value tok = parse_value(s);

        if (tok.tag < prefix_tag) {
            _backlog.push_back(tok);
            for (;;) {
                tok = parse_value(s);
                if (tok.tag != token_value_separator)
                    break;

                tok = parse_value(s);
                if (tok.tag > prefix_tag)
                    return tok.tag > error_tag ? tok : error_expecting_value;
                _backlog.push_back(tok);
            }
        }

        if (tok.tag == token_end_array) {
            size_t size = _backlog.size() - frame;
            _stack.push_back({size, prefix_tag});
            _stack.append(_backlog.end() - size, size);
            _backlog.resize(_backlog.size() - size);
            return {_stack.size() - size, array_tag};
        }

        return tok.tag > error_tag ? tok : error_missing_comma_or_bracket;
    }

    value parse_object(stream &s) {
        size_t frame = _backlog.size();

        value tok = parse_value(s);

        if (tok.tag == string_tag) {
            _backlog.push_back(tok);

            tok = parse_value(s);
            if (tok.tag != token_name_separator)
                return tok.tag > error_tag ? tok : error_missing_colon;

            tok = parse_value(s);
            if (tok.tag > prefix_tag)
                return tok.tag > error_tag ? tok : error_expecting_value;
            _backlog.push_back(tok);

            for (;;) {
                tok = parse_value(s);
                if (tok.tag != token_value_separator)
                    break;

                tok = parse_value(s);
                if (tok.tag != string_tag)
                    return tok.tag > error_tag ? tok : error_expecting_string;
                _backlog.push_back(tok);

                tok = parse_value(s);
                if (tok.tag != token_name_separator)
                    return tok.tag > error_tag ? tok : error_missing_colon;

                tok = parse_value(s);
                if (tok.tag > prefix_tag)
                    return tok.tag > error_tag ? tok : error_expecting_value;
                _backlog.push_back(tok);
            }
        }

        if (tok.tag == token_end_object) {
            size_t size = _backlog.size() - frame;
            _stack.push_back({size, prefix_tag});
            _stack.append(_backlog.end() - size, size);
            _backlog.resize(_backlog.size() - size);
            return {_stack.size() - size, object_tag};
        }

        return tok.tag > error_tag ? tok : error_missing_comma_or_bracket;
    }

    static constexpr size_t error_buffer_size = 80 * 3;

    static void print_error(char errbuf[error_buffer_size], const char *s, const char *endptr, unsigned int errnum) {
        size_t lineno = 1;
        const char *left = s;
        const char *right = s;
        while (*right) {
            if (*right++ == '\n') {
                if (endptr < right)
                    break;
                left = right;
                ++lineno;
            }
        }
        size_t column = endptr - left;
        if (column > 80)
            left = endptr - 40;
        if (right - endptr > 40)
            right = endptr + 40;

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

        snprintf(errbuf, error_buffer_size, "%zd:%zd: %s\n%.*s\n%*s", lineno, column, err2str[errnum - error_expecting_string], int(right - left), left, int(endptr - left), "^");
    }

    static vector<value> parse(const char *s, char errbuf[error_buffer_size] = nullptr) {
        parser p;
        stream in{s};
        value root = p.parse_value(in);
        if (root.tag < error_tag && in.skipws())
            root = error_second_root;
        if (root.tag > error_tag) {
            if (errbuf)
                print_error(errbuf, s, in._s, root.tag);
            return {};
        }
        p._stack.push_back(root);
        return static_cast<vector<value> &&>(p._stack);
    }
};
}
