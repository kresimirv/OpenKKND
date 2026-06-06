#include "src/Config.h"
#include <cstdio>
#include <cstring>

int Config::vga_width = 640;
int Config::vga_height = 480;

void Config::load(const char *directory) {
    char path[512];
    snprintf(path, sizeof(path), "%s/config.txt", directory);

    FILE *f = fopen(path, "r");
    if (!f) {
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        char key[128] = {0};
        int value = 0;
        if (sscanf(line, " %127[^=]= %d", key, &value) == 2) {
            if (strcmp(key, "vga_resolution_width") == 0 && value > 0) {
                vga_width = value;
            } else if (strcmp(key, "vga_resolution_height") == 0 && value > 0) {
                vga_height = value;
            }
        }
    }

    fclose(f);
}
