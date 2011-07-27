#!/bin/bash

for f in `ls ../*.png`; 
do 
  ff=`echo $f | cut -d/ -f2`;
  n=`echo $f | cut -d. -f4`; 
  echo -ne "$n $f -> $ff\n"
  eval `echo "gimp -i -b '(un2-run2-nmm \"$f\" \"$ff\" \"un2/$n\" \"166\" \"225\" \"190\" \"320\" )' -b '(gimp-quit 0)'"`
done
