#!/bin/bash

# Make sure script is being run as root
if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root!"
	exit 1
fi

# Read PID file, send SIGHUP to server
SERVER_FILE="/var/run/het_sched.pid"
if [ -e "$SERVER_FILE" ]; then
	kill -1 `cat $SERVER_FILE`
else
	echo "Could not open \"$SERVER_FILE\"...is the server running?"
	exit 1
fi
