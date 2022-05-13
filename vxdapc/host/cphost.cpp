#define CRLF "\r\n"
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shellapi.h>
#include <stdarg.h>
#include <commdlg.h>
#define IDM_ABOUT   100
#define IDM_QUIT    101
#define IDM_LOG     102
#define HOST_TRAYCMD  (WM_USER+1)
/* <synced> */
#define VAPC_IOCTL_GET_VERSION       1
#define VAPC_IOCTL_REGISTER_HOST     2
#define VAPC_IOCTL_UNREGISTER_HOST   3

static char *szAppName="OCP.devpDX5";
static HINSTANCE hInst;
static char *log;
static int err;
static int inabout, inlog;
static HWND logwnd;

HWND __export hWnd;
static HANDLE hEQuit, hEReady;

HINSTANCE __cdecl apcLoadLibrary(const char *dll);
void      __cdecl apcFreeLibrary(HINSTANCE dll);
void      __cdecl *apcGetProcAddress(HINSTANCE dll, const char *symbol);
void      __cdecl *apcGetAddress(const char *sym);

DWORD WINAPI            APCProc(PVOID param);
static void             BuildContextMenu();
static BOOL CALLBACK    DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
static void             DisplayLog();
void __export           AddLog(const char *fmt, ...);
void __export           SetError(int er);
static DWORD WINAPI     WindowThread(LPVOID);
int   WINAPI            WinMain(HINSTANCE _hInst, HINSTANCE hPrev, char *cmdline,int sw);
static void             UpdateIcon(HICON icon);

#pragma pack (push, 1)
struct query_s /* 0.2 */
{
  void *esp, *proc;
  long done, res, stackcopy;
};
#pragma pack (pop)

/* </synced> */

long stackncall(void *stack, int bytes, void *proc);
#pragma aux stackncall parm [esi] [ecx] [eax] modify [ebx ecx edx esi edi] value [eax] = \
  "cld" \
  "sub esp, ecx" \
  "mov ebx, ecx" \
  "mov edi, esp" \
  "rep movsb" \
  "call eax" \
  "add esp, ebx"

/* the "api" */

HINSTANCE __cdecl apcLoadLibrary(const char *dll)
{
#ifdef DEBUG
  AddLog("LoadLibrary(\"%s\")=", dll);
#endif
  HINSTANCE res=LoadLibraryA(dll);
#ifdef DEBUG
  AddLog("%p" CRLF, res);
#endif
  if (!res)
    {
      AddLog("The DLL %s couldn't be loaded." CRLF, dll);
      SetError(1);
    }
  return res;
}

void __cdecl apcFreeLibrary(HINSTANCE dll)
{
#ifdef DEBUG
  AddLog("FreeLibrary(%p);" CRLF, dll);
#endif
  FreeLibrary(dll);
}

void __cdecl *apcGetProcAddress(HINSTANCE dll, const char *symbol)
{
#ifdef DEBUG
  AddLog("GetProcAddress(%p, \"%s\")=", dll, symbol);
#endif
  void *res=GetProcAddress(dll, symbol);
#ifdef DEBUG
  AddLog("%p" CRLF, res);
#endif
  return res;
}

void __cdecl *apcGetAddress(const char *sym)
{
#ifdef DEBUG
  AddLog("apcGetAddress(%s);" CRLF, sym);
#endif
  if (!strcmp(sym, "apcLoadLibrary"))
    return apcLoadLibrary;
  if (!strcmp(sym, "apcFreeLibrary"))
    return apcFreeLibrary;
  if (!strcmp(sym, "apcGetProcAddress"))
    return apcGetProcAddress;
  return 0;
}

DWORD WINAPI APCProc(PVOID param)
{
  query_s &query=*(query_s*)param;
#ifdef DEBUG
  AddLog("APC call: query %p" CRLF, query);
#endif
  int q=(int)query.proc;
  if (q==-1)                    // the special case
    query.proc=apcGetAddress;

#ifdef DEBUG
  AddLog("APC call: query %p(%d) done: %d " CRLF, query.proc, query.stackcopy, query.done);
#endif
  query.res=stackncall((char*)query.esp, query.stackcopy, query.proc);

  query.done=!0;

  return 0;
}

void AddLog(const char *fmt, ...)
{
  static char buffer[1000];
  va_list ap;
  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  char *buf=new char[(log?strlen(log):0)+strlen(buffer)+1];
  if (!buf)
    return;
  if (log)
    strcpy(buf, log);
  else
    *buf=0;
  strcat(buf, buffer);
  delete[] log;
  log=buf;
  if (inlog)
    SetDlgItemText(logwnd, 102, log);
}

void __export SetError(int er)
{
  if (er>err)
    {
      if ((!err) && (er))
	UpdateIcon(LoadIcon(hInst, MAKEINTRESOURCE(202)));
      err=er;
    }
}

void BuildContextMenu()
{
  HMENU hmenu = CreatePopupMenu();
  if (hmenu)
    {
      POINT ptPos; 
      GetCursorPos(&ptPos);

      MENUITEMINFO mii;

      mii.cbSize=sizeof(mii);
      mii.fMask=MIIM_ID|MIIM_STATE|MIIM_TYPE;
      mii.fType=MFT_STRING;

      mii.fState=inabout?(MFS_CHECKED|MFS_DISABLED):MFS_ENABLED;

      mii.wID=IDM_ABOUT;
      mii.dwTypeData="&About";
      InsertMenuItem(hmenu, 0, FALSE, &mii);

      mii.fState=MFS_DEFAULT|(inlog?(MFS_CHECKED|MFS_DISABLED):MFS_ENABLED);
      mii.wID=IDM_LOG;
      mii.dwTypeData="&Log";
      InsertMenuItem(hmenu, 0, FALSE, &mii);

      mii.fState=MFS_ENABLED;

      mii.wID=IDM_ABOUT;
      mii.fType=MFT_SEPARATOR;
      InsertMenuItem(hmenu, 0, FALSE, &mii);

      mii.fType=MFT_STRING;
      mii.wID=IDM_QUIT;
      mii.dwTypeData="&Quit";
      InsertMenuItem(hmenu, 0, FALSE, &mii);

      /* AppendMenu(hmenu, MF_ENABLED, IDM_ABOUT, );
	 AppendMenu(hmenu, MF_ENABLED, IDM_LOG, "&Log");
	 AppendMenu(hmenu, MF_SEPARATOR, 0, "" );
	 AppendMenu(hmenu, MF_ENABLED, IDM_QUIT, "&Quit"); */
      SetForegroundWindow(hWnd);  //needed for tray popup
      TrackPopupMenu(hmenu, TPM_LEFTALIGN, ptPos.x, ptPos.y, 0, hWnd, NULL);
      PostMessage(hWnd, WM_NULL, 0, 0);
      DestroyMenu(hmenu);
    }
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  switch (iMsg)
    {
    case WM_INITDIALOG:
      // SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
      SetForegroundWindow(hWnd);
      switch (lParam)
	{
	case IDM_ABOUT:
	  return 1;
	case IDM_LOG:
	  logwnd=hWnd;
	  if (log)
	    SetDlgItemText(hWnd, 102, log);
	  else
	    SetDlgItemText(hWnd, 102, "[empty log]");
	  return 1;
	}
      return 0;
    case WM_COMMAND:
      switch (wParam)
	{
	case IDOK:
	case IDCANCEL:
	  EndDialog(hWnd,0);
	  return 1;
	case 101:   // save to file
	  {
	    OPENFILENAME ofn;
	    char filename[_MAX_FNAME]="";
	    ofn.lStructSize=sizeof(ofn);
	    ofn.hwndOwner=hWnd;
	    ofn.hInstance=hInst;
	    ofn.lpstrFilter="Log files\0*.txt\0";
	    ofn.lpstrCustomFilter=0;
	    ofn.nMaxCustFilter=0;
	    ofn.nFilterIndex=0;
	    ofn.lpstrFile=filename;
	    ofn.nMaxFile=_MAX_FNAME;
	    ofn.lpstrFileTitle=0;
	    ofn.nMaxFileTitle=0;
	    ofn.lpstrInitialDir=0;
	    ofn.lpstrTitle="Save log to file...";
	    ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY;
	    ofn.nFileOffset=0;
	    ofn.nFileExtension=0;
	    ofn.lpstrDefExt=0;
	    ofn.lCustData=0;
	    ofn.lpfnHook=0;
	    ofn.lpTemplateName=0;
	    if(GetSaveFileName(&ofn))
	      {
		FILE *f=fopen(ofn.lpstrFile, "wb");
		if (f)
		  {
		    fwrite(log, strlen(log), 1, f);
		    fclose(f);
		  } 
		else
		  MessageBox(hWnd, "failed to save", "OCP", MB_OK|MB_ICONERROR);
	      }

	  }
	}
      break;
    }
  return 0;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  switch (iMsg)
    {
    case WM_DESTROY :
      PostQuitMessage(0);
      return 0;
    case WM_COMMAND:
      {
	INT uItem=(UINT)LOWORD(wParam);
	INT fuFlags=(UINT)HIWORD(wParam);
	if (fuFlags&MF_DISABLED)
	  break;
	switch (uItem)
	  {
	  case IDM_ABOUT:
	    {
	      if (!inabout)
		{
		  inabout++;
		  DialogBoxParam(hInst, MAKEINTRESOURCE(100), 0, DialogProc, IDM_ABOUT);
		  inabout--;
		}
	      break;
	    }
	  case IDM_LOG:
	    DisplayLog();
	    break;
	  case IDM_QUIT:
	    PostQuitMessage(0);
	    break;
	  }
      }
    case HOST_TRAYCMD:
      {
	switch (lParam)
	  {
	  case WM_RBUTTONUP:
	    {
	      BuildContextMenu();
	      break;
	    }
	  case WM_LBUTTONDBLCLK:
	    {
	      DisplayLog();
	      break;
	    }
	  }
	return 0;
      }
    }
  return DefWindowProc(hwnd, iMsg, wParam, lParam) ;
}

void DisplayLog()
{
  if (!inlog)
    {
      inlog++;
      DialogBoxParam(hInst, MAKEINTRESOURCE(101), 0, DialogProc, IDM_LOG);
      inlog--;
    }
}

DWORD WINAPI WindowThread(LPVOID)
{
  hWnd = CreateWindow (szAppName,                // window class name
		       szAppName,                // window caption
		       WS_OVERLAPPED|WS_SYSMENU, // window style
		       CW_USEDEFAULT,            // initial x position
		       CW_USEDEFAULT,            // initial y position
		       'C',                      // initial x size
		       'P',                      // initial y size
		       NULL,                     // parent window handle
		       NULL,                     // window menu handle
		       hInst,                    // program instance handle
		       NULL) ;		         // creation parameters

  ShowWindow(hWnd, SW_HIDE);
  UpdateWindow(hWnd);
  
  NOTIFYICONDATA nid;
  nid.cbSize=sizeof(nid);
  nid.hWnd=hWnd;
  nid.uID=0;
  nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;
  nid.uCallbackMessage=HOST_TRAYCMD;
  nid.hIcon=LoadIcon(hInst, MAKEINTRESOURCE(202));
  strcpy(nid.szTip, "Open Cubic Player DirectSound Output");
  Shell_NotifyIcon(NIM_ADD, &nid);
  SetEvent(hEReady);
  MSG msg;
  while (GetMessage(&msg, hWnd, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  Shell_NotifyIcon(NIM_DELETE, &nid);
  SetEvent(hEQuit);
  return 0;
}

void AddLogWindowsError()
{
  LPVOID lpMsgBuf;
  FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		0, // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
  AddLog((char*)lpMsgBuf);
}

int WINAPI WinMain(HINSTANCE _hInst, HINSTANCE hPrev, char *cmdline,int sw)
{
  HWND otherwin;
  if (otherwin=FindWindow(szAppName, 0))
    {
      MessageBox(otherwin, "This program can only run once." CRLF, szAppName, MB_OK|MB_ICONEXCLAMATION);
      return 0;
    }
  AddLog("VAPC Host started on Windows ");
  hEQuit=CreateEvent(0, 0, 0, 0);
  hEReady=CreateEvent(0, 0, 0, 0);
  hInst=_hInst;

  OSVERSIONINFO ver;
  ver.dwOSVersionInfoSize=sizeof(ver);
  GetVersionEx(&ver);

  if (ver.dwPlatformId==VER_PLATFORM_WIN32_NT)
    {
      AddLog(" NT %d.%d", ver.dwMajorVersion, ver.dwMinorVersion);
      AddLog("ERROR: This Software won't work with Windows NT or 2k." CRLF);
      SetError(1);
      goto cphostloop;
    } 
  else
    {
      if ((ver.dwMajorVersion==4)&&(!ver.dwMinorVersion))
	AddLog("95");
      else if ((ver.dwMajorVersion==4)&&(ver.dwMinorVersion==0xA))
	AddLog("98");
      else
	AddLog("9x %d.%d", ver.dwMajorVersion, ver.dwMinorVersion);
    }
  if (*ver.szCSDVersion)
    AddLog(" (%s)", ver.szCSDVersion);
  AddLog(" (build %d)" CRLF, ver.dwBuildNumber & 0xFFFF);

  WNDCLASSEX  wndclass;
  wndclass.cbSize        = sizeof (wndclass);
  wndclass.style         = 0;
  wndclass.lpfnWndProc   = WndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hInst;
  wndclass.hIcon         = LoadIcon (hInst, MAKEINTRESOURCE(200));
  wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
  wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
  wndclass.lpszMenuName  = NULL;
  wndclass.lpszClassName = szAppName;
  wndclass.hIconSm       = LoadIcon (NULL, IDI_WINLOGO);

  RegisterClassEx (&wndclass);

  DWORD tid;
  CreateThread(0, 8192, WindowThread, 0, 0, &tid);

  if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
    AddLog("WARNING: Couldn't set priority class!" CRLF);

  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))
    AddLog("WARNING: Couldn't set thread priority!" CRLF);

  HANDLE hDevice;
  hDevice=CreateFile("\\\\.\\VAPC.VXD", 0, 0, 0, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, 0);
  if (hDevice==INVALID_HANDLE_VALUE)
    {
      AddLogWindowsError();
      AddLog("ERROR: Couldn't load VAPC.VXD! Is the VAPC.VXD in your windows\\system-directory?" CRLF);
      SetError(2);
      goto cphostloop;
    } 

  long version;
  DWORD bytesreturned;
  if (!DeviceIoControl(hDevice, VAPC_IOCTL_GET_VERSION, 0, 0, &version, 4, &bytesreturned, 0))
    {
      AddLogWindowsError();
      AddLog("ERROR: DeviceIoControl failed. Maybe you're using the wrong version." CRLF);
      SetError(2);
      goto cphostloop;
    }

  if (bytesreturned<4)
    {
      AddLog("ERROR: Failed to get version information from VxD." CRLF);
      SetError(2);
      goto cphostloop;
    }

  void *inBuf;
  inBuf=APCProc;
  if (!DeviceIoControl(hDevice, VAPC_IOCTL_REGISTER_HOST, &inBuf, 4, &version, 4, &bytesreturned, 0))
    {
      AddLogWindowsError();
      AddLog("ERROR: Failed to register in VxD." CRLF);
      SetError(2);
      goto cphostloop;
    }

  AddLog("Version %d.%02x of VxD detected." CRLF, version>>16, version&0xFFFF);
  AddLog("Listening... Everything should be alright." CRLF);

  WaitForSingleObject(hEReady, INFINITE);

  if (!err)
    UpdateIcon(LoadIcon(hInst, MAKEINTRESOURCE(200)));

 cphostloop:
  while (WaitForSingleObjectEx(hEQuit, INFINITE, TRUE)!=WAIT_OBJECT_0);

  CloseHandle(hEQuit);
  CloseHandle(hDevice);
  return 0;
}

void UpdateIcon(HICON icon)
{
  NOTIFYICONDATA nid;
  nid.cbSize=sizeof(nid);
  nid.hWnd=hWnd;
  nid.uID=0;
  nid.uFlags=NIF_ICON;
  nid.hIcon=icon;
  Shell_NotifyIcon(NIM_MODIFY, &nid);
}
