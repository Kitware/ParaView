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

#ifndef pqMultiBlockInspectorPanel_h
#define pqMultiBlockInspectorPanel_h

#include "pqComponentsModule.h"

#include "pqTimer.h"   // needed for pqTimer.
#include "vtkSetGet.h" // for VTK_LEGACY
#include <QIcon>
#include <QMap>
#include <QPointer>
#include <QWidget>

#include <iostream>

#if !defined(VTK_LEGACY_REMOVE)
class QModelIndex;
class QTreeWidget;
class QTreeWidgetItem;

class pqOutputPort;
class pqRepresentation;
class vtkDiscretizableColorTransferFunction;
class vtkEventQtSlotConnect;
class vtkPiecewiseFunction;
class vtkPVCompositeDataInformation;
class vtkSMProxy;

/**
 * @class pqMultiBlockInspectorPanel
 * @brief deprecated panel to view multiblock structure for a dataset.
 *
 * @deprecated in ParaView 5.4 and replaced by pqMultiBlockInspectorWidget.
 *
 * pqMultiBlockInspectorPanel has been deprecated and replaced by
 * pqMultiBlockInspectorWidget. Please update to use pqMultiBlockInspectorWidget
 * in your application instead. This class will be removed in future releases.
 */
class PQCOMPONENTS_EXPORT pqMultiBlockInspectorPanel : public QWidget
{
  Q_OBJECT

public:
  VTK_LEGACY(pqMultiBlockInspectorPanel(QWidget* parent = 0));
  ~pqMultiBlockInspectorPanel();

  pqOutputPort* getOutputPort() const;
  pqRepresentation* getRepresentation() const;

  QString lookupBlockName(unsigned int flatIndex) const;

public slots:
  /**
  * ParaView events
  */
  void onPortChanged(pqOutputPort* port);
  void onRepresentationChanged(pqRepresentation* representation);
  void onDataUpdated();

  void setBlockVisibility(unsigned int index, bool visible);
  void clearBlockVisibility(unsigned int index);
  void setBlockVisibility(const QList<unsigned int>& indices, bool visible);
  void clearBlockVisibility(const QList<unsigned int>& indices);

  void setBlockColor(unsigned int index, const QColor& color);
  void clearBlockColor(unsigned int index);
  void setBlockColor(const QList<unsigned int>& indices, const QColor& color);
  void clearBlockColor(const QList<unsigned int>& indices);

  void setBlockOpacity(unsigned int index, double opacity);
  void clearBlockOpacity(unsigned int index);
  void setBlockOpacity(const QList<unsigned int>& indices, double opacity);
  void clearBlockOpacity(const QList<unsigned int>& indices);

  void promptAndSetBlockOpacity(unsigned int index);
  void promptAndSetBlockOpacity(const QList<unsigned int>& indices);

  void showOnlyBlock(unsigned int index);
  void showOnlyBlocks(const QList<unsigned int>& indices);
  void showAllBlocks();

private slots:
  /**
  * ParaView events
  */
  void onSelectionChanged(pqOutputPort* port);
  void onColorArrayNameModified();

  void onCustomContextMenuRequested(const QPoint& pos);
  void onItemChanged(QTreeWidgetItem* item, int column);
  void updateTree();
  void updateTree(vtkPVCompositeDataInformation* iter, QTreeWidgetItem* parent, int& flatIndex,
    bool visibility, int inheritedColorIndex, int inheritedOpacityIndex);
  void onItemSelectionChanged();
  void onItemDoubleClicked(QTreeWidgetItem* item, int column);
  void updateBlockVisibilities();
  void updateBlockColors();
  void updateBlockOpacities();

private:
  Q_DISABLE_COPY(pqMultiBlockInspectorPanel)

  enum NodeType
  {
    INTERNAL_NODE,
    LEAF_NODE
  };

  void buildTree(vtkPVCompositeDataInformation* iter, QTreeWidgetItem* parent,
    int& flatIndex,  // composite-data id
    int& leafIndex); // leaf-only index.
  void unsetChildVisibilities(QTreeWidgetItem* parent);
  QIcon makeColorIcon(
    int flatIndex, NodeType nodeType, int inheritedColorIndex, int leafIndex = -1) const;
  QIcon makeOpacityIcon(int flatIndex, NodeType nodeType, int inheritedOpacityIndex) const;
  QIcon makeNullIcon() const;

private:
  QTreeWidget* TreeWidget;
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqRepresentation> Representation;
  QMap<unsigned int, bool> BlockVisibilites;
  QMap<unsigned int, QColor> BlockColors;
  QMap<unsigned int, double> BlockOpacities;
  vtkEventQtSlotConnect* PropertyListener;
  vtkSMProxy* ColorTransferProxy;
  vtkDiscretizableColorTransferFunction* ColorTransferFunction;
  vtkPiecewiseFunction* OpacityTransferFunction;
  unsigned int BlockColorsDistinctValues;
  int CompositeWrap;

  pqTimer UpdateUITimer;

  struct BlockIcon
  {
    bool HasColor;
    bool HasOpacity;
    QColor Color;
    double Opacity;

    bool operator<(const BlockIcon& other) const
    {
      QColor c = this->HasColor ? this->Color : QColor();
      c.setAlphaF(this->HasOpacity ? this->Opacity : 1.0);

      QColor oc = other.HasColor ? other.Color : QColor();
      oc.setAlphaF(other.HasOpacity ? other.Opacity : 1.0);

      return c.rgba() < oc.rgba();
    }
  };
};

#endif // !defined(VTK_LEGACY_REMOVE)
#endif // pqMultiBlockInspectorPanel_h
