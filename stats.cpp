#include "gason2.h"
#include "gason2dump.h"
#include <stdio.h>
#include <stdlib.h>

struct Stat {
    size_t objectCount;
    size_t arrayCount;
    size_t numberCount;
    size_t stringCount;
    size_t trueCount;
    size_t falseCount;
    size_t nullCount;

    size_t memberCount;  // Number of members in all objects
    size_t elementCount; // Number of elements in all arrays
    size_t stringLength; // Number of code units in all strings
};

static void GenStat(Stat &stat, const gason2::node &v) {
    switch (v.type()) {
    case gason2::type::array:
        for (auto i : v.elements())
            GenStat(stat, i);
        stat.elementCount += v.size();
        stat.arrayCount++;
        break;

    case gason2::type::object:
        for (size_t i = 0; i < v.size(); i += 2) {
            stat.stringLength += strlen(v[i].to_string());
            GenStat(stat, v[i + 1]);
        }
        stat.memberCount += v.size() / 2;
        stat.stringCount += v.size() / 2;
        stat.objectCount++;
        break;

    case gason2::type::string:
        stat.stringCount++;
        stat.stringLength += strlen(v.to_string());
        break;

    case gason2::type::number:
        stat.numberCount++;
        break;

    case gason2::type::boolean:
        if (v.to_bool())
            stat.trueCount++;
        else
            stat.falseCount++;
        break;

    case gason2::type::null:
        stat.nullCount++;
        break;

    default:
        break;
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            exit(EXIT_FAILURE);
        }

        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        gason2::vector<char> source;
        source.resize(size + 1);
        source.back() = '\0';
        fread(source.data(), 1, size, fp);
        fclose(fp);

        gason2::document doc;
        if (!doc.parse(source.data())) {
            gason2::dump::print_error(argv[i], source.data(), doc);
        }

        Stat stat = {};
        GenStat(stat, doc);
        printf("%s: %zd %zd %zd %zd %zd %zd %zd %zd %zd %zd\n",
               argv[i],
               stat.objectCount,
               stat.arrayCount,
               stat.numberCount,
               stat.stringCount,
               stat.trueCount,
               stat.falseCount,
               stat.nullCount,
               stat.memberCount,
               stat.elementCount,
               stat.stringLength);
    }

    return 0;
}
