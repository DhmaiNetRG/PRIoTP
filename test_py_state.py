import ascon._ascon as a

key = bytes.fromhex("bf473b6dd5d29bc4d9b4e2d4483c0716")
nonce = bytes.fromhex("00040000000000010000000000000000")

S = [0, 0, 0, 0, 0]
k = len(key) * 8
a_r = 12
b_r = 6
rate = 8

iv_zero_key_nonce = a.to_bytes([k, rate * 8, a_r, b_r] + (20-len(key))*[0]) + key + nonce
S[0], S[1], S[2], S[3], S[4] = a.bytes_to_state(iv_zero_key_nonce)

# Run ONE round
a.ascon_permutation(S, 1)
a.printstate(S, "State after round 1:")
