#!/bin/bash


for i in `seq 0 52`
do
  q=`printf %04.0f $i`
  echo "$1.$q.png -> $2.$q.png"
  #echo \'\(fix-pv \"./$1.$q.png\" \"./$2.$q.png\"\)\' 
  #gimp -i -b \'\(fix-pv "$1.$q.png" "$2.$q.png"\)\' -b '(gimp-quit 0)'
  eval `echo "gimp -i -b '(fix-pv \"./vrl.$q.png\" \"./vrlc.$q.png\")' -b '(gimp-quit 0)'"`
done

