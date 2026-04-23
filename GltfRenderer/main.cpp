#include "App.h"

#include <windows.h>

#include <iostream>

static bool s_LeftBtnDown = false;

static std::optional<POINT> s_PrevMousePos = std::nullopt;

static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_LBUTTONDOWN:
      s_LeftBtnDown = true;
      break;

    case WM_LBUTTONUP:
      s_PrevMousePos = std::nullopt;
      s_LeftBtnDown = false;
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}

int main() {
  HINSTANCE hinstance = GetModuleHandle(0);

  static constexpr wchar_t kWindowName[] = L"GltfRenderer";

  WNDCLASSEX wndClass = {
    .cbSize = sizeof(WNDCLASSEX),
    .style = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc = WindowProc,
    .hInstance = hinstance,
    .hCursor = LoadCursor(nullptr, IDC_ARROW),
    .lpszClassName = kWindowName
  };

  RegisterClassEx(&wndClass);

  static constexpr int kWindowWidth = 1024;
  static constexpr int kWindowHeight = 768;

  auto hwnd = CreateWindow(wndClass.lpszClassName, kWindowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
      CW_USEDEFAULT, kWindowWidth, kWindowHeight, nullptr, nullptr, hinstance, nullptr);

  ShowWindow(hwnd, SW_SHOW);

  App app;
  if (!app.Init(hwnd)) {
    return -1;
  }

  MSG msg = {};
  while (msg.message != WM_QUIT) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (s_LeftBtnDown) {
        POINT pos;
        GetCursorPos(&pos);

        static constexpr float kRadPerPixel = 0.01f;

        if (s_PrevMousePos) {
          float xDelta = static_cast<float>(pos.x - s_PrevMousePos->x);
          float yDelta = static_cast<float>(pos.y - s_PrevMousePos->y);

          app.AddYaw(Angle::Rad(xDelta * kRadPerPixel));
          app.AddPitch(Angle::Rad(yDelta * kRadPerPixel));
        }
         
        s_PrevMousePos = pos;
      }

      app.Render();

      Sleep(16);
    }
  }

  return 0;
}