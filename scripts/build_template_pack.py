"""Pack byte-preserving, PKHeX-passed candidates for offline 3DS assembly."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

def build_pack(source: Path, destination: Path):
    report=json.loads((source/'batch-report.json').read_text())
    if report.get('allLegal') is not True or report.get('failed')!=0:
        raise ValueError('Require a fully passed PKHeX batch report')
    rows=[];seen=set();filenames=set()
    for r in report['results']:
        dex=r['species'];name=r['file']
        if type(dex) is not int or not 1<=dex<=807 or dex in seen:
            raise ValueError('Pack requires unique National Dex species 1-807')
        if name in filenames or '..' in name or r.get('legal') is not True or Path(name).name!=name or not name.isascii() or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-' for c in name) or len(name)>95:
            raise ValueError('Invalid result or filename')
        data=(source/name).read_bytes();digest=hashlib.sha256(data).hexdigest()
        if len(data) not in (232,260) or r.get('sha256')!=digest:
            raise ValueError('PK7 bytes do not match the passed report')
        rows.append((dex,name,digest,data));seen.add(dex);filenames.add(name)
    if not rows:raise ValueError('Empty pack')
    # A new destination avoids merging stale library entries.
    destination.mkdir(parents=True,exist_ok=False)
    for dex,name,digest,data in rows:(destination/name).write_bytes(data)
    lines=[f'LBXT1\t{len(rows)}']+[f'{d}\t{n}\t{h}' for d,n,h,_ in rows]
    (destination/'templates.tsv').write_text('\n'.join(lines)+'\n')
    shutil.copy2(source/'batch-report.json',destination/'batch-report.json')
    return len(rows)
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('destination',type=Path)
    a=p.parse_args();print(f'Packed {build_pack(a.source,a.destination)} reviewed candidates')
