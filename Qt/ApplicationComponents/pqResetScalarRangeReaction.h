// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqResetScalarRangeReaction_h
#define pqResetScalarRangeReaction_h

#include "pqReaction.h"
#include "vtkSmartPointer.h"

#include "vtkParaViewDeprecation.h" // for deprecation

#include <QPointer>

class pqPipelineRepresentation;
class pqDataRepresentation;
class pqServer;
class vtkEventQtSlotConnect;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Reaction to reset the active lookup table's range to match the active
 * representation. You can disable tracking of the active representation,
 * instead explicitly provide one using setRepresentation() by pass
 * track_active_objects as false to the constructor.
 */
class PARAVIEW_DEPRECATED_IN_5_12_0("Use pqRescaleScalarRangeReaction instead")
  PQAPPLICATIONCOMPONENTS_EXPORT pqResetScalarRangeReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  enum Modes
  {
    DATA,
    CUSTOM,
    TEMPORAL,
    VISIBLE
  };

  /**
   * if \c track_active_objects is false, then the reaction will not track
   * pqActiveObjects automatically.
   */
  pqResetScalarRangeReaction(QAction* parent, bool track_active_objects = true, Modes mode = DATA);
  ~pqResetScalarRangeReaction() override;

  /**
   * @deprecated Use resetScalarRangeToData().
   */
  PARAVIEW_DEPRECATED_IN_5_12_0("Use resetScalarRangeToData() instead")
  static void resetScalarRange(pqPipelineRepresentation* repr = nullptr)
  {
    pqResetScalarRangeReaction::resetScalarRangeToData(repr);
  }

  /**
   * Reset to current data range.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool resetScalarRangeToData(pqPipelineRepresentation* repr = nullptr);

  /**
   * Reset range to a custom range.
   *
   * @param[in] repr The representation used to determine the transfer function
   *                 to change range on. If \c repr is `nullptr`, then the active
   *                 representation is used, if available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool resetScalarRangeToCustom(pqPipelineRepresentation* repr = nullptr);

  /**
   * Reset range to a custom range.
   *
   * @param[in] tfProxy The transfer function proxy to reset the range on.
   * @param[in] separateOpacity Show controls for setting the opacity function range
   *                            separately from the color transfer function.
   *
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool resetScalarRangeToCustom(vtkSMProxy* tfProxy, bool separateOpacity = false);

  /**
   * Reset range to data range over time.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool resetScalarRangeToDataOverTime(pqPipelineRepresentation* repr = nullptr);

  /**
   * Reset range to data range for data visible in the view.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool resetScalarRangeToVisible(pqPipelineRepresentation* repr = nullptr);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

  /**
   * Set the data representation explicitly when track_active_objects is false.
   */
  void setRepresentation(pqDataRepresentation* repr);

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override;

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void onServerAdded(pqServer* server);
  virtual void onAboutToRemoveServer(pqServer* server);

private:
  Q_DISABLE_COPY(pqResetScalarRangeReaction)
  QPointer<pqPipelineRepresentation> Representation;
  Modes Mode;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
};

#endif
