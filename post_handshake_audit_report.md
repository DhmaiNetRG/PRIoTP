# PRIoTPS Post-Handshake State Machine Audit

## 1. Full Call Graph after HSK_SUCCESS

**Client Call Graph:**
`	ext
transport_receive() 
  -> security_core_client_process_handshake() [HSK_SUCCESS]
  -> security_core_client_build_ack()
  -> sendto() / send() [Compact ACK sent]
main_loop() (PRTP_client.c)
  -> send_iotmsg()
      -> get_priotps_security_ctx()
      -> security_core_encrypt() [ASCON_ENCRYPT]
      -> send() (transmits SECURE BSON)
`

**Server Call Graph (Original Flawed):**
`	ext
PRTP_server.c: server_loop() poll()
  -> clients.c: read_client()
      -> recvfrom(MSG_PEEK, len=5) [SEH_FLAG_ACK]
      -> recvfrom(len=5) (consume ACK)
      -> security_core_server_process_ack() [SESSION_ESTABLISHED]
      -> return 5
  -> on_client_msg(msg = NULL)
      -> dereference msg->type [CRASH - SEGFAULT]
`

**Server Call Graph (Corrected):**
`	ext
PRTP_server.c: server_loop() poll()
  -> clients.c: read_client()
      -> recvfrom(MSG_PEEK, len=5) [SEH_FLAG_ACK]
      -> recvfrom(len=5) (consume ACK)
      -> security_core_server_process_ack() [SESSION_ESTABLISHED]
      -> return 5
  -> on_client_msg(msg = NULL)
      -> check msg == NULL, return 0 (Safely ignores)
  -> poll() triggers again for next packet
  -> clients.c: read_client()
      -> recvfrom(MSG_PEEK, len=113) [SEH_FLAG_ENC]
      -> transport_receive()
          -> security_core_decrypt() [ASCON_DECRYPT]
          -> process BSON payload
`

## 2. Runtime Logs for Every Audit Point

**Client Logs:**
`	ext
AUDIT: HSK_SEND | fd=3 | local_port=52666 | dest_ip=127.0.0.1 | dest_port=5005
AUDIT: recvfrom(len=109) | fd=3 | local_port=52666 | src_ip=127.0.0.1 | src_port=5005
AUDIT: SESSION_ADD sid=1 active=1 state=1
AUDIT: SEND_ACK dest_ip=127.0.0.1 dest_port=5005
AUDIT: ACK_SENT len=5
AUDIT: CLIENT_STATE active=1 state=1
AUDIT: ENCRYPT_CHECK session=0x64702156b200 active=1 state=1
AUDIT: SESSION_LOOKUP sid=1 found=0x64702156b200
AUDIT: ASCON_ENCRYPT payload=84
`

**Server Logs:**
`	ext
AUDIT: MSG_PEEK len=41 ip=127.0.0.1 port=52666 flags=0
AUDIT: HSK_RESPONSE_SENT | dest_ip=127.0.0.1 | dest_port=52666 | session=1
AUDIT: MSG_PEEK len=5 ip=127.0.0.1 port=52666 flags=2
AUDIT: READ_CLIENT_ACK len=5
AUDIT: SESSION_LOOKUP sid=1 found=0x64702156b200
AUDIT: SERVER_ACK sid=1
AUDIT: MSG_PEEK len=113 ip=127.0.0.1 port=52666 flags=1
AUDIT: recvfrom(len=113) | fd=4 | local_port=5005 | src_ip=127.0.0.1 | src_port=52666
AUDIT: SESSION_LOOKUP sid=1 found=0x64702156b200
AUDIT: ENCRYPT_CHECK session=0x64702156b200 active=1 state=1
AUDIT: ASCON_ENCRYPT payload=95
`

## 3. Session Table Contents After Handshake

Both client and server successfully populate session 1 dynamically.
- session_id = 1
- state = 1 (SESSION_ESTABLISHED)
- active = 1

## 4. Client State Variable Values

After receiving the SCT:
- node->handshake_done (Not applicable, client manages session solely in sec_ctx)
- sec_ctx->sessions.entries[0].state = 1
- sec_ctx->sessions.entries[0].active = 1

## 5. Server State Variable Values

After sending the SCT:
- node->handshake_done = 0

After receiving the ACK:
- node->handshake_done = 1
- sec_ctx->sessions.entries[0].state = 1

## 6. First Point Where Secure Mode Fails

The failure occurred precisely at PRTP/application/PRTP_server.c:124 immediately after the server successfully processed the client's ACK packet.

## 7. Root Cause Classification

**Fatal State Machine Interruption via NULL Pointer Dereference:**

When ead_client() safely processes the ACK packet, it consumes the datagram to prevent it from entering the BSON parser, and properly assigns *msg = NULL. 
The execution then returns to PRTP_server.c's server_loop(), which passes this msg pointer to on_client_msg(). However, on_client_msg() blindly dereferences msg->type, causing the server process to crash (Segfault).
A parallel bug exists in messages.c:free_iotmsg(), which also crashes when attempting to free the NULL pointer.

Because the server crashes upon receiving the ACK, it is never able to transition handshake_done to 1 across a steady connection state, causing all subsequent client encryptions to fail or simply disconnecting the socket. Additionally, 	ransport.c exhibited an OS-dependent sendto(EISCONN) bug on connected UDP sockets.

## 8. Exact Code Fix

1. Prevent PRTP_server.c:on_client_msg from dereferencing a NULL message.
2. Prevent messages.c:free_iotmsg from freeing a NULL message pointer.
3. Add a fallback to send() if sendto() returns EISCONN inside 	ransport.c when dispatching the ACK over a connected UDP socket.

## 9. Unified Diff Patch
The patch is saved to post_handshake_state_machine.patch in the repository root.
