// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRescaleScalarRangeReaction_h
#define pqRescaleScalarRangeReaction_h

#include "pqReaction.h"
#include "vtkSmartPointer.h"

#include <QPointer>

class pqDataRepresentation;
class pqPipelineRepresentation;
class pqRescaleScalarRangeToCustomDialog;
class pqRescaleScalarRangeToDataOverTimeDialog;
class pqServer;
class vtkEventQtSlotConnect;
class vtkSMProxy;

/**
 * @ingroup Reactions
 * Reaction to rescale the active lookup table's range to match the active
 * representation. You can disable tracking of the active representation,
 * instead explicitly provide one using setRepresentation() by pass
 * track_active_objects as false to the constructor.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqRescaleScalarRangeReaction : public pqReaction
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
  pqRescaleScalarRangeReaction(
    QAction* parent, bool track_active_objects = true, Modes mode = DATA);
  ~pqRescaleScalarRangeReaction() override;

  /**
   * Rescale to current data range.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool rescaleScalarRangeToData(pqPipelineRepresentation* repr = nullptr);

  /**
   * Rescale range to a custom range.
   *
   * @param[in] repr The representation used to determine the transfer function
   *                 to change range on. If \c repr is `nullptr`, then the active
   *                 representation is used, if available.
   * @returns a pointer to the dialog if the operation was successful, otherwise `nullptr`.
   */
  static pqRescaleScalarRangeToCustomDialog* rescaleScalarRangeToCustom(
    pqPipelineRepresentation* repr = nullptr);

  /**
   * Rescale range to a custom range.
   *
   * @param[in] tfProxy The transfer function proxy to rescale the range on.
   * @param[in] separateOpacity Show controls for setting the opacity function range
   *                            separately from the color transfer function.
   *
   * @returns a pointer to the dialog if the operation was successful, otherwise `nullptr`.
   */
  static pqRescaleScalarRangeToCustomDialog* rescaleScalarRangeToCustom(
    vtkSMProxy* tfProxy, bool separateOpacity = false);

  /**
   * Rescale range to data range over time.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static pqRescaleScalarRangeToDataOverTimeDialog* rescaleScalarRangeToDataOverTime(
    pqPipelineRepresentation* repr = nullptr);

  /**
   * Rescale range to data range for data visible in the view.
   *
   * @param[in] repr The data representation to use to determine the data range.
   *                 If `nullptr`, then the active representation is used, if
   *                 available.
   * @returns `true` if the operation was successful, otherwise `false`.
   */
  static bool rescaleScalarRangeToVisible(pqPipelineRepresentation* repr = nullptr);

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
  Q_DISABLE_COPY(pqRescaleScalarRangeReaction)
  QPointer<pqPipelineRepresentation> Representation;
  Modes Mode;
  vtkSmartPointer<vtkEventQtSlotConnect> Connection;
};

#endif
