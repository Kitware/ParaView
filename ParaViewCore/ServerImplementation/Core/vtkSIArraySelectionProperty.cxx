/*=========================================================================

  Program:   ParaView
  Module:    vtkSIArraySelectionProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIArraySelectionProperty.h"

#include "vtkArrayIterator.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSIProperty.h"
#include "vtkSMMessage.h"
#include "vtkStringArray.h"

#include <sstream>
#include <string>

vtkStandardNewMacro(vtkSIArraySelectionProperty);
//----------------------------------------------------------------------------
vtkSIArraySelectionProperty::vtkSIArraySelectionProperty()
{
}

//----------------------------------------------------------------------------
vtkSIArraySelectionProperty::~vtkSIArraySelectionProperty()
{
}

//----------------------------------------------------------------------------
void vtkSIArraySelectionProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSIArraySelectionProperty::Pull(vtkSMMessage* msgToFill)
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
  ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* result = prop->mutable_value();
  result->set_type(Variant::STRING);

  // Get the ID for the reader.
  vtkObjectBase* reader = this->GetVTKObject();
  if (reader != NULL)
  {
    std::ostringstream aname;
    aname << "GetNumberOf" << this->Command << "Arrays" << ends;

    // Get the number of arrays.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << reader << aname.str().c_str()
           << vtkClientServerStream::End;

    this->ProcessMessage(stream);
    stream.Reset();
    int numArrays = 0;
    if (!this->GetLastResult().GetArgument(0, 0, &numArrays))
    {
      vtkErrorMacro("Error getting number of arrays from reader.");
    }

    // For each array, get its name and status.
    for (int i = 0; i < numArrays; ++i)
    {
      std::ostringstream naname;
      naname << "Get" << this->Command << "ArrayName" << ends;

      // Get the array name.
      stream << vtkClientServerStream::Invoke << reader << naname.str().c_str() << i
             << vtkClientServerStream::End;
      if (!this->ProcessMessage(stream))
      {
        break;
      }
      stream.Reset();
      const char* pname;
      if (!this->GetLastResult().GetArgument(0, 0, &pname))
      {
        vtkErrorMacro("Error getting array name from reader.");
        break;
      }
      if (!pname)
      {
        // Initializing a std::string to NULL does not have a defined
        // behavior.
        break;
      }
      std::string name = pname;

      std::ostringstream saname;
      saname << "Get" << this->Command << "ArrayStatus" << ends;
      // Get the array status.
      stream << vtkClientServerStream::Invoke << reader << saname.str().c_str() << name.c_str()
             << vtkClientServerStream::End;
      if (!this->ProcessMessage(stream))
      {
        break;
      }
      stream.Reset();
      int status = 0;
      if (!this->GetLastResult().GetArgument(0, 0, &status))
      {
        vtkErrorMacro("Error getting array status from reader.");
        break;
      }

      // Store the name/status pair in the result message.
      result->add_txt(name.c_str());
      result->add_txt((status == 0) ? "0" : "1");
    }
  }
  else
  {
    vtkErrorMacro("GetArraySettings called on NULL vtkObject");
  }
  return true;
}
