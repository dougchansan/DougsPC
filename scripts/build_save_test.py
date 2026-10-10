"""Build host Moon adapter checks against the pinned PKSM-Core used on 3DS."""
from pathlib import Path
import argparse, subprocess, concurrent.futures
p=argparse.ArgumentParser();p.add_argument('core',type=Path);p.add_argument('output',type=Path);p.add_argument('--harness',type=Path,default=Path('tests/frontend/moon_save.cpp'));a=p.parse_args()
root=Path(__file__).resolve().parents[1];core=a.core.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
includes=['-I'+str(root/'3ds/LegacyBox3DS/include'),'-I'+str(core/'include'),*['-I'+str(d) for d in (core/'include').iterdir() if d.is_dir()],'-I'+str(core/'memecrypto')]
sources=list((core/'source').rglob('*.cpp'))+list((core/'memecrypto').glob('*.c'))+[root/'3ds/LegacyBox3DS/source'/n for n in ('moon_save.cpp','frontend.c','sha256.c','bundle.c')]+[(root/a.harness).resolve()]
# Reuse objects when unchanged; firmware and host tests both build the upstream sources unmodified.
def compile(item):
 i,s=item;o=out/(str(i)+'.o')
 if not o.exists() or root in s.parents or s.stat().st_mtime>o.stat().st_mtime:
  subprocess.run(['gcc' if s.suffix=='.c' else 'g++','-std=c11' if s.suffix=='.c' else '-std=c++20','-O1','-g','-ffunction-sections','-fdata-sections',*includes,'-D_PKSMCORE_PERSONAL_FOLDER="'+str(core/'personals')+'/"','-DLBX_ROOT="'+str(out/'sd')+'"','-c',str(s),'-o',str(o)],check=True)
 return str(o)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,enumerate(sources)))
subprocess.run(['g++','-Wl,--gc-sections',*objects,'-o',str(out/'moon-test')],check=True)
