// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLinkViewWidget_h
#define pqLinkViewWidget_h

#include <QWidget>
#include <pqCoreModule.h>

class pqRenderView;
class QCheckBox;
class QLineEdit;

/**
 * a popup window that helps the user select another view to link with
 */
class PQCORE_EXPORT pqLinkViewWidget : public QWidget
{
  Q_OBJECT
public:
  /**
   * constructor takes the first view
   */
  pqLinkViewWidget(pqRenderView* firstLink);
  /**
   * destructor
   */
  ~pqLinkViewWidget() override;

protected:
  /**
   * event filter to monitor user's selection
   */
  bool eventFilter(QObject* watched, QEvent* e) override;
  /**
   * watch internal events
   */
  bool event(QEvent* e) override;

private:
  pqRenderView* RenderView = nullptr;
  QLineEdit* LineEdit = nullptr;
  QCheckBox* InteractiveViewLinkCheckBox = nullptr;
  QCheckBox* CameraWidgetViewLinkCheckBox = nullptr;
};

#endif
