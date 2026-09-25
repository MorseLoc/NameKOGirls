#pragma once

#include "CombatRules.h"

#include <GLFW/glfw3.h>

#include <fstream>
#include <sstream>
#include <string>

namespace Combat
{
    struct Bindings
    {
        // Standardized GLFW gamepad buttons.
        int padA = GLFW_GAMEPAD_BUTTON_SQUARE;
        int padB = GLFW_GAMEPAD_BUTTON_CIRCLE;

        int padJump = GLFW_GAMEPAD_BUTTON_CROSS;
        int padJump2 = GLFW_GAMEPAD_BUTTON_TRIANGLE;

        int padBlock = GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
        int triggerBlock = GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;

        // Player 1 keyboard defaults.
        int left = GLFW_KEY_A;
        int right = GLFW_KEY_D;
        int up = GLFW_KEY_W;
        int down = GLFW_KEY_S;

        int normal = GLFW_KEY_J;
        int special = GLFW_KEY_K;
        int jump = GLFW_KEY_SPACE;
        int block = GLFW_KEY_L;
    };

    struct MenuInput
    {
        bool pause = false;
        bool up = false;
        bool down = false;
        bool confirm = false;
        bool back = false;
        bool debug = false;
        bool restart = false;
    };

    class HumanController
    {
    public:
        Bindings bindings{};

        // Assigned by FightMatch.
        // -1 means no controller is currently assigned.
        int joystick = -1;

        void SetKeyboardPlayer(int player)
        {
            if (player == 1)
            {
                bindings.left = GLFW_KEY_LEFT;
                bindings.right = GLFW_KEY_RIGHT;
                bindings.up = GLFW_KEY_UP;
                bindings.down = GLFW_KEY_DOWN;

                bindings.normal = GLFW_KEY_KP_1;
                bindings.special = GLFW_KEY_KP_2;
                bindings.jump = GLFW_KEY_KP_0;
                bindings.block = GLFW_KEY_KP_3;
            }
        }

        void Reset()
        {
            axisX = 0;
            axisY = 0;

            previousMenu = {};
            lastJoystick = -2;
        }

        Input Poll(GLFWwindow* window, MenuInput& edges)
        {
            auto key = [&](int code)
                {
                    return window &&
                        glfwGetKey(window, code) == GLFW_PRESS;
                };

            Input in{};

            bool left = key(bindings.left);
            bool right = key(bindings.right);
            bool up = key(bindings.up);
            bool down = key(bindings.down);

            in.a = key(bindings.normal);
            in.b = key(bindings.special);
            in.jump = key(bindings.jump);
            in.block = key(bindings.block);

            MenuInput menu{};

            menu.up = up;
            menu.down = down;
            menu.confirm = key(GLFW_KEY_ENTER);
            menu.back = key(GLFW_KEY_BACKSPACE);
            menu.debug = key(GLFW_KEY_F1);
            menu.restart = key(GLFW_KEY_R);

            // Escape is handled by the existing application callback.
            // Handling it here too would toggle pause twice.

            GLFWgamepadstate pad{};

            const bool connected =
                joystick >= 0 &&
                glfwGetGamepadState(joystick, &pad) == GLFW_TRUE;

            if (joystick != lastJoystick)
            {
                axisX = 0;
                axisY = 0;
                lastJoystick = joystick;
            }

            if (connected)
            {
                auto button = [&](int index)
                    {
                        return pad.buttons[index] == GLFW_PRESS;
                    };

                axisX = Digital(
                    pad.axes[GLFW_GAMEPAD_AXIS_LEFT_X],
                    axisX);

                // Combat input uses positive Y for up.
                axisY = Digital(
                    -pad.axes[GLFW_GAMEPAD_AXIS_LEFT_Y],
                    axisY);

                left |=
                    axisX < 0 ||
                    button(GLFW_GAMEPAD_BUTTON_DPAD_LEFT);

                right |=
                    axisX > 0 ||
                    button(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);

                up |=
                    axisY > 0 ||
                    button(GLFW_GAMEPAD_BUTTON_DPAD_UP);

                down |=
                    axisY < 0 ||
                    button(GLFW_GAMEPAD_BUTTON_DPAD_DOWN);

                in.a |= button(bindings.padA);
                in.b |= button(bindings.padB);

                in.jump |=
                    button(bindings.padJump) ||
                    button(bindings.padJump2);

                in.block |=
                    button(bindings.padBlock) ||
                    pad.axes[bindings.triggerBlock] > 0.15f;

                // PS5 Options button.
                menu.pause =
                    button(GLFW_GAMEPAD_BUTTON_START);

                // Cross confirms; Circle returns.
                menu.confirm |=
                    button(GLFW_GAMEPAD_BUTTON_CROSS);

                menu.back |=
                    button(GLFW_GAMEPAD_BUTTON_CIRCLE);

                // PS5 Create button toggles debug information.
                menu.debug |=
                    button(GLFW_GAMEPAD_BUTTON_BACK);

                // Press both stick buttons to restart the match.
                menu.restart |=
                    button(GLFW_GAMEPAD_BUTTON_LEFT_THUMB) &&
                    button(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);
            }

            // Opposing directions cancel to neutral.
            in.x = static_cast<int>(right) -
                static_cast<int>(left);

            in.y = static_cast<int>(up) -
                static_cast<int>(down);

            menu.up = up && !down;
            menu.down = down && !up;

            // Menu actions fire only on a new press.
            edges = {
                menu.pause && !previousMenu.pause,
                menu.up && !previousMenu.up,
                menu.down && !previousMenu.down,
                menu.confirm && !previousMenu.confirm,
                menu.back && !previousMenu.back,
                menu.debug && !previousMenu.debug,
                menu.restart && !previousMenu.restart
            };

            previousMenu = menu;

            return in;
        }

        // Optional SDL-format controller mappings.
        // GLFW's built-in mappings work without this file.
        static void LoadMappings(const std::string& path)
        {
            std::ifstream file(path);

            if (!file)
                return;

            std::ostringstream data;
            data << file.rdbuf();

            glfwUpdateGamepadMappings(data.str().c_str());
        }

    private:
        int axisX = 0;
        int axisY = 0;
        int lastJoystick = -2;

        MenuInput previousMenu{};

        // Separate press/release thresholds prevent stick jitter
        // from repeatedly generating direction presses.
        static int Digital(float value, int previous)
        {
            if (value > 0.55f)
                return 1;

            if (value < -0.55f)
                return -1;

            if (std::abs(value) < 0.35f)
                return 0;

            return previous;
        }
    };
}