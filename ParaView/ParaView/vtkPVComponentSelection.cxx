/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComponentSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComponentSelection.h"

#include "vtkArrayMap.txx"
#include "vtkKWCheckButton.h"
#include "vtkKWWidgetCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVComponentSelection);
vtkCxxRevisionMacro(vtkPVComponentSelection, "1.12");

//---------------------------------------------------------------------------
vtkPVComponentSelection::vtkPVComponentSelection()
{
  this->CheckButtons = vtkKWWidgetCollection::New();
  this->Initialized = 0;
  this->ObjectID.ID = 0;
  this->VariableName = NULL;
  this->NumberOfComponents = 0;
  this->LastAcceptedState = 0;
}

//---------------------------------------------------------------------------
vtkPVComponentSelection::~vtkPVComponentSelection()
{
  this->CheckButtons->Delete();
  this->ObjectID.ID = 0;
  this->SetVariableName(NULL);
  if (this->LastAcceptedState)
    {
    delete [] this->LastAcceptedState;
    this->LastAcceptedState = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::Create(vtkKWApplication *app)
{
  const char* wname;
  int i;
  vtkKWCheckButton *button;
  char compId[10];
  
  if (this->Application)
    {
    vtkErrorMacro("ComponentSelection already created");
    return;
    }
  
  this->SetApplication(app);
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

  this->LastAcceptedState = new unsigned char[this->NumberOfComponents];
  
  for (i = 0; i <= this->NumberOfComponents; i++)
    {
    button = vtkKWCheckButton::New();
    button->SetParent(this);
    button->Create(app, "");
    button->SetCommand(this, "ModifiedCallback");
    button->SetState(1);
    sprintf(compId, "%d", i);
    button->SetText(compId);
    this->CheckButtons->AddItem(button);
    ostrstream str;
    str << "Set" << this->VariableName << ends;
    pvApp->GetProcessModule()->GetStream()
      << vtkClientServerStream::Invoke << this->ObjectID << str.str() 
      << i << i << vtkClientServerStream::End;
    delete [] str.str();
    pvApp->GetProcessModule()->SendStreamToServer();
    this->Script("pack %s", button->GetWidgetName());
    button->Delete();
    this->LastAcceptedState[i] = 1;
    }
}


//---------------------------------------------------------------------------
void vtkPVComponentSelection::Trace(ofstream *file)
{
  int i;
  
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
    {
    *file << "$kw(" << this->GetTclName() << ") SetState "
          << i << " " << this->GetState(i) << endl;
    }
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::AcceptInternal(vtkClientServerID sourceID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int i;
  
  if (this->ModifiedFlag)
    {
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                    << "RemoveAllValues" << vtkClientServerStream::End;
    for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
      {
      if (this->GetState(i))
        {
        ostrstream str;
        str << "Set" << this->VariableName << ends;
        pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                        << str.str() << i << i << vtkClientServerStream::End;
        delete [] str.str();
        }
      else
        {
        ostrstream str;
        str << "Set" << this->VariableName << ends;
        pm->GetStream() << vtkClientServerStream::Invoke << sourceID
                        << str.str() << i << -1 << vtkClientServerStream::End;
        delete [] str.str();
        }
      this->LastAcceptedState[i] = this->GetState(i);
      }
    }
  
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  
  if ( ! this->Initialized)
    {
    this->Initialized = 1;
    return;
    }
  
  int i;

  for (i = 0; i < this->CheckButtons->GetNumberOfItems(); i++)
    {
    this->SetState(i, this->LastAcceptedState[i]);
    }
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVComponentSelection::SetState(int i, int state)
{
  if (i > this->CheckButtons->GetNumberOfItems()-1)
    {
    return;
    }
  
  ((vtkKWCheckButton*)this->CheckButtons->GetItemAsObject(i))->SetState(state);
}

//---------------------------------------------------------------------------
int vtkPVComponentSelection::GetState(int i)
{
  if (i > this->CheckButtons->GetNumberOfItems()-1)
    {
    return -1;
    }
  
  return ((vtkKWCheckButton*)this->CheckButtons->GetItemAsObject(i))->
    GetState();
}

vtkPVComponentSelection* vtkPVComponentSelection::ClonePrototype(
  vtkPVSource* pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVComponentSelection::SafeDownCast(clone);
}

void vtkPVComponentSelection::CopyProperties(vtkPVWidget* clone, 
                                             vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVComponentSelection* pvcs = vtkPVComponentSelection::SafeDownCast(clone);
  if (pvcs)
    {
    pvcs->SetNumberOfComponents(this->NumberOfComponents);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVComponentSelection.");
    }
}

//----------------------------------------------------------------------------
int vtkPVComponentSelection::ReadXMLAttributes(vtkPVXMLElement* element,
                                               vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  if(!element->GetScalarAttribute("number_of_components",
                                  &this->NumberOfComponents))
    {
    this->NumberOfComponents = 1;
    }
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVComponentSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfComponents: " << this->NumberOfComponents << endl;
}
