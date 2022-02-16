/*=========================================================================

   Program: ParaView
   Module:  pqSelectionEditor.h

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

========================================================================*/
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
