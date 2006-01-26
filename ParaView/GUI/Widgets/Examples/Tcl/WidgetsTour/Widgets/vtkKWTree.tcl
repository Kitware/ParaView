proc vtkKWTreeEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create a tree

  set tree1 [vtkKWTree New]
  $tree1 SetParent $parent
  $tree1 Create
  $tree1 SelectionFillOn
  $tree1 SetBalloonHelpString "A simple tree"
  $tree1 SetBorderWidth 2
  $tree1 SetReliefToGroove

  $tree1 AddNode "" "inbox_node" "Inbox"

  $tree1 AddNode "" "outbox_node" "Outbox"
  
  $tree1 AddNode "" "kitware_node" "Kitware"
  $tree1 SetNodeFontWeightToBold "kitware_node"
  $tree1 SetNodeSelectableFlag "kitware_node" 0
  $tree1 OpenTree "kitware_node"

  $tree1 AddNode "kitware_node" "berk_node" "Berk Geveci"
  
  $tree1 AddNode "kitware_node" "seb_node" "Sebastien Barre"
  
  $tree1 AddNode "kitware_node" "ken_node" "Ken Martin"
  
  pack [$tree1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  return "TypeCore"
}
