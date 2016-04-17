#include "jzon.h"
#include <stdlib.h>
#include <stdio.h>
#include <vector>

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
        std::vector<char> source;
        source.resize(size);
        fread(source.data(), 1, size, fp);
        fclose(fp);

        auto jzs = jzon::parser::parse(source.data());
        auto root = jzon::view(jzs);
        jzon::vector<char> buffer;
        root.stringify(buffer);
        buffer.push_back('\0');
        buffer.pop_back();
        printf("%s\n", buffer.begin());
    }
    return 0;
}
