#include <windows.h>
#include <sddl.h>
#include <iostream>
#include <string>
int clicks=0;
LRESULT CALLBACK fixture(HWND hwnd,UINT message,WPARAM w,LPARAM l){
    if(message==WM_COMMAND&&LOWORD(w)==1){++clicks;SetWindowTextW(GetDlgItem(hwnd,2),(L"Clicks: "+std::to_wstring(clicks)).c_str());return 0;}
    if(message==WM_CLOSE){DestroyWindow(hwnd);return 0;}
    if(message==WM_DESTROY){PostQuitMessage(0);return 0;}
    return DefWindowProcW(hwnd,message,w,l);
}
int main(int argc,char**argv){
    SetProcessDPIAware();
    WNDCLASSW cls{};cls.lpfnWndProc=fixture;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"WhatIsTestFixture";cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&cls);
    HWND hwnd=CreateWindowExW(0,cls.lpszClassName,L"窗口识别测试 · Test fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,80,80,480,310,nullptr,nullptr,cls.hInstance,nullptr);
    HWND child=CreateWindowExW(0,L"BUTTON",L"Target button — must not activate",WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,30,65,360,48,hwnd,reinterpret_cast<HMENU>(1),cls.hInstance,nullptr);
    CreateWindowExW(0,L"STATIC",L"Clicks: 0",WS_CHILD|WS_VISIBLE,30,135,360,30,hwnd,reinterpret_cast<HMENU>(2),cls.hInstance,nullptr);
    if(argc>1&&std::string(argv[1])=="--deny"){
        PSECURITY_DESCRIPTOR sd=nullptr;
        if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:(D;;0x1000;;;WD)(A;;GA;;;WD)",SDDL_REVISION_1,&sd,nullptr)||!SetKernelObjectSecurity(GetCurrentProcess(),DACL_SECURITY_INFORMATION,sd))return 2;
        LocalFree(sd);
    }
    std::cout<<"{\"hwnd\":"<<reinterpret_cast<unsigned long long>(hwnd)<<",\"child\":"<<reinterpret_cast<unsigned long long>(child)<<",\"pid\":"<<GetCurrentProcessId()<<"}"<<std::endl;
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;
}
