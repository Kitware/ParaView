// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
// Unfortunately we have two separate naming conventions for
// which field data array the channel name is coming from.
// Below are the two possibilities that we need to deal with.
const std::string InputArrayNameLegacyAPI = "__CatalystChannel__";
const std::string InputArrayNameV2API = "channel";
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
            dobj->GetFieldData()->GetAbstractArray(InputArrayNameLegacyAPI.c_str())))
      {
        if (array->GetNumberOfTuples() > 0)
        {
          this->ChannelName = array->GetValue(0);
          return;
        }
      }
      if (vtkStringArray* array = vtkStringArray::SafeDownCast(
            dobj->GetFieldData()->GetAbstractArray(InputArrayNameV2API.c_str())))
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
