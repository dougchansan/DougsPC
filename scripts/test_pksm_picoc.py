#!/usr/bin/env python3
"""Run the injector in PKSM's PicoC fork with a MOCKED in-memory save API.

This checks interpreter compatibility and control flow, NOT real PKSM save
serialization, legality, console behavior, Bank, or HOME. No user save is read.
Usage: python scripts/test_pksm_picoc.py path/to/picoc [path/to/injector.c]
Tested with FlagBrew/picoc e9ab1c9186bcaeab9c68e449192df03e033e3ef4.
"""
from __future__ import annotations
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

MOCK = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum Generation { GEN_FOUR, GEN_FIVE, GEN_SIX, GEN_SEVEN };
enum SAV_MaxField { MAX_SLOTS, MAX_BOXES };
enum PKX_Field { SPECIES };
int writes = 0;
int decrypts = 0;
int encrypts = 0;
int confirms = 0;
int accept = 1;
int corrupt = 0;
char mock_boxes[222720];
void gui_warn(char* text) { printf("WARN:%s\n",text); }
void gui_splash(char* text) { printf("SPLASH:%s\n",text); }
int gui_choice(char* text) { confirms++; return accept; }
int pkx_box_size(enum Generation gen) { return 232; }
int sav_get_max(enum SAV_MaxField field) { return 32; }
void sav_box_decrypt() { decrypts++; }
void sav_box_encrypt() { encrypts++; }
void sav_get_pkx(char* out,int box,int slot) { memcpy(out,&mock_boxes[(box*30+slot)*232],232); }
unsigned int pkx_get_value(char* data,enum Generation gen,enum PKX_Field field) { return (unsigned char)data[8]+256*(unsigned char)data[9]; }
int pkx_is_valid(char* data,enum Generation gen) { return (unsigned char)data[0] != 254; }
void sav_inject_pkx(char* data,enum Generation gen,int box,int slot,int edits) {
    memcpy(&mock_boxes[(box*30+slot)*232],data,232);
    if (corrupt) mock_boxes[(box*30+slot)*232+8]=0;
    writes++;
}
'''


def run_case(interpreter: Path, script: str, name: str, expected_writes: int,
             expected_text: str = '', version: int = 30) -> None:
    with tempfile.TemporaryDirectory(prefix='lbx-picoc-') as directory:
        root = Path(directory)
        inbox = root / 'inbox/current'
        inbox.mkdir(parents=True)
        batch = 'a' * 64
        files = []
        rows = []
        for i in range(9):
            size = 260 if name == 'party_files' else 232
            data = bytearray(size)
            data[8] = i + 1
            p = inbox / f'{i+1:03d}_Test.pk7'
            p.write_bytes(data)
            files.append(p)
            rows.append(f'{p.name}\t{hashlib.sha256(data).hexdigest()}\t0\t{i}')
        header = f'LBX1\t{batch}\t9\t0'
        ready = root / 'ready.flag'
        ready.write_text(f'LBX1\n{batch}\n9\n')
        setup = ''
        if name == 'missing_ready': ready.unlink()
        if name == 'stale_ready': ready.write_text(f'LBX1\n{"b" * 64}\n9\n')
        if name == 'wrong_ready_count': ready.write_text(f'LBX1\n{batch}\n8\n')
        if name == 'missing_last_file': files[-1].unlink()
        if name == 'short_last_file': files[-1].write_bytes(b'bad')
        if name == 'wrong_size': files[-1].write_bytes(bytes(233))
        if name == 'invalid_pk7':
            data = bytearray(files[-1].read_bytes()); data[0] = 254; files[-1].write_bytes(data)
        if name == 'occupied_last_slot': setup = 'mock_boxes[8*232+8]=25;'
        if name == 'cancel': setup = 'accept=0;'
        if name == 'readback_mismatch': setup = 'corrupt=1;'
        if name == 'duplicate_slot': rows[-1] = rows[-1].rsplit('\t',1)[0]+'\t0'
        if name == 'out_of_range_box': rows[-1] = rows[-1].replace('\t0\t8', '\t32\t8')
        if name == 'bad_index': rows[-1] = rows[-1].replace('\t0\t8', '\tabc\t8')
        if name == 'path_traversal': rows[-1] = rows[-1].replace(files[-1].name,'../evil.pk7')
        if name == 'bad_hash': rows[-1] = rows[-1].replace(rows[-1].split('\t')[1], 'g'*64)
        if name == 'malformed_header': header = 'not a manifest'
        if name == 'truncated_manifest': rows.pop()
        if name == 'extra_row': rows.append(rows[-1])
        manifest = inbox / 'batch.tsv'
        manifest.write_text(header+'\n'+'\n'.join(rows)+'\n')
        if name == 'crlf': manifest.write_bytes(manifest.read_bytes().replace(b'\n',b'\r\n'))
        if name == 'missing_manifest': manifest.unlink()
        # Only paths and the save API are substituted. Script logic is unchanged.
        program = script.replace('#include <pksm.h>', MOCK)
        program = program.replace('/3ds/LegacyBox', root.as_posix())
        program = program.replace('int main(int argc, char** argv)', 'int lbx_main(int argc, char** argv)')
        program += f'''\nint main() {{
    char version={version}; char* args[1]; args[0]=&version; {setup}
    int result=lbx_main(1,args);
    printf("MOCK_RESULT:%d WRITES:%d DECRYPT:%d ENCRYPT:%d CONFIRMS:%d\\n",result,writes,decrypts,encrypts,confirms);
    return result;
}}\n'''
        test_file = root / 'probe.c'
        test_file.write_text(program)
        result = subprocess.run([str(interpreter), str(test_file)], capture_output=True, text=True, timeout=20)
        match = re.search(r'MOCK_RESULT:(\d+) WRITES:(\d+) DECRYPT:(\d+) ENCRYPT:(\d+) CONFIRMS:(\d+)',result.stdout)
        assert result.returncode == 0 and match, (name, result.returncode, result.stdout, result.stderr)
        _, writes, decrypts, encrypts, _ = map(int, match.groups())
        assert writes == expected_writes, (name, result.stdout)
        assert decrypts == encrypts, (name, result.stdout)
        assert expected_text in result.stdout, (name, result.stdout)
        print(f'PASS {name}: {writes} mocked writes')


def main() -> int:
    interpreter = Path(sys.argv[1]).resolve()
    script_path = Path(sys.argv[2]) if len(sys.argv)>2 else Path(__file__).resolve().parents[1]/'integrations/pksm/legacybox-batch.c'
    script = script_path.read_text()
    success = 'batch staged and read-back checked'
    cases = [
        ('sun',9,success,30),('moon',9,success,31),('ultra_sun',9,success,32),('ultra_moon',9,success,33),
        ('party_files',9,success,30),('crlf',9,success,30),
        ('wrong_game',0,'select a Sun/Moon',24),
        ('missing_ready',0,'Batch not READY',30),('stale_ready',0,'Batch not READY',30),
        ('wrong_ready_count',0,'Batch not READY',30),
        ('missing_manifest',0,'Invalid or missing',30),('malformed_header',0,'Invalid or missing',30),
        ('truncated_manifest',0,'Invalid or missing',30),('extra_row',0,'Invalid or missing',30),
        ('duplicate_slot',0,'Invalid or missing',30),('bad_index',0,'Invalid or missing',30),
        ('path_traversal',0,'Invalid or missing',30),('bad_hash',0,'Invalid or missing',30),
        ('out_of_range_box',0,'Target box does not exist',30),
        ('missing_last_file',0,'Missing, unreadable',30),('short_last_file',0,'Missing, unreadable',30),
        ('wrong_size',0,'Missing, unreadable',30),('occupied_last_slot',0,'slot is occupied',30),
        ('invalid_pk7',0,'failed PKSM structural',30),('cancel',0,'',30),
        ('readback_mismatch',1,'DO NOT SAVE',30),
    ]
    for name,writes,text,version in cases:
        run_case(interpreter,script,name,writes,text,version)
    print(f'{len(cases)}/{len(cases)} host-side PicoC/mock tests passed. Hardware and real-save tests remain required.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
