/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeDataInformationIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataInformationIterator.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVCompositeDataInformation.h"

#include <vtkstd/vector>

class vtkPVCompositeDataInformationIterator::vtkInternal
{
public:
  struct vtkItem
    {
    vtkPVDataInformation* Node;
    unsigned int NextChildIndex;
    vtkItem(vtkPVDataInformation* node)
      {
      this->Node = node;
      this->NextChildIndex = 0;
      }
    };

  vtkstd::vector<vtkItem> Stack;
};

vtkStandardNewMacro(vtkPVCompositeDataInformationIterator);
vtkCxxRevisionMacro(vtkPVCompositeDataInformationIterator, "1.1");
vtkCxxSetObjectMacro(vtkPVCompositeDataInformationIterator, DataInformation, vtkPVDataInformation);
//----------------------------------------------------------------------------
vtkPVCompositeDataInformationIterator::vtkPVCompositeDataInformationIterator()
{
  this->Internal = new vtkInternal();
  this->DataInformation = 0;
  this->CurrentFlatIndex = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositeDataInformationIterator::~vtkPVCompositeDataInformationIterator()
{
  this->SetDataInformation(0);
  delete this->Internal;
  this->Internal = 0;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformationIterator::InitTraversal()
{
  this->Internal->Stack.clear();
  if (this->DataInformation)
    {
    this->Internal->Stack.push_back(vtkInternal::vtkItem(this->DataInformation));
    }
  this->CurrentFlatIndex = 0;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVCompositeDataInformationIterator::GetCurrentDataInformation()
{
  if (this->IsDoneWithTraversal())
    {
    return NULL;
    }

  vtkInternal::vtkItem& item = this->Internal->Stack.back();
  return item.Node;
}

//----------------------------------------------------------------------------
bool vtkPVCompositeDataInformationIterator::IsDoneWithTraversal()
{
  return (this->Internal->Stack.size() == 0);
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformationIterator::GoToNextItem()
{
  if (this->IsDoneWithTraversal())
    {
    return;
    }

  vtkInternal::vtkItem& item = this->Internal->Stack.back();
  if (item.Node)
    {
    vtkPVCompositeDataInformation* cdInfo = item.Node->GetCompositeDataInformation();
    if (cdInfo && cdInfo->GetDataIsComposite() && item.NextChildIndex < cdInfo->GetNumberOfChildren())
      {
      vtkPVDataInformation* current = cdInfo->GetDataInformation(item.NextChildIndex);
      // current may be NULL for multi piece datasets.
      item.NextChildIndex++;
      this->CurrentFlatIndex++;
      this->Internal->Stack.push_back(
        vtkInternal::vtkItem(current));
      return;
      }
    }
  this->Internal->Stack.pop_back();
  this->GoToNextItem(); 
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformationIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataInformation: " << this->DataInformation << endl;
}


