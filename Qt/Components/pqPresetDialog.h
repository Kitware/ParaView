// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPresetDialog_h
#define pqPresetDialog_h

#include "pqComponentsModule.h" // for exports
#include <QDialog>
#include <QModelIndex>
#include <QScopedPointer>    // for QScopedPointer
#include <vtk_jsoncpp_fwd.h> // for forward declarations

class QModelIndex;

/**
 * pqPresetDialog is the dialog used by to show the user with a choice of color
 * maps/opacity maps/presets to choose from. The Dialog can be customized to
 * show only indexed (or non-indexed) color maps using pqPresetDialog::Modes.
 * Application code should observe the pqPresetDialog::applyPreset() signal to
 * perform the applying of the preset to a specific transfer function proxy.
 * This class works with vtkSMTransferFunctionPresets, which acts as the preset
 * manager for the application with support to inspect existing presets as well
 * as updating them.
 */
class PQCOMPONENTS_EXPORT pqPresetDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;

public:
  /**
   * Used to control what kinds of presets are shown in the dialog.
   * This merely affects the presets that are hidden from the view.
   */
  enum Modes
  {
    SHOW_ALL,
    SHOW_INDEXED_COLORS_ONLY,
    SHOW_NON_INDEXED_COLORS_ONLY
  };

  pqPresetDialog(QWidget* parent = nullptr, Modes mode = SHOW_ALL);
  ~pqPresetDialog() override;

  /**
   * Set the current mode (which type of presets are displayed)
   */
  void setMode(Modes mode);

  /**
   * Set the current preset using its name.
   */
  void setCurrentPreset(const char* presetName);

  /**
   * Return current preset, if any.
   */
  const Json::Value& currentPreset();

  /**
   * Returns true if the user requested to load colors for the current preset.
   */
  bool loadColors() const;

  /**
   * Returns true if the user requested to load opacities for the current
   * preset.
   */
  bool loadOpacities() const;

  /**
   * Returns true if the user requested to load annotations for the current
   * preset.
   */
  bool loadAnnotations() const;

  /**
   * Returns the specified regularExpression.
   */
  QRegularExpression regularExpression();

  /**
   * Returns true if the user requested to preserve/use the preset data range.
   * If false, the user is expecting the current transfer function range to be
   * maintained.
   */
  bool usePresetRange() const;

  /**
   * Set when user can choose to load colors along with the default state.
   */
  void setCustomizableLoadColors(bool state, bool defaultValue = true);

  /**
   * Set when user can choose to load annotations along with the default state.
   */
  void setCustomizableLoadAnnotations(bool state, bool defaultValue = true);

  /**
   * Set when user can choose a regexp to load annotations along with the default state.
   */
  void setCustomizableAnnotationsRegexp(bool state, bool defaultValue = false);

  /**
   * Set when user can choose to load opacities along with the default state.
   */
  void setCustomizableLoadOpacities(bool state, bool defaultValue = true);

  /**
   * Set when user can choose to load usePresetRange along with the default state.
   */
  void setCustomizableUsePresetRange(bool state, bool defaultValue = false);

Q_SIGNALS:
  void applyPreset(const Json::Value& preset);

protected Q_SLOTS:
  void updateEnabledStateForSelection();
  void updateForSelectedIndex(const QModelIndex& proxyIndex);
  void triggerApply(const QModelIndex& proxyIndex = QModelIndex());
  void removePreset(const QModelIndex& idx = QModelIndex());
  void importPresets();
  void exportPresets();

  void setPresetIsAdvanced(int newState);

private Q_SLOTS:
  void updateGroups();
  void updateFiltering();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqPresetDialog)
  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
