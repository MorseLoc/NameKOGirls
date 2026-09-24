#pragma once

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <cmath>
#include <iostream>
#include <string>

class Character
{
public:
    Character() = default;

    Character(const Character&) = delete;
    Character& operator=(const Character&) = delete;

    bool LoadIdle(const std::string& path)
    {
        if (texture != 0)
            return true;

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
            std::cerr << "Could not load character animation: "
                << path << '\n';
            return false;
        }

        if (height != FRAME_HEIGHT ||
            width < FRAME_WIDTH ||
            width % FRAME_WIDTH != 0)
        {
            std::cerr
                << "Idle.png must contain one horizontal row of "
                << FRAME_WIDTH << " x " << FRAME_HEIGHT
                << " frames, without spacing.\n";

            stbi_image_free(pixels);
            return false;
        }

        GLint maximumSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumSize);

        if (width > maximumSize || height > maximumSize)
        {
            std::cerr << "Character sprite sheet is too large.\n";
            stbi_image_free(pixels);
            return false;
        }

        // Clear previous errors before checking this texture upload.
        while (glGetError() != GL_NO_ERROR)
        {
        }

        GLuint newTexture = 0;
        glGenTextures(1, &newTexture);
        glBindTexture(GL_TEXTURE_2D, newTexture);

        // Preserve sharp pixel edges.
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(
            GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

        if (glGetError() != GL_NO_ERROR || newTexture == 0)
        {
            if (newTexture != 0)
                glDeleteTextures(1, &newTexture);

            std::cerr << "Could not upload character texture.\n";
            return false;
        }

        texture = newTexture;
        frameCount = width / FRAME_WIDTH;
        currentFrame = 0;
        animationTime = 0.0;

        return true;
    }

    void Spawn(float spawnX, float groundY)
    {
        x = spawnX;
        floorY = groundY;

        currentFrame = 0;
        animationTime = 0.0;
    }

    void FaceOpponent(const Character& opponent)
    {
        // Keep the previous direction if both occupy the same X.
        if (opponent.x > x)
            facingRight = true;
        else if (opponent.x < x)
            facingRight = false;
    }

    void Update(double deltaTime)
    {
        if (texture == 0 || frameCount <= 0 || deltaTime <= 0.0)
            return;

        const double cycleDuration =
            static_cast<double>(frameCount) / IDLE_FPS;

        animationTime = std::fmod(
            animationTime + deltaTime,
            cycleDuration);

        currentFrame = static_cast<int>(animationTime * IDLE_FPS);

        if (currentFrame >= frameCount)
            currentFrame = 0;
    }

    void Draw() const
    {
        if (texture == 0)
            return;

        const float width = FRAME_WIDTH * DRAW_SCALE;
        const float height = FRAME_HEIGHT * DRAW_SCALE;

        const float left = x - FOOT_ANCHOR_X * DRAW_SCALE;
        const float top = floorY - FOOT_ANCHOR_Y * DRAW_SCALE;

        float uLeft =
            static_cast<float>(currentFrame) / frameCount;

        float uRight =
            static_cast<float>(currentFrame + 1) / frameCount;

        // Reverse texture coordinates to mirror the current frame.
        const bool shouldFlip =
            facingRight != ART_FACES_RIGHT;

        if (shouldFlip)
        {
            const float temporary = uLeft;
            uLeft = uRight;
            uRight = temporary;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture);
        glColor4f(1, 1, 1, 1);

        glBegin(GL_QUADS);

        glTexCoord2f(uLeft, 0);
        glVertex2f(left, top);

        glTexCoord2f(uRight, 0);
        glVertex2f(left + width, top);

        glTexCoord2f(uRight, 1);
        glVertex2f(left + width, top + height);

        glTexCoord2f(uLeft, 1);
        glVertex2f(left, top + height);

        glEnd();

        glDisable(GL_TEXTURE_2D);
    }

    // Call before the OpenGL window is destroyed.
    void Release()
    {
        if (texture != 0)
        {
            glDeleteTextures(1, &texture);
            texture = 0;
        }

        frameCount = 0;
    }

private:
    static constexpr int FRAME_WIDTH = 160;
    static constexpr int FRAME_HEIGHT = 192;

    static constexpr double IDLE_FPS = 12.0;
    static constexpr float DRAW_SCALE = 1.5f;

    // Your original Funghi sprite faces right.
    static constexpr bool ART_FACES_RIGHT = true;

    // Position of the ground anchor inside each unscaled frame.
    // Your original sprite's boots end around Y = 172.
    static constexpr float FOOT_ANCHOR_X = 80.0f;
    static constexpr float FOOT_ANCHOR_Y = 172.0f;

    GLuint texture = 0;

    int frameCount = 0;
    int currentFrame = 0;
    double animationTime = 0.0;

    float x = 0.0f;
    float floorY = 0.0f;
    bool facingRight = true;
};