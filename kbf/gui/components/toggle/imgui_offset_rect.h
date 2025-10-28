#pragma once

#include <kbf/cimgui/cimgui_funcs.hpp>

// Helper: ImOffsetRect A set of offsets to apply to an ImRect.
struct ImOffsetRect
{
    union
    {
        float Offsets[4];

        struct  
        {
            float       Top;
            float       Left;
            float       Bottom;
            float       Right;
        };
    };

    constexpr ImOffsetRect()                                                    : Top(0.0f), Left(0.0f), Bottom(0.0f), Right(0.0f)                  {}
    constexpr ImOffsetRect(const ImVec2& topLeft, const ImVec2& bottomRight)    : ImOffsetRect(topLeft.y, topLeft.x, bottomRight.y, bottomRight.x)  {}
    constexpr ImOffsetRect(const ImVec4& v)                                     : ImOffsetRect(v.x, v.y, v.z, v.w)                                  {}
    constexpr ImOffsetRect(float top, float left, float bottom, float right)    : Top(top), Left(left), Bottom(bottom), Right(right)                {}
    constexpr ImOffsetRect(float all)                                           : Top(all), Left(all), Bottom(all), Right(all)                      {}

    ImVec2      GetSize() const { return ImVec2(Left + Right, Top + Bottom); }
    float       GetWidth() const { return Left + Right; }
    float       GetHeight() const { return Top + Bottom; }
    float       GetAverage() const { return (Top + Left + Bottom + Right) / 4.0f; }
    ImOffsetRect MirrorHorizontally() const { return ImOffsetRect(Top, Right, Bottom, Left); }
    ImOffsetRect MirrorVertically() const { return ImOffsetRect(Bottom, Left, Top, Right); }
    ImOffsetRect Mirror() const { return ImOffsetRect(Bottom, Right, Top, Left); }
};

// Helpers: ImOffsetRect operators
static inline ImOffsetRect operator+(const ImOffsetRect& lhs, const ImOffsetRect& rhs) { return ImOffsetRect(lhs.Top + rhs.Top, lhs.Left + rhs.Left, lhs.Bottom + rhs.Bottom, lhs.Right + rhs.Right); }
static inline ImOffsetRect operator-(const ImOffsetRect& lhs, const ImOffsetRect& rhs) { return ImOffsetRect(lhs.Top - rhs.Top, lhs.Left - rhs.Left, lhs.Bottom - rhs.Bottom, lhs.Right - rhs.Right); }
static inline ImOffsetRect operator*(const ImOffsetRect& lhs, const ImOffsetRect& rhs) { return ImOffsetRect(lhs.Top * rhs.Top, lhs.Left * rhs.Left, lhs.Bottom * rhs.Bottom, lhs.Right * rhs.Right); }
