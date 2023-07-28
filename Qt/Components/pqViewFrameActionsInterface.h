// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqViewFrameActionsInterface_h
#define pqViewFrameActionsInterface_h

#include "pqComponentsModule.h"
#include <QtPlugin>

class pqViewFrame;
class pqView;

/**
 * pqViewFrameActionsInterface is an interface used by pqMultiViewWidget
 * to add actions/toolbuttons to pqViewFrame placed in a pqMultiViewWidget.
 * Thus, if you want to customize the buttons shown at the top of a view frame
 * in your application/plugin, this is the interface to implement.
 */
class PQCOMPONENTS_EXPORT pqViewFrameActionsInterface
{
public:
  virtual ~pqViewFrameActionsInterface();

  /**
   * This method is called after a frame is assigned to a view. The view may be
   * nullptr, indicating the frame has been assigned to an empty view. Frames are
   * never reused (except a frame assigned to an empty view).
   */
  virtual void frameConnected(pqViewFrame* frame, pqView* view) = 0;

private:
};
Q_DECLARE_INTERFACE(pqViewFrameActionsInterface, "com.kitware/paraview/viewframeactions");
#endif
