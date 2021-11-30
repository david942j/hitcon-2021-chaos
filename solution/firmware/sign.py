#!/usr/bin/env python3

import hashlib
import struct
import sys
from Crypto.PublicKey import RSA # pip install pycryptodome
from Crypto.Util.number import isPrime, inverse
import random

img = open(sys.argv[1], 'rb').read()
hsh = hashlib.sha256(img).digest()
key = RSA.import_key(open('build/key.pem', 'rb').read())
n = key.n
e = 65537
while True:
    new_n = n + (random.randint(0, 2 ** 32) << 1024)
    assert(new_n & ((1 << 1024) - 1) == n)
    if isPrime(new_n):
        n = new_n
        break
d = inverse(e, n - 1)

with open(sys.argv[2], 'wb') as f:
    f.write(struct.pack('I', len(img)))
    f.write(b'\x84')
    f.write(n.to_bytes(255, 'little'))
    sig = pow(int.from_bytes(hsh, 'little'), d, n)
    f.write(sig.to_bytes(256, 'little'))
    f.write(img)
