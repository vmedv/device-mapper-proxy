#!/bin/bash
set -euo pipefail

make
insmod dmp.ko
dmsetup create zero1 --table "0 100 zero"
dmsetup create dmp1 --table "0 100 dmp /dev/mapper/zero1"

dmsetup create zero2 --table "0 512 zero"
dmsetup create dmp2 --table "0 512 dmp /dev/mapper/zero2"
dmsetup create dmp3 --table "0 512 dmp /dev/mapper/dmp2"

