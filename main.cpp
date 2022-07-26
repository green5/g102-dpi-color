#pragma warning(disable:4996)
#include "std.h"
#include <shellapi.h>
#include "main.h"
#include "hid.cpp"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "../hidapi/x64/hidapi.lib")

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
HINSTANCE g_hInst;
HWND hwnd;

BOOL AddNotificationIcon() {
  NOTIFYICONDATA nid = { sizeof(nid) };
  nid.hWnd = hwnd;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_SHOWTIP;
  nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
  nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_NOTIFICATIONICON_0));
  Shell_NotifyIcon(NIM_ADD, &nid);
  nid.uVersion = NOTIFYICON_VERSION_4;
  return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL SetNotificationIcon(unsigned index) {
  NOTIFYICONDATA nid = { sizeof(nid) };
  nid.hWnd = hwnd;
  nid.uFlags = NIF_ICON;
  int idi = IDI_NOTIFICATIONICON_0 + min(index, 3);
  nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(idi));
  return Shell_NotifyIcon(NIM_MODIFY, &nid);
}

BOOL DeleteNotificationIcon() {
  NOTIFYICONDATA nid = { sizeof(nid) };
  nid.hWnd = hwnd;
  return Shell_NotifyIcon(NIM_DELETE, &nid);
}

void ShowContextMenu(HWND hwnd, POINT pt) {
  HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
  if (!hMenu) return;
  HMENU hSubMenu = GetSubMenu(hMenu, 0);
  if (hSubMenu) {
    SetForegroundWindow(hwnd);
    UINT uFlags = TPM_RIGHTBUTTON;
    if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
    {
      uFlags |= TPM_RIGHTALIGN;
    }
    else
    {
      uFlags |= TPM_LEFTALIGN;
    }
    TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
  }
  DestroyMenu(hMenu);
}

void WMAPP_NOTIFYCALLBACK_(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  POINT pt;
  switch (LOWORD(lParam))
  {
  case WM_CONTEXTMENU:
    pt = { LOWORD(wParam), HIWORD(wParam) };
    ShowContextMenu(hwnd, pt);
    break;
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message)
  {
  case WM_DESTROY:
    DeleteNotificationIcon();
    PostQuitMessage(0);
    break;
  case WM_COMMAND:
  {
    int const wmId = LOWORD(wParam);
    switch (wmId)
    {
    case IDM_EXIT:
      DestroyWindow(hwnd);
      break;
    }
  }
  break;
  case WMAPP_NOTIFYCALLBACK:
    WMAPP_NOTIFYCALLBACK_(hwnd, message, wParam, lParam);
    break;
  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

void createHidden() {
  WNDCLASSEX wcex;
  memset(&wcex, 0, sizeof(wcex));
  wcex.cbSize = sizeof(wcex);
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = g_hInst;
  wcex.lpszClassName = APPNAME;
  if (!RegisterClassEx(&wcex)) PLOG;
  hwnd = CreateWindow(wcex.lpszClassName, NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 250, 200, NULL, NULL, g_hInst, NULL);
  if (!hwnd) PLOG;
}

void setdpi(unsigned dpi) {
  string c = "r";
  if (dpi >= 1) c = "g";
  if (dpi >= 2) c = "b";
  if (dpi >= 3) c = "ffffff";
  setcolor(c);
  SetNotificationIcon(dpi);
}

#include <tlhelp32.h>

DWORD IsProcessRunning(const char* processName)
{
  PROCESSENTRY32 entry;
  DWORD pid = GetCurrentProcessId();
  entry.dwSize = sizeof(PROCESSENTRY32);
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (Process32First(snapshot, &entry))
    while (Process32Next(snapshot, &entry))
      if (pid != entry.th32ProcessID && !stricmp(entry.szExeFile, processName)) {
        CloseHandle(snapshot);
        return entry.th32ProcessID;
      }
  CloseHandle(snapshot);
  return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
  std::pline_ = [](const string& a, string& line) {
    OutputDebugString(line.c_str());
    if (a == "ERR") MessageBox(hwnd, line.c_str(), a.c_str(), MB_OK | MB_SYSTEMMODAL);
    if (a == "EXIT") DeleteNotificationIcon(), MessageBox(hwnd, line.c_str(), a.c_str(), MB_OK | MB_SYSTEMMODAL), exit(0);
  };
  if (IsProcessRunning("g102-dpi-color.exe")) return perr("g102-dpi-color.exe is running");
  if (!hidinit()) return perr("G102 mouse not found");
  if (lpCmdLine[0]) return setcolor(lpCmdLine), 0;
  g_hInst = hInstance;
  createHidden();
  if (!AddNotificationIcon()) PLOG;
  setdpi(0);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  hidexit();
  return 0;
}
