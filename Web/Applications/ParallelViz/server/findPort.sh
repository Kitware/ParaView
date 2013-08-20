 #!/bin/bash

 for port in {11111..11211}
 do
   # Linux : result=`netstat -vatn | grep ":$port " | wc -l`
   # OSX   : result=`netstat -vatn | grep ".$port " | wc -l`
   result=`netstat -vatn | grep ".$port " | wc -l`
   if [ $result == 0 ]
   then
       echo "$port"
       exit 0
   fi
 done
