// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqScalarBarVisibilityReaction_h
#define pqScalarBarVisibilityReaction_h

#include "pqReaction.h"
#include <QPointer>

#include "vtkNew.h" // For vtkNew

#include <vector> // For std::vector

class pqDataRepresentation;
class pqTimer;
class pqView;
class vtkSMColorMapEditorHelper;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Reaction to toggle scalar bar visibility.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqScalarBarVisibilityReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqScalarBarVisibilityReaction(QAction* parent, bool track_active_objects = true);
  ~pqScalarBarVisibilityReaction() override;

  /**
   * Returns the representation currently being used by the reaction.
   */
  pqDataRepresentation* representation() const;

  ///@{
  /**
   * Returns the scalar bar for the current representation, if any.
   */
  std::vector<vtkSMProxy*> scalarBarProxies() const;
  vtkSMProxy* scalarBarProxy() const
  {
    std::vector<vtkSMProxy*> proxies = this->scalarBarProxies();
    return proxies.empty() ? nullptr : proxies[0];
  }
  ///@}

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  ///@{
  /**
   * Set the representation.
   */
  void setRepresentation(pqDataRepresentation*, int selectedPropertiesType);
  void setRepresentation(pqDataRepresentation* repr)
  {
    this->setRepresentation(repr, 0 /*Representation*/);
  }
  void setActiveRepresentation();
  ///@}

  /**
   * set scalar bar visibility.
   */
  void setScalarBarVisibility(bool visible);

  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { this->setScalarBarVisibility(this->parentAction()->isChecked()); }

private:
  Q_DISABLE_COPY(pqScalarBarVisibilityReaction)

  QPointer<pqDataRepresentation> Representation;
  QPointer<pqView> View;
  QPointer<pqTimer> Timer;
  vtkNew<vtkSMColorMapEditorHelper> ColorMapEditorHelper;
};

#endif
