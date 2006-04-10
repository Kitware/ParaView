/*=========================================================================

   Program:   ParaQ
   Module:    pqDisplayProxyEditor.h

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

#ifndef _pqDisplayProxyEditor_h
#define _pqDisplayProxyEditor_h

#include <QWidget>
#include "pqWidgetsExport.h"
#include "pqSMProxy.h"
class vtkSMDisplayProxy;

class pqDisplayProxyEditorInternal;

/// Widget which provides an editor for the properties of a display proxy
class PQWIDGETS_EXPORT pqDisplayProxyEditor : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqDisplayProxyEditor(QWidget* p);
  /// destructor
  ~pqDisplayProxyEditor();

  /// set the proxy to display properties for  TODO: fix pqPipelineData so we don't need the sourceProxy
  void setDisplayProxy(vtkSMDisplayProxy* displayProxy, pqSMProxy sourceProxy);
  /// get the proxy for which properties are displayed
  vtkSMDisplayProxy* displayProxy();

protected slots:

  /// internally used to update the graphics window when a property changes
  void updateView();
  void colorByChanged(const QString& val);
  
protected:

  // the display proxy
  vtkSmartPointer<vtkSMDisplayProxy> DisplayProxy;
  // the source proxy for the display (TODO remove this when pqPipeLineData can do it for us)
  pqSMProxy SourceProxy;
  pqDisplayProxyEditorInternal* Internal;

};

#endif

