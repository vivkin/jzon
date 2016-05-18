#include "jzon.h"
#include "gason2dump.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    bool pretty = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--pretty")) {
            pretty = true;
            continue;
        }

        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            exit(EXIT_FAILURE);
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        jzon::vector<char> source;
        source.resize(size + 1);
        fread(source.data(), 1, size, fp);
        fclose(fp);

        auto doc = jzon::parser::parse(source.data());
        if (doc.empty()) {
            fprintf(stderr, "%s: error: parse failed\n", argv[i]);
            continue;
        }

        auto root = jzon::view(doc);
        jzon::vector<char> buffer;
        if (pretty)
            gason2::dump::prettify(buffer, root);
        else
            gason2::dump::stringify(buffer, root);
        buffer.push_back('\0');
        buffer.pop_back();
        printf("%s\n", buffer.begin());
    }
    return 0;
}
