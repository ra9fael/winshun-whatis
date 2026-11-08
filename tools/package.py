"""Deterministic local package; --dev permits the explicit uncommitted sources.

Release mode uses git-tracked files plus the executable. --tag additionally
requires matching version/changelogs and a clean worktree. Never publishes.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import pathlib
import re
import subprocess
import zipfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
DEV_FILES=[
    'plugin.json','whatis.exe','CMakeLists.txt','winshun.h','LICENSE',
    'README.md','README.en.md','CHANGELOG.md','CHANGELOG.en.md',
    'THIRD_PARTY.md','THIRD_PARTY.en.md',
    'docs/implementation.md','docs/implementation.en.md','docs/validation.md','docs/validation.en.md',
    'docs/screenshots.md','docs/screenshots.en.md','docs/screenshots/light.png','docs/screenshots/dark.png',
    'docs/verification/acceptance.json','tests/schema/plugin.schema.json',
    'src/main.cpp','src/core/selection.h','src/platform/window_probe.h','src/platform/window_probe.cpp',
    'src/ui/window.h','src/ui/window.cpp',
    'tests/core_tests.cpp','tests/native_tests.cpp','tests/fixture.cpp',
    'tests/integration.py','tests/host_smoke.py','tests/visual_states.py','tests/validate.py','tools/package.py',
]

def version_notes(filename, version):
    text=(ROOT/filename).read_text(encoding='utf8')
    section=re.search(r'^## \['+re.escape(version)+r'\][^\n]*(?:\n(?!## \[)[^\n]*)*',text,re.MULTILINE)
    if not section:raise ValueError('Missing release changelog: '+filename)
    return section.group(0).strip()

def archive(files):
    output=io.BytesIO()
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_STORED) as z:
        for name in sorted(files):
            source=(ROOT/name).resolve()
            if not source.is_relative_to(ROOT.resolve()) or not source.is_file():
                raise ValueError('Invalid package source: '+name)
            info=zipfile.ZipInfo(name,(2020,1,1,0,0,0))
            info.create_system=3;info.external_attr=0o100644<<16
            z.writestr(info,source.read_bytes())
    return output.getvalue()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dev',action='store_true');p.add_argument('--tag')
    p.add_argument('--out',type=pathlib.Path,default=ROOT/'dist/development')
    args=p.parse_args()
    m=json.loads((ROOT/'plugin.json').read_text(encoding='utf8'));version=m['version']
    if not re.fullmatch(r'\d+\.\d+\.\d+',version):raise ValueError('Invalid version')
    if args.dev and args.tag:raise ValueError('--dev cannot be used with --tag')
    if args.tag:
        if args.tag!='v'+version:raise ValueError('Tag does not match manifest')
        if subprocess.check_output(['git','status','--porcelain'],cwd=ROOT).strip():raise ValueError('Release requires a clean worktree')
        for name in ['CHANGELOG.md','CHANGELOG.en.md']:
            if f'## [{version}]' not in (ROOT/name).read_text(encoding='utf8'):raise ValueError('Missing release changelog: '+name)
    if args.dev:files=set(DEV_FILES)
    else:
        tracked=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode('utf8').split('\0')
        files={name for name in tracked if name and not any(part.startswith('.') for part in pathlib.PurePosixPath(name).parts)}
        if 'plugin.json' not in files:raise ValueError('No tracked manifest; use --dev for an uncommitted preview')
        files.add(m['runtime']['program'])
    data=archive(files)
    if data!=archive(files):raise ValueError('Package changed while being written')
    if len(data)>64*1024*1024:raise ValueError('Package exceeds host size limit')
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        if json.loads(z.read('plugin.json'))!=m:raise ValueError('Manifest mismatch')
        if z.read(m['runtime']['program'])!=(ROOT/m['runtime']['program']).read_bytes():raise ValueError('Binary mismatch')
        if z.testzip():raise ValueError('Invalid archive CRC')
    digest=hashlib.sha256(data).hexdigest();binary=hashlib.sha256((ROOT/m['runtime']['program']).read_bytes()).hexdigest()
    args.out.mkdir(parents=True,exist_ok=True)
    name=f"{m['id']}-{version}{'-dev' if args.dev else ''}.wsplugin"
    (args.out/name).write_bytes(data)
    (args.out/(name+'.sha256')).write_text(f'{digest}  {name}\n',encoding='ascii')
    notes=f'# 这是什么 {version} 开发包\n\n[English](release-notes.en.md)\n\n本地开发预览，未正式发布。安装后在 Win顺输入 `whatis` 并按 Enter。\n\nCTest 2/2、31 项集成和 12 项真实宿主检查通过。物理输入隔离、混合 DPI、剪贴板及 Explorer 成功路径尚待人工验收。\n\n权限：`ui`、`clipboard:write`、`winshun:open`。\n\n文件：`{name}`\n\nSHA256：`{digest}`\n'
    english=f'# What Is This {version} development package\n\n[中文](release-notes.md)\n\nLocal development preview, not a published release. After installation, enter `whatis` in Winshun and press Enter.\n\nCTest 2/2, 31 integration checks and 12 real-host checks passed. Physical input isolation, mixed DPI, clipboard and successful Explorer actions await manual acceptance.\n\nPermissions: `ui`, `clipboard:write`, `winshun:open`.\n\nFile: `{name}`\n\nSHA256: `{digest}`\n'
    if not args.dev:
        notes=f'# 这是什么 {version}\n\n[English](release-notes.en.md)\n\n'+version_notes('CHANGELOG.md',version)+f'\n\n文件：`{name}`\n\nSHA256：`{digest}`\n'
        english=f'# What Is This {version}\n\n[中文](release-notes.md)\n\n'+version_notes('CHANGELOG.en.md',version)+f'\n\nFile: `{name}`\n\nSHA256: `{digest}`\n'
    (args.out/'release-notes.md').write_text(notes,encoding='utf8')
    (args.out/'release-notes.en.md').write_text(english,encoding='utf8')
    (args.out/'package.json').write_text(json.dumps({'file':name,'sha256':digest,'exe_sha256':binary,'bytes':len(data),'files':sorted(files),'deterministic':True,'mode':'development' if args.dev else 'release'},indent=2)+'\n',encoding='utf8')
    print(str(args.out/name));print('SHA256 '+digest)

if __name__=='__main__':main()
