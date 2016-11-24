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

static void GenStat(Stat &stat, const gason2::value &v) {
    switch (v.type()) {
    case gason2::type::array:
        for (auto i : v.elements())
            GenStat(stat, i);
        stat.elementCount += v.size();
        stat.arrayCount++;
        break;

    case gason2::type::object:
        for (auto i : v.members()) {
            stat.stringLength += strlen(i.name().to_string());
            GenStat(stat, i.value());
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
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s [file ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("%10.10s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s\n",
           "object",
           "array",
           "number",
           "string",
           "true",
           "false",
           "null",
           "member",
           "element",
           "#string");

    for (int i = 1; i < argc; ++i) {
        FILE *fp = strcmp(argv[i], "-") ? fopen(argv[i], "rb") : stdin;
        if (!fp) {
            perror(argv[i]);
            exit(EXIT_FAILURE);
        }

        gason2::vector<char> src;
        while (!feof(fp)) {
            char buf[BUFSIZ];
            src.append(buf, fread(buf, 1, sizeof(buf), fp));
        }
        src.push_back('\0');
        src.pop_back();
        fclose(fp);

        gason2::document doc;
        if (!doc.parse(src.data()))
            gason2::dump::print_error(argv[i], src.data(), doc);

        Stat stat = {};
        GenStat(stat, doc);
        printf("%10zu %10zu %10zu %10zu %10zu %10zu %10zu %10zu %10zu %10zu %s\n",
               stat.objectCount,
               stat.arrayCount,
               stat.numberCount,
               stat.stringCount,
               stat.trueCount,
               stat.falseCount,
               stat.nullCount,
               stat.memberCount,
               stat.elementCount,
               stat.stringLength,
               argv[i]);
    }

    return 0;
}
