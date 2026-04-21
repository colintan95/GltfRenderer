#include "App.h"

#include <windows.h>

static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
  }

  return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int cmdShow) {
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

  ShowWindow(hwnd, cmdShow);

  App app(hwnd);

  MSG msg = {};
  while (msg.message != WM_QUIT) {
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      app.Render();

      Sleep(16);
    }
  }
}