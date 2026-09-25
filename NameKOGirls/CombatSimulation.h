#pragma once

#include "CombatRules.h"
#include <utility>

namespace Combat
{
    struct Fighter
    {
        float x = 0;
        float height = 0;
        float vy = 0;
        float knockVx = 0;
        float health = 100;
        float actualVx = 0;

        int facing = 1;
        int tick = 0;
        int stun = 0;
        int blockstun = 0;
        int hitstop = 0;
        int landing = 0;

        int dashDirection = 0;
        int phase = 0;
        int phaseTick = 0;
        int chargeTicks = 0;

        int lastLeft = -1000;
        int lastRight = -1000;

        float targetX = 0;
        float burrowStartX = 0;
        float charge = 0;

        bool grounded = true;
        bool guarding = false;
        bool crouched = false;
        bool upperUsed = false;
        bool crashUsed = false;

        bool boost = false;
        bool airAttack = false;
        bool landedAttack = false;
        bool underground = false;
        bool freshLanding = false;

        Move move = Move::Free;

        Input held{};
        Input previous{};
        Pending pending{};

        std::array<bool, 3> hit{ { false, false, false } };

        std::uint64_t instance = 0;
        int animTicks = 0;
    };

    struct Strike
    {
        bool valid = false;

        Box box{};
        MoveData data{};

        int id = 0;
        int direction = 1;

        std::uint64_t instance = 0;
    };

    class Simulation
    {
    public:
        ArenaSettings arena{};
        Tuning tuning{};

        std::array<Fighter, 2> fighters{};

        int clock = 0;

        void Reset()
        {
            fighters = {};
            clock = 0;

            for (int i = 0; i < 2; ++i)
            {
                auto& f = fighters[i];

                f.x = i ? arena.spawn2 : arena.spawn1;
                f.health = tuning.maxHealth;
                f.facing = i ? -1 : 1;
            }
        }

        bool Finished() const
        {
            return fighters[0].health <= 0 ||
                fighters[1].health <= 0;
        }

        int Winner() const
        {
            if (fighters[0].health <= 0)
                return fighters[1].health <= 0 ? -1 : 1;

            return 0;
        }

        void ClearInputs()
        {
            for (auto& f : fighters)
            {
                f.pending = {};
                f.held = {};
                f.previous = {};
                f.lastLeft = -1000;
                f.lastRight = -1000;
            }
        }

        void SyncInput(int i, const Input& input)
        {
            fighters[i].held = input;
            fighters[i].previous = input;
            fighters[i].pending = {};
        }

        // Called once per input poll, including during hitstop.
        // Presses survive render frames that have no simulation tick.
        void Submit(int i, Input in)
        {
            auto& f = fighters[i];

            in.x = std::clamp(in.x, -1, 1);
            in.y = std::clamp(in.y, -1, 1);

            const auto old = f.previous;

            f.previous = in;
            f.held = in;

            if (Finished())
                return;

            const bool a = in.a && !old.a;
            const bool b = in.b && !old.b;

            const bool jump =
                (in.jump && !old.jump) ||
                (in.y == 1 && old.y != 1);

            int dash = 0;

            if (in.x && in.x != old.x)
            {
                int& last = in.x > 0 ?
                    f.lastRight : f.lastLeft;

                if (clock - last <= tuning.doubleTapTicks)
                {
                    dash = in.x;
                    last = -1000;
                }
                else
                {
                    last = clock;
                }
            }

            if (in.block)
            {
                f.pending = {};
                return;
            }

            // For simultaneous presses within one poll:
            // B takes precedence over A, then jump.
            if (jump)
                f.pending = { Button::Jump, in.x, in.y, 0, false };

            if (a)
                f.pending = { Button::A, in.x, in.y, 0, dash != 0 };

            if (b)
                f.pending = { Button::B, in.x, in.y, 0, false };

            if (dash &&
                !a &&
                !b &&
                !jump &&
                CanAct(f) &&
                f.hitstop == 0 &&
                f.grounded &&
                f.landing == 0)
            {
                Start(f, Move::Dash);
                f.dashDirection = dash;
            }
        }

        Box Hurt(const Fighter& f) const
        {
            const float h =
                tuning.hurtHeight * (f.crouched ? 0.5f : 1.0f);

            return {
                f.x - tuning.hurtWidth * 0.5f,
                arena.floor - f.height - h,
                tuning.hurtWidth,
                h
            };
        }

        Box Push(const Fighter& f) const
        {
            return {
                f.x - tuning.pushWidth * 0.5f,
                arena.floor - f.height - tuning.pushHeight,
                tuning.pushWidth,
                tuning.pushHeight
            };
        }

        Box World(const Fighter& f, LocalBox b) const
        {
            return {
                f.facing > 0 ?
                    f.x + b.forward :
                    f.x - b.forward - b.width,

                arena.floor - f.height - b.above - b.height,
                b.width,
                b.height
            };
        }

        Strike Attack(const Fighter& f) const
        {
            Strike s;

            s.instance = f.instance;
            s.direction = f.facing;

            if (f.health <= 0 ||
                f.stun > 0 ||
                f.underground ||
                f.landedAttack)
            {
                return s;
            }

            auto d = Data(f.move, f.charge);
            int id = 0;
            bool active = false;

            if (f.move == Move::Spin)
            {
                if (f.tick >= 10 && f.tick <= 12)
                {
                    active = true;
                }
                else if (f.tick >= 19 && f.tick <= 21)
                {
                    active = true;
                    id = 1;
                }
                else if (f.tick >= 28 && f.tick <= 30)
                {
                    active = true;
                    id = 2;

                    d.damage = 6;
                    d.knockX = 430;
                    d.knockUp = 90;
                    d.hitstun = 26;
                    d.blockstun = 16;
                    d.hitstop = 5;
                }
            }
            else if (f.move == Move::Crash)
            {
                active = f.phase == 2;
            }
            else if (IsNormal(f.move) ||
                f.move == Move::Punch ||
                f.move == Move::Burrow)
            {
                active =
                    f.tick >= d.startup &&
                    f.tick < d.startup + d.active;
            }

            if (active && !f.hit[id])
            {
                s.valid = true;
                s.id = id;
                s.data = d;
                s.box = World(f, d.box);
            }

            return s;
        }

        void Step()
        {
            if (Finished())
                return;

            ++clock;

            const auto before = fighters;
            std::array<bool, 2> frozen{};

            for (int i = 0; i < 2; ++i)
            {
                auto& f = fighters[i];

                frozen[i] = f.hitstop > 0;

                if (frozen[i])
                {
                    --f.hitstop;
                    continue;
                }

                TickFighter(i);
            }

            ResolvePush(before, frozen);

            // Capture both attacks and defenses before applying damage.
            // This allows simultaneous attacks to trade.
            const auto snapshot = fighters;

            Strike attacks[2] = {
                Attack(snapshot[0]),
                Attack(snapshot[1])
            };

            bool connects[2] = { false, false };

            for (int i = 0; i < 2; ++i)
            {
                const int j = 1 - i;

                if (!frozen[i] &&
                    !frozen[j] &&
                    attacks[i].valid &&
                    !snapshot[j].underground)
                {
                    Box oldHit = attacks[i].box;

                    oldHit.x += before[i].x - snapshot[i].x;
                    oldHit.y -= before[i].height - snapshot[i].height;

                    connects[i] = Swept(
                        oldHit,
                        attacks[i].box,
                        Hurt(before[j]),
                        Hurt(snapshot[j]));
                }
            }

            // Register hits before an interruption changes a move.
            for (int i = 0; i < 2; ++i)
            {
                if (connects[i])
                    fighters[i].hit[attacks[i].id] = true;
            }

            for (int i = 0; i < 2; ++i)
            {
                if (connects[i])
                {
                    ApplyHit(
                        1 - i,
                        attacks[i],
                        snapshot[1 - i].guarding);
                }
            }

            for (int i = 0; i < 2; ++i)
            {
                if (connects[i])
                {
                    fighters[i].hitstop = std::max(
                        fighters[i].hitstop,
                        attacks[i].data.hitstop);
                }
            }

            for (int i = 0; i < 2; ++i)
            {
                auto& f = fighters[i];

                // Keep the final descent hit active through touchdown.
                if (!frozen[i] &&
                    f.move == Move::Crash &&
                    f.phase == 2 &&
                    f.grounded)
                {
                    f.phase = 3;
                    f.phaseTick = 0;
                }

                if (!frozen[i] &&
                    f.pending.button != Button::None &&
                    ++f.pending.age >= tuning.bufferTicks)
                {
                    f.pending = {};
                }
            }
        }

        // Continuous box sweep using relative motion.
        // Fast lunges and dives cannot skip a target between ticks.
        static bool Swept(Box a0, Box a1, Box b0, Box b1)
        {
            if (a0.Overlaps(b0) || a1.Overlaps(b1))
                return true;

            const float dx =
                (a1.x - a0.x) - (b1.x - b0.x);

            const float dy =
                (a1.y - a0.y) - (b1.y - b0.y);

            float enter = 0;
            float leave = 1;

            auto slab = [&](
                float lo,
                float hi,
                float blo,
                float bhi,
                float d)
                {
                    if (std::abs(d) < 0.00001f)
                        return hi > blo && lo < bhi;

                    float t0 = (blo - hi) / d;
                    float t1 = (bhi - lo) / d;

                    if (t0 > t1)
                        std::swap(t0, t1);

                    enter = std::max(enter, t0);
                    leave = std::min(leave, t1);

                    return enter <= leave;
                };

            return
                slab(
                    a0.x, a0.x + a0.w,
                    b0.x, b0.x + b0.w,
                    dx) &&
                slab(
                    a0.y, a0.y + a0.h,
                    b0.y, b0.y + b0.h,
                    dy) &&
                leave >= 0 &&
                enter <= 1;
        }

    private:
        bool CanAct(const Fighter& f) const
        {
            return
                f.stun == 0 &&
                f.blockstun == 0 &&
                (f.move == Move::Free || f.move == Move::Run);
        }

        void Start(Fighter& f, Move move)
        {
            f.move = move;
            f.tick = 0;
            f.phase = 0;
            f.phaseTick = 0;
            f.hit = {};

            ++f.instance;

            f.airAttack = !f.grounded;
            f.landedAttack = false;
            f.guarding = false;
            f.boost = false;

            if (move == Move::Charge)
            {
                f.chargeTicks = 0;
                f.charge = 0;
            }
        }

        void End(Fighter& f)
        {
            f.move = Move::Free;
            f.tick = 0;
            f.phase = 0;
            f.phaseTick = 0;
            f.landedAttack = false;
            f.underground = false;
        }

        void Face(Fighter& f, const Fighter& other)
        {
            if (other.x > f.x)
                f.facing = 1;
            else if (other.x < f.x)
                f.facing = -1;
        }

        void BeginPending(Fighter& f, const Fighter& other)
        {
            if (f.pending.button == Button::None ||
                f.held.block ||
                f.stun ||
                f.blockstun ||
                f.landing)
            {
                return;
            }

            const auto p = f.pending;

            const bool dashA =
                p.button == Button::A &&
                p.y == 0 &&
                (f.move == Move::Dash ||
                    f.move == Move::Run ||
                    p.dashChord) &&
                f.grounded;

            const bool canAttackFromDash =
                p.button == Button::A &&
                f.move == Move::Dash &&
                f.grounded;

            if (!CanAct(f) && !canAttackFromDash)
                return;

            if (p.button == Button::Jump)
            {
                if (!f.grounded)
                    return;

                Face(f, other);

                f.vy = tuning.jumpSpeed;
                f.grounded = false;
                f.crouched = false;

                End(f);
                f.pending = {};
                return;
            }

            Move move = Move::Free;
            bool wantBoost = false;

            if (p.button == Button::A)
            {
                if (dashA)
                {
                    move = Move::Headbutt;
                    Face(f, other);
                }
                else if (p.y > 0)
                {
                    move = Move::Uppercut;
                    wantBoost = true;
                }
                else if (p.y < 0)
                {
                    move = f.grounded ?
                        Move::LowSwipe : Move::DownKick;
                }
                else if (!f.grounded)
                {
                    move = f.vy > 0 ?
                        Move::Uppercut : Move::DownKick;
                }
                else
                {
                    move = f.held.x ?
                        Move::Swat : Move::Jab;
                }
            }
            else
            {
                if (p.y > 0)
                {
                    if (f.crashUsed)
                        return;

                    move = Move::Crash;
                    f.crashUsed = true;
                }
                else if (p.y < 0)
                {
                    move = f.grounded ?
                        Move::Burrow : Move::BurrowDive;
                }
                else if (p.x)
                {
                    move = Move::Spin;
                    Face(f, other);
                }
                else
                {
                    move = Move::Charge;
                }
            }

            Start(f, move);
            f.pending = {};

            if (move == Move::Uppercut &&
                wantBoost &&
                !f.upperUsed)
            {
                f.upperUsed = true;
                f.boost = true;
            }

            if (move == Move::DownKick && p.y < 0)
            {
                f.vy = std::min(
                    f.vy,
                    -0.25f * tuning.jumpSpeed);
            }

            if (move == Move::Burrow)
            {
                f.grounded = true;
                f.height = 0;
                f.vy = 0;
            }
        }

        void TickFighter(int i)
        {
            auto& f = fighters[i];
            const auto& other = fighters[1 - i];

            f.freshLanding = false;

            // Finish completed moves before checking buffered input.
            if (IsNormal(f.move) ||
                f.move == Move::Punch ||
                f.move == Move::Spin ||
                f.move == Move::Burrow)
            {
                if (f.tick >= Data(f.move, f.charge).Total())
                    End(f);
            }

            if (f.move == Move::Crash &&
                f.phase == 3 &&
                f.phaseTick >= 36)
            {
                End(f);
            }

            if (f.move == Move::Dash &&
                f.tick >= tuning.dashTicks)
            {
                const int direction = f.dashDirection;
                const bool forward = direction == f.facing;

                End(f);

                if (forward && f.held.x == direction)
                    f.move = Move::Run;
            }

            if (f.move == Move::Run &&
                (f.held.x != f.facing || !f.grounded))
            {
                End(f);
            }

            const bool stunned = f.stun > 0;
            const bool blocked = f.blockstun > 0;
            const bool landingLocked = f.landing > 0;

            if (stunned)
                --f.stun;

            if (blocked)
                --f.blockstun;

            if (landingLocked)
                --f.landing;

            const bool free =
                (f.move == Move::Free ||
                    f.move == Move::Run ||
                    f.move == Move::Dash) &&
                !stunned;

            if (f.grounded && free && !blocked)
                Face(f, other);

            bool newGuard =
                blocked ||
                (f.held.block &&
                    free &&
                    (!landingLocked ||
                        f.landing < tuning.emptyLanding));

            // Empty landing permits guard.
            // Attack landing recovery does not.
            if (landingLocked && f.airAttack)
                newGuard = blocked;

            if (newGuard &&
                !f.guarding &&
                !f.grounded &&
                f.vy > 0)
            {
                f.vy = 0;
            }

            f.guarding = newGuard;

            if (f.held.block)
                f.pending = {};

            if (newGuard && f.move != Move::Free)
                End(f);

            if (!stunned &&
                !blocked &&
                !landingLocked &&
                !newGuard)
            {
                BeginPending(f, other);
            }

            f.crouched =
                (f.grounded &&
                    f.held.y < 0 &&
                    (f.move == Move::Free || f.guarding)) ||
                f.move == Move::LowSwipe;

            ++f.animTicks;

            if (f.move != Move::Free)
                ++f.tick;

            float vx = 0;

            if (!stunned && !blocked && !f.guarding)
            {
                const bool canSteer =
                    f.move == Move::Free ||
                    f.move == Move::Run ||
                    f.move == Move::Swat ||
                    (!f.grounded &&
                        (f.move == Move::Uppercut ||
                            f.move == Move::DownKick));

                if (canSteer && !f.crouched && !landingLocked)
                {
                    float speed = tuning.walk *
                        (f.grounded ? 1.0f : tuning.airRatio);

                    if (f.held.x != f.facing)
                        speed *= tuning.backwardRatio;

                    if (f.move == Move::Run)
                        speed = tuning.walk * tuning.runRatio;

                    vx = f.held.x * speed;
                }

                if (f.move == Move::Dash)
                {
                    const float directionRatio =
                        f.dashDirection == f.facing ?
                        1.0f : tuning.backwardRatio;

                    vx = f.dashDirection *
                        tuning.walk *
                        tuning.dashRatio *
                        directionRatio;
                }

                if (f.move == Move::Headbutt &&
                    f.tick >= 6 &&
                    f.tick <= 18)
                {
                    vx = f.facing * tuning.headbuttSpeed;
                }

                if (f.move == Move::Spin &&
                    f.tick >= 10 &&
                    f.tick <= 30)
                {
                    vx = f.facing * tuning.spinSpeed;
                }
            }

            if (f.move == Move::Uppercut &&
                f.tick == 8 &&
                f.boost)
            {
                f.vy = std::min(
                    std::max(f.vy, 0.0f) +
                    0.5f * tuning.jumpSpeed,
                    1.25f * tuning.jumpSpeed);

                f.grounded = false;
                f.airAttack = true;
            }

            if (f.move == Move::Charge)
            {
                if (!f.held.b ||
                    f.chargeTicks >= tuning.maxChargeTicks)
                {
                    f.charge =
                        static_cast<float>(f.chargeTicks) /
                        tuning.maxChargeTicks;

                    Start(f, Move::Punch);
                    f.tick = 1;
                }
                else
                {
                    ++f.chargeTicks;
                }
            }

            if (f.move == Move::BurrowDive && f.tick > 8)
                f.vy = -tuning.diveSpeed;

            if (f.move == Move::Burrow)
            {
                f.height = 0;
                f.vy = 0;
                f.knockVx = 0;
                f.grounded = true;

                if (f.tick == 19)
                {
                    f.burrowStartX = f.x;

                    f.targetX = std::clamp(
                        other.x,
                        arena.left,
                        arena.right);
                }

                f.underground =
                    f.tick >= 19 && f.tick <= 54;

                if (f.tick >= 19 && f.tick <= 36)
                {
                    f.x = f.burrowStartX +
                        (f.targetX - f.burrowStartX) *
                        static_cast<float>(f.tick - 18) / 18.0f;
                }

                vx = 0;
            }

            bool apex = false;

            if (f.move == Move::Crash)
            {
                if (f.phase == 0 && f.tick == 12)
                {
                    f.vy = tuning.jumpSpeed * std::sqrt(1.6f);
                    f.grounded = false;
                }

                if (f.phase == 0 &&
                    f.tick > 12 &&
                    f.vy <= 0)
                {
                    f.phase = 1;
                    f.phaseTick = 0;
                    f.vy = 0;
                }

                if (f.phase == 1)
                {
                    apex = true;

                    if (++f.phaseTick > 6)
                    {
                        f.phase = 2;
                        f.phaseTick = 0;
                        apex = false;
                        f.vy = -tuning.diveSpeed;
                    }
                }
                else if (f.phase == 2)
                {
                    f.vy = -tuning.diveSpeed;
                }
                else if (f.phase == 3)
                {
                    ++f.phaseTick;
                }
            }

            f.actualVx = vx + f.knockVx;

            f.x = std::clamp(
                f.x + f.actualVx * StepSeconds,
                arena.left,
                arena.right);

            f.knockVx *= f.grounded ? 0.85f : 0.98f;

            if (!f.grounded && !apex)
            {
                f.height +=
                    f.vy * StepSeconds -
                    0.5f * tuning.gravity *
                    StepSeconds * StepSeconds;

                f.vy -= tuning.gravity * StepSeconds;

                if (f.height <= 0.0001f)
                {
                    f.height = 0;
                    f.vy = 0;
                    f.grounded = true;
                    f.freshLanding = true;
                    f.upperUsed = false;
                    f.crashUsed = false;

                    if (f.move == Move::BurrowDive)
                    {
                        Start(f, Move::Burrow);
                    }
                    else if (f.move == Move::Crash &&
                        f.phase == 2)
                    {
                        // The descent hit remains active this tick.
                        // Step() starts landing recovery afterward.
                    }
                    else if (IsNormal(f.move))
                    {
                        f.landedAttack = true;
                        f.landing = tuning.attackLanding;
                        f.airAttack = true;
                    }
                    else if (f.move == Move::Spin ||
                        f.move == Move::Charge ||
                        f.move == Move::Punch)
                    {
                        f.landing = tuning.specialLanding;
                        f.airAttack = true;
                    }
                    else
                    {
                        f.landing = tuning.emptyLanding;
                        f.airAttack = false;
                    }
                }
            }
        }

        void ResolvePush(
            const std::array<Fighter, 2>& old,
            const std::array<bool, 2>& frozen)
        {
            auto& a = fighters[0];
            auto& b = fighters[1];

            if (a.underground || b.underground)
                return;

            const Box pa = Push(a);
            const Box pb = Push(b);

            const bool vertical =
                pa.y < pb.y + pb.h &&
                pa.y + pa.h > pb.y;

            if (!vertical)
                return;

            const float minimum = tuning.pushWidth;

            const bool crossed =
                (old[0].x < old[1].x && a.x > b.x) ||
                (old[0].x > old[1].x && a.x < b.x);

            if (std::abs(a.x - b.x) >= minimum && !crossed)
                return;

            const int order =
                old[0].x <= old[1].x ? 1 : -1;

            // Restore collision at a safe adjacent emergence point.
            if (a.move == Move::Burrow && a.tick == 55)
            {
                const float left = b.x - minimum;
                const float right = b.x + minimum;

                const bool chooseLeft =
                    left >= arena.left &&
                    (right > arena.right ||
                        std::abs(a.x - left) <=
                        std::abs(a.x - right));

                a.x = chooseLeft ? left : right;
                a.x = std::clamp(a.x, arena.left, arena.right);
                return;
            }

            if (b.move == Move::Burrow && b.tick == 55)
            {
                const float left = a.x - minimum;
                const float right = a.x + minimum;

                const bool chooseRight =
                    right <= arena.right &&
                    (left < arena.left ||
                        std::abs(b.x - right) <=
                        std::abs(b.x - left));

                b.x = chooseRight ? right : left;
                b.x = std::clamp(b.x, arena.left, arena.right);
                return;
            }

            float overlap = minimum - order * (b.x - a.x);

            if (overlap <= 0)
                return;

            if (frozen[0] && !frozen[1])
            {
                b.x += order * overlap;
            }
            else if (frozen[1] && !frozen[0])
            {
                a.x -= order * overlap;
            }
            else
            {
                a.x -= order * overlap * 0.5f;
                b.x += order * overlap * 0.5f;
            }

            a.x = std::clamp(a.x, arena.left, arena.right);
            b.x = std::clamp(b.x, arena.left, arena.right);

            overlap = minimum - order * (b.x - a.x);

            if (overlap > 0)
            {
                if (a.x <= arena.left || a.x >= arena.right)
                {
                    b.x = std::clamp(
                        a.x + order * minimum,
                        arena.left,
                        arena.right);
                }
                else
                {
                    a.x = std::clamp(
                        b.x - order * minimum,
                        arena.left,
                        arena.right);
                }
            }
        }

        void ApplyHit(int victim, const Strike& s, bool blocked)
        {
            auto& f = fighters[victim];

            const float scale = blocked ? tuning.chip : 1.0f;

            f.health = std::max(
                0.0f,
                f.health - s.data.damage * scale);

            f.knockVx =
                s.direction * s.data.knockX * scale;

            if (!blocked)
            {
                End(f);

                f.pending = {};
                f.guarding = false;
                f.crouched = false;
                f.stun = s.data.hitstun;
                f.blockstun = 0;
            }
            else
            {
                f.blockstun = s.data.blockstun;
                f.guarding = true;
            }

            const float up = s.data.knockUp * scale;

            if (up > 0 || !f.grounded)
            {
                f.vy = blocked ? f.vy + up : up;
                f.grounded = false;
            }

            f.hitstop = std::max(
                f.hitstop,
                s.data.hitstop);
        }
    };
}