/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeDataInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataInformation.h"

#include "vtkClientServerStream.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkTimerLog.h"

#include "vtkSmartPointer.h"
#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVCompositeDataInformation);
vtkCxxRevisionMacro(vtkPVCompositeDataInformation, "1.3");

struct vtkPVCompositeDataInformationInternals
{
  typedef 
  vtkstd::vector<vtkSmartPointer<vtkPVDataInformation> > 
  LevelDataInformationType;

  typedef 
  vtkstd::vector<LevelDataInformationType> DataInformationType;

  DataInformationType DataInformation;
};

//----------------------------------------------------------------------------
vtkPVCompositeDataInformation::vtkPVCompositeDataInformation()
{
  this->Internal = new vtkPVCompositeDataInformationInternals;
  this->DataIsComposite = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositeDataInformation::~vtkPVCompositeDataInformation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DataIsComposite: " << this->DataIsComposite << endl;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::Initialize()
{
  this->DataIsComposite = 0;
  this->Internal->DataInformation.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeDataInformation::GetNumberOfLevels()
{
  return this->Internal->DataInformation.size();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeDataInformation::GetNumberOfDataSets(
  unsigned int level)
{
  if (level >= this->GetNumberOfLevels())
    {
    return 0;
    }

  vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
    this->Internal->DataInformation[level];

  return ldata.size();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVCompositeDataInformation::GetDataInformation(
  unsigned int level, unsigned int idx)
{
  if (level >= this->GetNumberOfLevels())
    {
    return 0;
    }

  vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
    this->Internal->DataInformation[level];

  if (idx >= ldata.size())
    {
    return 0;
    }

  return ldata[idx];
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  vtkHierarchicalDataSet* hds = 
    vtkHierarchicalDataSet::SafeDownCast(object);
  if (!hds)
    {
    return;
    }

  vtkTimerLog::MarkStartEvent("Copying information from composite data");

  this->DataIsComposite = 1;

  unsigned int numLevels = hds->GetNumberOfLevels();
  this->Internal->DataInformation.resize(numLevels);
  for (unsigned int i=0; i<numLevels; i++)
    {
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    unsigned int numDataSets = hds->GetNumberOfDataSets(i);
    ldata.resize(numDataSets);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      vtkDataObject* dobj = hds->GetDataSet(i, j);
      if (dobj)
        {
        vtkPVDataInformation* dataInf = vtkPVDataInformation::New();
        dataInf->CopyFromObject(dobj);
        ldata[j] = dataInf;
        dataInf->Delete();
        }
      }
    }

  vtkTimerLog::MarkEndEvent("Copying information from composite data");
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVCompositeDataInformation *info;

  info = vtkPVCompositeDataInformation::SafeDownCast(pvi);
  if (info == NULL)
    {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
    }

  unsigned int otherNumLevels = info->Internal->DataInformation.size();
  unsigned int numLevels = this->Internal->DataInformation.size();
  if ( otherNumLevels > numLevels )
    {
    numLevels = otherNumLevels;
    this->Internal->DataInformation.resize(numLevels);
    }

  for (unsigned int i=0; i < otherNumLevels; i++)
    {
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& 
      otherldata = info->Internal->DataInformation[i];
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    unsigned otherNumDataSets = otherldata.size();
    unsigned numDataSets = ldata.size();
    if (otherNumDataSets > numDataSets)
      {
      numDataSets = otherNumDataSets;
      ldata.resize(numDataSets);
      }
    for (unsigned int j=0; j < otherNumDataSets; j++)
      {
      vtkPVDataInformation* otherInfo = otherldata[j];
      vtkPVDataInformation* localInfo = ldata[j];
      if (otherInfo)
        {
        if (localInfo)
          {
          localInfo->AddInformation(otherInfo);
          }
        else
          {
          vtkPVDataInformation* dinf = vtkPVDataInformation::New();
          dinf->AddInformation(otherInfo);
          ldata[j] = dinf;
          dinf->Delete();
          }
        }
      }
    }
         

}
//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyToStream(
  vtkClientServerStream* css) const
{
  unsigned int i, j;

  vtkTimerLog::MarkStartEvent("Copying composite information to stream");
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->DataIsComposite;
  if (!this->DataIsComposite)
    {
    vtkTimerLog::MarkEndEvent("Copying composite information to stream");
    *css << vtkClientServerStream::End;
    return;
    }
  unsigned int numLevels = this->Internal->DataInformation.size();
  *css << numLevels;
  for(i=0; i<numLevels; i++)
    {
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    *css << ldata.size();
    }

  vtkClientServerStream dcss;
  size_t length;
  const unsigned char* data;

  for(i=0; i<numLevels; i++)
    {
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    unsigned int numDataSets = ldata.size();
    for(j=0; j<numDataSets; j++)
      {
      vtkPVDataInformation* dataInf = ldata[j];
      if (dataInf)
        {
        *css << i;
        *css << j;
        dcss.Reset();
        dataInf->CopyToStream(&dcss);
        dcss.GetData(&data, &length);
        *css << vtkClientServerStream::InsertArray(data, length);
        }
      }
    }
  *css << numLevels;
  *css << vtkClientServerStream::End;
  vtkTimerLog::MarkEndEvent("Copying composite information to stream");
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromStream(
  const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->DataIsComposite))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  if (!this->DataIsComposite)
    {
    return;
    }
  unsigned int numLevels;
  if(!css->GetArgument(0, 1, &numLevels))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  int msgIdx = 1;
  this->Internal->DataInformation.resize(numLevels);
  for (unsigned int i=0; i<numLevels; i++)
    {
    unsigned int numDataSets;
    msgIdx++;
    if(!css->GetArgument(0, msgIdx, &numDataSets))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    ldata.resize(numDataSets);
    }

  while (1)
    {
    msgIdx++;
    unsigned int levelIdx, dataSetIdx;
    if(!css->GetArgument(0, msgIdx, &levelIdx))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }
    if (levelIdx >= numLevels)
      {
      break;
      }
    msgIdx++;
    if(!css->GetArgument(0, msgIdx, &dataSetIdx))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }

    vtkTypeUInt32 length;
    vtkstd::vector<unsigned char> data;
    vtkClientServerStream dcss;
    
    msgIdx++;
    // Data information.
    vtkPVDataInformation* dataInf = vtkPVDataInformation::New();
    if(!css->GetArgumentLength(0, msgIdx, &length))
      {
      vtkErrorMacro("Error parsing length of cell data information.");
      dataInf->Delete();
      return;
      }
    data.resize(length);
    if(!css->GetArgument(0, msgIdx, &*data.begin(), length))
      {
      vtkErrorMacro("Error parsing cell data information.");
      dataInf->Delete();
      return;
      }
    dcss.SetData(&*data.begin(), length);
    dataInf->CopyFromStream(&dcss);
    vtkPVCompositeDataInformationInternals::LevelDataInformationType& ldata = 
      this->Internal->DataInformation[levelIdx];
    ldata[dataSetIdx] = dataInf;
    dataInf->Delete();
    }

}
