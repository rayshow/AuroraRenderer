#pragma once

#define UNICODE 1
#include<windows.h>
#include<windowsx.h>
#include "vulkan/vulkan_win32.h"

#include"../common/top_window.h"
#include"../common/input_process.h"
#include"../generated/vk_common.h"
#include"win_global.h"



PROJECT_NAMESPACE_BEGIN

// message
class WinTopWindow 
    : public CommonTopWindow< WinTopWindow >
    , public WinMessageHandler
{
public:
    WinTopWindow()
    {
        WNDCLASSEX wc;
        std::memset(&wc, 0, sizeof(WNDCLASSEX));
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = AppWndProc;
        wc.hInstance = GWinInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"WinMainWindow";
        RegisterClassEx(&wc);
    }

    void initialize()
    {
        auto& rect = properties.get_rect();
        RECT windowRect = { 0, 0, rect.get_width(), rect.get_height() };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        _window = CreateWindow(
            L"WinMainWindow",
            L"WinMainWindow",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            GWinInstance,
            nullptr
        );

        _app = this;
    }

    void finalize() {
        _window = nullptr;
    }

    // surface
    VkSurfaceKHR createSurface(VkInstance instance, VkPhysicalDevice device) const
    {
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
        ZeroVulkanStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
        surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
        surfaceCreateInfo.hwnd = (HWND)_window;
        //vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, outSurface);
    }

    void* getWindowHandle()
    {
        return _window;
    }

    virtual void tick() {
        pumpMessages();
    }

    static i32 ProcessMessage(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
    {
        InputManager& SInputManager = InputManager::getInstance();
        switch (msg)
        {
        case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
            return 0;
        }
        case WM_KEYDOWN:
        {
            const i32 keycode = HIWORD(lParam) & 0x1FF;
            KeyboardType key = SInputManager.GetKeyFromKeyCode(keycode);
            _app->OnKeyDown(key);
            return 0;
        }
        case WM_KEYUP:
        {
            const i32 keycode = HIWORD(lParam) & 0x1FF;
            KeyboardType key = SInputManager.GetKeyFromKeyCode(keycode);
            _app->OnKeyUp(key);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            MouseType button = MouseType::MOUSE_BUTTON_LEFT;
            i32 action = 0;

            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);

            I32Vector2 pos{ x,y };

            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP)
            {
                button = MouseType::MOUSE_BUTTON_LEFT;
            }
            else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP)
            {
                button = MouseType::MOUSE_BUTTON_RIGHT;
            }
            else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP)
            {
                button = MouseType::MOUSE_BUTTON_MIDDLE;
            }
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
            {
                button = MouseType::MOUSE_BUTTON_4;
            }
            else
            {
                button = MouseType::MOUSE_BUTTON_5;
            }

            if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN)
            {
                action = 1;
                SetCapture(hwnd);
            }
            else
            {
                action = 0;
                ReleaseCapture();
            }

            if (action == 1)
            {
                _app->OnMouseDown(button, pos);
            }
            else
            {
                _app->OnMouseUp(button, pos);
            }

            if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONUP)
            {
                return TRUE;
            }

            return 0;
        }
        case WM_MOUSEMOVE:
        {
            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);
            I32Vector2 pos(x, y);
            _app->OnMouseMove(pos);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);
            I32Vector2 pos(x, y);
            _app->OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);
            I32Vector2 pos(x, y);
            _app->OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
            return 0;
        }
        case WM_SIZE:
        {
            _app->OnSizeChanged(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        case WM_PAINT:
        {
            _app->OnOSPai32();
            return 0;
        }
        case WM_CLOSE:
        {
            _app->OnRequestingExit();
            return 0;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        }

        return (i32)DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static LRESULT CALLBACK AppWndProc(HWND hwnd, u32 msg, WPARAM wParam, LPARAM lParam)
    {
        return ProcessMessage(hwnd, msg, wParam, lParam);
    }

    void minimize() {
        _minimized = true;
        _maximized = false;
    }

    void maximize() {
        _minimized = false;
        _maximized = true;
    }

    void show() {
        if (!_visible) {
            _visible = true;
            ShowWindow(_window, SW_SHOW);
        }
    }

    void hide() {
        if (_visible) {
            _visible = false;
            ShowWindow(_window, SW_HIDE);
        }
    }

    void pumpMessages()
    {
        MSG msg = {};
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    virtual bool OnKeyDown(const KeyboardType key) override
    {

        return false;
    }

    virtual bool OnKeyUp(const KeyboardType key) override
    {

        return false;
    }

    virtual bool OnMouseDown(MouseType type, const I32Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseUp(MouseType type, const I32Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseDoubleClick(MouseType type, const I32Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseWheel(const float delta, const I32Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseMove(const I32Vector2& pos) override
    {
        return false;
    }

    virtual bool OnTouchStarted(const std::vector<I32Vector2>& locations) override
    {

        return false;
    }

    virtual bool OnTouchMoved(const std::vector<I32Vector2>& locations) override
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
        return WindowSizeLimits();
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
        GRequestExit = true;
    }

private:
    HWND                _window{ 0 };
    bool                _visible{ false };
    float               _aspectRatio{ 0.0f };
    float               _DPIScaleFactor{ 0.0f };
    bool                _minimized{ false };
    bool                _maximized{ false };
    static WinMessageHandler* _app;
};
WinMessageHandler* WinTopWindow::_app = nullptr;

PROJECT_NAMESPACE_END