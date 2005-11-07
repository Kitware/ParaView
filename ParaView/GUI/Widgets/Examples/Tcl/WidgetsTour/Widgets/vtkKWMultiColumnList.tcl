proc vtkKWMultiColumnListEntryPoint {parent win} {

  set app [$parent GetApplication] 

  set projects {
    {"KWWidgets" "1.0" "Sebastien Barre" 1 0 "1.0 0.5 1.0" 75}
    {"ParaView" "2.3" "Ken Martin"       5 1 "1.0 0.0 0.0" 34}
    {"VolView"   "3.0" "Rick Avila"      4 1 "0.0 1.0 0.0" 55}
    {"CMake"     "3.0" "Bill Hoffman"    3 0 "0.0 0.0 1.0" 85}
  }

  # -----------------------------------------------------------------------

  # Create a multi-column list

  vtkKWMultiColumnList mcl1
  mcl1 SetParent $parent
  mcl1 Create $app
  mcl1 SetBalloonHelpString \
    "A simple multicolumn list. Columns can be resized moved and sorted.\
    Double-click on some entries to edit them."
  mcl1 MovableColumnsOn
  mcl1 SetWidth 0
  mcl1 SetPotentialCellColorsChangedCommand \
    mcl1 "RefreshColorsOfAllCellsWithWindowCommand"
  mcl1 SetColumnSortedCommand \
    mcl1 "RefreshColorsOfAllCellsWithWindowCommand"

  # Add the columns make some of them editable

  set col_index [mcl1 AddColumn "Project"] 

  set col_index [mcl1 AddColumn "Version"] 
  mcl1 SetColumnAlignmentToCenter $col_index

  set col_index [mcl1 AddColumn "Maintainer"] 
  mcl1 ColumnEditableOn $col_index
  
  set col_index [mcl1 AddColumn "Team Size"] 
  mcl1 ColumnEditableOn $col_index
  mcl1 SetColumnAlignmentToCenter $col_index

  set col_index [mcl1 AddColumn ""]
  mcl1 SetColumnFormatCommandToEmptyOutput $col_index

  set col_index [mcl1 AddColumn "Color"]
  mcl1 ColumnEditableOn $col_index
  mcl1 SetColumnFormatCommandToEmptyOutput $col_index

  # The completion command is special. Instead of displaying the value,
  # we will display a frame which length will represent the % of completion
  # In order to do so we have to hide the text and later on set a 
  # a callback on each cell that will create that internal frame

  set col_index [mcl1 AddColumn "Completion"] 
  mcl1 SetColumnLabelImageToPredefinedIcon $col_index 61
  mcl1 SetColumnWidth $col_index -75
  mcl1 ColumnResizableOff $col_index
  mcl1 ColumnStretchableOff $col_index
  mcl1 SetColumnFormatCommandToEmptyOutput $col_index

  mcl1 SetColumnLabelImageToPredefinedIcon 4 62

  # The callback that is invoked for each cell in the completion column. 

  proc CreateCompletionCellCallback {tw row col w} {
    frame $w -bg "#882233" -relief groove -bd 2 -height 10 -width [expr [mcl1 GetCellTextAsDouble $row $col] * 0.01 * 70]
    mcl1 AddBindingsToWidgetName $w
  }

  # Insert each project entry

  for {set i 0} {$i < [llength $projects]} {incr i} {
    set project [lindex $projects $i]
    mcl1 InsertCellText $i 0 [lindex $project 0]
    mcl1 InsertCellText $i 1 [lindex $project 1]
    mcl1 InsertCellText $i 2 [lindex $project 2]
    mcl1 InsertCellTextAsInt $i 3 [lindex $project 3]

    mcl1 InsertCellTextAsInt $i 4 [lindex $project 4]
    mcl1 SetCellWindowCommandToCheckButton $i 4

    mcl1 InsertCellText $i 5 [lindex $project 5]
    mcl1 SetCellWindowCommandToColorButton $i 5

    mcl1 InsertCellTextAsDouble $i 6 [lindex $project 6]
    mcl1 SetCellWindowCommand $i 6 "" "CreateCompletionCellCallback"
    }

  pack [mcl1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  return "TypeCore"
}

proc vtkKWMultiColumnListFinalizePoint {} {
  mcl1 Delete
}

