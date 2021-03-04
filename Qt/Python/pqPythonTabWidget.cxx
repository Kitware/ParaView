/*=========================================================================

   Program: ParaView
   Module:    pqPythonTabWidget.h

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

#include "pqPythonTabWidget.h"

#include "pqCoreUtilities.h"
#include "pqPythonEditorActions.h"
#include "pqPythonTextArea.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabBar>

//-----------------------------------------------------------------------------
pqPythonTabWidget::pqPythonTabWidget(QWidget* parent)
  : QTabWidget(parent)
{
  this->addNewTabButton();
  this->createNewEmptyTab();
  this->setMovable(true);
  this->setTabsClosable(true);

  this->connect(this, &QTabWidget::tabCloseRequested, [this](int index) {
    pqPythonTextArea* widget = this->getWidget<pqPythonTextArea>(index);
    if (widget)
    {
      // First focus to the tab the user wants to delete
      // and update the QTabBar
      const int widgetId = this->indexOf(widget);
      this->setCurrentIndex(widgetId);
      this->repaint();

      // We stop all display updates
      // to avoid segfaults
      this->setUpdatesEnabled(false);
      if (this->count() == 2)
      {
        // Only delete the widget if it's not empty
        if (widget->isEmpty())
        {
          this->setUpdatesEnabled(true);
          return;
        }

        this->createNewEmptyTab();
      }

      if (widget->saveOnClose())
      {
        if (this->currentIndex() == index)
        {
          this->setCurrentIndex((index + 1) % (this->count() - 1));
          this->lastFocus = this->getWidget<pqPythonTextArea>(this->currentIndex());
        }

        this->removeTab(index);
        widget->deleteLater();
      }
      this->setUpdatesEnabled(true);
    }
  });
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::connectActions(pqPythonEditorActions& actions)
{
  pqPythonEditorActions::connect(actions, this);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::updateActions(pqPythonEditorActions& actions)
{
  if (this->lastFocus)
  {
    this->lastFocus->disconnectActions(actions);
  }

  this->lastFocus = this->getCurrentTextArea();
  this->lastFocus->connectActions(actions);
}

//-----------------------------------------------------------------------------
pqPythonTextArea* pqPythonTabWidget::getCurrentTextArea() const
{
  return this->getWidget<pqPythonTextArea>(this->currentIndex());
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::addNewTextArea(const QString& filename)
{
  // If the filename is empty abort
  if (filename.isEmpty())
  {
    return;
  }

  // We first check if this file is already opened
  const QFileInfo fileInfo(filename);
  for (int i = 0; i < this->count() - 1; ++i)
  {
    pqPythonTextArea* widget = this->getWidget<pqPythonTextArea>(i);
    if (widget && QFileInfo(widget->getFilename()) == fileInfo)
    {
      this->setCurrentIndex(i);
      return;
    }
  }

  if (!this->getCurrentTextArea()->isEmpty())
  {
    this->createNewEmptyTab();
  }

  // If not create a new tab
  pqPythonTextArea* tArea = this->getCurrentTextArea();
  tArea->openFile(filename);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::closeCurrentTab()
{
  this->tabCloseRequested(this->currentIndex());
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::keyPressEvent(QKeyEvent* keyEvent)
{
  if (keyEvent->matches(QKeySequence::AddTab))
  {
    this->createNewEmptyTab();
    keyEvent->accept();
  }
  else if (keyEvent->matches(QKeySequence::NextChild))
  {
    const int nextTabId = (this->currentIndex() + 1) % (this->count() - 1);
    this->setCurrentIndex(nextTabId);
  }
  else if (keyEvent->matches(QKeySequence::PreviousChild))
  {
    const int nextTabId = (this->currentIndex() + this->count() - 2) % (this->count() - 1);
    this->setCurrentIndex(nextTabId);
  }

  keyEvent->ignore();
}

void pqPythonTabWidget::mousePressEvent(QMouseEvent* mouseEvent)
{
  if (mouseEvent->button() == Qt::RightButton)
  {
    const QPoint& mousePosition = mouseEvent->pos();
    const int tabIndex = this->tabBar()->tabAt(mousePosition);

    if (tabIndex >= 0 && tabIndex < this->count())
    {
      QMenu contextMenu;

      QAction closeTabAction;
      closeTabAction.setText(tr("Close"));
      this->connect(&closeTabAction, &QAction::triggered,
        [this, tabIndex]() { this->tabCloseRequested(tabIndex); });
      contextMenu.addAction(&closeTabAction);

      contextMenu.exec(mouseEvent->globalPos());
    }

    mouseEvent->accept();
  }

  mouseEvent->ignore();
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::updateTab(QWidget* widget)
{
  const int idx = this->indexOf(widget);
  if (idx != -1)
  {
    this->tabBar()->setTabButton(
      idx, QTabBar::LeftSide, this->getTabLabel(reinterpret_cast<pqPythonTextArea*>(widget)));
  }
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::createNewEmptyTab()
{
  pqPythonTextArea* widget = new pqPythonTextArea(this);

  this->insertTab(this->count() - 1, widget, "");
  this->setCurrentIndex(this->indexOf(widget));
  this->lastFocus = widget;

  this->updateTab(widget);

  this->connect(widget, &pqPythonTextArea::fileOpened,
    [this, widget](const QString&) { this->updateTab(widget); });

  this->connect(widget, &pqPythonTextArea::fileSaved,
    [this, widget](const QString&) { this->updateTab(widget); });

  this->connect(widget, &pqPythonTextArea::fileSavedAsMacro,
    [this, widget](const QString&) { this->updateTab(widget); });

  this->connect(
    widget, &pqPythonTextArea::contentChanged, [this, widget]() { this->updateTab(widget); });

  this->connect(widget, &pqPythonTextArea::fileOpened, this, &pqPythonTabWidget::fileOpened);
  this->connect(widget, &pqPythonTextArea::fileSaved, this, &pqPythonTabWidget::fileSaved);
  this->connect(
    widget, &pqPythonTextArea::fileSavedAsMacro, this, &pqPythonTabWidget::fileSavedAsMacro);

  this->setTabCloseButton(widget);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::setTabCloseButton(pqPythonTextArea* widget)
{
  QCloseLabel* label = new QCloseLabel(widget, widget);
  this->tabBar()->setTabButton(this->count() - 2, QTabBar::RightSide, label);
  this->connect(label, &QCloseLabel::onClicked, [this](QWidget* widget) {
    int idx = this->indexOf(widget);
    if (idx >= 0 && idx < this->count() - 1)
    {
      this->tabBar()->tabCloseRequested(idx);
    }
  });
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::addNewTabButton()
{
  QLabel* label = new QLabel("+");
  this->addTab(new QLabel("Adds a new tab"), QString());
  this->tabBar()->setTabButton(0, QTabBar::RightSide, label);

  this->connect(this, &QTabWidget::tabBarClicked, [this](int index) {
    if (index == this->count() - 1)
    {
      this->createNewEmptyTab();
    }
  });
}

//-----------------------------------------------------------------------------
bool pqPythonTabWidget::saveOnClose()
{
  bool rValue = true;
  for (int i = 0; i < this->count() - 1; ++i)
  {
    pqPythonTextArea* widget = this->getWidget<pqPythonTextArea>(i);
    if (widget)
    {
      rValue &= widget->saveOnClose();
    }
  }

  return rValue;
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::createParaviewTraceTab()
{
  if (!this->TraceWidget)
  {
    if (!this->getCurrentTextArea()->isEmpty())
    {
      this->createNewEmptyTab();
    }
    this->TraceWidget = this->getCurrentTextArea();
  }
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::updateTrace(const QString& str)
{
  this->createParaviewTraceTab();

  const int traceWidgetId = this->indexOf(this->TraceWidget);
  this->tabBar()->setCurrentIndex(traceWidgetId);
  this->TraceWidget->setText(str);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::stopTrace(const QString& str)
{
  this->createParaviewTraceTab();

  const int traceWidgetId = this->indexOf(this->TraceWidget);
  this->tabBar()->setCurrentIndex(traceWidgetId);
  this->TraceWidget->setText(str);
  this->TraceWidget = nullptr;
  this->updateTab(this->widget(traceWidgetId));
}

//-----------------------------------------------------------------------------
QLabel* pqPythonTabWidget::getTabLabel(const pqPythonTextArea* widget)
{
  const auto GenerateTabName = [this, widget]() {
    const auto QColorToString = [](const QColor& color) { return color.name(QColor::HexRgb); };
    const auto ColorText = [&QColorToString](const QColor& color, const QString& text) {
      return "<span style=\"color:" + QColorToString(color) + "\">" + text + "</span>";
    };

    const QString& filename = widget->getFilename();
    const QFileInfo fileInfo(filename);
    const QString macroDir = pqCoreUtilities::getParaViewUserDirectory();

    QString tabname;

    // Insert a tag for the tab type
    if (widget == this->TraceWidget)
    {
      tabname = ColorText(Qt::GlobalColor::darkCyan, "[Trace] ");
    }
    else if (filename.contains(macroDir))
    {
      tabname = ColorText(Qt::GlobalColor::darkGreen, "[Macro] ");
    }

    // Insert the tab name (either an empty file, or an actual opened file)
    if (filename.isEmpty())
    {
      tabname += "New File";
    }
    else
    {
      tabname += ColorText(Qt::GlobalColor::gray, fileInfo.dir().absolutePath()) + "/";

      if (widget->isDirty())
      {
        tabname += "<b>" + ColorText(Qt::GlobalColor::red, fileInfo.fileName()) + "</b>";
      }
      else
      {
        tabname += fileInfo.fileName();
      }
    }

    // Replace the home folder path with ~
    const QString homeDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    tabname.replace(homeDirectory, "~");

    return tabname;
  };

  return new QLabel(GenerateTabName());
}
