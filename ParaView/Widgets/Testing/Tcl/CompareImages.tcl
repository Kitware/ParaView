proc CompareImage { view } {
   global argc argv

   vtkKWTesting testing
   testing SetRenderView $view
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
