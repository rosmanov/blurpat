#!/bin/bash -
# Input:
# - BLURPAT_MASKS_DIR=./tmp/m environment variable can be set to assign path to the masks directory
# - $@ -  in-file,out-file pairs
#
# Usage example: test.sh org.jpg,out.jpg org2.jpg,out2.jpg org3.jpg,out3.jpg
dir=$(cd $(dirname "$0"); pwd)
cd $dir


declare -A img
#declare -A img=(
#[org.jpg]=out.jpg
#[org2.jpg]=out2.jpg
#[org3.jpg]=out3.jpg
#)
for pair in $@; do
  img[${pair/,*/}]=${pair/*,/}
done
if [[ ${#img[@]} == 0 ]]; then
  echo >&2 "Invalid input"
  exit 2
fi

: ${BLURPAT_MASKS_DIR:="$dir/tmp/m"}
: ${masks_dir:="$BLURPAT_MASKS_DIR"}

executable="$dir/bin/blurpat -vv -k 5 -d 100 -r 0,-200,10000,10000"
masks=`find $masks_dir/ -type f`

#####################################################################

function print_header()
{
  echo -e "\033[1;32m>>\033[0m\033[1m $1\033[0m";
}

function get_best_threshold()
{
  local org_img="$1"
  local out_img="$2"
  local result=$3

  local max_mssim=0
  local best_threshold=35
  for threshold in 35 45 60 80; do
    mssim=$($executable -t${threshold} -i "${org_img}" -o "${out_img}" $masks \
      | grep 'using MSSIM' | sed -r 's/.* ([0-9\.]+)$/\1/g')

    [[ -z "$mssim" ]] && continue

    echo Threshold: $threshold MSSIM: $mssim

    if [[ $mssim > $max_mssim ]]; then
      max_mssim=$mssim
      best_threshold=$threshold
    fi
  done

  print_header "org_img: $org_img out_img: $out_img max_mssim: $max_mssim best threshold: $best_threshold"

  eval $result="'$best_threshold'"
}

#####################################################################

for k in ${!img[@]}; do
  print_header "Processing $k -> ${img[$k]}"
  get_best_threshold "$k" "${img[$k]}" t
  $executable -t ${t} -i ${k} -o ${img[$k]} $masks
  sleep 1
done
