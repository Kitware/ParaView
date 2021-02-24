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
#ifndef pqComparativeContextView_h
#define pqComparativeContextView_h

#include "pqContextView.h"
#include <QPointer>

class vtkSMComparativeViewProxy;

/**
* The abstract base class for comparative chart views. It handles the layout
* of the individual chart views in the comparative view.
*/
class PQCORE_EXPORT pqComparativeContextView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  ~pqComparativeContextView() override;

  /**
  * \returns the internal vtkContextView which provides the implementation for
  * the chart rendering.
  */
  vtkContextView* getVTKContextView() const override;

  /**
  * Returns the context view proxy associated with this object.
  */
  vtkSMContextViewProxy* getContextViewProxy() const override;

  /**
  * \returns the comparative view proxy.
  */
  vtkSMComparativeViewProxy* getComparativeViewProxy() const;

  /**
  * Returns the proxy of the root plot view in the comparative view.
  */
  virtual vtkSMViewProxy* getViewProxy() const;

protected Q_SLOTS:
  /**
  * Called when the layout on the comparative vis changes.
  */
  void updateViewWidgets();

protected:
  /**
  * Create the QWidget for this view.
  */
  QWidget* createWidget() override;

protected:
  /**
  * Constructor:
  * \c type  :- view type.
  * \c group :- SManager registration group.
  * \c name  :- SManager registration name.
  * \c view  :- View proxy.
  * \c server:- server on which the proxy is created.
  * \c parent:- QObject parent.
  */
  pqComparativeContextView(const QString& type, const QString& group, const QString& name,
    vtkSMComparativeViewProxy* view, pqServer* server, QObject* parent = nullptr);

  QPointer<QWidget> Widget;

private:
  Q_DISABLE_COPY(pqComparativeContextView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
