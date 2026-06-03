

add_subdirectory(Application)
add_subdirectory(Engine)
add_subdirectory(Infrastructure)


set(UNSORTED_SOURCES
	_unsorted_data.cpp
	Coroutine.cpp
    Cursor.cpp
    kknd.cpp
    Random.cpp
    Render.cpp
    RenderDrawHandlers.cpp
    Script.cpp
    ScriptEvent.cpp
    Sprite.cpp
	stru29.cpp
	stru31.cpp
    Map.cpp
    Pathfind.cpp
    Development.cpp
    Level.cpp
    SaveLoad.cpp
	Sidebar.cpp
)

set(UNSORTED_HEADERS
	_unsorted_data.h
    _unsorted_functions.h
    Coroutine.h
    Cursor.h
    hexrays-defs.h
    kknd.h
    Random.h
    Render.h
    RenderDrawHandlers.h
	Script.h
	ScriptEvent.h
    Sprite.h
	stru29.h
	stru31.h
    Map.h
    Pathfind.h
	Sidebar.h
)

add_library(Unsorted STATIC ${UNSORTED_SOURCES} ${UNSORTED_HEADERS})

add_library(Audio ${CMAKE_CURRENT_LIST_DIR}/Sound.h ${CMAKE_CURRENT_LIST_DIR}/Sound.cpp)
add_library(Video ${CMAKE_CURRENT_LIST_DIR}/Video.h ${CMAKE_CURRENT_LIST_DIR}/Video.cpp)

if(WIN32)
  target_link_libraries(Audio winmm.lib dsound.lib)
endif()

target_link_libraries(Audio ${SDL2_LIBRARY})
target_link_libraries(Video ${SDL2_LIBRARY})

target_link_libraries(Audio Engine Infrastructure)
target_link_libraries(Video Unsorted Engine Infrastructure)
target_link_libraries(Unsorted Engine Infrastructure Audio Video)
