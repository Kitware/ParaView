/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerIdCollectionInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerIdCollectionInformation.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"

#include "vtkAreaPicker.h"
#include "vtkProp3DCollection.h"
#include "vtkProp.h"

#include "vtkProcessModule.h"
#include "vtkClientServerID.h"
#include <vtkstd/set>

vtkStandardNewMacro(vtkPVClientServerIdCollectionInformation);
vtkCxxRevisionMacro(vtkPVClientServerIdCollectionInformation, "1.3");

typedef vtkstd::set<vtkClientServerID> vtkClientServerIdSetBase;
class vtkClientServerIdSetType : public vtkClientServerIdSetBase {};

//----------------------------------------------------------------------------
vtkPVClientServerIdCollectionInformation::
  vtkPVClientServerIdCollectionInformation()
{
  this->ClientServerIds = new vtkClientServerIdSetType;
}

//----------------------------------------------------------------------------
vtkPVClientServerIdCollectionInformation::
  ~vtkPVClientServerIdCollectionInformation()
{
  delete this->ClientServerIds;
}

//----------------------------------------------------------------------------
void vtkPVClientServerIdCollectionInformation::
  PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  cerr << indent << "Ids: ";
  vtkstd::set<vtkClientServerID>::const_iterator IdIter;
  for (IdIter = this->ClientServerIds->begin();
       IdIter != this->ClientServerIds->end();
       IdIter++)
    {
     os << *IdIter << " ";
    }
  os << endl;

}

//----------------------------------------------------------------------------
void vtkPVClientServerIdCollectionInformation
  ::CopyFromObject(vtkObject* obj)
{
  vtkAreaPicker* areaPicker = vtkAreaPicker::SafeDownCast(obj);
  if (areaPicker)
    {
    vtkProp3DCollection *props = areaPicker->GetProp3Ds();
    vtkProcessModule *processModule = vtkProcessModule::GetProcessModule();
    
    props->InitTraversal();
    vtkProp *prop;
    while ( (prop = props->GetNextProp()) )
      {
      vtkClientServerID id = processModule->GetIDFromObject(prop);
      this->ClientServerIds->insert(id);
      }    
    return;
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerIdCollectionInformation
  ::AddInformation(vtkPVInformation* pvi)
{
  vtkPVClientServerIdCollectionInformation* other = 
    vtkPVClientServerIdCollectionInformation::SafeDownCast(pvi);
  if (!other)
    {    
    return;
    }

  vtkstd::set<vtkClientServerID>::const_iterator IdIter;
  for (IdIter = other->ClientServerIds->begin();
       IdIter != other->ClientServerIds->end();
       IdIter++)
    {
    this->ClientServerIds->insert(*IdIter);
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerIdCollectionInformation
  ::CopyToStream(vtkClientServerStream* css) const
{
  css->Reset();

  *css << vtkClientServerStream::Reply;

  vtkstd::set<vtkClientServerID>::const_iterator IdIter;
  for (IdIter = this->ClientServerIds->begin();
       IdIter != this->ClientServerIds->end();
       IdIter++)
    {
    *css << *IdIter;
    }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVClientServerIdCollectionInformation
  ::CopyFromStream(const vtkClientServerStream* css)
{
  int numIds;
  numIds = css->GetNumberOfArguments(0);
  vtkClientServerID nextId;
  for (int i = 0; i < numIds; i++)
    {
    css->GetArgument(0, i, &nextId);
    this->ClientServerIds->insert(nextId);
    }
}

//----------------------------------------------------------------------------
int vtkPVClientServerIdCollectionInformation
  ::Contains(vtkClientServerID *id) 
{
  if (this->ClientServerIds->find(*id) != this->ClientServerIds->end())
    {
    return 1;
    }
  else 
    {
    return 0;
    }
}

