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
        gason2::vector<char> source;
        source.resize(size + 1);
        source[size] = '\0';
        fread(source.data(), 1, size, fp);
        fclose(fp);

        char errbuf[gason2::parser::error_buffer_size];
        auto doc = gason2::parser::parse(source.data(), errbuf);
        if (doc.empty()) {
            fprintf(stderr, "%s:%s\n", argv[i], errbuf);
            continue;
        }

        auto root = gason2::view(doc);
        gason2::vector<char> buffer;
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
