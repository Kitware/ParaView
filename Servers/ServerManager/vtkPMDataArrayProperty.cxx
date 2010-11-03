/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMDataArrayProperty.h"
#include "vtkPMProperty.h"
#include "vtkObjectFactory.h"
#include "vtkDataArray.h"
#include "vtkArrayIterator.h"
#include "vtkStringArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"

//*****************************************************************************
template<class iterT>
void writeValues(iterT* iter, Variant *var)
{


  vtkIdType numValues = iter->GetNumberOfValues();
  for (vtkIdType cc=0; cc < numValues; cc++)
    {
    var->add_float64(iter->GetValue(cc));
    }
}



//*****************************************************************************
vtkStandardNewMacro(vtkPMDataArrayProperty);
//----------------------------------------------------------------------------
vtkPMDataArrayProperty::vtkPMDataArrayProperty()
{
}

//----------------------------------------------------------------------------
vtkPMDataArrayProperty::~vtkPMDataArrayProperty()
{
}

//----------------------------------------------------------------------------
void vtkPMDataArrayProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
bool vtkPMDataArrayProperty::Pull(vtkSMMessage* msgToFill)
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
  str << vtkClientServerStream::Invoke
    << this->GetVTKObjectID() << this->GetCommand()
    << vtkClientServerStream::End;

  this->ProcessMessage(str);

  // Get the result
  vtkDataArray* dataArray = NULL;
  if (!this->GetLastResult().GetArgument(0, 0, (vtkObjectBase**)&dataArray))
    {
    vtkErrorMacro( "Error getting return value of command: "
                   << this->GetCommand());
    return false;
    }

  // Create property and add it to the message
  ProxyState_Property *prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());

  // If no values, then just return OK with an empty content
  if(!dataArray)
    {
    return true;
    }

  // Need to fill the property content
  Variant *var = prop->mutable_value();
  vtkIdType numValues = dataArray->GetNumberOfComponents()
                        * dataArray->GetNumberOfTuples();
  vtkDoubleArray *dataDouble = NULL;
  vtkIntArray *dataInt = NULL;
  vtkStringArray *dataString = NULL;

  switch (dataArray->GetDataType())
    {
    case VTK_STRING:
      var->set_type(Variant::STRING);
      dataString = vtkStringArray::SafeDownCast(dataArray);
      for (vtkIdType cc=0; cc < numValues; cc++)
        {
        var->add_txt(dataString->GetValue(cc));
        }
      break;
    case VTK_DOUBLE:
      var->set_type(Variant::FLOAT64);
      dataDouble = vtkDoubleArray::SafeDownCast(dataArray);
      for (vtkIdType cc=0; cc < numValues; cc++)
        {
        var->add_float64(dataDouble->GetValue(cc));
        }
      break;
    case VTK_INT:
      var->set_type(Variant::INT);
      dataInt = vtkIntArray::SafeDownCast(dataArray);
      for (vtkIdType cc=0; cc < numValues; cc++)
        {
        var->add_integer(dataInt->GetValue(cc));
        }
      break;
//    case VTK_ID_TYPE:
//      var->set_type(Variant::IDTYPE);
//      vtkIdType *dataIdType = vtkIdType::SafeDownCast(dataArray);
//      for (vtkIdType cc=0; cc < numValues; cc++)
//        {
//        var->add_idtype(dataIdType->GetValue(cc));
//        }
//      break;
    default:
      vtkWarningMacro("The Pull method of vtkPMDataArrayProperty do not support "
                      << dataArray->GetDataTypeAsString() << " array type.");
      return false;
    }

  return true;
}
