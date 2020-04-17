/*=========================================================================

   Program: ParaView
   Module:    pqProgressWidget.h

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

=========================================================================*/
#ifndef pqProgressWidget_h
#define pqProgressWidget_h

#include <QScopedPointer>
#include <QWidget>

#include "pqWidgetsModule.h"

class QToolButton;
class pqProgressWidgetLabel;

/**
 * @class pqProgressWidget
 * @brief widget to show progress in a QStatusBar.
 *
 * pqProgressWidget is a widget designed to be used in the QStatusBar of the
 * application to show progress for time consuming tasks in the application.
 *
 * pqProgressWidget is a replacement for QProgressBar. It has the following
 * differences with QProgressBar.
 *
 * \li 1. It adds support for an abort button that can be enabled to allow
 *        aborting for interruptible processing while progress in active.
 * \li 2. It does not use QMacStyle on OsX instead uses "fusion" or "cleanlooks"
 *        show that the text is shown on the progress bar.
 * \li 3. It does not render progress bar grove for a more "flat" look and avoid
 *        dramatic UI change when toggling between showing progress and not.
 */
class PQWIDGETS_EXPORT pqProgressWidget : public QWidget
{
  Q_OBJECT;
  typedef QWidget Superclass;
  Q_PROPERTY(QString readyText READ readyText WRITE setReadyText)
  Q_PROPERTY(QString busyText READ busyText WRITE setBusyText)
public:
  pqProgressWidget(QWidget* parent = 0);
  ~pqProgressWidget() override;

  /**
   * @deprecated in ParaView 5.5. Use `abortButton` instead.
   */
  QToolButton* getAbortButton() const { return this->AbortButton; }

  /**
   * Provides access to the abort button.
   */
  QToolButton* abortButton() const { return this->AbortButton; }

  //@{
  /**
   * Set the text to use by default when the progress bar is not enabled
   * which typically corresponds to application not being busy.
   * Default value is empty.
   */
  void setReadyText(const QString&);
  const QString& readyText() const { return this->ReadyText; }
  //@}

  //@{
  /**
   * Set the text to use by default when the progress bar is enabled
   * which typically corresponds to application being busy.
   * Default value is "Busy".
   */
  void setBusyText(const QString&);
  const QString& busyText() const { return this->BusyText; }
  //@}

public Q_SLOTS:
  /**
   * Set the progress. Progress must be enabled by calling 'enableProgress`
   * otherwise this method will have no effect.
   */
  void setProgress(const QString& message, int value);

  /**
   * Enabled/disable the progress. This is different from
   * enabling/disabling the widget itself. This shows/hides
   * the progress part of the widget.
   */
  void enableProgress(bool enabled);

  /**
   * Enable/disable the abort button.
   */
  void enableAbort(bool enabled);

Q_SIGNALS:
  /**
   * triggered with the abort button is pressed.
   */
  void abortPressed();

protected:
  pqProgressWidgetLabel* ProgressBar;
  QToolButton* AbortButton;

  /**
   * request Qt to paint the widgets.
   */
  void updateUI();

private:
  Q_DISABLE_COPY(pqProgressWidget);
  QString ReadyText;
  QString BusyText;
};

#endif
