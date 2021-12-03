#!/usr/bin/env python3

from hashlib import sha1
import os
import random
import shutil
import signal
import string
import sys
import tempfile
import time

if len(sys.argv) != 1:
    print('Usage: {}' % sys.argv[0])
    exit(2)

def validate(nbits, resource, stamp) -> bool:
    if not stamp.startswith('1:'):
        print('only hashcash v1 supported')
        exit(1)
    if stamp.split(':')[3] != resource:
        print('resource unmatch')
        exit(1)

    i = 0
    total = 0
    N = int(nbits/8)
    hashed = sha1(stamp.encode('utf-8')).digest()

    while i < N:
        total |= hashed[i]
        i += 1

    remainder = nbits % 8
    if remainder != 0:
        total |= hashed[i] >> (8 - remainder)

    return total == 0

def proof_of_work():
    bits = 26
    rand_resource = ''.join(random.choice(string.ascii_lowercase) for i in range(8))
    print('Proof of Work - Give me the token of:')
    print('hashcash -mb{} {}'.format(bits, rand_resource))
    sys.stdout.flush()

    stamp = sys.stdin.readline().strip()

    if not validate(bits, rand_resource, stamp):
        print('invalid')
        exit(1)

def run_with(fs):
    os.execl('/home/chaos/run.sh', 'run.sh', fs)

clean_disk = os.environ.get('DISK', '/home/chaos/disk')

def pack(di, disk_dir):
    os.system(f'cd {disk_dir} && find . | cpio -o -Hnewc | gzip -9 > {di}/fs.cpio.gz')

def do_upload(target, max_size):
    print(f'File size in bytes? (MAX: {max_size})')
    n = int(input())
    if n <= 0 or n > max_size:
        print('Invalid size {}' % n)
        exit(1)
    print(f'Reading {n} bytes..')
    sys.stdout.flush()
    i = 0
    with open(target, 'wb') as f:
        while i < n:
            r = sys.stdin.buffer.read(n - i)
            if len(r) == 0:
                print('EOF detected')
                exit(1)
            f.write(r)
            i += len(r)

def upload(chal):
    tmpdir = tempfile.TemporaryDirectory(prefix=f'{chal}-')
    tmpdisk = tmpdir.name + '/disk'
    shutil.copytree(clean_disk, tmpdisk, symlinks=True)
    print('Upload firmware? (y/N)')
    if input().lower() == 'y':
        target = '/'.join([tmpdisk, 'lib', 'firmware', 'chaos'])
        do_upload(target, 10 * 1024)
        os.chmod(target, 0o444)
        print('File uploaded to /lib/firmware/chaos.')
    print('Upload run? (y/N)')
    if input().lower() == 'y':
        target = '/'.join([tmpdisk, 'home', 'chaos', 'run'])
        do_upload(target, 1024 * 1024)
        os.chmod(target, 0o755)
        print('File uploaded to /home/chaos/run.')
    return tmpdir

def run():
    proof_of_work()
    fs = upload('chaos')
    sys.stdout.flush()
    time.sleep(1)
    pack(fs.name, fs.name + '/disk')
    run_with(fs.name + '/fs.cpio.gz')

signal.alarm(240)
run()
