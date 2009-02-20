/*=========================================================================

   Program: ParaView
   Module:    pqChartViewContextMenuHandler.h

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

#ifndef _pqChartViewContextMenuHandler_h
#define _pqChartViewContextMenuHandler_h


#include "pqComponentsExport.h"
#include "pqViewContextMenuHandler.h"

class pqActiveViewOptionsManager;
class pqChartViewContextMenu;


/// \class pqChartViewContextMenuHandler
/// \brief
///   The pqChartViewContextMenuHandler class sets up and cleans up
///   context menus for chart view types.
class PQCOMPONENTS_EXPORT pqChartViewContextMenuHandler :
    public pqViewContextMenuHandler
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart view context menu handler.
  /// \param parent The parent object.
  pqChartViewContextMenuHandler(QObject *parent=0);
  virtual ~pqChartViewContextMenuHandler() {}

  /// \brief
  ///   Gets the view options manager.
  /// \return
  ///   A pointer to the view options manager.
  pqActiveViewOptionsManager *getOptionsManager() const {return this->Manager;}

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
  /// A context menu is only added to chart views. A
  /// pqChartViewContextMenu object is added to the view to handle the
  /// context menu requests.
  ///
  /// \param view The view to set up.
  virtual void setupContextMenu(pqView *view);

  /// \brief
  ///   Cleans up the context menu for the given view.
  ///
  /// The context menu request handler is removed from the view if it
  /// is a chart view type.
  ///
  /// \param view The view to clean up.
  virtual void cleanupContextMenu(pqView *view);

signals:
  /// Emitted when a screenshot has been requested.
  void screenshotRequested();

protected:
  /// \brief
  ///   Creates the chart context menu setup object for the view.
  ///
  /// Sub-classes can override this method to change the type of
  /// context menu item provided.
  ///
  /// \param view The chart view to set up.
  /// \return
  ///   A new chart context menu setup object.
  virtual pqChartViewContextMenu *createContextMenu(pqView *view);

private:
  /// Stores the options dialog handler.
  pqActiveViewOptionsManager *Manager;
};

#endif
