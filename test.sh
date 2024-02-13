#!/bin/bash
set -euo pipefail
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

./init.sh

function t1() {
  echo -e "${GREEN}initialized succesfully${NC}"
  echo -e "${BLUE}t1: basic operations with one dmp device${NC}"
  if dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1 && 
    dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1 &&
    cat /sys/module/dmp/zero1/stat
  then echo -e "  ${GREEN}passed${NC}"
  else echo -e "  ${RED}failed${NC}"
  fi
}

function t2() {
  echo -e "${BLUE}t2: check no intersections between different devices${NC}"
  dev1before=$(cat /sys/module/dmp/zero1/stat)
  dd of=/dev/null if=/dev/mapper/dmp2 bs=4k count=1 
  dd if=/dev/random of=/dev/mapper/dmp2 bs=4k count=1
  dev1after=$(cat /sys/module/dmp/zero1/stat)
  if [ "$dev1before" = "$dev1after" ];
  then echo -e "  ${GREEN}passed${NC}"
  else echo -e "  ${RED}failed:${NC}\nexpected: \n${dev1before}\nactual: \n${dev1after}"
  fi
}

function t3() {
  echo -e "${BLUE}t3: check that proxy correctly propagate BIOs${NC}"
  declare -a first_proxy_before 
  IFS=' ' read -a first_proxy_before <<< $(cat /sys/module/dmp/zero2/stat | tr -dc '0-9\n' | sed '/^$/d' | tr '\n' ' ' | cut -d' ' -f3,4)
  IFS=' ' read -a second_proxy_before <<< $(cat /sys/module/dmp/dmp2/stat | tr -dc '0-9\n' | sed '/^$/d' | tr '\n' ' ' | cut -d' ' -f3,4)
  dd if=/dev/random of=/dev/mapper/dmp3 bs=100 count=1
  IFS=' ' read -a first_proxy_after <<< $(cat /sys/module/dmp/zero2/stat | tr -dc '0-9\n' | sed '/^$/d' | tr '\n' ' ' | cut -d' ' -f3,4)
  IFS=' ' read -a second_proxy_after <<< $(cat /sys/module/dmp/dmp2/stat | tr -dc '0-9\n' | sed '/^$/d' | tr '\n' ' ' | cut -d' ' -f3,4)
  diffs_before=()
  for i in ${!first_proxy_before[@]}; do
    diffs_before+=($((first_proxy_before[$i] - second_proxy_before[$i])))
  done
  diffs_after=()
  for i in ${!first_proxy_after[@]}; do
    diffs_after+=($((first_proxy_after[$i] - second_proxy_after[$i])))
  done
  if [[ "" == $(echo ${diffs_before[@]} ${diffs_after[@]} | tr ' ' '\n' | sort | uniq -u) ]];
  then echo -e "  ${GREEN}passed${NC}"
  else echo -e "  ${RED}failed:${NC}\nexpected: ${diffs_before[@]}\nactual:   ${diffs_after[@]}"
  fi
}

t1
t2
t3
./end.sh

