#include "src/platform/window_probe.h"
#include <iostream>
int main(){
    using namespace whatis;enableDpiAwareness();
    const HWND window=CreateWindowExW(0,L"STATIC",L"Native test",WS_POPUP,0,0,100,100,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return 1;
    const auto identity=identify(window);const auto result=inspect(identity);
    bool ok=result.hasPath()&&result.identity.pid==GetCurrentProcessId()&&stillSame(result)&&result.created!=0;
    const auto roundTrip=wide(utf8(L"C:\\测试\\path with spaces.exe"));
    ok=ok&&roundTrip==L"C:\\测试\\path with spaces.exe";
    DestroyWindow(window);ok=ok&&!stillSame(result)&&inspect(identity).stale;
    std::cout<<(ok?"PASS":"FAIL")<<" native path, identity, Unicode and closed-window checks\n";
    return ok?0:1;
}
