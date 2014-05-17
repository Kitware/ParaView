/*=========================================================================

   Program: ParaView
   Module:    pqComparativeRenderView.h

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
#ifndef __pqComparativeRenderView_h 
#define __pqComparativeRenderView_h

#include "pqRenderView.h"

class vtkSMComparativeViewProxy;

/// RenderView used for comparative visualization (or film-strip visualization).
class PQCORE_EXPORT pqComparativeRenderView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;
public:
  static QString comparativeRenderViewType() { return "ComparativeRenderView"; }

  // Constructor:
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqComparativeRenderView( const QString& group,
                const QString& name, 
                vtkSMViewProxy* renModule, 
                pqServer* server, 
                QObject* parent=NULL);
  virtual ~pqComparativeRenderView();

  /// Returns the comparative view proxy.
  vtkSMComparativeViewProxy* getComparativeRenderViewProxy() const;

  /// Returns the root render view in the comparative view.
  virtual vtkSMRenderViewProxy* getRenderViewProxy() const;

  /// This method is temporarily overridden to handle image capture.
  /// Eventually, we need to move this logic to vtkSMComparativeViewProxy.
  virtual vtkImageData* captureImage(int magnification);
  using Superclass::captureImage;

protected slots:
  /// Called when the layout on the comparative vis changes.
  void onComparativeVisLayoutChanged();

protected:
  /// Creates a new instance of the QWidget subclass to be used to show this
  /// view. Default implementation creates a QVTKWidget.
  virtual QWidget* createWidget();

  /// Use this method to initialize the pqObject state using the
  /// underlying vtkSMProxy. This needs to be done only once,
  /// after the object has been created. 
  virtual void initialize();

private:
  pqComparativeRenderView(const pqComparativeRenderView&); // Not implemented.
  void operator=(const pqComparativeRenderView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
