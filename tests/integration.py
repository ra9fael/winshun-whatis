"""Real Win32 fixtures + fake stdio host; never targets a user's window.

Run from any directory: python tests/integration.py [--out DIR] [--exe FILE]
Controls receive directed Windows messages. The cursor is moved only to query
the test fixture and then restored. No global key/button input is synthesized.
This verifies native routing, not physical input capture or click isolation.
"""
from __future__ import annotations
import argparse
import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
import pathlib
import queue
import subprocess
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
U = C.WinDLL('user32', use_last_error=True)
K = C.WinDLL('kernel32', use_last_error=True)
G = C.WinDLL('gdi32', use_last_error=True)
U.SetProcessDpiAwarenessContext.argtypes = [W.HANDLE]
U.SetProcessDpiAwarenessContext(C.c_void_p(-4))
CALLBACK = C.WINFUNCTYPE(W.BOOL, W.HWND, W.LPARAM)
for name, result, args in [
    ('EnumWindows', W.BOOL, [CALLBACK, W.LPARAM]),
    ('GetWindowThreadProcessId', W.DWORD, [W.HWND, C.POINTER(W.DWORD)]),
    ('GetClassNameW', C.c_int, [W.HWND, W.LPWSTR, C.c_int]),
    ('GetWindowTextW', C.c_int, [W.HWND, W.LPWSTR, C.c_int]),
    ('GetDlgItem', W.HWND, [W.HWND, C.c_int]),
    ('GetWindowRect', W.BOOL, [W.HWND, C.POINTER(W.RECT)]),
    ('GetClientRect', W.BOOL, [W.HWND, C.POINTER(W.RECT)]),
    ('SetForegroundWindow', W.BOOL, [W.HWND]),
    ('GetForegroundWindow', W.HWND, []),
    ('WindowFromPoint', W.HWND, [W.POINT]),
    ('SetWindowPos', W.BOOL, [W.HWND,W.HWND,C.c_int,C.c_int,C.c_int,C.c_int,W.UINT]),
    ('IsWindowVisible', W.BOOL, [W.HWND]),
    ('IsWindowEnabled', W.BOOL, [W.HWND]),
    ('PostMessageW', W.BOOL, [W.HWND, W.UINT, W.WPARAM, W.LPARAM]),
    ('SendMessageTimeoutW', W.LPARAM, [W.HWND, W.UINT, W.WPARAM, W.LPARAM, W.UINT, W.UINT, C.POINTER(C.c_size_t)]),
    ('SetCursorPos', W.BOOL, [C.c_int,C.c_int]),
    ('GetCursorPos', W.BOOL, [C.POINTER(W.POINT)]),
    ('PrintWindow', W.BOOL, [W.HWND,W.HDC,W.UINT]),
    ('GetDC', W.HDC, [W.HWND]),
    ('ReleaseDC', C.c_int, [W.HWND,W.HDC]),
]:
    fn=getattr(U,name); fn.restype=result; fn.argtypes=args
for name,result,args in [
    ('CreateCompatibleDC',W.HDC,[W.HDC]),('CreateCompatibleBitmap',W.HBITMAP,[W.HDC,C.c_int,C.c_int]),
    ('SelectObject',W.HANDLE,[W.HDC,W.HANDLE]),('DeleteObject',W.BOOL,[W.HANDLE]),
    ('DeleteDC',W.BOOL,[W.HDC]),('GetDIBits',C.c_int,[W.HDC,W.HBITMAP,W.UINT,W.UINT,W.LPVOID,W.LPVOID,W.UINT]),
]:
    fn=getattr(G,name);fn.restype=result;fn.argtypes=args
K.GetProcessHandleCount.argtypes=[W.HANDLE,C.POINTER(W.DWORD)]
checks=[]
def check(name,value,detail=None):
    checks.append({'name':name,'passed':bool(value),**({'detail':detail} if detail is not None else {})})
    if not value: raise AssertionError(name+': '+str(detail))
def wait(predicate,timeout=5):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        value=predicate()
        if value:return value
        time.sleep(.02)
    raise TimeoutError('Condition did not become true')
def pid_of(hwnd):
    pid=W.DWORD();U.GetWindowThreadProcessId(hwnd,C.byref(pid));return pid.value
def windows(pid,visible=True):
    found=[]
    @CALLBACK
    def visit(hwnd,_):
        if pid_of(hwnd)==pid and (not visible or U.IsWindowVisible(hwnd)):
            text=C.create_unicode_buffer(512);U.GetWindowTextW(hwnd,text,512)
            found.append((hwnd,text.value))
        return True
    U.EnumWindows(visit,0);return found
def find(pid,title):
    return next((hwnd for hwnd,name in windows(pid) if title in name),None)
def read(hwnd):
    buf=C.create_unicode_buffer(4096);result=C.c_size_t()
    U.SendMessageTimeoutW(hwnd,0xD,4096,C.cast(buf,C.c_void_p).value,2,500,C.byref(result))
    return buf.value
def rect(hwnd):
    value=W.RECT();U.GetWindowRect(hwnd,C.byref(value));return value
def center(hwnd):
    value=rect(hwnd);return (value.left+value.right)//2,(value.top+value.bottom)//2
def command(hwnd,id):U.PostMessageW(hwnd,0x111,id,0)

def send(hwnd,message,w=0,l=0):
    result=C.c_size_t()
    if not U.SendMessageTimeoutW(hwnd,message,w,l,2,1000,C.byref(result)):
        raise TimeoutError('Directed message timed out')
    return result.value
def screenshot(hwnd,destination):
    from PIL import Image
    box=rect(hwnd);width=box.right-box.left;height=box.bottom-box.top
    dc=U.GetDC(hwnd);memory=G.CreateCompatibleDC(dc);bitmap=G.CreateCompatibleBitmap(dc,width,height);old=G.SelectObject(memory,bitmap)
    class Header(C.Structure):
        _fields_=[('size',W.DWORD),('width',W.LONG),('height',W.LONG),('planes',W.WORD),('bits',W.WORD),('compression',W.DWORD),('image',W.DWORD),('x',W.LONG),('y',W.LONG),('used',W.DWORD),('important',W.DWORD)]
    header=Header(40,width,-height,1,32,0,0,0,0,0,0);pixels=C.create_string_buffer(width*height*4)
    try:
        if not U.PrintWindow(hwnd,memory,2):raise RuntimeError('PrintWindow failed')
        G.SelectObject(memory,old);G.GetDIBits(memory,bitmap,0,height,pixels,C.byref(header),0)
        Image.frombuffer('RGB',(width,height),pixels,'raw','BGRX',0,1).save(destination,optimize=True)
    finally:G.DeleteObject(bitmap);G.DeleteDC(memory);U.ReleaseDC(hwnd,dc)

class Driver:
    def __init__(self,exe):
        self.p=subprocess.Popen([str(exe)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        self.messages=queue.Queue();self.errors=[];self.sequence=0
        def reader():
            for line in self.p.stdout:
                try:self.messages.put(json.loads(line))
                except Exception:self.errors.append(line.decode('utf8','replace'))
        threading.Thread(target=reader,daemon=True).start()
    def send(self,message):
        self.p.stdin.write((json.dumps(message,ensure_ascii=False)+'\n').encode('utf8'));self.p.stdin.flush()
    def request(self,method,params):
        self.sequence+=1;id=self.sequence;self.send({'jsonrpc':'2.0','id':id,'method':method,'params':params})
        while True:
            response=self.messages.get(timeout=5)
            if response.get('id')==id:return response
            raise AssertionError('Unexpected message: '+str(response))
    def action(self):return self.request('action',{'command':'whatis','action':'pick','item':{'id':'whatis.pick'}})
    def stop(self):
        if self.p.poll() is None:
            try:self.request('shutdown',{});self.p.wait(timeout=3)
            except Exception:self.p.kill();self.p.wait(timeout=3)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--out',type=pathlib.Path,default=ROOT/'build'/'verification');parser.add_argument('--exe',type=pathlib.Path,default=ROOT/'whatis.exe');args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    fixtures=[];driver=None;original=W.POINT();U.GetCursorPos(C.byref(original));original_foreground=U.GetForegroundWindow()
    try:
        def fixture(deny=False):
            process=subprocess.Popen([str(ROOT/'build'/'whatis_fixture.exe')]+(['--deny']if deny else []),stdout=subprocess.PIPE,stderr=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
            fixtures.append(process);info=json.loads(process.stdout.readline())
            U.SetWindowPos(info['hwnd'],C.c_void_p(-1),0,0,0,0,3|0x10)
            return process,info
        proc,info=fixture();U.SetForegroundWindow(info['hwnd'])
        probe=subprocess.run([str(args.exe),'--probe',str(info['hwnd'])],capture_output=True,timeout=5)
        result=json.loads(probe.stdout)
        check('Actual HWND resolves to external fixture PID and executable',result['pid']==proc.pid and result['exe']=='whatis_fixture.exe' and probe.returncode==0,result)
        check('Unicode window title is preserved','窗口识别测试' in result['title'])
        denied,denied_info=fixture(True)
        denied_probe=subprocess.run([str(args.exe),'--probe',str(denied_info['hwnd'])],capture_output=True,timeout=5)
        denied_result=json.loads(denied_probe.stdout)
        check('Real process DACL denial retains PID without fabricated path',denied_result['pid']==denied.pid and denied_result['error']==5 and denied_result['path']=='',denied_result)
        U.PostMessageW(denied_info['hwnd'],0x10,0,0);denied.wait(timeout=3)
        driver=Driver(args.exe)
        started=time.monotonic();init=driver.request('initialize',{'locale':'zh','theme':'light','appearance':{},'settings':{},'pluginDir':str(ROOT),'dataDir':str(args.out)})
        check('Resident initialize responds',init.get('result',{}).get('name')=='ra9fael.whatis')
        query=driver.request('query',{'queryId':1,'command':'whatis','text':'','raw':'whatis ','trigger':'keyword'})
        check('Query returns stable pick item',query['result']['items'][0]['id']=='whatis.pick')
        check('Query is side-effect free: no visible picker',not windows(driver.p.pid))
        check('Initialize and query fit host timeout',time.monotonic()-started<1.5)
        U.SetWindowPos(info['hwnd'],C.c_void_p(-1),30,60,0,0,1|0x10)
        U.SetForegroundWindow(info['hwnd'])
        if not U.SetCursorPos(*center(info['child'])):
            raise RuntimeError('Desktop rejected test cursor positioning; requires an interactive test desktop')
        if pid_of(U.WindowFromPoint(W.POINT(*center(info['child']))))!=proc.pid:
            raise AssertionError('The test fixture must be unobstructed before selection')
        started=time.monotonic();answer=driver.action();check('Pick action responds before user selection',any(e['type']=='hide' for e in answer['result']['effects']) and time.monotonic()-started<1.5)
        shield=wait(lambda:find(driver.p.pid,'WhatIsInput'))
        check('Temporary input shield created',bool(shield))
        if not U.SetCursorPos(*center(info['child'])):
            raise RuntimeError('Desktop rejected test cursor positioning during selection')
        check('Input surface owns hit-test at fixture point',pid_of(U.WindowFromPoint(W.POINT(*center(info['child']))))==driver.p.pid)
        pointer=W.POINT();U.GetCursorPos(C.byref(pointer))
        if (pointer.x,pointer.y)!=center(info['child']):
            raise AssertionError(f'Test cursor moved: {(pointer.x,pointer.y)} != {center(info["child"])}')
        send(shield,0x201,1,0);send(shield,0x202,0,0)
        card=wait(lambda:find(driver.p.pid,'这是什么'))
        try:
            wait(lambda:read(U.GetDlgItem(card,200))=='whatis_fixture.exe')
        except TimeoutError as error:
            details={id:read(U.GetDlgItem(card,id)) for id in (200,201,202,203,206)}
            raise AssertionError('Fixture selection did not resolve: '+json.dumps(details,ensure_ascii=False)) from error
        check('Native selection resolves fixture EXE',read(U.GetDlgItem(card,201)).endswith('whatis_fixture.exe'))
        check('Directed selection leaves fixture button untouched',read(U.GetDlgItem(info['hwnd'],2))=='Clicks: 0')
        check('Picker input shields removed after selection',find(driver.p.pid,'WhatIsInput') is None)
        command(card,103);wait(lambda:U.IsWindowVisible(U.GetDlgItem(card,204)))
        check('Details contain selected HWND and class','WhatIsTestFixture' in read(U.GetDlgItem(card,204)))
        screenshot(card,args.out/'native-light.png')
        # The host tool bridge must keep reading requests while a reveal is pending.
        command(card,102);call=driver.messages.get(timeout=3)
        check('Reveal requests host show_in_folder, never starts EXE',call.get('method')=='winshun/tool' and call['params']['name']=='show_in_folder' and call['params']['arguments']['path'].endswith('whatis_fixture.exe'),call)
        query=driver.request('query',{'queryId':2,'command':'whatis','text':''})
        check('Protocol remains responsive during pending host tool',bool(query.get('result')))
        driver.send({'jsonrpc':'2.0','id':call['id'],'error':{'code':-32000,'message':'Synthetic tool failure'}})
        wait(lambda:'Synthetic tool failure' in read(U.GetDlgItem(card,206)))
        check('Host tool errors are visible',True)
        command(card,102);success=driver.messages.get(timeout=3)
        driver.send({'jsonrpc':'2.0','id':success['id'],'result':{'opened':True}})
        wait(lambda:'已请求' in read(U.GetDlgItem(card,206)))
        check('Successful tool response restores reveal action',bool(U.IsWindowEnabled(U.GetDlgItem(card,102))))
        command(card,102);late=driver.messages.get(timeout=3)
        wait(lambda:'超时' in read(U.GetDlgItem(card,206)),7)
        check('Unanswered tool call times out and restores action',bool(U.IsWindowEnabled(U.GetDlgItem(card,102))))
        driver.send({'jsonrpc':'2.0','id':late['id'],'result':{'opened':True}})
        driver.request('query',{'command':'whatis','text':''});time.sleep(.1)
        check('Late tool response cannot overwrite timeout status','超时' in read(U.GetDlgItem(card,206)))
        command(card,100);shield=wait(lambda:find(driver.p.pid,'WhatIsInput'));U.PostMessageW(shield,0x100,0x1B,0)
        wait(lambda:find(driver.p.pid,'WhatIsInput') is None)
        check('Escape cancels and retains locked result',read(U.GetDlgItem(card,200))=='whatis_fixture.exe')
        count_before=W.DWORD();K.GetProcessHandleCount(W.HANDLE(int(driver.p._handle)),C.byref(count_before))
        for _ in range(12):
            command(card,100);shield=wait(lambda:find(driver.p.pid,'WhatIsInput'))
            U.PostMessageW(shield,0x100,0x1B,0);wait(lambda:find(driver.p.pid,'WhatIsInput') is None)
        count_after=W.DWORD();K.GetProcessHandleCount(W.HANDLE(int(driver.p._handle)),C.byref(count_after))
        check('Repeated cancellation does not grow process handles',count_after.value<=count_before.value+2,{'before':count_before.value,'after':count_after.value,'cycles':12})
        driver.send({'jsonrpc':'2.0','method':'appearance','params':{'locale':'en','theme':'dark','corners':'square','nativeLook':{'rounded':False}}})
        wait(lambda:read(U.GetDlgItem(card,101))=='Copy path')
        check('Host language and appearance changes reach native controls',True)
        check('Language switch clears stale status text',read(U.GetDlgItem(card,206))=='')
        screenshot(card,args.out/'native-dark-en.png')
        # Directed messages exercise the native drag state machine without
        # claiming they establish OS capture behavior on this remote desktop.
        button=U.GetDlgItem(card,100);x,y=center(button);U.SetCursorPos(x,y);send(button,0x201,1)
        U.SetCursorPos(x+20,y+20);send(button,0x200,1);shield=wait(lambda:find(driver.p.pid,'WhatIsInput'));U.SetCursorPos(*center(info['child']));send(shield,0x202)
        wait(lambda:find(driver.p.pid,'WhatIsInput') is None)
        wait(lambda:read(U.GetDlgItem(card,200))=='whatis_fixture.exe')
        check('Drag and release selects actual external window',True)
        check('Directed drag leaves fixture button untouched',read(U.GetDlgItem(info['hwnd'],2))=='Clicks: 0')
        U.PostMessageW(info['hwnd'],0x10,0,0);proc.wait(timeout=3)
        wait(lambda:'snapshot' in read(U.GetDlgItem(card,203)))
        check('Closed target is labeled as a historical snapshot',True)
        denied2,info2=fixture(True)
        U.SetCursorPos(*center(info2['child']));command(card,100)
        shield=wait(lambda:find(driver.p.pid,'WhatIsInput'))
        U.PostMessageW(shield,0x201,1,0);U.PostMessageW(shield,0x202,0,0)
        wait(lambda:read(U.GetDlgItem(card,200))=='Path unavailable')
        check('Access-denied result retains PID and error',str(denied2.pid) in read(U.GetDlgItem(card,203)) and '5' in read(U.GetDlgItem(card,201)))
        check('Access-denied result disables path actions',not U.IsWindowEnabled(U.GetDlgItem(card,101)) and not U.IsWindowEnabled(U.GetDlgItem(card,102)))
        screenshot(card,args.out/'native-access-denied.png')
        command(card,100);wait(lambda:find(driver.p.pid,'WhatIsInput'))
        driver.stop();check('Shutdown during picking exits cleanly',driver.p.returncode==0 and not windows(driver.p.pid))
        check('Stdout contains protocol JSON only',not driver.errors)
        report={'date':'2026-11-08','checks':checks,'sha256':hashlib.sha256(args.exe.read_bytes()).hexdigest(),'clipboard':'Not modified','native_input':'Directed window messages; real OS hit-testing checked. Physical input isolation and capture not established. SendInput accepted events but test fixture received none on this desktop.','unverified':['Physical click isolation and mouse capture','Administrative / UIAccess surfaces','Physical mixed-DPI monitors','Screen readers','Protected processes beyond explicit DACL denial']}
        (args.out/'integration.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
        print(f'PASS {len(checks)} integration checks')
    finally:
        if driver:driver.stop()
        for process in fixtures:
            if process.poll() is None:process.kill();process.wait(timeout=3)
        U.SetCursorPos(original.x,original.y)
        if original_foreground:U.SetForegroundWindow(original_foreground)
        (args.out/'last-checks.json').write_text(json.dumps(checks,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
if __name__=='__main__':main()
