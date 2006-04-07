/*=========================================================================

  Module:    vtkKWUserInterfaceManagerDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWUserInterfaceManagerDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWNotebook.h"
#include "vtkKWPushButton.h"
#include "vtkKWSeparator.h"
#include "vtkKWSplitFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWTopLevel.h"
#include "vtkKWTree.h"
#include "vtkKWTreeWithScrollbars.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWindowBase.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>
#include <vtksys/stl/list>
#include <vtksys/stl/algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWUserInterfaceManagerDialog);
vtkCxxRevisionMacro(vtkKWUserInterfaceManagerDialog, "1.15");

//----------------------------------------------------------------------------
class vtkKWUserInterfaceManagerDialogInternals
{
public:
  vtksys_stl::string SelectedNode;
  vtksys_stl::string SelectedSection;
  vtksys_stl::string SelectedSectionOldPackingPosition;
};

//----------------------------------------------------------------------------
vtkKWUserInterfaceManagerDialog::vtkKWUserInterfaceManagerDialog()
{
  this->Notebook    = vtkKWNotebook::New();
  this->TopLevel    = vtkKWTopLevel::New();
  this->SplitFrame  = vtkKWSplitFrame::New();
  this->CloseButton = vtkKWPushButton::New();
  this->Separator = vtkKWSeparator::New();
  this->Tree        = vtkKWTreeWithScrollbars::New();

  this->Internals = new vtkKWUserInterfaceManagerDialogInternals;

  this->PanelNodeVisibility = 0;
  this->PageNodeVisibility = 1;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManagerDialog::~vtkKWUserInterfaceManagerDialog()
{
  if (this->Notebook)
    {
    this->Notebook->Delete();
    this->Notebook = NULL;
    }

  if (this->TopLevel)
    {
    this->TopLevel->Delete();
    this->TopLevel = NULL;
    }

  if (this->SplitFrame)
    {
    this->SplitFrame->Delete();
    this->SplitFrame = NULL;
    }

  if (this->CloseButton)
    {
    this->CloseButton->Delete();
    this->CloseButton = NULL;
    }

  if (this->Separator)
    {
    this->Separator->Delete();
    this->Separator = NULL;
    }

  if (this->Tree)
    {
    this->Tree->Delete();
    this->Tree = NULL;
    }

  // Delete the container

  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkKWApplication* vtkKWUserInterfaceManagerDialog::GetApplication()
{
  if (!this->Superclass::GetApplication() &&
      this->TopLevel && this->TopLevel->GetApplication())
    {
    this->SetApplication(this->TopLevel->GetApplication());
    }

  return this->Superclass::GetApplication();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::Create()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create();

  // Create the dialog

  if (!this->TopLevel->GetMasterWindow())
    {
    this->TopLevel->SetMasterWindow(this->GetApplication()->GetNthWindow(0));
    }
  this->TopLevel->SetApplication(this->GetApplication());
  this->TopLevel->Create();
  this->TopLevel->ModalOff();
  this->TopLevel->SetSize(600, 400);
  this->TopLevel->SetMinimumSize(600, 400);

  vtkKWWidget *parent = this->TopLevel;

  // Create the splitframe

  this->SplitFrame->SetParent(parent);
  this->SplitFrame->Create();
  this->SplitFrame->SetFrame1Size(220);
  this->SplitFrame->SetFrame1MinimumSize(this->SplitFrame->GetFrame1Size());
  
  this->Script("pack %s -side top -expand y -fill both -padx 2 -pady 4", 
               this->SplitFrame->GetWidgetName());
  
  // Create the tree

  this->Tree->SetParent(this->SplitFrame->GetFrame1());
  this->Tree->Create();
  this->Tree->HorizontalScrollbarVisibilityOff();
  this->Tree->VerticalScrollbarVisibilityOff();
  this->Tree->SetPadX(0);
  this->Tree->SetPadY(0);
  this->Tree->SetBorderWidth(2);
  this->Tree->SetReliefToSunken();

  vtkKWTree *tree = this->Tree->GetWidget();
  tree->SetHighlightThickness(0);
  tree->RedrawOnIdleOn();
  tree->SelectionFillOn();
  tree->SetWidth(350 / 8);
  tree->SetSelectionChangedCommand(this, "SelectionChangedCallback");

  this->Script("pack %s -side top -expand y -fill both -padx 1", 
               this->Tree->GetWidgetName());
    
  // Separator

  this->Separator->SetParent(parent);
  this->Separator->Create();
  
  this->Script("pack %s -side top -fill x  -padx 2 -pady 0", 
               this->Separator->GetWidgetName());
  
  // Close button

  this->CloseButton->SetParent(parent);
  this->CloseButton->Create();
  this->CloseButton->SetText(
    ks_("User Interface Manager Dialog|Button|Close"));
  this->CloseButton->SetWidth(20);
  this->CloseButton->SetCommand(this->TopLevel, "Withdraw");
  
  this->Script("pack %s -side top -anchor e -fill none -padx 2 -pady 4", 
               this->CloseButton->GetWidgetName());
  
  // Create the notebook
  // Don't pack it though, it's just here fore storage

  this->Notebook->SetParent(this->SplitFrame->GetFrame2());
  this->Notebook->Create();
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::AddPage(
  vtkKWUserInterfacePanel *panel, 
  const char *title, 
  const char *balloon, 
  vtkKWIcon *icon)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not add a page if the manager has not been created.");
    return -1;
    }
 
  if (!panel)
    {
    vtkErrorMacro("Can not add a page to a NULL panel.");
    return -1;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not add a page to a panel that is not in the manager.");
    return -1;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to add a page to.");
    return -1;
    }

  // Use the panel id as a tag in the notebook, so that the pages belonging
  // to this panel will correspond to notebook pages sharing a same tag.

  return this->Notebook->AddPage(title, balloon, icon, tag);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceManagerDialog::GetPageWidget(int id)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not query a page if the manager has not been created.");
    return NULL;
    }

  // Since each page has a unique id, whatever the panel it belongs to, just 
  // retrieve the frame of the corresponding notebook page.

  return this->Notebook->GetFrame(id);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceManagerDialog::GetPageWidget(
  vtkKWUserInterfacePanel *panel, 
  const char *title)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not query a page if the manager has not been created.");
    return NULL;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not query a page from a NULL panel.");
    return NULL;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not query a page from a panel that is not "
                  "in the manager.");
    return NULL;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to query a page.");
    return NULL;
    }

  // Access the notebook page that has this specific title among the notebook 
  // pages that share the same tag (i.e. among the pages that belong to the 
  // same panel). This allow pages from different panels to have the same 
  // title.

  return this->Notebook->GetFrame(title, tag);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceManagerDialog::GetPagesParentWidget(
  vtkKWUserInterfacePanel *vtkNotUsed(panel))
{
  // UI is packed in each notebook page, but we are actually going to move
  // them around, from the notebook pages to the right panel of the split 
  // frame, so they need a common parent

  return this->SplitFrame->GetFrame2();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::RaisePage(int id)
{
  this->RaiseSection(id, NULL);
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::RaisePage(
  vtkKWUserInterfacePanel *panel, 
  const char *title)
{
  this->RaiseSection(panel, title, NULL);
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::ShowPanel(
  vtkKWUserInterfacePanel *panel)
{
  this->RaiseSection(panel, NULL, NULL);
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::HidePanel(
  vtkKWUserInterfacePanel *)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::IsPanelVisible(
  vtkKWUserInterfacePanel *)
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::RemovePageWidgets(
  vtkKWUserInterfacePanel *panel)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not remove page widgets if the manager has not " 
                  "been created.");
    return 0;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not remove page widgets from a NULL panel.");
    return 0;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not remove page widgets from a panel that is not "
                  "in the manager.");
    return 0;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to remove page widgets.");
    return 0;
    }

  // Remove the pages that share the same tag (i.e. the pages that 
  // belong to the same panel).

  this->Notebook->RemovePagesMatchingTag(tag);

  return 1;
}

//----------------------------------------------------------------------------
vtkKWUserInterfacePanel* 
vtkKWUserInterfaceManagerDialog::GetPanelFromPageId(int page_id)
{
  if (!this->Notebook || !this->Notebook->HasPage(page_id))
    {
    return 0;
    }

  return this->GetPanel(this->Notebook->GetPageTag(page_id));
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::SetPanelNodeVisibility(int v)
{
  if (this->PanelNodeVisibility == v)
    {
    return;
    }

  this->PanelNodeVisibility = v;
  this->Modified();

  this->PopulateTree();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::SetPageNodeVisibility(int v)
{
  if (this->PageNodeVisibility == v)
    {
    return;
    }

  this->PageNodeVisibility = v;
  this->Modified();

  this->PopulateTree();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::NumberOfPanelsChanged()
{
  this->PopulateTree();
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::CreateAllPanels()
{
  int nb_created = 0;
  for (int i = 0; i < this->GetNumberOfPanels(); i++)
    {
    vtkKWUserInterfacePanel *panel = this->GetNthPanel(i);
    if (panel && !panel->IsCreated())
      {
      panel->Create();
      nb_created++;
      }
    }
  return nb_created;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::GetWidgetLocation(
  const char *widget, vtkKWUserInterfacePanel **panel, int *page_id)
{
  if (!widget || !*widget)
    {
    return 0;
    }

  // if the widget is the one selected, i.e. packed in the split frame
  // then retrieve its old location

  if (this->Internals->SelectedSection.size() &&
      this->Internals->SelectedSectionOldPackingPosition.size() &&
      !strcmp(widget, this->Internals->SelectedSection.c_str()))
    {
    *page_id = this->Notebook->GetPageIdFromFrameWidgetName(
      this->Internals->SelectedSectionOldPackingPosition.c_str());
    }
  else
    {
    ostrstream in_str;
    if (!vtkKWTkUtilities::GetMasterInPack(
          this->GetApplication()->GetMainInterp(), widget, in_str))
      {
      return 0;
      }
    in_str << ends;
    *page_id = this->Notebook->GetPageIdFromFrameWidgetName(in_str.str());
    in_str.rdbuf()->freeze(0);
    }

  if (*page_id < 0)
    {
    return 0;
    }
    
  *panel = this->GetPanelFromPageId(*page_id);
  if (!*panel)
    {
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::PopulateTree()
{
  if (!this->Tree || !this->Tree->IsCreated() || !this->Notebook)
    {
    return;
    }

  vtkKWTree *tree = this->Tree->GetWidget();
  if (!tree || !tree->IsCreated())
    {
    return;
    }

  int i;

  // Preserve the old selection

  vtksys_stl::string selected_node = tree->GetSelection();

  // Make sure all panels are created

  this->CreateAllPanels();

  // Remove all nodes
  // IMPORTANT: this changes the selection (since the tree is now empty)
  // thus triggers SelectionChangedCallback, which will pretty much
  // set the current selection variable in this->Internals to empty
  // and pack the currection selection widget/section back to its notebook
  // page.

  tree->DeleteAllNodes();

  // Browse each UI elements that is a children of the page's parent widget

  vtkKWWidget *parent = this->GetPagesParentWidget(NULL);
  if (!parent)
    {
    return;
    }

  vtksys_stl::string first_node;

  int nb_children = parent->GetNumberOfChildren();
  for (i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parent->GetNthChild(i);
    if (!widget)
      {
      continue;
      }

    // Is that child a labeled frame (or one inside a simple frame)

    vtkKWFrameWithLabel *frame = 
      vtkKWFrameWithLabel::SafeDownCast(widget);
    if (!frame && widget->GetNumberOfChildren() == 1)
      {
      frame = vtkKWFrameWithLabel::SafeDownCast(widget->GetNthChild(0));
      }
    if (!frame)
      {
      continue;
      }

    // Find where it is packed right now in the notebook, and retrieve the
    // corresponding notebook page id, as well as the panel it belongs to
    
    vtkKWUserInterfacePanel *panel;
    int page_id;
    if (!widget->IsPacked() || 
        !this->GetWidgetLocation(widget->GetWidgetName(), &panel, &page_id))
      {
      continue;
      }

    vtksys_stl::string parent_node;

    // Add a node for the panel, if needed

    vtksys_stl::string panel_node(parent_node);
    panel_node += "_";
    panel_node += panel->GetTclName();
    if (this->PanelNodeVisibility)
      {
      if (!tree->HasNode(panel_node.c_str()))
        {
        tree->AddNode(parent_node.c_str(), 
                      panel_node.c_str(), 
                      panel->GetName());
        tree->OpenNode(panel_node.c_str());
        tree->SetNodeSelectableFlag(panel_node.c_str(), 0);
        tree->SetNodeFontWeightToBold(panel_node.c_str());
        }
      parent_node = panel_node;
      }

    // Add a node for the page, if needed

    vtksys_stl::string page_node(panel_node);
    page_node += "_";
    page_node += this->Notebook->GetFrame(page_id)->GetTclName();
    if (this->PageNodeVisibility)
      {
      if (!tree->HasNode(page_node.c_str()))
        {
        tree->AddNode(parent_node.c_str(), 
                      page_node.c_str(), 
                      this->Notebook->GetPageTitle(page_id));
        tree->OpenNode(page_node.c_str());
        tree->SetNodeSelectableFlag(page_node.c_str(), 0);
        tree->SetNodeFontWeightToBold(page_node.c_str());
        }
      parent_node = page_node;
      }

    // Add a node for the section (the child)
    
    vtksys_stl::string section_node(page_node);
    section_node += "_";
    section_node += frame->GetTclName();
    if (!tree->HasNode(section_node.c_str()))
      {
      tree->AddNode(parent_node.c_str(), 
                    section_node.c_str(), 
                    frame->GetLabel()->GetText());
      tree->OpenNode(section_node.c_str());
      tree->SetNodeUserData(section_node.c_str(), widget->GetWidgetName());
      }
    if (!first_node.size())
      {
      first_node = section_node;
      }
    }

  // Try to bring back the old selection, otherwise select the first one

  if (tree->HasNode(selected_node.c_str()))
    {
    tree->SelectSingleNode(selected_node.c_str());
    }
  else if (first_node.size())
    {
    tree->SelectSingleNode(first_node.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::RaiseSection(
  vtkKWUserInterfacePanel *target_panel, 
  const char *target_page_title, 
  const char *target_section)
{
  if (!this->Tree || !this->Tree->IsCreated() || !this->Notebook)
    {
    return;
    }

  vtkKWTree *tree = this->Tree->GetWidget();
  if (!tree || !tree->IsCreated())
    {
    return;
    }

  int i;
  vtkKWUserInterfacePanel *panel;
  int page_id;

  // If target section is not specified, and the current selection
  // matches the target panel (and eventually page title) already, then keep
  // the current selection, it's as good as it gets
  
  if (target_panel && 
      (!target_section || !*target_section) &&
      this->GetWidgetLocation(
        this->Internals->SelectedSection.c_str(), &panel, &page_id) && 
      target_panel == panel && 
      (!target_page_title || !*target_page_title ||
       !strcmp(target_page_title, this->Notebook->GetPageTitle(page_id))))
    {
    this->TopLevel->Display();
    return;
    }

  // There is no real way to find out when to populate the tree again,
  // even by maintaining a count of the chilren, so let's repopulate

  this->PopulateTree();
  tree->ClearSelection();

  // Browse each UI elements that is a children of the page's parent widget

  vtkKWWidget *parent = this->GetPagesParentWidget(NULL);
  if (!parent)
    {
    return;
    }

  int nb_children = parent->GetNumberOfChildren();
  for (i = 0; i < nb_children; i++)
    {
    vtkKWWidget *widget = parent->GetNthChild(i);
    if (!widget)
      {
      continue;
      }

    // Is that child a labeled frame (or one inside a simple frame)

    vtkKWFrameWithLabel *frame = 
      vtkKWFrameWithLabel::SafeDownCast(widget);
    if (!frame && widget->GetNumberOfChildren() == 1)
      {
      frame = vtkKWFrameWithLabel::SafeDownCast(widget->GetNthChild(0));
      }
    if (!frame)
      {
      continue;
      }

    // Find where it is packed right now in the notebook, and retrieve the
    // corresponding notebook page id, as well as the panel it belongs to
    
    if (!widget->IsPacked() || 
        !this->GetWidgetLocation(widget->GetWidgetName(), &panel, &page_id))
      {
      continue;
      }

    // Does it match what we are looking for ?

    if ((!target_panel || target_panel == panel) &&
        (!target_page_title || !*target_page_title || 
         !strcmp(target_page_title, this->Notebook->GetPageTitle(page_id))) &&
        (!target_section || !*target_section ||
         !strcmp(target_section, frame->GetLabel()->GetText())))
      {
      vtksys_stl::string node;
      node += "_";
      node += panel->GetTclName();
      node += "_";
      node += this->Notebook->GetFrame(page_id)->GetTclName();
      node += "_";
      node += frame->GetTclName();
      if (tree->HasNode(node.c_str()))
        {
        tree->SelectSingleNode(node.c_str());
        this->ShowSelectedNodeSection();
        this->TopLevel->Display();
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::RaiseSection(
  int target_page_id, 
  const char *target_section)
{
  vtkKWUserInterfacePanel *target_panel = 
    this->GetPanelFromPageId(target_page_id);
  const char *target_page_title = NULL;
  if (this->Notebook)
    {
    target_page_title = this->Notebook->GetPageTitle(target_page_id);
    }
  this->RaiseSection(target_panel, target_page_title, target_section);
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::SetVerticalScrollbarVisibility(int arg)
{
  if (this->Tree)
    {
    this->Tree->SetVerticalScrollbarVisibility(arg);
    }
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::GetVerticalScrollbarVisibility()
{
  if (this->Tree)
    {
    return this->Tree->GetVerticalScrollbarVisibility();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::SelectionChangedCallback()
{
  this->ShowSelectedNodeSection();
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManagerDialog::ShowSelectedNodeSection()
{
  if (!this->Tree || !this->Tree->IsCreated())
    {
    return 0;
    }

  vtkKWTree *tree = this->Tree->GetWidget();
  if (!tree || !tree->IsCreated())
    {
    return 0;
    }

  // First unpack the previously selected section

  if (this->Internals->SelectedSection.size() &&
      this->Internals->SelectedSectionOldPackingPosition.size())
    {
    this->Script("pack %s -in %s", 
                 this->Internals->SelectedSection.c_str(), 
                 this->Internals->SelectedSectionOldPackingPosition.c_str());
    }

  // Then pack the selected section
  // Make sure we save where the section was packed previously, so that
  // it can be moved back properly

  vtksys_stl::string selected_node, selected_section, selected_section_old_pos;
  int res = 0;

  if (tree->HasSelection())
    {
    selected_node = tree->GetSelection();
    selected_section = tree->GetNodeUserData(selected_node.c_str());
    if (selected_section.size())
      {
      ostrstream in_str;
      if (vtkKWTkUtilities::GetMasterInPack(
            this->GetApplication()->GetMainInterp(), 
            selected_section.c_str(), 
            in_str))
        {
        in_str << ends;
        selected_section_old_pos = in_str.str();
        tree->SeeNode(selected_node.c_str());
        this->Script("pack %s -in %s", 
                     selected_section.c_str(), 
                     this->SplitFrame->GetFrame2()->GetWidgetName());
        res = 1;
        }
      in_str.rdbuf()->freeze(0);
      }
    }

  if (res)
    {
    this->Internals->SelectedNode = selected_node;
    this->Internals->SelectedSection = selected_section;
    this->Internals->SelectedSectionOldPackingPosition = 
      selected_section_old_pos;
    }
  else
    {
    this->Internals->SelectedNode = "";
    this->Internals->SelectedSection = "";
    this->Internals->SelectedSectionOldPackingPosition = "";
    }

  return res;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManagerDialog::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TopLevel: " << this->TopLevel << endl;
}
