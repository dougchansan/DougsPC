"""Resolve a native LBXR1 request using reviewed templates; never invent encounters."""
from __future__ import annotations
import argparse
import copy
import json
from pathlib import Path
NATURES = 'Hardy Lonely Brave Adamant Naughty Bold Docile Relaxed Impish Lax Timid Hasty Serious Jolly Naive Modest Mild Quiet Bashful Rash Calm Gentle Sassy Careful Quirky'.split()

def resolve_request(request: Path, template_manifest: Path) -> dict:
    lines=request.read_text(encoding='ascii').splitlines()
    header=lines[0].split('\t') if lines else []
    if not ((len(header)==6 and header[0]=='LBXR1') or (len(header)==9 and header[0]=='LBXR2')):
        raise ValueError('Invalid LBXR request header')
    count,box,level,nature,shiny=map(int,header[1:6])
    mode,seed,budget=0,0,1
    if header[0]=='LBXR2':
        mode=int(header[6]);budget=int(header[8])
        if len(header[7])!=8 or any(c not in '0123456789abcdefABCDEF' for c in header[7]):
            raise ValueError('Seed must be eight hex digits')
        seed=int(header[7],16)
        if mode not in (0,1) or budget not in (0,1,2):raise ValueError('Invalid RNG settings')
    dex=list(map(int,lines[1:]))
    if not (1<=count<=807 and len(dex)==count and len(set(dex))==count and
            all(1<=d<=807 for d in dex) and 0<=box<32 and count<=960-box*30 and
            0<=level<=100 and 0<=nature<=25 and 0<=shiny<=2):
        raise ValueError('Invalid count, duplicate species, options, or destination capacity')
    original=json.loads(template_manifest.read_text(encoding='utf-8'))
    templates={e['national_dex']:e for e in original['entries']}
    missing=[d for d in dex if d not in templates]
    if missing: raise ValueError(f'No reviewed template for species: {missing}. Nothing generated.')
    result=copy.deepcopy(original)
    result['name']='LegacyBox3DS selection'
    result['entries']=[]
    for d in dex:
        e=copy.deepcopy(templates[d])
        # Fixed events retain all distribution fields; editing requires a specific adapter.
        fixed=e.get('ot_policy') not in ('trainer',)
        if fixed and (level or nature or shiny or mode or budget!=1):
            raise ValueError(f'Species {d}: fixed/event template cannot be batch edited')
        if level:
            if level<e['template']['level_min']: raise ValueError(f'Species {d}: below encounter level')
            e['target_level']=level
        if nature:
            e.setdefault('competitive',{})['nature']=NATURES[nature-1]
        if shiny:
            # No guessed eligibility: require an explicit reviewed allow/deny field.
            eligibility=e['template'].get('shiny_allowed')
            if shiny==1 and eligibility is not True:
                raise ValueError(f'Species {d}: shiny eligibility unreviewed or locked')
            e.setdefault('traits',{})['shiny']='force_shiny' if shiny==1 else 'force_non_shiny'
        result['entries'].append(e)
    result['rng']={'scope':'vc_transfer_rolls','mode':'fixed' if mode else 'default','seed_hex':f'{seed:08X}' if mode else None,'search_attempts':(5000,50000,250000)[budget]}
    result['frontend_request']={'schema':header[0],'start_box':box,'status':'UNVERIFIED'}
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('request',type=Path)
    p.add_argument('output',type=Path)
    p.add_argument('--templates',type=Path,default=Path(__file__).resolve().parents[1]/'manifests/heritage-001-009.json')
    a=p.parse_args()
    result=resolve_request(a.request,a.templates)
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('Request resolved; PKHeX generation/validation still required.')
if __name__=='__main__': main()
