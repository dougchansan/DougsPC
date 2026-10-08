import importlib.util
import json
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('companion',ROOT/'scripts/rosalina_companion.py')
r=importlib.util.module_from_spec(spec);spec.loader.exec_module(r)
class RosalinaTests(unittest.TestCase):
    def test_protocol_neutral_and_active_low_right(self):
        self.assertEqual(struct.unpack('<5I',r.packet()),(0xFFF,0x02000000,0x007FF7FF,0x80800081,0))
        self.assertEqual(struct.unpack('<5I',r.packet(['RIGHT']))[0],0xFEF)
    def test_always_releases_after_interruption(self):
        class Sock:
            def __init__(self):self.sent=[]
            def sendto(self,data,address):self.sent.append(data)
        sock=Sock()
        with patch.object(r.time,'sleep',side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):r.tap(sock,('127.0.0.1',4950),['A'])
        self.assertEqual(sock.sent[-3:],[r.packet()]*3)
    def test_profile_requires_checkpoint_and_bounded_input(self):
        profile,digest=r.load_profile(ROOT/'integrations/rosalina/dpad-probe.json')
        self.assertEqual(len(digest),64)
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/'profile';profile['segments'][0]['actions'][0]['hold']=60
            p.write_text(json.dumps(profile))
            with self.assertRaises(ValueError):r.load_profile(p)
