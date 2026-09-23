add_executable(NameKOGirls
    "NameKOGirls.cpp"
    "NameKOGirls.h"
    "CharacterSelectScreen.h"
    "FightArena.h"
)

target_link_libraries(NameKOGirls PRIVATE
    glfw
    OpenGL::GL
    nameko_image
)

# Lets the game find images regardless of the launch directory.
target_compile_definitions(NameKOGirls PRIVATE
    NAMEKO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(MSVC)
    target_compile_options(NameKOGirls PRIVATE /W4 /utf-8)
endif()