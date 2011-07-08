#!/bin/bash

if [ $# != 6 ] ; then
  echo "Usage: $0 infile outfile wifth height offx offy"
  exit 1
fi

gimp -i -b "(sq-crop \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\")" -b '(gimp-quit 0)'
