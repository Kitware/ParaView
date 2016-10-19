/*=========================================================================

  Program:   ParaView
  Module:    vtkSIIndexSelectionProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIIndexSelectionProperty.h"

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

vtkStandardNewMacro(vtkSIIndexSelectionProperty)

  //----------------------------------------------------------------------------
  vtkSIIndexSelectionProperty::vtkSIIndexSelectionProperty()
{
}

//----------------------------------------------------------------------------
vtkSIIndexSelectionProperty::~vtkSIIndexSelectionProperty()
{
}

//----------------------------------------------------------------------------
void vtkSIIndexSelectionProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSIIndexSelectionProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
  {
    return false;
  }
  if (!this->GetCommand())
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
    aname << "GetNumberOf" << this->Command << "s" << ends;

    // Get the number of arrays.
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke << reader << aname.str().c_str()
           << vtkClientServerStream::End;

    this->ProcessMessage(stream);
    stream.Reset();
    int numArrays = 0;
    if (!this->GetLastResult().GetArgument(0, 0, &numArrays))
    {
      vtkErrorMacro("Error getting number of " << this->Command << "s from reader.");
    }

    // For each array, get its name and status.
    for (int i = 0; i < numArrays; ++i)
    {
      std::ostringstream naname;
      naname << "Get" << this->Command << "Name" << ends;

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
        vtkErrorMacro("Error getting " << this->Command << " name from reader.");
        break;
      }
      if (!pname)
      {
        // Initializing a std::string to NULL does not have a defined
        // behavior.
        break;
      }
      std::string name = pname;

      std::ostringstream caname;
      caname << "Get" << this->Command << "CurrentIndex" << ends;
      // Get the array status.
      stream << vtkClientServerStream::Invoke << reader << caname.str().c_str() << name.c_str()
             << vtkClientServerStream::End;
      if (!this->ProcessMessage(stream))
      {
        break;
      }
      stream.Reset();
      int currIdx = 0;
      if (!this->GetLastResult().GetArgument(0, 0, &currIdx))
      {
        vtkErrorMacro("Error getting index from reader.");
        break;
      }

      std::ostringstream saname;
      saname << "Get" << this->Command << "Size" << ends;
      // Get the array status.
      stream << vtkClientServerStream::Invoke << reader << saname.str().c_str() << name.c_str()
             << vtkClientServerStream::End;
      if (!this->ProcessMessage(stream))
      {
        break;
      }
      stream.Reset();
      int dimSize = 0;
      if (!this->GetLastResult().GetArgument(0, 0, &dimSize))
      {
        vtkErrorMacro("Error getting size from reader.");
        break;
      }

      // Store the name/status pair in the result message.
      result->add_txt(name.c_str());
      std::ostringstream str;
      str << currIdx;
      result->add_txt(str.str());

      str.str("");
      str << dimSize;
      result->add_txt(str.str());
    }
  }
  else
  {
    vtkErrorMacro("Called on NULL vtkObject");
  }

  return true;
}
