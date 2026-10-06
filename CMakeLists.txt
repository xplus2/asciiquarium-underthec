cmake_minimum_required(VERSION 3.16)
project(underthec C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (NOT EMSCRIPTEN AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT UNDERTHEC_IPO_OK LANGUAGES C)
    if (UNDERTHEC_IPO_OK)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
endif ()

file(READ ${CMAKE_SOURCE_DIR}/src/version.h UNDERTHEC_VERSION_H)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/version.h)
string(REGEX MATCH "TOOL_VERSION \"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"" _ "${UNDERTHEC_VERSION_H}")
set(UNDERTHEC_VERSION_STRING "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
foreach (part MAJOR MINOR PATCH)
    string(REGEX MATCH "TOOL_VERSION_${part} ([0-9]+)" _ "${UNDERTHEC_VERSION_H}")
    list(APPEND UNDERTHEC_VERSION_PARTS "${CMAKE_MATCH_1}")
endforeach ()
string(REPLACE ";" "." UNDERTHEC_VERSION_PARTS "${UNDERTHEC_VERSION_PARTS}")
if (NOT UNDERTHEC_VERSION_STRING STREQUAL UNDERTHEC_VERSION_PARTS)
    message(FATAL_ERROR "src/version.h: TOOL_VERSION ${UNDERTHEC_VERSION_STRING} != MAJOR.MINOR.PATCH ${UNDERTHEC_VERSION_PARTS}")
endif ()

option(UNDERTHEC_STATIC "link statically where the target platform allows it" ON)

set(UNDERTHEC_XSCREENSAVER_BINDIR "libexec/xscreensaver" CACHE PATH "install dir for the xscreensaver binary")
set(UNDERTHEC_XSCREENSAVER_CONFIGDIR "libexec/xscreensaver" CACHE PATH "install dir for the xscreensaver .xml config-spec")

set(UNDERTHEC_CORE_SOURCES
        src/app.c
        src/opts.c
        src/canvas.c
        src/color.c
        src/rng.c
        src/entity/utf8.c
        src/entity/list.c
        src/entity/shape.c
        src/entity/tick.c
        src/entity/seaweed.c
        src/entity/collide.c
        src/entity/draw.c
        src/ui/help.c
        src/ui/settings.c
        src/scene/scene.c
        src/scene/effects.c
        src/scene/creatures.c
        src/scene/environment.c
        src/scene/kaiju.c
        src/scene/fishhook.c
        src/scene/feed.c
        src/scene/turtle.c
)
set(UNDERTHEC_SOURCES ${UNDERTHEC_CORE_SOURCES})

if (ANDROID)
    add_library(underthec_android SHARED
            ${UNDERTHEC_CORE_SOURCES}
            src/config.c
            src/target/androidtv/android_main.c
    )
    target_include_directories(underthec_android PRIVATE src)
    target_link_libraries(underthec_android PRIVATE log)
    target_compile_options(underthec_android PRIVATE -Wall -Wextra)
    return()
endif ()

if (EMSCRIPTEN)
    list(APPEND UNDERTHEC_SOURCES src/config.c src/target/web/web_main.c)
else ()
    list(APPEND UNDERTHEC_SOURCES
            src/main.c
            src/ui/args.c
            src/ui/cli_help.c
            src/target/terminal/term_common.c
            src/target/teletext/tt_render.c
            src/target/teletext/tt_t42.c
            src/target/teletext/tt_ts.c
            src/target/teletext/tt_net.c
    )
    if (WIN32)
        enable_language(RC)
        list(APPEND UNDERTHEC_SOURCES src/target/terminal/term_windows.c src/target/windows/underthec.rc)
        set_source_files_properties(src/target/windows/underthec.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/src/target/windows/underthec.ico)
    else ()
        list(APPEND UNDERTHEC_SOURCES src/target/terminal/term_posix.c)
    endif ()
endif ()

add_executable(underthec ${UNDERTHEC_SOURCES})
target_include_directories(underthec PRIVATE src)

if (WIN32 AND NOT EMSCRIPTEN)
    target_link_libraries(underthec PRIVATE shell32)
endif ()

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(underthec PRIVATE -Wall -Wextra)
endif ()

if (EMSCRIPTEN)
    if (UNDERTHEC_STATIC)
        message(STATUS "UNDERTHEC_STATIC has no effect on emscripten")
    endif ()
    set_target_properties(underthec PROPERTIES SUFFIX ".js")
    target_link_options(underthec PRIVATE
            -sMODULARIZE=1
            -sEXPORT_NAME=underthec
            -sENVIRONMENT=web
            -sALLOW_MEMORY_GROWTH=1
            -sEXPORTED_FUNCTIONS=_web_opt,_web_init,_web_fps,_web_resize,_web_key,_web_click,_web_frame,_web_width,_web_height,_web_settings_open,_web_help_open
            -sEXPORTED_RUNTIME_METHODS=HEAPU32,ccall
    )
    add_custom_command(TARGET underthec POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/target/web/index.html $<TARGET_FILE_DIR:underthec>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/target/web/apple-touch-icon.png $<TARGET_FILE_DIR:underthec>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/target/windows/underthec.ico $<TARGET_FILE_DIR:underthec>/favicon.ico
    )
else ()
    if (UNDERTHEC_STATIC AND NOT APPLE)
        target_link_options(underthec PRIVATE -static)
    endif ()
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} -s")
endif ()

if (WIN32 AND NOT EMSCRIPTEN)
    enable_language(RC)
    add_executable(underthec_scr WIN32
            ${UNDERTHEC_CORE_SOURCES}
            src/config.c
            src/target/scr_windows/scr_main.c
            src/target/scr_windows/scr_cfg.c
            src/target/scr_windows/scr.rc
    )
    set_source_files_properties(src/target/scr_windows/scr.rc PROPERTIES OBJECT_DEPENDS "${CMAKE_SOURCE_DIR}/src/target/scr_windows/scr.manifest;${CMAKE_SOURCE_DIR}/src/target/windows/scr.ico")
    target_include_directories(underthec_scr PRIVATE src)
    set_target_properties(underthec_scr PROPERTIES OUTPUT_NAME underthec SUFFIX ".scr")
    target_link_libraries(underthec_scr PRIVATE gdi32 advapi32)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(underthec_scr PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra>)
    endif ()
    if (UNDERTHEC_STATIC)
        target_link_options(underthec_scr PRIVATE -static)
    endif ()
    install(TARGETS underthec_scr RUNTIME DESTINATION bin)
endif ()

if (NOT WIN32 AND NOT EMSCRIPTEN)
    find_package(X11)
    if (X11_FOUND)
        add_executable(underthec_xscr
                ${UNDERTHEC_CORE_SOURCES}
                src/config.c
                src/target/scr_x11/xscr_main.c
                src/target/scr_x11/xscr_font.c
        )
        target_include_directories(underthec_xscr PRIVATE src ${X11_INCLUDE_DIR})
        target_link_libraries(underthec_xscr PRIVATE ${X11_LIBRARIES})
        if (X11_Xft_FOUND)
            find_package(Freetype)
            if (FREETYPE_FOUND)
                target_compile_definitions(underthec_xscr PRIVATE HAVE_XFT)
                target_include_directories(underthec_xscr PRIVATE ${X11_Xft_INCLUDE_PATH} ${FREETYPE_INCLUDE_DIRS})
                target_link_libraries(underthec_xscr PRIVATE ${X11_Xft_LIB} ${FREETYPE_LIBRARIES})
            else ()
                message(STATUS "Xft found but freetype2 dev headers are missing, building without xft")
            endif ()
        endif ()
        if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(underthec_xscr PRIVATE -Wall -Wextra)
        endif ()
        if (UNDERTHEC_STATIC)
            message(STATUS "UNDERTHEC_STATIC has no effect on the xscreensaver build (X11/Xft are not static)")
        endif ()
        install(PROGRAMS $<TARGET_FILE:underthec_xscr> DESTINATION ${UNDERTHEC_XSCREENSAVER_BINDIR} RENAME underthec)
        install(FILES src/target/scr_x11/underthec.xml DESTINATION ${UNDERTHEC_XSCREENSAVER_CONFIGDIR})
    else ()
        message(STATUS "X11 dev headers not found, skipping xscreensaver")
    endif ()
endif ()

if (NOT WIN32 AND NOT APPLE AND NOT EMSCRIPTEN)
    set(UNDERTHEC_VLOCK_MODULEDIR "/usr/lib/vlock/modules" CACHE PATH "install dir for the vlock plugin module")
    add_library(underthec_vlock MODULE
            ${UNDERTHEC_CORE_SOURCES}
            src/config.c
            src/target/terminal/term_common.c
            src/target/terminal/term_posix.c
            src/target/vlock/vlock_module.c
    )
    target_include_directories(underthec_vlock PRIVATE src)
    set_target_properties(underthec_vlock PROPERTIES
            OUTPUT_NAME underthec
            PREFIX ""
            POSITION_INDEPENDENT_CODE ON
    )
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(underthec_vlock PRIVATE -Wall -Wextra)
    endif ()
    install(TARGETS underthec_vlock LIBRARY DESTINATION ${UNDERTHEC_VLOCK_MODULEDIR})
endif ()

if (NOT WIN32 AND NOT APPLE AND NOT EMSCRIPTEN)
    option(UNDERTHEC_PLASMA_WALLPAPER "build the Plasma wallpaper plugin if Qt6 Quick is found" ON)
    set(UNDERTHEC_PLASMA_PACKAGEDIR "share/plasma/wallpapers/org.underthec.wallpaper" CACHE PATH "install dir for the Plasma wallpaper package")
    if (UNDERTHEC_PLASMA_WALLPAPER)
        include(CheckLanguage)
        check_language(CXX)
        if (CMAKE_CXX_COMPILER)
            enable_language(CXX)
            find_package(Qt6 6.4 QUIET COMPONENTS Quick)
        endif ()
        if (Qt6_FOUND)
            set(UNDERTHEC_QML_MODULEDIR "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}" CACHE PATH "install dir for the QML modules (Qt import path)")
            set(CMAKE_CXX_STANDARD 17)
            set(CMAKE_CXX_STANDARD_REQUIRED ON)
            add_library(plasma_wallpaper MODULE
                    ${UNDERTHEC_CORE_SOURCES}
                    src/config.c
                    src/target/plasma/plasma_item.cpp
                    src/target/plasma/plasma_plugin.cpp
            )
            target_include_directories(plasma_wallpaper PRIVATE src src/target/plasma)
            target_link_libraries(plasma_wallpaper PRIVATE Qt6::Quick)
            set_target_properties(plasma_wallpaper PROPERTIES
                    OUTPUT_NAME underthec_qml
                    AUTOMOC ON
                    POSITION_INDEPENDENT_CODE ON
                    INTERPROCEDURAL_OPTIMIZATION OFF
                    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plasma_wallpaper/qml/org/underthec
            )
            if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
                target_compile_options(plasma_wallpaper PRIVATE -Wall -Wextra)
            endif ()
            add_custom_command(TARGET plasma_wallpaper POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/src/target/plasma/qmldir ${CMAKE_BINARY_DIR}/plasma_wallpaper/qml/org/underthec/qmldir
                    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/target/plasma/package ${CMAKE_BINARY_DIR}/plasma_wallpaper/package
            )
            install(TARGETS plasma_wallpaper LIBRARY DESTINATION ${UNDERTHEC_QML_MODULEDIR}/org/underthec)
            install(FILES src/target/plasma/qmldir DESTINATION ${UNDERTHEC_QML_MODULEDIR}/org/underthec)
            install(DIRECTORY src/target/plasma/package/ DESTINATION ${UNDERTHEC_PLASMA_PACKAGEDIR})
        else ()
            message(STATUS "Qt6 Quick 6.4+ dev files or C++ compiler not found, skipping plasma wallpaper")
        endif ()
    endif ()
endif ()

if (APPLE AND NOT EMSCRIPTEN)
    option(UNDERTHEC_MACOS_SAVER "build the macOS screensaver bundle" ON)
    set(UNDERTHEC_MACOS_SAVERDIR "/Library/Screen Savers" CACHE PATH "install dir for the macOS screensaver bundle")
    if (UNDERTHEC_MACOS_SAVER)
        enable_language(OBJC)
        configure_file(src/target/scr_macos/Info.plist.in ${CMAKE_BINARY_DIR}/macos_saver/Info.plist @ONLY)
        add_library(macos_saver MODULE
                ${UNDERTHEC_CORE_SOURCES}
                src/config.c
                src/target/scr_macos/saver_view.m
                src/target/scr_macos/saver_cfg.m
        )
        target_include_directories(macos_saver PRIVATE src)
        target_link_libraries(macos_saver PRIVATE "-framework Cocoa" "-framework ScreenSaver")
        target_compile_options(macos_saver PRIVATE -Wall -Wextra $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>)
        set_target_properties(macos_saver PROPERTIES
                BUNDLE TRUE
                BUNDLE_EXTENSION saver
                OUTPUT_NAME underthec
                MACOSX_BUNDLE_INFO_PLIST ${CMAKE_BINARY_DIR}/macos_saver/Info.plist
                INTERPROCEDURAL_OPTIMIZATION OFF
        )
        install(TARGETS macos_saver LIBRARY DESTINATION "${UNDERTHEC_MACOS_SAVERDIR}")
    endif ()
endif ()

if (NOT EMSCRIPTEN)
    install(TARGETS underthec RUNTIME DESTINATION bin)
endif ()
