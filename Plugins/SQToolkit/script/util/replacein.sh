#!/bin/bash

if [ -z "$1" ]
then
  echo "Error \$1 is not set to a search term."
  exit
fi

if [ -z "$2" ]
then
  echo -n "\$2 is not set to a replace term. Contuinue (y/n)?"
  read PMT;
  case "$PMT" in
    "y" )
    echo "Ok.";
    ;;

    * )
      echo "Stop.";
      exit
      ;;
   esac
fi

echo "Replace $1 with $2"
 
for f in `grep "$1" ./ -rInl --exclude-dir=.svn --exclude=\*~`;
do
  echo -n "in $f (y,a,n,q)?";

  if [ -z "$ALL" ]
  then
    read PMT; 
  else
    echo
  fi

  case "$PMT" in 
    "y" )
      sed -i "s/$1/$2/g" $f
      ;;

   "a" )
      ALL="1"
      PMT="y"
      sed -i "s/$1/$2/g" $f
      ;;

    * )
      echo  skip;
      ;; 

  esac;
done

