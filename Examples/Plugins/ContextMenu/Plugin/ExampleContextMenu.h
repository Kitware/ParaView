/*=========================================================================

   Program: ParaView
   Module:  ExampleContextMenu.h

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
#ifndef ExampleContextMenu_h
#define ExampleContextMenu_h

#include "pqContextMenuInterface.h"

/**
 * @class ExampleContextMenu
 * @brief Add a context menu to pqViews.
 *
 * ExampleContextMenu is the interface which plugins adding a context menu
 * to pqViews should implement. One would typically use the `add_paraview_context_menu`
 * CMake macro to ensure an instance of the class is created and registered with
 * the pqPipelineContextMenuBehavior class (which is responsible for creating
 * the context menu).
 */

class ExampleContextMenu : public QObject, public pqContextMenuInterface
{
  Q_OBJECT
  Q_INTERFACES(pqContextMenuInterface)
public:
  ExampleContextMenu();
  ExampleContextMenu(QObject* parent);
  virtual ~ExampleContextMenu();

  /// This method is called when a context menu is requested,
  /// usually by a right click in a pqView instance.
  ///
  /// This method should return true if the context is one
  /// handled by this instance (and presumably it will modify
  /// the provided QMenu) and false otherwise (indicating the
  /// context is not one this instance handles).
  ///
  /// Each registered interface is called until one returns
  /// true, so your implementation should return false as
  /// quickly as possible.
  ///
  /// If dataContext is a pqDataRepresentation and holds
  /// multiblock data, the dataBlockContext is a list of
  /// block IDs to which the menu actions should apply.
  bool contextMenu(QMenu* menu, pqView* viewContext, const QPoint& viewPoint,
    pqRepresentation* dataContext, const QList<unsigned int>& dataBlockContext) const override;

  /// Run before the default context menu (which has a priority of 0).
  int priority() const override { return 1; }

public slots:
  virtual void twiddleThumbsAction();

private:
  Q_DISABLE_COPY(ExampleContextMenu)
};

#endif
