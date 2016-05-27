#include "gason2.h"
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

        gason2::document doc;
        if (doc.parse(source.data())) {
            gason2::vector<char> buffer;
            if (pretty)
                gason2::dump::prettify(buffer, doc);
            else
                gason2::dump::stringify(buffer, doc);
            buffer.push_back('\0');
            buffer.pop_back();
            printf("%s\n", buffer.begin());
        } else {
            char buffer[160];
            gason2::dump::error_string(buffer, sizeof(buffer), source.data(), doc.error_offset(), doc.error_num());
            fprintf(stderr, "%s:%s\n", argv[i], buffer);
        }
    }
    return 0;
}
