/*=========================================================================

   Program: ParaView
   Module:    pqExodusIIPanel.h

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
cxxPROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqExodusIIPanel_h
#define _pqExodusIIPanel_h

#include "pqNamedObjectPanel.h"
#include "pqComponentsExport.h"

class pqOutputPort;
class pqTreeWidgetItemObject;
class QPixmap;
class QTreeWidget;
class QTreeWidgetItem;
class vtkPVArrayInformation;
class vtkSMProperty;

class PQCOMPONENTS_EXPORT pqExodusIIPanel :
  public pqNamedObjectPanel
{
  Q_OBJECT
  typedef pqNamedObjectPanel Superclass;
public:
  /// constructor
  pqExodusIIPanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~pqExodusIIPanel();

protected slots:
  void applyDisplacements(int);
  void displChanged(bool);

  void modeChanged(int);

  // When the "Refresh" button is pressed, call a method on the reader
  // which will cause the reader's TIME_STEPS() and TIME_RANGE() output
  // information keys to update. Then "pull" their values from the server
  // in order to update the pqTimeKeeper.
  void onRefresh();

  /// Gets the SIL from the reader and updates the GUI.
  void updateSIL();

  /// Called when the global selection changes. We check if the newly created
  /// selection is a block-based selection on the data produced by the reader.
  /// If so, we enable the "Check Selected", "Uncheck Selected" buttons which
  /// make it possible for the user to quickly select blocks to read/not read.
  /// BUG #8281.
  /// For this to work, it requires that the pqSelectionManager is registered
  /// with the pqApplicationCore as "SelectionManager".
  void onSelectionChanged(pqOutputPort*);

  /// Slots called when the corresponding buttons are pressed.
  void setSelectedBlocksCheckState(bool check=true);
  void uncheckSelectedBlocks()
    { this->setSelectedBlocksCheckState(false); }
protected:
  
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  pqTreeWidgetItemObject* DisplItem;

  class pqUI;
  pqUI* UI;

};

#endif

