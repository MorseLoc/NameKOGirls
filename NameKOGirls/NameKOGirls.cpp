#define NOMINMAX
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#include "CharacterSelectScreen.h"
#include "FightArena.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <string>

constexpr float DESIGN_WIDTH = 1280.0f;
constexpr float DESIGN_HEIGHT = 720.0f;

struct WindowState
{
    int x = 100;
    int y = 100;
    int width = 1280;
    int height = 720;
};

enum class Screen
{
    Title,
    CharacterSelect,
    FightPlaceholder,
    EditorPlaceholder,
    Fight
};

struct AppState
{
    Screen screen = Screen::Title;
    int selectedButton = 0;
    bool mouseWasDown = false;

    CharacterSelectScreen characterSelect;
    FightArena fightArena;
};

struct ProgramState
{
    WindowState window;
    AppState app;
};

struct Button
{
    float x;
    float y;
    float width;
    float height;
    const char* label;
};

const std::array<Button, 3> titleButtons =
{ {
    { 460.0f, 405.0f, 360.0f, 64.0f, "FIGHT" },
    { 460.0f, 485.0f, 360.0f, 64.0f, "EDITOR" },
    { 460.0f, 565.0f, 360.0f, 64.0f, "QUIT GAME" }
} };

static void ToggleFullscreen(GLFWwindow* window)
{
    auto* state = static_cast<ProgramState*>(
        glfwGetWindowUserPointer(window));

    if (glfwGetWindowMonitor(window))
    {
        glfwSetWindowMonitor(
            window, nullptr,
            state->window.x,
            state->window.y,
            state->window.width,
            state->window.height,
            GLFW_DONT_CARE);
    }
    else
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        if (!monitor)
            return;

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (!mode)
            return;

        glfwGetWindowPos(
            window,
            &state->window.x,
            &state->window.y);

        glfwGetWindowSize(
            window,
            &state->window.width,
            &state->window.height);

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate);
    }

    glfwSwapInterval(1);
}

static void ActivateButton(GLFWwindow* window, AppState& app, int buttonIndex)
{
    switch (buttonIndex)
    {
    case 0:
        app.characterSelect.Enter();
        app.screen = Screen::CharacterSelect;
        break;

    case 1:
        app.screen = Screen::EditorPlaceholder;
        break;

    case 2:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

static void OnKey(
    GLFWwindow* window,
    int key,
    int /*scancode*/,
    int action,
    int /*mods*/)
{
    if (action != GLFW_PRESS)
        return;

    auto* state = static_cast<ProgramState*>(
        glfwGetWindowUserPointer(window));

    AppState& app = state->app;

    if (key == GLFW_KEY_F)
    {
        ToggleFullscreen(window);
        return;
    }

    if (key == GLFW_KEY_ESCAPE)
    {
        if (app.screen == Screen::Fight)
        {
            app.fightArena.TogglePause();
        }
        else if (app.screen == Screen::Title)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        else
        {
            app.screen = Screen::Title;
        }

        return;
    }

    if (app.screen != Screen::Title)
        return;

    if (key == GLFW_KEY_UP || key == GLFW_KEY_W)
    {
        app.selectedButton = (app.selectedButton + 2) % 3;
    }
    else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S)
    {
        app.selectedButton = (app.selectedButton + 1) % 3;
    }
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE)
    {
        ActivateButton(window, app, app.selectedButton);
    }
}

static bool PointInsideButton(float x, float y, const Button& button)
{
    return x >= button.x &&
        x <= button.x + button.width &&
        y >= button.y &&
        y <= button.y + button.height;
}

static const std::array<unsigned char, 7>& GetGlyph(char character)
{
    static const std::array<unsigned char, 7> blank =
    { 0, 0, 0, 0, 0, 0, 0 };

    static const std::array<unsigned char, 7> a =
    { 14, 17, 17, 31, 17, 17, 17 };
    static const std::array<unsigned char, 7> c =
    { 14, 17, 16, 16, 16, 17, 14 };
    static const std::array<unsigned char, 7> d =
    { 30, 17, 17, 17, 17, 17, 30 };
    static const std::array<unsigned char, 7> e =
    { 31, 16, 16, 30, 16, 16, 31 };
    static const std::array<unsigned char, 7> f =
    { 31, 16, 16, 30, 16, 16, 16 };
    static const std::array<unsigned char, 7> g =
    { 14, 17, 16, 23, 17, 17, 14 };
    static const std::array<unsigned char, 7> h =
    { 17, 17, 17, 31, 17, 17, 17 };
    static const std::array<unsigned char, 7> i =
    { 31, 4, 4, 4, 4, 4, 31 };
    static const std::array<unsigned char, 7> l =
    { 16, 16, 16, 16, 16, 16, 31 };
    static const std::array<unsigned char, 7> m =
    { 17, 27, 21, 21, 17, 17, 17 };
    static const std::array<unsigned char, 7> n =
    { 17, 25, 21, 19, 17, 17, 17 };
    static const std::array<unsigned char, 7> o =
    { 14, 17, 17, 17, 17, 17, 14 };
    static const std::array<unsigned char, 7> q =
    { 14, 17, 17, 17, 21, 18, 13 };
    static const std::array<unsigned char, 7> r =
    { 30, 17, 17, 30, 20, 18, 17 };
    static const std::array<unsigned char, 7> s =
    { 15, 16, 16, 14, 1, 1, 30 };
    static const std::array<unsigned char, 7> t =
    { 31, 4, 4, 4, 4, 4, 4 };
    static const std::array<unsigned char, 7> u =
    { 17, 17, 17, 17, 17, 17, 14 };
    static const std::array<unsigned char, 7> w =
    { 17, 17, 17, 21, 21, 21, 10 };
    static const std::array<unsigned char, 7> x =
    { 17, 17, 10, 4, 10, 17, 17 };
    static const std::array<unsigned char, 7> y =
    { 17, 17, 10, 4, 4, 4, 4 };

    switch (character)
    {
    case 'A': return a;
    case 'C': return c;
    case 'D': return d;
    case 'E': return e;
    case 'F': return f;
    case 'G': return g;
    case 'H': return h;
    case 'I': return i;
    case 'L': return l;
    case 'M': return m;
    case 'N': return n;
    case 'O': return o;
    case 'Q': return q;
    case 'R': return r;
    case 'S': return s;
    case 'T': return t;
    case 'U': return u;
    case 'W': return w;
    case 'X': return x;
    case 'Y': return y;
    default: return blank;
    }
}

static float TextWidth(const char* text, float pixelSize)
{
    float width = 0.0f;

    for (const char* character = text; *character; ++character)
        width += (*character == ' ') ? pixelSize * 4.0f : pixelSize * 6.0f;

    return width - pixelSize;
}

static void DrawText(
    const char* text,
    float x,
    float y,
    float pixelSize,
    float red,
    float green,
    float blue)
{
    glColor3f(red, green, blue);

    float cursorX = x;

    for (const char* character = text; *character; ++character)
    {
        if (*character == ' ')
        {
            cursorX += pixelSize * 4.0f;
            continue;
        }

        const auto& glyph = GetGlyph(*character);

        for (int row = 0; row < 7; ++row)
        {
            for (int column = 0; column < 5; ++column)
            {
                if ((glyph[row] & (1 << (4 - column))) == 0)
                    continue;

                const float left = cursorX + column * pixelSize;
                const float top = y + row * pixelSize;

                glBegin(GL_QUADS);
                glVertex2f(left, top);
                glVertex2f(left + pixelSize, top);
                glVertex2f(left + pixelSize, top + pixelSize);
                glVertex2f(left, top + pixelSize);
                glEnd();
            }
        }

        cursorX += pixelSize * 6.0f;
    }
}

static void DrawButton(const Button& button, bool selected, bool hovered)
{
    const bool highlighted = selected || hovered;

    if (highlighted)
        glColor4f(0.40f, 0.15f, 0.10f, 0.97f);
    else
        glColor4f(0.10f, 0.04f, 0.03f, 0.88f);

    glBegin(GL_QUADS);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();

    if (highlighted)
        glColor3f(1.0f, 0.82f, 0.46f);
    else
        glColor3f(0.70f, 0.42f, 0.27f);

    glLineWidth(3.0f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(button.x, button.y);
    glVertex2f(button.x + button.width, button.y);
    glVertex2f(button.x + button.width, button.y + button.height);
    glVertex2f(button.x, button.y + button.height);
    glEnd();

    constexpr float textSize = 4.0f;
    const float labelWidth = TextWidth(button.label, textSize);

    DrawText(
        button.label,
        button.x + (button.width - labelWidth) * 0.5f,
        button.y + 18.0f,
        textSize,
        1.0f, 1.00f, 1.00f);
}

static void DrawDarkOverlay()
{
    glColor4f(0.02f, 0.01f, 0.03f, 0.72f);

    glBegin(GL_QUADS);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(DESIGN_WIDTH, 0.0f);
    glVertex2f(DESIGN_WIDTH, DESIGN_HEIGHT);
    glVertex2f(0.0f, DESIGN_HEIGHT);
    glEnd();
}

int main()
{
    glfwSetErrorCallback([](int code, const char* message)
        {
            std::cerr << "GLFW error " << code << ": "
                << message << '\n';
        });

    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "NameKOGirls", nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ProgramState state;
    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, OnKey);

    if (!state.app.characterSelect.Load(
        std::string(NAMEKO_ASSET_DIR) + "/CharacterSelect.png"))
    {
        std::cerr << "Could not load CharacterSelect.png\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const std::string imagePath =
        std::string(NAMEKO_ASSET_DIR) + "/TitleScreen.png";

    int imageWidth = 0;
    int imageHeight = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(false);

    unsigned char* pixels = stbi_load(
        imagePath.c_str(),
        &imageWidth,
        &imageHeight,
        &channels,
        STBI_rgb_alpha);

    if (!pixels)
    {
        std::cerr << "Could not load: " << imagePath << '\n';

        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    constexpr GLint clampToEdge = 0x812F;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        imageWidth,
        imageHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels);

    stbi_image_free(pixels);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        if (framebufferWidth <= 0 || framebufferHeight <= 0 ||
            windowWidth <= 0 || windowHeight <= 0)
        {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        const float scale = std::min(
            framebufferWidth / DESIGN_WIDTH,
            framebufferHeight / DESIGN_HEIGHT);

        const int viewWidth = static_cast<int>(DESIGN_WIDTH * scale);
        const int viewHeight = static_cast<int>(DESIGN_HEIGHT * scale);
        const int viewX = (framebufferWidth - viewWidth) / 2;
        const int viewY = (framebufferHeight - viewHeight) / 2;

        glViewport(
            viewX,
            framebufferHeight - viewY - viewHeight,
            viewWidth,
            viewHeight);

        glClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, DESIGN_WIDTH, DESIGN_HEIGHT, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, texture);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(DESIGN_WIDTH, 0.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(DESIGN_WIDTH, DESIGN_HEIGHT);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, DESIGN_HEIGHT);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        double cursorX = 0.0;
        double cursorY = 0.0;
        glfwGetCursorPos(window, &cursorX, &cursorY);

        const float framebufferMouseX =
            static_cast<float>(cursorX) *
            framebufferWidth / windowWidth;

        const float framebufferMouseY =
            static_cast<float>(cursorY) *
            framebufferHeight / windowHeight;

        const float designMouseX =
            (framebufferMouseX - viewX) / scale;

        const float designMouseY =
            (framebufferMouseY - viewY) / scale;

        const bool mouseDown =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) ==
            GLFW_PRESS;

        AppState& app = state.app;

        if (app.screen == Screen::CharacterSelect)
        {
            const bool startRequested = app.characterSelect.Update(
                designMouseX,
                designMouseY,
                mouseDown);

            app.characterSelect.Draw();

            if (startRequested)
            {
                const auto& player1 = app.characterSelect.GetPlayer(0);
                const auto& player2 = app.characterSelect.GetPlayer(1);

                std::cout
                    << "Player 1: "
                    << app.characterSelect.GetCharacterName(
                        player1.characterIndex)
                    << (player1.isNPC ? " (NPC)" : " (Human)")
                    << '\n'
                    << "Player 2: "
                    << app.characterSelect.GetCharacterName(
                        player2.characterIndex)
                    << (player2.isNPC ? " (NPC)" : " (Human)")
                    << '\n';

                if (app.fightArena.Enter(
                    std::string(NAMEKO_ASSET_DIR) + "/Stages",
                    window,
                    player1.isNPC,
                    player2.isNPC))
                {
                    app.screen = Screen::Fight;
                }
            }
        }

        else if (app.screen == Screen::Fight)
        {
            const FightArena::Action action = app.fightArena.Update(
                designMouseX,
                designMouseY,
                mouseDown);

            app.fightArena.Draw(DrawText);

            if (action == FightArena::Action::ExitToCharacterSelect)
            {
                app.screen = Screen::CharacterSelect;
            }
        }

        else if (app.screen == Screen::Title)
        {
            for (int index = 0; index < 3; ++index)
            {
                const bool hovered = PointInsideButton(
                    designMouseX,
                    designMouseY,
                    titleButtons[index]);

                if (hovered)
                    app.selectedButton = index;

                DrawButton(
                    titleButtons[index],
                    app.selectedButton == index,
                    hovered);

                if (hovered && mouseDown && !app.mouseWasDown)
                    ActivateButton(window, app, index);
            }

            DrawText(
                "F TOGGLE FULLSCREEN",
                497.0f,
                674.0f,
                3.0f,
                0.95f, 0.86f, 0.68f);
        }
        else
        {
            DrawDarkOverlay();

            const char* heading =
                app.screen == Screen::FightPlaceholder
                ? "FIGHT MODE"
                : "EDITOR MODE";

            constexpr float headingSize = 7.0f;
            DrawText(
                heading,
                (DESIGN_WIDTH - TextWidth(heading, headingSize)) * 0.5f,
                285.0f,
                headingSize,
                1.0f, 0.85f, 0.51f);

            constexpr float messageSize = 4.0f;
            DrawText(
                "COMING SOON",
                (DESIGN_WIDTH - TextWidth("COMING SOON", messageSize)) * 0.5f,
                370.0f,
                messageSize,
                1.0f, 0.93f, 0.80f);

            constexpr float backSize = 3.0f;
            DrawText(
                "ESC TO RETURN",
                (DESIGN_WIDTH - TextWidth("ESC TO RETURN", backSize)) * 0.5f,
                630.0f,
                backSize,
                0.95f, 0.86f, 0.68f);
        }

        app.mouseWasDown = mouseDown;
        glfwSwapBuffers(window);
    }

    state.app.fightArena.Release();
    state.app.characterSelect.Release();

    glDeleteTextures(1, &texture);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}