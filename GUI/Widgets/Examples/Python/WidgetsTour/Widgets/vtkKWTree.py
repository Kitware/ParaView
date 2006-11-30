from kwwidgets import vtkKWTree
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWTreeEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create a tree
    
    tree1 = vtkKWTree()
    tree1.SetParent(parent)
    tree1.Create()
    tree1.SelectionFillOn()
    tree1.SetBalloonHelpString("A simple tree")
    tree1.SetBorderWidth(2)
    tree1.SetReliefToGroove()
    tree1.EnableReparentingOn()
    
    tree1.AddNode(None, "inbox_node", "Inbox")
    
    tree1.AddNode(None, "outbox_node", "Outbox")
    
    tree1.AddNode(None, "kitware_node", "Kitware")
    tree1.SetNodeFontWeightToBold("kitware_node")
    tree1.SetNodeSelectableFlag("kitware_node", 0)
    tree1.OpenTree("kitware_node")
    
    tree1.AddNode("kitware_node", "berk_node", "Berk Geveci")
    
    tree1.AddNode("kitware_node", "seb_node", "Sebastien Barre")
    
    tree1.AddNode("kitware_node", "ken_node", "Ken Martin")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        tree1.GetWidgetName())
    
    
    
    return "TypeCore"
