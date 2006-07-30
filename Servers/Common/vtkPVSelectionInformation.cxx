/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSelectionInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"

vtkStandardNewMacro(vtkPVSelectionInformation);
vtkCxxRevisionMacro(vtkPVSelectionInformation, "1.3");

//----------------------------------------------------------------------------
vtkPVSelectionInformation::vtkPVSelectionInformation()
{
  this->Selection = vtkSelection::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionInformation::~vtkPVSelectionInformation()
{
  if (this->Selection)
    {
    this->Selection->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Selection: ";
  this->Selection->PrintSelf(os, indent.GetNextIndent());
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::Initialize()
{
  this->Selection->Clear();
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyFromObject(vtkObject* obj)
{
  this->Initialize();
  
  vtkSelection* sel = vtkSelection::SafeDownCast(obj);
  if (sel)
    {
    this->Selection->DeepCopy(sel);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
    {
    return;
    }

  vtkPVSelectionInformation* sInfo = 
    vtkPVSelectionInformation::SafeDownCast(info);
  if (!sInfo)
    {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
    }

  this->Selection->CopyChildren(sInfo->Selection);
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  ostrstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 0, this->Selection);
  res << ends;
  *css << res.str();
  delete[] res.str();

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVSelectionInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Initialize();

  const char* xml = 0;
  if(!css->GetArgument(0, 0, &xml))
    {
    vtkErrorMacro("Error parsing selection xml from message.");
    return;
    }
  this->Selection = vtkSelectionSerializer::Parse(xml);
}

