/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef __pqComparativeContextView_h
#define __pqComparativeContextView_h

#include "pqContextView.h"
#include <QPointer>

class vtkSMComparativeViewProxy;

/// The abstract base class for comparative chart views. It handles the layout
/// of the individual chart views in the comparative view.
class PQCORE_EXPORT pqComparativeContextView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  virtual ~pqComparativeContextView();

  /// Return a widget associated with this view.
  virtual QWidget* getWidget();

  /// \returns the internal vtkContextView which provides the implementation for
  /// the chart rendering.
  virtual vtkContextView* getVTKChartView() const;

  /// \returns the comparative view proxy.
  vtkSMComparativeViewProxy* getComparativeViewProxy() const;

  /// Returns the proxy of the root plot view in the comparative view.
  virtual vtkSMViewProxy* getViewProxy() const;

  /// Sets default values for the underlying proxy.
  /// This is during the initialization stage of the pqProxy
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called.
  virtual void setDefaultPropertyValues();

protected slots:
  /// Called when the layout on the comparative vis changes.
  void onComparativeVisLayoutChanged();

protected:
  /// On creation of the view, we need to layout the initial plots.
  virtual void initialize();

protected:
  /// Constructor:
  /// \c type  :- view type.
  /// \c group :- SManager registration group.
  /// \c name  :- SManager registration name.
  /// \c view  :- View proxy.
  /// \c server:- server on which the proxy is created.
  /// \c parent:- QObject parent.
  pqComparativeContextView(const QString& type,
    const QString& group,
    const QString& name,
    vtkSMComparativeViewProxy* view,
    pqServer* server,
    QObject* parent=NULL);

  QPointer<QWidget> Widget;

private:
  pqComparativeContextView(const pqComparativeContextView&); // Not implemented.
  void operator=(const pqComparativeContextView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
