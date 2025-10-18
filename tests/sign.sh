#!/bin/sh
set -e

export PINENTRY_USER_DATA='root=,green;entry=white,green'

gpgconf --kill gpg-agent
gpg-connect-agent reloadagent /bye

echo "lacinia venetatis" >lacinia.txt
rm -f *.gpg
gpg --sign lacinia.txt
gpg --verify lacinia.txt.gpg
rm -f lacinia.txt*
