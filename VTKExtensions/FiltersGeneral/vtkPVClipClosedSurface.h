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
/**
 * @class   vtkPVClipClosedSurface
 * @brief   Clipper for generating closed surfaces
 *
 *
 *  This is a subclass of vtkClipClosedSurface
*/

#ifndef vtkPVClipClosedSurface_h
#define vtkPVClipClosedSurface_h

#include "vtkClipClosedSurface.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkPlane;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkPVClipClosedSurface : public vtkClipClosedSurface
{
public:
  vtkTypeMacro(vtkPVClipClosedSurface, vtkClipClosedSurface);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVClipClosedSurface* New();

  //@{
  /**
   * Set/Get the InsideOut flag (off by default)
   */
  vtkSetMacro(InsideOut, int);
  vtkGetMacro(InsideOut, int);
  vtkBooleanMacro(InsideOut, int);
  //@}

  /**
   * Set the clipping plane.
   */
  void SetClippingPlane(vtkPlane* plane);

protected:
  vtkPVClipClosedSurface();
  ~vtkPVClipClosedSurface() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int InsideOut;
  vtkPlane* ClippingPlane;

private:
  vtkPVClipClosedSurface(const vtkPVClipClosedSurface&) = delete;
  void operator=(const vtkPVClipClosedSurface&) = delete;
};

#endif
