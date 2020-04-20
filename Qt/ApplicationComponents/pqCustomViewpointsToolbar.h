/*=========================================================================

   Program: ParaView
   Module:    pqCustomViewpointsToolbar.h

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
#ifndef pqCustomViewpointsToolbar_h
#define pqCustomViewpointsToolbar_h

#include "pqApplicationComponentsModule.h"

#include <QPixmap>
#include <QPointer>
#include <QToolBar>

/**
* pqCustomViewpointsToolbar is the toolbar that has buttons for using and configuring
* custom views (aka camera positions)
*/
class QAction;

class PQAPPLICATIONCOMPONENTS_EXPORT pqCustomViewpointsToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqCustomViewpointsToolbar(const QString& title, QWidget* parentObject = 0)
    : Superclass(title, parentObject)
    , BasePixmap(64, 64)
  {
    this->constructor();
  }
  pqCustomViewpointsToolbar(QWidget* parentObject = 0)
    : Superclass(parentObject)
    , BasePixmap(64, 64)
  {
    this->constructor();
  }
  ~pqCustomViewpointsToolbar() override = default;

protected Q_SLOTS:

  /**
   * Clear and recreate all custom viewpoint actions
   * based on current settings
   */
  void updateCustomViewpointActions();

  /**
   * Update the state of the toolbuttons
   * depending of the type of the current active view
   */
  void updateEnabledState();

  /**
   * Open the Custom Viewpoint
   * button dialog to configure the viewpoints
   * manually
   */
  void configureCustomViewpoints();

  /**
   * Slot to apply a custom view point
   */
  void applyCustomViewpoint();

  /**
   * Slot to add current viewpoint
   * to a new custom viewpoint
   */
  void addCurrentViewpointToCustomViewpoints();

  /**
   * Slot to set a custom viewpoint
   * to a current viewpoint
   */
  void setToCurrentViewpoint();

  /**
   * Slot to delete a custom view point
   */
  void deleteCustomViewpoint();

private:
  Q_DISABLE_COPY(pqCustomViewpointsToolbar)
  void constructor();

  QPointer<QAction> PlusAction;
  QPointer<QAction> ConfigAction;
  QPixmap BasePixmap;
  QPixmap PlusPixmap;
  QPixmap ConfigPixmap;
  QVector<QPointer<QAction> > ViewpointActions;
};

#endif
