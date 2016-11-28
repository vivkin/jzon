#include "gason2.h"
#include "gason2dump.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s [-v] [file ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    bool verbose = false;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            verbose = true;
            continue;
        }

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
        if (doc.parse(src.data())) {
            gason2::vector<char> buffer;
            if (verbose)
                gason2::dump::prettify(buffer, doc);
            else
                gason2::dump::stringify(buffer, doc);
            buffer.push_back('\0');
            buffer.pop_back();
            printf("%s\n", buffer.begin());
        } else {
            gason2::dump::print_error(argv[i], src.data(), doc);
        }
    }

    return 0;
}
