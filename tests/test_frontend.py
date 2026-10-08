import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('request',ROOT/'scripts/frontend_request.py')
r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
T=ROOT/'manifests/heritage-001-009.json'
class FrontendTests(unittest.TestCase):
    def test_native_request_roundtrip(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);exe=p/'core';request=p/'request.tsv'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(ROOT/'3ds/LegacyBox3DS/include'),str(ROOT/'tests/frontend/core.c'),str(ROOT/'3ds/LegacyBox3DS/source/frontend.c'),'-o',str(exe)],check=True)
            subprocess.run([str(exe),str(request)],check=True)
            x=r.resolve_request(request,T)
            self.assertEqual([e['national_dex'] for e in x['entries']],list(range(1,10)))
            self.assertEqual(x['frontend_request']['status'],'UNVERIFIED')
    def test_rejects_missing_duplicate_and_locked(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'r'
            for text in ['LBXR1\t1\t0\t0\t0\t0\n807\n','LBXR1\t2\t0\t0\t0\t0\n1\n1\n','LBXR1\t1\t0\t0\t0\t1\n1\n','LBXR1\t1\t31\t101\t0\t0\n1\n']:
                p.write_text(text)
                with self.assertRaises(ValueError):r.resolve_request(p,T)
    def test_edit_keeps_original_template_and_metadata(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'r';p.write_text('LBXR1\t1\t7\t50\t16\t2\n1\n')
            original=json.loads(T.read_text())['entries'][0]
            e=r.resolve_request(p,T)['entries'][0]
            self.assertEqual(e['target_level'],50)
            self.assertEqual(e['competitive']['nature'],'Modest')
            self.assertEqual(e['template'],original['template'])
            self.assertEqual(e['traits']['shiny'],'force_non_shiny')
    def test_offline_assembly_preserves_current_and_matches_builder_id(self):
        import hashlib
        spec=importlib.util.spec_from_file_location('pack',ROOT/'scripts/build_template_pack.py')
        pack=importlib.util.module_from_spec(spec);spec.loader.exec_module(pack)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp);source=p/'source';source.mkdir();sd=p/'sd';sd.mkdir()
            results=[]
            for dex in (1,2):
                name=f'{dex:03d}_test.pk7';data=bytes([dex])*232
                (source/name).write_bytes(data)
                results.append({'species':dex,'file':name,'sha256':hashlib.sha256(data).hexdigest(),'legal':True})
            (source/'batch-report.json').write_text(json.dumps({'allLegal':True,'failed':0,'results':results}))
            pack.build_pack(source,sd/'templates')
            (sd/'inbox/current').mkdir(parents=True);(sd/'inbox/current/sentinel').write_text('old batch')
            (sd/'ready.flag').write_text('stale')
            exe=p/'assemble';base=ROOT/'3ds/LegacyBox3DS'
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',f'-DLBX_ROOT="{sd}"','-I'+str(base/'include'),str(ROOT/'tests/frontend/assemble.c'),*[str(base/'source'/n) for n in ('frontend.c','templates.c','bundle.c','sha256.c')],'-o',str(exe)],check=True)
            import shutil
            candidate=sd/'templates/001_test.pk7';original=candidate.read_bytes()
            candidate.write_bytes(b'x'*232)
            subprocess.run([str(exe),'expect-failure'],check=True)
            self.assertEqual((sd/'ready.flag').read_text(),'stale')
            self.assertEqual((sd/'inbox/current/sentinel').read_text(),'old batch')
            candidate.write_bytes(original)
            shutil.rmtree(sd/'inbox/frontend-new')
            subprocess.run([str(exe)],check=True)
            self.assertEqual((sd/'inbox/frontend-backup/sentinel').read_text(),'old batch')
            self.assertFalse((sd/'ready.flag').exists())
            material='\n'.join(f"{r['file']}:{r['sha256']}:7:{i}" for i,r in enumerate(results))
            self.assertIn(hashlib.sha256(material.encode()).hexdigest(),(sd/'inbox/current/batch.tsv').read_text())
    def test_rng_v2_preserves_full_unsigned_seed_and_budget(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'r';p.write_text('LBXR2\t1\t0\t0\t0\t0\t1\tFFFFFFFF\t2\n1\n')
            x=r.resolve_request(p,T)
            self.assertEqual(x['rng'],{'scope':'vc_transfer_rolls','mode':'fixed','seed_hex':'FFFFFFFF','search_attempts':250000})
            p.write_text('LBXR2\t1\t0\t0\t0\t0\t1\t00000000\t0\n1\n')
            self.assertEqual(r.resolve_request(p,T)['rng']['seed_hex'],'00000000')
            for seed,mode,budget in [('100000000',1,1),('ZZZZZZZZ',1,1),('12345678',2,1),('12345678',1,3)]:
                p.write_text(f'LBXR2\t1\t0\t0\t0\t0\t{mode}\t{seed}\t{budget}\n1\n')
                with self.assertRaises(ValueError):r.resolve_request(p,T)
