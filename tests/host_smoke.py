"""L3: production Winshun preview host, isolated profile and owned fixtures.

Uses directed window messages, never global key/button input. Requires a built
Winshun with --probe-runtime --live. Only processes created here are stopped.
"""
from __future__ import annotations
import argparse
import ctypes as C
from datetime import datetime, timezone
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import time
import integration as t
from window_icons import check_window_icons

ROOT = pathlib.Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--host',type=pathlib.Path,default=ROOT.parent/'WinShun/build/debug/WinShun.exe')
    p.add_argument('--out',type=pathlib.Path,default=ROOT/'build/host-smoke')
    p.add_argument('--qt',type=pathlib.Path,help='Qt kit root when the host has not been deployed')
    p.add_argument('--icons-only',action='store_true',help='Verify window icons and shutdown without selecting another window')
    args=p.parse_args();args.out=args.out.resolve();args.host=args.host.resolve()
    if args.qt:args.qt=args.qt.resolve()
    args.out.mkdir(parents=True,exist_ok=True)
    for name in ['stop','cmd.txt','ack.txt','windows.json']:
        (args.out/name).unlink(missing_ok=True)
    deploy=ROOT/'build/host-fixtures/ra9fael.whatis';deploy.mkdir(parents=True,exist_ok=True)
    for name in ['plugin.json','whatis.exe']:shutil.copy2(ROOT/name,deploy/name)
    target=ROOT/'build/host-target/whatis_fixture.exe';target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'build/whatis_fixture.exe',target)
    original=t.W.POINT();t.U.GetCursorPos(C.byref(original));foreground=t.U.GetForegroundWindow()
    host=fixture=None;plugin_pid=None;plugin_handle=None;card=None;profile=None
    log=(args.out/'process.log').open('wb');report={'time_utc':datetime.now(timezone.utc).isoformat(),'checks':t.checks}
    def commands(*lines):
        ack=args.out/'ack.txt';previous=ack.read_text() if ack.exists() else ''
        temp=args.out/'cmd.pending';temp.write_text('\n'.join(lines)+'\n',encoding='utf8');temp.replace(args.out/'cmd.txt')
        t.wait(lambda:ack.exists() and ack.read_text()!=previous,15)
    def plugin_window():
        found=[]
        @t.CALLBACK
        def visit(hwnd,_):
            name=C.create_unicode_buffer(128);t.U.GetClassNameW(hwnd,name,128)
            if name.value=='WinShun.WhatIs.Window' and t.U.IsWindowVisible(hwnd):found.append(hwnd)
            return True
        t.U.EnumWindows(visit,0)
        return found
    before=set(plugin_window())
    t.K.OpenProcess.argtypes=[t.W.DWORD,t.W.BOOL,t.W.DWORD];t.K.OpenProcess.restype=t.W.HANDLE
    t.K.WaitForSingleObject.argtypes=[t.W.HANDLE,t.W.DWORD];t.K.WaitForSingleObject.restype=t.W.DWORD
    t.K.CloseHandle.argtypes=[t.W.HANDLE]
    def clean_profile():
        if profile and profile.exists():
            assert profile.resolve().parent==pathlib.Path(tempfile.gettempdir()).resolve()
            assert profile.name.startswith('WinShun-preview-')
            shutil.rmtree(profile)
    try:
        fixture=subprocess.Popen([str(target)],stdout=subprocess.PIPE,creationflags=subprocess.CREATE_NO_WINDOW)
        info=json.loads(fixture.stdout.readline())
        t.U.SetWindowPos(info['hwnd'],C.c_void_p(-1),30,60,0,0,1|0x10)
        environment=os.environ.copy()
        if args.qt:
            environment['PATH']=str(args.qt/'bin')+os.pathsep+environment['PATH']
            environment['QT_PLUGIN_PATH']=str(args.qt/'plugins')
        host=subprocess.Popen([str(args.host),'--probe-runtime',str(args.out),'--runtime-plugins',str(deploy.parent),'--live'],cwd=args.host.parent,stdout=log,stderr=log,env=environment,creationflags=subprocess.CREATE_NO_WINDOW)
        commands('show whatis fixture','windows')
        visible=json.loads((args.out/'windows.json').read_text(encoding='utf8'))
        candidates=[w for w in visible if w['w']>200 and w['h']>50]
        if not candidates:raise AssertionError(visible)
        launcher=int(candidates[0]['hwnd'])
        t.check('Production host opened its launcher',t.pid_of(launcher)==host.pid,visible)
        time.sleep(.8)
        t.screenshot(launcher,args.out/'host-query.png')
        t.U.SetCursorPos(*t.center(info['child']))
        t.U.PostMessageW(launcher,0x100,0x0D,0);t.U.PostMessageW(launcher,0x101,0x0D,0)
        native=t.wait(lambda:next((w for w in plugin_window() if w not in before),None),8)
        plugin_pid=t.pid_of(native)
        plugin_handle=t.K.OpenProcess(0x100000,False,plugin_pid)
        assert plugin_handle,'Cannot observe the test-owned plugin lifecycle'
        probe=subprocess.run([str(ROOT/'whatis.exe'),'--probe',str(native)],capture_output=True,timeout=5,creationflags=subprocess.CREATE_NO_WINDOW)
        identity=json.loads(probe.stdout);binary=pathlib.Path(identity['path'])
        t.check('Query and Enter start the real installed plugin',binary.name=='whatis.exe' and binary.parent.name=='ra9fael.whatis',identity)
        profile=binary.parent.parent.parent
        t.check('Host uses its private preview profile',profile.name.startswith('WinShun-preview-'),str(profile))
        t.check('Installed executable matches this build',hashlib.sha256(binary.read_bytes()).digest()==hashlib.sha256((ROOT/'whatis.exe').read_bytes()).digest())
        # Inspecting the installed image can outlast the 200 ms activation
        # guard. Restart on the owned card before the directed selection.
        shield=t.find(plugin_pid,'WhatIsInput')
        if shield:
            t.U.PostMessageW(shield,0x100,0x1B,0)
            t.wait(lambda:t.find(plugin_pid,'WhatIsInput') is None)
        card=t.wait(lambda:t.find(plugin_pid,'这是什么'))
        check_window_icons(card, 'real host window')
        if args.icons_only:
            t.screenshot(card,args.out/'host-icons.png')
            report.update(host=str(args.host),plugin_pid=plugin_pid,profile=str(profile),
                sha256=hashlib.sha256((ROOT/'whatis.exe').read_bytes()).hexdigest(),
                scope='Window icon assignment, pixels, caption capture and lifecycle',
                unverified=['Physical taskbar/Alt+Tab interaction','Window selection and unrelated actions'])
            (args.out/'stop').touch();host.wait(timeout=15)
            report['host_exit_code']=host.returncode
            t.check('Isolated production host exits successfully',host.returncode==0,host.returncode)
            t.wait(lambda:not t.windows(plugin_pid,False))
            t.check('Host shutdown exits the plugin and removes its windows',t.K.WaitForSingleObject(plugin_handle,3000)==0)
            clean_profile()
            t.check('Task-owned temporary profile is cleaned after host shutdown',not profile.exists())
            print(f'PASS {len(t.checks)} real-host icon checks')
            return
        t.U.SetCursorPos(*t.center(info['child']))
        t.command(card,100);shield=t.wait(lambda:t.find(plugin_pid,'WhatIsInput'))
        t.U.PostMessageW(shield,0x201,1,0);t.U.PostMessageW(shield,0x202,0,0)
        card=t.wait(lambda:t.find(plugin_pid,'这是什么'))
        t.wait(lambda:t.read(t.U.GetDlgItem(card,200))=='whatis_fixture.exe')
        t.check('Sandboxed plugin resolves actual fixture EXE',t.read(t.U.GetDlgItem(card,201)).endswith('whatis_fixture.exe'))
        t.check('Result includes actual fixture PID',str(fixture.pid) in t.read(t.U.GetDlgItem(card,203)))
        t.check('Native selection removed all input shields',t.find(plugin_pid,'WhatIsInput') is None)
        t.screenshot(card,args.out/'host-result.png')
        # Repeat through the production action path, using the resident process.
        commands('show whatis fixture','windows')
        visible2=json.loads((args.out/'windows.json').read_text(encoding='utf8'))
        launcher2=int(next(w for w in visible2 if w['w']>200 and w['h']>50)['hwnd'])
        time.sleep(.3)
        t.U.PostMessageW(launcher2,0x100,0x0D,0);t.U.PostMessageW(launcher2,0x101,0x0D,0)
        shield=t.wait(lambda:t.find(plugin_pid,'WhatIsInput'))
        t.U.PostMessageW(shield,0x100,0x1B,0)
        t.wait(lambda:t.find(plugin_pid,'WhatIsInput') is None)
        t.check('Second host action reuses resident process and cancels cleanly',t.read(t.U.GetDlgItem(card,200))=='whatis_fixture.exe')
        t.U.PostMessageW(info['hwnd'],0x10,0,0);fixture.wait(timeout=3);target.unlink()
        t.command(card,102)
        t.wait(lambda:'无法定位文件' in t.read(t.U.GetDlgItem(card,206)),8)
        failure=t.read(t.U.GetDlgItem(card,206))
        t.check('Real host tool roundtrip returns a missing-file error', 'no such file' in failure.lower(),failure)
        report.update(host=str(args.host),host_pid=host.pid,plugin_pid=plugin_pid,profile=str(profile),sha256=hashlib.sha256((ROOT/'whatis.exe').read_bytes()).hexdigest(),input='Directed WM messages; no global key/button input',clipboard='Not modified',unverified=['Physical input capture','Real clipboard copy','Explorer reveal via real host'])
        (args.out/'stop').touch();host.wait(timeout=15)
        t.check('Isolated production host exits successfully',host.returncode==0)
        t.wait(lambda:not t.windows(plugin_pid,False))
        t.check('Host shutdown exits the plugin and removes its windows',t.K.WaitForSingleObject(plugin_handle,3000)==0 and not t.windows(plugin_pid,False))
        report['host_removed_profile']=not profile.exists()
        # Preview attempts deletion before App destruction; resident binaries
        # can leave a partial profile. Clean only the exact profile we observed.
        clean_profile()
        t.check('Task-owned temporary profile is cleaned after host shutdown',not profile.exists())
        print(f'PASS {len(t.checks)} real-host checks')
    finally:
        if host and host.poll() is None:
            (args.out/'stop').touch()
            try:host.wait(timeout=15)
            except subprocess.TimeoutExpired:host.kill();host.wait(timeout=3)
        if fixture and fixture.poll() is None:fixture.kill();fixture.wait(timeout=3)
        if plugin_handle:t.K.WaitForSingleObject(plugin_handle,3000);t.K.CloseHandle(plugin_handle)
        clean_profile()
        target.unlink(missing_ok=True)
        log.close();t.U.SetCursorPos(original.x,original.y)
        if foreground:t.U.SetForegroundWindow(foreground)
        report['checks']=t.checks
        (args.out/'host-smoke.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')

if __name__=='__main__':main()
