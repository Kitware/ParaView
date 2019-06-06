/*=========================================================================

   Program: ParaView
   Module:  pqContextMenuInterface.h

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
#ifndef pqContextMenuInterface_h
#define pqContextMenuInterface_h

#include "pqComponentsModule.h"
#include <QList>
#include <QObject>

/**
 * @class pqContextMenuInterface
 * @brief Interface class for plugins that add a context menu to pqViews.
 *
 * pqContextMenuInterface is the interface which plugins adding a context menu
 * to pqViews should implement. One would typically use the `add_paraview_context_menu`
 * CMake macro to ensure an instance of the class is created and registered with
 * the pqPipelineContextMenuBehavior class (which is responsible for creating
 * the context menu).
 */
class QMenu;
class pqView;
class pqRepresentation;

class PQCOMPONENTS_EXPORT pqContextMenuInterface
{
public:
  pqContextMenuInterface();
  virtual ~pqContextMenuInterface();

  /// This method is called when a context menu is requested,
  /// usually by a right click in a pqView instance.
  ///
  /// This method should return true if (a) the context is one
  /// handled by this instance (and presumably it will modify
  /// the provided QMenu); and (b) this instance should be the
  /// last interface to contribute to the menu.
  /// Returning false indicates the context is not one this
  /// instance handles *or* that interfaces with a lower priority
  /// may modify the menu.
  ///
  /// Each registered interface is called in order of descending
  /// priority until one returns true, so your implementation
  /// should return false as quickly as possible.
  ///
  /// If dataContext is a pqDataRepresentation and holds
  /// multiblock data, the dataBlockContext is a list of
  /// block IDs to which the menu actions should apply.
  virtual bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const = 0;

  /// This method's return value is used to set the precedence of the interface.
  ///
  /// Interfaces with greater priority are invoked before others and may cause
  /// menu-building to terminate early.
  /// ParaView's default context-menu interface uses a priority of 0 and returns false.
  ///
  /// If you wish to modify the default menu, assign a negative priority to your interface.
  /// If you wish to override the default menu, assign a positive priority to your interface
  /// and have `contextMenu()` return true.
  virtual int priority() const { return -1; }

private:
  Q_DISABLE_COPY(pqContextMenuInterface)
};

Q_DECLARE_INTERFACE(pqContextMenuInterface, "com.kitware/paraview/contextmenu")
#endif
