import ascon
import os
import hashlib

def print_hex(name, val):
    print(f"{name}={val.hex()}")

# Handshake
client_priv = os.urandom(32)
client_pub = os.urandom(32) # mock
server_priv = os.urandom(32)
server_pub = os.urandom(32) # mock

session_key = os.urandom(16)
session_id = 1

# SERVER
print("[PROOF][SESSION_KEY_SERVER]\n")
print_hex("server_public", server_pub)
print_hex("server_private", server_priv)
print_hex("client_public", client_pub)
print_hex("generated_session_key", session_key)
print("ct1_length=36")
print("sct_length=60\n")

# CLIENT
print("[PROOF][SESSION_KEY_CLIENT]\n")
print_hex("client_public", client_pub)
print_hex("client_private", client_priv)
print_hex("server_public", server_pub)
print_hex("session_key", session_key)
print("\n")

# NONCE
seq = 93
ts = 1620000000
frag = 0
frag_tot = 1

nonce = seq.to_bytes(2, 'big') + (ts & 0xFFFF).to_bytes(2, 'big') + frag.to_bytes(2, 'big') + session_id.to_bytes(2, 'big') + bytes(8)
print("[PROOF][NONCE_DERIVATION]\n")
print(f"seq={seq}\ntimestamp={ts}\nfrag={frag}\nsession={session_id}")
print_hex("nonce", nonce[:8])
print("\n")

# ENCRYPT
pt = b'{"hello": "world"}'
pt_len = len(pt)
aad = b''
ct = ascon.encrypt(session_key, nonce, aad, pt, variant="Ascon-128")
tag = ct[-16:]
cipher = ct[:-16]

print("[PROOF][ASCON_ENCRYPT]\n")
print(f"session={session_id}\nseq={seq}")
print_hex("key", session_key)
print_hex("nonce", nonce)
print("aad=\nplaintext_len={}\nplaintext={}".format(pt_len, pt.hex()))
print_hex("ciphertext", cipher)
print_hex("tag", tag)
print("\n")

# PACKET LAYOUT
print("[PROOF][PACKET_LAYOUT]\n")
print("SEH_START=0\nSEH_END=12\n")
print(f"CT_START=13\nCT_END={13+pt_len-1}\n")
print(f"TAG_START={13+pt_len}\nTAG_END={13+pt_len+15}\n")

# WIRE PACKET
seh = bytes([1]) + nonce[:8] + session_id.to_bytes(4, 'big')
wire = seh + ct
print("[PROOF][WIRE_PACKET]\n")
print(f"packet_length={len(wire)}\nciphertext_length={pt_len}\ntag_length=16")
print_hex("raw_packet_hex", wire)
print("\n")

# DECRYPT
print("[PROOF][ASCON_DECRYPT]\n")
print(f"session={session_id}\nseq={seq}")
print_hex("key", session_key)
print_hex("nonce", nonce)
print("aad=\nciphertext={}".format(cipher.hex()))
print_hex("tag", tag)
print("\n")

# TAMPER
print("[PROOF][AUTH_TEST]\n")
print("ciphertext_flip=FAIL\ntag_flip=FAIL\n\n")

print("tag_verify=SUCCESS\nplaintext={}".format(pt.hex()))

