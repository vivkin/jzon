#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

namespace gason2 {
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

enum class type : unsigned int {
    number = 0xFFF80000,
    null,
    boolean,
    string,
    array,
    object,
};

enum class error : unsigned int {
    expecting_string = 0xFFF90000,
    expecting_value,
    invalid_literal_name,
    invalid_number,
    invalid_string_char,
    invalid_string_escape,
    invalid_surrogate_pair,
    missing_colon,
    missing_comma_or_bracket,
    unexpected_character,
};

union value {
    char bytes[8];
    double number;
    struct {
        unsigned int payload;
        union {
            gason2::type type;
            gason2::error error;
        } tag;
    };

    value() = default;
    constexpr value(double x) : number(x) {}
    constexpr value(type t, size_t x = 0) : payload(x), tag{t} {}
    constexpr value(error t) : tag{static_cast<type>(t)} {}

    constexpr bool is_nan() const { return tag.type > type::number; }
    constexpr bool is_error() const { return tag.type > type::object; }
};

class node {
protected:
    value _data;
    const value *_storage;

public:
    node(value data = type::null, const value *storage = nullptr) : _data(data), _storage(storage) {}
    node(const value *pointer, const value *storage) : _data(*pointer), _storage(storage) {}

    gason2::type type() const { return _data.is_nan() ? _data.tag.type : type::number; }

    bool is_number() const { return !_data.is_nan(); }
    bool is_null() const { return _data.tag.type == type::null; }
    bool is_bool() const { return _data.tag.type == type::boolean; }
    bool is_string() const { return _data.tag.type == type::string; }
    bool is_array() const { return _data.tag.type == type::array; }
    bool is_object() const { return _data.tag.type == type::object; }

    double to_number() const { return is_number() ? _data.number : 0; }
    bool to_bool() const { return is_bool() && _data.payload; }
    const char *to_string() const { return is_string() ? _storage[_data.payload].bytes : ""; }
    size_t size() const { return _data.tag.type > type::string ? _storage[_data.payload - 1].payload : 0; }
    node operator[](size_t index) const { return index < size() ? node{_storage + _data.payload + index, _storage} : node{}; }
    node operator[](const char *name) const {
        if (is_object())
            for (size_t i = 0, i_end = size(); i < i_end; i += 2)
                if (!strcmp(operator[](i).to_string(), name))
                    return operator[](i + 1);
        return {};
    }

    class member {
        const value *_pointer, *_storage;

    public:
        member(const value *pointer = nullptr, const value *storage = nullptr) : _pointer(pointer), _storage(storage) {}
        node name() const { return {_pointer + 0, _storage}; }
        node value() const { return {_pointer + 1, _storage}; }
    };

    template <size_t N, typename T>
    class iterator {
        const value *_pointer, *_storage;

    public:
        iterator(const value *pointer = nullptr, const value *storage = nullptr) : _pointer(pointer), _storage(storage) {}
        iterator &operator++() {
            _pointer += N;
            return *this;
        }
        iterator operator++(int) {
            auto temp = *this;
            operator++();
            return temp;
        }
        bool operator==(const iterator &x) const { return _pointer == x._pointer && _storage == x._storage; };
        bool operator!=(const iterator &x) const { return _pointer != x._pointer || _storage != x._storage; };
        T operator*() const { return {_pointer, _storage}; }
    };

    template <typename T>
    class iterator_range {
        T _first, _last;

    public:
        iterator_range(T first, T last) : _first(first), _last(last) {}
        T begin() const { return _first; }
        T end() const { return _last; }
    };

    iterator_range<iterator<1, node>> elements() const {
        return {{_storage + _data.payload, _storage}, {_storage + _data.payload + size(), _storage}};
    }

    iterator_range<iterator<2, member>> members() const {
        return {{_storage + _data.payload, _storage}, {_storage + _data.payload + size(), _storage}};
    }
};

struct stream {
    const char *_s;

    const char *c_str() const { return _s; }
    int peek() const { return static_cast<unsigned char>(*_s); }
    int getch() { return static_cast<unsigned char>(*_s++); }
    int skipws() {
        while (peek() == '\x20' || peek() == '\x9' || peek() == '\xD' || peek() == '\xA')
            getch();
        return peek();
    }
};

struct parser {
    static inline bool is_digit(int c) { return c >= '0' && c <= '9'; }

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
            significand = static_cast<double>(integer);
            while (is_digit(s.peek()))
                significand = (significand * 10) + (s.getch() - '0');
        }

        if (s.peek() == '.') {
            s.getch();

            while (is_digit(s.peek()) && integer < 0x1FFFFFFFFFFFFFull) {
                integer = (integer * 10) + (s.getch() - '0');
                --fraction;
            }

            while (is_digit(s.peek()))
                s.getch();
        }

        if (integer < 0x1999999999999999ull)
            significand = static_cast<double>(integer);

        if ((s.peek() | 0x20) == 'e') {
            s.getch();
            if (s.peek() == '-') {
                s.getch();
                if (!is_digit(s.peek()))
                    return error::invalid_number;
                while (is_digit(s.peek()))
                    exponent = (exponent * 10) - (s.getch() - '0');
            } else {
                if (s.peek() == '+')
                    s.getch();
                if (!is_digit(s.peek()))
                    return error::invalid_number;
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

    static int parse_hex(stream &s) {
        int cp = 0;
        for (int i = 0; i < 4; ++i) {
            if (is_digit(s.peek()))
                cp = (cp * 16) + (s.getch() - '0');
            else if ((s.peek() | 0x20) >= 'a' && (s.peek() | 0x20) <= 'f')
                cp = (cp * 16) + ((s.getch() | 0x20) - 'a' + 10);
            else
                return -1;
        }
        return cp;
    }

    static value parse_string(stream &s, vector<value> &v) {
        for (size_t length = 0, offset = v.size();;) {
            v.resize(v.size() + 4);

            char *first = (v.begin() + offset)->bytes + length;
            char *last = v.end()->bytes - 5;

            while (first < last) {
                int ch = s.getch();

                if (ch < ' ')
                    return error::invalid_string_char;

                if (ch == '"') {
                    *first++ = '\0';
                    length = first - (v.begin() + offset)->bytes;
                    v.resize(offset + ((length + sizeof(value)) / sizeof(value)));
                    return {type::string, offset};
                }

                if (ch == '\\') {
                    switch (s.getch()) {
                    // clang-format off
                    case '\x22': ch = '"'; break;
                    case '\x2F': ch = '/'; break;
                    case '\x5C': ch = '\\'; break;
                    case '\x62': ch = '\b'; break;
                    case '\x66': ch = '\f'; break;
                    case '\x6E': ch = '\n'; break;
                    case '\x72': ch = '\r'; break;
                    case '\x74': ch = '\t'; break;
                    // clang-format on
                    case '\x75':
                        if ((ch = parse_hex(s)) < 0)
                            return error::invalid_string_escape;

                        if (ch >= 0xD800 && ch <= 0xDBFF) {
                            if (s.getch() != '\\' || s.getch() != '\x75')
                                return error::invalid_surrogate_pair;
                            int low = parse_hex(s);
                            if (low < 0xDC00 || low > 0xDFFF)
                                return error::invalid_surrogate_pair;
                            ch = 0x10000 + ((ch & 0x3FF) << 10) + (low & 0x3FF);
                        }

                        if (ch < 0x80) {
                            *first++ = (char)ch;
                        } else if (ch < 0x800) {
                            *first++ = 0xC0 | ((char)(ch >> 6));
                            *first++ = 0x80 | (ch & 0x3F);
                        } else if (ch < 0x10000) {
                            *first++ = 0xE0 | ((char)(ch >> 12));
                            *first++ = 0x80 | ((ch >> 6) & 0x3F);
                            *first++ = 0x80 | (ch & 0x3F);
                        } else {
                            *first++ = 0xF0 | ((char)(ch >> 18));
                            *first++ = 0x80 | ((ch >> 12) & 0x3F);
                            *first++ = 0x80 | ((ch >> 6) & 0x3F);
                            *first++ = 0x80 | (ch & 0x3F);
                        }
                        continue;
                    default:
                        return error::invalid_string_escape;
                    }
                }

                *first++ = ch;
            }

            length = first - (v.begin() + offset)->bytes;
        }
    }

    vector<value> _backlog;
    vector<value> _storage;

    value parse_value(stream &s) {
        switch (s.skipws()) {
        case '"':
            s.getch();
            return parse_string(s, _storage);
        case 'f':
            s.getch();
            if (s.getch() == 'a' && s.getch() == 'l' && s.getch() == 's' && s.getch() == 'e')
                return {type::boolean, false};
            return error::invalid_literal_name;
        case 't':
            s.getch();
            if (s.getch() == 'r' && s.getch() == 'u' && s.getch() == 'e')
                return {type::boolean, true};
            return error::invalid_literal_name;
        case 'n':
            s.getch();
            if (s.getch() == 'u' && s.getch() == 'l' && s.getch() == 'l')
                return type::null;
            return error::invalid_literal_name;
        case '[': {
            s.getch();
            size_t frame = _backlog.size();
            if (s.skipws() != ']') {
            element:
                _backlog.push_back(parse_value(s));
                if (_backlog.back().is_error())
                    return _backlog.back();

                if (s.skipws() == ',') {
                    s.getch();
                    goto element;
                }
            }

            if (s.getch() != ']')
                return error::missing_comma_or_bracket;

            size_t size = _backlog.size() - frame;
            _storage.push_back({type::array, size});
            _storage.append(_backlog.begin() + frame, size);
            _backlog.resize(frame);
            return {type::array, _storage.size() - size};
        }
        case '{': {
            s.getch();
            size_t frame = _backlog.size();
            if (s.skipws() != '}') {
            member:
                if (s.peek() != '"')
                    return error::expecting_string;
                s.getch();
                _backlog.push_back(parse_string(s, _storage));
                if (_backlog.back().is_error())
                    return _backlog.back();

                if (s.skipws() != ':')
                    return error::missing_colon;
                s.getch();
                _backlog.push_back(parse_value(s));
                if (_backlog.back().is_error())
                    return _backlog.back();

                if (s.skipws() == ',') {
                    s.getch();
                    s.skipws();
                    goto member;
                }
            }

            if (s.getch() != '}')
                return error::missing_comma_or_bracket;

            size_t size = _backlog.size() - frame;
            _storage.push_back({type::object, size});
            _storage.append(_backlog.begin() + frame, size);
            _backlog.resize(frame);
            return {type::object, _storage.size() - size};
        }
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
        return error::expecting_value;
    }
};

class document : public node {
    vector<value> _storage;

public:
    bool parse(const char *json) {
        stream s{json};
        parser p;
        _data = p.parse_value(s);

        if (!_data.is_error() && s.skipws())
            _data = error::unexpected_character;

        if (_data.is_error()) {
            _data.payload = s.c_str() - json;
            return false;
        }

        _storage = static_cast<vector<value> &&>(p._storage);
        node::_storage = _storage.data();

        return true;
    }

    error error_code() const { return _data.tag.error; }
    size_t error_offset() const { return _data.is_error() ? _data.payload : 0; }
};
}
