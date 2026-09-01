import re

with open('PRTP/application/PRTP_server.c', 'r') as f:
    code = f.read()

# Fix 1: on_client_msg NULL check
code = code.replace(
    '  *response = NULL;\n\n  if( msg->type == LIST ) {',
    '  *response = NULL;\n\n  if (msg == NULL) return 0;\n\n  if( msg->type == LIST ) {'
)

# Fix 3: set is_server
code = code.replace(
    '  init_systems();\n  \n  if( signal(SIGINT, signal_handler) == SIG_IGN )',
    '  init_systems();\n  priotps_security_ctx_t *sctx = get_priotps_security_ctx();\n  if (sctx) sctx->is_server = 1;\n  \n  if( signal(SIGINT, signal_handler) == SIG_IGN )'
)

with open('PRTP/application/PRTP_server.c', 'w') as f:
    f.write(code)
