/*=========================================================================

  Module:    vtkKWSeparator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSeparator.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWSeparator );
vtkCxxRevisionMacro(vtkKWSeparator, "1.3");

//----------------------------------------------------------------------------
vtkKWSeparator::vtkKWSeparator()
{
  this->Orientation = vtkKWSeparator::OrientationHorizontal;
  this->Thickness   = 2;
}

//----------------------------------------------------------------------------
void vtkKWSeparator::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->SetBorderWidth(2);

#if defined(_WIN32)
  this->SetReliefToGroove();
#else
  this->SetReliefToSunken();
#endif

  this->UpdateAspect();
}

//----------------------------------------------------------------------------
void vtkKWSeparator::UpdateAspect()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->Orientation == vtkKWSeparator::OrientationVertical)
    {
    this->SetWidth(this->Thickness);
    this->SetHeight(0);
    }
  else
    {
    this->SetWidth(0);
    this->SetHeight(this->Thickness);
    }
}

//----------------------------------------------------------------------------
void vtkKWSeparator::SetOrientation(int arg)
{
  if (arg < vtkKWSeparator::OrientationHorizontal)
    {
    arg = vtkKWSeparator::OrientationHorizontal;
    }
  if (arg > vtkKWSeparator::OrientationVertical)
    {
    arg = vtkKWSeparator::OrientationVertical;
    }
  if (this->Orientation == arg)
    {
    return;
    }

  this->Orientation = arg;

  this->Modified();

  this->UpdateAspect();
}

//----------------------------------------------------------------------------
void vtkKWSeparator::SetThickness(int arg)
{
  if (this->Thickness == arg || this->Thickness < 0)
    {
    return;
    }

  this->Thickness = arg;

  this->Modified();

  this->UpdateAspect();
}

//----------------------------------------------------------------------------
void vtkKWSeparator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Orientation: "<< this->Orientation << endl;
  os << indent << "Thickness: "<< this->Thickness << endl;
}

