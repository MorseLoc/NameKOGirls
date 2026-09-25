#pragma once

#include "Character.h"
#include "CombatInput.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>

class FightMatch
{
public:
    Combat::Simulation simulation;
    bool debug = false;

    bool Start(
        const std::string& idlePath,
        GLFWwindow* window,
        bool npc1,
        bool npc2)
    {
        Release();

        host = window;
        npc = { npc1, npc2 };

        if (!sprites[0].LoadIdle(idlePath) ||
            !sprites[1].LoadIdle(idlePath))
        {
            Release();
            return false;
        }

        // Idle.png lives in assets/Characters/Funghi.
        const auto assetsFolder =
            std::filesystem::path(idlePath)
            .parent_path()
            .parent_path()
            .parent_path();

        Combat::HumanController::LoadMappings(
            (assetsFolder / "gamecontrollerdb.txt").string());

        controllers[0].SetKeyboardPlayer(0);
        controllers[1].SetKeyboardPlayer(1);

        for (auto& controller : controllers)
            controller.Reset();

        simulation.Reset();

        accumulator = 0;
        running = true;
        ignoreNextPoll = true;

        return true;
    }

    Combat::MenuInput Poll()
    {
        Combat::MenuInput all{};

        if (!running)
            return all;

        // Remove disconnected controller assignments.
        for (int i = 0; i < 2; ++i)
        {
            if (controllers[i].joystick >= 0 &&
                !glfwJoystickIsGamepad(controllers[i].joystick))
            {
                controllers[i].joystick = -1;
            }
        }

        // Assign available controllers to human-controlled slots.
        for (int id = GLFW_JOYSTICK_1;
            id <= GLFW_JOYSTICK_LAST;
            ++id)
        {
            if (!glfwJoystickIsGamepad(id))
                continue;

            if (controllers[0].joystick == id ||
                controllers[1].joystick == id)
            {
                continue;
            }

            for (int i = 0; i < 2; ++i)
            {
                if (!npc[i] && controllers[i].joystick < 0)
                {
                    controllers[i].joystick = id;
                    break;
                }
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            Combat::MenuInput menu{};

            inputs[i] = controllers[i].Poll(host, menu);

            all.pause |= menu.pause;
            all.up |= menu.up;
            all.down |= menu.down;
            all.confirm |= menu.confirm;
            all.back |= menu.back;
            all.debug |= menu.debug;
            all.restart |= menu.restart;
        }

        // Prevent held menu buttons from becoming fresh combat presses
        // when entering the arena.
        if (ignoreNextPoll)
        {
            all = {};
            ignoreNextPoll = false;
            Sync();
        }

        return all;
    }

    void Update(double delta, bool paused)
    {
        if (!running)
            return;

        if (paused)
        {
            Sync();
            accumulator = 0;
            return;
        }

        // Sample human input even when this render frame does not
        // contain a simulation tick.
        for (int i = 0; i < 2; ++i)
        {
            if (!npc[i])
                simulation.Submit(i, inputs[i]);
        }

        accumulator += std::clamp(delta, 0.0, 0.1);

        constexpr double fixedStep = 1.0 / 60.0;

        while (accumulator + 1e-9 >= fixedStep)
        {
            for (int i = 0; i < 2; ++i)
            {
                if (npc[i])
                    simulation.Submit(i, NPC(i));
            }

            simulation.Step();
            accumulator -= fixedStep;
        }
    }

    void Sync()
    {
        // Clear queued actions and double-tap history, then establish
        // currently held buttons without creating new press events.
        simulation.ClearInputs();

        for (int i = 0; i < 2; ++i)
            simulation.SyncInput(i, inputs[i]);
    }

    void Restart()
    {
        simulation.Reset();
        accumulator = 0;
        Sync();
    }

    // Call before destroying the OpenGL window.
    void Release()
    {
        sprites[0].Release();
        sprites[1].Release();

        running = false;
        accumulator = 0;

        for (auto& controller : controllers)
            controller.joystick = -1;
    }

    void Draw() const
    {
        if (!running)
            return;

        glDisable(GL_TEXTURE_2D);

        for (int i = 0; i < 2; ++i)
        {
            const auto& fighter = simulation.fighters[i];

            sprites[i].Draw(
                fighter,
                simulation.arena,
                i);

            const auto strike = simulation.Attack(fighter);

            // Visible attack volumes make moves testable before
            // their finished animation sheets are available.
            if (strike.valid)
            {
                glColor4f(1.0f, 0.65f, 0.18f, 0.22f);

                CombatDraw::Fill(
                    strike.box.x,
                    strike.box.y,
                    strike.box.w,
                    strike.box.h);

                glColor4f(1.0f, 0.75f, 0.25f, 0.85f);
                CombatDraw::Outline(strike.box);
            }

            if (debug && !fighter.underground)
            {
                // Green: hurtbox.
                glColor3f(0.2f, 1.0f, 0.45f);
                CombatDraw::Outline(simulation.Hurt(fighter));

                // Blue: body collision/pushbox.
                glColor3f(0.25f, 0.65f, 1.0f);
                CombatDraw::Outline(
                    simulation.Push(fighter),
                    1.0f);
            }

            const float x = i ? 770.0f : 50.0f;

            glColor4f(0.04f, 0.035f, 0.03f, 0.9f);
            CombatDraw::Fill(x, 30, 460, 28);

            if (i == 0)
                glColor3f(0.3f, 0.85f, 0.65f);
            else
                glColor3f(1.0f, 0.35f, 0.5f);

            const float healthRatio =
                fighter.health / simulation.tuning.maxHealth;

            CombatDraw::Fill(
                x + 3,
                33,
                454 * healthRatio,
                22);

            glColor3f(1.0f, 1.0f, 1.0f);

            char text[96];

            std::snprintf(
                text,
                sizeof(text),
                "P%d FUNGHI  HP %.1f  %s",
                i + 1,
                fighter.health,
                npc[i] ? "NPC" : "HUMAN");

            CombatDraw::Text(text, x, 66, 2);

            CombatDraw::Text(
                fighter.guarding ?
                "GUARD" : Combat::Name(fighter.move),
                x,
                89,
                2);

            if (!npc[i])
            {
                CombatDraw::Text(
                    controllers[i].joystick >= 0 ?
                    "GAMEPAD + KEYBOARD" :
                    "KEYBOARD / NO MAPPED PAD",
                    x,
                    111,
                    1.5f);
            }

            if (debug)
            {
                std::snprintf(
                    text,
                    sizeof(text),
                    "TICK %d PHASE %d QUEUE %d AGE %d",
                    fighter.tick,
                    fighter.phase,
                    static_cast<int>(fighter.pending.button),
                    fighter.pending.age);

                CombatDraw::Text(text, x, 137, 1.5f);

                std::snprintf(
                    text,
                    sizeof(text),
                    "VX %.0f VY %.0f FACE %d UP A %d UP B %d",
                    fighter.actualVx,
                    fighter.vy,
                    fighter.facing,
                    static_cast<int>(fighter.upperUsed),
                    static_cast<int>(fighter.crashUsed));

                CombatDraw::Text(text, x, 155, 1.5f);

                std::snprintf(
                    text,
                    sizeof(text),
                    "STUN %d BLOCK %d STOP %d LAND %d",
                    fighter.stun,
                    fighter.blockstun,
                    fighter.hitstop,
                    fighter.landing);

                CombatDraw::Text(text, x, 173, 1.5f);
            }
        }

        glColor3f(1.0f, 0.9f, 0.7f);

        CombatDraw::Text(
            "F1 / CREATE: BOXES   R / L3+R3: REMATCH   OPTIONS / ESC: PAUSE",
            80,
            685,
            2);

        if (simulation.Finished())
        {
            glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
            CombatDraw::Fill(365, 280, 550, 120);

            glColor3f(1.0f, 0.85f, 0.5f);

            const int winner = simulation.Winner();

            const std::string title = winner < 0 ?
                "DOUBLE KO" :
                "PLAYER " + std::to_string(winner + 1) + " WINS";

            CombatDraw::Text(title, 410, 305, 4);

            CombatDraw::Text(
                "R OR L3+R3 TO REMATCH",
                410,
                360,
                2);
        }
    }

private:
    Character sprites[2];

    std::array<Combat::HumanController, 2> controllers;
    std::array<Combat::Input, 2> inputs{};
    std::array<bool, 2> npc{};

    GLFWwindow* host = nullptr;

    bool running = false;
    bool ignoreNextPoll = true;

    double accumulator = 0;

    Combat::Input NPC(int i)
    {
        // Basic test opponent using the same input and move system
        // as a human player.
        const auto& fighter = simulation.fighters[i];
        const auto& other = simulation.fighters[1 - i];

        Combat::Input in{};

        const float distance =
            std::abs(other.x - fighter.x);

        in.x = other.x > fighter.x ? 1 : -1;

        if (distance < 115)
            in.x = 0;

        const int time = simulation.clock + i * 17;

        if (fighter.move == Combat::Move::Charge)
        {
            in.b = fighter.chargeTicks < 35;
            in.x = 0;
            return in;
        }

        if (Combat::IsAttack(other.move) &&
            distance < 180 &&
            time % 90 < 22)
        {
            in.block = true;
            in.x = 0;
            return in;
        }

        if (time % 37 == 0)
        {
            const int choice = (time / 37 + i) % 9;

            if (choice == 0)
            {
                in.jump = true;
            }
            else if (choice <= 4)
            {
                in.a = true;

                if (choice == 2)
                    in.y = -1;
                else if (choice == 3)
                    in.y = 1;
            }
            else
            {
                in.b = true;

                if (choice == 5)
                    in.y = -1;
                else if (choice == 6)
                    in.y = 1;

                if (choice == 7)
                    in.x = 0;
            }
        }

        return in;
    }
};