proc ComparePNG { testImage altImage } {
   global argc argv

   if { $altImage != {} } {
       for {set i  1} {$i < [expr $argc - 1]} {incr i} {
           if {[lindex $argv $i] == "-V"} {
               set argv [lreplace $argv [expr $i + 1] [expr $i + 1] $altImage]
              break
           }
       }
   }

   vtkKWTesting testing
   testing SetComparisonImage "$testImage.png"
   for {set i 1} {$i < $argc} {incr i} {
     testing AddArgument "[lindex $argv $i]"
   }

   set threshold -1
   
   if {$threshold == -1} {
      set threshold 10
   }

   set res [ testing RegressionTest ${threshold} ]

   testing Delete  
   return $res
}
