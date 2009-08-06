/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotView.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#ifndef __pqScatterPlotView_h 
#define __pqScatterPlotView_h

#include "pqRenderViewBase.h"

class vtkSMScatterPlotViewProxy;
class ManipulatorType;

/// RenderView used for scatter plot visualization
/// It only takes care of the camera manipulators that can be 2D or 3D
/// depending if the Z-Array is enabled or not.
class PQCORE_EXPORT pqScatterPlotView : public pqRenderViewBase
{
  Q_OBJECT
  typedef pqRenderViewBase Superclass;
public:
  static QString scatterPlotViewType() { return "ScatterPlotRenderView"; }
  static QString scatterPlotViewTypeName() { return "Scatter Plot View"; }

  // Constructor:
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqScatterPlotView( const QString& group,
                const QString& name, 
                vtkSMViewProxy* renModule, 
                pqServer* server, 
                QObject* parent=NULL);
  virtual ~pqScatterPlotView();
  
  /// Returns the render view proxy associated with this object.
  virtual vtkSMScatterPlotViewProxy* getScatterPlotViewProxy() const;
    
  /// Returns a array of 9 ManipulatorType objects defining
  /// default set of camera manipulators used by this type of view.
  /// The default camera manipulator is in 2D.
  static const ManipulatorType* getDefaultManipulatorTypes()
    { return pqScatterPlotView::TwoDManipulatorTypes; }

  /// Resets the camera to include all visible data.
  /// It is essential to call this resetCamera, to ensure that the reset camera
  /// action gets pushed on the interaction undo stack.
  virtual void resetCamera();
  
  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);
  virtual vtkImageData* captureImage(const QSize& size)
    { return this->Superclass::captureImage(size); }
  
  /// Change the camera mode into 2D or 3D
  void set3DMode(bool);
  
  /// Get the camera mode: 2D or 3D
  bool get3DMode()const;

protected:
  /// Return the name of the group used for global settings (except interactor
  /// style).
  virtual const char* globalSettingsGroup() const
    { return "renderModule"; }

  /// Return the name of the group used for view-sepecific settings such as
  /// background color, lighting.
  virtual const char* viewSettingsGroup() const
    { return "renderModule2D"; }

  /// Returns the name of the group in which to save the interactor style
  /// settings.
  virtual const char* interactorStyleSettingsGroup() const
    { return "scatterPlotModule/InteractorStyle"; }

  /// Must be overridden to return the default manipulator types.
  virtual const ManipulatorType* getDefaultManipulatorTypesInternal();

  /// Setups up RenderModule and QVTKWidget binding.
  /// This method is called for all pqRenderView objects irrespective
  /// of whether it is created from state/undo-redo/python or by the GUI. Hence
  /// don't change any render module properties here.
  virtual void initializeWidgets();

private:
  pqScatterPlotView(const pqScatterPlotView&); // Not implemented.
  void operator=(const pqScatterPlotView&); // Not implemented.

  static ManipulatorType TwoDManipulatorTypes[9];
  static ManipulatorType ThreeDManipulatorTypes[9];

  class pqInternal;
  pqInternal* Internal;
};

#endif


