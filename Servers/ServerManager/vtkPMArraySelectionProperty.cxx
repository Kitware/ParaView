/*=========================================================================

  Program:   ParaView
  Module:    vtkPMArraySelectionProperty

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMArraySelectionProperty.h"
#include "vtkPMProperty.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkArrayIterator.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPMArraySelectionProperty);
//----------------------------------------------------------------------------
vtkPMArraySelectionProperty::vtkPMArraySelectionProperty()
{
}

//----------------------------------------------------------------------------
vtkPMArraySelectionProperty::~vtkPMArraySelectionProperty()
{
}

//----------------------------------------------------------------------------
void vtkPMArraySelectionProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkPMArraySelectionProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
    {
    return false;
    }
  if (!this->GetCommand()) // Hold the arraName
    {
    return false;
    }

  // Create property and add it to the message
  ProxyState_Property *prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *result = prop->mutable_value();
  result->set_type(Variant::STRING);

  // Get the ID for the reader.
  vtkClientServerID readerID = this->GetVTKObjectID();
  if(readerID.ID)
    {
    vtksys_ios::ostringstream aname;
    aname << "GetNumberOf" << this->Command << "Arrays" << ends;

    // Get the number of arrays.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
        << readerID
        << aname.str().c_str()
        << vtkClientServerStream::End;

    this->ProcessMessage(stream);
    stream.Reset();
    int numArrays = 0;
    if(!this->GetLastResult().GetArgument(0, 0, &numArrays))
      {
      vtkErrorMacro("Error getting number of arrays from reader.");
      }

    // For each array, get its name and status.
    for(int i=0; i < numArrays; ++i)
      {
      vtksys_ios::ostringstream naname;
      naname << "Get" << this->Command << "ArrayName" << ends;

      // Get the array name.
      stream << vtkClientServerStream::Invoke
             << readerID
             << naname.str().c_str()
             << i
             << vtkClientServerStream::End;
      if(!this->ProcessMessage(stream))
        {
        break;
        }
      stream.Reset();
      const char* pname;
      if(!this->GetLastResult().GetArgument(0, 0, &pname))
        {
        vtkErrorMacro("Error getting array name from reader.");
        break;
        }
      if (!pname)
        {
        // Initializing a vtkstd::string to NULL does not have a defined
        // behavior.
        break;
        }
      vtkstd::string name = pname;

      vtksys_ios::ostringstream saname;
      saname << "Get" << this->Command << "ArrayStatus" << ends;
      // Get the array status.
      stream << vtkClientServerStream::Invoke
             << readerID
             << saname.str().c_str()
             << name.c_str()
             << vtkClientServerStream::End;
      if(!this->ProcessMessage(stream))
        {
        break;
        }
      stream.Reset();
      int status = 0;
      if(!this->GetLastResult().GetArgument(0, 0, &status))
        {
        vtkErrorMacro("Error getting array status from reader.");
        break;
        }

      // Store the name/status pair in the result message.
      result->add_txt(name.c_str());
      result->add_txt( (status == 0) ? "0" : "1" );
      }
    }
  else
    {
    vtkErrorMacro("GetArraySettings cannot get an ID from "
                  << this->GetVTKObject()->GetClassName()
                  << "(" << this->GetVTKObject() << ").");
    }
  return true;
}
