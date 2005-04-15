if { ![$Application GetExitStatus] } {
    set doBatch 0

    for {set i  1} {$i < [expr $argc - 1]} {incr i} {
        if {[lindex $argv $i] == "-B"} {
            set doBatch 1
            set batchName [lindex $argv [expr $i + 1]]
        }
    }
    if { $doBatch } {

        update
        [[$Application GetMainWindow] GetMainView] ForceRender

        [$Application GetMainWindow] SaveBatchScript "$batchName.pvb" 0 "$batchName.png" {}
        $Application ExitAfterLoadScriptOff
        $Application DestroyGUI
        update

        $Application LoadScript "$batchName.pvb"
        catch { file delete -force "$batchName.pvb" }
        $Application ExitAfterLoadScriptOn

        set batchValid {}
        for {set i  1} {$i < [expr $argc - 1]} {incr i} {
            if {[lindex $argv $i] == "-BV"} {
                set batchValid [lindex $argv [expr $i + 1]]
            }
        }

        for {set i  1} {$i < [expr $argc - 1]} {incr i} {
            if {[lindex $argv $i] == "-BC"} {
                source [lindex $argv [expr $i + 1]]
                $Application SetExitStatus [ComparePNG $batchName $batchValid]
            }
        }
        catch { file delete -force "$batchName.png" }

        $Application Exit
    }
}
