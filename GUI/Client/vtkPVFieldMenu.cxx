/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFieldMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFieldMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVInputMenu.h"
#include "vtkPVInputProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFieldMenu);
vtkCxxRevisionMacro(vtkPVFieldMenu, "1.22");


vtkCxxSetObjectMacro(vtkPVFieldMenu, InputMenu, vtkPVInputMenu);


int vtkPVFieldMenuCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVFieldMenu::vtkPVFieldMenu()
{
  this->CommandFunction = vtkPVFieldMenuCommand;
  
  this->InputMenu = NULL;
  this->Label = vtkKWLabel::New();
  this->FieldMenu = vtkKWOptionMenu::New();
  this->Value = vtkDataSet::POINT_DATA_FIELD;  
  
}

//----------------------------------------------------------------------------
vtkPVFieldMenu::~vtkPVFieldMenu()
{
  this->Label->Delete();
  this->Label = NULL;
  this->FieldMenu->Delete();
  this->FieldMenu = NULL;

  this->SetInputMenu(NULL);
  
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  if (this->Value == vtkDataSet::POINT_DATA_FIELD)
    {
    os << indent << "Value: Point Data. \n";
    }
  if (this->Value == vtkDataSet::CELL_DATA_FIELD)
    {
    os << indent << "Value: Cell Data. \n";
    }
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVFieldMenu::GetInputProperty()
{
  if (this->PVSource == NULL)
    {
    return NULL;
    }

  // Should we get the input name from the input menu?
  return this->PVSource->GetInputProperty("Input");
}


//----------------------------------------------------------------------------
void vtkPVFieldMenu::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Label->SetLabel("Attribute Mode");
  this->Label->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->FieldMenu->SetParent(this);
  this->FieldMenu->Create(app, "");
  this->FieldMenu->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->FieldMenu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SetValue(int field)
{
  if (field == this->Value)
    {
    return;
    }

  vtkSMProperty* prop = this->GetSMProperty();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        if (field == edom->GetEntryValue(i))
          {
          this->FieldMenu->SetValue(edom->GetEntryText(i));
          }
        }
      }
    else
      {
      vtkErrorMacro("Required domain (field_list) could not be found");
      }
    }
  
  this->Value = field;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Accept()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    ivp->SetNumberOfElements(1);
    ivp->SetElement(0, this->Value);
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceName());
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVFieldMenu::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Value << endl;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Initialize()
{
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::ResetInternal()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    this->SetValue(ivp->GetElement(0));
    }

  this->ModifiedFlag = 0;

  // Do we really need to update?
  // What causes dependent widgets like ArrayMenu to update?
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  if (sourceID.ID == 0)
    {
    vtkErrorMacro("Sanity check failed. ");
    return;
    }

  *file << "  [$pvTemp" << sourceID 
        << " GetProperty AttributeMode] SetElements1 " << this->Value << endl;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::UpdateProperty()
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    ivp->SetUncheckedElement(0, this->Value);
    ivp->UpdateDependentDomains();
    }
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Update()
{
  vtkSMProperty* prop = this->GetSMProperty();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      int valSet = 0;
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        if (this->Value == edom->GetEntryValue(i))
          {
          valSet = 1;
          }
        }
      if (!valSet)
        {
        if (numEntries > 0)
          {
          this->Value = edom->GetEntryValue(0);
          }
        }
      }
    else
      {
      vtkErrorMacro("Required property (field_list) could not be found.");
      }
    }

  this->UpdateProperty();

  this->FieldMenu->DeleteAllEntries();
  if (prop)
    {
    vtkSMEnumerationDomain* edom = vtkSMEnumerationDomain::SafeDownCast(
      prop->GetDomain("field_list"));
    if (edom)
      {
      const char* valid = 0;
      unsigned int numEntries = edom->GetNumberOfEntries();
      for (unsigned int i=0; i<numEntries; i++)
        {
        ostrstream com;
        com << "SetValue " << edom->GetEntryValue(i) << ends;
        this->FieldMenu->AddEntryWithCommand(
          edom->GetEntryText(i), this, com.str());
        delete[] com.str();
        if (this->Value == edom->GetEntryValue(i))
          {
          valid = edom->GetEntryText(i);
          }
        }
      if (valid)
        {
        this->FieldMenu->SetCurrentEntry(valid);
        }
      }
    else
      {
      vtkErrorMacro("Required domain (field_list) could not be found.");
      }
    }

  // This updates any array menu dependent on this widget.
  this->Superclass::Update();
}


//----------------------------------------------------------------------------
vtkPVFieldMenu* vtkPVFieldMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVFieldMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
// It looks like I could leave this for the superclass.!!!!!!!!!!!!!!!
vtkPVWidget* vtkPVFieldMenu::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVFieldMenu* pvfm = vtkPVFieldMenu::SafeDownCast(pvWidget);
    if (!pvfm)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  
  vtkPVFieldMenu* pvamm = vtkPVFieldMenu::SafeDownCast(clone);
  if (pvamm)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvamm->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to vtkPVAttributeMenu.");
    }

}

//----------------------------------------------------------------------------
int vtkPVFieldMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
    
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
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
    
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->FieldMenu);
  this->PropagateEnableState(this->InputMenu);
}
