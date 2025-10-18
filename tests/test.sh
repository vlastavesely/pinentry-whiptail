#!/bin/sh

# Simulate a GPG request to pinentry
cat <<EOF | PINENTRY_USER_DATA='root=,red;entry=white,brightred' ./pinentry-whiptail
SETDESC Test GPG passphrase entry
OPTION ttytype=xterm-256color
SETDESC Please enter the passphrase to unlock the OpenPGP secret key:%0A%22Vlasta Vesely <vlastavesely@proton.me>%22%0A4096-bit RSA key, ID AA11AA11AA11AA11,%0Acreated 2017-08-19.%0A
SETPROMPT Passphrase:
GETPIN
SETERROR Bad Passphrase (try 2 of 3)
GETPIN
BYE
EOF
