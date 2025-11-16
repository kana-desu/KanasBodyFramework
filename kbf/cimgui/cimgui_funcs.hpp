#pragma once

// I am incredibly ashamed, yet perversely proud, of whatever the fuck is happening here - Kana.
//  Should the morbid urge seize you to port every ImGui function here, make a script - or risk the slow decay of your sanity

#include <kbf/debug/log_string.hpp>
#include <cassert>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <kbf/cimgui/cimgui.h>

// Define function pointer types and their actual function pointers
// Note: Name should be the name of the cimgui function WITHOUT the ig prefix.
#define IM_FUNC_SIG(name, ret_type, ...)               \
    typedef ret_type (*ig##name##_t)(__VA_ARGS__);     \
    inline ig##name##_t ptr_##name = nullptr

#define IM_MEMBER_FUNC_SIG(name, ret_type, ...)        \
    typedef ret_type (*Im##name##_t)(__VA_ARGS__);     \
    inline Im##name##_t ptr_##name = nullptr

#define IM_FUNC_SIG_EXPLICIT(name, ret_type, ...)    \
    typedef ret_type (*##name##_t)(__VA_ARGS__);     \
    inline name##_t ptr_##name = nullptr

// Wrapper allowing for default arguments
// TODO: Would LOVE to not have to specify args_call, but I don't think it's possible.
// Optionally, separate name for func ptr
#define IM_FUNC(name, ret_type, args_decl, args_call) \
    inline ret_type name args_decl {                  \
        return ptr_##name args_call ;                 \
    }                                                 \

#define IM_FUNC_OVERLOAD(name, ptrName, ret_type, args_decl, args_call) \
    inline ret_type name args_decl {                                    \
        return ptr_##ptrName args_call ;                                \
    }                                                                   \

// Read from DLL, with error checking
#define IM_GET_FUNC(name, ...)                                                                    \
    ptr_##name = (ig##name##_t)GetProcAddress(dinput8, "ig" #name ##__VA_ARGS__);                 \
    if (!ptr_##name) MessageBoxA(0, LOG_STRING("Failed to get CImGui function: ig" #name), "Error", 0)

#define IM_GET_MEMBER_FUNC(name, ...)                                                                    \
    ptr_##name = (Im##name##_t)GetProcAddress(dinput8, "Im" #name ##__VA_ARGS__);                 \
    if (!ptr_##name) MessageBoxA(0, LOG_STRING("Failed to get CImGui function: Im" #name), "Error", 0)

#define IM_GET_FUNC_EXPLICIT(name, funcName, ...)                                         \
    ptr_##name = (##name##_t)GetProcAddress(dinput8, #funcName ##__VA_ARGS__);              \
    if (!ptr_##name) MessageBoxA(0, LOG_STRING("Failed to get CImGui function:" #funcName), "Error", 0)

#define ASSERT_LOADED(name) \
    assert(ptr_##name != nullptr && "CImGui function not loaded: (Im/ig)" #name)

#include <Windows.h>
#include <filesystem>

// Util macros moved over from ImGui
#define IM_ARRAYSIZE(_ARR)          ((int)(sizeof(_ARR) / sizeof(*(_ARR)))) 
#define IM_ASSERT(_EXPR)            assert(_EXPR)                               // You can override the default assert handler by editing imconfig.h

// Data Type Operators
//inline bool operator<(const ImVec2 & a, const ImVec2 & b) { return (a.x < b.x) || (a.x == b.x && a.y < b.y); }
//inline bool operator>(const ImVec2 & a, const ImVec2 & b) { return (a.x > b.x) || (a.x == b.x && a.y > b.y); }
//inline bool operator<=(const ImVec2 & a, const ImVec2 & b) { return (a.x < b.x) || (a.x == b.x && a.y <= b.y); }
//inline bool operator>=(const ImVec2 & a, const ImVec2 & b) { return (a.x > b.x) || (a.x == b.x && a.y >= b.y); }
//inline bool operator<(const ImVec4 & a, const ImVec4 & b) { return (a.x < b.x) || (a.x == b.x && ((a.y < b.y) || (a.y == b.y && ((a.z < b.z) || (a.z == b.z && a.w < b.w))))); }
//inline bool operator>(const ImVec4 & a, const ImVec4 & b) { return (a.x > b.x) || (a.x == b.x && ((a.y > b.y) || (a.y == b.y && ((a.z > b.z) || (a.z == b.z && a.w > b.w))))); }
//inline bool operator<=(const ImVec4 & a, const ImVec4 & b) { return (a.x < b.x) || (a.x == b.x && ((a.y < b.y) || (a.y == b.y && ((a.z < b.z) || (a.z == b.z && a.w <= b.w))))); }
//inline bool operator>=(const ImVec4 & a, const ImVec4 & b) { return (a.x > b.x) || (a.x == b.x && ((a.y > b.y) || (a.y == b.y && ((a.z > b.z) || (a.z == b.z && a.w >= b.w))))); }

inline ImVec2  operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
inline ImVec2  operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
inline ImVec2  operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
inline ImVec2  operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
inline ImVec2  operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
inline ImVec2  operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
inline ImVec2  operator-(const ImVec2& lhs) { return ImVec2(-lhs.x, -lhs.y); }
inline ImVec2& operator*=(ImVec2& lhs, const float rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
inline ImVec2& operator/=(ImVec2& lhs, const float rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
inline ImVec2& operator*=(ImVec2& lhs, const ImVec2& rhs) { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
inline ImVec2& operator/=(ImVec2& lhs, const ImVec2& rhs) { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
inline bool    operator==(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool    operator!=(const ImVec2& lhs, const ImVec2& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }
inline ImVec4  operator+(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
inline ImVec4  operator-(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
inline ImVec4  operator*(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w); }
inline bool    operator==(const ImVec4& lhs, const ImVec4& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w; }
inline bool    operator!=(const ImVec4& lhs, const ImVec4& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w; }

// Math
template<typename T> inline T ImMin(T lhs, T rhs) { return lhs < rhs ? lhs : rhs; }
template<typename T> inline T ImMax(T lhs, T rhs) { return lhs >= rhs ? lhs : rhs; }
template<typename T> inline T ImClamp(T v, T mn, T mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }
template<typename T> inline T ImLerp(T a, T b, float t) { return (T)(a + (b - a) * t); }

inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y); }
inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x >= rhs.x ? lhs.x : rhs.x, lhs.y >= rhs.y ? lhs.y : rhs.y); }
inline ImVec2 ImClamp(const ImVec2& v, const ImVec2& mn, const ImVec2& mx) { return ImVec2((v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y); }
inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, float t) { return ImVec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t); }
inline ImVec2 ImLerp(const ImVec2& a, const ImVec2& b, const ImVec2& t) { return ImVec2(a.x + (b.x - a.x) * t.x, a.y + (b.y - a.y) * t.y); }
inline ImVec4 ImLerp(const ImVec4& a, const ImVec4& b, float t) { return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t); }
inline float  ImSaturate(float f) { return (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f; }
inline float  ImLengthSqr(const ImVec2& lhs) { return (lhs.x * lhs.x) + (lhs.y * lhs.y); }
inline float  ImLengthSqr(const ImVec4& lhs) { return (lhs.x * lhs.x) + (lhs.y * lhs.y) + (lhs.z * lhs.z) + (lhs.w * lhs.w); }
//inline float  ImInvLength(const ImVec2& lhs, float fail_value) { float d = (lhs.x * lhs.x) + (lhs.y * lhs.y); if (d > 0.0f) return ImRsqrt(d); return fail_value; }
inline float  ImTrunc(float f) { return (float)(int)(f); }
inline ImVec2 ImTrunc(const ImVec2& v) { return ImVec2((float)(int)(v.x), (float)(int)(v.y)); }
inline float  ImFloor(float f) { return (float)((f >= 0 || (float)(int)f == f) ? (int)f : (int)f - 1); } // Decent replacement for floorf()
inline ImVec2 ImFloor(const ImVec2& v) { return ImVec2(ImFloor(v.x), ImFloor(v.y)); }
inline int    ImModPositive(int a, int b) { return (a + b) % b; }
inline float  ImDot(const ImVec2& a, const ImVec2& b) { return a.x * b.x + a.y * b.y; }
inline ImVec2 ImRotate(const ImVec2& v, float cos_a, float sin_a) { return ImVec2(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a); }
inline float  ImLinearSweep(float current, float target, float speed) { if (current < target) return ImMin(current + speed, target); if (current > target) return ImMax(current - speed, target); return current; }
inline float  ImLinearRemapClamp(float s0, float s1, float d0, float d1, float x) { return ImSaturate((x - s0) / (s1 - s0)) * (d1 - d0) + d0; }
inline ImVec2 ImMul(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
inline bool   ImIsFloatAboveGuaranteedIntegerPrecision(float f) { return f <= -16777216 || f >= 16777216; }
inline float  ImExponentialMovingAverage(float avg, float sample, int n) { avg -= avg / n; avg += sample / n; return avg; }


// Utils for already-defined structs
struct ImRectUtils
{
    static inline ImVec2 GetCenter(const ImRect& a) { return ImVec2((a.Min.x + a.Max.x) * 0.5f, (a.Min.y + a.Max.y) * 0.5f); }
    static inline ImVec2 GetSize(const ImRect& a) { return ImVec2(a.Max.x - a.Min.x, a.Max.y - a.Min.y); }
    static inline float  GetWidth(const ImRect& a) { return a.Max.x - a.Min.x; }
    static inline float  GetHeight(const ImRect& a) { return a.Max.y - a.Min.y; }
    static inline float  GetArea(const ImRect& a) { return (a.Max.x - a.Min.x) * (a.Max.y - a.Min.y); }
    static inline ImVec2 GetTL(const ImRect& a) { return a.Min; }
    static inline ImVec2 GetTR(const ImRect& a) { return ImVec2(a.Max.x, a.Min.y); }
    static inline ImVec2 GetBL(const ImRect& a) { return ImVec2(a.Min.x, a.Max.y); }
    static inline ImVec2 GetBR(const ImRect& a) { return a.Max; }

    static inline bool Contains(const ImRect& a, const ImVec2& p) { return p.x >= a.Min.x && p.y >= a.Min.y && p.x < a.Max.x && p.y < a.Max.y; }
    static inline bool Contains(const ImRect& a, const ImRect& b) { return b.Min.x >= a.Min.x && b.Min.y >= a.Min.y && b.Max.x <= a.Max.x && b.Max.y <= a.Max.y; }
    static inline bool ContainsWithPad(const ImRect& a, const ImVec2& p, const ImVec2& pad) { return p.x >= a.Min.x - pad.x && p.y >= a.Min.y - pad.y && p.x < a.Max.x + pad.x && p.y < a.Max.y + pad.y; }
    static inline bool Overlaps(const ImRect& a, const ImRect& b) { return b.Min.y < a.Max.y && b.Max.y > a.Min.y && b.Min.x < a.Max.x && b.Max.x > a.Min.x; }

    static inline void Add(ImRect& a, const ImVec2& p) { if (a.Min.x > p.x) a.Min.x = p.x; if (a.Min.y > p.y) a.Min.y = p.y; if (a.Max.x < p.x) a.Max.x = p.x; if (a.Max.y < p.y) a.Max.y = p.y; }
    static inline void Add(ImRect& a, const ImRect& b) { if (a.Min.x > b.Min.x) a.Min.x = b.Min.x; if (a.Min.y > b.Min.y) a.Min.y = b.Min.y; if (a.Max.x < b.Max.x) a.Max.x = b.Max.x; if (a.Max.y < b.Max.y) a.Max.y = b.Max.y; }
    static inline void Expand(ImRect& a, float amount) { a.Min.x -= amount; a.Min.y -= amount; a.Max.x += amount; a.Max.y += amount; }
    static inline void Expand(ImRect& a, const ImVec2& amount) { a.Min.x -= amount.x; a.Min.y -= amount.y; a.Max.x += amount.x; a.Max.y += amount.y; }
    static inline void Translate(ImRect& a, const ImVec2& d) { a.Min.x += d.x; a.Min.y += d.y; a.Max.x += d.x; a.Max.y += d.y; }
    static inline void TranslateX(ImRect& a, float dx) { a.Min.x += dx; a.Max.x += dx; }
    static inline void TranslateY(ImRect& a, float dy) { a.Min.y += dy; a.Max.y += dy; }
    static inline void ClipWith(ImRect& a, const ImRect& b) { a.Min = ImMax(a.Min, b.Min); a.Max = ImMin(a.Max, b.Max); }
    static inline void ClipWithFull(ImRect& a, const ImRect& b) { a.Min = ImClamp(a.Min, b.Min, b.Max); a.Max = ImClamp(a.Max, b.Min, b.Max); }
    //static inline void Floor(ImRect& a) { a.Min.x = IM_TRUNC(a.Min.x); a.Min.y = IM_TRUNC(a.Min.y); a.Max.x = IM_TRUNC(a.Max.x); a.Max.y = IM_TRUNC(a.Max.y); }    static inline bool IsInverted(const ImRect& a) { return a.Min.x > a.Max.x || a.Min.y > a.Max.y; }
    static inline ImVec4 ToVec4(const ImRect& a) { return ImVec4(a.Min.x, a.Min.y, a.Max.x, a.Max.y); }
};

// Default arguments and signatures so that we only have to manually specify them again if we want default arguments.
#define SIG_Begin               const char* name, bool* p_open, ImGuiWindowFlags flags
#define ARG_Begin               name, p_open, flags
#define SIG_Text                const char* fmt, ...
#define ARG_Text                fmt
#define SIG_TreeNode_Str        const char* label
#define ARG_TreeNode_Str        label
#define SIG_BeginTable          const char* str_id, int columns, ImGuiTableFlags flags, const ImVec2 outer_size, float inner_width 
#define ARG_BeginTable          str_id, columns, flags, outer_size, inner_width
#define SIG_TableHeader         const char* label
#define ARG_TableHeader         label
#define SIG_TableNextRow        ImGuiTableRowFlags row_flags, float min_row_height
#define ARG_TableNextRow        row_flags, min_row_height
#define SIG_PushStyleVar_Float  ImGuiStyleVar idx, float val
#define ARG_PushStyleVar_Float  idx, val
#define SIG_PushStyleVar_Vec2   ImGuiStyleVar idx, const ImVec2 val 
#define ARG_PushStyleVar_Vec2   idx, val
#define SIG_CalcTextSizeByArg   ImVec2* pOut, const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width
#define ARG_CalcTextSizeByArg   pOut, text, text_end, hide_text_after_double_hash, wrap_width                                       
#define SIG_SetCursorPosX       float local_x
#define ARG_SetCursorPosX       local_x
#define SIG_GetColumnWidth      int column_index
#define ARG_GetColumnWidth      column_index
#define SIG_PushTextWrapPos     float wrap_local_pos_x
#define ARG_PushTextWrapPos     wrap_local_pos_x
#define SIG_TextWrapped         const char* fmt, ...
#define ARG_TextWrapped         fmt
#define SIG_Button		        const char* label, const ImVec2 size_arg
#define ARG_Button		        label, size_arg
#define SIG_PushStyleColor_U32  ImGuiCol idx, ImU32 col
#define ARG_PushStyleColor_U32  idx, col
#define SIG_PushStyleColor_Vec4 ImGuiCol idx, const ImVec4 col        
#define ARG_PushStyleColor_Vec4 idx, col                              
#define SIG_SameLine            float offset_from_start_x, float spacing
#define ARG_SameLine            offset_from_start_x, spacing
#define SIG_PopStyleColor       int count
#define ARG_PopStyleColor       count
#define SIG_PopStyleVar         int count
#define ARG_PopStyleVar         count
#define SIG_SetNextWindowSize   const ImVec2 size, ImGuiCond cond        
#define ARG_SetNextWindowSize   size, cond                               
#define SIG_BeginChild_Str      const char* str_id, const ImVec2 size, ImGuiChildFlags child_flags, ImGuiWindowFlags flags      
#define ARG_BeginChild_Str      str_id, size, child_flags, flags                                                                
#define SIG_BeginChild_ID       ImGuiID id, const ImVec2 size, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags       
#define ARG_BeginChild_ID       id, size, child_flags, window_flags                                                             
#define SIG_GetStyleColorVec4   ImGuiCol idx
#define ARG_GetStyleColorVec4   idx
#define SIG_Selectable_Bool     const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2 size         
#define ARG_Selectable_Bool     label, selected, flags, size                                                            
#define SIG_Selectable_BoolPtr  const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2 size      
#define ARG_Selectable_BoolPtr  label, p_selected, flags, size                                                          
#define SIG_SetItemTooltip      const char* fmt, ...
#define ARG_SetItemTooltip      fmt
#define SIG_GetColorU32_U32     ImU32 col, float alpha_mul
#define ARG_GetColorU32_U32     col, alpha_mul
#define SIG_GetColorU32_Col     ImGuiCol idx, float alpha_mul
#define ARG_GetColorU32_Col     idx, alpha_mul
#define SIG_GetColorU32_Vec4    const ImVec4 col     
#define ARG_GetColorU32_Vec4    col                  
#define SIG_PushFont            ImFont* font, float font_size_base_unscaled
#define ARG_PushFont            font, font_size_base_unscaled              
#define SIG_PushItemWidth       float item_width
#define ARG_PushItemWidth       item_width
#define SIG_InputTextWithHint   const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data
#define ARG_InputTextWithHint   label, hint, buf, buf_size, flags, callback, user_data
#define SIG_SetWindowSize_Vec2  const ImVec2 size, ImGuiCond cond
#define ARG_SetWindowSize_Vec2  size, cond
#define SIG_SetWindowSize_Str   const char* name, const ImVec2 size, ImGuiCond cond
#define ARG_SetWindowSize_Str   name, size, cond
#define SIG_SetWindowSize_WindowPtr ImGuiWindow* window, const ImVec2 size, ImGuiCond cond
#define ARG_SetWindowSize_WindowPtr window, size, cond
#define SIG_BeginTabBar         const char* str_id, const ImGuiTabBarFlags flags
#define ARG_BeginTabBar         str_id, flags
#define SIG_BeginTabItem        const char* label, bool* p_open, ImGuiTabItemFlags flags
#define ARG_BeginTabItem        label, p_open, flags
#define SIG_Checkbox		    const char* label, bool* v
#define ARG_Checkbox		    label, v
#define SIG_Dummy			    const ImVec2 size  
#define ARG_Dummy			    size               
#define SIG_TextUnformatted     const char* text, const char* text_end
#define ARG_TextUnformatted     text, text_end
#define SIG_SetScrollHereY      float center_y_ratio
#define ARG_SetScrollHereY      center_y_ratio
#define SIG_InputText           const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data
#define ARG_InputText           label, buf, buf_size, flags, callback, user_data
#define SIG_BeginCombo          const char* label, const char* preview_value, ImGuiComboFlags flags
#define ARG_BeginCombo          label, preview_value, flags
#define SIG_BeginDisabled       bool disabled
#define ARG_BeginDisabled       disabled
#define SIG_SetCursorPosY       float local_y
#define ARG_SetCursorPosY       local_y
#define SIG_SetNextItemWidth    float item_width
#define ARG_SetNextItemWidth    item_width
#define SIG_SexNextWindowPos   const ImVec2 pos,ImGuiCond cond,const ImVec2 pivot
#define ARG_SetNextWindowPos    pos, cond, pivot
#define SIG_TableSetupColumn    const char* label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImGuiID user_id
#define ARG_TableSetupColumn    label, flags, init_width_or_weight, user_id                                                
#define SIG_TableSetupScrollFreeze int cols, int rows
#define ARG_TableSetupScrollFreeze cols, rows
#define SIG_DragFloat           const char* label,float* v,float v_speed,float v_min,float v_max,const char* format,ImGuiSliderFlags flags
#define ARG_DragFloat           label, v, v_speed, v_min, v_max, format, flags
#define SIG_InvisibleButton     const char* str_id, const ImVec2 size, ImGuiButtonFlags flags
#define ARG_InvisibleButton     str_id, size, flags
#define SIG_IsItemHovered       ImGuiHoveredFlags flags
#define ARG_IsItemHovered       flags
#define SIG_Indent              float indent_w
#define ARG_Indent              indent_w
#define SIG_Unindent            float indent_w
#define ARG_Unindent            indent_w
#define SIG_SeparatorText       const char* label
#define ARG_SeparatorText       label            
#define SIG_CollapsingHeader_TreeNodeFlags const char* label, ImGuiTreeNodeFlags flags
#define ARG_CollapsingHeader_TreeNodeFlags label, flags
#define SIG_CollapsingHeader_BoolPtr       const char* label, bool* p_visible, ImGuiTreeNodeFlags flags
#define ARG_CollapsingHeader_BoolPtr       label, p_visible, flags
#define SIG_SetTooltip		    const char* fmt, ...
#define ARG_SetTooltip		    fmt
#define SIG_IsItemClicked       ImGuiMouseButton mouse_button
#define ARG_IsItemClicked       mouse_button
#define SIG_IsMouseHoveringRect const ImVec2 r_min, const ImVec2 r_max, bool clip
#define ARG_IsMouseHoveringRect r_min, r_max, clip
#define SIG_ProgressBar         float fraction, const ImVec2 size_arg, const char* overlay
#define ARG_ProgressBar         fraction, size_arg, overlay
#define SIG_GetIO_ContextPtr    ImGuiContext* ctx
#define ARG_GetIO_ContextPtr    ctx
#define SIG_IsMouseClicked_Bool ImGuiMouseButton button, bool repeat
#define ARG_IsMouseClicked_Bool button, repeat
#define SIG_IsWindowHovered     ImGuiHoveredFlags flags
#define ARG_IsWindowHovered     flags
#define SIG_IsMouseReleased_Nil ImGuiMouseButton button
#define ARG_IsMouseReleased_Nil button
#define SIG_ColorConvertRGBtoHSV float r, float g, float b, float* out_h, float* out_s, float* out_v
#define ARG_ColorConvertRGBtoHSV r, g, b, out_h, out_s, out_v
#define SIG_ColorConvertHSVtoRGB float h, float s, float v, float* out_r, float* out_g, float* out_b
#define ARG_ColorConvertHSVtoRGB h, s, v, out_r, out_g, out_b
#define SIG_IsKeyDown_Nil       ImGuiKey key
#define ARG_IsKeyDown_Nil       key
#define SIG_IsKeyPressed_Bool    ImGuiKey key, bool repeat
#define ARG_IsKeyPressed_Bool    key, repeat
#define SIG_VSliderFloat        const char* label, const ImVec2 size, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags
#define ARG_VSliderFloat        label, size, v, v_min, v_max, format, flags
#define SIG_TableSetColumnIndex int column_n
#define ARG_TableSetColumnIndex column_n
#define SIG_ArrowButton         const char* str_id, ImGuiDir dir
#define ARG_ArrowButton         str_id, dir
#define SIG_SetCursorPos        const ImVec2 local_pos
#define ARG_SetCursorPos        local_pos             
#define SIG_TableSetBgColor     ImGuiTableBgTarget target, ImU32 color, int column_n
#define ARG_TableSetBgColor     target, color, column_n
#define SIG_SliderInt           const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags
#define ARG_SliderInt           label, v, v_min, v_max, format, flags
#define SIG_MenuItem_Bool       const char* label, const char* shortcut, bool selected, bool enabled
#define ARG_MenuItem_Bool       label, shortcut, selected, enabled
#define SIG_MenuItem_BoolPtr    const char* label, const char* shortcut, bool* p_selected, bool enabled
#define ARG_MenuItem_BoolPtr    label, shortcut, p_selected, enabled
#define SIG_SetNextWindowSizeConstraints const ImVec2 size_min, const ImVec2 size_max, ImGuiSizeCallback custom_callback, void* custom_callback_data
#define ARG_SetNextWindowSizeConstraints size_min, size_max, custom_callback, custom_callback_data
#define SIG_GetCursorPosByArg          ImVec2* pOut 
#define ARG_GetCursorPosByArg          pOut         
#define SIG_GetContentRegionAvailByArg ImVec2* pOut
#define ARG_GetContentRegionAvailByArg pOut
#define SIG_GetWindowSizeByArg         ImVec2* pOut
#define ARG_GetWindowSizeByArg         pOut
#define SIG_GetCursorScreenPosByArg    ImVec2* pOut
#define ARG_GetCursorScreenPosByArg    pOut
#define SIG_GetItemRectMinByArg        ImVec2* pOut
#define ARG_GetItemRectMinByArg        pOut
#define SIG_StyleColorsClassicByArg    ImGuiStyle* dst
#define ARG_StyleColorsClassicByArg    dst
#define SIG_StyleColorsDarkByArg       ImGuiStyle* dst
#define ARG_StyleColorsDarkByArg       dst
#define SIG_GetMousePosByArg           ImVec2* pOut
#define ARG_GetMousePosByArg           pOut
#define SIG_TableGetCellBgRectByArg    ImRect* pOut, const ImGuiTable* table, int column_n
#define ARG_TableGetCellBgRectByArg    pOut, table, column_n
#define SIG_BeginPopupContextItem      const char* str_id, ImGuiPopupFlags popup_flags
#define ARG_BeginPopupContextItem      str_id, popup_flags
#define SIG_ShowStyleEditor     ImGuiStyle* ref
#define ARG_ShowStyleEditor     ref
#define SIG_ShowDemoWindow      bool* p_open
#define ARG_ShowDemoWindow      p_open
#define SIG_SetCursorScreenPos  const ImVec2 pos
#define ARG_SetCursorScreenPos  pos
#define SIG_ColorEdit4          const char* label, float col[4], ImGuiColorEditFlags flags
#define ARG_ColorEdit4          label, col, flags
#define SIG_DragFloat4          const char* label,float v[4],float v_speed,float v_min,float v_max,const char* format,ImGuiSliderFlags flags
#define ARG_DragFloat4          label, v, v_speed, v_min, v_max, format, flags
// Funcs for nested objects
#define SIG_FontAtlas_AddFontFromFileTTF  ImFontAtlas* self,const char* filename,float size_pixels,const ImFontConfig* font_cfg,const ImWchar* glyph_ranges
#define ARG_FontAtlas_AddFontFromFileTTF  self, filename, size_pixels, font_cfg, glyph_ranges
#define SIG_DrawList_AddText_Vec2    ImDrawList* self, const ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end
#define ARG_DrawList_AddText_Vec2    self, pos, col, text_begin, text_end
#define SIG_DrawList_AddText_FontPtr ImDrawList* self,ImFont* font,float font_size,const ImVec2 pos,ImU32 col,const char* text_begin,const char* text_end,float wrap_width,const ImVec4* cpu_fine_clip_rect
#define ARG_DrawList_AddText_FontPtr self, font, font_size, pos, col, text_begin, text_end, wrap_width, cpu_fine_clip_rect
#define SIG_DrawList_AddCircleFilled ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments
#define ARG_DrawList_AddCircleFilled self, center, radius, col, num_segments
#define SIG_DrawList_AddCircle       ImDrawList* self,const ImVec2 center,float radius,ImU32 col,int num_segments,float thickness
#define ARG_DrawList_AddCircle       self, center, radius, col, num_segments, thickness
#define SIG_DrawList_AddLine         ImDrawList* self,const ImVec2 p1,const ImVec2 p2,ImU32 col,float thickness
#define ARG_DrawList_AddLine         self, p1, p2, col, thickness
#define SIG_DrawList_AddRectFilled   ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawFlags flags
#define ARG_DrawList_AddRectFilled   self, p_min, p_max, col, rounding, flags
#define SIG_DrawList_AddRect         ImDrawList* self,const ImVec2 p_min,const ImVec2 p_max,ImU32 col,float rounding,ImDrawFlags flags,float thickness
#define ARG_DrawList_AddRect         self, p_min, p_max, col, rounding, flags, thickness
#define SIG_GuiViewport_GetCenter    ImVec2* out_center, ImGuiViewport* viewport
#define ARG_GuiViewport_GetCenter    out_center, viewport
#define SIG_GuiWindow_GetID_Str      ImGuiWindow* self,const char* str,const char* str_end
#define ARG_GuiWindow_GetID_Str      self, str, str_end
#define SIG_GuiWindow_GetID_Ptr      ImGuiWindow* self,const void* ptr
#define ARG_GuiWindow_GetID_Ptr      self, ptr
#define SIG_GuiWindow_GetID_Int      ImGuiWindow* self,int n
#define ARG_GuiWindow_GetID_Int      self, n

// (Probably) Internal functions
#define SIG_RenderBullet        ImDrawList* draw_list, const ImVec2 pos, ImU32 col
#define ARG_RenderBullet        draw_list, pos, col
#define SIG_LogRenderedText     const ImVec2* ref_pos, const char* text, const char* text_end
#define ARG_LogRenderedText     ref_pos, text, text_end
#define SIG_RenderText          ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash
#define ARG_RenderText          pos, text, text_end, hide_text_after_hash
#define SIG_ItemSize_Vec2       const ImVec2 size, float text_baseline_y
#define ARG_ItemSize_Vec2       size, text_baseline_y
#define SIG_ItemSize_Rect       const ImRect bb, float text_baseline_y 
#define ARG_ItemSize_Rect       bb, text_baseline_y
#define SIG_ItemAdd             const ImRect bb,ImGuiID id,const ImRect* nav_bb,ImGuiItemFlags extra_flags
#define ARG_ItemAdd             bb, id, nav_bb, extra_flags
#define SIG_ButtonBehavior      const ImRect bb,ImGuiID id,bool* out_hovered,bool* out_held,ImGuiButtonFlags flags
#define ARG_ButtonBehavior      bb, id, out_hovered, out_held, flags
#define SIG_MarkItemEdited      ImGuiID id
#define ARG_MarkItemEdited      id

namespace CImGui {
    // Ignore these stupid fucking intellisense warnings that are just wrong
    IM_FUNC_SIG(Begin,                   bool, SIG_Begin);
    IM_FUNC_SIG(End,                     void);
    IM_FUNC_SIG(Text,                    void, SIG_Text);
    IM_FUNC_SIG(TreeNode_Str,            bool, SIG_TreeNode_Str);
    IM_FUNC_SIG(TreePop,                 void);
    IM_FUNC_SIG(BeginTable,              bool, SIG_BeginTable);
    IM_FUNC_SIG(TableNextColumn,         bool);
    IM_FUNC_SIG(TableHeader,             void, SIG_TableHeader);
    IM_FUNC_SIG(TableNextRow,            void, SIG_TableNextRow);
    IM_FUNC_SIG(EndTable,                void);
    IM_FUNC_SIG(PushStyleVar_Float,      void, SIG_PushStyleVar_Float);
    IM_FUNC_SIG(PushStyleVar_Vec2,       void, SIG_PushStyleVar_Vec2);
    IM_FUNC_SIG(SetCursorPosX,           void, SIG_SetCursorPosX);
    IM_FUNC_SIG(GetCursorPosX,           float);
    IM_FUNC_SIG(GetColumnWidth,          float, SIG_GetColumnWidth);
    IM_FUNC_SIG(PushTextWrapPos,         void, SIG_PushTextWrapPos);
    IM_FUNC_SIG(TextWrapped,             void, SIG_TextWrapped);
    IM_FUNC_SIG(PopTextWrapPos,          void);
    IM_FUNC_SIG(Spacing,                 void);
    IM_FUNC_SIG(Button,                  bool, SIG_Button);
    IM_FUNC_SIG(PushStyleColor_U32,      void, SIG_PushStyleColor_U32);
    IM_FUNC_SIG(PushStyleColor_Vec4,     void, SIG_PushStyleColor_Vec4);
    IM_FUNC_SIG(SameLine,                void, SIG_SameLine);
    IM_FUNC_SIG(PopStyleColor,           void, SIG_PopStyleColor);
    IM_FUNC_SIG(PopStyleVar,             void, SIG_PopStyleVar);
    IM_FUNC_SIG(SetNextWindowFocus,      void);
    IM_FUNC_SIG(SetNextWindowSize,       void, SIG_SetNextWindowSize);
    IM_FUNC_SIG(BeginChild_Str,          bool, SIG_BeginChild_Str);
    IM_FUNC_SIG(BeginChild_ID,           bool, SIG_BeginChild_ID);
    IM_FUNC_SIG(GetStyleColorVec4,       const ImVec4*, SIG_GetStyleColorVec4);
    IM_FUNC_SIG(Selectable_Bool,         bool, SIG_Selectable_Bool);
    IM_FUNC_SIG(Selectable_BoolPtr,      bool, SIG_Selectable_BoolPtr);
    IM_FUNC_SIG(SetItemTooltip,          void, SIG_SetItemTooltip);
    IM_FUNC_SIG(GetColorU32_U32,         ImU32, SIG_GetColorU32_U32);
    IM_FUNC_SIG(GetColorU32_Col,         ImU32, SIG_GetColorU32_Col);
    IM_FUNC_SIG(GetColorU32_Vec4,        ImU32, SIG_GetColorU32_Vec4);
    IM_FUNC_SIG(PushFont,                void, SIG_PushFont);
    IM_FUNC_SIG(PopFont, 			     void);
    IM_FUNC_SIG(EndChild,                void);
    IM_FUNC_SIG(PushItemWidth,           void, SIG_PushItemWidth);
    IM_FUNC_SIG(PopItemWidth,            void);
    IM_FUNC_SIG(InputTextWithHint,       bool, SIG_InputTextWithHint);
    IM_FUNC_SIG(GetCursorPosY, 	         float);
    IM_FUNC_SIG(SetWindowSize_Vec2,      void, SIG_SetWindowSize_Vec2);
    IM_FUNC_SIG(SetWindowSize_Str,       void, SIG_SetWindowSize_Str);
    IM_FUNC_SIG(SetWindowSize_WindowPtr, void, SIG_SetWindowSize_WindowPtr);
    IM_FUNC_SIG(BeginTabBar,             bool, SIG_BeginTabBar);
    IM_FUNC_SIG(EndTabBar,               void);
    IM_FUNC_SIG(BeginTabItem,            bool, SIG_BeginTabItem);
    IM_FUNC_SIG(EndTabItem,              void);
    IM_FUNC_SIG(Checkbox,                bool, SIG_Checkbox);
    IM_FUNC_SIG(Dummy,                   void, SIG_Dummy);
    IM_FUNC_SIG(TextUnformatted,         void, SIG_TextUnformatted);
    IM_FUNC_SIG(SetScrollHereY,          void, SIG_SetScrollHereY);
    IM_FUNC_SIG(InputText,               bool, SIG_InputText);
    IM_FUNC_SIG(BeginCombo,              bool, SIG_BeginCombo);
    IM_FUNC_SIG(EndCombo,                void);
    IM_FUNC_SIG(BeginDisabled,           void, SIG_BeginDisabled);
    IM_FUNC_SIG(EndDisabled,             void);
    IM_FUNC_SIG(Separator,               void);
    IM_FUNC_SIG(GetTextLineHeight,	     float);
    IM_FUNC_SIG(SetCursorPosY,           void, SIG_SetCursorPosY);
    IM_FUNC_SIG(SetNextItemWidth,        void, SIG_SetNextItemWidth);
    IM_FUNC_SIG(SetNextWindowPos,        void, SIG_SexNextWindowPos);
    IM_FUNC_SIG(TableSetupColumn,        void, SIG_TableSetupColumn);
    IM_FUNC_SIG(TableSetupScrollFreeze,  void, SIG_TableSetupScrollFreeze);
    IM_FUNC_SIG(TableHeadersRow,         void);
    IM_FUNC_SIG(TableGetSortSpecs,       ImGuiTableSortSpecs*);
	IM_FUNC_SIG(DragFloat,               bool, SIG_DragFloat);
    IM_FUNC_SIG(GetFontSize,             float);
    IM_FUNC_SIG(InvisibleButton,         bool, SIG_InvisibleButton);
    IM_FUNC_SIG(IsItemHovered,           bool, SIG_IsItemHovered);
    IM_FUNC_SIG(Indent,                  void, SIG_Indent);
    IM_FUNC_SIG(Unindent,                void, SIG_Unindent);
    IM_FUNC_SIG(SeparatorText,           void, SIG_SeparatorText);
	IM_FUNC_SIG(CollapsingHeader_TreeNodeFlags, bool, SIG_CollapsingHeader_TreeNodeFlags);
	IM_FUNC_SIG(CollapsingHeader_BoolPtr,       bool, SIG_CollapsingHeader_BoolPtr);
    IM_FUNC_SIG(SetTooltip,              void, SIG_SetTooltip);
    IM_FUNC_SIG(IsItemClicked,           bool, SIG_IsItemClicked);
    IM_FUNC_SIG(IsMouseHoveringRect,     bool, SIG_IsMouseHoveringRect);
    IM_FUNC_SIG(GetFrameHeight,          float);
    IM_FUNC_SIG(ProgressBar,             bool, SIG_ProgressBar);
    IM_FUNC_SIG(GetIO,                   ImGuiIO*);
    IM_FUNC_SIG(IsMouseClicked_Bool,     bool, SIG_IsMouseClicked_Bool);
    IM_FUNC_SIG(IsWindowHovered,         bool, SIG_IsWindowHovered);
    IM_FUNC_SIG(IsMouseReleased_Nil,     bool, SIG_IsMouseReleased_Nil);
	IM_FUNC_SIG(ColorConvertRGBtoHSV,    void, SIG_ColorConvertRGBtoHSV);
	IM_FUNC_SIG(ColorConvertHSVtoRGB,    void, SIG_ColorConvertHSVtoRGB);
    IM_FUNC_SIG(IsKeyDown_Nil,           bool, SIG_IsKeyDown_Nil);
    IM_FUNC_SIG(IsKeyPressed_Bool,        bool, SIG_IsKeyPressed_Bool);
    IM_FUNC_SIG(IsItemActive,            bool);
    IM_FUNC_SIG(VSliderFloat,            bool, SIG_VSliderFloat);
    IM_FUNC_SIG(TableSetColumnIndex,     bool, SIG_TableSetColumnIndex);
    IM_FUNC_SIG(ArrowButton,             bool, SIG_ArrowButton);
    IM_FUNC_SIG(GetCurrentWindow,        ImGuiWindow*);
    IM_FUNC_SIG(SetCursorPos,            void, SIG_SetCursorPos);
    IM_FUNC_SIG(TableSetBgColor,         void, SIG_TableSetBgColor);
    IM_FUNC_SIG(BeginMenuBar,            bool);
    IM_FUNC_SIG(EndMenuBar,              void);
    IM_FUNC_SIG(SliderInt,               bool, SIG_SliderInt);
    IM_FUNC_SIG(MenuItem_Bool,           bool, SIG_MenuItem_Bool);
    IM_FUNC_SIG(MenuItem_BoolPtr,        bool, SIG_MenuItem_BoolPtr);
	IM_FUNC_SIG(SetNextWindowSizeConstraints, void, SIG_SetNextWindowSizeConstraints);
    IM_FUNC_SIG(GetCurrentContext,       ImGuiContext*);
    IM_FUNC_SIG(GetVersion,              const char*);
    IM_FUNC_SIG(BeginPopupContextItem,   bool, SIG_BeginPopupContextItem);
    IM_FUNC_SIG(EndPopup,                void);
    IM_FUNC_SIG(TableGetRowIndex,        int);
    IM_FUNC_SIG(GetFrameHeightWithSpacing, float);
    IM_FUNC_SIG(ShowStyleEditor,         void, SIG_ShowStyleEditor);
    IM_FUNC_SIG(ShowDemoWindow,          void, SIG_ShowDemoWindow);
    IM_FUNC_SIG(SetItemDefaultFocus,     void);
    IM_FUNC_SIG(GetCurrentTable,         ImGuiTable*);
    IM_FUNC_SIG(SetNextItemAllowOverlap, void);
    IM_FUNC_SIG(SetCursorScreenPos,      void, SIG_SetCursorScreenPos);
    IM_FUNC_SIG(ColorEdit4,              bool, SIG_ColorEdit4);
    IM_FUNC_SIG(DragFloat4,              bool, SIG_DragFloat4);
    // Object Member Functions
    IM_MEMBER_FUNC_SIG(FontAtlas_AddFontFromFileTTF,  ImFont*, SIG_FontAtlas_AddFontFromFileTTF);
    IM_MEMBER_FUNC_SIG(DrawList_AddText_Vec2,    void, SIG_DrawList_AddText_Vec2);
	IM_MEMBER_FUNC_SIG(DrawList_AddText_FontPtr, void, SIG_DrawList_AddText_FontPtr);
	IM_MEMBER_FUNC_SIG(DrawList_AddCircleFilled, void, SIG_DrawList_AddCircleFilled);
	IM_MEMBER_FUNC_SIG(DrawList_AddCircle,       void, SIG_DrawList_AddCircle);
	IM_MEMBER_FUNC_SIG(DrawList_AddLine,         void, SIG_DrawList_AddLine);
	IM_MEMBER_FUNC_SIG(DrawList_AddRectFilled,   void, SIG_DrawList_AddRectFilled);
	IM_MEMBER_FUNC_SIG(DrawList_AddRect,         void, SIG_DrawList_AddRect);
    IM_MEMBER_FUNC_SIG(GuiViewport_GetCenter,    void, SIG_GuiViewport_GetCenter);
    IM_MEMBER_FUNC_SIG(GuiWindow_GetID_Str,      ImGuiID, SIG_GuiWindow_GetID_Str);
    IM_MEMBER_FUNC_SIG(GuiWindow_GetID_Ptr,      ImGuiID, SIG_GuiWindow_GetID_Ptr);
    IM_MEMBER_FUNC_SIG(GuiWindow_GetID_Int,      ImGuiID, SIG_GuiWindow_GetID_Int);
    // Internal
    IM_FUNC_SIG(RenderBullet,            void, SIG_RenderBullet);
    IM_FUNC_SIG(LogRenderedText,         void, SIG_LogRenderedText);
    IM_FUNC_SIG(RenderText,              void, SIG_RenderText);
    IM_FUNC_SIG(ItemSize_Vec2,           void, SIG_ItemSize_Vec2);
    IM_FUNC_SIG(ItemSize_Rect,           void, SIG_ItemSize_Rect);
    IM_FUNC_SIG(ItemAdd,                 bool, SIG_ItemAdd);
    IM_FUNC_SIG(ButtonBehavior,          bool, SIG_ButtonBehavior);
    IM_FUNC_SIG(MarkItemEdited,          void, SIG_MarkItemEdited);
    // Functions that need wrappers to handle their opaque ptr return types / inconvenient signatures
    IM_FUNC_SIG_EXPLICIT(GetWindowDrawListOpaque, ImDrawList*);
    IM_FUNC_SIG_EXPLICIT(GetMainViewportOpaque,	  ImGuiViewport*);
    IM_FUNC_SIG_EXPLICIT(GetStylePtr,             ImGuiStyle*);
    IM_FUNC_SIG_EXPLICIT(GetCursorPosByArg,          void, SIG_GetCursorPosByArg);
	IM_FUNC_SIG_EXPLICIT(CalcTextSizeByArg,          void, SIG_CalcTextSizeByArg);
    IM_FUNC_SIG_EXPLICIT(GetContentRegionAvailByArg, void, SIG_GetContentRegionAvailByArg);
    IM_FUNC_SIG_EXPLICIT(GetWindowSizeByArg,         void, SIG_GetWindowSizeByArg);
    IM_FUNC_SIG_EXPLICIT(GetCursorScreenPosByArg,    void, SIG_GetCursorScreenPosByArg);
    IM_FUNC_SIG_EXPLICIT(GetItemRectMinByArg,        void, SIG_GetItemRectMinByArg);
    IM_FUNC_SIG_EXPLICIT(StyleColorsClassicByArg,    void, SIG_StyleColorsClassicByArg);
    IM_FUNC_SIG_EXPLICIT(StyleColorsDarkByArg,       void, SIG_StyleColorsDarkByArg);
    IM_FUNC_SIG_EXPLICIT(GetMousePosByArg,           void, SIG_GetMousePosByArg);
    IM_FUNC_SIG_EXPLICIT(TableGetCellBgRectByArg,    void, SIG_TableGetCellBgRectByArg);


    IM_FUNC(Begin,                  bool,        (const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0), (ARG_Begin));
    IM_FUNC(End,                    void,        (), ());
    IM_FUNC(Text,                   void,        (SIG_Text), (ARG_Text));
    IM_FUNC(TreeNode_Str,           bool,        (SIG_TreeNode_Str), (ARG_TreeNode_Str));
    IM_FUNC(TreePop,                void,        (), ());
    IM_FUNC(BeginTable,             bool,        (const char* str_id, int columns, ImGuiTableFlags flags = 0, const ImVec2 outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f), (ARG_BeginTable));
    IM_FUNC(TableNextColumn,        bool,        (), ());
    IM_FUNC(TableHeader,            void,        (SIG_TableHeader), (ARG_TableHeader));
    IM_FUNC(TableNextRow,           void,        (ImGuiTableRowFlags row_flags = 0, float min_row_height = 0.0f), (ARG_TableNextRow));
    IM_FUNC(EndTable,               void,        (), ());
    IM_FUNC_OVERLOAD(PushStyleVar, PushStyleVar_Float, void, (SIG_PushStyleVar_Float), (ARG_PushStyleVar_Float));
    IM_FUNC_OVERLOAD(PushStyleVar, PushStyleVar_Vec2,  void, (SIG_PushStyleVar_Vec2), (ARG_PushStyleVar_Vec2));
    IM_FUNC(SetCursorPosX,          void,        (SIG_SetCursorPosX), (ARG_SetCursorPosX));
    IM_FUNC(GetCursorPosX,          float,       (), ());
    IM_FUNC(GetColumnWidth,         float,       (int column_index = -1), (ARG_GetColumnWidth));
    IM_FUNC(PushTextWrapPos,        void,        (float wrap_local_pos_x = 0.0f), (ARG_PushTextWrapPos));
    IM_FUNC(TextWrapped,            void,        (SIG_TextWrapped), (ARG_TextWrapped));
    IM_FUNC(PopTextWrapPos,         void,        (), ());
    IM_FUNC(Spacing,                void,        (), ());
    IM_FUNC(Button,                 bool,        (const char* label, const ImVec2 size_arg = ImVec2(0, 0)), (ARG_Button));
    IM_FUNC_OVERLOAD(PushStyleColor, PushStyleColor_U32,  void, (SIG_PushStyleColor_U32), (ARG_PushStyleColor_U32));
    IM_FUNC_OVERLOAD(PushStyleColor, PushStyleColor_Vec4, void, (SIG_PushStyleColor_Vec4), (ARG_PushStyleColor_Vec4));
    IM_FUNC(SameLine,               void,        (float offset_from_start_x = 0.0f, float spacing = -1.0f), (ARG_SameLine));
    IM_FUNC(PopStyleColor,          void,        (int count = 1), (ARG_PopStyleColor));
    IM_FUNC(PopStyleVar,            void,        (int count = 1), (ARG_PopStyleVar));
    IM_FUNC(SetNextWindowFocus,     void,        (), ());
    IM_FUNC(SetNextWindowSize,      void,        (const ImVec2 size, ImGuiCond cond = 0), (ARG_SetNextWindowSize));
    IM_FUNC_OVERLOAD(BeginChild, BeginChild_Str, bool, (const char* str_id, const ImVec2 size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags flags = 0), (ARG_BeginChild_Str));
    IM_FUNC_OVERLOAD(BeginChild, BeginChild_ID,  bool, (ImGuiID id, const ImVec2 size = ImVec2(0, 0), ImGuiChildFlags child_flags = 0, ImGuiWindowFlags window_flags = 0), (ARG_BeginChild_ID));
    IM_FUNC(GetStyleColorVec4,      const ImVec4*, (SIG_GetStyleColorVec4), (ARG_GetStyleColorVec4));
    IM_FUNC_OVERLOAD(Selectable, Selectable_Bool,    bool, (const char* label, bool selected = false, ImGuiSelectableFlags flags = 0, const ImVec2 size = ImVec2(0, 0)), (ARG_Selectable_Bool));
    IM_FUNC_OVERLOAD(Selectable, Selectable_BoolPtr, bool, (const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0, const ImVec2 size = ImVec2(0, 0)), (ARG_Selectable_BoolPtr));
    IM_FUNC(SetItemTooltip,         void,        (SIG_SetItemTooltip), (ARG_SetItemTooltip));
    IM_FUNC_OVERLOAD(GetColorU32, GetColorU32_U32,  ImU32, (ImU32 col, float alpha_mul = 1.0f), (ARG_GetColorU32_U32));
    IM_FUNC_OVERLOAD(GetColorU32, GetColorU32_Col,  ImU32, (ImGuiCol idx, float alpha_mul = 1.0f), (ARG_GetColorU32_Col));
    IM_FUNC_OVERLOAD(GetColorU32, GetColorU32_Vec4, ImU32, (SIG_GetColorU32_Vec4), (ARG_GetColorU32_Vec4));
    IM_FUNC(PushFont,               void,        (SIG_PushFont), (ARG_PushFont));
    IM_FUNC(PopFont,                void,        (), ());
    IM_FUNC(EndChild,               void,        (), ());
    IM_FUNC(PushItemWidth,          void,        (SIG_PushItemWidth), (ARG_PushItemWidth));
    IM_FUNC(PopItemWidth,           void,        (), ());
    IM_FUNC(InputTextWithHint,      bool,        (const char* label, const char* hint, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr), (ARG_InputTextWithHint));
    IM_FUNC(GetCursorPosY,          float,       (), ());
    IM_FUNC_OVERLOAD(SetWindowSize, SetWindowSize_Vec2, void, (const ImVec2 size, ImGuiCond cond = 0), (ARG_SetWindowSize_Vec2));
    IM_FUNC_OVERLOAD(SetWindowSize, SetWindowSize_Str, void, (const char* name, const ImVec2 size, ImGuiCond cond = 0), (ARG_SetWindowSize_Str));
    IM_FUNC_OVERLOAD(SetWindowSize, SetWindowSize_WindowPtr, void, (ImGuiWindow* window, const ImVec2 size, ImGuiCond cond = 0), (ARG_SetWindowSize_WindowPtr));
    IM_FUNC(BeginTabBar,            bool,        (const char* str_id, ImGuiTabBarFlags flags = 0), (ARG_BeginTabBar));
    IM_FUNC(EndTabBar,              void,        (), ());
    IM_FUNC(BeginTabItem,           bool,        (const char* label, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0), (ARG_BeginTabItem));
    IM_FUNC(EndTabItem,             void,        (), ());
    IM_FUNC(Checkbox,               bool,        (const char* label, bool* v), (ARG_Checkbox));
    IM_FUNC(Dummy,                  void,        (const ImVec2 size), (ARG_Dummy));
    IM_FUNC(TextUnformatted,        void,        (const char* text, const char* text_end = nullptr), (ARG_TextUnformatted));
    IM_FUNC(SetScrollHereY,         void,        (float center_y_ratio = 0.5f), (ARG_SetScrollHereY));
    IM_FUNC(InputText,              bool,        (const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr), (ARG_InputText));
    IM_FUNC(BeginCombo,             bool,        (const char* label, const char* preview_value, ImGuiComboFlags flags = 0), (ARG_BeginCombo));
    IM_FUNC(EndCombo,               void,        (), ());
    IM_FUNC(BeginDisabled,          void,        (bool disabled = true), (ARG_BeginDisabled));
    IM_FUNC(EndDisabled,            void,        (), ());
    IM_FUNC(Separator,              void,        (), ());
    IM_FUNC(GetTextLineHeight,      float,       (), ());
    IM_FUNC(SetCursorPosY,          void,        (SIG_SetCursorPosY), (ARG_SetCursorPosY));
    IM_FUNC(SetNextItemWidth,       void,        (SIG_SetNextItemWidth), (ARG_SetNextItemWidth));
    IM_FUNC(SetNextWindowPos,       void,        (const ImVec2 pos, ImGuiCond cond = 0, const ImVec2 pivot = ImVec2(0, 0)), (ARG_SetNextWindowPos));
    IM_FUNC(TableSetupColumn,       void,        (const char* label, ImGuiTableColumnFlags flags = 0, float init_width_or_weight = 0.0f, ImGuiID user_id = 0), (ARG_TableSetupColumn));
	IM_FUNC(TableSetupScrollFreeze, void,        (int cols, int rows), (ARG_TableSetupScrollFreeze));
    IM_FUNC(TableHeadersRow,        void,        (), ());
    IM_FUNC(TableGetSortSpecs,      ImGuiTableSortSpecs*, (), ());
	IM_FUNC(DragFloat,              bool, (const char* label, float* v, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0), (ARG_DragFloat));
    IM_FUNC(GetFontSize,            float, (), ());
    IM_FUNC(InvisibleButton,        bool, (const char* str_id, const ImVec2 size, ImGuiButtonFlags flags = 0), (ARG_InvisibleButton));
    IM_FUNC(IsItemHovered,          bool, (ImGuiHoveredFlags flags = 0), (ARG_IsItemHovered));
    IM_FUNC(Indent,                 void, (float indent_w = 0.0f), (ARG_Indent));
    IM_FUNC(Unindent,               void, (float indent_w = 0.0f), (ARG_Unindent));
    IM_FUNC(SeparatorText,          void, (SIG_SeparatorText), (ARG_SeparatorText));
	IM_FUNC_OVERLOAD(CollapsingHeader, CollapsingHeader_TreeNodeFlags, bool, (const char* label, ImGuiTreeNodeFlags flags = 0), (ARG_CollapsingHeader_TreeNodeFlags));
	IM_FUNC_OVERLOAD(CollapsingHeader, CollapsingHeader_BoolPtr,       bool, (const char* label, bool* p_visible, ImGuiTreeNodeFlags flags = 0), (ARG_CollapsingHeader_BoolPtr));
    IM_FUNC(SetTooltip,             void, (const char* fmt, ...), (ARG_SetTooltip));
    IM_FUNC(IsItemClicked,          bool, (ImGuiMouseButton mouse_button = 0), (ARG_IsItemClicked));
    IM_FUNC(IsMouseHoveringRect,    bool, (const ImVec2 r_min, const ImVec2 r_max, bool clip = true), (ARG_IsMouseHoveringRect));
    IM_FUNC(GetFrameHeight,         float, (), ());
    IM_FUNC(ProgressBar,            bool, (float fraction, const ImVec2 size_arg = ImVec2(-FLT_MIN, 0), const char* overlay = nullptr), (ARG_ProgressBar));
    IM_FUNC(GetIO,                  ImGuiIO*, (), ());
	IM_FUNC_OVERLOAD(IsMouseClicked, IsMouseClicked_Bool, bool, (ImGuiMouseButton button, bool repeat = false), (ARG_IsMouseClicked_Bool));
	IM_FUNC(IsWindowHovered,        bool, (ImGuiHoveredFlags flags = 0), (ARG_IsWindowHovered));
	IM_FUNC_OVERLOAD(IsMouseReleased, IsMouseReleased_Nil, bool, (SIG_IsMouseReleased_Nil), (ARG_IsMouseReleased_Nil));
    IM_FUNC(ColorConvertRGBtoHSV,   void, (SIG_ColorConvertRGBtoHSV), (ARG_ColorConvertRGBtoHSV));
    IM_FUNC(ColorConvertHSVtoRGB,   void, (SIG_ColorConvertHSVtoRGB), (ARG_ColorConvertHSVtoRGB));
	IM_FUNC_OVERLOAD(IsKeyDown, IsKeyDown_Nil, bool, (SIG_IsKeyDown_Nil), (ARG_IsKeyDown_Nil));
	IM_FUNC_OVERLOAD(IsKeyPressed, IsKeyPressed_Bool, bool, (SIG_IsKeyPressed_Bool), (ARG_IsKeyPressed_Bool));
    IM_FUNC(IsItemActive,           bool, (), ());
    IM_FUNC(VSliderFloat,           bool, (const char* label, const ImVec2 size, float* v, float v_min, float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0), (ARG_VSliderFloat));
	IM_FUNC(TableSetColumnIndex,    bool, (SIG_TableSetColumnIndex), (ARG_TableSetColumnIndex));
    IM_FUNC(ArrowButton,            bool, (const char* str_id, ImGuiDir dir), (ARG_ArrowButton));
    IM_FUNC(GetCurrentWindow,       ImGuiWindow*, (), ());
    IM_FUNC(SetCursorPos,           void, (SIG_SetCursorPos), (ARG_SetCursorPos));
    IM_FUNC(TableSetBgColor,        void, (ImGuiTableBgTarget target, ImU32 color, int column_n = -1), (ARG_TableSetBgColor));
    IM_FUNC(BeginMenuBar,           bool, (), ());
    IM_FUNC(EndMenuBar,             void, (), ());
    IM_FUNC(SliderInt,              bool, (const char* label, int* v, int v_min, int v_max, const char* format = "%d", ImGuiSliderFlags flags = 0), (ARG_SliderInt));
	IM_FUNC_OVERLOAD(MenuItem, MenuItem_Bool,    bool, (const char* label, const char* shortcut = nullptr, bool selected = false, bool enabled = true), (ARG_MenuItem_Bool));
	IM_FUNC_OVERLOAD(MenuItem, MenuItem_BoolPtr, bool, (const char* label, const char* shortcut, bool* p_selected = nullptr, bool enabled = true), (ARG_MenuItem_BoolPtr));
    IM_FUNC(SetNextWindowSizeConstraints, void, (const ImVec2 size_min, const ImVec2 size_max, ImGuiSizeCallback custom_callback = nullptr, void* custom_callback_data = nullptr), (ARG_SetNextWindowSizeConstraints));
	IM_FUNC(GetCurrentContext,     ImGuiContext*, (), ());
    IM_FUNC(GetVersion,            const char*, (), ());
	IM_FUNC(BeginPopupContextItem, bool, (const char* str_id, ImGuiPopupFlags popup_flags = 0), (ARG_BeginPopupContextItem));
    IM_FUNC(EndPopup,              void, (), ());
    IM_FUNC(TableGetRowIndex,      int, (), ());
	IM_FUNC(GetFrameHeightWithSpacing, float, (), ());
    IM_FUNC(ShowStyleEditor,       void, (ImGuiStyle* ref = nullptr), (ARG_ShowStyleEditor));
    IM_FUNC(ShowDemoWindow,        void, (bool* p_open = nullptr), (ARG_ShowDemoWindow));
    IM_FUNC(SetItemDefaultFocus,   void, (), ());
    IM_FUNC(GetCurrentTable,       ImGuiTable*, (), ());
	IM_FUNC(SetNextItemAllowOverlap, void, (), ());
    IM_FUNC(SetCursorScreenPos,    void, (SIG_SetCursorScreenPos), (ARG_SetCursorScreenPos));
    IM_FUNC(ColorEdit4,            bool, (SIG_ColorEdit4), (ARG_ColorEdit4));
    IM_FUNC(DragFloat4,            bool, (const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0), (ARG_DragFloat4));
	// Neseted Object Functions
	IM_FUNC(FontAtlas_AddFontFromFileTTF, ImFont*, (ImFontAtlas* self, const char* filename, float size_pixels = 0.0f, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr), (ARG_FontAtlas_AddFontFromFileTTF));
	IM_FUNC_OVERLOAD(DrawList_AddText, DrawList_AddText_Vec2,    void, (ImDrawList* self, const ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end = nullptr), (ARG_DrawList_AddText_Vec2));
	IM_FUNC_OVERLOAD(DrawList_AddText, DrawList_AddText_FontPtr, void, (ImDrawList* self, ImFont* font, float font_size, const ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end = nullptr, float wrap_width = 0.0f, const ImVec4* cpu_fine_clip_rect = nullptr), (ARG_DrawList_AddText_FontPtr));
	IM_FUNC(DrawList_AddCircleFilled,  void, (ImDrawList* self, const ImVec2 center, float radius, ImU32 col, int num_segments = 0), (ARG_DrawList_AddCircleFilled));
    IM_FUNC(DrawList_AddCircle,        void, (ImDrawList* self, const ImVec2 center, float radius, ImU32 col, int num_segments, float thickness = 1.0f), (ARG_DrawList_AddCircle));
    IM_FUNC(DrawList_AddLine,          void, (ImDrawList* self, const ImVec2 p1, const ImVec2 p2, ImU32 col, float thickness = 1.0f), (ARG_DrawList_AddLine));
    IM_FUNC(DrawList_AddRectFilled,    void, (ImDrawList* self, const ImVec2 p_min, const ImVec2 p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0), (ARG_DrawList_AddRectFilled));
    IM_FUNC(DrawList_AddRect,          void, (ImDrawList* self, const ImVec2 p_min, const ImVec2 p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f), (ARG_DrawList_AddRect));
    IM_FUNC(GuiViewport_GetCenter,     void, (SIG_GuiViewport_GetCenter), (ARG_GuiViewport_GetCenter));
    IM_FUNC(GuiWindow_GetID_Str,       ImGuiID, (ImGuiWindow* self, const char* str, const char* str_end = nullptr), (ARG_GuiWindow_GetID_Str));
    IM_FUNC(GuiWindow_GetID_Ptr,       ImGuiID, (SIG_GuiWindow_GetID_Ptr), (ARG_GuiWindow_GetID_Ptr));
    IM_FUNC(GuiWindow_GetID_Int,       ImGuiID, (SIG_GuiWindow_GetID_Int), (ARG_GuiWindow_GetID_Int));

    // Internal
    IM_FUNC(RenderBullet,           void, (SIG_RenderBullet), (ARG_RenderBullet));
    IM_FUNC(LogRenderedText,        void, (const ImVec2* ref_pos, const char* text, const char* text_end = nullptr), (ARG_LogRenderedText));
    IM_FUNC(RenderText,             void, (ImVec2 pos, const char* text, const char* text_end = nullptr, bool hide_text_after_hash = false), (ARG_RenderText));
	IM_FUNC_OVERLOAD(ItemSize, ItemSize_Vec2, void, (const ImVec2 size, float text_baseline_y = -1.0f), (ARG_ItemSize_Vec2));
	IM_FUNC_OVERLOAD(ItemSize, ItemSize_Rect, void, (const ImRect bb, float text_baseline_y = -1.0f), (ARG_ItemSize_Rect));
    IM_FUNC(ItemAdd,                bool, (const ImRect bb, ImGuiID id, const ImRect* nav_bb = NULL, ImGuiItemFlags extra_flags = 0), (ARG_ItemAdd));
	IM_FUNC(ButtonBehavior,         bool, (const ImRect bb, ImGuiID id, bool* out_hovered, bool* out_held, ImGuiButtonFlags flags = 0), (ARG_ButtonBehavior));
    IM_FUNC(MarkItemEdited,         void, (SIG_MarkItemEdited), (ARG_MarkItemEdited));

    // Functions that return opaque pointers / need wrappers for convenience
    IM_FUNC(GetWindowDrawListOpaque, ImDrawList*, (), ());
    IM_FUNC(GetMainViewportOpaque,   ImGuiViewport*, (), ());
    IM_FUNC(GetStylePtr,             ImGuiStyle*, (), ());
    IM_FUNC(GetCursorPosByArg,          void, (SIG_GetCursorPosByArg), (ARG_GetCursorPosByArg));
    IM_FUNC(CalcTextSizeByArg,          void, (ImVec2* pOut, const char* text, const char* text_end = nullptr, bool hide_text_after_double_hash = false, float wrap_width = -1.0f), (ARG_CalcTextSizeByArg));
	IM_FUNC(GetContentRegionAvailByArg, void, (SIG_GetContentRegionAvailByArg), (ARG_GetContentRegionAvailByArg));
	IM_FUNC(GetWindowSizeByArg,         void, (SIG_GetWindowSizeByArg), (ARG_GetWindowSizeByArg));
	IM_FUNC(GetCursorScreenPosByArg,    void, (SIG_GetCursorScreenPosByArg), (ARG_GetCursorScreenPosByArg));
	IM_FUNC(GetItemRectMinByArg,        void, (SIG_GetItemRectMinByArg), (ARG_GetItemRectMinByArg));
    IM_FUNC(StyleColorsClassicByArg,    void, (SIG_StyleColorsClassicByArg), (ARG_StyleColorsClassicByArg));
    IM_FUNC(StyleColorsDarkByArg,       void, (SIG_StyleColorsDarkByArg), (ARG_StyleColorsDarkByArg));
    IM_FUNC(GetMousePosByArg,           void, (SIG_GetMousePosByArg), (ARG_GetMousePosByArg));
    IM_FUNC(TableGetCellBgRectByArg,    void, (SIG_TableGetCellBgRectByArg), (ARG_TableGetCellBgRectByArg));

    inline void initializeFuncs() {
        // TODO: FREE LIBRARY!!
        //       Though... probably doesn't matter since plugin only loads once.
        std::filesystem::path dinput8_relpath = "dinput8.dll";
        std::filesystem::path dinput8_abspath = std::filesystem::absolute(dinput8_relpath);

        #ifdef _DEBUG 
            MessageBoxA(0, dinput8_abspath.string().c_str(), "KBF DEBUG WINDOW", 0);
        #endif

        HMODULE dinput8 = LoadLibrary(dinput8_abspath.string().c_str());
        if (!dinput8) {
            MessageBoxA(0, "FATAL ERROR: Couldn't load dinput8.dll", "Kana's Body Framework - Init Failed", 0);

            //MessageBoxA(0, std::format("Couldn't load dinput8.dll at {}", dinput8_abspath.string()).c_str(), "Kana's Body Framework: Initialize Failed", 0);
            return;
        }

        IM_GET_FUNC(Begin);
        IM_GET_FUNC(End);
        IM_GET_FUNC(Text);
        IM_GET_FUNC(TreeNode_Str);
        IM_GET_FUNC(TreePop);
        IM_GET_FUNC(BeginTable);
        IM_GET_FUNC(TableNextColumn);
        IM_GET_FUNC(TableHeader);
        IM_GET_FUNC(TableNextRow);
        IM_GET_FUNC(EndTable);
        IM_GET_FUNC(PushStyleVar_Float);
        IM_GET_FUNC(PushStyleVar_Vec2);
        IM_GET_FUNC(SetCursorPosX);
        IM_GET_FUNC(GetCursorPosX);
        IM_GET_FUNC(GetColumnWidth);
        IM_GET_FUNC(PushTextWrapPos);
        IM_GET_FUNC(TextWrapped);
        IM_GET_FUNC(PopTextWrapPos);
        IM_GET_FUNC(Spacing);
        IM_GET_FUNC(Button);
        IM_GET_FUNC(PushStyleColor_U32);
        IM_GET_FUNC(PushStyleColor_Vec4);
        IM_GET_FUNC(SameLine);
        IM_GET_FUNC(PopStyleColor);
        IM_GET_FUNC(PopStyleVar);
        IM_GET_FUNC(SetNextWindowFocus);
        IM_GET_FUNC(SetNextWindowSize);
        IM_GET_FUNC(BeginChild_Str);
        IM_GET_FUNC(BeginChild_ID);
        IM_GET_FUNC(GetStyleColorVec4);
        IM_GET_FUNC(Selectable_Bool);
        IM_GET_FUNC(Selectable_BoolPtr);
        IM_GET_FUNC(SetItemTooltip);
        IM_GET_FUNC(GetColorU32_U32);
        IM_GET_FUNC(GetColorU32_Col);
        IM_GET_FUNC(GetColorU32_Vec4);
        IM_GET_FUNC(PushFont);
        IM_GET_FUNC(PopFont);
        IM_GET_FUNC(EndChild);
        IM_GET_FUNC(PushItemWidth);
        IM_GET_FUNC(PopItemWidth);
        IM_GET_FUNC(InputTextWithHint);
        IM_GET_FUNC(GetCursorPosY);
        IM_GET_FUNC(SetWindowSize_Vec2);
        IM_GET_FUNC(SetWindowSize_Str);
        IM_GET_FUNC(SetWindowSize_WindowPtr);
        IM_GET_FUNC(BeginTabBar);
        IM_GET_FUNC(EndTabBar);
        IM_GET_FUNC(BeginTabItem);
        IM_GET_FUNC(EndTabItem);
        IM_GET_FUNC(Checkbox);
        IM_GET_FUNC(Dummy);
        IM_GET_FUNC(TextUnformatted);
        IM_GET_FUNC(SetScrollHereY);
        IM_GET_FUNC(InputText);
        IM_GET_FUNC(BeginCombo);
        IM_GET_FUNC(EndCombo);
        IM_GET_FUNC(BeginDisabled);
        IM_GET_FUNC(EndDisabled);
        IM_GET_FUNC(Separator);
        IM_GET_FUNC(GetTextLineHeight);
        IM_GET_FUNC(SetCursorPosY);
        IM_GET_FUNC(SetNextItemWidth);
		IM_GET_FUNC(SetNextWindowPos);
		IM_GET_FUNC(TableSetupColumn);
		IM_GET_FUNC(TableSetupScrollFreeze);
		IM_GET_FUNC(TableHeadersRow);
		IM_GET_FUNC(TableGetSortSpecs);
        IM_GET_FUNC(DragFloat);
		IM_GET_FUNC(GetFontSize);
		IM_GET_FUNC(InvisibleButton);
		IM_GET_FUNC(IsItemHovered);
		IM_GET_FUNC(Indent);
		IM_GET_FUNC(Unindent);
		IM_GET_FUNC(SeparatorText);
		IM_GET_FUNC(CollapsingHeader_TreeNodeFlags);
		IM_GET_FUNC(CollapsingHeader_BoolPtr);
		IM_GET_FUNC(SetTooltip);
		IM_GET_FUNC(IsItemClicked);
		IM_GET_FUNC(IsMouseHoveringRect);
		IM_GET_FUNC(GetFrameHeight);
		IM_GET_FUNC(ProgressBar);
		IM_GET_FUNC(GetIO, "_Nil");
		IM_GET_FUNC(IsMouseClicked_Bool);
		IM_GET_FUNC(IsWindowHovered);
		IM_GET_FUNC(IsMouseReleased_Nil);
		IM_GET_FUNC(ColorConvertRGBtoHSV);
		IM_GET_FUNC(ColorConvertHSVtoRGB);
		IM_GET_FUNC(IsKeyDown_Nil);
		IM_GET_FUNC(IsKeyPressed_Bool);
		IM_GET_FUNC(IsItemActive);
		IM_GET_FUNC(VSliderFloat);
		IM_GET_FUNC(TableSetColumnIndex);
		IM_GET_FUNC(ArrowButton);
		IM_GET_FUNC(GetCurrentWindow);
		IM_GET_FUNC(SetCursorPos);
		IM_GET_FUNC(TableSetBgColor);
		IM_GET_FUNC(BeginMenuBar);
		IM_GET_FUNC(EndMenuBar);
		IM_GET_FUNC(SliderInt);
		IM_GET_FUNC(MenuItem_Bool);
		IM_GET_FUNC(MenuItem_BoolPtr);
		IM_GET_FUNC(SetNextWindowSizeConstraints);
		IM_GET_FUNC(GetCurrentContext);
        IM_GET_FUNC(GetVersion);
		IM_GET_FUNC(BeginPopupContextItem);
		IM_GET_FUNC(EndPopup);
		IM_GET_FUNC(TableGetRowIndex);
		IM_GET_FUNC(GetFrameHeightWithSpacing);
		IM_GET_FUNC(ShowStyleEditor);
        IM_GET_FUNC(ShowDemoWindow);
		IM_GET_FUNC(SetItemDefaultFocus);
		IM_GET_FUNC(GetCurrentTable);
		IM_GET_FUNC(SetNextItemAllowOverlap);
        IM_GET_FUNC(SetCursorScreenPos);
        IM_GET_FUNC(ColorEdit4);
        IM_GET_FUNC(DragFloat4);
		// Nested Object Functions
		IM_GET_MEMBER_FUNC(FontAtlas_AddFontFromFileTTF);
		IM_GET_MEMBER_FUNC(DrawList_AddText_Vec2);
		IM_GET_MEMBER_FUNC(DrawList_AddText_FontPtr);
		IM_GET_MEMBER_FUNC(DrawList_AddCircleFilled);
		IM_GET_MEMBER_FUNC(DrawList_AddCircle);
		IM_GET_MEMBER_FUNC(DrawList_AddLine);
		IM_GET_MEMBER_FUNC(DrawList_AddRectFilled);
		IM_GET_MEMBER_FUNC(DrawList_AddRect);
		IM_GET_MEMBER_FUNC(GuiViewport_GetCenter);
		IM_GET_MEMBER_FUNC(GuiWindow_GetID_Str);
		IM_GET_MEMBER_FUNC(GuiWindow_GetID_Ptr);
		IM_GET_MEMBER_FUNC(GuiWindow_GetID_Int);
        // Internal
		IM_GET_FUNC(RenderBullet);
		IM_GET_FUNC(LogRenderedText);
		IM_GET_FUNC(RenderText);
		IM_GET_FUNC(ItemSize_Vec2);
		IM_GET_FUNC(ItemSize_Rect);
		IM_GET_FUNC(ItemAdd);
		IM_GET_FUNC(ButtonBehavior);
		IM_GET_FUNC(MarkItemEdited);

        // Functions that wrap around an ImGui function with a different name
        IM_GET_FUNC_EXPLICIT(GetWindowDrawListOpaque,    igGetWindowDrawList);
        IM_GET_FUNC_EXPLICIT(GetMainViewportOpaque,      igGetMainViewport);
        IM_GET_FUNC_EXPLICIT(GetStylePtr,                igGetStyle);
		IM_GET_FUNC_EXPLICIT(GetCursorPosByArg,          igGetCursorPos);
        IM_GET_FUNC_EXPLICIT(CalcTextSizeByArg,          igCalcTextSize);
		IM_GET_FUNC_EXPLICIT(GetContentRegionAvailByArg, igGetContentRegionAvail);
		IM_GET_FUNC_EXPLICIT(GetWindowSizeByArg,         igGetWindowSize);
		IM_GET_FUNC_EXPLICIT(GetCursorScreenPosByArg,    igGetCursorScreenPos);
		IM_GET_FUNC_EXPLICIT(GetItemRectMinByArg,        igGetItemRectMin);
        IM_GET_FUNC_EXPLICIT(StyleColorsClassicByArg,    igStyleColorsClassic);
        IM_GET_FUNC_EXPLICIT(StyleColorsDarkByArg,       igStyleColorsDark);
        IM_GET_FUNC_EXPLICIT(GetMousePosByArg,           igGetMousePos);
        IM_GET_FUNC_EXPLICIT(TableGetCellBgRectByArg,    igTableGetCellBgRect);

        ASSERT_LOADED(Begin);
        ASSERT_LOADED(End);                    
        ASSERT_LOADED(Text);                   
        ASSERT_LOADED(TreeNode_Str);           
        ASSERT_LOADED(TreePop);                
        ASSERT_LOADED(BeginTable);             
        ASSERT_LOADED(TableNextColumn);        
        ASSERT_LOADED(TableHeader);            
        ASSERT_LOADED(TableNextRow);           
        ASSERT_LOADED(EndTable);               
        ASSERT_LOADED(PushStyleVar_Float);     
        ASSERT_LOADED(PushStyleVar_Vec2);      
        ASSERT_LOADED(SetCursorPosX);          
        ASSERT_LOADED(GetCursorPosX);          
        ASSERT_LOADED(GetColumnWidth);         
        ASSERT_LOADED(PushTextWrapPos);        
        ASSERT_LOADED(TextWrapped);            
        ASSERT_LOADED(PopTextWrapPos);         
        ASSERT_LOADED(Spacing);                
        ASSERT_LOADED(Button);                 
        ASSERT_LOADED(PushStyleColor_U32);     
        ASSERT_LOADED(PushStyleColor_Vec4);    
        ASSERT_LOADED(SameLine);               
        ASSERT_LOADED(PopStyleColor);          
        ASSERT_LOADED(PopStyleVar);            
        ASSERT_LOADED(SetNextWindowFocus);     
        ASSERT_LOADED(SetNextWindowSize);      
        ASSERT_LOADED(GetContentRegionAvailByArg);  
        ASSERT_LOADED(GetWindowSizeByArg);          
        ASSERT_LOADED(BeginChild_Str);         
        ASSERT_LOADED(BeginChild_ID);          
        ASSERT_LOADED(GetStyleColorVec4);      
        ASSERT_LOADED(Selectable_Bool);        
        ASSERT_LOADED(Selectable_BoolPtr);     
        ASSERT_LOADED(SetItemTooltip);         
        ASSERT_LOADED(GetCursorScreenPosByArg);     
        ASSERT_LOADED(GetItemRectMinByArg);         
        ASSERT_LOADED(GetColorU32_U32);        
        ASSERT_LOADED(GetColorU32_Col);        
        ASSERT_LOADED(GetColorU32_Vec4);       
        ASSERT_LOADED(PushFont);               
        ASSERT_LOADED(PopFont); 			    
        ASSERT_LOADED(EndChild);               
        ASSERT_LOADED(PushItemWidth);          
        ASSERT_LOADED(PopItemWidth);           
        ASSERT_LOADED(InputTextWithHint);      
        ASSERT_LOADED(GetCursorPosY); 	        
        ASSERT_LOADED(SetWindowSize_Vec2);     
        ASSERT_LOADED(SetWindowSize_Str);      
        ASSERT_LOADED(SetWindowSize_WindowPtr);
        ASSERT_LOADED(BeginTabBar);            
        ASSERT_LOADED(EndTabBar);              
        ASSERT_LOADED(BeginTabItem);           
        ASSERT_LOADED(EndTabItem);             
        ASSERT_LOADED(Checkbox);               
        ASSERT_LOADED(Dummy);                  
        ASSERT_LOADED(TextUnformatted);        
        ASSERT_LOADED(SetScrollHereY);         
        ASSERT_LOADED(InputText);              
        ASSERT_LOADED(BeginCombo);             
        ASSERT_LOADED(EndCombo);               
        ASSERT_LOADED(BeginDisabled);          
        ASSERT_LOADED(EndDisabled);            
        ASSERT_LOADED(Separator);              
        ASSERT_LOADED(GetTextLineHeight);	    
        ASSERT_LOADED(SetCursorPosY);          
        ASSERT_LOADED(SetNextItemWidth);       
        ASSERT_LOADED(SetNextWindowPos);       
        ASSERT_LOADED(TableSetupColumn);       
        ASSERT_LOADED(TableSetupScrollFreeze); 
        ASSERT_LOADED(TableHeadersRow);        
        ASSERT_LOADED(TableGetSortSpecs);      
        ASSERT_LOADED(DragFloat);              
        ASSERT_LOADED(GetFontSize);            
        ASSERT_LOADED(InvisibleButton);        
        ASSERT_LOADED(IsItemHovered);          
        ASSERT_LOADED(Indent);                 
        ASSERT_LOADED(Unindent);               
        ASSERT_LOADED(SeparatorText);          
        ASSERT_LOADED(CollapsingHeader_TreeNodeFlags);
        ASSERT_LOADED(CollapsingHeader_BoolPtr);
        ASSERT_LOADED(SetTooltip);             
        ASSERT_LOADED(IsItemClicked);          
        ASSERT_LOADED(IsMouseHoveringRect);    
        ASSERT_LOADED(GetFrameHeight);         
        ASSERT_LOADED(ProgressBar);            
        ASSERT_LOADED(GetIO);              
        ASSERT_LOADED(IsMouseClicked_Bool);    
        ASSERT_LOADED(IsWindowHovered);        
        ASSERT_LOADED(IsMouseReleased_Nil);    
        ASSERT_LOADED(ColorConvertRGBtoHSV);   
        ASSERT_LOADED(ColorConvertHSVtoRGB);   
        ASSERT_LOADED(IsKeyDown_Nil);          
		ASSERT_LOADED(IsKeyPressed_Bool);
        ASSERT_LOADED(IsItemActive);           
        ASSERT_LOADED(VSliderFloat);           
        ASSERT_LOADED(TableSetColumnIndex);    
        ASSERT_LOADED(ArrowButton);            
        ASSERT_LOADED(GetCurrentWindow);       
        ASSERT_LOADED(SetCursorPos);           
        ASSERT_LOADED(TableSetBgColor);        
        ASSERT_LOADED(BeginMenuBar);           
        ASSERT_LOADED(EndMenuBar);             
        ASSERT_LOADED(SliderInt);              
        ASSERT_LOADED(MenuItem_Bool);          
        ASSERT_LOADED(MenuItem_BoolPtr);       
        ASSERT_LOADED(SetNextWindowSizeConstraints);
        ASSERT_LOADED(GetCurrentContext);   
        ASSERT_LOADED(StyleColorsClassicByArg);
		ASSERT_LOADED(StyleColorsDarkByArg);
        ASSERT_LOADED(GetVersion);
		ASSERT_LOADED(BeginPopupContextItem);
		ASSERT_LOADED(EndPopup);
		ASSERT_LOADED(TableGetRowIndex);
		ASSERT_LOADED(GetFrameHeightWithSpacing);
		ASSERT_LOADED(ShowStyleEditor);
		ASSERT_LOADED(ShowDemoWindow);
		ASSERT_LOADED(SetItemDefaultFocus);
		ASSERT_LOADED(GetMousePosByArg);
		ASSERT_LOADED(GetCurrentTable);
		ASSERT_LOADED(SetNextItemAllowOverlap);
		ASSERT_LOADED(SetCursorScreenPos);
        ASSERT_LOADED(ColorEdit4);
        ASSERT_LOADED(DragFloat4);

        ASSERT_LOADED(FontAtlas_AddFontFromFileTTF);
        ASSERT_LOADED(DrawList_AddText_Vec2);  
        ASSERT_LOADED(DrawList_AddText_FontPtr);
        ASSERT_LOADED(DrawList_AddCircleFilled);
        ASSERT_LOADED(DrawList_AddCircle);     
        ASSERT_LOADED(DrawList_AddLine);       
        ASSERT_LOADED(DrawList_AddRectFilled); 
        ASSERT_LOADED(DrawList_AddRect);       
        ASSERT_LOADED(GuiViewport_GetCenter);     
        ASSERT_LOADED(GuiWindow_GetID_Str);       
        ASSERT_LOADED(GuiWindow_GetID_Ptr);       
        ASSERT_LOADED(GuiWindow_GetID_Int);       

        ASSERT_LOADED(RenderBullet   ); 
        ASSERT_LOADED(LogRenderedText); 
        ASSERT_LOADED(RenderText     ); 
        ASSERT_LOADED(ItemSize_Vec2  ); 
        ASSERT_LOADED(ItemSize_Rect  ); 
        ASSERT_LOADED(ItemAdd        ); 
        ASSERT_LOADED(ButtonBehavior ); 
        ASSERT_LOADED(MarkItemEdited ); 

        ASSERT_LOADED(GetWindowDrawListOpaque);
        ASSERT_LOADED(GetMainViewportOpaque);	
        ASSERT_LOADED(GetStylePtr);         
        ASSERT_LOADED(GetCursorPosByArg);
		ASSERT_LOADED(CalcTextSizeByArg);

        // IMPORTANT - Make sure ImGui Version matches expected

        constexpr const char* requiredVersion = "1.92.0"; // TODO: Pull this from Reframework's ImGui dependency.
        std::string version = CImGui::GetVersion();
        assert(version == requiredVersion && "ImGui Version Mismatch");
        if (version != requiredVersion) {
            std::string errMessage = std::format(
                "FATAL ERROR: ImGui Version Mismatch: Found: {}, Required: {}. Ensure the latest version of reframework is installed.",
                version, 
                requiredVersion);
            MessageBoxA(0, errMessage.c_str(), "Kana's Body Framework - Init Failed", 0);
        }
    }

    // TODO: Any functions here need to be fetched from the DLL
        // Custom Function Wrappers
    struct ImDrawListTransparent {
        ImDrawList* drawList;
        ImDrawListTransparent(ImDrawList* dl) : drawList(dl) {}

        void AddText(const ImVec2& pos, ImU32 col, const char* text_begin, const char* text_end = nullptr) {
            DrawList_AddText(drawList, pos, col, text_begin, text_end);
        }

        void AddText(ImFont* font, float font_size, const ImVec2 pos, ImU32 col, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect) {
            DrawList_AddText(drawList, font, font_size, pos, col, text_begin, text_end, wrap_width, cpu_fine_clip_rect);
        }

        void AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments = 0) {
            DrawList_AddCircleFilled(drawList, center, radius, col, num_segments);
		}

        void AddCircle(const ImVec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f) {
            DrawList_AddCircle(drawList, center, radius, col, num_segments, thickness);
		}

        void AddLine(const ImVec2& p1, const ImVec2& p2, ImU32 col, float thickness = 1.0f) {
			DrawList_AddLine(drawList, p1, p2, col, thickness);
        }

        void AddRectFilled(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0) {
			DrawList_AddRectFilled(drawList, p_min, p_max, col, rounding, flags);
        }

        void AddRect(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, float rounding = 0.0f, ImDrawFlags flags = 0, float thickness = 1.0f) {
			DrawList_AddRect(drawList, p_min, p_max, col, rounding, flags, thickness);
        }
    };

    inline std::unique_ptr<ImDrawListTransparent> GetWindowDrawList() {
        ImDrawList* dl = GetWindowDrawListOpaque();
        return std::make_unique<ImDrawListTransparent>(dl);
    }

    struct ImGuiViewportTransparent {
        ImGuiViewport* viewport;
        ImGuiViewportTransparent(ImGuiViewport* vp) : viewport(vp) {}

        ImVec2 GetCenter() const {
            ImVec2 out = ImVec2(0, 0);
            GuiViewport_GetCenter(&out, viewport);
            return out;
        }
	};

    inline std::unique_ptr<ImGuiViewportTransparent> GetMainViewport() {
        ImGuiViewport* vp = GetMainViewportOpaque();
        return std::make_unique<ImGuiViewportTransparent>(vp);
	}

    inline ImGuiStyle& GetStyle() {
        ImGuiStyle* style = GetStylePtr();
        assert(style != nullptr && "GetStylePtr returned nullptr");
		return *style;
    }

    inline ImVec2 GetCursorPos() {
        ImVec2 pos = ImVec2(0, 0);
        GetCursorPosByArg(&pos);
        return pos;
	}

    inline ImVec2 CalcTextSize(const char* text, const char* text_end = nullptr, bool hide_text_after_hash = false, float wrap_width = -1.0f) {
        ImVec2 size = ImVec2(0, 0);
        CalcTextSizeByArg(&size, text, text_end, hide_text_after_hash, wrap_width);
        return size;
    }

    inline ImVec2 GetContentRegionAvail() {
        ImVec2 avail = ImVec2(0, 0);
        GetContentRegionAvailByArg(&avail);
        return avail;
    }

    inline ImVec2 GetWindowSize() {
        ImVec2 size = ImVec2(0, 0);
        GetWindowSizeByArg(&size);
        return size;
    }

    inline ImVec2 GetCursorScreenPos() {
        ImVec2 pos = ImVec2(0, 0);
        GetCursorScreenPosByArg(&pos);
        return pos;
    }

    inline ImVec2 GetItemRectMin() {
        ImVec2 min = ImVec2(0, 0);
        GetItemRectMinByArg(&min);
        return min;
	}

    inline ImVec2 GetMousePos() {
        ImVec2 pos = ImVec2(0, 0);
        GetMousePosByArg(&pos);
        return pos;
    }

    inline ImRect TableGetCellBgRect(const ImGuiTable* table, int column_n) {
        ImRect rect = ImRect();
        TableGetCellBgRectByArg(&rect, table, column_n);
        return rect;
    }

    inline ImFont* AddFontFromFileTTF(ImFontAtlas* self, const char* filename, float size_pixels = 0.0f, const ImFontConfig* font_cfg = nullptr, const ImWchar* glyph_ranges = nullptr) {
        return FontAtlas_AddFontFromFileTTF(self, filename, size_pixels, font_cfg, glyph_ranges);
	}

    struct ImGuiWindowUtils {
        inline static ImGuiID GetID(ImGuiWindow* self, const char* str, const char* str_end = nullptr) {
            return GuiWindow_GetID_Str(self, str, str_end);
        }
        inline static ImGuiID GetID(ImGuiWindow* self, const void* ptr) {
            return GuiWindow_GetID_Ptr(self, ptr);
        }
        inline static ImGuiID GetID(ImGuiWindow* self, int n) {
            return GuiWindow_GetID_Int(self, n);
		}
    };

}