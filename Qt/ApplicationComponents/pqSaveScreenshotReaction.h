/*=========================================================================

   Program: ParaView
   Module:    pqSaveScreenshotReaction.h

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
  static void saveScreenshot(bool clipboardMode = false);

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
    vtkSMSaveScreenshotProxy* saveProxy, const QString& defaultExtension);

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
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
