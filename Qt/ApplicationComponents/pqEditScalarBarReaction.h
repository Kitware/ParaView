// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqEditScalarBarReaction_h
#define pqEditScalarBarReaction_h

#include "pqReaction.h"

#include <QPointer>

class pqDataRepresentation;
class pqScalarBarVisibilityReaction;

/**
 * @ingroup Reactions
 * Reaction to allow editing of scalar bar properties using a
 * pqProxyWidgetDialog.
 *
 * Reaction allows editing of scalar bar properties using a
 * pqProxyWidgetDialog. Internally, it uses pqScalarBarVisibilityReaction to
 * track the visibility state for the scalar to enable/disable the parent
 * action.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqEditScalarBarReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqEditScalarBarReaction(QAction* parent = nullptr, bool track_active_objects = true);
  ~pqEditScalarBarReaction() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Set the active representation. This should only be used when
   * \c track_active_objects is false. If used when \c track_active_objects is
   * true, the representation will get replaced whenever the active
   * representation changes.
   */
  void setRepresentation(pqDataRepresentation*, int selectedPropertiesType);
  void setRepresentation(pqDataRepresentation* repr)
  {
    this->setRepresentation(repr, 0 /*Representation*/);
  }
  ///@}

  /**
   * Set Scalar Bar Visibility Reaction
   */
  void setScalarBarVisibilityReaction(pqScalarBarVisibilityReaction* reaction);

  /**
   * Show the editor dialog for editing scalar bar properties.
   */
  bool editScalarBar();

protected Q_SLOTS:
  /**
   * Updates the enabled state. Applications need not explicitly call
   * this.
   */
  void updateEnableState() override;

  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

private:
  Q_DISABLE_COPY(pqEditScalarBarReaction)

  QPointer<pqScalarBarVisibilityReaction> createDefaultScalarBarVisibilityReaction(
    bool track_active_objects);

  QPointer<pqScalarBarVisibilityReaction> SBVReaction;
};

#endif
