#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define UNICODE 1
#include<windows.h>
#include<windowsx.h>

#include"../common/top_window.h"
#include"../common/input_process.h"

PROJECT_NAMESPACE_BEGIN

// message
class WinTopWindow : public TopWindow, public WinMessageHandler
{
public:
    WinTopWindow(std::string const& title, i32 width, i32 height)
        : TopWindow(title, width, height)
    {
        WNDCLASSEX wc;
        std::memset(&wc, 0, sizeof(WNDCLASSEX));
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wc.lpfnWndProc = AppWndProc;
        wc.hInstance = GInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"WinMainWindow";
        RegisterClassEx(&wc);
    }

    void initialize()
    {
        const i32Rect& rect = getRect();
        RECT windowRect = { 0, 0, rect.width, rect.height };
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
            GInstance,
            nullptr
        );

        _Application = this;
    }

    void finalize() {
        _window = nullptr;
    }

    virtual void createVkSurface(VkInstance instance, VkSurfaceKHR* outSurface) override
    {
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
        ZeroVulkanStruct(surfaceCreateInfo, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
        surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
        surfaceCreateInfo.hwnd = (HWND)_window;
        //vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, outSurface);
    }

    virtual void* getWindowHandle() override
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
            _Application->OnKeyDown(key);
            return 0;
        }
        case WM_KEYUP:
        {
            const i32 keycode = HIWORD(lParam) & 0x1FF;
            KeyboardType key = SInputManager.GetKeyFromKeyCode(keycode);
            _Application->OnKeyUp(key);
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

            Vector2 pos((float)x, (float)y);

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
                _Application->OnMouseDown(button, pos);
            }
            else
            {
                _Application->OnMouseUp(button, pos);
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
            Vector2 pos((float)x, (float)y);
            _Application->OnMouseMove(pos);
            return 0;
        }
        case WM_MOUSEWHEEL:
        {
            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);
            Vector2 pos((float)x, (float)y);
            _Application->OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
            return 0;
        }
        case WM_MOUSEHWHEEL:
        {
            const i32 x = GET_X_LPARAM(lParam);
            const i32 y = GET_Y_LPARAM(lParam);
            Vector2 pos((float)x, (float)y);
            _Application->OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
            return 0;
        }
        case WM_SIZE:
        {
            _Application->OnSizeChanged(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        case WM_PAINT:
        {
            _Application->OnOSPai32();
            return 0;
        }
        case WM_CLOSE:
        {
            _Application->OnRequestingExit();
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

    virtual bool OnMouseDown(MouseType type, const Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseUp(MouseType type, const Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseDoubleClick(MouseType type, const Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseWheel(const float delta, const Vector2& pos) override
    {

        return false;
    }

    virtual bool OnMouseMove(const Vector2& pos) override
    {
        return false;
    }

    virtual bool OnTouchStarted(const std::vector<Vector2>& locations) override
    {

        return false;
    }

    virtual bool OnTouchMoved(const std::vector<Vector2>& locations) override
    {

        return false;
    }

    virtual bool OnTouchEnded(const std::vector<Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchForceChanged(const std::vector<Vector2>& locations)
    {

        return false;
    }

    virtual bool OnTouchFirstMove(const std::vector<Vector2>& locations)
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
    WindowMode          _windowMode{ WindowMode::Windowed };
    bool                _visible{ false };
    float               _aspectRatio{ 0.0f };
    float               _DPIScaleFactor{ 0.0f };
    bool                _minimized{ false };
    bool                _maximized{ false };
    static WinMessageHandler* _Application;
};
WinMessageHandler* WinWindow::_Application = nullptr;

PROJECT_NAMESPACE_END