proc vtkKWEntryEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a entry

  set entry1 [vtkKWEntry New]
  $entry1 SetParent $parent
  $entry1 Create
  $entry1 SetBalloonHelpString "A simple entry"

  pack [$entry1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another entry but larger and read-only

  set entry2 [vtkKWEntry New]
  $entry2 SetParent $parent
  $entry2 Create
  $entry2 SetWidth 20
  $entry2 ReadOnlyOn
  $entry2 SetValue "read-only entry"
  $entry2 SetBalloonHelpString "Another entry larger and read-only"

  pack [$entry2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create another entry with a label this time

  set entry3 [vtkKWEntryWithLabel New]
  $entry3 SetParent $parent
  $entry3 Create
  $entry3 SetLabelText "Another entry with a label in front:"
  $entry3 SetBalloonHelpString \
    "This is a vtkKWEntryWithLabel i.e. a entry associated to a\
    label that can be positioned around the entry."

  pack [$entry3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6

  # -----------------------------------------------------------------------

  # Create a set of entry
  # An easy way to create a bunch of related widgets without allocating
  # them one by one

  set entry_set [vtkKWEntrySet New]
  $entry_set SetParent $parent
  $entry_set Create
  $entry_set SetBorderWidth 2
  $entry_set SetReliefToGroove
  $entry_set SetWidgetsPadX 1
  $entry_set SetWidgetsPadY 1
  $entry_set SetPadX 1
  $entry_set SetPadY 1
  $entry_set SetMaximumNumberOfWidgetsInPackingDirection 2

  for {set id 0} {$id < 4} {incr id} {

    set entry [$entry_set AddWidget $id] 
    $entry SetBalloonHelpString \
      "This entry is part of a unique set a vtkKWEntrySet,\
      which provides an easy way to create a bunch of related widgets\
      without allocating them one by one. The widgets can be layout as a\
      NxM grid."
  }

  # Let's be creative. The first one sets the value of the third one
  
  [$entry_set GetWidget 0] SetValue "Enter a value here..."
  [$entry_set GetWidget 2] SetValue "...and it will show here."

  [$entry_set GetWidget 0] SetCommand [$entry_set GetWidget 2] "SetValue"

  pack [$entry_set GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 6
}

proc vtkKWEntryGetType {} {
  return "TypeCore"
}
