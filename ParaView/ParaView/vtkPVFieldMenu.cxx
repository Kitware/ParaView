/*=========================================================================

  Program:   
  Module:    vtkPVFieldMenu.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVFieldMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkKWLabel.h"
#include "vtkPVSource.h"
#include "vtkSource.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFieldMenu);
vtkCxxRevisionMacro(vtkPVFieldMenu, "1.1");


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
  
  this->SuppressReset = 1;
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
void vtkPVFieldMenu::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());


  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Label->SetLabel("Attribute Mode:");
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

  if (field == vtkDataSet::POINT_DATA_FIELD)
    {
    this->FieldMenu->SetValue("Point Data");
    }
  else if (field == vtkDataSet::CELL_DATA_FIELD)
    {
    this->FieldMenu->SetValue("Cell Data");
    }
  
  this->Value = field;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVFieldMenu::GetFieldInformation()
{
  vtkPVData *pvd;

  if (this->InputMenu == NULL)
    {
    return NULL;
    }
  pvd = this->InputMenu->GetPVData();
  if (pvd == NULL)
    {
    return NULL;
    }

  switch (this->Value)
    {
    case vtkDataSet::DATA_OBJECT_FIELD:
      vtkErrorMacro("We do not handle data object fields yet.");
      return NULL;
    case vtkDataSet::POINT_DATA_FIELD:
      return pvd->GetDataInformation()->GetPointDataInformation();
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      return pvd->GetDataInformation()->GetCellDataInformation();
      break;
    }

  vtkErrorMacro("Unknown field.");
  return NULL; 
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::AcceptInternal(const char* sourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->Value == vtkDataSet::POINT_DATA_FIELD ||
      this->Value == vtkDataSet::CELL_DATA_FIELD)
    {
    pvApp->BroadcastScript("%s SetAttributeMode %d", 
                           sourceTclName, this->Value);
    }

  this->ModifiedFlag = 0;
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
void vtkPVFieldMenu::ResetInternal(const char* sourceTclName)
{
  // Get the selected array form the VTK filter.
  this->Script("%s SetValue [%s GetAttributeMode]",
               this->GetTclName(), 
               sourceTclName);

  this->ModifiedFlag = 0;
  // Do we really need to update?
  // What causes dependent widgets like ArrayMenu to update?
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SaveInBatchScriptForPart(ofstream *file,
                                              const char* sourceTclName)
{
  if (sourceTclName == NULL)
    {
    vtkErrorMacro("Sanity check failed. ");
    return;
    }

  *file << "\t";
  *file << sourceTclName << " SetAttributeMode " << this->Value << endl;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Update()
{
  int cellFlag, pointFlag;

  // This updates any array menu dependent on this widget.
  this->vtkPVWidget::Update();

  this->FieldMenu->ClearEntries();
  if (this->InputMenu == NULL)
    {
    // Add both.
    this->FieldMenu->AddEntryWithCommand("Point Data", this,
                                         "SetValue 0");
    this->FieldMenu->AddEntryWithCommand("Cell Data", this,
                                                 "SetValue 1");
    this->FieldMenu->SetCurrentEntry("Point Data");
    return;
    }  

  vtkPVSource* pvs = this->InputMenu->GetCurrentValue();
  if (pvs == NULL)
    {
    return;
    }
  vtkPVDataInformation* dataInfo = pvs->GetPVOutput()->GetDataInformation();
  if (dataInfo == NULL)
    {
    return;
    }
  
  pointFlag = cellFlag = 0;
  if (this->CheckField(dataInfo->GetPointDataInformation()))
    {
    this->FieldMenu->AddEntryWithCommand("Point Data", this,
                                         "SetValue 0");
    pointFlag = 1;
    }

  if (this->CheckField(dataInfo->GetCellDataInformation()))
    {
    this->FieldMenu->AddEntryWithCommand("Cell Data", this,
                                         "SetValue 1");
    cellFlag = 1;
    }
  if (pointFlag)
    {
    this->FieldMenu->SetCurrentEntry("Point Data");
    }
  else if (cellFlag)
    {
    this->FieldMenu->SetCurrentEntry("Cell Data");
    }
}


//----------------------------------------------------------------------------
int vtkPVFieldMenu::CheckField(vtkPVDataSetAttributesInformation* info)
{
  return 1;
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

