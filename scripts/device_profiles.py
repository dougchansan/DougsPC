"""Native IV search targets from reviewed Gen I VC manifests."""
import argparse
import hashlib
import json
from pathlib import Path
IV_ORDER=('HP','Atk','Def','Spe','SpA','SpD')
HP_TYPES='Fighting Flying Poison Ground Rock Bug Ghost Steel Fire Water Grass Electric Psychic Ice Dragon Dark'.split()

def search_profile(entry):
    template=entry['template']
    if (entry.get('ot_policy')!='trainer' or template.get('origin_kind')!='virtual_console' or
        template.get('generation')!=1 or template.get('verification_status')!='cross_checked' or
        not any(x in template.get('source_game','').lower() for x in ('red','blue','yellow')) or
        not 1<=entry['national_dex']<=151):
        raise ValueError('Device search requires a reviewed, editable Gen I VC template')
    competitive=entry.get('competitive',{})
    desired=competitive.get('ivs',{})
    if any(k not in IV_ORDER or type(v) is not int or not 0<=v<=31 for k,v in desired.items()):
        raise ValueError('Invalid desired IVs')
    moves=competitive.get('moves',[])
    types=[m[len('Hidden Power '):].strip() for m in moves if m.startswith('Hidden Power ')]
    if len(set(types))>1 or any(t not in HP_TYPES for t in types):
        raise ValueError('Invalid Hidden Power target')
    profile={'dex':entry['national_dex'],'template_id':template['template_id'],
             'ivs':[desired.get(k,-1) for k in IV_ORDER],
             'hidden_power':HP_TYPES.index(types[0]) if types else -1}
    profile['hash']=hashlib.sha256(json.dumps(profile,sort_keys=True,separators=(',',':')).encode('ascii')).hexdigest()
    return profile

def build(manifest,output):
    entries=json.loads(manifest.read_text())['entries']
    profiles=[search_profile(e) for e in entries]
    if not profiles or len({p['dex'] for p in profiles})!=len(profiles):
        raise ValueError('Empty or duplicate search profiles')
    lines=['/* Generated from reviewed manifest by scripts/device_profiles.py. */',
           'static const LbxSearchProfile device_profiles[]={']
    for p in profiles:
        ivs=','.join(str(x) for x in p['ivs'])
        lines.append(f'    {{{p["dex"]},{{{ivs}}},{p["hidden_power"]},"{p["hash"]}"}},')
    lines.append('};')
    output.write_text('\n'.join(lines)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args();build(args.manifest,args.output)
