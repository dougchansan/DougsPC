"""Checkpointed Luma InputRedirection controller; no screen-state detection.
Protocol reference: LumaTeam/Luma3DS input_redirection.c, blob
60c18213e12301d38dfa13ceaa6198f8b36e5bf7. No upstream code incorporated.
"""
from __future__ import annotations
import argparse
import hashlib
import ipaddress
import json
import socket
import struct
import time
from pathlib import Path
BUTTONS={'A':1,'B':2,'SELECT':4,'START':8,'RIGHT':16,'LEFT':32,'UP':64,'DOWN':128,'R':256,'L':512,'X':1024,'Y':2048}

def packet(buttons=()):
    mask=0
    for button in buttons:mask|=BUTTONS[button]
    return struct.pack('<5I',0xFFF^mask,0x02000000,0x007FF7FF,0x80800081,0)

def load_profile(path):
    raw=path.read_bytes();p=json.loads(raw)
    if p.get('schema')!='LBX-CONTROL-1' or not isinstance(p.get('segments'),list) or not 1<=len(p['segments'])<=100:
        raise ValueError('Invalid control profile')
    for seg in p['segments']:
        if not isinstance(seg.get('checkpoint'),str) or not seg['checkpoint'].strip():raise ValueError('Every segment needs an observed-screen checkpoint')
        if not isinstance(seg.get('actions'),list) or not 1<=len(seg['actions'])<=60:raise ValueError('Invalid action count')
        for a in seg['actions']:
            if not isinstance(a.get('buttons'),list) or not 1<=len(a['buttons'])<=4 or any(b not in BUTTONS for b in a['buttons']):raise ValueError('Invalid buttons')
            for name,default in [('hold',0.15),('wait',0.3)]:
                t=a.get(name,default)
                if type(t) not in (int,float) or not 0.05<=t<=5:raise ValueError('Invalid hold/wait duration')
    return p,hashlib.sha256(raw).hexdigest()

def tap(sock,address,buttons,hold=0.15,wait=0.3):
    try:
        deadline=time.monotonic()+hold
        while time.monotonic()<deadline:
            sock.sendto(packet(buttons),address);time.sleep(1/60)
    finally:
        # Release even on Ctrl+C; repeated releases reduce UDP packet-loss risk.
        for _ in range(3):sock.sendto(packet(),address)
    time.sleep(wait)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('profile',type=Path);p.add_argument('--host',required=True)
    p.add_argument('--journal',type=Path,default=Path('rosalina-journal.json'))
    p.add_argument('--execute',action='store_true',help='Send input only after each screen checkpoint is confirmed')
    a=p.parse_args();profile,digest=load_profile(a.profile)
    ip=ipaddress.ip_address(a.host)
    if ip.version!=4 or not ip.is_private:raise ValueError('Use the 3DS LAN IPv4 address')
    journal={'profile_sha256':digest,'next_segment':0,'status':'unverified'}
    if a.journal.exists():
        journal=json.loads(a.journal.read_text())
        if journal.get('profile_sha256')!=digest:raise ValueError('Journal belongs to a different profile')
        n=journal.get('next_segment')
        if type(n) is not int or not 0<=n<=len(profile['segments']):raise ValueError('Invalid journal position')
    if not a.execute:
        for i,seg in enumerate(profile['segments']):print(i,seg['checkpoint'],seg['actions'])
        print('Dry run only. No input sent.');return
    with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sock:
        for i in range(journal['next_segment'],len(profile['segments'])):
            seg=profile['segments'][i]
            print(f"Observe 3DS screen: {seg['checkpoint']}")
            if input('Type READY to send this segment, or anything else to stop: ').strip()!='READY':return
            for action in seg['actions']:tap(sock,(str(ip),4950),action['buttons'],action.get('hold',0.15),action.get('wait',0.3))
            print('Inputs sent; no screen result has been verified.')
            if input('Type OBSERVED only after confirming the expected result; otherwise stop: ').strip()!='OBSERVED':return
            journal['next_segment']=i+1
            journal['status']='operator_observed_inputs_only'
            temp=a.journal.with_suffix(a.journal.suffix+'.tmp')
            temp.write_text(json.dumps(journal,indent=2)+'\n');temp.replace(a.journal)
        print('Profile finished. Bank/HOME acceptance must be recorded separately.')
if __name__=='__main__':main()
