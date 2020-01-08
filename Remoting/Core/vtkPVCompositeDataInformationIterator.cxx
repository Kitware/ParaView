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
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"

#include <vector>

class vtkPVCompositeDataInformationIterator::vtkInternal
{
public:
  struct vtkItem
  {
    vtkPVDataInformation* Node;
    unsigned int NextChildIndex;
    const char* Name;
    vtkItem(vtkPVDataInformation* node, const char* name)
    {
      this->Node = node;
      this->NextChildIndex = 0;
      this->Name = name;
    }
  };

  std::vector<vtkItem> Stack;
};

vtkStandardNewMacro(vtkPVCompositeDataInformationIterator);
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
    this->Internal->Stack.push_back(vtkInternal::vtkItem(this->DataInformation, NULL));
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
const char* vtkPVCompositeDataInformationIterator::GetCurrentName()
{
  if (this->IsDoneWithTraversal())
  {
    return NULL;
  }
  vtkInternal::vtkItem& item = this->Internal->Stack.back();
  return item.Name;
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
    if (cdInfo && cdInfo->GetDataIsComposite() &&
      item.NextChildIndex < cdInfo->GetNumberOfChildren())
    {
      vtkPVDataInformation* current = cdInfo->GetDataInformation(item.NextChildIndex);
      const char* name = cdInfo->GetName(item.NextChildIndex);
      // current may be NULL for multi piece datasets.
      item.NextChildIndex++;
      this->CurrentFlatIndex++;
      this->Internal->Stack.push_back(vtkInternal::vtkItem(current, name));
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
  os << indent << "CurrentFlatIndex: " << this->CurrentFlatIndex << endl;
}
