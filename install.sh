#!/bin/bash

INSTALLPATH=/usr/local/bin
BINARY=cpubgline

echo -e "cpubgline  Copyright (C) 2017  Ralf Stemmer <ralf.stemmer@gmx.net>"
echo -e "This program comes with \e[1;33mABSOLUTELY NO WARRANTY\e[0m."
echo -e "This is free software, and you are welcome to redistribute it"
echo -e "under certain conditions.\n"

install -m 755 -v -s -g root -o root $BINARY -D $INSTALLPATH/$BINARY


# vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4

