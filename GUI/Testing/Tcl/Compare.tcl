proc Compare {Application argv argc {threshold 10}} {
   # Fix the size of the image.
   set MainView [[$Application GetMainWindow] GetMainView]
   $MainView SetRenderWindowSize 300 300
   update
   [[$Application GetMainWindow] GetMainView] ForceRender


   for {set i  1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-C"} {
         source [lindex $argv [expr $i + 1]]
         $Application SetExitStatus [CompareImage [[$Application GetMainWindow] GetMainView] $threshold]
      }
   }

   if { ![$Application GetExitStatus] } {
      [ $MainView GetCornerAnnotation ] SetVisibility 0
      for {set i  1} {$i < [expr $argc - 1]} {incr i} {
         if {[lindex $argv $i] == "-BT"} {
            source [lindex $argv [expr $i + 1]]
         }
      }
   }
}

