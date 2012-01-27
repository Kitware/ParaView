/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkViewLayout.h"

#include "vtkObjectFactory.h"
#include "vtkTileDisplayHelper.h"

vtkStandardNewMacro(vtkViewLayout);
//----------------------------------------------------------------------------
vtkViewLayout::vtkViewLayout()
{
}

//----------------------------------------------------------------------------
vtkViewLayout::~vtkViewLayout()
{
}

//----------------------------------------------------------------------------
void vtkViewLayout::ResetTileDisplay()
{
  vtkTileDisplayHelper::GetInstance()->ResetEnabledKeys();
}

//----------------------------------------------------------------------------
void vtkViewLayout::ShowOnTileDisplay(unsigned int val)
{
  vtkTileDisplayHelper::GetInstance()->EnableKey(val);
}

//----------------------------------------------------------------------------
void vtkViewLayout::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
