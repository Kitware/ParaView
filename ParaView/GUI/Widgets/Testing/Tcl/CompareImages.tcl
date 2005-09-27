proc CompareImage { view {threshold 10}} {
   global argc argv

   vtkKWTesting testing
   testing SetRenderView $view
   for {set i 1} {$i < $argc} {incr i} {
     testing AddArgument "[lindex $argv $i]"
   }

   set res [ testing RegressionTest ${threshold} ]

   testing Delete  
   return $res
}
