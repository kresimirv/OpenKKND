#pragma once

class Config {
public:
    static int vga_width;
    static int vga_height;
    static bool vga_fullscreen;

    static void load(const char *directory);
};
