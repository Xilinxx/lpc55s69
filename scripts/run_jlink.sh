#!/bin/bash
# Very advanced script to automatically start the JLink Debug server.

# Try if gnome-terminal is available...
if ! command -v JLinkGDBServer &> /dev/null
then
	echo "JLinkGDBServer is not installed... See following link:"
	echo "https://www.segger.com/products/debug-probes/j-link/tools/j-link-gdb-server/about-j-link-gdb-server/"
	exit -1
fi

TERMINAL=""
# Try if gnome-terminal is available...
if command -v gnome-terminal &> /dev/null
then
	TERMINAL="gnome-terminal"
fi

# Backup terminal client...
if ! $TERMINAL -v ""
then
	if command -v gnome-terminal &> /dev/null
	then
		TERMINAL="xterm"
	fi
fi

echo "Using $TERMINAL as terminal client"
echo "Running JLink Debug Server"
$TERMINAL -e "JLinkGDBServer -select USB -device LPC55S69 -endian little -if SWD -speed auto -noir -noLocalhostOnly"
