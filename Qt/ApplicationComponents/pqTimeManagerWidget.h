/*=========================================================================

   Program: ParaView
   Module:  pqTimeManagerWidget.h

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
#ifndef pqTimeManagerWidget_h
#define pqTimeManagerWidget_h

#include "pqApplicationComponentsModule.h"

#include <QWidget>

#include <memory> // for unique_ptr

class pqAnimationScene;

/**
 * pqTimeManagerWidget is the main widget for the Time Manager dock.
 *
 * It contains widgets to control current time and animation.
 * This is the main graphical interface where to:
 *  - setup scene time including stride
 *  - select temporal sources that contributes to available time
 *  - create and edit animation tracks
 *
 * @sa pqTimelineWidget, pqTimelineView and pqTimelineModel.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimeManagerWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqTimeManagerWidget(QWidget* parent = nullptr);
  ~pqTimeManagerWidget() override;

protected Q_SLOTS:
  void updateWidgetsVisibility();

  /**
   * When settings changed, we need to redraw the widget
   * to adapt notation and precision.
   */
  void onSettingsChanged();

  /**
   * Set active scene. Updates some connections.
   */
  void setActiveScene(pqAnimationScene*);

private:
  Q_DISABLE_COPY(pqTimeManagerWidget)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
