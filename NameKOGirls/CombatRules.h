#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Combat
{
    constexpr float StepSeconds = 1.0f / 60.0f;
    struct Box
    {
        float x = 0, y = 0, w = 0, h = 0;
        bool Overlaps(const Box& b) const
        {
            return x < b.x + b.w && x + w > b.x && y < b.y + b.h && y + h > b.y;
        }
    };
    struct LocalBox { float forward = 0, above = 0, width = 0, height = 0; };
    struct ArenaSettings
    {
        float left = 60, right = 1220, floor = 490;
        float spawn1 = 250, spawn2 = 1030;
    };
    struct Tuning
    {
        float walk = 270, backwardRatio = .65f, runRatio = 1.65f, dashRatio = 2.5f;
        float airRatio = .9f, gravity = 1875, jumpSpeed = 750;
        float maxHealth = 100, hurtWidth = 96, hurtHeight = 210;
        float pushWidth = 64, pushHeight = 174;
        float headbuttSpeed = 740, spinSpeed = 310, diveSpeed = 1050;
        int bufferTicks = 9, doubleTapTicks = 15, dashTicks = 12;
        int maxChargeTicks = 60, emptyLanding = 3, attackLanding = 8, specialLanding = 10;
        float chip = .10f;
    };
    enum class Move
    {
        Free, Dash, Run, Jab, Swat, LowSwipe, Uppercut, DownKick,
        Headbutt, Charge, Punch, BurrowDive, Burrow, Crash, Spin
    };
    inline const char* Name(Move m)
    {
        switch (m) {
        case Move::Free:return "READY"; case Move::Dash:return "DASH";
        case Move::Run:return "RUN"; case Move::Jab:return "JAB";
        case Move::Swat:return "SWAT"; case Move::LowSwipe:return "LOW SWIPE";
        case Move::Uppercut:return "UPPERCUT"; case Move::DownKick:return "DOWN KICK";
        case Move::Headbutt:return "HEADBUTT"; case Move::Charge:return "CHARGE";
        case Move::Punch:return "PUNCH"; case Move::BurrowDive:return "BURROW DIVE";
        case Move::Burrow:return "BURROW"; case Move::Crash:return "CRASH";
        case Move::Spin:return "SPIN";
        }
        return "READY";
    }
    struct MoveData
    {
        int startup = 1, active = 0, recovery = 0;
        float damage = 0, knockX = 0, knockUp = 0;
        int hitstun = 0, blockstun = 0, hitstop = 0;
        LocalBox box;
        int Total() const { return startup - 1 + active + recovery; }
    };
    // All distances are in the existing 1280 x 720 design coordinates.
    // Local attack boxes extend forward from the feet/root; above is bottom height.
    inline MoveData Data(Move m, float charge = 0)
    {
        switch (m) {
        case Move::Jab: return { 6,3,10,4,160,0,15,9,3,{24,108,94,44} };
        case Move::Swat:return { 8,4,14,3,140,0,16,10,3,{22,100,112,54} };
        case Move::LowSwipe:return { 9,5,17,6,270,0,22,13,4,{14,5,130,46} };
        case Move::Uppercut:return { 8,6,20,7,190,490,24,14,5,{10,95,86,135} };
        case Move::DownKick:return { 9,8,18,7,150,-420,22,13,5,{0,-42,82,94} };
        case Move::Headbutt:return { 11,8,28,10,520,120,28,16,6,{22,70,136,94} };
        case Move::Punch:return { 12,5,26 + int(std::lround(8 * charge)),8 + 10 * charge,
            300 + 350 * charge,80 + 120 * charge,25 + int(std::lround(10 * charge)),
            15 + int(std::lround(5 * charge)),5 + int(std::lround(3 * charge)),{20,90,152,70} };
        case Move::Burrow:return { 61,6,30,12,220,600,30,18,6,{-48,0,96,244} };
        case Move::Crash:return { 1,1,36,16,570,120,32,20,8,{-48,-26,96,120} };
        case Move::Spin:return { 10,21,24,3,60,0,16,10,2,{-108,88,216,62} };
        default:return {};
        }
    }
    inline bool IsNormal(Move m)
    {
        return m == Move::Jab || m == Move::Swat || m == Move::LowSwipe || m == Move::Uppercut || m == Move::DownKick || m == Move::Headbutt;
    }
    inline bool IsAttack(Move m)
    {
        return IsNormal(m) || m == Move::Charge || m == Move::Punch || m == Move::BurrowDive || m == Move::Burrow || m == Move::Crash || m == Move::Spin;
    }
    struct Input { int x = 0, y = 0; bool a = false, b = false, jump = false, block = false; };
    enum class Button { None, Jump, A, B };
    struct Pending { Button button = Button::None; int x = 0, y = 0, age = 0; bool dashChord = false; };
}
