proc Compare {Application argv argc} {
   # Fix the size of the image.
   set MainView [[$Application GetMainWindow] GetMainView]
   $MainView SetRenderWindowSize 300 300
   update
   [[$Application GetMainWindow] GetMainView] ForceRender


   for {set i  1} {$i < [expr $argc - 1]} {incr i} {
      if {[lindex $argv $i] == "-C"} {
         source [lindex $argv [expr $i + 1]]
         $Application SetExitStatus [CompareImage [[$Application GetMainWindow] GetMainView]]
      }
   }

   if { ![$Application GetExitStatus] } {
       #no need to hide the colormaps for batch testing anymore
       #set colorMaps [[$Application GetMainWindow] GetPVColorMaps]
       #$colorMaps InitTraversal
       #while { 1 } {
       #    set colorMap [$colorMaps GetNextItemAsObject]
       #    if { $colorMap == {} } {
       #        break
       #    }
       #    $colorMap SetScalarBarVisibility 0
       #}
       [ $MainView GetCornerAnnotation ] SetVisibility 0
      for {set i  1} {$i < [expr $argc - 1]} {incr i} {
         if {[lindex $argv $i] == "-BT"} {
            source [lindex $argv [expr $i + 1]]
         }
      }
   }
}

