import socket
import struct
import time

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_address = ('127.0.0.1', 5005)

# Test 1: Empty packet
s.sendto(b'', server_address)

# Test 2: 5-byte HSK
s.sendto(b'\x00' + b'\x00'*4, server_address)

# Test 3: 109-byte HSK with garbage
s.sendto(b'\x00' + b'\xff'*108, server_address)

# Test 4: 5-byte ACK
s.sendto(b'\x02' + b'\x00'*4, server_address)

# Test 5: 113-byte ENC
s.sendto(b'\x01' + b'\x00'*112, server_address)

# Test 6: BSON packet (missing NULL terminator)
s.sendto(b'\x05\x00\x00\x00', server_address)

# Test 7: Large BSON packet
s.sendto(b'\xff\xff\xff\xff' + b'A'*2000, server_address)

print('Done fuzzing')
