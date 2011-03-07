/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProxyDefinitionIterator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProxyDefinitionIterator.h"

#include "vtkObjectFactory.h"
class vtkPVXMLElement;

//-----------------------------------------------------------------------------
vtkPVProxyDefinitionIterator::vtkPVProxyDefinitionIterator()
{
}
//---------------------------------------------------------------------------
vtkPVProxyDefinitionIterator::~vtkPVProxyDefinitionIterator()
{
}
//---------------------------------------------------------------------------
void vtkPVProxyDefinitionIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkPVProxyDefinitionIterator::IsDoneWithTraversal()
{
  return true;
}
//---------------------------------------------------------------------------
void vtkPVProxyDefinitionIterator::GoToNextItem()
{
}
//---------------------------------------------------------------------------
vtkPVXMLElement* vtkPVProxyDefinitionIterator::GetProxyDefinition()
{
  return 0;
}
//---------------------------------------------------------------------------
void vtkPVProxyDefinitionIterator::GoToFirstItem()
{
}
//---------------------------------------------------------------------------
bool vtkPVProxyDefinitionIterator::IsCustom()
{
  return false;
}
//---------------------------------------------------------------------------
const char* vtkPVProxyDefinitionIterator::GetGroupName()
{
  return 0;
}
//---------------------------------------------------------------------------
const char* vtkPVProxyDefinitionIterator::GetProxyName()
{
  return 0;
}
//---------------------------------------------------------------------------
void vtkPVProxyDefinitionIterator::GoToNextGroup()
{
}
