"""Check actual window icons against app.ico, including DPI changes and cleanup.

Creates only a test-owned plugin process. Window messages target its hidden
card; no global keyboard/mouse input or user process changes are needed.
"""
from __future__ import annotations
import argparse
import ctypes as C
from ctypes import wintypes as W
import hashlib
import json
from pathlib import Path
import integration as t

ROOT = Path(__file__).resolve().parents[1]

class IconInfo(C.Structure):
    _fields_ = [('is_icon', W.BOOL), ('x', W.DWORD), ('y', W.DWORD),
                ('mask', W.HBITMAP), ('color', W.HBITMAP)]

class Bitmap(C.Structure):
    _fields_ = [('type', W.LONG), ('width', W.LONG), ('height', W.LONG),
                ('stride', W.LONG), ('planes', W.WORD), ('bits', W.WORD),
                ('pixels', W.LPVOID)]

class BitmapHeader(C.Structure):
    _fields_ = [('size', W.DWORD), ('width', W.LONG), ('height', W.LONG),
                ('planes', W.WORD), ('bits', W.WORD), ('compression', W.DWORD),
                ('image', W.DWORD), ('x', W.LONG), ('y', W.LONG),
                ('used', W.DWORD), ('important', W.DWORD)]

for library, name, result, args in [
    (t.U, 'GetIconInfo', W.BOOL, [W.HICON, C.POINTER(IconInfo)]),
    (t.U, 'GetDpiForWindow', W.UINT, [W.HWND]),
    (t.U, 'GetSystemMetricsForDpi', C.c_int, [C.c_int, W.UINT]),
    (t.U, 'LoadImageW', W.HANDLE, [W.HINSTANCE, W.LPCWSTR, W.UINT, C.c_int, C.c_int, W.UINT]),
    (t.U, 'DestroyIcon', W.BOOL, [W.HICON]),
    (t.U, 'DrawIconEx', W.BOOL, [W.HDC, C.c_int, C.c_int, W.HICON, C.c_int, C.c_int, W.UINT, W.HBRUSH, W.UINT]),
    (t.U, 'GetGuiResources', W.DWORD, [W.HANDLE, W.DWORD]),
    (t.G, 'GetObjectW', C.c_int, [W.HANDLE, C.c_int, W.LPVOID]),
    (t.G, 'CreateDIBSection', W.HBITMAP, [W.HDC, W.LPVOID, W.UINT, C.POINTER(W.LPVOID), W.HANDLE, W.DWORD]),
]:
    function = getattr(library, name)
    function.restype, function.argtypes = result, args

def icon_size(icon):
    info = IconInfo()
    if not t.U.GetIconInfo(icon, C.byref(info)):
        raise C.WinError(C.get_last_error())
    try:
        bitmap = Bitmap()
        if not t.G.GetObjectW(info.color, C.sizeof(bitmap), C.byref(bitmap)):
            raise C.WinError(C.get_last_error())
        return bitmap.width, bitmap.height
    finally:
        for handle in (info.color, info.mask):
            if handle: t.G.DeleteObject(handle)

def icon_pixels(icon, size):
    dc = t.G.CreateCompatibleDC(None)
    header = BitmapHeader(40, size, -size, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = W.LPVOID()
    bitmap = t.G.CreateDIBSection(dc, C.byref(header), 0, C.byref(pixels), None, 0)
    if not bitmap:
        t.G.DeleteDC(dc)
        raise C.WinError(C.get_last_error())
    old = t.G.SelectObject(dc, bitmap)
    try:
        C.memset(pixels, 0, size * size * 4)
        if not t.U.DrawIconEx(dc, 0, 0, icon, size, size, 0, None, 3):
            raise C.WinError(C.get_last_error())
        return C.string_at(pixels, size * size * 4)
    finally:
        t.G.SelectObject(dc, old)
        t.G.DeleteObject(bitmap)
        t.G.DeleteDC(dc)

def check_window_icons(hwnd, label, dpi=None):
    dpi = dpi or t.U.GetDpiForWindow(hwnd)
    for kind, metric, name in [(1, 11, 'large'), (0, 49, 'small')]:
        icon = t.send(hwnd, 0x7F, kind)  # WM_GETICON
        t.check(f'{label}: {name} window icon is assigned', bool(icon))
        size = t.U.GetSystemMetricsForDpi(metric, dpi)
        t.check(f'{label}: {name} icon matches DPI', icon_size(icon) == (size, size), {'dpi': dpi, 'size': icon_size(icon)})
        expected = t.U.LoadImageW(None, str(ROOT/'app.ico'), 1, size, size, 0x10)
        if not expected: raise C.WinError(C.get_last_error())
        try:
            t.check(f'{label}: {name} icon pixels match app.ico', icon_pixels(icon, size) == icon_pixels(expected, size))
        finally:
            t.U.DestroyIcon(expected)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, default=ROOT/'whatis.exe')
    parser.add_argument('--out', type=Path, default=ROOT/'build/window-icons-20261109/checks')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    driver = t.Driver(args.exe.resolve())
    try:
        response = driver.request('initialize', {'locale': 'en', 'theme': 'light', 'appearance': {}, 'settings': {}, 'pluginDir': str(ROOT), 'dataDir': str(args.out)})
        t.check('Plugin initializes', 'result' in response)
        # Initialization queues an appearance update after creating the card.
        # Its English title confirms that update has finished replacing icons.
        card = t.wait(lambda: next((hwnd for hwnd, title in t.windows(driver.p.pid, False) if title == 'What Is This · Winshun'), None))
        t.check('Icon test keeps the card hidden', not t.U.IsWindowVisible(card))
        check_window_icons(card, 'initial')
        for dpi in [96, 120, 144, 192, 96]:
            bounds = t.rect(card)
            t.send(card, 0x2E0, (dpi << 16) | dpi, C.addressof(bounds))
            check_window_icons(card, f'DPI {dpi}', dpi)
        def appearance(index):
            driver.send({'jsonrpc': '2.0', 'method': 'appearance', 'params': {
                'locale': 'en' if index % 2 else 'zh', 'theme': 'dark' if index % 2 else 'light',
                'uiSize': 'roomy' if index % 2 else 'compact'}})
            # The title changes after the UI thread has replaced its icons.
            title = 'What Is This · Winshun' if index % 2 else '这是什么 · Win顺'
            t.wait(lambda: t.read(card) == title)
        for index in range(6): appearance(index)
        before = [t.U.GetGuiResources(int(driver.p._handle), kind) for kind in [0, 1]]
        for index in range(60): appearance(index)
        after = [t.U.GetGuiResources(int(driver.p._handle), kind) for kind in [0, 1]]
        check_window_icons(card, 'after appearance changes', 96)
        t.check('Repeated appearance changes keep GUI resource counts bounded', all(b-a <= 3 for a, b in zip(before, after)), {'before': before, 'after': after})
        driver.stop()
        t.check('Plugin shuts down cleanly', driver.p.returncode == 0)
        print(f'PASS {len(t.checks)} window icon checks')
    finally:
        driver.stop()
        (args.out/'window-icons.json').write_text(json.dumps({'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(), 'checks': t.checks, 'input': 'Directed messages to a hidden test-owned window'}, ensure_ascii=False, indent=2)+'\n', encoding='utf8')

if __name__ == '__main__': main()
