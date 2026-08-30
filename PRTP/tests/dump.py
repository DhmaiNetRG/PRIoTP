import os, sys, time
from priotps_tests import *
sp, cp = 6100, 6101
write_psk_conf(0, GOOD_KEY_HEX)
srv = start_server(sp, cp)
settle(1.5)
cli = start_client(cp)
settle(2.0)
sensor_burst("Hello", SERVER_IP, sp, count=1)
settle(2.5)
print("--- CLIENT LOGS ---")
print("\n".join(cli.log()[-40:]))
srv.stop()
cli.stop()
