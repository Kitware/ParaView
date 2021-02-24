/*=========================================================================

 Program:   ParaView
 Module:    vtkPVCatalystChannelInformation.cxx

 Copyright (c) Kitware, Inc.
 All rights reserved.
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPVCatalystChannelInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDataObject.h"
#include "vtkFieldData.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

vtkStandardNewMacro(vtkPVCatalystChannelInformation);

namespace
{
// this should probably be coming from vtkCPProcessor::GetInputArrayName()
// but I don't want to introduce a dependency on that module
const std::string InputArrayName = "__CatalystChannel__";
}

//----------------------------------------------------------------------------
vtkPVCatalystChannelInformation::vtkPVCatalystChannelInformation() = default;

//----------------------------------------------------------------------------
vtkPVCatalystChannelInformation::~vtkPVCatalystChannelInformation() = default;

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::Initialize()
{
  this->ChannelName.clear();
}

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ChannelName: " << this->ChannelName << endl;
}

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::CopyFromObject(vtkObject* obj)
{
  if (vtkAlgorithm* algo = vtkAlgorithm::SafeDownCast(obj))
  {
    // Locate named array in dataset
    if (vtkDataObject* dobj = algo->GetOutputDataObject(0))
    {
      if (vtkStringArray* array = vtkStringArray::SafeDownCast(
            dobj->GetFieldData()->GetAbstractArray(InputArrayName.c_str())))
      {
        if (array->GetNumberOfTuples() > 0)
        {
          this->ChannelName = array->GetValue(0);
          return;
        }
      }
    }
  }
  this->ChannelName.clear();
  return;
}

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
  {
    return;
  }

  vtkPVCatalystChannelInformation* aInfo = vtkPVCatalystChannelInformation::SafeDownCast(info);
  if (!aInfo)
  {
    vtkErrorMacro("Could not downcast info to catalyst channel info.");
    return;
  }
  this->ChannelName = aInfo->GetChannelName();
}

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->ChannelName;
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVCatalystChannelInformation::CopyFromStream(const vtkClientServerStream* css)
{
  if (!css->GetArgument(0, 0, &this->ChannelName))
  {
    vtkErrorMacro("Error parsing dataset channel name from message.");
  }
}
