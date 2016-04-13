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

void printJson(const jzon::value *data, jzon::value x, int indent = 0) {
    using namespace jzon;
    switch (x.tag()) {
    case array_tag: {
        auto a = array(data, x);
        if (a.size()) {
            puts("[");
            for (auto i : a) {
                printf("%*s", indent + 2, "");
                printJson(data, i, indent + 2);
                puts(",");
            }
            printf("%*s]", indent, "");
        } else {
            printf("[]");
        }
    } break;
    case object_tag: {
        auto a = array(data, x);
        if (a.size()) {
            puts("{");
            for (size_t i = 0; i < a.size(); i += 2) {
                printf("%*s", indent + 2, "");
                printJson(data, a[i + 0], indent + 2);
                printf(": ");
                printJson(data, a[i + 1], indent + 2);
                puts(",");
            }
            printf("%*s}", indent, "");
        } else {
            printf("{}");
        }
    } break;
    case string_tag:
        printJsonString((const char *)(data + x.integer));
        break;
    case number_tag:
        printf("%f", x.number);
        break;
    case false_tag:
        printf("false");
        break;
    case true_tag:
        printf("true");
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
        printJson(jzs._data, jzs.top());
        putchar('\n');
    }
    return 0;
}
