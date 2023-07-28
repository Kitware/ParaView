// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSaveScreenshotReaction_h
#define pqSaveScreenshotReaction_h

#include "pqReaction.h"

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

class vtkSMSaveScreenshotProxy;

/**
 * @ingroup Reactions
 * @class pqSaveScreenshotReaction
 * @brief Reaction to save a screenshot.
 *
 * pqSaveScreenshotReaction can be connected to a QAction to prompt user and save
 * screenshots or captures from views using the `("misc", "SaveScreenshot")` proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveScreenshotReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
   * Constructor. Parent cannot be nullptr.
   */
  pqSaveScreenshotReaction(QAction* parent, bool clipboardMode = false);

  /**
   * Saves the screenshot.
   * Note that this method is static. Applications can simply use this without
   * having to create a reaction instance.
   * If clipboardMode is true, no advanced options are requested and the
   * screenshot is copied to the clipboard
   */
  static bool saveScreenshot(bool clipboardMode = false);

  /**
   * Save a screenshot given the filename and image properties.
   * This method is provided only for convenience. This doesn't expose any of the
   * advanced options available to users when saving screenshots.
   */
  static bool saveScreenshot(
    const QString& filename, const QSize& size, int quality, bool all_views = false);

  /**
   * Copy a screenshot to the clipboard.
   */
  static bool copyScreenshotToClipboard(const QSize& size, bool all_views = false);

  /**
   * Take a screenshot of the active view or all the views at the given size
   * and return it as a vtkImageData.
   */
  static vtkSmartPointer<vtkImageData> takeScreenshot(const QSize& size, bool all_views = false);

  /**
   * Prompt user for filename using the various format proxies listed for the
   * given proxy.
   */
  static QString promptFileName(
    vtkSMSaveScreenshotProxy* saveProxy, const QString& defaultExtension, vtkTypeUInt32& location);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override { pqSaveScreenshotReaction::saveScreenshot(this->ClipboardMode); }

  bool ClipboardMode;

private:
  Q_DISABLE_COPY(pqSaveScreenshotReaction)
};

#endif
