/*=========================================================================

   Program: ParaView
   Module:    pqExodusPanel.h

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

#ifndef _pqExodusPanel_h
#define _pqExodusPanel_h

#include "pqNamedObjectPanel.h"
#include "pqComponentsExport.h"

class pqTreeWidgetItemObject;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class vtkPVArrayInformation;

class PQCOMPONENTS_EXPORT pqExodusPanel :
  public pqNamedObjectPanel
{
  Q_OBJECT
  Q_PROPERTY(int displayType READ displayType WRITE setDisplayType)
public:
  /// constructor
  pqExodusPanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~pqExodusPanel();

  void reset();

signals:
  void displayTypeChanged();

protected slots:
  void setDisplayType(int);
  void applyDisplacements(int);
  void displChanged(bool);

  void modeChanged(int);

  void updateDataRanges();
  void propertyChanged();

  void blockItemChanged(QTreeWidgetItem*);
  void hierarchyItemChanged(QTreeWidgetItem*);
  void materialItemChanged(QTreeWidgetItem*);
  void selectionItemChanged(QTreeWidgetItem*, const QString&);
 
  void updatePendingChangedItems();
protected:
  int displayType() const;
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  static QString formatDataFor(vtkPVArrayInformation* ai);

  pqTreeWidgetItemObject* DisplItem;

  bool DataUpdateInProgress;

  class pqUI;
  pqUI* UI;

};

#endif

