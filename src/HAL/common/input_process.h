#pragma once
#include<unordered_map>
#include"core/type.h"
#include"Core/util/singleton.h"

PROJECT_NAMESPACE_BEGIN

enum class KeyboardType
{
    KEY_UNKNOWN = -1,
    KEY_SPACE = 32,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_MINUS = 45,
    KEY_PERIOD = 46,
    KEY_SLASH = 47,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_SEMICOLON = 59,
    KEY_EQUAL = 61,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_LEFT_BRACKET = 91,
    KEY_BACKSLASH = 92,
    KEY_RIGHT_BRACKET = 93,
    KEY_GRAVE_ACCENT = 96,
    KEY_WORLD_1 = 161,
    KEY_WORLD_2 = 162,
    KEY_ESCAPE = 256,
    KEY_ENTER = 257,
    KEY_TAB = 258,
    KEY_BACKSPACE = 259,
    KEY_INSERT = 260,
    KEY_DELETE = 261,
    KEY_RIGHT = 262,
    KEY_LEFT = 263,
    KEY_DOWN = 264,
    KEY_UP = 265,
    KEY_PAGE_UP = 266,
    KEY_PAGE_DOWN = 267,
    KEY_HOME = 268,
    KEY_END = 269,
    KEY_CAPS_LOCK = 280,
    KEY_SCROLL_LOCK = 281,
    KEY_NUM_LOCK = 282,
    KEY_PRi32_SCREEN = 283,
    KEY_PAUSE = 284,
    KEY_F1 = 290,
    KEY_F2 = 291,
    KEY_F3 = 292,
    KEY_F4 = 293,
    KEY_F5 = 294,
    KEY_F6 = 295,
    KEY_F7 = 296,
    KEY_F8 = 297,
    KEY_F9 = 298,
    KEY_F10 = 299,
    KEY_F11 = 300,
    KEY_F12 = 301,
    KEY_F13 = 302,
    KEY_F14 = 303,
    KEY_F15 = 304,
    KEY_F16 = 305,
    KEY_F17 = 306,
    KEY_F18 = 307,
    KEY_F19 = 308,
    KEY_F20 = 309,
    KEY_F21 = 310,
    KEY_F22 = 311,
    KEY_F23 = 312,
    KEY_F24 = 313,
    KEY_F25 = 314,
    KEY_KP_0 = 320,
    KEY_KP_1 = 321,
    KEY_KP_2 = 322,
    KEY_KP_3 = 323,
    KEY_KP_4 = 324,
    KEY_KP_5 = 325,
    KEY_KP_6 = 326,
    KEY_KP_7 = 327,
    KEY_KP_8 = 328,
    KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330,
    KEY_KP_DIVIDE = 331,
    KEY_KP_MULTIPLY = 332,
    KEY_KP_SUBTRACT = 333,
    KEY_KP_ADD = 334,
    KEY_KP_ENTER = 335,
    KEY_KP_EQUAL = 336,
    KEY_LEFT_SHIFT = 340,
    KEY_LEFT_CONTROL = 341,
    KEY_LEFT_ALT = 342,
    KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344,
    KEY_RIGHT_CONTROL = 345,
    KEY_RIGHT_ALT = 346,
    KEY_RIGHT_SUPER = 347,
    KEY_MENU = 348,
    KEY_MOD_SHIFT = 0x0001,
    KEY_MOD_CONTROL = 0x0002,
    KEY_MOD_ALT = 0x0004,
    KEY_MOD_SUPER = 0x0008,
    KEY_MOD_CAPS_LOCK = 0x0010,
    KEY_MOD_NUM_LOCK = 0x0020,
};

enum class MouseType
{
    MOUSE_BUTTON_1 = 0,
    MOUSE_BUTTON_2 = 1,
    MOUSE_BUTTON_3 = 2,
    MOUSE_BUTTON_4 = 3,
    MOUSE_BUTTON_5 = 4,
    MOUSE_BUTTON_6 = 5,
    MOUSE_BUTTON_7 = 6,
    MOUSE_BUTTON_8 = 7,
    MOUSE_BUTTON_LAST = MOUSE_BUTTON_8,
    MOUSE_BUTTON_LEFT = MOUSE_BUTTON_1,
    MOUSE_BUTTON_RIGHT = MOUSE_BUTTON_2,
    MOUSE_BUTTON_MIDDLE = MOUSE_BUTTON_3,
};

class InputManager : public Singleton<InputManager>
{
public:
    InputManager() {}
    void Init()
    {
        s_KeyboardTypesMap[0x00B] = KeyboardType::KEY_0;
        s_KeyboardTypesMap[0x002] = KeyboardType::KEY_1;
        s_KeyboardTypesMap[0x003] = KeyboardType::KEY_2;
        s_KeyboardTypesMap[0x004] = KeyboardType::KEY_3;
        s_KeyboardTypesMap[0x005] = KeyboardType::KEY_4;
        s_KeyboardTypesMap[0x006] = KeyboardType::KEY_5;
        s_KeyboardTypesMap[0x007] = KeyboardType::KEY_6;
        s_KeyboardTypesMap[0x008] = KeyboardType::KEY_7;
        s_KeyboardTypesMap[0x009] = KeyboardType::KEY_8;
        s_KeyboardTypesMap[0x00A] = KeyboardType::KEY_9;
        s_KeyboardTypesMap[0x01E] = KeyboardType::KEY_A;
        s_KeyboardTypesMap[0x030] = KeyboardType::KEY_B;
        s_KeyboardTypesMap[0x02E] = KeyboardType::KEY_C;
        s_KeyboardTypesMap[0x020] = KeyboardType::KEY_D;
        s_KeyboardTypesMap[0x012] = KeyboardType::KEY_E;
        s_KeyboardTypesMap[0x021] = KeyboardType::KEY_F;
        s_KeyboardTypesMap[0x022] = KeyboardType::KEY_G;
        s_KeyboardTypesMap[0x023] = KeyboardType::KEY_H;
        s_KeyboardTypesMap[0x017] = KeyboardType::KEY_I;
        s_KeyboardTypesMap[0x024] = KeyboardType::KEY_J;
        s_KeyboardTypesMap[0x025] = KeyboardType::KEY_K;
        s_KeyboardTypesMap[0x026] = KeyboardType::KEY_L;
        s_KeyboardTypesMap[0x032] = KeyboardType::KEY_M;
        s_KeyboardTypesMap[0x031] = KeyboardType::KEY_N;
        s_KeyboardTypesMap[0x018] = KeyboardType::KEY_O;
        s_KeyboardTypesMap[0x019] = KeyboardType::KEY_P;
        s_KeyboardTypesMap[0x010] = KeyboardType::KEY_Q;
        s_KeyboardTypesMap[0x013] = KeyboardType::KEY_R;
        s_KeyboardTypesMap[0x01F] = KeyboardType::KEY_S;
        s_KeyboardTypesMap[0x014] = KeyboardType::KEY_T;
        s_KeyboardTypesMap[0x016] = KeyboardType::KEY_U;
        s_KeyboardTypesMap[0x02F] = KeyboardType::KEY_V;
        s_KeyboardTypesMap[0x011] = KeyboardType::KEY_W;
        s_KeyboardTypesMap[0x02D] = KeyboardType::KEY_X;
        s_KeyboardTypesMap[0x015] = KeyboardType::KEY_Y;
        s_KeyboardTypesMap[0x02C] = KeyboardType::KEY_Z;
        s_KeyboardTypesMap[0x028] = KeyboardType::KEY_APOSTROPHE;
        s_KeyboardTypesMap[0x02B] = KeyboardType::KEY_BACKSLASH;
        s_KeyboardTypesMap[0x033] = KeyboardType::KEY_COMMA;
        s_KeyboardTypesMap[0x00D] = KeyboardType::KEY_EQUAL;
        s_KeyboardTypesMap[0x029] = KeyboardType::KEY_GRAVE_ACCENT;
        s_KeyboardTypesMap[0x01A] = KeyboardType::KEY_LEFT_BRACKET;
        s_KeyboardTypesMap[0x00C] = KeyboardType::KEY_MINUS;
        s_KeyboardTypesMap[0x034] = KeyboardType::KEY_PERIOD;
        s_KeyboardTypesMap[0x01B] = KeyboardType::KEY_RIGHT_BRACKET;
        s_KeyboardTypesMap[0x027] = KeyboardType::KEY_SEMICOLON;
        s_KeyboardTypesMap[0x035] = KeyboardType::KEY_SLASH;
        s_KeyboardTypesMap[0x056] = KeyboardType::KEY_WORLD_2;
        s_KeyboardTypesMap[0x00E] = KeyboardType::KEY_BACKSPACE;
        s_KeyboardTypesMap[0x153] = KeyboardType::KEY_DELETE;
        s_KeyboardTypesMap[0x14F] = KeyboardType::KEY_END;
        s_KeyboardTypesMap[0x01C] = KeyboardType::KEY_ENTER;
        s_KeyboardTypesMap[0x001] = KeyboardType::KEY_ESCAPE;
        s_KeyboardTypesMap[0x147] = KeyboardType::KEY_HOME;
        s_KeyboardTypesMap[0x152] = KeyboardType::KEY_INSERT;
        s_KeyboardTypesMap[0x15D] = KeyboardType::KEY_MENU;
        s_KeyboardTypesMap[0x151] = KeyboardType::KEY_PAGE_DOWN;
        s_KeyboardTypesMap[0x149] = KeyboardType::KEY_PAGE_UP;
        s_KeyboardTypesMap[0x045] = KeyboardType::KEY_PAUSE;
        s_KeyboardTypesMap[0x146] = KeyboardType::KEY_PAUSE;
        s_KeyboardTypesMap[0x039] = KeyboardType::KEY_SPACE;
        s_KeyboardTypesMap[0x00F] = KeyboardType::KEY_TAB;
        s_KeyboardTypesMap[0x03A] = KeyboardType::KEY_CAPS_LOCK;
        s_KeyboardTypesMap[0x145] = KeyboardType::KEY_NUM_LOCK;
        s_KeyboardTypesMap[0x046] = KeyboardType::KEY_SCROLL_LOCK;
        s_KeyboardTypesMap[0x03B] = KeyboardType::KEY_F1;
        s_KeyboardTypesMap[0x03C] = KeyboardType::KEY_F2;
        s_KeyboardTypesMap[0x03D] = KeyboardType::KEY_F3;
        s_KeyboardTypesMap[0x03E] = KeyboardType::KEY_F4;
        s_KeyboardTypesMap[0x03F] = KeyboardType::KEY_F5;
        s_KeyboardTypesMap[0x040] = KeyboardType::KEY_F6;
        s_KeyboardTypesMap[0x041] = KeyboardType::KEY_F7;
        s_KeyboardTypesMap[0x042] = KeyboardType::KEY_F8;
        s_KeyboardTypesMap[0x043] = KeyboardType::KEY_F9;
        s_KeyboardTypesMap[0x044] = KeyboardType::KEY_F10;
        s_KeyboardTypesMap[0x057] = KeyboardType::KEY_F11;
        s_KeyboardTypesMap[0x058] = KeyboardType::KEY_F12;
        s_KeyboardTypesMap[0x064] = KeyboardType::KEY_F13;
        s_KeyboardTypesMap[0x065] = KeyboardType::KEY_F14;
        s_KeyboardTypesMap[0x066] = KeyboardType::KEY_F15;
        s_KeyboardTypesMap[0x067] = KeyboardType::KEY_F16;
        s_KeyboardTypesMap[0x068] = KeyboardType::KEY_F17;
        s_KeyboardTypesMap[0x069] = KeyboardType::KEY_F18;
        s_KeyboardTypesMap[0x06A] = KeyboardType::KEY_F19;
        s_KeyboardTypesMap[0x06B] = KeyboardType::KEY_F20;
        s_KeyboardTypesMap[0x06C] = KeyboardType::KEY_F21;
        s_KeyboardTypesMap[0x06D] = KeyboardType::KEY_F22;
        s_KeyboardTypesMap[0x06E] = KeyboardType::KEY_F23;
        s_KeyboardTypesMap[0x076] = KeyboardType::KEY_F24;
        s_KeyboardTypesMap[0x038] = KeyboardType::KEY_LEFT_ALT;
        s_KeyboardTypesMap[0x01D] = KeyboardType::KEY_LEFT_CONTROL;
        s_KeyboardTypesMap[0x02A] = KeyboardType::KEY_LEFT_SHIFT;
        s_KeyboardTypesMap[0x15B] = KeyboardType::KEY_LEFT_SUPER;
        s_KeyboardTypesMap[0x137] = KeyboardType::KEY_PRi32_SCREEN;
        s_KeyboardTypesMap[0x138] = KeyboardType::KEY_RIGHT_ALT;
        s_KeyboardTypesMap[0x11D] = KeyboardType::KEY_RIGHT_CONTROL;
        s_KeyboardTypesMap[0x036] = KeyboardType::KEY_RIGHT_SHIFT;
        s_KeyboardTypesMap[0x15C] = KeyboardType::KEY_RIGHT_SUPER;
        s_KeyboardTypesMap[0x150] = KeyboardType::KEY_DOWN;
        s_KeyboardTypesMap[0x14B] = KeyboardType::KEY_LEFT;
        s_KeyboardTypesMap[0x14D] = KeyboardType::KEY_RIGHT;
        s_KeyboardTypesMap[0x148] = KeyboardType::KEY_UP;

        s_KeyboardTypesMap[0x052] = KeyboardType::KEY_KP_0;
        s_KeyboardTypesMap[0x04F] = KeyboardType::KEY_KP_1;
        s_KeyboardTypesMap[0x050] = KeyboardType::KEY_KP_2;
        s_KeyboardTypesMap[0x051] = KeyboardType::KEY_KP_3;
        s_KeyboardTypesMap[0x04B] = KeyboardType::KEY_KP_4;
        s_KeyboardTypesMap[0x04C] = KeyboardType::KEY_KP_5;
        s_KeyboardTypesMap[0x04D] = KeyboardType::KEY_KP_6;
        s_KeyboardTypesMap[0x047] = KeyboardType::KEY_KP_7;
        s_KeyboardTypesMap[0x048] = KeyboardType::KEY_KP_8;
        s_KeyboardTypesMap[0x049] = KeyboardType::KEY_KP_9;
        s_KeyboardTypesMap[0x04E] = KeyboardType::KEY_KP_ADD;
        s_KeyboardTypesMap[0x053] = KeyboardType::KEY_KP_DECIMAL;
        s_KeyboardTypesMap[0x135] = KeyboardType::KEY_KP_DIVIDE;
        s_KeyboardTypesMap[0x11C] = KeyboardType::KEY_KP_ENTER;
        s_KeyboardTypesMap[0x037] = KeyboardType::KEY_KP_MULTIPLY;
        s_KeyboardTypesMap[0x04A] = KeyboardType::KEY_KP_SUBTRACT;
    }

    RS_FORCEINLINE KeyboardType GetKeyFromKeyCode(i32 keyCode)
    {
        auto it = s_KeyboardTypesMap.find(keyCode);
        if (it == s_KeyboardTypesMap.end())
        {
            return KeyboardType::KEY_UNKNOWN;
        }
        return it->second;
    }

    RS_FORCEINLINE void Reset()
    {
        s_MouseDelta = 0;
        s_IsMouseMoveing = false;
    }

    RS_FORCEINLINE bool IsMouseDown(MouseType type)
    {
        auto it = s_MouseActions.find((i32)type);
        if (it == s_MouseActions.end())
        {
            return false;
        }
        return it->second == true;
    }

    RS_FORCEINLINE bool IsMouseUp(MouseType type)
    {
        auto it = s_MouseActions.find((i32)type);
        if (it == s_MouseActions.end())
        {
            return false;
        }
        return it->second == false;
    }

    RS_FORCEINLINE bool IsKeyDown(KeyboardType key)
    {
        auto it = s_KeyActions.find((i32)key);
        if (it == s_KeyActions.end())
        {
            return false;
        }
        return it->second == true;
    }

    RS_FORCEINLINE bool IsKeyUp(KeyboardType key)
    {
        auto it = s_KeyActions.find((i32)key);
        if (it == s_KeyActions.end())
        {
            return false;
        }
        return it->second == false;
    }

    RS_FORCEINLINE const I32Vector2& GetMousePosition()
    {
        return s_MouseLocation;
    }

    RS_FORCEINLINE float GetMouseDelta()
    {
        return s_MouseDelta;
    }
    RS_FORCEINLINE bool IsMouseMoving()
    {
        return s_IsMouseMoveing;
    }

protected:

    //friend class Application;

    void static OnKeyDown(KeyboardType key) {}

    void static OnKeyUp(KeyboardType key) {}

    void static OnMouseDown(MouseType type, const I32Vector2& pos) {}

    void static OnMouseUp(MouseType type, const I32Vector2& pos) {}
    void static OnMouseMove(const I32Vector2& pos) {}

    void static OnMouseWheel(const float delta, const I32Vector2& pos) {}

private:

    bool     s_IsMouseMoveing{};
    float    s_MouseDelta{};
    I32Vector2  s_MouseLocation{};

    std::unordered_map<i32, bool>              s_KeyActions{};
    std::unordered_map<i32, bool>              s_MouseActions{};
    std::unordered_map<i32, KeyboardType>      s_KeyboardTypesMap{};
};

struct WindowSizeLimits
{
    AR_THIS_CLASS(WindowSizeLimits);
    AR_ATTRIBUTE(f32, minWidth);
    AR_ATTRIBUTE(f32, minHeight);
    AR_ATTRIBUTE(f32, maxWidth);
    AR_ATTRIBUTE(f32, maxHeight);
};


class WinMessageHandler
{
public:

    virtual bool OnKeyDown(const KeyboardType key)
    {

        return false;
    }

    virtual bool OnKeyUp(const KeyboardType key)
    {

        return false;
    }

    virtual bool OnMouseDown(MouseType type, const I32Vector2& pos)
    {

        return false;
    }

    virtual bool OnMouseUp(MouseType type, const I32Vector2& pos)
    {

        return false;
    }

    virtual bool OnMouseDoubleClick(MouseType type, const I32Vector2& pos)
    {

        return false;
    }

    virtual bool OnMouseWheel(const float delta, const I32Vector2& pos)
    {

        return false;
    }

    virtual bool OnMouseMove(const I32Vector2& pos)
    {
        return false;
    }

    virtual bool OnTouchStarted(const std::vector<I32Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchMoved(const std::vector<I32Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchEnded(const std::vector<I32Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchForceChanged(const std::vector<I32Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchFirstMove(const std::vector<I32Vector2>& locations)
    {

        return false;
    }

    virtual bool OnSizeChanged(const i32 width, const i32 height)
    {

        return false;
    }

    virtual void OnOSPai32()
    {

    }

    virtual WindowSizeLimits GetSizeLimitsForWindow() const
    {
        return WindowSizeLimits{};
    }

    virtual void OnResizingWindow()
    {

    }

    virtual bool BeginReshapingWindow()
    {

        return true;
    }

    virtual void FinishedReshapingWindow()
    {

    }

    virtual void HandleDPIScaleChanged()
    {

    }

    virtual void SignalSystemDPIChanged()
    {

    }

    virtual void OnMovedWindow(const i32 x, const i32 y)
    {

    }

    virtual void OnWindowClose()
    {

    }

    virtual void OnRequestingExit()
    {

    }
};

PROJECT_NAMESPACE_END