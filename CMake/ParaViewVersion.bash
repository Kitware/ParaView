#!/usr/bin/env bash
# Update the version component if it looks like a date or -f is given.
if test "$1" = "-f"; then shift ; n='*.*\(\)' ; else n='\{8\}\(.*\)' ; fi
if test "$#" -gt 0; then echo 1>&2 "usage: ParaViewVersion.bash [-f]"; exit 1; fi
sed -i.bak -e '
s/^\([0-9]\+\.[0-9]\+\.\)[0-9]'"$n"'$/\1'"$(date +%Y%m%d)"'\2/
' "${BASH_SOURCE%/*}/../version.txt"
rm "${BASH_SOURCE%/*}/../version.txt.bak"
