#pragma once

#include "CombatSimulation.h"
#include "CombatDraw.h"

#include <stb_image.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Loads and draws one fighter's animations.
// Gameplay state is managed by Combat::Simulation.
class Character
{
public:
    Character() = default;

    Character(const Character&) = delete;
    Character& operator=(const Character&) = delete;

    static constexpr float DrawScale = 1.5f;

    bool LoadIdle(const std::string& path)
    {
        Release();

        const auto root =
            std::filesystem::path(path).parent_path();

        if (!LoadClip("Idle", path, true))
            return false;

        const char* names[] = {
            "Walk",
            "Backstep",
            "Run",
            "Dash",
            "BackDash",
            "JumpRise",
            "JumpFall",
            "Landing",
            "Crouch",
            "Guard",
            "AirGuard",
            "Hit",
            "BlockImpact",
            "Jab",
            "Swat",
            "LowSwipe",
            "Uppercut",
            "DownKick",
            "Headbutt",
            "Charge",
            "Punch",
            "BurrowDive",
            "Burrow",
            "Crash",
            "Spin"
        };

        for (const char* name : names)
        {
            const auto animationPath =
                root / (std::string(name) + ".png");

            if (std::filesystem::exists(animationPath))
            {
                LoadClip(
                    name,
                    animationPath.string(),
                    false);
            }
        }

        return true;
    }

    // Call while the OpenGL context still exists.
    void Release()
    {
        for (auto& entry : clips)
        {
            if (entry.second.texture)
                glDeleteTextures(1, &entry.second.texture);
        }

        clips.clear();
    }

    void Draw(
        const Combat::Fighter& fighter,
        const Combat::ArenaSettings& arena,
        int player) const
    {
        using namespace Combat;

        const auto& f = fighter;

        glDisable(GL_TEXTURE_2D);

        // Placeholder underground movement and warning effects.
        if (f.underground)
        {
            const float pulse =
                static_cast<float>(f.tick % 6);

            glColor4f(0.56f, 0.35f, 0.20f, 1.0f);

            CombatDraw::Fill(
                f.x - 35,
                arena.floor - 8 - pulse,
                70,
                8 + pulse);

            if (f.tick >= 37)
            {
                glColor3f(1.0f, 0.8f, 0.35f);

                CombatDraw::Text(
                    "!",
                    f.x - 4,
                    arena.floor - 30,
                    2);
            }

            return;
        }

        const std::string name = ClipName(f);

        auto it = clips.find(name);
        const bool fallback = it == clips.end();

        if (fallback)
            it = clips.find("Idle");

        if (it == clips.end())
            return;

        const Clip& clip = it->second;

        if (lastClip != name ||
            f.animTicks < clipStartedAt ||
            lastInstance != f.instance)
        {
            lastClip = name;
            clipStartedAt = f.animTicks;
            lastInstance = f.instance;
        }

        const int frame = Frame(
            f,
            clip,
            name,
            fallback,
            f.animTicks - clipStartedAt);

        // Compress the idle fallback when crouching.
        const float scaleY =
            fallback && f.crouched ? 0.5f : 1.0f;

        const float width = 160 * DrawScale;
        const float height = 192 * DrawScale * scaleY;

        // Feet/root anchor within each sprite cell: (80, 172).
        const float x = f.x - 80 * DrawScale;

        const float y =
            arena.floor -
            f.height -
            172 * DrawScale * scaleY;

        float u0 = static_cast<float>(frame) / clip.count;
        float u1 = static_cast<float>(frame + 1) / clip.count;

        // Source artwork faces right.
        if (f.facing < 0)
            std::swap(u0, u1);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, clip.texture);

        if (f.health <= 0)
        {
            glColor4f(0.45f, 0.35f, 0.35f, 1.0f);
        }
        else if (f.stun > 0)
        {
            glColor4f(1.0f, 0.48f, 0.48f, 1.0f);
        }
        else if (f.guarding)
        {
            glColor4f(0.6f, 0.8f, 1.0f, 1.0f);
        }
        else if (player == 1)
        {
            glColor4f(0.92f, 0.92f, 1.0f, 1.0f);
        }
        else
        {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }

        glPushMatrix();

        // Rotate the fallback sprite during the headfirst crash.
        if (fallback &&
            f.move == Move::Crash &&
            f.phase == 2)
        {
            const float centerY = y + height * 0.5f;

            glTranslatef(f.x, centerY, 0);
            glRotatef(180, 0, 0, 1);
            glTranslatef(-f.x, -centerY, 0);
        }

        glBegin(GL_QUADS);

        glTexCoord2f(u0, 0);
        glVertex2f(x, y);

        glTexCoord2f(u1, 0);
        glVertex2f(x + width, y);

        glTexCoord2f(u1, 1);
        glVertex2f(x + width, y + height);

        glTexCoord2f(u0, 1);
        glVertex2f(x, y + height);

        glEnd();

        glPopMatrix();
        glDisable(GL_TEXTURE_2D);

        if (f.move == Move::Charge)
        {
            glColor3f(0.12f, 0.1f, 0.08f);

            CombatDraw::Fill(
                f.x - 45,
                y - 16,
                90,
                8);

            const float chargeRatio = std::min(
                1.0f,
                static_cast<float>(f.chargeTicks) / 60.0f);

            glColor3f(1.0f, 0.78f, 0.3f);

            CombatDraw::Fill(
                f.x - 45,
                y - 16,
                90 * chargeRatio,
                8);
        }

        // Identify players when both use Funghi's artwork.
        if (player == 0)
            glColor3f(0.3f, 0.8f, 1.0f);
        else
            glColor3f(1.0f, 0.45f, 0.6f);

        CombatDraw::Text(
            player == 0 ? "1" : "2",
            f.x - 5,
            y - 32,
            2);
    }

private:
    struct Clip
    {
        GLuint texture = 0;
        int count = 0;
        std::vector<int> durations;
    };

    std::map<std::string, Clip> clips;

    mutable std::string lastClip;
    mutable int clipStartedAt = 0;
    mutable std::uint64_t lastInstance = 0;

    bool LoadClip(
        const std::string& key,
        const std::string& path,
        bool required)
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(false);

        unsigned char* data = stbi_load(
            path.c_str(),
            &width,
            &height,
            &channels,
            STBI_rgb_alpha);

        if (!data)
        {
            if (required)
                std::cerr << "Cannot load " << path << '\n';

            return false;
        }

        GLint limit = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);

        if (height != 192 ||
            width < 160 ||
            width % 160 != 0 ||
            width > limit ||
            height > limit)
        {
            stbi_image_free(data);

            std::cerr
                << "Expected horizontal 160x192 cells: "
                << path << '\n';

            return false;
        }

        // Clear existing errors before checking this upload.
        while (glGetError() != GL_NO_ERROR)
        {
        }

        Clip clip;
        clip.count = width / 160;

        glGenTextures(1, &clip.texture);
        glBindTexture(GL_TEXTURE_2D, clip.texture);

        // Nearest filtering preserves sharp pixel edges.
        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_NEAREST);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_NEAREST);

        constexpr GLint clampToEdge = 0x812F;

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            clampToEdge);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            clampToEdge);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data);

        stbi_image_free(data);

        if (glGetError() != GL_NO_ERROR || !clip.texture)
        {
            if (clip.texture)
                glDeleteTextures(1, &clip.texture);

            return false;
        }

        // Optional matching timing file, such as Jab.ticks.
        // Provide one positive duration per drawing.
        // Durations use simulation ticks: 60 ticks = one second.
        auto timingPath = std::filesystem::path(path);
        timingPath.replace_extension(".ticks");

        std::ifstream timingFile(timingPath);

        int duration = 0;

        while (timingFile >> duration)
        {
            if (duration <= 0)
            {
                clip.durations.clear();
                break;
            }

            clip.durations.push_back(duration);
        }

        if (static_cast<int>(clip.durations.size()) != clip.count)
            clip.durations.clear();

        clips.emplace(key, std::move(clip));

        return true;
    }

    static std::string ClipName(const Combat::Fighter& f)
    {
        using namespace Combat;

        if (f.stun)
            return "Hit";

        if (f.blockstun)
            return "BlockImpact";

        if (f.guarding)
            return f.grounded ? "Guard" : "AirGuard";

        switch (f.move)
        {
        case Move::Dash:
            return f.dashDirection == f.facing ?
                "Dash" : "BackDash";

        case Move::Run:
            return "Run";

        case Move::Jab:
            return "Jab";

        case Move::Swat:
            return "Swat";

        case Move::LowSwipe:
            return "LowSwipe";

        case Move::Uppercut:
            return "Uppercut";

        case Move::DownKick:
            return "DownKick";

        case Move::Headbutt:
            return "Headbutt";

        case Move::Charge:
            return "Charge";

        case Move::Punch:
            return "Punch";

        case Move::BurrowDive:
            return "BurrowDive";

        case Move::Burrow:
            return "Burrow";

        case Move::Crash:
            return "Crash";

        case Move::Spin:
            return "Spin";

        default:
            break;
        }

        if (!f.grounded)
            return f.vy > 0 ? "JumpRise" : "JumpFall";

        if (f.landing)
            return "Landing";

        if (f.crouched)
            return "Crouch";

        if (std::abs(f.actualVx) > 5)
        {
            return f.actualVx * f.facing > 0 ?
                "Walk" : "Backstep";
        }

        return "Idle";
    }

    static int Frame(
        const Combat::Fighter& f,
        const Clip& clip,
        const std::string& name,
        bool fallback,
        int elapsed)
    {
        using namespace Combat;

        int tick = std::max(0, f.tick - 1);

        const bool loop =
            fallback ||
            name == "Idle" ||
            name == "Walk" ||
            name == "Backstep" ||
            name == "Run" ||
            name == "Charge";

        if (clip.durations.empty())
        {
            // Five simulation ticks per drawing = 12 FPS.
            if (loop)
                return (f.animTicks / 5) % clip.count;

            if (IsNormal(f.move) ||
                f.move == Move::Punch ||
                f.move == Move::Spin ||
                f.move == Move::Burrow)
            {
                const int total = std::max(
                    1,
                    Data(f.move, f.charge).Total());

                return std::min(
                    clip.count - 1,
                    tick * clip.count / total);
            }

            if (f.move == Move::Dash)
            {
                return std::min(
                    clip.count - 1,
                    tick * clip.count / 12);
            }

            if (f.move == Move::Crash)
            {
                int section = 0;

                if (f.phase == 0)
                    section = f.tick < 12 ? 0 : 1;
                else if (f.phase == 1)
                    section = 2;
                else if (f.phase == 2)
                    section = 3;
                else
                    section = 4;

                return std::min(
                    clip.count - 1,
                    section * (clip.count - 1) / 4);
            }

            // Play other clips once, then hold the final drawing.
            return std::min(
                clip.count - 1,
                elapsed / 5);
        }

        int total = 0;

        for (int duration : clip.durations)
            total += duration;

        if (loop)
        {
            tick = f.animTicks % total;
        }
        else
        {
            const bool usesMoveTimeline =
                IsAttack(f.move) ||
                f.move == Move::Dash;

            tick = std::min(
                usesMoveTimeline ? tick : elapsed,
                total - 1);
        }

        for (int i = 0; i < clip.count; ++i)
        {
            if (tick < clip.durations[i])
                return i;

            tick -= clip.durations[i];
        }

        return clip.count - 1;
    }
};