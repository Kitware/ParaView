/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectArrays.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectArrays.h"

#include "vtkCollection.h"
#include "vtkDataSet.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWPushButton.h"
#include "vtkKWWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkSMPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkStringList.h"
#include "vtkCollectionIterator.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectArrays);
vtkCxxRevisionMacro(vtkPVSelectArrays, "1.4");
vtkCxxSetObjectMacro(vtkPVSelectArrays, InputMenu, vtkPVInputMenu);

int vtkPVSelectArraysCommand(ClientData cd, Tcl_Interp *interp,
                                int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSelectArrays::vtkPVSelectArrays()
{
  this->CommandFunction = vtkPVSelectArraysCommand;
  
  this->Field = vtkDataSet::CELL_DATA_FIELD;
  this->Deactivate = 0;
  this->FilterArrays = 0;

  this->ButtonFrame = vtkKWWidget::New();
  this->ShowAllLabel = vtkKWLabel::New();
  this->ShowAllCheck = vtkKWCheckButton::New();

  this->ArraySelectionList = vtkKWListBox::New();
  this->ArrayLabelCollection = vtkCollection::New();
  this->SelectedArrayNames = vtkStringList::New();

  this->InputMenu = NULL;
  this->Active = 1;
}

//----------------------------------------------------------------------------
vtkPVSelectArrays::~vtkPVSelectArrays()
{
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  this->ShowAllLabel->Delete();
  this->ShowAllLabel = NULL;
  this->ShowAllCheck->Delete();
  this->ShowAllCheck = NULL;

  this->ArraySelectionList->Delete();
  this->ArraySelectionList = NULL;
  this->ArrayLabelCollection->Delete();
  this->ArrayLabelCollection = NULL;
  this->SelectedArrayNames->Delete();
  this->SelectedArrayNames = NULL;

  this->SetInputMenu(NULL);
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  
  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->ButtonFrame->GetWidgetName());
  this->ShowAllLabel->SetParent(this->ButtonFrame);
  this->ShowAllLabel->Create(pvApp, "");
  this->ShowAllLabel->SetText("Show All");
  this->ShowAllCheck->SetParent(this->ButtonFrame);
  this->ShowAllCheck->Create(pvApp, "");
  this->ShowAllCheck->SetState(0);
  this->ShowAllCheck->SetCommand(this, "ShowAllArraysCheckCallback");

  this->ShowAllCheck->SetBalloonHelpString("Hide arrays that are not called 'Volume Fraction'");

  if (this->FilterArrays)
    {
    this->Script("pack %s %s -side left -fill x -expand t",
                 this->ShowAllLabel->GetWidgetName(),
                 this->ShowAllCheck->GetWidgetName());
    }
    
  this->ArraySelectionList->SetParent(this);
  this->ArraySelectionList->ScrollbarOff();
  this->ArraySelectionList->Create(app, "-selectmode extended");
  this->ArraySelectionList->SetHeight(0);
  this->ArraySelectionList->SetSingleClickCallback(this,"ModifiedCallback");
  // I assume we need focus for control and alt modifiers.
  this->Script("bind %s <Enter> {focus %s}",
               this->ArraySelectionList->GetWidgetName(),
               this->ArraySelectionList->GetWidgetName());

  this->Script("pack %s -side top -fill both -expand t",
               this->ArraySelectionList->GetWidgetName());
  this->ArraySelectionList->SetBalloonHelpString("Select parts to extract. Use control key for toggling selection. Use shift key for extended selection");

  // There is no current way to get a modified call back, so assume
  // the user will change the list.  This widget will only be used once anyway.
  this->ModifiedCallback();
}


//----------------------------------------------------------------------------
void vtkPVSelectArrays::Inactivate()
{
  const char* arrayName;
  int num, idx;
  vtkKWLabel* label;

  this->Active = 0;

  this->Script("pack forget %s %s", this->ButtonFrame->GetWidgetName(),
               this->ArraySelectionList->GetWidgetName());

  this->SelectedArrayNames->RemoveAllItems();
  num = this->ArraySelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    if (this->ArraySelectionList->GetSelectState(idx))
      {
      arrayName = this->ArraySelectionList->GetItem(idx);
      this->SelectedArrayNames->AddString(arrayName);
      label = vtkKWLabel::New();
      label->SetParent(this);
      label->SetText(arrayName);
      label->Create(this->GetApplication(), "");
      this->Script("pack %s -side top -anchor w",
                   label->GetWidgetName());
      this->ArrayLabelCollection->AddItem(label);
      label->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::Accept()
{
  int num, idx;
  const char* arrayName;
  int state;
  int modFlag = this->GetModifiedFlag();

  if ( ! this->Active)
    {
    return;
    }
  
  num = this->ArraySelectionList->GetNumberOfItems();

  vtkPVApplication *pvApp = this->GetPVApplication();

  if (modFlag && this->Deactivate)
    {
    this->Inactivate();
    }
 
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (!svp)
    {
    return;
    }

  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->PVSource->GetVTKSourceID(0) << "RemoveAllVolumeArrayNames"
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  int count = 0;
  
  // Now loop through the input mask setting the selection states.
  svp->SetNumberOfElements(0);
  for (idx = 0; idx < num; ++idx)
    {
    state = this->ArraySelectionList->GetSelectState(idx);
    if (state)
      {
      arrayName = this->ArraySelectionList->GetItem(idx); 
      svp->SetElement(count, arrayName);
      count++;
      }
    }

  this->Superclass::Accept();
}


//---------------------------------------------------------------------------
void vtkPVSelectArrays::ClearAllSelections()
{
  int idx, num;

  if ( ! this->Active)
    {
    vtkErrorMacro("Trying to change the selection of an inactive widget.");
    return;
    }

  num = this->ArraySelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    this->ArraySelectionList->SetSelectState(idx, 0);
    }  
}

//---------------------------------------------------------------------------
void vtkPVSelectArrays::SetSelectState(const char* arrayName, int val)
{
  int idx, num;
  const char* listArrayName;

  if ( ! this->Active)
    {
    vtkErrorMacro("Trying to change the selection of an inactive widget.");
    return;
    }

  num = this->ArraySelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    listArrayName = this->ArraySelectionList->GetItem(idx);
    if (strcmp(arrayName,listArrayName) == 0)
      {
      this->ModifiedCallback();
      this->ArraySelectionList->SetSelectState(idx, val);
      return;
      }
    }

  vtkErrorMacro("Could not find array with name " << arrayName);
}


//---------------------------------------------------------------------------
void vtkPVSelectArrays::Trace(ofstream *file)
{
  int num, idx, state;
  const char* arrayName;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") ClearAllSelections\n";

  num = this->ArraySelectionList->GetNumberOfItems();
  for (idx = 0; idx < num; ++idx)
    {
    state = this->ArraySelectionList->GetSelectState(idx);
    if (state)
      {
      arrayName = this->ArraySelectionList->GetItem(idx); 
      *file << "$kw(" << this->GetTclName() << ") SetSelectState {"
            << arrayName << "} 1\n"; 
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::Initialize()
{
  if (this->Active)
    {
    this->ArraySelectionList->DeleteAll();
    this->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::ResetInternal()
{
  this->Initialize();
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::Update()
{
  int showAll = this->ShowAllCheck->GetState();
  int volumeFlag;
  int voidFlag;
  int num, idx;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;

  if ( ! this->Active)
    {
    return;
    }

  this->ArraySelectionList->DeleteAll();
  if (this->InputMenu == NULL)
    {
    return;
    }    

  if (this->Field == vtkDataSet::CELL_DATA_FIELD)
    {
    attrInfo = this->InputMenu->GetCurrentValue()->GetDataInformation()->GetCellDataInformation();
    }
  else
    {
    attrInfo = this->InputMenu->GetCurrentValue()->GetDataInformation()->GetPointDataInformation();
    }
  num = attrInfo->GetNumberOfArrays();
  int count = 0;
  for (idx = 0; idx < num; ++idx)
    {
    arrayInfo = attrInfo->GetArrayInformation(idx);
    if ( ! this->FilterArrays)
      {
      this->ArraySelectionList->InsertEntry(count, arrayInfo->GetName());
      this->ArraySelectionList->SetSelectState(count, 1);
      ++count;
      }
    else if (arrayInfo->GetNumberOfComponents() == 1)
      { // Number of components is hard coded into the filter.
      // It may not be necessary.  Vectors would not have name volume fraction.
      volumeFlag = this->StringMatch(arrayInfo->GetName());
      voidFlag = 0;
      if (strncmp(arrayInfo->GetName(), "Void", 4) == 0
          || strncmp(arrayInfo->GetName(), "void", 4) == 0)
        {
        voidFlag = 1;
        }
      if (showAll || ! this->FilterArrays || this->StringMatch(arrayInfo->GetName()))
        {
        this->ArraySelectionList->InsertEntry(count, arrayInfo->GetName());
        // It would be nice to get rid of the void volume fraction.
        if (volumeFlag && ! voidFlag)
          {
          this->ArraySelectionList->SetSelectState(count, 1);
          }
        ++count;
        }
      }
    }
  // Update now clears selection.
  // We could try to restore the selections.
  // Now loop through the input mask setting the selection states.
  //for (idx = 0; idx < num; ++idx)
  //  {
  //  this->Script("%s SetSelectState %d [%s GetInputMask %d]",
  //               this->PartSelectionList->GetTclName(),
  //               idx, vtkSourceTclName, idx);
  //  }
  // Because list box does not notify us when it is modified ...
  //this->ModifiedFlag = 0;

}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::ShowAllArraysCheckCallback()
{
  this->Update();
}

//----------------------------------------------------------------------------
int vtkPVSelectArrays::StringMatch(const char* arrayName)
{
  const char* p;
  p = arrayName;

  while (*p != '\0')
    {
    if (strncmp(p,"Fraction",8) == 0 || strncmp(p, "fraction", 8) == 0)
      {
      return 1;
      }
    else if (strncmp(p,"VOLM",4) == 0)
      {
      return 1;
      }
    ++p;
    }
  return 0;
}

//----------------------------------------------------------------------------
// Multiple input filter has only one VTK source.
void vtkPVSelectArrays::SaveInBatchScript(ofstream *file)
{
  int num, idx;

  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  num = this->SelectedArrayNames->GetNumberOfStrings();

  *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
        << " GetProperty AddVolumeArrayName] SetNumberOfElements "
        << num << endl;

  // Now loop through the input mask setting the selection states.
  for (idx = 0; idx < num; ++idx)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty "
          << this->SMPropertyName << "] SetElement " << idx << " {"
          << this->SelectedArrayNames->GetString(idx) << "}" << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVSelectArrays::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectArrays* pvsa = vtkPVSelectArrays::SafeDownCast(clone);
  if (pvsa)
    {
    pvsa->Field = this->Field;
    pvsa->Deactivate = this->Deactivate;
    pvsa->FilterArrays = this->FilterArrays;
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvsa->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVSelectArrays.");
    }
}

//----------------------------------------------------------------------------
int vtkPVSelectArrays::ReadXMLAttributes(vtkPVXMLElement* element,
                                            vtkPVXMLPackageParser* parser)
{
  if (!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
    
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if (input_menu)
    {
    vtkPVXMLElement* ime = element->LookupElement(input_menu);
    if (!ime)
      {
      vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
      return 0;
      }
    
    vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
    if(!imw)
      {
      if(w) { w->Delete(); }
      vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
      return 0;
      }
    imw->AddDependent(this);
    this->SetInputMenu(imw);
    imw->Delete();
    }
  
  const char* field = element->GetAttribute("field");
  if (field)
    {
    if (strcmp(field, "Cell") == 0)
      {
      this->Field = vtkDataSet::CELL_DATA_FIELD;
      }
    else if (strcmp(field, "Point") == 0)
      {
      this->Field = vtkDataSet::POINT_DATA_FIELD;
      }
    else
      {
      vtkErrorMacro("Unknown field " << field);
      }
    }

  // Setup the InputMenu.
  const char* filterArrays = element->GetAttribute("filter_arrays");
  if (filterArrays)
    {
    if (strcmp(filterArrays, "On") == 0 || strcmp(filterArrays, "on") == 0 || 
        strcmp(filterArrays, "True") == 0 || strcmp(filterArrays, "true") == 0 || 
        strcmp(filterArrays, "1") == 0)
      {
      this->FilterArrays = 1;
      }
    else if (strcmp(filterArrays, "Off") == 0 || strcmp(filterArrays, "off") == 0 || 
             strcmp(filterArrays, "False") == 0 || strcmp(filterArrays, "false") == 0 || 
             strcmp(filterArrays, "0") == 0)
      {
      this->FilterArrays = 0;
      }
    else
      {
      vtkErrorMacro("Unknown boolean " << filterArrays);
      }
    }

  // Setup the InputMenu.
  const char* deactivate = element->GetAttribute("deactivate");
  if (deactivate)
    {
    if (strcmp(deactivate, "On") == 0 || strcmp(deactivate, "on") == 0 || 
        strcmp(deactivate, "True") == 0 || strcmp(deactivate, "true") == 0 || 
        strcmp(deactivate, "1") == 0)
      {
      this->Deactivate = 1;
      }
    else if (strcmp(deactivate, "Off") == 0 || strcmp(deactivate, "off") == 0 || 
             strcmp(deactivate, "False") == 0 || strcmp(deactivate, "false") == 0 || 
             strcmp(deactivate, "0") == 0)
      {
      this->Deactivate = 0;
      }
    else
      {
      vtkErrorMacro("Unknown boolean " << deactivate);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ButtonFrame);
  this->PropagateEnableState(this->ShowAllLabel);
  this->PropagateEnableState(this->ShowAllCheck);

  this->PropagateEnableState(this->ArraySelectionList);

  vtkCollectionIterator* sit = this->ArrayLabelCollection->NewIterator();
  for ( sit->InitTraversal(); !sit->IsDoneWithTraversal(); sit->GoToNextItem() )
    {
    this->PropagateEnableState(vtkKWWidget::SafeDownCast(sit->GetCurrentObject()));
    }
  sit->Delete();

  this->PropagateEnableState(this->InputMenu);
}

//----------------------------------------------------------------------------
void vtkPVSelectArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
}
