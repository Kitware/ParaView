/*=========================================================================

   Program:   ParaQ
   Module:    pqPicking.h

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


#ifndef _pqPicking_h
#define _pqPicking_h

#include "pqWidgetsExport.h"
#include <QObject>
#include <vtkType.h>
class vtkObject;
class vtkCommand;
class vtkRenderWindowInteractor;
class vtkSMRenderModuleProxy;
class vtkSMProxy;
class vtkSMDisplayProxy;
class vtkSMPointLabelDisplayProxy;
class vtkUnstructuredGrid;

/// class to do picking
class PQWIDGETS_EXPORT pqPicking : public QObject
{
  Q_OBJECT
public:
  pqPicking(vtkSMRenderModuleProxy* rm, QObject* p);
  ~pqPicking();

public slots:

  /// compute selection when given the render window interactor
  void computeSelection(vtkObject* style, unsigned long, void*, void*, vtkCommand*);
  
  /// pick a cell on a current source proxy with given screen coordinates
  void computeSelection(vtkRenderWindowInteractor* iren, int X, int Y);

signals:
  /// emit selection changed, proxy and dataset is given
  void selectionChanged(vtkSMProxy* p, vtkUnstructuredGrid* selections);

private:
  vtkSMRenderModuleProxy* RenderModule;
  vtkSMProxy* PickFilter;
  vtkSMDisplayProxy* PickDisplay;
  vtkSMPointLabelDisplayProxy* PickRetriever;
  vtkUnstructuredGrid* EmptySet;

};

#endif

