#include <assert.h>
#include <wchar.h>
#include <windows.h>

// #define DEMO1
#define DEMO2

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

int main(int argc, const char* argv[])
{
#ifdef DEMO1
  demo1();
#endif
#ifdef DEMO2
  demo2();
#endif
}
