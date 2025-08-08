#include <assert.h>
#include <wchar.h>
#include <windows.h>

#ifdef _MSC_VER
# pragma comment(lib, "gdi32.lib")
# pragma comment(lib, "user32.lib")
#endif

// #define DEMO1
// #define DEMO2
#define DEMO3

void thread_log(PCWSTR fmt, ...)
{
  PWSTR name = NULL;
  GetThreadDescription(GetCurrentThread(), &name);

  wchar_t buf[1024];
  va_list args;
  va_start(args, fmt);
  _vsnwprintf(buf, sizeof(buf)/sizeof(wchar_t), fmt, args);
  va_end(args);

  // Print with thread name
  wprintf(L"[%s] %s\n", name, buf);

  LocalFree(name);
}

//
// Demo 1
//

DWORD demo1_thread(LPVOID arg)
{
  SetThreadDescription(GetCurrentThread(), L"thread-2");
  for (int i = 0; i < 10; ++i) {
    thread_log(L"demo1: %d/%d", i+1, 10);
    Sleep(250);
  }
  return 0;
}

void demo1(void)
{
  SetThreadDescription(GetCurrentThread(), L"thread-1");
  thread_log(L"demo1: starting thread");
  HANDLE th = CreateThread(NULL, 0, demo1_thread, 0, 0, 0);
  assert(th);

  WaitForSingleObject(th, INFINITE);
  thread_log(L"demo1: all done");
  CloseHandle(th);
}

//
// Demo 2
//

typedef struct Demo2_Ctx Demo2_Ctx;
struct Demo2_Ctx
{
  HANDLE init_event;
  HANDLE stop_event;
};

DWORD demo2_thread(LPVOID arg)
{
  Demo2_Ctx* ctx = (Demo2_Ctx*)arg;

  SetThreadDescription(GetCurrentThread(), L"thread-2");

  thread_log(L"initializing");
  Sleep(1000);
  thread_log(L"ready");
  SetEvent(ctx->init_event);

  while (1) {
    DWORD wait = WaitForSingleObject(ctx->stop_event, 0);
    if (wait == WAIT_OBJECT_0) {
      break;
    }
    thread_log(L"tick");
    Sleep(500);
  }

  thread_log(L"done");

  return 0;
}

void demo2(void)
{
  SetThreadDescription(GetCurrentThread(), L"thread-1");

  Demo2_Ctx ctx = { 0 };
  ctx.init_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  ctx.stop_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  assert(ctx.init_event && ctx.stop_event);

  thread_log(L"starting thread");
  HANDLE th = CreateThread(NULL, 0, demo2_thread, &ctx, 0, 0);
  assert(th);

  thread_log(L"waiting for thread to init");
  WaitForSingleObject(ctx.init_event, INFINITE);

  thread_log(L"waiting a little bit");
  Sleep(5000);

  thread_log(L"telling the other guy to stop");
  SetEvent(ctx.stop_event);

  WaitForSingleObject(th, INFINITE);
  thread_log(L"all done");
  CloseHandle(th);
  CloseHandle(ctx.stop_event);
  CloseHandle(ctx.init_event);
}

//
// Demo 3
//

typedef struct Demo3_Ctx Demo3_Ctx;
struct Demo3_Ctx
{
  HANDLE        init_event;
  volatile LONG init_result;
};

LRESULT CALLBACK demo3_wproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rect;

    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        GetClientRect(hwnd, &rect);
        FrameRect(hdc, &rect, GetStockObject(BLACK_BRUSH));

        rect.left++; rect.top++; rect.right--; rect.bottom--;
        FrameRect(hdc, &rect, GetStockObject(WHITE_BRUSH));

        rect.left++; rect.top++; rect.right--; rect.bottom--;
        FrameRect(hdc, &rect, GetStockObject(BLACK_BRUSH));

        EndPaint(hwnd, &ps);
        break;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
}

DWORD demo3_thread(LPVOID arg)
{
  int result = 1;
  Demo3_Ctx* ctx = (Demo3_Ctx*)arg;
  SetThreadDescription(GetCurrentThread(), L"thread-2");

  thread_log(L"initializing");

  HWND hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, WC_DIALOG, NULL,
                             WS_POPUP | WS_VISIBLE, 0, 0, 256, 256,
                             NULL, NULL, NULL, NULL);
  if (hwnd) {
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)demo3_wproc);
  } else {
    thread_log(L"Failed to create window: %lu", GetLastError());
    result = 0;
  }  

  thread_log(L"ready");
  InterlockedExchange(&ctx->init_result, result);
  SetEvent(ctx->init_event);

  if (result) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  thread_log(L"done");
  if (hwnd) {
    DestroyWindow(hwnd);
  }
  return 0;
}

void demo3(void)
{
  SetThreadDescription(GetCurrentThread(), L"thread-1");

  Demo3_Ctx ctx = { 0 };
  ctx.init_result = 1;
  ctx.init_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  assert(ctx.init_event);

  thread_log(L"starting thread");
  DWORD th_id;
  HANDLE th = CreateThread(NULL, 0, demo3_thread, &ctx, 0, &th_id);
  assert(th);

  thread_log(L"waiting for thread to init");
  WaitForSingleObject(ctx.init_event, INFINITE);
  if (!ctx.init_result) {
    thread_log(L"Failed to initialize");
    goto done;
  }

  thread_log(L"waiting a little bit");
  Sleep(5000);

  thread_log(L"telling the other guy to stop");
  PostThreadMessage(th_id, WM_QUIT, 0, 0);

  WaitForSingleObject(th, INFINITE);
  thread_log(L"all done");
done:
  if (th) {
    CloseHandle(th);
  }
  if (ctx.init_event) {
    CloseHandle(ctx.init_event);
  }
}

int main(int argc, const char* argv[])
{
#ifdef DEMO1
  demo1();
#endif
#ifdef DEMO2
  demo2();
#endif
#ifdef DEMO3
  demo3();
#endif
}
