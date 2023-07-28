// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionEditor_h
#define pqSelectionEditor_h

#include "pqApplicationComponentsModule.h"
#include <QWidget>

class QItemSelection;
class pqDataRepresentation;
class pqOutputPort;
class pqServer;
class pqPipelineSource;
class pqView;

/**
 * @brief pqSelectionEditor is a widget to combine multiple selections of different types.
 *
 * This widget allows you to add the active selection of a specific dataset to a list of saved
 * selections. Saved selections can be added and/or removed. The relation of the saved
 * selections can be defined using a boolean expression. The combined selection can be set as
 * the active selection of a specific dataset.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSelectionEditor : public QWidget
{
  Q_OBJECT
  using Superclass = QWidget;

public:
  pqSelectionEditor(QWidget* parent = nullptr);
  ~pqSelectionEditor() override;

private Q_SLOTS:
  void onActiveServerChanged(pqServer* server);
  void onActiveViewChanged(pqView*);
  void onAboutToRemoveSource(pqPipelineSource*);
  void onActiveSourceChanged(pqPipelineSource* source);
  void onVisibilityChanged(pqPipelineSource*, pqDataRepresentation*);
  void onSelectionChanged(pqOutputPort*);
  void onExpressionChanged(const QString&);
  void onTableSelectionChanged(const QItemSelection&, const QItemSelection&);
  void onAddActiveSelection();
  void onRemoveSelectedSelection();
  void onRemoveAllSelections();
  void onActivateCombinedSelections();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSelectionEditor);

  void removeAllSelections(int elementType);
  void clearInteractiveSelection();
  void showInteractiveSelection(unsigned int row);
  void hideInteractiveSelection();

  class pqInternal;
  pqInternal* Internal;
};

#endif
