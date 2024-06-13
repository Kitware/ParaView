// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEditColorMapReaction_h
#define pqEditColorMapReaction_h

#include "pqReaction.h"
#include <QPointer> // For QPointer

#include <vtkNew.h> // For vtkNew

class pqPipelineRepresentation;
class pqDataRepresentation;
class vtkSMColorMapEditorHelper;

/**
 * @ingroup Reactions
 * Reaction to edit the active representation's color map or solid color.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditColorMapReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqEditColorMapReaction(QAction* parent, bool track_active_objects = true);
  ~pqEditColorMapReaction() override;

  /**
   * Edit active representation's color map (or solid color).
   */
  void editColorMap(pqPipelineRepresentation* repr = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

  ///@{
  /**
   * Set the active representation.
   */
  void setRepresentation(pqDataRepresentation*, int selectedPropertiesType);
  void setRepresentation(pqDataRepresentation* repr)
  {
    this->setRepresentation(repr, 0 /*Representation*/);
  }
  void setActiveRepresentation();
  ///@}

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEditColorMapReaction)
  QPointer<pqPipelineRepresentation> Representation;

  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;
};

#endif
