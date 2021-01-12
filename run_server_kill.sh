PROCESS_ID=$(ps aux | grep redirection_udp_server | awk '{print $2}' | head -1)
kill ${PROCESS_ID}
