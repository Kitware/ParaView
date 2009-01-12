/*=========================================================================

   Program: ParaView
   Module:    pqActiveView.h

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

=========================================================================*/
#ifndef __pqActiveView_h
#define __pqActiveView_h

#include <QObject>
#include "pqComponentsExport.h"

class pqView;
class vtkSMProxySelectionModel;
class vtkEventQtSlotConnect;

/// Provides a central location for managing an "active" view
/// (note that a "view" could be a 3D render view, a plot, or
/// any other type of view).
///
/// A slot is provided to set the currently-active view, and
/// a signal notifies observers when the active view changes
/// 
/// To make it possible to synchronize the active view with the python shell,
/// this class now internal uses vtkSMProxySelectionModel (registered as
/// "ActiveView" with the proxy manager.
class PQCOMPONENTS_EXPORT pqActiveView : public QObject
{
  Q_OBJECT
  
public:
  static pqActiveView& instance();
  
  /// Returns the currently-active view (could return NULL)
  pqView* current();
  
signals:
  /// Signal emitted whenever the currently-active view changes
  void changed(pqView* view);

public slots:
  /// Called to set the currently-active view
  void setCurrent(pqView* view);

private slots:
  /// called when vtkSMProxySelectionModel's current changes.
  void smCurrentChanged();

  /// called when a view is destroyed. We ensure that it's not same as the
  //active view. If so, we set the active view to 0.
  void onViewRemoved(pqView*);

private:
  pqActiveView();
  pqActiveView(const pqActiveView&); // Not implemented.
  void operator=(const pqActiveView&); // Not implemented.
  ~pqActiveView();

  vtkEventQtSlotConnect* VTKConnect;
  pqView* ActiveView;
  vtkSMProxySelectionModel* SMActiveView;
};

#endif
