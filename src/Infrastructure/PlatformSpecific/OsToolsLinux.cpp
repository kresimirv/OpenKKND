#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <thread>

#include <sys/types.h>

#include <SDL2/SDL.h>

#include "src/Infrastructure/PlatformSpecific/OsTools.h"


std::string OsGetUserName() {
    struct passwd *pd = getpwuid(getuid());
    if (pd == nullptr) {
        return std::string(constUnknownUser);
    } else {
        return std::string(pd->pw_name);
    }
}

std::string OsGetCurrentDirectory() {
    char buffer[4096];

    char *answer = getcwd(buffer, sizeof(buffer));
    if (!answer) {
        answer = "";
    }

    return std::string(answer);
}

std::string OsGetExecutableDirectory() {
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        return OsGetCurrentDirectory();
    }
    buffer[len] = '\0';
    char *slash = strrchr(buffer, '/');
    if (slash) {
        *slash = '\0';
    }
    return std::string(buffer);
}

std::string OsGetCommandLine() {
    return std::string("");
}

void OsDebugString(const char *str) {
    printf("%s\n", str);
}


void OsInitConsole() {
}

void OsInitTimer() {
}

unsigned long OsGetPrecisionTime() {
    return SDL_GetTicks();
}

unsigned int OsGetTime() {
    return (unsigned int)SDL_GetTicks();
}

void OsShowCursor(bool show) {
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

void OsThreadSleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void OsGetApplicationConfig(
    char *installation_drive_letter,
    bool *is_minimal_install,
    char *game_data_installation_dir
) {
    *installation_drive_letter = '.';
    *is_minimal_install = 1;
    std::string exe_dir = OsGetExecutableDirectory();
    strncpy(game_data_installation_dir, exe_dir.c_str(), 255);
    game_data_installation_dir[255] = '\0';
}
