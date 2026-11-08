#include "src/core/selection.h"
#include <iostream>
#include <stdexcept>
int checks=0;
void check(bool value,const char* name){++checks;if(!value)throw std::runtime_error(name);}
int main(){try{
    using namespace whatis;
    Selection s;Identity a{1,11},b{2,22};
    Result ra;ra.identity=a;ra.path=L"C:\\Tools\\测试.exe";
    check(ra.executable()==L"测试.exe","unicode executable");
    check(ra.hasPath(),"path available");
    check(!ra.knownHost(),"ordinary process not relabeled");
    s.begin();check(s.phase==Phase::Picking,"begin");
    check(s.point(a),"candidate change");auto old=s.ticket();
    check(s.accept(old,ra),"accept current preview");
    check(!s.locked,"preview not a locked result");
    check(s.confirm(),"confirm");check(!s.accept(old,ra),"reject pre-confirm response");
    check(s.accept(s.ticket(),ra),"accept fresh confirmed result");
    check(s.phase==Phase::Locked&&s.locked->identity==a,"locked");
    s.begin();s.point(b);auto canceled=s.ticket();s.cancel();Result rb;rb.identity=b;
    check(!s.accept(canceled,rb),"reject canceled response");
    check(s.locked->identity==a&&s.phase==Phase::Locked,"cancel restores previous");
    s.begin();s.point(a);old=s.ticket();s.point(b);check(!s.accept(old,ra),"reject old candidate");
    s.begin();s.point(a);check(!s.accept(old,ra),"reject previous session same HWND");
    Result wrong;wrong.identity=b;check(!s.accept(s.ticket(),wrong),"reject mismatched identity");
    s.point({});check(!s.confirm(),"cannot lock empty desktop");
    s.point(b);s.confirm();rb.error=5;check(s.accept(s.ticket(),rb),"retain denied result");
    check(s.locked->identity==b&&!s.locked->hasPath(),"denied retains PID");
    Result host;host.path=L"C:\\Windows\\System32\\APPLICATIONFRAMEHOST.EXE";
    check(host.knownHost(),"host classification case insensitive");
    host.path=L"C:\\x\\chrome.exe";check(!host.knownHost(),"browser not guessed as packaged host");
    s.begin();old=s.ticket();s.cancel();check(!s.accept(old,rb),"late result cannot reopen");
    std::cout<<"PASS "<<checks<<" core checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
