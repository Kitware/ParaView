/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDataArrayProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIDataArrayProperty.h"

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

vtkStandardNewMacro(vtkSIDataArrayProperty);
//----------------------------------------------------------------------------
vtkSIDataArrayProperty::vtkSIDataArrayProperty() = default;

//----------------------------------------------------------------------------
vtkSIDataArrayProperty::~vtkSIDataArrayProperty() = default;

//----------------------------------------------------------------------------
void vtkSIDataArrayProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkSIDataArrayProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
  {
    return false;
  }

  if (!this->GetCommand())
  {
    return false;
  }

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke << this->GetVTKObject() << this->GetCommand()
      << vtkClientServerStream::End;

  this->ProcessMessage(str);

  // Get the result
  vtkAbstractArray* abstractArray = nullptr;
  if (!this->GetLastResult().GetArgument(0, 0, (vtkObjectBase**)&abstractArray))
  {
    vtkErrorMacro("Error getting return value of command: " << this->GetCommand());
    return false;
  }
  vtkStringArray* stringArray = vtkStringArray::SafeDownCast(abstractArray);
  vtkDataArray* dataArray = vtkDataArray::SafeDownCast(abstractArray);

  // Create property and add it to the message
  ProxyState_Property* prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* var = prop->mutable_value();

  // If no values, then just return OK with an empty content
  if (!dataArray && !stringArray)
  {
    return true;
  }

  // Need to fill the property content with the proper type
  // Right now only those types are supported
  // - vtkDoubleArray
  // - vtkIntArray
  // - vtkIdTypeArray
  // - vtkStringArray
  vtkIdType numValues = abstractArray->GetNumberOfComponents() * abstractArray->GetNumberOfTuples();
  if (dataArray)
  {
    vtkDoubleArray* dataDouble = nullptr;
    vtkIntArray* dataInt = nullptr;
    vtkIdTypeArray* dataIdType = nullptr;
    switch (dataArray->GetDataType())
    {
      case VTK_DOUBLE:
        var->set_type(Variant::FLOAT64);
        dataDouble = vtkDoubleArray::SafeDownCast(dataArray);
        for (vtkIdType cc = 0; cc < numValues; cc++)
        {
          var->add_float64(dataDouble->GetValue(cc));
        }
        break;
      case VTK_INT:
        var->set_type(Variant::INT);
        dataInt = vtkIntArray::SafeDownCast(dataArray);
        for (vtkIdType cc = 0; cc < numValues; cc++)
        {
          var->add_integer(dataInt->GetValue(cc));
        }
        break;
      case VTK_ID_TYPE:
        var->set_type(Variant::IDTYPE);
        dataIdType = vtkIdTypeArray::SafeDownCast(dataArray);
        for (vtkIdType cc = 0; cc < numValues; cc++)
        {
          var->add_idtype(dataIdType->GetValue(cc));
        }
        break;
      default:
        vtkWarningMacro("The Pull method of vtkSIDataArrayProperty do not support "
          << dataArray->GetDataTypeAsString() << " array type.");
        return false;
    }
  }
  else if (stringArray)
  {
    var->set_type(Variant::STRING);
    for (vtkIdType cc = 0; cc < numValues; cc++)
    {
      var->add_txt(stringArray->GetValue(cc));
    }
  }
  return true;
}
