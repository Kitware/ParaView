/*=========================================================================

   Program: ParaView
   Module:  pqPresetDialog.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

  pqPresetDialog(QWidget* parent = 0, Modes mode = SHOW_ALL);
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
  * Returns true if the user requested to load colors for the
  * current preset.
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
  * Set when user can choose to load opacities along with the default state.
  */
  void setCustomizableLoadOpacities(bool state, bool defaultValue = true);

  /**
  * Set when user can choose to load usePresetRange along with the default state.
  */
  void setCustomizableUsePresetRange(bool state, bool defaultValue = false);

signals:
  void applyPreset(const Json::Value& preset);

protected slots:
  void updateEnabledStateForSelection();
  void updateForSelectedIndex(const QModelIndex& proxyIndex);
  void triggerApply(const QModelIndex& proxyIndex = QModelIndex());
  void removePreset(const QModelIndex& idx = QModelIndex());
  void importPresets();
  void exportPresets();
  void onRejected();

  void setPresetIsAdvanced(int newState);

protected:
  void showEvent(QShowEvent* e) override;
  void closeEvent(QCloseEvent* e) override;

private slots:
  void updateGroups();

private:
  Q_DISABLE_COPY(pqPresetDialog)
  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
