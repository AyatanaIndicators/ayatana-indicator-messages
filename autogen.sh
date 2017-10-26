#!/bin/sh

PKG_NAME="ayatana-indicator-messages"

which mate-autogen || {
	echo "You need mate-common from the MATE Desktop Environment"
	exit 1
}

. mate-autogen
