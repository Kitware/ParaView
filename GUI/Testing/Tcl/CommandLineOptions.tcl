$Application ExitAfterLoadScriptOn
$Application PromptBeforeExitOff

# override the 1MB composite threshold default to avoid
# compositing during testing, unless requested by the
# test
set view [$Application GetMainView]
if { $view != "" } {
    set ui [$view GetRenderModuleUI]
    catch {$ui SetCompositeThreshold 10}
}

proc ParseCommandLine {view argv argc} {
   global DataDir rmui TempDir
   if {[info exists argc]} { 
      set argcm1 [expr $argc - 1]
      for {set i 0} {$i < $argcm1} {incr i} {
         if {[lindex $argv $i] == "-D" && $i < $argcm1} {
            set DataDir [lindex $argv [expr $i + 1]]
         }
         if {[lindex $argv $i] == "-UC" && $i < $argcm1} {
            set rmui [$view GetRenderModuleUI]
            catch {$rmui SetCompositeThreshold 0}
         }
         if {[lindex $argv $i] == "-T" && $i < $argcm1} {
            set TempDir [lindex $argv [expr $i + 1]]
         }
      }
   }
}
