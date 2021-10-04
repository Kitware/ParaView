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

#include "pqClickableLabel.h"
#include "pqCoreUtilities.h"
#include "pqPythonEditorActions.h"
#include "pqPythonScriptEditor.h"
#include "pqPythonTextArea.h"
#include "pqTextLinker.h"

#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QStandardPaths>
#include <QTabBar>
#include <QTextStream>

//-----------------------------------------------------------------------------
pqPythonTabWidget::pqPythonTabWidget(QWidget* parent)
  : QTabWidget(parent)
{
  this->addNewTabWidget();
  this->createNewEmptyTab();
  this->setMovable(false);

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
  const int lastIdx = this->indexOf(this->lastFocus);
  const int currentIdx = this->currentIndex();

  if (currentIdx == this->count() - 1)
  {
    if (lastIdx == -1)
    {
      this->setCurrentIndex(0);
    }
    else
    {
      this->setCurrentIndex(lastIdx);
    }

    return;
  }

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
void pqPythonTabWidget::addNewTextArea(const QString& fileName)
{
  // If the fileName is empty abort
  if (fileName.isEmpty())
  {
    return;
  }

  const int isOpened = this->fileIsOpened(fileName);
  if (isOpened != -1)
  {
    this->setCurrentIndex(isOpened);
    return;
  }

  pqPythonTextArea* tArea = this->getCurrentTextArea();
  if (!tArea->isEmpty() || tArea->isLinked())
  {
    this->createNewEmptyTab();
    tArea = this->getCurrentTextArea();
  }

  // If not create a new tab
  tArea->openFile(fileName);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::loadFile(const QString& fileName)
{
  // If the fileName is empty abort
  if (fileName.isEmpty())
  {
    return;
  }

  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text))
  {
    QMessageBox::warning(pqCoreUtilities::mainWidget(), QString("Sorry!"),
      QString("Cannot open file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }

  QTextStream in(&file);
  const QString str = in.readAll();

  // If not create a new tab
  pqPythonTextArea* tArea = this->getCurrentTextArea();
  tArea->setText(str);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::closeCurrentTab()
{
  Q_EMIT this->tabCloseRequested(this->currentIndex());
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
    QString tabName;
    QString elidedTabName;
    QString unstyledTabName;
    this->generateTabName(
      qobject_cast<pqPythonTextArea*>(widget), tabName, elidedTabName, unstyledTabName);
    pqClickableLabel* cLabel =
      new pqClickableLabel(widget, elidedTabName, tabName, unstyledTabName, nullptr, widget);
    this->tabBar()->setTabButton(idx, QTabBar::LeftSide, cLabel);
    this->connect(cLabel, &pqClickableLabel::onClicked, [this](QWidget* clickedWidget) {
      int tmpIdx = this->indexOf(clickedWidget);
      if (tmpIdx >= 0 && tmpIdx < this->count() - 1)
      {
        this->setCurrentIndex(tmpIdx);
      }
    });
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

  this->connect(
    widget, &pqPythonTextArea::contentChanged, [this, widget]() { this->updateTab(widget); });

  this->connect(widget, &pqPythonTextArea::fileOpened, this, &pqPythonTabWidget::fileOpened);
  this->connect(widget, &pqPythonTextArea::fileSaved, this, &pqPythonTabWidget::fileSaved);
  this->setTabCloseButton(widget);
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::setTabCloseButton(pqPythonTextArea* widget)
{
  QPixmap closePixmap = this->style()->standardIcon(QStyle::SP_TitleBarCloseButton).pixmap(16, 16);
  QString label = "Close Tab";
  pqClickableLabel* cLabel =
    new pqClickableLabel(widget, label, label, label, &closePixmap, widget);
  this->tabBar()->setTabButton(this->count() - 2, QTabBar::RightSide, cLabel);
  this->connect(cLabel, &pqClickableLabel::onClicked, [this](QWidget* clickedWidget) {
    int idx = this->indexOf(clickedWidget);
    if (idx >= 0 && idx < this->count() - 1)
    {
      Q_EMIT this->tabCloseRequested(idx);
    }
  });
}

//-----------------------------------------------------------------------------
void pqPythonTabWidget::addNewTabWidget()
{
  QWidget* newTabWidget = new QWidget(this);
  this->addTab(newTabWidget, "+");

  // New tab widget is always the last one
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
    pqPythonTextArea* widget = this->getCurrentTextArea();
    if (!widget->isEmpty() || widget->isLinked())
    {
      this->createNewEmptyTab();
      widget = this->getCurrentTextArea();
    }
    this->TraceWidget = widget;
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
void pqPythonTabWidget::generateTabName(const pqPythonTextArea* widget, QString& tabName,
  QString& elidedTabName, QString& unstyledTabName) const
{
  const auto QColorToString = [](const QColor& color) { return color.name(QColor::HexRgb); };
  const auto ColorText = [&QColorToString](const QColor& color, const QString& text) {
    return "<span style=\"color:" + QColorToString(color) + "\">" + text + "</span>";
  };

  QString fileName = widget->getFilename();
  const QFileInfo fileInfo(fileName);
  const QString macroDir = pqPythonScriptEditor::getMacrosDir();
  const QString scriptDir = pqPythonScriptEditor::getScriptsDir();

  // Insert a tag for the tab type
  QString prefix;
  QColor prefixColor;
  if (widget == this->TraceWidget)
  {
    prefix = "[Trace]";
    prefixColor = Qt::GlobalColor::darkCyan;
  }
  else if (fileName.contains(macroDir))
  {
    prefix = "[Macro]";
    prefixColor = Qt::GlobalColor::darkGreen;
  }
  else if (fileName.contains(scriptDir))
  {
    prefix = "[Script]";
    prefixColor = Qt::GlobalColor::darkBlue;
  }
  tabName = ColorText(prefixColor, prefix);
  unstyledTabName = prefix;

  if (widget->isLinked())
  {
    tabName += ColorText(Qt::GlobalColor::darkYellow, "[Linked]");
    unstyledTabName += "[Linked]";
  }

  tabName += " ";
  unstyledTabName += " ";

  if (fileName.isEmpty())
  {
    tabName += "New File";
    elidedTabName = tabName;
    unstyledTabName += "New File";
  }
  else
  {
    QString absoluteDirPath = fileInfo.dir().absolutePath();

    // Replace the home folder path with ~
    const QString homeDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    absoluteDirPath.replace(homeDirectory, "~");

    elidedTabName = tabName;
    tabName += ColorText(Qt::GlobalColor::gray, absoluteDirPath) + "/";
    unstyledTabName += absoluteDirPath + "/";

    // Path can get long, elide it
    QLabel tmpLabel;
    QFontMetrics metrics(tmpLabel.font());
    QString elidedAbsoluteDirPath = metrics.elidedText(absoluteDirPath, Qt::ElideMiddle, 200);
    elidedTabName += ColorText(Qt::GlobalColor::gray, elidedAbsoluteDirPath) + "/";

    QString styledFileName;
    if (widget->isDirty())
    {
      styledFileName = "<b>" + ColorText(Qt::GlobalColor::red, fileInfo.fileName()) + "</b>";
    }
    else
    {
      styledFileName = fileInfo.fileName();
    }
    tabName += styledFileName;
    unstyledTabName += styledFileName;
    elidedTabName += styledFileName;
  }
}

//-----------------------------------------------------------------------------
int pqPythonTabWidget::fileIsOpened(const QString& fileName) const
{
  const QFileInfo fileInfo(fileName);
  for (int i = 0; i < this->count() - 1; ++i)
  {
    pqPythonTextArea* widget = this->getWidget<pqPythonTextArea>(i);
    if (widget && QFileInfo(widget->getFilename()) == fileInfo)
    {
      return i;
    }
  }

  return -1;
}
