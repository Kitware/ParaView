/*=========================================================================

   Program:   ParaQ
   Module:    pqPointSourceWidget.h

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

#ifndef _pqPointSourceWidget_h
#define _pqPointSourceWidget_h

#include "pqSMProxy.h"

#include <QWidget>

class pqPropertyManager;

/// Provides a complete Qt UI for working with a vtkPointSource filter
class pqPointSourceWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqPointSourceWidget(QWidget* p);
  ~pqPointSourceWidget();

  /** Sets a "reference" proxy that will be used to provide bounds
  for the 3D point widget */
  void setReferenceProxy(pqSMProxy proxy);
  /** Sets the vtkPointSource proxy that will actually be controlled
  by user interaction */
  void setControlledProxy(pqSMProxy proxy);

  /// Enables the UI, making the 3D widget visible
  void showWidget(pqPropertyManager* property_manager);
  /// Accepts pending changes, pushing them to the server manager
  void accept();
  /// Resets pending changes, restoring the original state
  void reset();
  /// Disables the UI, hiding the 3D widget
  void hideWidget(pqPropertyManager* property_manager);

signals:
  /// Signal emitted whenever any part of the UI is modified
  void widgetChanged();

private slots:
  void widgetChanged(const QString&);

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
