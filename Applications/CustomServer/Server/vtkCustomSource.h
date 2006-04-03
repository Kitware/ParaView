/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __vtkCustomSource_h
#define __vtkCustomSource_h

#include "vtkPolyDataAlgorithm.h"

/// Sample custom VTK component (actually, a copy of vtkCubeSource)
class vtkCustomSource : public vtkPolyDataAlgorithm 
{
public:
  static vtkCustomSource *New();
  vtkTypeRevisionMacro(vtkCustomSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Set the length of the cube in the x-direction.
  vtkSetClampMacro(XLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(XLength,double);

  /// Set the length of the cube in the y-direction.
  vtkSetClampMacro(YLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(YLength,double);

  /// Set the length of the cube in the z-direction.
  vtkSetClampMacro(ZLength,double,0.0,VTK_DOUBLE_MAX);
  vtkGetMacro(ZLength,double);

  /// Set the center of the cube.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  /// Convenience method allows creation of cube by specifying bounding box.
  void SetBounds(double xMin, double xMax,
                 double yMin, double yMax,
                 double zMin, double zMax);
  void SetBounds(double bounds[6]);

protected:
  vtkCustomSource(double xL=1.0, double yL=1.0, double zL=1.0);
  ~vtkCustomSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double XLength;
  double YLength;
  double ZLength;
  double Center[3];
private:
  vtkCustomSource(const vtkCustomSource&);  // Not implemented.
  void operator=(const vtkCustomSource&);  // Not implemented.
};

#endif
