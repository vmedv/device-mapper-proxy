#!/bin/bash
set -euo pipefail

dmsetup remove dmp3
dmsetup remove dmp2
dmsetup remove dmp1
dmsetup remove zero1
dmsetup remove zero2
rmmod dmp

