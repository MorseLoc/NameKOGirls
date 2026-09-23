#pragma once

#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <array>
#include <string>
#include <vector>

class CharacterSelectScreen
{
public:
    struct Rect
    {
        float x, y, width, height;

        bool Contains(float px, float py) const
        {
            return px >= x && px < x + width &&
                py >= y && py < y + height;
        }
    };

    struct Character
    {
        std::string name;
        Rect bounds;
    };

    struct PlayerSelection
    {
        int characterIndex = -1;
        bool isNPC = false;
    };

    CharacterSelectScreen() = default;

    CharacterSelectScreen(const CharacterSelectScreen&) = delete;
    CharacterSelectScreen& operator=(const CharacterSelectScreen&) = delete;

    bool Load(const std::string& imagePath)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(false);

        unsigned char* pixels = stbi_load(
            imagePath.c_str(),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha);

        if (!pixels)
            return false;

        Release();

        glGenTextures(1, &backgroundTexture);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);

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
        return true;
    }

    // Call before destroying the OpenGL window.
    void Release()
    {
        if (backgroundTexture != 0)
        {
            glDeleteTextures(1, &backgroundTexture);
            backgroundTexture = 0;
        }
    }

    void Enter()
    {
        players = {};
        nextPlayer = 0;

        // Prevent the click that opened this screen from selecting a girl.
        mouseWasDown = true;
    }

    const PlayerSelection& GetPlayer(int index) const
    {
        return players.at(index);
    }

    const std::string& GetCharacterName(int index) const
    {
        return characters.at(index).name;
    }

    // Coordinates use the same 1280 x 720 space as your main menu.
    // Add entries here when you expand the artwork.
    void AddCharacter(const std::string& name, const Rect& bounds)
    {
        characters.push_back({ name, bounds });
    }

    bool Ready() const
    {
        return players[0].characterIndex >= 0 &&
            players[1].characterIndex >= 0;
    }

    // Returns true only when Start Game is clicked with both players ready.
    bool Update(float mouseX, float mouseY, bool mouseDown)
    {
        cursorX = mouseX;
        cursorY = mouseY;

        const bool clicked = mouseDown && !mouseWasDown;
        mouseWasDown = mouseDown;

        if (!clicked)
            return false;

        for (int player = 0; player < 2; ++player)
        {
            if (playerPanels[player].Contains(mouseX, mouseY))
            {
                players[player].isNPC = !players[player].isNPC;
                return false;
            }
        }

        if (startButton.Contains(mouseX, mouseY))
            return Ready();

        for (int index = 0;
            index < static_cast<int>(characters.size());
            ++index)
        {
            if (characters[index].bounds.Contains(mouseX, mouseY))
            {
                players[nextPlayer].characterIndex = index;
                nextPlayer = 1 - nextPlayer;
                break;
            }
        }

        return false;
    }

    void Draw() const
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glColor4f(1, 1, 1, 1);

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(1280, 0);
        glTexCoord2f(1, 1); glVertex2f(1280, 720);
        glTexCoord2f(0, 1); glVertex2f(0, 720);
        glEnd();

        glDisable(GL_TEXTURE_2D);

        // Hover outline shows which player the next click will select.
        for (const Character& character : characters)
        {
            if (character.bounds.Contains(cursorX, cursorY))
            {
                SetPlayerColor(nextPlayer);
                Outline(character.bounds, 3);
            }
        }

        for (int player = 0; player < 2; ++player)
        {
            const int index = players[player].characterIndex;

            if (index >= 0 &&
                index < static_cast<int>(characters.size()))
            {
                DrawBadge(characters[index].bounds, player);
            }

            DrawPlayerControl(player);
        }

        const bool enabled = Ready();
        const bool hovered = startButton.Contains(cursorX, cursorY);

        if (!enabled)
            glColor4f(0.10f, 0.07f, 0.09f, 0.96f);
        else if (hovered)
            glColor4f(0.48f, 0.19f, 0.16f, 0.98f);
        else
            glColor4f(0.23f, 0.09f, 0.12f, 0.98f);

        Fill(startButton);

        if (enabled)
            glColor3f(1.0f, 0.80f, 0.47f);
        else
            glColor3f(0.43f, 0.36f, 0.32f);

        Outline(startButton, 2);

        const float brightness = enabled ? 1.0f : 0.48f;
        glColor3f(brightness, brightness, brightness);

        const float textWidth = TextWidth("START GAME", 3);
        Text(
            "START GAME",
            startButton.x + (startButton.width - textWidth) / 2,
            startButton.y + (startButton.height - 21) / 2,
            3);
    }

private:
    GLuint backgroundTexture = 0;

    std::vector<Character> characters =
    {
        { "Funghi",   { 26, 137, 298, 416 } },
        { "Withered", { 337, 137, 297, 416 } },
        { "Blanched", { 647, 137, 297, 416 } },
        { "Growly",   { 957, 137, 298, 416 } }
    };

    std::array<PlayerSelection, 2> players{};

    const std::array<Rect, 2> playerPanels =
    { {
        { 60, 587, 292, 88 },
        { 930, 587, 292, 88 }
    } };

    const Rect startButton{ 490, 607, 300, 55 };

    int nextPlayer = 0;
    bool mouseWasDown = true;

    float cursorX = -1;
    float cursorY = -1;

    static void Fill(const Rect& rect)
    {
        glBegin(GL_QUADS);
        glVertex2f(rect.x, rect.y);
        glVertex2f(rect.x + rect.width, rect.y);
        glVertex2f(rect.x + rect.width, rect.y + rect.height);
        glVertex2f(rect.x, rect.y + rect.height);
        glEnd();
    }

    // Filled strips keep borders consistent when the viewport scales.
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

    static void SetPlayerColor(int player)
    {
        if (player == 0)
            glColor3f(0.35f, 0.80f, 1.0f);
        else
            glColor3f(1.0f, 0.40f, 0.55f);
    }

    static void DrawBadge(const Rect& portrait, int player)
    {
        constexpr float size = 40;

        const float x = player == 0
            ? portrait.x + 8
            : portrait.x + portrait.width - size - 8;

        const float y = portrait.y + 8;

        const Rect badge{ x, y, size, size };

        glColor4f(0.035f, 0.025f, 0.04f, 0.96f);
        Fill(badge);

        SetPlayerColor(player);
        Outline(badge, 3);

        glColor3f(1, 1, 1);
        Text(player == 0 ? "1" : "2", x + 10, y + 6, 4);
    }

    void DrawPlayerControl(int player) const
    {
        const Rect& panel = playerPanels[player];

        // Cover the original baked-in HUMAN / NPC line.
        glColor4f(0.055f, 0.035f, 0.045f, 1.0f);
        Fill({ panel.x + 78, 640, 143, 27 });

        const Rect human{ panel.x + 83, 644, 66, 22 };
        const Rect npc{ panel.x + 161, 644, 44, 22 };

        const bool isNPC = players[player].isNPC;
        const Rect& active = isNPC ? npc : human;

        if (player == 0)
            glColor4f(0.10f, 0.28f, 0.37f, 1);
        else
            glColor4f(0.37f, 0.10f, 0.18f, 1);

        Fill(active);
        SetPlayerColor(player);
        Outline(active, 1);

        if (!isNPC)
            glColor3f(1, 1, 1);
        else
            glColor3f(0.50f, 0.47f, 0.46f);

        Text("HUMAN", human.x + 4, human.y + 4, 2);

        if (isNPC)
            glColor3f(1, 1, 1);
        else
            glColor3f(0.50f, 0.47f, 0.46f);

        Text("NPC", npc.x + 5, npc.y + 4, 2);
    }

    // Small independent font for this class's controls.
    static std::array<unsigned char, 7> Glyph(char character)
    {
        switch (character)
        {
        case 'A': return { { 14,17,17,31,17,17,17 } };
        case 'C': return { { 14,17,16,16,16,17,14 } };
        case 'E': return { { 31,16,16,30,16,16,31 } };
        case 'G': return { { 14,17,16,23,17,17,14 } };
        case 'H': return { { 17,17,17,31,17,17,17 } };
        case 'M': return { { 17,27,21,21,17,17,17 } };
        case 'N': return { { 17,25,21,19,17,17,17 } };
        case 'P': return { { 30,17,17,30,16,16,16 } };
        case 'R': return { { 30,17,17,30,20,18,17 } };
        case 'S': return { { 15,16,16,14,1,1,30 } };
        case 'T': return { { 31,4,4,4,4,4,4 } };
        case 'U': return { { 17,17,17,17,17,17,14 } };
        case '1': return { { 4,12,4,4,4,4,14 } };
        case '2': return { { 14,17,1,2,4,8,31 } };
        default:  return { { 0,0,0,0,0,0,0 } };
        }
    }

    static float TextWidth(const char* text, float size)
    {
        float width = 0;

        for (const char* c = text; *c; ++c)
            width += (*c == ' ' ? 4 : 6) * size;

        return width > 0 ? width - size : 0;
    }

    static void Text(
        const char* text, float x, float y, float size)
    {
        for (const char* c = text; *c; ++c)
        {
            const auto glyph = Glyph(*c);

            for (int row = 0; row < 7; ++row)
            {
                for (int column = 0; column < 5; ++column)
                {
                    if (glyph[row] & (1 << (4 - column)))
                    {
                        Fill({
                            x + column * size,
                            y + row * size,
                            size,
                            size
                            });
                    }
                }
            }

            x += (*c == ' ' ? 4 : 6) * size;
        }
    }
};