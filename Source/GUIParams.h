#pragma once

namespace gui
{
    static constexpr float SensitiveDrag = .2f;
    static constexpr float WheelDefaultSpeed = .02f;
    static constexpr float WheelInertia = .9f;

    static constexpr float LockAlpha = .4f;

    struct Parametr :
        public Comp
    {
        Parametr(Utils& u, PID _pID) :
            Comp(u, param::toTooltip(_pID)),
            param(*u.getParam(_pID)),
            valNorm(param.getValue()),
            locked(param.isLocked())
        {}

        PID getPID() const noexcept { return param.id; }

    protected:
        Param& param;
        float valNorm;
        bool locked;
    };
}