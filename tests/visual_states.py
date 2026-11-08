"""Native visual-state captures and layout checks using test-owned windows.

No global mouse/key input; the original pointer position is restored.
"""
import argparse
import ctypes as C
import hashlib
import json
import pathlib
import subprocess
import time
import integration as t

ROOT=pathlib.Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser();p.add_argument('--out',type=pathlib.Path,default=ROOT/'build/ui-refresh/visual')
    args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    driver=None;fixture=None;original=t.W.POINT();t.U.GetCursorPos(C.byref(original));foreground=t.U.GetForegroundWindow()
    t.U.GetWindowLongPtrW.argtypes=[t.W.HWND,C.c_int];t.U.GetWindowLongPtrW.restype=C.c_ssize_t
    try:
        driver=t.Driver(ROOT/'whatis.exe')
        driver.request('initialize',{'locale':'zh','theme':'light','appearance':{},'settings':{},'pluginDir':str(ROOT),'dataDir':str(args.out)})
        driver.action();shield=t.wait(lambda:t.find(driver.p.pid,'WhatIsInput'))
        t.U.PostMessageW(shield,0x100,0x1B,0)
        card=t.wait(lambda:t.find(driver.p.pid,'这是什么'))
        time.sleep(.1);t.screenshot(card,args.out/'empty-light.png')
        t.check('Empty state hides result fields and actions',all(not t.U.IsWindowVisible(t.U.GetDlgItem(card,id)) for id in [101,102,103,201,203,204]))
        r=t.rect(card);result=C.c_size_t();t.U.SendMessageTimeoutW(card,0x84,0,((r.top+25)<<16)|(r.left+200),2,500,C.byref(result))
        t.check('Custom caption exposes a draggable area',result.value==2)
        fixture=subprocess.Popen([str(ROOT/'build/whatis_fixture.exe')],stdout=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        info=json.loads(fixture.stdout.readline());t.U.SetWindowPos(info['hwnd'],C.c_void_p(-1),30,60,0,0,1|0x10)
        t.U.SetCursorPos(*t.center(info['child']));t.command(card,100)
        shield=t.wait(lambda:t.find(driver.p.pid,'WhatIsInput'));t.U.PostMessageW(shield,0x201,1,0);t.U.PostMessageW(shield,0x202,0,0)
        t.wait(lambda:t.read(t.U.GetDlgItem(card,200))=='whatis_fixture.exe')
        time.sleep(.08);t.screenshot(card,args.out/'result-light.png')
        collapsed=t.rect(card).bottom-t.rect(card).top
        t.check('Default result card is compact',collapsed<=400,{'height':collapsed})
        t.check('Result keeps the full selectable path',t.read(t.U.GetDlgItem(card,201)).endswith('whatis_fixture.exe'))
        button=t.U.GetDlgItem(card,101);t.U.PostMessageW(button,0x200,0,0);time.sleep(.05)
        t.screenshot(card,args.out/'copy-hover.png');t.U.PostMessageW(button,0x2A3,0,0)
        t.command(card,103);t.wait(lambda:t.U.IsWindowVisible(t.U.GetDlgItem(card,204)))
        t.screenshot(card,args.out/'details-light.png')
        t.check('Details expand the card without losing primary actions',t.rect(card).bottom-t.rect(card).top>collapsed and t.U.IsWindowVisible(button))
        t.command(card,103)
        for name,locale,theme,size,rounded in [
            ('result-dark','zh','dark','standard',True),
            ('result-dark-en','en','dark','standard',True),
            ('roomy-square-en','en','light','roomy',False),
            ('compact-zh','zh','light','compact',True),
        ]:
            driver.send({'jsonrpc':'2.0','method':'appearance','params':{'locale':locale,'theme':theme,'uiSize':size,'nativeLook':{'rounded':rounded}}})
            time.sleep(.15)
            outer=t.rect(card)
            for id in [100,101,102,103,200,201,202,203,207,208,105]:
                inner=t.rect(t.U.GetDlgItem(card,id))
                t.check(name+': control '+str(id)+' fits',outer.left<=inner.left and inner.right<=outer.right and outer.top<=inner.top and inner.bottom<=outer.bottom)
            t.screenshot(card,args.out/(name+'.png'))
        # WM_DPICHANGED to an owned HWND exercises scaled layout only; it is
        # not evidence of physical mixed-monitor hit-testing.
        driver.send({'jsonrpc':'2.0','method':'appearance','params':{'locale':'zh','theme':'light','uiSize':'standard'}})
        time.sleep(.1);outer=t.rect(card)
        t.U.SendMessageTimeoutW(card,0x2E0,(144<<16)|144,C.addressof(outer),2,500,C.byref(result))
        time.sleep(.1);t.screenshot(card,args.out/'layout-150.png')
        t.check('Synthetic 150-percent DPI message scales native layout',t.rect(card).right-t.rect(card).left==720)
        t.command(card,105);t.wait(lambda:not t.U.IsWindowVisible(card))
        t.check('Custom close button hides the card',not t.U.IsWindowVisible(card))
        driver.stop();t.check('Visual-state process exits cleanly',driver.p.returncode==0)
        report={'checks':t.checks,'exe_sha256':hashlib.sha256((ROOT/'whatis.exe').read_bytes()).hexdigest(),'input':'Directed window messages; physical mouse and mixed-DPI monitors not covered'}
        (args.out/'visual.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
        print(f'PASS {len(t.checks)} visual and layout checks')
    finally:
        if driver:driver.stop()
        if fixture and fixture.poll() is None:fixture.kill();fixture.wait(timeout=3)
        t.U.SetCursorPos(original.x,original.y)
        if foreground:t.U.SetForegroundWindow(foreground)

if __name__=='__main__':main()
