cmake_minimum_required(VERSION 3.21)
project(Mercury VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")

set(BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(EXT_DIR ${PROJECT_SOURCE_DIR}/ext)

set(QUILL_NO_EXCEPTIONS ON)
set(SKIP_PORTABILITY_TESTS ON)
set(SKIP_PERFORMANCE_COMPARISON ON)
set(JUST_INSTALL_CEREAL ON)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
#set(_FBX_ROOT_DIR "/home/krzysztof/fbx-sdk")

# bullet options
set(BUILD_CPU_DEMOS OFF)
set(BUILD_OPENGL_DEMOS OFF)
set(BUILD_BULLET2_DEMOS OFF)
set(BUILD_EXTRAS OFF)
set(BUILD_UNIT_TESTS OFF)
set(BUILD_ENET OFF)
set(BUILD_CLSOCKET OFF)
add_subdirectory(${EXT_DIR}/bullet)

set(ozz_build_samples OFF)
set(ozz_build_howtos OFF)
set(ozz_build_tests OFF)
add_subdirectory(${EXT_DIR}/ozz-animation)

add_subdirectory(${EXT_DIR}/json)
add_subdirectory(${EXT_DIR}/entt)
add_subdirectory(${EXT_DIR}/cereal)
add_subdirectory(${EXT_DIR}/quill)
add_subdirectory(${EXT_DIR}/sol2)
add_subdirectory(${EXT_DIR}/doctest)
add_subdirectory(${EXT_DIR}/glfw)

set(FILAMENT_OUT_DIR ${EXT_DIR}/filament/out)
set(FILAMENT_LIB_DIR ${FILAMENT_OUT_DIR}/lib/x86_64)
set(FILAMENT_LIBS
    ${FILAMENT_OUT_DIR}/filament/libfilament.a
    ${FILAMENT_OUT_DIR}/filament/backend/libbackend.a
    ${FILAMENT_OUT_DIR}/filament/backend/libvkshaders.a
    ${FILAMENT_OUT_DIR}/libs/bluevk/libbluevk.a
    ${FILAMENT_OUT_DIR}/libs/filabridge/libfilabridge.a
    ${FILAMENT_OUT_DIR}/libs/filaflat/libfilaflat.a
    ${FILAMENT_OUT_DIR}/libs/utils/libutils.a
    ${FILAMENT_OUT_DIR}/libs/geometry/libgeometry.a
    ${FILAMENT_OUT_DIR}/third_party/smol-v/tnt/libsmol-v.a
    ${FILAMENT_OUT_DIR}/libs/ibl/libibl.a
    ${FILAMENT_OUT_DIR}/libs/filameshio/libfilameshio.a
    ${FILAMENT_OUT_DIR}/third_party/meshoptimizer/libmeshoptimizer.a
    ${FILAMENT_OUT_DIR}/libs/filagui/libfilagui.a)
add_custom_command(
    OUTPUT ${FILAMENT_LIBS}
    WORKING_DIRECTORY ${EXT_DIR}/filament
    COMMAND CC=clang CXX=clang++ cmake -B out -DFILAMENT_SKIP_SAMPLES=ON -DFILAMENT_ENABLE_MATDBG=OFF -DFILAMENT_SUPPORTS_OPENGL=OFF && cmake --build out
    VERBATIM
)
add_custom_target(filament ALL DEPENDS ${FILAMENT_LIBS})

set(ASSIMP_LIBS
    ${EXT_DIR}/assimp/out/code/libassimp.a
    ${EXT_DIR}/assimp/out/contrib/irrXML/libIrrXML.a
    ${EXT_DIR}/assimp/out/contrib/zlib/libzlibstatic.a)
add_custom_command(
    OUTPUT ${ASSIMP_LIBS}
    WORKING_DIRECTORY ${EXT_DIR}/assimp
    COMMAND CC=clang CXX=clang++ cmake . -B out -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_TESTS=OFF -DASSIMP_INSTALL=OFF -DASSIMP_BUILD_ZLIB=ON && cmake --build out
    VERBATIM
)
add_custom_target(assimp ALL DEPENDS ${ASSIMP_LIBS})

set(LUAJIT_LIB ${EXT_DIR}/LuaJIT/lib/libluajit-5.1.a)
add_custom_command(
    OUTPUT ${LUAJIT_LIB}
    WORKING_DIRECTORY ${EXT_DIR}/LuaJIT
    COMMAND make install PREFIX=${EXT_DIR}/LuaJIT -j$(nproc)
)
add_custom_target(LuaJIT ALL DEPENDS ${LUAJIT_LIB})

file(GLOB SOURCE_FILES "${SRC_DIR}/*.cpp")
file(GLOB IMGUI_SOURCE_FILES "${EXT_DIR}/imgui/*.cpp" "${EXT_DIR}/imgui/backends/imgui_impl_glfw.cpp")
file(GLOB IMNODES_SOURCE_FILES "${EXT_DIR}/imnodes/*.cpp")
file(GLOB IMGUI_COLOR_TEXT_EDIT_SOURCE_FILES "${EXT_DIR}/ImGuiColorTextEdit/*.cpp")

#foreach(TARGET Mercury Mercury-tests)
foreach(TARGET Mercury)
    add_executable(${TARGET} ${SOURCE_FILES} ${IMGUI_SOURCE_FILES} ${IMNODES_SOURCE_FILES} ${IMGUI_COLOR_TEXT_EDIT_SOURCE_FILES})
    set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 20)
    set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD_REQURIED ON)
    set_property(TARGET ${TARGET} PROPERTY CXX_EXTENSIONS OFF)
    set_property(TARGET ${TARGET} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
    add_dependencies(${TARGET} filament)
    add_dependencies(${TARGET} assimp)
    add_dependencies(${TARGET} LuaJIT)
    target_link_libraries(${TARGET} PRIVATE ${FILAMENT_LIBS})
    target_link_libraries(${TARGET} PRIVATE ${CMAKE_DL_LIBS})
    target_link_libraries(${TARGET} PRIVATE EnTT::EnTT)
    target_link_libraries(${TARGET} PRIVATE cereal)
    target_link_libraries(${TARGET} PRIVATE quill)
    target_link_libraries(${TARGET} PRIVATE sol2)
    target_link_libraries(${TARGET} PRIVATE glfw)
    target_link_libraries(${TARGET} PRIVATE BulletDynamics BulletCollision LinearMath)
    target_link_libraries(${TARGET} PRIVATE ozz_base ozz_geometry ozz_animation)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/assimp/include/)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/LuaJIT/include/luajit-2.1 ${EXT_DIR}/imgui ${EXT_DIR}/imnodes ${EXT_DIR}/ImGuiColorTextEdit)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/bullet/src) # otherwise LinearMath is not found
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/bullet/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/ozz-animation/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/libs/filagui/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/filament/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/filament/backend/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/libs/math/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/libs/utils/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/libs/filameshio/include)
    target_include_directories(${TARGET} PUBLIC ${EXT_DIR}/filament/libs/filabridge/include)
    target_link_libraries(${TARGET} PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(${TARGET} PRIVATE ${ASSIMP_LIBS})
    target_link_libraries(${TARGET} PRIVATE ${LUAJIT_LIB})
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra  -Werror -Wno-deprecated-volatile -Wno-nested-anon-types -Wno-gnu-anonymous-struct -Wno-unused-parameter -Wno-sign-compare -Wno-reorder-ctor -Wno-unused-variable -Wno-deprecated-copy -Wno-deprecated-declarations -Wno-unused-but-set-variable)
    # no -pedantic cause not working with filament
        # -Wno-sign-compare, -Wno-reorder-ctor, -Wno-unused-variable – TextEditor
        # -Wno-deprecated-copy – ozz
endforeach()

target_compile_definitions(Mercury PRIVATE DOCTEST_CONFIG_DISABLE)
target_link_libraries(Mercury PRIVATE doctest)

#target_link_libraries(Mercury-tests PRIVATE doctest_with_main)
