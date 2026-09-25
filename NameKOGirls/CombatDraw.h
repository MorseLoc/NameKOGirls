#pragma once

#include "CombatRules.h"

#include <GLFW/glfw3.h>

#include <array>
#include <cctype>
#include <string>

namespace CombatDraw
{
    inline void Fill(float x, float y, float width, float height)
    {
        glBegin(GL_QUADS);

        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);

        glEnd();
    }

    inline void Outline(
        const Combat::Box& box,
        float thickness = 2.0f)
    {
        Fill(
            box.x,
            box.y,
            box.w,
            thickness);

        Fill(
            box.x,
            box.y + box.h - thickness,
            box.w,
            thickness);

        Fill(
            box.x,
            box.y,
            thickness,
            box.h);

        Fill(
            box.x + box.w - thickness,
            box.y,
            thickness,
            box.h);
    }

    // Each character is a 5-column, 7-row bitmap.
    // Each number describes the enabled pixels in one row.
    inline std::array<unsigned char, 7> Glyph(char character)
    {
        switch (character)
        {
        case 'A':
            return { 14, 17, 17, 31, 17, 17, 17 };

        case 'B':
            return { 30, 17, 17, 30, 17, 17, 30 };

        case 'C':
            return { 14, 17, 16, 16, 16, 17, 14 };

        case 'D':
            return { 30, 17, 17, 17, 17, 17, 30 };

        case 'E':
            return { 31, 16, 16, 30, 16, 16, 31 };

        case 'F':
            return { 31, 16, 16, 30, 16, 16, 16 };

        case 'G':
            return { 14, 17, 16, 23, 17, 17, 14 };

        case 'H':
            return { 17, 17, 17, 31, 17, 17, 17 };

        case 'I':
            return { 31, 4, 4, 4, 4, 4, 31 };

        case 'J':
            return { 7, 2, 2, 2, 18, 18, 12 };

        case 'K':
            return { 17, 18, 20, 24, 20, 18, 17 };

        case 'L':
            return { 16, 16, 16, 16, 16, 16, 31 };

        case 'M':
            return { 17, 27, 21, 21, 17, 17, 17 };

        case 'N':
            return { 17, 25, 25, 21, 19, 19, 17 };

        case 'O':
            return { 14, 17, 17, 17, 17, 17, 14 };

        case 'P':
            return { 30, 17, 17, 30, 16, 16, 16 };

        case 'Q':
            return { 14, 17, 17, 17, 21, 18, 13 };

        case 'R':
            return { 30, 17, 17, 30, 20, 18, 17 };

        case 'S':
            return { 15, 16, 16, 14, 1, 1, 30 };

        case 'T':
            return { 31, 4, 4, 4, 4, 4, 4 };

        case 'U':
            return { 17, 17, 17, 17, 17, 17, 14 };

        case 'V':
            return { 17, 17, 17, 17, 17, 10, 4 };

        case 'W':
            return { 17, 17, 17, 21, 21, 21, 10 };

        case 'X':
            return { 17, 17, 10, 4, 10, 17, 17 };

        case 'Y':
            return { 17, 17, 10, 4, 4, 4, 4 };

        case 'Z':
            return { 31, 1, 2, 4, 8, 16, 31 };

        case '0':
            return { 14, 17, 19, 21, 25, 17, 14 };

        case '1':
            return { 4, 12, 4, 4, 4, 4, 14 };

        case '2':
            return { 14, 17, 1, 2, 4, 8, 31 };

        case '3':
            return { 30, 1, 1, 14, 1, 1, 30 };

        case '4':
            return { 2, 6, 10, 18, 31, 2, 2 };

        case '5':
            return { 31, 16, 16, 30, 1, 1, 30 };

        case '6':
            return { 14, 16, 16, 30, 17, 17, 14 };

        case '7':
            return { 31, 1, 2, 4, 8, 8, 8 };

        case '8':
            return { 14, 17, 17, 14, 17, 17, 14 };

        case '9':
            return { 14, 17, 17, 15, 1, 1, 14 };

        case '-':
            return { 0, 0, 0, 31, 0, 0, 0 };

        case ':':
            return { 0, 4, 4, 0, 4, 4, 0 };

        case '.':
            return { 0, 0, 0, 0, 0, 4, 4 };

        case '/':
            return { 1, 1, 2, 4, 8, 16, 16 };

        case '+':
            return { 0, 4, 4, 31, 4, 4, 0 };

        case '!':
            return { 4, 4, 4, 4, 4, 0, 4 };

        default:
            return {};
        }
    }

    // Uses the current OpenGL color.
    inline void Text(
        const std::string& text,
        float x,
        float y,
        float size = 2.0f)
    {
        glDisable(GL_TEXTURE_2D);

        for (unsigned char character : text)
        {
            const auto glyph = Glyph(
                static_cast<char>(std::toupper(character)));

            for (int row = 0; row < 7; ++row)
            {
                for (int column = 0; column < 5; ++column)
                {
                    const int mask = 1 << (4 - column);

                    if (glyph[row] & mask)
                    {
                        Fill(
                            x + column * size,
                            y + row * size,
                            size,
                            size);
                    }
                }
            }

            x += 6.0f * size;
        }
    }
}