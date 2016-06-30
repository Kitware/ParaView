/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCinemaDatabaseInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCinemaDatabaseInformation.h"

#include "vtkCinemaDatabase.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include <cassert>

vtkStandardNewMacro(vtkPVCinemaDatabaseInformation);
//----------------------------------------------------------------------------
vtkPVCinemaDatabaseInformation::vtkPVCinemaDatabaseInformation()
{
  this->SetRootOnly(1);
}

//----------------------------------------------------------------------------
vtkPVCinemaDatabaseInformation::~vtkPVCinemaDatabaseInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::Reset()
{
  this->PipelineObjects.clear();
  this->ControlParameters.clear();
  this->ControlParameterValues.clear();
  this->PipelineObjectParents.clear();
  this->PipelineObjectVisibilities.clear();
}

//----------------------------------------------------------------------------
const vtkPVCinemaDatabaseInformation::VectorOfStrings&
vtkPVCinemaDatabaseInformation::GetControlParameters(const std::string& parameter) const
{
  static VectorOfStrings empty;
  ControlParametersType::const_iterator iter = this->ControlParameters.find(parameter);
  return (iter != this->ControlParameters.end()) ? iter->second : empty;
}

//----------------------------------------------------------------------------
const vtkPVCinemaDatabaseInformation::VectorOfStrings&
vtkPVCinemaDatabaseInformation::GetControlParameterValues(const std::string& parameter) const
{
  static VectorOfStrings empty;
  ControlParametersType::const_iterator iter = this->ControlParameterValues.find(parameter);
  return (iter != this->ControlParameterValues.end()) ? iter->second : empty;
}

//----------------------------------------------------------------------------
const vtkPVCinemaDatabaseInformation::VectorOfStrings&
vtkPVCinemaDatabaseInformation::GetPipelineObjectParents(const std::string& object) const
{
  static VectorOfStrings empty;
  PipelineObjectParentsType::const_iterator iter = this->PipelineObjectParents.find(object);
  return (iter != this->PipelineObjectParents.end()) ? iter->second : empty;
}

//----------------------------------------------------------------------------
bool vtkPVCinemaDatabaseInformation::GetPipelineObjectVisibility(const std::string& object) const
{
  PipelineObjectVisibilitiesType::const_iterator iter =
    this->PipelineObjectVisibilities.find(object);
  return (iter != this->PipelineObjectVisibilities.end()) ? iter->second : false;
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::CopyFromObject(vtkObject* obj)
{
  this->Reset();

  if (vtkCinemaDatabase* cdbase = vtkCinemaDatabase::SafeDownCast(obj))
  {
    this->PipelineObjects = cdbase->GetPipelineObjects();
    for (PipelineObjectsType::const_iterator iter = this->PipelineObjects.begin();
         iter != this->PipelineObjects.end(); ++iter)
    {
      VectorOfStrings& cparams = this->ControlParameters[*iter];
      cparams = cdbase->GetControlParameters(*iter);
      for (size_t cc = 0; cc < cparams.size(); ++cc)
      {
        if (this->ControlParameterValues.find(cparams[cc]) == this->ControlParameterValues.end())
        {
          this->ControlParameterValues[cparams[cc]] =
            cdbase->GetControlParameterValues(cparams[cc]);
        }
      }

      this->PipelineObjectParents[*iter] = cdbase->GetPipelineObjectParents(*iter);
      this->PipelineObjectVisibilities[*iter] = cdbase->GetPipelineObjectVisibility(*iter);
    }
    assert(this->ControlParameters.size() == this->PipelineObjects.size());
  }
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::AddInformation(vtkPVInformation* other)
{
  if (vtkPVCinemaDatabaseInformation* cother = vtkPVCinemaDatabaseInformation::SafeDownCast(other))
  {
    this->PipelineObjects = cother->PipelineObjects;
    this->ControlParameters = cother->ControlParameters;
    this->PipelineObjectParents = cother->PipelineObjectParents;
    this->PipelineObjectVisibilities = cother->PipelineObjectVisibilities;
  }
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::CopyToStream(vtkClientServerStream* css)
{
  *css << vtkClientServerStream::Reply << static_cast<int>(this->PipelineObjects.size());

  for (PipelineObjectsType::const_iterator piter = this->PipelineObjects.begin();
       piter != this->PipelineObjects.end(); ++piter)
  {
    *css << piter->c_str();
  }

  *css << static_cast<int>(this->ControlParameters.size());
  for (ControlParametersType::const_iterator citer = this->ControlParameters.begin();
       citer != this->ControlParameters.end(); ++citer)
  {
    *css << citer->first.c_str();
    *css << static_cast<int>(citer->second.size());
    for (VectorOfStrings::const_iterator viter = citer->second.begin();
         viter != citer->second.end(); ++viter)
    {
      *css << viter->c_str();
    }
  }

  *css << static_cast<int>(this->ControlParameterValues.size());
  for (ControlParametersType::const_iterator citer = this->ControlParameterValues.begin();
       citer != this->ControlParameterValues.end(); ++citer)
  {
    *css << citer->first.c_str();
    *css << static_cast<int>(citer->second.size());
    for (VectorOfStrings::const_iterator viter = citer->second.begin();
         viter != citer->second.end(); ++viter)
    {
      *css << viter->c_str();
    }
  }

  *css << static_cast<int>(this->PipelineObjectParents.size());
  for (PipelineObjectParentsType::const_iterator citer = this->PipelineObjectParents.begin();
       citer != this->PipelineObjectParents.end(); ++citer)
  {
    *css << citer->first.c_str();
    *css << static_cast<int>(citer->second.size());
    for (VectorOfStrings::const_iterator viter = citer->second.begin();
         viter != citer->second.end(); ++viter)
    {
      *css << viter->c_str();
    }
  }

  *css << static_cast<int>(this->PipelineObjectVisibilities.size());
  for (PipelineObjectVisibilitiesType::const_iterator citer =
         this->PipelineObjectVisibilities.begin();
       citer != this->PipelineObjectVisibilities.end(); ++citer)
  {
    *css << citer->first.c_str();
    *css << (citer->second ? 1 : 0);
  }
  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Reset();

#define PARSE_NEXT_VALUE(_ivarName)                                                                \
  if (!css->GetArgument(0, i++, &_ivarName))                                                       \
  {                                                                                                \
    vtkErrorMacro("Error parsing " #_ivarName " from message.");                                   \
    this->Reset();                                                                                 \
    return;                                                                                        \
  }

  int i = 0;
  int count = 0;
  PARSE_NEXT_VALUE(count);

  this->PipelineObjects.resize(count);
  for (int cc = 0; cc < count; ++cc)
  {
    PARSE_NEXT_VALUE(this->PipelineObjects[cc]);
  }

  PARSE_NEXT_VALUE(count);
  for (int cc = 0; cc < count; ++cc)
  {
    std::string key;
    PARSE_NEXT_VALUE(key);

    int vcount;
    PARSE_NEXT_VALUE(vcount);

    VectorOfStrings& item = this->ControlParameters[key];
    item.resize(vcount);
    for (int kk = 0; kk < vcount; ++kk)
    {
      PARSE_NEXT_VALUE(item[kk]);
    }
  }

  PARSE_NEXT_VALUE(count);
  for (int cc = 0; cc < count; ++cc)
  {
    std::string key;
    PARSE_NEXT_VALUE(key);

    int vcount;
    PARSE_NEXT_VALUE(vcount);

    VectorOfStrings& item = this->ControlParameterValues[key];
    item.resize(vcount);
    for (int kk = 0; kk < vcount; ++kk)
    {
      PARSE_NEXT_VALUE(item[kk]);
    }
  }

  PARSE_NEXT_VALUE(count);
  for (int cc = 0; cc < count; ++cc)
  {
    std::string key;
    PARSE_NEXT_VALUE(key);

    int vcount;
    PARSE_NEXT_VALUE(vcount);

    VectorOfStrings& item = this->PipelineObjectParents[key];
    item.resize(vcount);
    for (int kk = 0; kk < vcount; ++kk)
    {
      PARSE_NEXT_VALUE(item[kk]);
    }
  }

  PARSE_NEXT_VALUE(count);
  for (int cc = 0; cc < count; ++cc)
  {
    std::string key;
    PARSE_NEXT_VALUE(key);

    int val;
    PARSE_NEXT_VALUE(val);
    this->PipelineObjectVisibilities[key] = (val != 0);
  }

#undef PARSE_NEXT_VALUE
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVCinemaDatabaseInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
