/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGroupInputsWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGroupInputsWidget.h"

#include "vtkCollection.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVSourceCollection.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVTraceHelper.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGroupInputsWidget);
vtkCxxRevisionMacro(vtkPVGroupInputsWidget, "1.31");

class vtkPVSourceVectorInternals
{
public:
  typedef vtkstd::vector<vtkPVSource*> VectorType;
  VectorType InputsVector;
};

int vtkPVGroupInputsWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVGroupInputsWidget::vtkPVGroupInputsWidget()
{
  this->Internal = new vtkPVSourceVectorInternals;
  this->CommandFunction = vtkPVGroupInputsWidgetCommand;
  
  this->PartSelectionList = vtkKWListBox::New();
  this->PartLabelCollection = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkPVGroupInputsWidget::~vtkPVGroupInputsWidget()
{
  this->PartSelectionList->Delete();
  this->PartSelectionList = NULL;
  this->PartLabelCollection->Delete();
  this->PartLabelCollection = NULL;
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->PartSelectionList->SetParent(this);
  this->PartSelectionList->ScrollbarOff();
  this->PartSelectionList->Create(app, "-selectmode extended");
  this->PartSelectionList->SetHeight(0);
  // I assume we need focus for control and alt modifiers.
  this->Script("bind %s <Enter> {focus %s}",
               this->PartSelectionList->GetWidgetName(),
               this->PartSelectionList->GetWidgetName());

  this->Script("pack %s -side top -fill both -expand t",
               this->PartSelectionList->GetWidgetName());

  // There is no current way to get a modified call back, so assume
  // the user will change the list.  This widget will only be used once anyway.
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
int vtkPVGroupInputsWidget::CheckSource(vtkPVSource *pvs)
{
  if (pvs == this->PVSource || pvs == NULL)
    {
    return 0;
    }

  // Has to meet all requirments from XML filter description.
  vtkSMInputProperty* ip = this->GetInputProperty();
  if ( !ip )
    {
    return 0;
    }
  ip->RemoveAllUncheckedProxies();
  ip->AddUncheckedProxy(pvs->GetProxy());
  if (!ip->IsInDomains())
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkSMInputProperty* vtkPVGroupInputsWidget::GetInputProperty()
{
  return vtkSMInputProperty::SafeDownCast(this->GetSMProperty());
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Initialize()
{
  int idx;
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;

  vtkPVApplication *pvApp = this->GetPVApplication();

  pvWin = this->PVSource->GetPVWindow();
  sources = pvWin->GetSourceList("Sources");

  this->PartSelectionList->DeleteAll();
  idx = 0;
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (this->CheckSource(pvs))
      {
      char* label = pvApp->GetTextRepresentation(pvs);
      this->PartSelectionList->InsertEntry(idx, label);
      delete[] label;
      ++idx;
      }
    }

  // Set visible inputs to selected.
  idx = 0;
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (this->CheckSource(pvs))
      {
      if (pvs->GetVisibility())
        {
        this->PartSelectionList->SetSelectState(idx, 1);
        }
      ++idx;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::ResetInternal()
{
  this->Initialize();
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Inactivate()
{
  int num, idx;
  vtkKWLabel* label;

  this->Script("pack forget %s", this->PartSelectionList->GetWidgetName());

  num = this->PartSelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    if (this->PartSelectionList->GetSelectState(idx))
      {
      label = vtkKWLabel::New();
      label->SetParent(this);
      label->SetText(this->PartSelectionList->GetItem(idx));
      label->Create(this->GetApplication(), "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->PartLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Accept()
{
  int idx=0;
  int state;
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;

  pvWin = this->PVSource->GetPVWindow();
  sources = pvWin->GetSourceList("Sources");

  if (this->ModifiedFlag)
    {
    this->Inactivate();
    }

  // Now loop through the input mask setting the selection states.
  this->PVSource->RemoveAllPVInputs();  
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (this->CheckSource(pvs))
      {
      state = this->PartSelectionList->GetSelectState(idx);
      if (state)
        {
        // Keep a list of selected inputs for later use.
        this->Internal->InputsVector.push_back(pvs);
        
        this->PVSource->AddPVInput(pvs);
        // SetPVinput does all this for us.
        // Special replace input feature.
        // Visibility of ALL selected input turned off.
        // Setting the input should change visibility, but
        // for some reason it only works for the first input.
        // I am lazy and do not want to debug this ...
        pvs->SetVisibility(0);
        }
      ++idx;
      }
    }

  this->Superclass::Accept();
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
// Used only before the first accept.  Not used by any call back.
// Used by tracing and saving state
void vtkPVGroupInputsWidget::SetSelectState(vtkPVSource *input, int val)
{
  int idx;
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;

  pvWin = this->PVSource->GetPVWindow();
  sources = pvWin->GetSourceList("Sources");

  // Find the source in the list.
  sources->InitTraversal();
  idx = 0;
  while ( (pvs = sources->GetNextPVSource()) )
    {
    if (pvs == input)
      {
      this->PartSelectionList->SetSelectState(idx, val);
      this->ModifiedCallback();
      return;
      }
    ++idx;
    }

  if (val == 1)
    {
    vtkErrorMacro("Could not find source: " << input->GetLabel());
    }
}


//---------------------------------------------------------------------------
void vtkPVGroupInputsWidget::Trace(ofstream *file)
{
  vtkPVSource *pvs;

  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }
  *file << "$kw(" << this->GetTclName() << ") AllOffCallback" << endl;
 
  for (unsigned int i=0; i < this->Internal->InputsVector.size(); ++i)
    {
    pvs = this->Internal->InputsVector[i];
    if ( ! pvs->GetTraceHelper()->Initialize(file))
      {
      vtkErrorMacro("Could not initialize trace for object.");
      }
    else
      {
      *file << "$kw(" << this->GetTclName() << ") SetSelectState $kw("
            << pvs->GetTclName()  << ") 1" << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::AllOnCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();

  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 1);
    }

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::AllOffCallback()
{
  int num, idx;

  num = this->PartSelectionList->GetNumberOfItems();

  for (idx = 0; idx < num; ++idx)
    {
    this->PartSelectionList->SetSelectState(idx, 0);
    }

  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::SaveInBatchScript(ofstream*)
{
}

//----------------------------------------------------------------------------
void vtkPVGroupInputsWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
