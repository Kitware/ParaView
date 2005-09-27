vtkKWTesting pv_testing
proc CompareMultipleImages { } {
   global argc argv

   for {set i 1} {$i < $argc} {incr i} {
     pv_testing AddArgument "[lindex $argv $i]"
   }

   set threshold -1
   
   if {$threshold == -1} {
      set threshold 10
   }

   set res [ pv_testing RegressionTest ${threshold} ]

   pv_testing Delete  
   return $res
}
