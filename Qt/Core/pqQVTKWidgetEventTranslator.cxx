/*=========================================================================

   Program: ParaView
   Module:    pqQVTKWidgetEventTranslator.cxx

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

#include "pqQVTKWidgetEventTranslator.h"
#include "pqApplicationCore.h"
#include "pqCoreTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqEventTypes.h"
#include "pqFileDialog.h"

#include "vtkRenderWindow.h"

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLWidget.h"
#include "QVTKOpenGLWindow.h"

pqQVTKWidgetEventTranslator::pqQVTKWidgetEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

pqQVTKWidgetEventTranslator::~pqQVTKWidgetEventTranslator()
{
}

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

  if (QVTKOpenGLWidget* const qvtkWidget = qobject_cast<QVTKOpenGLWidget*>(Object))
  {
    rw = qvtkWidget->embeddedOpenGLWindow() ? qvtkWidget->renderWindow() : nullptr;
  }

  if (QVTKOpenGLNativeWidget* const qvtkNativeWidget =
        qobject_cast<QVTKOpenGLNativeWidget*>(Object))
  {
    rw = qvtkNativeWidget->renderWindow();
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
          double normalized_x = mouseEvent->x() / static_cast<double>(size.width());
          double normalized_y = mouseEvent->y() / static_cast<double>(size.height());
          emit recordEvent(widget, "mousePress", QString("(%1,%2,%3,%4,%5)")
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
          double normalized_x = mouseEvent->x() / static_cast<double>(size.width());
          double normalized_y = mouseEvent->y() / static_cast<double>(size.height());
          // Move to the place where the mouse was released and then release it.
          // This mimics drag without actually having to save all the intermediate
          // mouse move positions.
          emit recordEvent(widget, "mouseMove", QString("(%1,%2,%3,%4,%5)")
                                                  .arg(normalized_x)
                                                  .arg(normalized_y)
                                                  .arg(mouseEvent->button())
                                                  .arg(mouseEvent->buttons())
                                                  .arg(mouseEvent->modifiers()));
          emit recordEvent(widget, "mouseRelease", QString("(%1,%2,%3,%4,%5)")
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
        emit recordEvent(widget, "keyEvent", data);
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
      pqFileDialog file_dialog(
        NULL, pqCoreUtilities::mainWidget(), tr("Save Screenshot:"), baselineDir.path(), filters);
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
      emit recordEvent(Object, pqCoreTestUtility::PQ_COMPAREVIEW_PROPERTY_NAME,
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
