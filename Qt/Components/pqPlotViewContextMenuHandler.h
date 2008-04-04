/*=========================================================================

   Program: ParaView
   Module:    pqPlotViewContextMenuHandler.h

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

/// \file pqPlotViewContextMenuHandler.h
/// \date 9/19/2007

#ifndef _pqPlotViewContextMenuHandler_h
#define _pqPlotViewContextMenuHandler_h


#include "pqComponentsExport.h"
#include "pqViewContextMenuHandler.h"

class pqActiveViewOptionsManager;


/// \class pqPlotViewContextMenuHandler
/// \brief
///   The pqPlotViewContextMenuHandler class sets up and cleans up
///   context menus for the plot view type.
class PQCOMPONENTS_EXPORT pqPlotViewContextMenuHandler :
    public pqViewContextMenuHandler
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a plot view context menu handler.
  /// \param parent The parent object.
  pqPlotViewContextMenuHandler(QObject *parent=0);
  virtual ~pqPlotViewContextMenuHandler() {}

  /// \brief
  ///   Sets the view options manager.
  ///
  /// The view options manager is used to open the options dialog for
  /// the chart.
  ///
  /// \param manager The view options manager.
  void setOptionsManager(pqActiveViewOptionsManager *manager);

  /// \brief
  ///   Sets up the context menu for the given view.
  ///
  /// A context menu is only added to plot views. A
  /// pqPlotViewContextMenu object is added to the view to handle the
  /// context menu requests.
  ///
  /// \param view The view to set up.
  virtual void setupContextMenu(pqView *view);

  /// \brief
  ///   Cleans up the context menu for the given view.
  ///
  /// The context menu request handler is removed from the view if it
  /// is a plot view type.
  ///
  /// \param view The view to clean up.
  virtual void cleanupContextMenu(pqView *view);

signals:
  /// Emitted when a screenshot has been requested.
  void screenshotRequested();

private:
  /// Stores the options dialog handler.
  pqActiveViewOptionsManager *Manager;
};

#endif
