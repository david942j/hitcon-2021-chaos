#!/usr/bin/env python3

import hashlib
import struct
import sys
from Crypto.PublicKey import RSA # pip install pycryptodome

img = open(sys.argv[1], 'rb').read()
hsh = hashlib.sha256(img).digest()
key = RSA.import_key(open('key.pem', 'rb').read())
with open(sys.argv[2], 'wb') as f:
    f.write(struct.pack('I', len(img)))
    f.write(b'\x80')
    f.write(key.n.to_bytes(255, 'little'))
    sig = pow(int.from_bytes(hsh, 'little'), key.d, key.n)
    f.write(sig.to_bytes(256, 'little'))
    f.write(img)
