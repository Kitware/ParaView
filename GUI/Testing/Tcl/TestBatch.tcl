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
        [[$Application GetMainWindow] GetMainView] SaveAsImage ${batchName}session.png

        [$Application GetMainWindow] SaveBatchScript "$batchName.pvb" 0 "$batchName.png" {}
        $Application ExitOnReturnOff
        $Application Exit
        update

        $Application LoadScript "$batchName.pvb"
#        catch { file delete -force "$batchName.pvb" }
        $Application ExitOnReturnOn

        for {set i  1} {$i < [expr $argc - 1]} {incr i} {
            if {[lindex $argv $i] == "-BC"} {
                source [lindex $argv [expr $i + 1]]
                $Application SetExitStatus [ComparePNG ${batchName}session]
            }
        }
        catch { file delete -force "$batchName.png" }
        catch { file delete -force "$batchNamesession.png" }
    }
}
