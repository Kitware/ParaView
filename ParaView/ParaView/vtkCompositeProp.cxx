/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeProp.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeProp.h"

#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"

vtkStandardNewMacro(vtkCompositeProp);

vtkCxxRevisionMacro(vtkCompositeProp, "1.1");

// Creates an Prop with the following defaults: visibility on.
vtkCompositeProp::vtkCompositeProp()
{
  this->Props = vtkPropCollection::New();
}

vtkCompositeProp::~vtkCompositeProp()
{
  this->Props->Delete();
}

#define vtkMAX(a,b) (((a)>(b))?(a):(b))
#define vtkMIN(a,b) (((a)<(b))?(a):(b))

float *vtkCompositeProp::GetBounds()
{
  // Calculate bounds
  int cc;
  for (cc =0; cc < 3; cc ++ )
    {
    this->Bounds[cc*2] = VTK_FLOAT_MAX;
    this->Bounds[cc*2+1] = VTK_FLOAT_MIN;    
    }

  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    float *tb = p->GetBounds();
    if ( tb )
      {
      for ( cc = 0; cc < 3; cc ++ )
	{	
	this->Bounds[cc*2] = vtkMIN(this->Bounds[cc*2], tb[cc*2]);
	this->Bounds[cc*2+1] = vtkMAX(this->Bounds[cc*2+1], tb[cc*2+1]);
	}
      }
    }

  for ( cc = 0; cc < 6; cc ++ )
    {
    if ( this->Bounds[cc] >= VTK_FLOAT_MAX-1 ||
	 this->Bounds[cc] <= VTK_FLOAT_MIN+1 )
      {
      return 0;
      }
    }
  return this->Bounds;
}

// This method is invoked if the prop is picked.
// This method is invoked if the prop is picked.
void vtkCompositeProp::Pick()
{
  this->Superclass::Pick();
}

int vtkCompositeProp::RenderOpaqueGeometry(vtkViewport *v)
{
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    p->RenderOpaqueGeometry(v);
    }
}

int vtkCompositeProp::RenderTranslucentGeometry(vtkViewport *v)
{
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    p->RenderTranslucentGeometry(v);
    }
}

int vtkCompositeProp::RenderOverlay(vtkViewport *v)
{
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    p->RenderOverlay(v);
    }
}

void vtkCompositeProp::ReleaseGraphicsResources(vtkWindow *v)
{
  vtkProp *p = 0;
  this->Props->InitTraversal();
  while( (p = this->Props->GetNextProp()) )
    {
    p->ReleaseGraphicsResources(v);
    }
}

void vtkCompositeProp::AddProp(vtkProp *p)
{
  this->Props->AddItem(p);
}

void vtkCompositeProp::RemoveProp(vtkProp *p)
{
  this->Props->RemoveItem(p);
}

void vtkCompositeProp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Props: " << this->Props << endl;
}

