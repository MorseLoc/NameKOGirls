#pragma once

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

class FightArena
{
public:
    enum class Action
    {
        None,
        ExitToCharacterSelect
    };

    // Uses the existing DrawText function in NameKOGirls.cpp.
    using TextRenderer = void (*)(
        const char*, float, float, float,
        float, float, float);

    FightArena() = default;

    FightArena(const FightArena&) = delete;
    FightArena& operator=(const FightArena&) = delete;

    bool Enter(const std::string& folderPath)
    {
        std::vector<std::filesystem::path> images;

        try
        {
            for (const auto& entry :
                std::filesystem::directory_iterator(folderPath))
            {
                if (!entry.is_regular_file())
                    continue;

                std::string extension = entry.path().extension().string();

                std::transform(
                    extension.begin(),
                    extension.end(),
                    extension.begin(),
                    [](unsigned char c)
                    {
                        return static_cast<char>(std::tolower(c));
                    });

                if (extension == ".png" ||
                    extension == ".jpg" ||
                    extension == ".jpeg" ||
                    extension == ".bmp" ||
                    extension == ".tga")
                {
                    images.push_back(entry.path());
                }
            }
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            std::cerr << "Could not read Stages folder: "
                << error.what() << '\n';
            return false;
        }

        if (images.empty())
        {
            std::cerr << "No supported images in: "
                << folderPath << '\n';
            return false;
        }

        // Shuffle so the first usable image is chosen randomly.
        // A damaged image will not stop other stages from loading.
        std::shuffle(images.begin(), images.end(), randomEngine);

        for (const auto& image : images)
        {
            if (LoadStage(image.string()))
            {
                paused = false;
                mouseWasDown = true;
                cursorX = -1.0f;
                cursorY = -1.0f;
                return true;
            }
        }

        std::cerr << "None of the stage images could be loaded.\n";
        return false;
    }

    void TogglePause()
    {
        paused = !paused;

        // Require a fresh click after opening or closing the menu.
        mouseWasDown = true;
    }

    bool IsPaused() const
    {
        return paused;
    }

    Action Update(float mouseX, float mouseY, bool mouseDown)
    {
        cursorX = mouseX;
        cursorY = mouseY;

        const bool clicked = mouseDown && !mouseWasDown;
        mouseWasDown = mouseDown;

        if (!paused || !clicked)
            return Action::None;

        if (continueButton.Contains(mouseX, mouseY))
        {
            paused = false;
            return Action::None;
        }

        if (exitButton.Contains(mouseX, mouseY))
        {
            paused = false;
            return Action::ExitToCharacterSelect;
        }

        return Action::None;
    }

    void Draw(TextRenderer drawText) const
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.02f, 0.02f, 0.03f, 1.0f);
        Fill({ 0, 0, WIDTH, HEIGHT });

        if (stageTexture != 0)
        {
            // Fit the image without stretching it.
            const float scale = std::min(
                WIDTH / static_cast<float>(imageWidth),
                HEIGHT / static_cast<float>(imageHeight));

            const float width = imageWidth * scale;
            const float height = imageHeight * scale;
            const float x = (WIDTH - width) * 0.5f;
            const float y = (HEIGHT - height) * 0.5f;

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, stageTexture);
            glColor4f(1, 1, 1, 1);

            glBegin(GL_QUADS);
            glTexCoord2f(0, 0);
            glVertex2f(x, y);

            glTexCoord2f(1, 0);
            glVertex2f(x + width, y);

            glTexCoord2f(1, 1);
            glVertex2f(x + width, y + height);

            glTexCoord2f(0, 1);
            glVertex2f(x, y + height);
            glEnd();

            glDisable(GL_TEXTURE_2D);
        }

        if (!paused)
            return;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.01f, 0.01f, 0.02f, 0.72f);
        Fill({ 0, 0, WIDTH, HEIGHT });

        DrawButton(continueButton, "CONTINUE", drawText);
        DrawButton(exitButton, "EXIT TO MENU", drawText);
    }

    // Call while the OpenGL window still exists.
    void Release()
    {
        if (stageTexture != 0)
        {
            glDeleteTextures(1, &stageTexture);
            stageTexture = 0;
        }
    }

private:
    struct Rect
    {
        float x, y, width, height;

        bool Contains(float px, float py) const
        {
            return px >= x && px < x + width &&
                py >= y && py < y + height;
        }
    };

    static constexpr float WIDTH = 1280.0f;
    static constexpr float HEIGHT = 720.0f;

    GLuint stageTexture = 0;
    int imageWidth = 0;
    int imageHeight = 0;

    bool paused = false;
    bool mouseWasDown = true;

    float cursorX = -1.0f;
    float cursorY = -1.0f;

    const Rect continueButton{ 440, 280, 400, 68 };
    const Rect exitButton{ 440, 372, 400, 68 };

    std::mt19937 randomEngine{ std::random_device{}() };

    bool LoadStage(const std::string& path)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(false);

        unsigned char* pixels = stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha);

        if (!pixels)
        {
            const char* reason = stbi_failure_reason();

            std::cerr << "Could not load stage: " << path
                << " (" << (reason ? reason : "unknown error")
                << ")\n";
            return false;
        }

        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

        if (width <= 0 || height <= 0 ||
            width > maxTextureSize || height > maxTextureSize)
        {
            std::cerr << "Unsupported stage dimensions: "
                << path << '\n';
            stbi_image_free(pixels);
            return false;
        }

        // Clear earlier GL errors before checking this upload.
        while (glGetError() != GL_NO_ERROR)
        {
        }

        GLuint newTexture = 0;
        glGenTextures(1, &newTexture);
        glBindTexture(GL_TEXTURE_2D, newTexture);

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        constexpr GLint clampToEdge = 0x812F;

        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clampToEdge);
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clampToEdge);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels);

        stbi_image_free(pixels);

        const GLenum error = glGetError();

        if (newTexture == 0 || error != GL_NO_ERROR)
        {
            if (newTexture != 0)
                glDeleteTextures(1, &newTexture);

            std::cerr << "Could not upload stage texture: "
                << path << '\n';
            return false;
        }

        Release();

        stageTexture = newTexture;
        imageWidth = width;
        imageHeight = height;

        std::cout << "Stage loaded: " << path << '\n';
        return true;
    }

    static void Fill(const Rect& rect)
    {
        glBegin(GL_QUADS);
        glVertex2f(rect.x, rect.y);
        glVertex2f(rect.x + rect.width, rect.y);
        glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
    }

    static void Outline(const Rect& rect, float thickness)
    {
        Fill({ rect.x, rect.y, rect.width, thickness });

        Fill({
            rect.x,
            rect.y + rect.height - thickness,
            rect.width,
            thickness
            });

        Fill({ rect.x, rect.y, thickness, rect.height });

        Fill({
            rect.x + rect.width - thickness,
            rect.y,
            thickness,
            rect.height
            });
    }

    static float LabelWidth(const char* text, float size)
    {
        float width = 0;

        for (const char* c = text; *c; ++c)
            width += (*c == ' ' ? 4.0f : 6.0f) * size;

        return width > 0 ? width - size : 0;
    }

    void DrawButton(
        const Rect& rect,
        const char* label,
        TextRenderer drawText) const
    {
        const bool hovered = rect.Contains(cursorX, cursorY);

        if (hovered)
            glColor4f(0.40f, 0.15f, 0.10f, 0.98f);
        else
            glColor4f(0.10f, 0.04f, 0.03f, 0.96f);

        Fill(rect);

        if (hovered)
            glColor3f(1.0f, 0.82f, 0.46f);
        else
            glColor3f(0.70f, 0.42f, 0.27f);

        Outline(rect, 3.0f);

        constexpr float textSize = 4.0f;

        drawText(
            label,
            rect.x + (rect.width - LabelWidth(label, textSize)) * 0.5f,
            rect.y + (rect.height - 7.0f * textSize) * 0.5f,
            textSize,
            1.0f, 1.0f, 1.0f);
    }
};