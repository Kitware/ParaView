/*=========================================================================

   Program: ParaView
   Module:    pqTwoDRenderView.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef __pqTwoDRenderView_h 
#define __pqTwoDRenderView_h

#include "pqRenderViewBase.h"

/// pqRenderViewBase specialization used for 2D image views.
class PQCORE_EXPORT pqTwoDRenderView : public pqRenderViewBase
{
  Q_OBJECT
  typedef pqRenderViewBase Superclass;
public:
  static QString twoDRenderViewType() { return "2DRenderView"; }
  static QString twoDRenderViewTypeName() { return "2D View"; }

  pqTwoDRenderView(const QString& group,
                const QString& name, 
                vtkSMViewProxy* renModule, 
                pqServer* server, 
                QObject* parent=NULL);
  virtual ~pqTwoDRenderView();

  /// Returns a array of 9 ManipulatorType objects defining
  /// default set of camera manipulators used by this type of view.
  /// There are exactly 9 entires in the returned array. It's is deliberately
  /// returned as non-constant. Developers can change the default by directly
  /// updating the entries.
  static ManipulatorType* getDefaultManipulatorTypes()
    { return pqTwoDRenderView::DefaultManipulatorTypes; }

  /// Resets the camera to include all visible data.
  /// It is essential to call this resetCamera, to ensure that the reset camera
  /// action gets pushed on the interaction undo stack.
  virtual void resetCamera();

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);
  virtual vtkImageData* captureImage(const QSize& size)
    { return this->Superclass::captureImage(size); }

  /// returns whether a source can be displayed in this view module 
  virtual bool canDisplay(pqOutputPort* opPort) const;
  
protected slots:
  /// Called when representationVisibilityChanged() is fired.
  /// Since this view can only show 1 image at a time, we need to ensure that no
  /// other repr is currently visible.
  void updateVisibility(pqRepresentation* repr, bool visible);

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
    { return "renderModule2D/InteractorStyle"; }

  /// Must be overridden to return the default manipulator types.
  virtual const ManipulatorType* getDefaultManipulatorTypesInternal()
    { return pqTwoDRenderView::getDefaultManipulatorTypes(); }

  /// Setups up RenderModule and QVTKWidget binding.
  /// This method is called for all pqTwoDRenderView objects irrespective
  /// of whether it is created from state/undo-redo/python or by the GUI. Hence
  /// don't change any render module properties here.
  virtual void initializeWidgets();
private:
  pqTwoDRenderView(const pqTwoDRenderView&); // Not implemented.
  void operator=(const pqTwoDRenderView&); // Not implemented.

  static ManipulatorType DefaultManipulatorTypes[9];
  bool InitializedWidgets;
};

#endif


