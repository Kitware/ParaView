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

class PQCOMPONENTS_EXPORT pqMultiBlockInspectorPanel : public QWidget
{
  Q_OBJECT

public:
  pqMultiBlockInspectorPanel(QWidget *parent = 0);
  ~pqMultiBlockInspectorPanel();

  pqOutputPort* getOutputPort() const;
  pqRepresentation* getRepresentation() const;

public slots:
  void setOutputPort(pqOutputPort *port);
  void setRepresentation(pqRepresentation *representation);
  void updateInformation();

private slots:
  void treeWidgetCustomContextMenuRequested(const QPoint &pos);
  void toggleBlockVisibility(QAction *action);
  void blockItemChanged(QTreeWidgetItem *item, int column);

private:
  Q_DISABLE_COPY(pqMultiBlockInspectorPanel)

  void setBlockVisibility(unsigned int index, bool visible);
  void clearBlockVisibility(unsigned int index);
  void updateBlockVisibilities();
  void updateBlockVisibilities(vtkPVCompositeDataInformation *iter,
                               QTreeWidgetItem *parent,
                               unsigned int &flatIndex,
                               bool visibility);
  void buildTree(vtkPVCompositeDataInformation *iter,
                 QTreeWidgetItem *parent,
                 unsigned int &flatIndex);
  void unsetChildVisibilities(QTreeWidgetItem *parent);

private:
  QTreeWidget *TreeWidget;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqRepresentation> Representation;
  QMap<unsigned int, bool> BlockVisibilites;
};

#endif // __pqMultiBlockInspectorPanel_h
