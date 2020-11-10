/*=========================================================================

   Program: ParaView
   Module:    pqTimelineScrollbar.h

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

#ifndef pqTimelineScrollbar_h
#define pqTimelineScrollbar_h

#include "pqWidgetsModule.h"

#include <QWidget>

class QScrollBar;
class QSpacerItem;

class pqAnimationModel;

/**
* A widget offering a scrollbar useful to interact with the timeline
* from the animation model.
*/
class PQWIDGETS_EXPORT pqTimelineScrollbar : public QWidget
{
  Q_OBJECT

public:
  pqTimelineScrollbar(QWidget* p = nullptr);
  ~pqTimelineScrollbar() override = default;

  /**
  * connects to an existing animation model
  * if the parameter is nullptr, any already existing connection is removed
  */
  void setAnimationModel(pqAnimationModel* model);

  /**
  * connects to an existing spacing constraint notifier
  * if the parameter is nullptr, any already existing connection is removed
  */
  void linkSpacing(QObject* spaceNotifier);

protected Q_SLOTS:

  /**
  * called when the offset of the time scrollbar
  * must be updated
  */
  void updateTimeScrollbar();

  /**
  * called when the time scrollbar must be updated
  */
  void updateTimeScrollbarOffset(int);

  /**
  * called when the time scrollbar is being used in the GUI
  */
  void setTimeZoom(int);

private:
  QScrollBar* TimeScrollBar = nullptr;
  QSpacerItem* ScrollBarSpacer = nullptr;

  QObject* SpacingNotifier = nullptr;

  pqAnimationModel* AnimationModel = nullptr;
};

#endif // pqTimelineScrollbar_h
