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
            self.assertEqual(result['rng']['producer'],'LBX-DEVICE-SEARCH-1')
            self.assertEqual(len(result['entries']),9)
            for e,row in zip(result['entries'],p.read_text().splitlines()[1:]):
                fields=row.split('\t')
                self.assertEqual(e['device_rolls'],dict(zip(('ec_hex','pid_hex','ivs_hex'),fields[1:4]),
                    search_attempts=int(fields[4]),best_attempt=int(fields[5]),profile_hash=fields[6]))
                self.assertTrue(1<=int(fields[5])<=int(fields[4])<=50000)
                self.assertEqual(e['target_level'],json.loads((ROOT/'manifests/heritage-001-009.json').read_text())['entries'][e['national_dex']-1]['target_level'])
            for mode,row in [(2,'1\t00000000\t00000000\t00000000'),
                             (2,'1\t00000000\t00000000\tFFFFFFFF'),
                             (2,'152\t00000000\t00000000\t3FFFFFFF'),
                             (1,'1\t00000000\t00000000\t3FFFFFFF'),
                             (2,'1\tZZZZZZZZ\t00000000\t3FFFFFFF')]:
                p.write_text(f'LBXR3\t1\t0\t0\t0\t0\t{mode}\tFFFFFFFF\t1\n{row}\n')
                with self.assertRaises(ValueError):r.resolve_request(p,ROOT/'manifests/heritage-001-009.json')

    def test_native_search_best_candidate_and_early_stop(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe=Path(tmp)/'search'
            subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-I'+str(ROOT/'3ds/LegacyBox3DS/include'),
                str(ROOT/'tests/frontend/search_rng.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe)],env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'),check=True)

    def test_search_profiles_are_bound_to_manifest_and_budget(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'request.tsv';export_request(p)
            lines=p.read_text().splitlines();row=lines[1].split('\t')
            for index,value in [(4,'250001'),(5,'0'),(6,'0'*64)]:
                bad=list(row);bad[index]=value
                p.write_text('\n'.join([lines[0],'\t'.join(bad),*lines[2:]])+'\n')
                with self.assertRaises(ValueError):r.resolve_request(p,ROOT/'manifests/heritage-001-009.json')
            p.write_text('\n'.join(lines)+'\n')
            changed=json.loads((ROOT/'manifests/heritage-001-009.json').read_text())
            changed['entries'][0]['competitive']['ivs']['Atk']=1
            manifest=Path(tmp)/'changed.json';manifest.write_text(json.dumps(changed))
            with self.assertRaises(ValueError):r.resolve_request(p,manifest)
