/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVClipClosedSurface.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClipClosedSurface - Clipper for generating closed surfaces
//
// .SECTION Description
//  This is a subclass of vtkClipClosedSurface

#ifndef __vtkPVClipClosedSurface_h
#define __vtkPVClipClosedSurface_h

#include "vtkClipClosedSurface.h"

class vtkPlane;

class VTK_EXPORT vtkPVClipClosedSurface : public vtkClipClosedSurface
{
public:
  vtkTypeMacro( vtkPVClipClosedSurface, vtkClipClosedSurface );
  void PrintSelf( ostream & os, vtkIndent indent );
  static vtkPVClipClosedSurface * New();

  // Description:
  // Set/Get the InsideOut flag (off by default)
  vtkSetMacro( InsideOut, int );
  vtkGetMacro( InsideOut, int );
  vtkBooleanMacro( InsideOut, int );

  // Description:
  // Set the clipping plane.
  void   SetClippingPlane( vtkPlane * plane );

protected:
  vtkPVClipClosedSurface();
  ~vtkPVClipClosedSurface();

  virtual int RequestData( vtkInformation        * request,
                           vtkInformationVector ** inputVector,
                           vtkInformationVector  * outputVector );

  int         InsideOut;
  vtkPlane  * ClippingPlane;

private:

  vtkPVClipClosedSurface( const vtkPVClipClosedSurface & ); // Not implemented
  void operator = ( const vtkPVClipClosedSurface & );       // Not implemented
};

#endif
