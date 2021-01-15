if [[ $(mount | grep nfs4) ]]; then
    echo "efs fs-89d4d58c mounted"
else
    sudo mount -t efs fs-89d4d58c:/ ~/efs
    if [ $? -eq "0" ]; then
        echo "efs fs-89d4d58c mounted"
    else
        echo "efs mount failed"
    fi
fi

PROCESS_ID=$(ps aux | grep udp_closedloop_client | awk '{print $2}' | head -1)
kill ${PROCESS_ID}