/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyDefinitionIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyDefinitionIterator.h"

#include "vtkObjectFactory.h"
class vtkPVXMLElement;

//-----------------------------------------------------------------------------
vtkSMProxyDefinitionIterator::vtkSMProxyDefinitionIterator()
{
}
//---------------------------------------------------------------------------
vtkSMProxyDefinitionIterator::~vtkSMProxyDefinitionIterator()
{
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkSMProxyDefinitionIterator::IsDoneWithTraversal()
{
  return true;
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::GoToNextItem()
{
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxyDefinitionIterator::GetProxyDefinition()
{
  return 0;
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::GoToFirstItem()
{
}
//---------------------------------------------------------------------------
bool vtkSMProxyDefinitionIterator::IsCustom()
{
  return false;
}
//---------------------------------------------------------------------------
const char* vtkSMProxyDefinitionIterator::GetGroupName()
{
  return 0;
}
//---------------------------------------------------------------------------
const char* vtkSMProxyDefinitionIterator::GetProxyName()
{
  return 0;
}
//---------------------------------------------------------------------------
void vtkSMProxyDefinitionIterator::GoToNextGroup()
{
}
