/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateVersionControllerBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateVersionControllerBase.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkSMStateVersionControllerBase::vtkSMStateVersionControllerBase()
{
}

//----------------------------------------------------------------------------
vtkSMStateVersionControllerBase::~vtkSMStateVersionControllerBase()
{
}

//----------------------------------------------------------------------------
void vtkSMStateVersionControllerBase::ReadVersion(vtkPVXMLElement* root,
  int version[3])
{
  const char* str_version = root->GetAttribute("version");
  if (str_version)
    {
    sscanf(str_version, "%d.%d.%d", &version[0], &version[1], &version[2]);
    }
}

//----------------------------------------------------------------------------
void vtkSMStateVersionControllerBase::Select(vtkPVXMLElement* root,
  const char* childName,
  const char* childAttrs[],
  bool (*funcPtr)(vtkPVXMLElement*, void*),
  void* callData)
{
  bool restart = true;
  do
    {
    restart = false;
    unsigned int max = root->GetNumberOfNestedElements();
    for (unsigned int cc=0; cc < max; cc++)
      {
      vtkPVXMLElement* child = root->GetNestedElement(cc);
      if (child->GetName() && (strcmp(child->GetName(), childName)==0))
        {
        // Does the child have all the attributes requested?
        bool match=true;
        if (childAttrs && childAttrs[0])
          {
          int i=0;
          while (childAttrs[i] && childAttrs[i+1])
            {
            const char* attrValue = child->GetAttribute(childAttrs[i]);
            if (!attrValue || (strcmp(attrValue, childAttrs[i+1])!=0))
              {
              match = false;
              break;
              }
            i+=2;
            }

          if (match)
            {
            if (!(*funcPtr)(child, callData))
              {
              restart = true;
              break;
              }
            }
          }
        }
      }
    } while (restart);
}

//----------------------------------------------------------------------------
static bool vtkSMStateVersionControllerBaseSetAttributes(
  vtkPVXMLElement* root, void* callData)
{
  const char** newAttrs = reinterpret_cast<const char**>(callData);
  if (newAttrs)
    {
    int cc=0;
    while (newAttrs[cc] && newAttrs[cc+1])
      {
      root->SetAttribute(newAttrs[cc], newAttrs[cc+1]);
      cc+=2;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionControllerBase::SelectAndSetAttributes(vtkPVXMLElement* root,
  const char* childName,
  const char* childAttrs[],
  const char* newAttrs[])
{
  this->Select(root, childName, childAttrs,
    &vtkSMStateVersionControllerBaseSetAttributes,
    newAttrs);
}

//----------------------------------------------------------------------------
static bool vtkSMStateVersionControllerBaseRemove(
  vtkPVXMLElement* root, void* )
{
  vtkPVXMLElement* parent = root->GetParent();
  if (parent)
    {
    parent->RemoveNestedElement(root);
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMStateVersionControllerBase::SelectAndRemove(vtkPVXMLElement* root,
    const char* childName,
    const char* childAttrs[])
{
  this->Select(root, childName, childAttrs,
    &vtkSMStateVersionControllerBaseRemove, 0);
}

//----------------------------------------------------------------------------
void vtkSMStateVersionControllerBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


