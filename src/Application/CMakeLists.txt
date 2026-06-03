
add_subdirectory(Scripts)



add_library(Application "")

target_sources(Application
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/Game.h"
        "${CMAKE_CURRENT_LIST_DIR}/GameConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/GameConfigFactory.h"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/GameWindowObserver.h"
        "${CMAKE_CURRENT_LIST_DIR}/GameWindowObserver.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/Game.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/GameFactory.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/GameConfig.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/GameConfigFactory.cpp"
)



target_link_libraries(
    Application
    ApplicationScripts
)