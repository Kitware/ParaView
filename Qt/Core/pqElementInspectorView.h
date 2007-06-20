/*=========================================================================

   Program: ParaView
   Module:    pqElementInspectorView.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqElementInspectorView_h 
#define __pqElementInspectorView_h

#include "pqView.h"

/// pqElementInspectorView is the view use by Element inspector widget.
class PQCORE_EXPORT pqElementInspectorView : public pqView 
{
  Q_OBJECT
  typedef pqView Superclass;
public:
  static QString eiViewType() { return "ElementInspectorView";}
  static QString eiViewTypeName() { return "Element Inspector"; }

public:
 pqElementInspectorView(const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* parent=NULL);
  virtual ~pqElementInspectorView();

  /// Return a widget associated with this view.
  /// This view has no widget.
  virtual QWidget* getWidget() 
    { return 0; }

  /// This view does not support saving to image.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/)
    { return false; }

  /// This view does not support image caprture, return 0;
  virtual vtkImageData* captureImage(int /*magnification*/)
    { return 0; }
  
  /// Currently, this view can show only Extraction filters.
  virtual bool canDisplaySource(pqPipelineSource* source) const;

private:
  pqElementInspectorView(const pqElementInspectorView&); // Not implemented.
  void operator=(const pqElementInspectorView&); // Not implemented.
};

#endif


