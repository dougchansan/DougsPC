import importlib.util
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('request',ROOT/'scripts/frontend_request.py')
r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)

def export_request(destination):
    base=ROOT/'3ds/LegacyBox3DS'
    exe=destination.parent/'device-rng'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                    '-I'+str(base/'include'),str(ROOT/'tests/frontend/device_rng.c'),
                    str(base/'source/frontend.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe),str(destination)],check=True)

class DeviceRngTests(unittest.TestCase):
    def test_device_export_roundtrip_and_replay(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'request.tsv';export_request(p);original=p.read_bytes()
            export_request(p);self.assertEqual(p.read_bytes(),original)
            result=r.resolve_request(p,ROOT/'manifests/heritage-001-009.json')
            self.assertEqual(result['rng']['search_attempts'],1)
            self.assertEqual(result['rng']['producer'],'LBX-DEVICE-ROLLS-1')
            self.assertEqual(len(result['entries']),9)
            for e,row in zip(result['entries'],p.read_text().splitlines()[1:]):
                fields=row.split('\t')
                self.assertEqual(e['device_rolls'],dict(zip(('ec_hex','pid_hex','ivs_hex'),fields[1:])))
                self.assertEqual(e['target_level'],json.loads((ROOT/'manifests/heritage-001-009.json').read_text())['entries'][e['national_dex']-1]['target_level'])
            for mode,row in [(2,'1\t00000000\t00000000\t00000000'),
                             (2,'1\t00000000\t00000000\tFFFFFFFF'),
                             (2,'152\t00000000\t00000000\t3FFFFFFF'),
                             (1,'1\t00000000\t00000000\t3FFFFFFF'),
                             (2,'1\tZZZZZZZZ\t00000000\t3FFFFFFF')]:
                p.write_text(f'LBXR3\t1\t0\t0\t0\t0\t{mode}\tFFFFFFFF\t1\n{row}\n')
                with self.assertRaises(ValueError):r.resolve_request(p,ROOT/'manifests/heritage-001-009.json')
