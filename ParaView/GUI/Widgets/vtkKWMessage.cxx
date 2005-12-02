/*=========================================================================

  Module:    vtkKWMessage.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMessage.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMessage );
vtkCxxRevisionMacro(vtkKWMessage, "1.2");

//----------------------------------------------------------------------------
vtkKWMessage::vtkKWMessage()
{
  this->Text = NULL;
}

//----------------------------------------------------------------------------
vtkKWMessage::~vtkKWMessage()
{
  if (this->Text) 
    { 
    delete [] this->Text; 
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetText(const char* _arg)
{
  if (this->Text == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Text && _arg && (!strcmp(this->Text, _arg))) 
    {
    return;
    }

  if (this->Text) 
    { 
    delete [] this->Text; 
    }

  if (_arg)
    {
    this->Text = new char[strlen(_arg) + 1];
    strcpy(this->Text, _arg);
    }
  else
    {
    this->Text = NULL;
    }

  this->Modified();

  this->UpdateText();
} 

//----------------------------------------------------------------------------
void vtkKWMessage::UpdateText()
{
  if (this->IsCreated())
    {
    // NULL is handled correctly as ""
    this->SetTextOption("-text", this->Text); 
    }
}

//----------------------------------------------------------------------------
void vtkKWMessage::Create()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget("message"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetJustificationToLeft();

  this->UpdateText();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetAspectRatio(int aspect)
{
  this->SetConfigurationOptionAsInt("-aspect", aspect);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetAspectRatio()
{
  return this->GetConfigurationOptionAsInt("-aspect");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetJustification(int justification)
{
  this->SetConfigurationOption(
    "-justify", 
    vtkKWTkOptions::GetJustificationAsTkOptionValue(justification));
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetJustification()
{
  return vtkKWTkOptions::GetJustificationFromTkOptionValue(
    this->GetConfigurationOption("-justify"));
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWTkOptions::GetAnchorAsTkOptionValue(anchor));
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetAnchor()
{
  return vtkKWTkOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWMessage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Text: ";
  if (this->Text)
    {
    os << this->Text << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}

