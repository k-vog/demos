#include <assert.h>
#include <stdio.h>

#include <windows.h>

#ifdef _MSC_VER
# pragma comment(lib, "gdi32.lib")
# pragma comment(lib, "user32.lib")
#endif

#define ARRLEN(arr) (sizeof(arr) / sizeof((arr)[0]))

static LPWSTR TempFormat(LPCWSTR fmt, ...)
{
  static WCHAR buf[1024];

  va_list va; va_start(va, fmt);
  _vsnwprintf(buf, ARRLEN(buf), fmt, va);
  va_end(va);

  return buf;
}

static void LogMessage(LPCWSTR fmt, ...)
{
  static WCHAR buf[1024];

  va_list va; va_start(va, fmt);
  _vsnwprintf(buf, ARRLEN(buf), fmt, va);
  va_end(va);

  wprintf(L"%ls\n", buf);
}

static void LogError(LPCWSTR szMessage)
{
  szMessage = szMessage ? szMessage : L"Error";

  LPWSTR szError = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPWSTR)&szError, 0, NULL);

  LogMessage(L"%ls: %ls", szMessage, szError);

  LocalFree(szError);
  DebugBreak();
}

static void PushFlagString(LPWSTR szBuffer, size_t length, LPCWSTR szFlag)
{
  if (szBuffer[0]) {
    wcsncat(szBuffer, L" | ", length);
  }
  wcsncat(szBuffer, szFlag, length);
}

static LPWSTR GetWindowStyleString(DWORD dwStyle)
{
  static WCHAR szBuffer[1024];

  szBuffer[0] = 0;

#define BIND_COMPOSITE_STYLE(style)                           \
    if ((dwStyle & style) == style) {                         \
      PushFlagString(szBuffer, ARRLEN(szBuffer), L"" #style); \
      dwStyle &= ~style;                                      \
    }
  BIND_COMPOSITE_STYLE(WS_OVERLAPPEDWINDOW);
  BIND_COMPOSITE_STYLE(WS_POPUPWINDOW);
  BIND_COMPOSITE_STYLE(WS_TILEDWINDOW);
#undef BIND_COMPOSITE_STYLE

  for (size_t i = 0; i < sizeof(dwStyle) * 8; ++i) {
    const DWORD dwFlag = 1 << i;
    if (dwStyle & dwFlag) {
      LPCWSTR szFlag = NULL;
#define BIND_STYLE(style) case style: { szFlag = L"" #style; } break;
      switch (dwFlag) {
        BIND_STYLE(WS_BORDER);
        BIND_STYLE(WS_CAPTION);
        BIND_STYLE(WS_CHILD);
        BIND_STYLE(WS_CLIPCHILDREN);
        BIND_STYLE(WS_CLIPSIBLINGS);
        BIND_STYLE(WS_DISABLED);
        BIND_STYLE(WS_DLGFRAME);
        BIND_STYLE(WS_HSCROLL);
        BIND_STYLE(WS_MAXIMIZE);
        BIND_STYLE(WS_MAXIMIZEBOX);
        BIND_STYLE(WS_MINIMIZE);
        BIND_STYLE(WS_MINIMIZEBOX);
        BIND_STYLE(WS_POPUP);
        BIND_STYLE(WS_SIZEBOX);
        BIND_STYLE(WS_SYSMENU);
        BIND_STYLE(WS_TILED);
        BIND_STYLE(WS_VISIBLE);
        BIND_STYLE(WS_VSCROLL);
        default: { szFlag = L"(unknown flag)"; } break;
      };
      PushFlagString(szBuffer, ARRLEN(szBuffer), szFlag);
#undef BIND_STYLE
    }
  }

  return szBuffer;
};

static LPCWSTR GetWindowExStyleString(DWORD dwExStyle)
{
  static WCHAR szBuffer[1024];

  szBuffer[0] = 0;

#define BIND_COMPOSITE_STYLE(style)                           \
    if ((dwExStyle & style) == style) {                       \
      PushFlagString(szBuffer, ARRLEN(szBuffer), L"" #style); \
      dwExStyle &= ~style;                                    \
    }
  BIND_COMPOSITE_STYLE(WS_EX_OVERLAPPEDWINDOW);
  BIND_COMPOSITE_STYLE(WS_EX_PALETTEWINDOW);
#undef BIND_COMPOSITE_STYLE

  for (size_t i = 0; i < sizeof(dwExStyle) * 8; ++i) {
    const DWORD dwFlag = 1 << i;
    if (dwExStyle & dwFlag) {
      LPCWSTR szFlag = NULL;
#define BIND_STYLE(style) case style: { szFlag = L"" #style; } break;
      switch (dwFlag) {
        BIND_STYLE(WS_EX_ACCEPTFILES);
        BIND_STYLE(WS_EX_APPWINDOW);
        BIND_STYLE(WS_EX_CLIENTEDGE);
        BIND_STYLE(WS_EX_COMPOSITED);
        BIND_STYLE(WS_EX_CONTEXTHELP);
        BIND_STYLE(WS_EX_CONTROLPARENT);
        BIND_STYLE(WS_EX_DLGMODALFRAME);
        BIND_STYLE(WS_EX_LAYERED);
        BIND_STYLE(WS_EX_LAYOUTRTL);
        BIND_STYLE(WS_EX_LEFTSCROLLBAR);
        BIND_STYLE(WS_EX_MDICHILD);
        BIND_STYLE(WS_EX_NOACTIVATE);
        BIND_STYLE(WS_EX_NOINHERITLAYOUT);
        BIND_STYLE(WS_EX_RIGHT);
        BIND_STYLE(WS_EX_RTLREADING);
        BIND_STYLE(WS_EX_TOOLWINDOW);
        BIND_STYLE(WS_EX_TOPMOST);
        BIND_STYLE(WS_EX_TRANSPARENT);
        BIND_STYLE(WS_EX_WINDOWEDGE);
        default: { szFlag = L"(unknown flag)"; } break;
      };
      PushFlagString(szBuffer, ARRLEN(szBuffer), szFlag);
#undef BIND_STYLE
    }
  }

  return szBuffer;
}

typedef struct Window Window;
struct Window
{
  HWND  hWnd;
  DWORD dwStyle;
  DWORD dwExStyle;

  Window* pNext;
};

static Window* pWindowList = 0;

static HWND PushWindow(WNDCLASSW* pWC, DWORD dwStyle, DWORD dwExStyle)
{
  static UINT wID = 0;
  static UINT wX = 25;
  static UINT wY = 25;
  static const UINT wW = 256;
  static const UINT wH = 256;

  ++wID;

  Window* pWnd = calloc(1, sizeof(Window));
  pWnd->dwStyle = dwStyle;
  pWnd->dwExStyle = dwExStyle;

  LPCWSTR szTitle = TempFormat(L"Window #%d", wID);
  LogMessage(L"%s:", szTitle);
  LogMessage(L"    dwStyle: %s", GetWindowStyleString(dwStyle));
  LogMessage(L"  dwExStyle: %s", GetWindowExStyleString(dwExStyle));
  pWnd->hWnd = CreateWindowExW(pWnd->dwExStyle, pWC->lpszClassName, szTitle, pWnd->dwStyle,
                               wX, wY, wW, wH, NULL, NULL, NULL, pWnd);
  if (!pWnd->hWnd) {
    LogError(L"CreateWindowExW failed");
  }

  wX += wW + 10;

  pWnd->pNext = pWindowList;
  pWindowList = pWnd;

  ShowWindow(pWnd->hWnd, SW_SHOW);

  return pWnd->hWnd;
}

static LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
  case WM_CREATE: {
    CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pCreate->lpCreateParams);
  } break;
  case WM_PAINT: {
    static HBRUSH hBgBrush = NULL;
    if (!hBgBrush) {
      hBgBrush = CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
      assert(hBgBrush);
    }

    static HBRUSH hFgBrush = NULL;
    if (!hFgBrush) {
      hFgBrush = CreateSolidBrush(RGB(0xFF, 0x00, 0x00));
      assert(hFgBrush);
    }

    static HFONT hFont = NULL;
    if (!hFont) {
      hFont = CreateFontW(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
          DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
      assert(hFont);
    }

    Window* w = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    RECT rect;
    // background
    GetClientRect(hWnd, &rect);
    FillRect(hdc, &rect, hBgBrush);
    // corner
    RECT box = rect;
    box.left = box.right - 10;
    box.top = box.bottom - 10;
    FillRect(hdc, &box, hFgBrush);
    // text
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, hFont);
    rect.left += 5; rect.top += 5;
#define PUSH_TEXT(text)                                                 \
      DrawTextW(hdc, text, -1, &rect, DT_LEFT | DT_TOP | DT_NOPREFIX);  \
      rect.top += 12;

    PUSH_TEXT(TempFormat(L"  dwStyle: %s", GetWindowStyleString(w->dwStyle)));
    PUSH_TEXT(TempFormat(L"dwExStyle: %s", GetWindowExStyleString(w->dwExStyle)));

#undef PUSH_TEXT

    EndPaint(hWnd, &ps);
    return 0;
  } break;
  case WM_CLOSE: {
    PostQuitMessage(0);
  } break;
  }
  return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static void Demo(WNDCLASSW* wc)
{
  HWND hWnd = NULL;

  // Regular window
  hWnd = PushWindow(wc, WS_OVERLAPPEDWINDOW, 0);

  // Popup window
  hWnd = PushWindow(wc, WS_POPUPWINDOW, 0);

  // Both
  hWnd = PushWindow(wc, WS_POPUPWINDOW, WS_EX_OVERLAPPEDWINDOW);

  // Regular window, transparent background
  hWnd = PushWindow(wc, WS_OVERLAPPEDWINDOW, WS_EX_LAYERED | WS_EX_TOPMOST);
  SetLayeredWindowAttributes(hWnd, RGB(0xFF, 0xFF, 0xFF), 0, LWA_COLORKEY);
  UpdateWindow(hWnd);
}

int main(int argc, const char* argv[])
{
  WNDCLASSW wc = { 0 };
  wc.lpfnWndProc = WndProc;
  wc.lpszClassName = L"DemoWindowClass";

  if (!RegisterClassW(&wc)) {
    LogError(L"RegisterClassW failed");
  }

  Demo(&wc);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  Window* pNextWindow = NULL;
  for (Window* pWindow = pWindowList; pWindow; pWindow = pNextWindow) {
    pNextWindow = pWindow->pNext;
    DestroyWindow(pWindow->hWnd);
    free(pWindow);
  }

  return 0;
}
