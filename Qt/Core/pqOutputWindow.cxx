/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindow.cxx

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
#include "pqOutputWindow.h"
#include "ui_pqOutputWindow.h"

#include <QAction>
#include <QList>
#include <QMenu>

#include "pqApplicationCore.h"
#include "pqCheckBoxDelegate.h"
#include "pqOutputWindowModel.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkObjectFactory.h"
#include "vtkPVProgressHandler.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"

namespace
{

bool parsePythonMessage(const QString& text, QList<QString>* location, QList<QString>* message)
{
  QRegExp reMessage("(Traceback .*\n(?: +.*\n)+)([^ ].*)");
  int pos = 0;

  while ((pos = reMessage.indexIn(text, pos)) != -1)
  {
    location->push_back(reMessage.cap(1).trimmed());
    message->push_back(reMessage.cap(2).trimmed());
    std::cerr << location->at(location->size() - 1).toStdString() << std::endl;
    pos += reMessage.matchedLength();
  }
  return location->size() > 0;
}

bool parseVtkMessage(int messageType, const QString& text, QString* location, QString* message)
{
  // WARNING: This array has to match the order of pqOutputWindow::MessageType
  const char* messageStart[] = { "ERROR: ", "Warning: ", "Debug : " };
  const char* reAlternateWarningPattern = "Generic Warning: (.* line \\d+)(.*)";
  const char* reLast = "(.*0x\\d+[^:]*): (.+)";
  QString reMessagePattern = QString(messageStart[messageType]) + reLast;
  QRegExp reMessage(reMessagePattern);
  QRegExp reAlternateWarning(reAlternateWarningPattern);
  if (reMessage.exactMatch(text))
  {
    *location = reMessage.cap(1);
    *message = reMessage.cap(2);
    return true;
  }
  else if (messageType == pqOutputWindow::WARNING && reAlternateWarning.exactMatch(text))
  {
    *location = reAlternateWarning.cap(1);
    *message = reAlternateWarning.cap(2);
    return true;
  }
  return false;
}

void warnOnce()
{
  static bool warned = false;
  if (!warned)
  {
    warned = true;
    VTK_LEGACY_BODY(pqOutputWindow, "ParaView 5.4");
  }
}
};

//////////////////////////////////////////////////////////////////////
// pqOutputWindow::pqImplementation

struct pqOutputWindow::pqImplementation
{
  pqImplementation(QObject* parent)
  {
    // pqOutputWindow is the parent, so this object is deleted then.
    this->TableModel = new pqOutputWindowModel(parent, Messages);

    this->ResizeRowsTimer.setInterval(100);
    this->ResizeRowsTimer.setSingleShot(true);

    this->ResizeColumnsTimer.setInterval(100);
    this->ResizeColumnsTimer.setSingleShot(true);
  }

  Ui::pqOutputWindow Ui;
  QStringList SuppressionExpressions;
  QList<MessageT> Messages;
  pqOutputWindowModel* TableModel;

  pqTimer ResizeRowsTimer;
  pqTimer ResizeColumnsTimer;
};

//////////////////////////////////////////////////////////////////////
// pqOutputWindow

pqOutputWindow::pqOutputWindow(QWidget* Parent)
  : Superclass(Parent)
  , StartEventIndex(0)
  , Implementation(new pqImplementation(this))
{
  this->setupSuppressionExpressions();

  for (int i = 0; i < MESSAGE_TYPE_COUNT; ++i)
  {
    this->Show[i] = true;
  }
  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  ui.setupUi(this);
  this->setObjectName("outputDialog");
  this->setWindowTitle(tr("Output Messages"));

  QObject::connect(ui.clearButton, SIGNAL(clicked(bool)), this, SLOT(clear()));
  QObject::connect(
    ui.checkBoxConsoleView, SIGNAL(stateChanged(int)), this, SLOT(setConsoleView(int)));
  QMenu* filterMenu = new QMenu(this);
  QAction* errorAction = new QAction(tr("Errors"), this);
  errorAction->setCheckable(true);
  errorAction->setChecked(true);
  filterMenu->addAction(errorAction);
  QObject::connect(errorAction, SIGNAL(toggled(bool)), this, SLOT(errorToggled(bool)));
  QAction* warningAction = new QAction(tr("Warnings"), this);
  warningAction->setCheckable(true);
  warningAction->setChecked(true);
  QObject::connect(warningAction, SIGNAL(toggled(bool)), this, SLOT(warningToggled(bool)));
  filterMenu->addAction(warningAction);
  QAction* debugAction = new QAction(tr("Debug"), this);
  debugAction->setCheckable(true);
  debugAction->setChecked(true);
  QObject::connect(debugAction, SIGNAL(toggled(bool)), this, SLOT(debugToggled(bool)));
  filterMenu->addAction(debugAction);
  ui.filterButton->setMenu(filterMenu);

  ui.tableView->verticalHeader()->hide();
  ui.tableView->horizontalHeader()->hide();
  ui.tableView->setShowGrid(false);
  ui.tableView->horizontalHeader()->setStretchLastSection(true);
  ui.tableView->setSelectionMode(QAbstractItemView::NoSelection);
  ui.tableView->setFocusPolicy(Qt::NoFocus);
  ui.tableView->setItemDelegateForColumn(0, new pqCheckBoxDelegate(ui.tableView));
  ui.tableView->setModel(this->Implementation->TableModel);

  this->Implementation->ResizeRowsTimer.connect(
    this->Implementation->TableModel, SIGNAL(resizeRowsToContents()), SLOT(start()));

  ui.tableView->connect(
    &this->Implementation->ResizeRowsTimer, SIGNAL(timeout()), SLOT(resizeRowsToContents()));

  this->Implementation->ResizeColumnsTimer.connect(
    this->Implementation->TableModel, SIGNAL(resizeColumnsToContents()), SLOT(start()));

  ui.tableView->connect(
    &this->Implementation->ResizeColumnsTimer, SIGNAL(timeout()), SLOT(resizeColumnsToContents()));
}

//-----------------------------------------------------------------------------
pqOutputWindow::~pqOutputWindow()
{
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayTextInWindow(const QString& text)
{
  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkGreen);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);
  ui.consoleWidget->printString(text);
  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addPythonMessages(DEBUG, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayErrorTextInWindow(const QString& text)
{
  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);
  ui.consoleWidget->printString(text);
  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addPythonMessages(ERROR, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayText(const QString& text)
{
  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkGreen);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);

  ui.consoleWidget->printString(text + "\n");
  cerr << text.toLocal8Bit().data() << endl;
  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addMessage(DEBUG, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayWarningText(const QString& text)
{
  Ui::pqOutputWindow& ui = this->Implementation->Ui;

  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::black);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);

  ui.consoleWidget->printString(text + "\n");
  cerr << text.toLocal8Bit().data() << endl;

  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addMessage(WARNING, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayGenericWarningText(const QString& text)
{
  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::black);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);

  ui.consoleWidget->printString(text + "\n");
  cerr << text.toLocal8Bit().data() << endl;

  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addMessage(WARNING, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onDisplayErrorText(const QString& text)
{
  if (this->shouldMessageBeSuppressed(text))
  {
    return;
  }

  Ui::pqOutputWindow& ui = this->Implementation->Ui;

  QTextCharFormat format = ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  ui.consoleWidget->setFormat(format);

  ui.consoleWidget->printString(text + "\n");
  cerr << text.toLocal8Bit().data() << endl;

  if (!text.trimmed().isEmpty())
  {
    this->show();
    this->addMessage(ERROR, text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::accept()
{
  this->hide();
  Superclass::accept();
}

//-----------------------------------------------------------------------------
void pqOutputWindow::reject()
{
  this->hide();
  Superclass::reject();
}

//-----------------------------------------------------------------------------
void pqOutputWindow::clear()
{
  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  ui.consoleWidget->clear();
  this->Implementation->Messages.clear();
  this->Implementation->TableModel->clear();
}

//-----------------------------------------------------------------------------
void pqOutputWindow::showEvent(QShowEvent* e)
{
  warnOnce();
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    core->settings()->restoreState("OutputWindow", *this);
  }
  Superclass::showEvent(e);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::hideEvent(QHideEvent* e)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    core->settings()->saveState(*this, "OutputWindow");
  }
  Superclass::hideEvent(e);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::setConsoleView(int on)
{
  Ui::pqOutputWindow& ui = this->Implementation->Ui;
  ui.stackedWidget->setCurrentIndex(on ? 1 : 0);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::errorToggled(bool checked)
{
  this->Show[ERROR] = checked;
  this->Implementation->TableModel->ShowMessages(this->Show);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::warningToggled(bool checked)
{
  this->Show[WARNING] = checked;
  this->Implementation->TableModel->ShowMessages(this->Show);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::debugToggled(bool checked)
{
  this->Show[DEBUG] = checked;
  this->Implementation->TableModel->ShowMessages(this->Show);
}

//-----------------------------------------------------------------------------
void pqOutputWindow::setupSuppressionExpressions()
{
  this->Implementation->SuppressionExpressions
    << "QEventDispatcherUNIX::unregisterTimer"
    << "looking for 'HistogramView"
    << "(looking for 'XYPlot"
    << "Unrecognised OpenGL version"
    /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
     * repeated menu actions in the menus. */
    << "DBusMenuExporterPrivate"
    << "DBusMenuExporterDBus"
    /* This error appears in Qt 5.6 on Mac OS X 10.11.1 (and maybe others) */
    << "QNSView mouseDragged: Internal mouse button tracking invalid";
}

//-----------------------------------------------------------------------------
bool pqOutputWindow::shouldMessageBeSuppressed(const QString& text)
{
  if (text.contains("Unrecognised OpenGL version") ||
    /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
     * repeated menu actions in the menus. */
    text.contains("DBusMenuExporterPrivate") || text.contains("DBusMenuExporterDBus") ||
    /* Skip XCB errors coming from Qt 5 tests. */
    text.contains("QXcbConnection: XCB"))
  {
    return true;
  }

  // See if message should be suppressed
  for (int i = 0; i < this->Implementation->SuppressionExpressions.size(); ++i)
  {
    if (text.contains(this->Implementation->SuppressionExpressions[i]))
    {
      return true;
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
void pqOutputWindow::addMessage(int messageType, const QString& text)
{
  QString location;
  QString message = text;
  // const char* s = "Traceback (most recent call last):\n  File \"<string>\", line 4, in <module>\n
  // File
  // \"/home/danlipsa/build/ParaView-output-window-15240/lib/site-packages/paraview/calculator.py\",
  // line 102, in execute\n    retVal = compute(inputs, expression, ns=variables)\n  File
  // \"/home/danlipsa/build/ParaView-output-window-15240/lib/site-packages/paraview/calculator.py\",
  // line 74, in compute\n    retVal = eval(expression, globals(), mylocals)\n  File \"<string>\",
  // line 1\n    $\n    ^\nSyntaxError: unexpected EOF while parsing";
  // parsePythonMessage(0, s, &location, &message);
  parseVtkMessage(messageType, text, &location, &message);
  this->addMessage(messageType, location.trimmed(), message.trimmed());
}

//-----------------------------------------------------------------------------
void pqOutputWindow::addPythonMessages(int messageType, const QString& text)
{
  QList<QString> location;
  QList<QString> message;
  if (parsePythonMessage(text, &location, &message))
  {
    for (int i = 0; i < location.size(); ++i)
    {
      this->addMessage(messageType, location[i], message[i]);
    }
  }
  else
  {
    this->addMessage(messageType, "", text);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::addMessage(int messageType, const QString& location, const QString& message)
{
  for (int j = this->Implementation->Messages.size() - 1; j >= this->StartEventIndex; --j)
  {
    MessageT& wholeMessage = this->Implementation->Messages[j];
    if (wholeMessage.Type == messageType && wholeMessage.Message == message &&
      wholeMessage.Location == location)
    {
      ++wholeMessage.Count;
      this->Implementation->TableModel->updateCount(j);
      return;
    }
  }
  // this is a new message
  this->Implementation->Messages.push_back(
    MessageT(pqOutputWindow::MessageType(messageType), 1, location, message));
  if (this->Show[messageType])
  {
    this->Implementation->TableModel->appendLastRow();
  }
}

//-----------------------------------------------------------------------------
void pqOutputWindow::onProgressStartEvent()
{
  this->StartEventIndex = this->Implementation->Messages.size();
}
