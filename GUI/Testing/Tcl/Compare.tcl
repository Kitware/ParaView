proc Compare {Application argv argc} {
   # Fix the size of the image.
   set MainView [[$Application GetMainWindow] GetMainView]
   $MainView SetRenderWindowSize 300 300
   update
   [[$Application GetMainWindow] GetMainView] ForceRender

   set didRegression 0

   for {set i  1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-C"} {
         source [lindex $argv [expr $i + 1]]
         $Application SetExitStatus [CompareImage [[$Application GetMainWindow] GetMainView]]
         set didRegression 1
      }
   }

   if { ! $didRegression } {
      for {set i  1} {$i < [expr $argc - 1]} {incr i} {
         if {[lindex $argv $i] == "-BT"} {
            source [lindex $argv [expr $i + 1]]
         }
      }
   }
}

