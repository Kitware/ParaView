// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqQVTKWidgetEventTranslator.h"
#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqEventTypes.h"
#include "pqFileDialog.h"
#include "pqQVTKWidget.h"

#include "vtkRenderWindow.h"

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLStereoWidget.h"

pqQVTKWidgetEventTranslator::pqQVTKWidgetEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

pqQVTKWidgetEventTranslator::~pqQVTKWidgetEventTranslator() = default;

bool pqQVTKWidgetEventTranslator::translateEvent(
  QObject* Object, QEvent* Event, int eventType, bool& error)
{
  // Only translate events for QWidget subclasses that internally use a render
  // window
  QWidget* const widget = qobject_cast<QWidget*>(Object);
  if (!widget)
  {
    return false;
  }

  // Look for a render window in the possible widget types.
  vtkRenderWindow* rw = nullptr;

  if (QVTKOpenGLStereoWidget* const qvtkWidget = qobject_cast<QVTKOpenGLStereoWidget*>(Object))
  {
    rw = qvtkWidget->embeddedOpenGLWindow() ? qvtkWidget->renderWindow() : nullptr;
  }

  if (QVTKOpenGLNativeWidget* const qvtkNativeWidget =
        qobject_cast<QVTKOpenGLNativeWidget*>(Object))
  {
    rw = qvtkNativeWidget->renderWindow();
  }

  if (pqQVTKWidget* const qvtkWidget = qobject_cast<pqQVTKWidget*>(Object))
  {
    rw = qvtkWidget->renderWindow();
  }

  // Could not find a render window, don't translate the event
  if (rw == nullptr)
  {
    return false;
  }

  if (eventType == pqEventTypes::ACTION_EVENT)
  {
    switch (Event->type())
    {
      // ContextMenu are supported via mousePress
      case QEvent::ContextMenu:
      {
        return true;
        break;
      }
      case QEvent::MouseButtonPress:
      {
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(Event);
        if (mouseEvent)
        {
          QSize size = widget->size();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
          auto pos = mouseEvent->localPos();
#else
          auto pos = mouseEvent->position();
#endif
          double normalized_x = pos.x() / static_cast<double>(size.width());
          double normalized_y = pos.y() / static_cast<double>(size.height());
          Q_EMIT recordEvent(widget, "mousePress",
            QString("(%1,%2,%3,%4,%5)")
              .arg(normalized_x)
              .arg(normalized_y)
              .arg(mouseEvent->button())
              .arg(mouseEvent->buttons())
              .arg(mouseEvent->modifiers()));
        }
        return true;
        break;
      }

      case QEvent::MouseButtonRelease:
      {
        QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(Event);
        if (mouseEvent)
        {
          QSize size = widget->size();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
          auto pos = mouseEvent->localPos();
#else
          auto pos = mouseEvent->position();
#endif
          double normalized_x = pos.x() / static_cast<double>(size.width());
          double normalized_y = pos.y() / static_cast<double>(size.height());
          // Move to the place where the mouse was released and then release it.
          // This mimics drag without actually having to save all the intermediate
          // mouse move positions.
          Q_EMIT recordEvent(widget, "mouseMove",
            QString("(%1,%2,%3,%4,%5)")
              .arg(normalized_x)
              .arg(normalized_y)
              .arg(mouseEvent->button())
              .arg(mouseEvent->buttons())
              .arg(mouseEvent->modifiers()));
          Q_EMIT recordEvent(widget, "mouseRelease",
            QString("(%1,%2,%3,%4,%5)")
              .arg(normalized_x)
              .arg(normalized_y)
              .arg(mouseEvent->button())
              .arg(mouseEvent->buttons())
              .arg(mouseEvent->modifiers()));
        }
        return true;
        break;
      }

      case QEvent::KeyPress:
      case QEvent::KeyRelease:
      {
        QKeyEvent* ke = static_cast<QKeyEvent*>(Event);
        QString data = QString("%1:%2:%3:%4:%5:%6")
                         .arg(ke->type())
                         .arg(ke->key())
                         .arg(static_cast<int>(ke->modifiers()))
                         .arg(ke->text())
                         .arg(ke->isAutoRepeat())
                         .arg(ke->count());
        Q_EMIT recordEvent(widget, "keyEvent", data);
        return true;
        break;
      }

      default:
      {
        break;
      }
    }
  }
  else if (eventType == pqEventTypes::CHECK_EVENT)
  {
    if (Event->type() == QEvent::MouseButtonRelease)
    {
      // Dir to save the image in
      QDir baselineDir(pqCoreTestUtility::BaselineDirectory());

      // Resize widget to 300x300
      int width = 300, height = 300;
      QSize oldSize = widget->size();
      QSize oldMaxSize = widget->maximumSize();
      widget->setMaximumSize(width, height);
      widget->resize(width, height);

      // Setup File Save Dialog
      QString filters;
      filters += "PNG image (*.png)";
      filters += ";;BMP image (*.bmp)";
      filters += ";;TIFF image (*.tif)";
      filters += ";;PPM image (*.ppm)";
      filters += ";;JPG image (*.jpg)";
      pqFileDialog file_dialog(nullptr, pqCoreUtilities::mainWidget(), tr("Save Screenshot:"),
        baselineDir.path(), filters, false);
      file_dialog.setObjectName("FileSaveScreenshotDialog");
      file_dialog.setFileMode(pqFileDialog::AnyFile);

      // Pause recording whilie selecting file to save
      pqTestUtility* testUtil = pqApplicationCore::instance()->testUtility();
      testUtil->pauseRecords(true);

      // Execute file save dialog
      if (file_dialog.exec() != QDialog::Accepted)
      {
        error = false;
        testUtil->pauseRecords(false);
        widget->setMaximumSize(oldMaxSize);
        widget->resize(oldSize);
        return true;
      }
      QString file = file_dialog.getSelectedFiles()[0];

      // Save screenshot
      int offRen = rw->GetOffScreenRendering();
      rw->SetOffScreenRendering(1);
      pqCoreTestUtility::SaveScreenshot(rw, file);
      rw->SetOffScreenRendering(offRen);

      // Get a relative to saved file
      QString relPathFile = baselineDir.relativeFilePath(file);

      // Restore recording
      testUtil->pauseRecords(false);

      // Restore widget size
      widget->setMaximumSize(oldMaxSize);
      widget->resize(oldSize);

      // Emit record signal
      Q_EMIT recordEvent(Object, pqCoreTestUtility::PQ_COMPAREVIEW_PROPERTY_NAME,
        "$PARAVIEW_TEST_BASELINE_DIR/" + relPathFile, pqEventTypes::CHECK_EVENT);
      return true;
    }
    if (Event->type() == QEvent::MouseMove)
    {
      return true;
    }
  }
  return this->Superclass::translateEvent(Object, Event, eventType, error);
}
