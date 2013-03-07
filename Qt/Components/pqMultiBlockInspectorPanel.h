/*=========================================================================

  Program:   ParaView
  Module:    pqMultiBlockInspectorPanel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __pqMultiBlockInspectorPanel_h
#define __pqMultiBlockInspectorPanel_h

#include "pqComponentsModule.h"

#include <QMap>
#include <QWidget>
#include <QPointer>

class QModelIndex;
class QTreeWidget;
class QTreeWidgetItem;

class pqOutputPort;
class pqRepresentation;
class vtkPVCompositeDataInformation;
class vtkEventQtSlotConnect;

class PQCOMPONENTS_EXPORT pqMultiBlockInspectorPanel : public QWidget
{
  Q_OBJECT

public:
  pqMultiBlockInspectorPanel(QWidget *parent = 0);
  ~pqMultiBlockInspectorPanel();

  pqOutputPort* getOutputPort() const;
  pqRepresentation* getRepresentation() const;

  QString lookupBlockName(unsigned int flatIndex) const;

public slots:
  void setOutputPort(pqOutputPort *port);
  void setRepresentation(pqRepresentation *representation);
  void updateInformation();
  void setBlockVisibility(unsigned int index, bool visible);
  void clearBlockVisibility(unsigned int index);
  void showOnlyBlock(unsigned int index);

private slots:
  void treeWidgetCustomContextMenuRequested(const QPoint &pos);
  void toggleBlockVisibility(QAction *action);
  void blockItemChanged(QTreeWidgetItem *item, int column);
  void updateTreeWidgetBlockVisibilities();
  void updateTreeWidgetBlockVisibilities(
    vtkPVCompositeDataInformation *iter,
    QTreeWidgetItem *parent,
    unsigned int &flatIndex,
    bool visibility);
  void currentSelectionChanged(pqOutputPort *port);
  void currentTreeItemSelectionChanged();

private:
  Q_DISABLE_COPY(pqMultiBlockInspectorPanel)

  void updateBlockVisibilities();
  void buildTree(vtkPVCompositeDataInformation *iter,
                 QTreeWidgetItem *parent,
                 unsigned int &flatIndex);
  void unsetChildVisibilities(QTreeWidgetItem *parent);

private:
  QTreeWidget *TreeWidget;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqRepresentation> Representation;
  QMap<unsigned int, bool> BlockVisibilites;
  vtkEventQtSlotConnect *VisibilityPropertyListener;
};

#endif // __pqMultiBlockInspectorPanel_h
