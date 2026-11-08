"""L0: validate the real schema, bilingual file links, PE machine and package.

Requires jsonschema; also accepts build/validation-deps for an isolated install.
"""
import argparse
import hashlib
import json
import pathlib
import re
import struct
import sys
import zipfile

ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'build/validation-deps'))
from jsonschema import Draft202012Validator

def main():
    p=argparse.ArgumentParser();p.add_argument('--schema',type=pathlib.Path,default=ROOT/'tests/schema/plugin.schema.json')
    p.add_argument('--package-dir',type=pathlib.Path,default=ROOT/'dist/development')
    p.add_argument('--out',type=pathlib.Path,default=ROOT/'build/verification/static.json');args=p.parse_args()
    m=json.loads((ROOT/'plugin.json').read_text(encoding='utf8'));schema=json.loads(args.schema.read_text(encoding='utf8'))
    Draft202012Validator.check_schema(schema);Draft202012Validator(schema).validate(m)
    for text in [m['name'],m['description'],m['commands'][0]['title'],m['commands'][0]['hint']]:assert text['zh'] and text['en']
    docs=list(ROOT.glob('*.md'))+list((ROOT/'docs').rglob('*.md'))
    for doc in docs:
        counterpart=doc.with_name(doc.name[:-6]+'.md' if doc.name.endswith('.en.md') else doc.stem+'.en.md')
        assert counterpart.exists(),str(counterpart)
        text=doc.read_text(encoding='utf8');assert f'({counterpart.name})' in '\n'.join(text.splitlines()[:10]),str(doc)
        for target in re.findall(r'\]\(([^)]+)\)',text):
            if target.startswith(('http:','https:','#','mailto:')):continue
            assert (doc.parent/target.split('#')[0]).exists(),f'{doc}: {target}'
    binary=(ROOT/m['runtime']['program']).read_bytes();assert binary[:2]==b'MZ'
    offset=struct.unpack_from('<I',binary,0x3c)[0];assert binary[offset:offset+4]==b'PE\0\0'
    assert struct.unpack_from('<H',binary,offset+4)[0]==0x8664,'Must be Windows x64'
    package=json.loads((args.package_dir/'package.json').read_text())
    data=(args.package_dir/package['file']).read_bytes()
    assert hashlib.sha256(data).hexdigest()==package['sha256']
    for name,other in [('release-notes.md','release-notes.en.md'),('release-notes.en.md','release-notes.md')]:
        notes=(args.package_dir/name).read_text(encoding='utf8')
        assert f'({other})' in notes, name
        assert package['sha256'] in notes and m['version'] in notes, name
        if package['mode']=='release':
            assert f'## [{m["version"]}]' in notes and '## [Unreleased]' not in notes, name
    with zipfile.ZipFile(args.package_dir/package['file']) as z:
        assert z.read('whatis.exe')==binary
        assert json.loads(z.read('plugin.json'))==m
        assert sorted(z.namelist())==package['files']
        assert not any(n.startswith(('build/','dist/','.git/')) for n in z.namelist())
    report={'manifest_schema':True,'schema_sha256':hashlib.sha256(args.schema.read_bytes()).hexdigest(),'bilingual_docs':len(docs),'local_doc_links':True,'pe_machine':'x64','package_hash_and_contents':True,'sha256':hashlib.sha256(binary).hexdigest()}
    args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    print('PASS schema, bilingual documents, x64 binary and package contents')

if __name__=='__main__':main()
