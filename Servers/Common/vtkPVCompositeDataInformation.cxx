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
#include "vtkCompositeDataIterator.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVCompositeDataInformation);
vtkCxxRevisionMacro(vtkPVCompositeDataInformation, "1.11");

struct vtkPVCompositeDataInformationInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkPVDataInformation> > 
    VectorOfDataInformation;

  VectorOfDataInformation ChildrenInformation;
};

//----------------------------------------------------------------------------
vtkPVCompositeDataInformation::vtkPVCompositeDataInformation()
{
  this->Internal = new vtkPVCompositeDataInformationInternals;
  this->DataIsComposite = 0;
  this->DataIsMultiPiece = 0;
  this->NumberOfPieces = 0;
  this->FlatIndexMax = 0;

  // DON'T FORGET TO UPDATE Initialize().
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
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVCompositeDataInformation::GetDataInformationForCompositeIndex(
  int *index)
{
  if (!this->DataIsComposite)
    {
    return 0;
    }

  if (this->DataIsMultiPiece)
    {
    if (*index < this->NumberOfPieces)
      {
      *index=-1;
      return NULL;
      }
    (*index -= this->NumberOfPieces);
    }

  vtkPVCompositeDataInformationInternals::VectorOfDataInformation::iterator iter = 
    this->Internal->ChildrenInformation.begin();
  for ( ; iter!= this->Internal->ChildrenInformation.end(); ++iter)
    {
    if (iter->GetPointer())
      {
      vtkPVDataInformation* info = 
        iter->GetPointer()->GetDataInformationForCompositeIndex(index);
      if (*index==-1)
        {
        return info;
        }
      }
    else
      {
      index--;
      if (index<0)
        {
        return NULL;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::Initialize()
{
  this->DataIsMultiPiece = 0;
  this->NumberOfPieces = 0;
  this->DataIsComposite = 0;
  this->FlatIndexMax = 0;
  this->Internal->ChildrenInformation.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeDataInformation::GetNumberOfChildren()
{
  return this->DataIsMultiPiece?
    this->NumberOfPieces : 
    this->Internal->ChildrenInformation.size();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVCompositeDataInformation::GetDataInformation(
  unsigned int idx)
{
  if (this->DataIsMultiPiece)
    {
    return NULL;
    }

  if (idx >= this->Internal->ChildrenInformation.size())
    {
    return NULL;
    }

  return this->Internal->ChildrenInformation[idx];
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  vtkCompositeDataSet* cds = 
    vtkCompositeDataSet::SafeDownCast(object);
  if (!cds)
    {
    return;
    }

  this->DataIsComposite = 1;

  vtkMultiPieceDataSet* mpDS = vtkMultiPieceDataSet::SafeDownCast(cds);
  if (mpDS)
    {
    this->DataIsMultiPiece = 1;
    this->FlatIndexMax = mpDS->GetNumberOfPieces(); // 0 for self and (1, num-pieces) 
                                                    // for the pieces.
    this->SetNumberOfPieces(mpDS->GetNumberOfPieces());
    return;
    }

  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(cds->NewIterator());
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  iter->SkipEmptyNodesOff();

  // vtkTimerLog::MarkStartEvent("Copying information from composite data");
  unsigned int index=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), index++)
    {
    vtkSmartPointer<vtkPVDataInformation> childInfo;
    vtkDataObject* curDO = iter->GetCurrentDataObject();
    if (curDO)
      {
      childInfo = vtkSmartPointer<vtkPVDataInformation>::New();
      childInfo->CopyFromObject(curDO); 

      vtkPVCompositeDataInformation* cd = 
        childInfo->GetCompositeDataInformation();
      this->FlatIndexMax += (cd->GetFlatIndexMax()+1);
      }
    this->Internal->ChildrenInformation.resize(index+1);
    this->Internal->ChildrenInformation[index] = childInfo;
    }
  // vtkTimerLog::MarkEndEvent("Copying information from composite data");
}

//----------------------------------------------------------------------------
// Called to merge informations from two processess.
void vtkPVCompositeDataInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVCompositeDataInformation *info;

  info = vtkPVCompositeDataInformation::SafeDownCast(pvi);
  if (info == NULL)
    {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
    }

  this->DataIsComposite = info->GetDataIsComposite();
  this->DataIsMultiPiece = info->GetDataIsMultiPiece();
  if (this->DataIsMultiPiece)
    {
    if (this->NumberOfPieces != info->NumberOfPieces)
      {
      // vtkWarningMacro("Mismatch in number of pieces among processes.");
      }
    if (info->NumberOfPieces > this->NumberOfPieces)
      {
      this->NumberOfPieces = info->NumberOfPieces;
      }
    return;
    }

  unsigned int otherNumChildren = info->Internal->ChildrenInformation.size();
  unsigned int numChildren = this->Internal->ChildrenInformation.size();
  if ( otherNumChildren > numChildren)
    {
    numChildren = otherNumChildren;
    this->Internal->ChildrenInformation.resize(numChildren);
    }

  for (unsigned int i=0; i < otherNumChildren; i++)
    {
    vtkPVDataInformation* otherInfo = info->Internal->ChildrenInformation[i];
    vtkPVDataInformation* localInfo = this->Internal->ChildrenInformation[i];
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
        this->Internal->ChildrenInformation[i] = dinf;
        dinf->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyToStream(
  vtkClientServerStream* css)
{
//  vtkTimerLog::MarkStartEvent("Copying composite information to stream");
  css->Reset();
  *css  << vtkClientServerStream::Reply
        << this->DataIsComposite
        << this->DataIsMultiPiece
        << this->NumberOfPieces;

  unsigned int numChildren = this->Internal->ChildrenInformation.size();
  *css << numChildren;
  
  // If data is a vtkHierarchicalBoxDataSet or sub-class, do not get the
  // information for sub-datasets. There may be a lot of them.
  for(unsigned i=0; i<numChildren; i++)
    {
    vtkPVDataInformation* dataInf = this->Internal->ChildrenInformation[i];
    if (dataInf)
      {
      *css << i;
      vtkClientServerStream dcss;
      dataInf->CopyToStream(&dcss);
      
      size_t length;
      const unsigned char* data;
      dcss.GetData(&data, &length);
      *css << vtkClientServerStream::InsertArray(data, length);
      }
    }
  *css << numChildren; // DONE marker
  *css << vtkClientServerStream::End;
//  vtkTimerLog::MarkEndEvent("Copying composite information to stream");
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromStream(
  const vtkClientServerStream* css)
{
  this->Initialize();

  if(!css->GetArgument(0, 0, &this->DataIsComposite))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }

  if(!css->GetArgument(0, 1, &this->DataIsMultiPiece))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }

  if(!css->GetArgument(0, 2, &this->NumberOfPieces))
    {
    vtkErrorMacro("Error parsing number of pieces.");
    return;
    }

  unsigned int numChildren;
  if(!css->GetArgument(0, 3, &numChildren))
    {
    vtkErrorMacro("Error parsing number of children.");
    return;
    }
  int msgIdx = 3;
  this->Internal->ChildrenInformation.resize(numChildren);
  
  while (1)
    {
    msgIdx++;
    unsigned int childIdx;
    if(!css->GetArgument(0, msgIdx, &childIdx))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }
    if (childIdx >= numChildren) //receiver DONE marker.
      {
      break;
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
    this->Internal->ChildrenInformation[childIdx] = dataInf;
    dataInf->Delete();
    }

}
