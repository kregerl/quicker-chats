#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chatter.h"


int main(int argc, char** argv) {
    const char* usage = "USAGE: quickerchat [OPTIONS] FILE\n"
                        "  FILE should be a mappings file for mapping keys to strings.\n"
                        "  OPTIONS\n"
                        "    -id    The device id to use, from xinput\n";

    if (argc > 4 || argc == 1) {
        printf("%s\n", usage);
        return EXIT_SUCCESS;
    }

    int id = -1;
    const char* mappings_path = NULL;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (strcmp(arg, "-id") == 0) {
            if (i + 1 >= argc) {
                printf("Expected an argument after '-id'\n");
                printf("%s\n", usage);
                return EXIT_SUCCESS;
            }
            id = (int) strtol(argv[i + 1], NULL, 10);
            i += 1;
        } else {
            mappings_path = argv[i];
        }
    }

    if (!mappings_path) {
        fprintf(stderr, "Expected a mappings file path\n");
        printf("%s\n", usage);
        exit(EXIT_FAILURE);
    }

    init(id, mappings_path);
    return EXIT_SUCCESS;
}
