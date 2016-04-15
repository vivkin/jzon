#include "jzon.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>

void printJsonString(const char *s) {
    putchar('"');
    while (*s) {
        int c = *s++;
        if (c == '"')
            printf("\\\"");
        else if (c == '\\')
            printf("\\\\");
        else if (c == '\b')
            printf("\\b");
        else if (c == '\f')
            printf("\\f");
        else if (c == '\n')
            printf("\\n");
        else if (c == '\r')
            printf("\\r");
        else if (c == '\t')
            printf("\\t");
        else
            putchar(c);
    }
    putchar('"');
}

void printJson(jzon::view v, int indent = 0) {
    using namespace jzon;
    switch (v.tag()) {
    case array_tag: {
        if (v.size()) {
            puts("[");
            for (size_t i = 0; i < v.size(); ++i) {
                printf("%*s", indent + 2, "");
                printJson(v[i], indent + 2);
                puts(",");
            }
            printf("%*s]", indent, "");
        } else {
            printf("[]");
        }
    } break;
    case object_tag: {
        if (v.size()) {
            puts("{");
            for (size_t i = 0; i < v.size(); i += 2) {
                printf("%*s", indent + 2, "");
                printJson(v[i + 0], indent + 2);
                printf(": ");
                printJson(v[i + 1], indent + 2);
                puts(",");
            }
            printf("%*s}", indent, "");
        } else {
            printf("{}");
        }
    } break;
    case string_tag:
        printJsonString(v.get_string());
        break;
    case number_tag:
        printf("%f", v.get_number());
        break;
    case bool_tag:
        printf(v.get_bool() ? "true" : "false");
        break;
    case null_tag:
        printf("null");
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
        std::vector<char> buffer;
        buffer.resize(size + 1);
        fread(buffer.data(), 1, size, fp);
        fclose(fp);

        auto jzs = jzon::parser::parse(buffer.data());
        printJson({jzs._data, jzs.top()});
        putchar('\n');
    }
    return 0;
}
