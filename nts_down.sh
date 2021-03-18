#!/bin/bash

# Kill 210 Server, monitor, proxy
echo "========== Start Killing 210 NTSocks Process =========="
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
if pgrep server; then sudo pkill server; fi

# Kill 211 Client, monitor, proxy
ssh ntb-server2@10.176.22.211 << eeooff
echo "========== Start Killing 211 NTSocks Process =========="
if pgrep ntb-monitor; then pkill ntb-monitor; fi
if pgrep ntb_proxy; then sudo pkill ntb_proxy; fi
if pgrep client; then sudo pkill client; fi

exit
eeooff
