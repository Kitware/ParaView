proc vtkKWComboBoxEntryPoint {parent win} {

  set app [$parent GetApplication] 

  set days {"Monday" "Tuesday" "Wednesday" "Thursday" "Friday"}

  set numbers {"123" "42" "007" "1789"}

  # -----------------------------------------------------------------------

  # Create a combobox

  vtkKWComboBox combobox1
  combobox1 SetParent $parent
  combobox1 Create
  combobox1 SetBalloonHelpString "A simple combobox"

  for {set i 0} {$i < [llength $days]} {incr i} {

    combobox1 AddValue [lindex $days $i]
    }

  pack [combobox1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another combobox but larger and read-only

  vtkKWComboBox combobox2
  combobox2 SetParent $parent
  combobox2 Create
  combobox2 SetWidth 20
  combobox2 ReadOnlyOn
  combobox2 SetValue "read-only combobox"
  combobox2 SetBalloonHelpString "Another combobox larger and read-only"

  for {set i 0} {$i < [llength $numbers]} {incr i} {

    combobox2 AddValue [lindex $numbers $i]
    }

  pack [combobox2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another combobox with a label this time

  vtkKWComboBoxWithLabel combobox3
  combobox3 SetParent $parent
  combobox3 Create
  combobox3 SetLabelText "Another combobox with a label in front:"
  combobox3 SetBalloonHelpString \
    "This is a vtkKWComboBoxWithLabel i.e. a combobox associated to a\
    label that can be positioned around the combobox."

  for {set i 0} {$i < [llength $days]} {incr i} {

    [combobox3 GetWidget] AddValue [lindex $days $i]
    }

  pack [combobox3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a set of combobox
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  vtkKWComboBoxSet combobox_set
  combobox_set SetParent $parent
  combobox_set Create
  combobox_set SetBorderWidth 2
  combobox_set SetReliefToGroove
  combobox_set SetWidgetsPadX 1
  combobox_set SetWidgetsPadY 1
  combobox_set SetPadX 1
  combobox_set SetPadY 1
  combobox_set SetMaximumNumberOfWidgetsInPackingDirection 2

  for {set id 0} {$id < 4} {incr id} {

    set combobox [combobox_set AddWidget $id] 
    $combobox SetBalloonHelpString \
      "This combobox is part of a unique set a vtkKWComboBoxSet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid."

    for {set i 0} {$i < [llength $days]} {incr i} {

      $combobox AddValue [lindex $days $i]
      }
    }

  # Let's be creative. The first one sets the value of the third one
  
  [combobox_set GetWidget 0] SetValue "Enter a value here..."
  [combobox_set GetWidget 2] SetValue "...and it will show here."
  [combobox_set GetWidget 2] DeleteAllValues
  [combobox_set GetWidget 0] SetCommand [combobox_set GetWidget 2] {SetValue [[combobox_set GetWidget 0] GetValue]}

  # Let's be creative. The second one adds its value to the fourth one
  
  [combobox_set GetWidget 1] SetValue "Enter a value here..."
  [combobox_set GetWidget 3] SetValue "...and it will be added here."
  [combobox_set GetWidget 3] DeleteAllValues
  [combobox_set GetWidget 1] SetCommand [combobox_set GetWidget 3] {AddValue [[combobox_set GetWidget 1] GetValue]}

  pack [combobox_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  return "TypeCore"
}

proc vtkKWComboBoxFinalizePoint {} {
  combobox1 Delete
  combobox2 Delete
  combobox3 Delete
  combobox_set Delete
}

