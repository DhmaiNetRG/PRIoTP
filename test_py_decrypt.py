import ascon._ascon as a
import sys

key = bytes.fromhex("bf473b6dd5d29bc4d9b4e2d4483c0716")
nonce = bytes.fromhex("00040000000000010000000000000000")
ct_hex = "b4504a3eed4dc38f6bcdf793d32becfca29556817a58a41285608dbb4eefc68863edf7d35e53dac31ae679e3fd950f867027a777304646a1e838f6eafe7e8732ae26da0f5de0eea559165c02f535fedf43d0e364cb03ef9655dca9db634da7f73b512f2c1880b85926ff178b8eb18f9bab49c4fb7de4f1f10d3dabeceb146baec47e7fd4fdb562bc4d9c09238e5e403ba10155cf229993f22bcffc670bf6ef2128a33238c95ec551955a7002cbb8e62e9516d287c850aaa65a8c7d814f4938c90753d6140ab4cce878ba0f38f85e06bcd719"

pt = a.ascon_decrypt(key, nonce, b"", bytes.fromhex(ct_hex), variant="Ascon-128")
if pt is None:
    print("Python Decryption Failed")
    sys.exit(1)
print("Python Decrypt of C Output (hex):", pt.hex())
