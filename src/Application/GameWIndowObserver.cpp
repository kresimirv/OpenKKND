#include "src/Application/GameWindowObserver.h"
#include "src/_unsorted_data.h"

using Application::GameWindowObserver;

void GameWindowObserver::OnClose() {
    _47DCF4_wm_quit_received = true;

    if (auto p = game.lock()) {
        p->Exit();
    }
}
