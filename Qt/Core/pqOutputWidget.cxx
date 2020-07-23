/*=========================================================================

   Program: ParaView
   Module:  pqOutputWidget.cxx

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
#include "pqOutputWidget.h"
#include "ui_pqOutputWidget.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqSettings.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

#include <QClipboard>
#include <QMutexLocker>
#include <QPointer>
#include <QScopedValueRollback>
#include <QStandardItemModel>
#include <QStringList>
#include <QStyle>

#include <fstream>

namespace OutputWidgetInternals
{
/// Used when pqOutputWidget is registered with vtkOutputWindow as the default
/// output window for VTK messages.
class OutputWindow : public vtkOutputWindow
{
  QtMsgType ConvertMessageType(const MessageTypes& type)
  {
    switch (type)
    {
      case vtkOutputWindow::MESSAGE_TYPE_TEXT:
        return QtInfoMsg;
      case vtkOutputWindow::MESSAGE_TYPE_ERROR:
        return QtCriticalMsg;
      case vtkOutputWindow::MESSAGE_TYPE_WARNING:
      case vtkOutputWindow::MESSAGE_TYPE_GENERIC_WARNING:
        return QtWarningMsg;
      case vtkOutputWindow::MESSAGE_TYPE_DEBUG:
        return QtDebugMsg;
    }
    return QtInfoMsg;
  }

public:
  static OutputWindow* New();
  vtkTypeMacro(OutputWindow, vtkOutputWindow);
  void SetWidget(pqOutputWidget* widget) { this->Widget = widget; }

  void DisplayText(const char* msg) override
  {
    QMutexLocker locker(&this->MutexGenericMessage);
    const auto msgType = this->ConvertMessageType(this->GetCurrentMessageType());
    if (this->Widget)
    {
      const QString qmsg(msg);
      MessageHandler::handlerVTK(msgType, qmsg);
      if (this->Widget->suppress(qmsg, msgType))
      {
        return;
      }
    }
    this->Superclass::DisplayText(msg);
  }

protected:
  OutputWindow() { this->PromptUserOff(); }
  ~OutputWindow() override {}

  QPointer<pqOutputWidget> Widget;
  QMutex MutexGenericMessage;

private:
  OutputWindow(const OutputWindow&) = delete;
  void operator=(const OutputWindow&) = delete;
};
vtkStandardNewMacro(OutputWindow);
}

MessageHandler::MessageHandler(QObject* parent)
  : QObject(parent)
{
  qRegisterMetaType<QtMsgType>();
  connect(this, &MessageHandler::message, this, &MessageHandler::displayMessage);
}

void MessageHandler::install(pqOutputWidget* widget)
{
  auto self = MessageHandler::instance();
  qInstallMessageHandler(MessageHandler::handler);
  if (widget)
  {
    connect(self, &MessageHandler::showMessage, widget, &pqOutputWidget::displayMessage);
  }
}

void MessageHandler::handler(QtMsgType type, const QMessageLogContext& cntxt, const QString& msg)
{
  QString formattedMsg = qFormatLogMessage(type, cntxt, msg);
  formattedMsg += "\n";
  Q_EMIT instance()->message(type, formattedMsg);
}

void MessageHandler::handlerVTK(QtMsgType type, const QString& msg)
{
  Q_EMIT instance()->showMessage(msg, type);
}

MessageHandler* MessageHandler::instance()
{
  static MessageHandler instance;
  return &instance;
}

void MessageHandler::displayMessage(QtMsgType type, const QString& msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  vtkOutputWindow* vtkWindow = vtkOutputWindow::GetInstance();
  if (vtkWindow)
  {
    switch (type)
    {
      case QtDebugMsg:
        vtkWindow->DisplayDebugText(localMsg.constData());
        break;

      case QtInfoMsg:
        vtkWindow->DisplayText(localMsg.constData());
        break;

      case QtWarningMsg:
        vtkWindow->DisplayWarningText(localMsg.constData());
        break;

      case QtCriticalMsg:
        vtkWindow->DisplayErrorText(localMsg.constData());
        break;

      case QtFatalMsg:
        vtkWindow->DisplayErrorText(localMsg.constData());
        abort();
        break;
    }
  }
}

class pqOutputWidget::pqInternals
{
  pqOutputWidget* Parent;
  static const int COLUMN_COUNT = 1;
  static const int COLUMN_DATA = 0;

  static QStandardItem* newEmptyItem()
  {
    auto item = new QStandardItem();
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    return item;
  }

public:
  Ui::OutputWidget Ui;
  QPointer<QStandardItemModel> Model;
  vtkNew<OutputWidgetInternals::OutputWindow> VTKOutputWindow;
  QStringList SuppressedStrings;

  pqInternals(pqOutputWidget* self)
    : Parent(self)
  {
    this->Ui.setupUi(self);
    this->Ui.filterButton->hide(); // for now. not sure how useful the filter is.
    this->Model = new QStandardItemModel(self);
    this->Model->setColumnCount(2);
    this->Ui.treeView->setModel(this->Model);
    this->Ui.treeView->header()->moveSection(COLUMN_COUNT, COLUMN_DATA);

    this->VTKOutputWindow->SetWidget(self);

    // this list needs to be reevaluated. For now, leaving it untouched.
    this->SuppressedStrings << "QEventDispatcherUNIX::unregisterTimer"
                            << "looking for 'HistogramView"
                            << "(looking for 'XYPlot"
                            << "Unrecognised OpenGL version"
                            /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
                             * repeated menu actions in the menus. */
                            << "DBusMenuExporterPrivate"
                            << "DBusMenuExporterDBus"
                            /* This error appears in Qt 5.6 on Mac OS X 10.11.1 (and maybe others)
                               */
                            << "QNSView mouseDragged: Internal mouse button tracking invalid"
                            << "Unrecognised OpenGL version"
                            /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
                             * repeated menu actions in the menus. */
                            << "DBusMenuExporterPrivate"
                            << "DBusMenuExporterDBus"
                            /* Skip XCB errors coming from Qt 5 tests. */
                            << "QXcbConnection: XCB"
                            /* This error message appears on some HDPi screens with not clear
                               reasons */
                            << "QWindowsWindow::setGeometry: Unable to set geometry"
                            /* Skip qt.qpa.xcb errors */
                            << "qt.qpa.xcb: internal error"
                            /* suppress "warning: internal error:  void
                               QXcbWindow::setNetWmStateOnUnmappedWindow() called on mapped window"
                               */
                            << "QXcbWindow::setNetWmStateOnUnmappedWindow";
  }

  void displayMessageInConsole(const QString& message, QtMsgType type)
  {
    QTextCharFormat originalFormat = this->Ui.consoleWidget->getFormat();
    QTextCharFormat curFormat(originalFormat);
    curFormat.setForeground(this->foregroundColor(type));
    curFormat.clearBackground();
    this->Ui.consoleWidget->setFormat(curFormat);
    this->Ui.consoleWidget->printString(message);
    this->Ui.consoleWidget->setFormat(originalFormat);
  }

  void addMessageToTree(const QString& message, QtMsgType type, const QString& summary)
  {
    // Check if message is duplicate of the last one. If so, we just increment
    // the counter.
    QStandardItem* rootItem = this->Model->invisibleRootItem();
    int lastIndex = rootItem->rowCount() - 1;
    if (lastIndex >= 0)
    {
      QStandardItem* lastSummaryItem = rootItem->child(lastIndex, COLUMN_DATA);
      QStandardItem* lastMessageItem = lastSummaryItem->child(0, COLUMN_DATA);
      if (lastSummaryItem->text() == summary && lastMessageItem->text() == message)
      {
        QStandardItem* lastSummaryCount = rootItem->child(lastIndex, COLUMN_COUNT);
        int count = lastSummaryCount->text().toInt();
        count = (count <= 0) ? 2 : count + 1;
        lastSummaryCount->setText(QString::number(count));
        lastSummaryCount->setTextAlignment(Qt::AlignRight);
        return;
      }
    }

    QStandardItem* summaryItem = new QStandardItem(tr(summary));
    summaryItem->setFlags(summaryItem->flags() ^ Qt::ItemIsEditable);
    summaryItem->setForeground(this->foregroundColor(type));
    summaryItem->setIcon(this->icon(type));
    summaryItem->setData(QVariant(Qt::AlignLeft | Qt::AlignTop), Qt::TextAlignmentRole);

    QStandardItem* messageItem = new QStandardItem(tr(message));
    messageItem->setFlags(messageItem->flags() ^ Qt::ItemIsEditable);
    messageItem->setForeground(this->foregroundColor(type));
    messageItem->setData(QVariant(Qt::AlignLeft | Qt::AlignTop), Qt::TextAlignmentRole);

    QList<QStandardItem*> items;
    items << messageItem << this->newEmptyItem();
    summaryItem->appendRow(items);
    items.clear();

    items << summaryItem << this->newEmptyItem();
    rootItem->appendRow(items);
  }

  void clear()
  {
    this->Model->clear();
    this->Ui.consoleWidget->clear();
    this->Model->setColumnCount(2);
    this->Ui.treeView->header()->moveSection(COLUMN_COUNT, COLUMN_DATA);
  }

  void setFontSize(int fontSize)
  {
    this->Ui.consoleWidget->setFontSize(fontSize);

    QFont font;
    font.setPointSize(fontSize);
    this->Ui.treeView->setFont(font);
  }

  QIcon icon(QtMsgType type)
  {
    switch (type)
    {
      case QtDebugMsg:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxInformation);

      case QtCriticalMsg:
      case QtFatalMsg:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxCritical);

      case QtWarningMsg:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxWarning);

      case QtInfoMsg:
      default:
        return QIcon();
    }
  }

  QColor foregroundColor(QtMsgType type)
  {
    switch (type)
    {
      case QtInfoMsg:
      case QtDebugMsg:
        return QColor(Qt::darkGreen);

      case QtCriticalMsg:
      case QtFatalMsg:
      case QtWarningMsg:
        return QColor(Qt::darkRed);

      default:
        return QColor(Qt::black);
    }
  }

  void setSettingsKey(const QString& key)
  {
    this->SettingsKey = key;
    if (!key.isEmpty())
    {
      pqSettings* settings = pqApplicationCore::instance()->settings();
      this->Parent->showFullMessages(
        settings->value(QString("%1.ShowFullMessages").arg(key), false).toBool());
    }
  }

  void saveSettings()
  {
    if (!this->SettingsKey.isEmpty())
    {
      pqSettings* settings = pqApplicationCore::instance()->settings();
      settings->setValue(QString("%1.ShowFullMessages").arg(this->SettingsKey),
        this->Ui.showFullMessagesCheckBox->isChecked());
    }
  }

  const QString& settingsKey() const { return this->SettingsKey; }

  /**
   * add a list of strings to be subpressed
   * this is thread safe.
   */
  void suppress(const QStringList& substrs)
  {
    QMutexLocker locker(&this->SuppressionMutex);
    this->SuppressedStrings.append(substrs);
  }

  /**
   * returns true if the message should be/is suppressed.
   * this is thread safe.
   */
  bool suppress(const QString& message, QtMsgType)
  {
    QMutexLocker locker(&this->SuppressionMutex);
    foreach (const QString& substr, this->SuppressedStrings)
    {
      if (message.contains(substr))
      {
        return true;
      }
    }
    return false;
  }

private:
  QString tr(const QString& sourceText) const
  {
    return QApplication::translate("pqOutputWidget", sourceText.toUtf8().data());
  }
  QString SettingsKey;
  QMutex SuppressionMutex;
};

//-----------------------------------------------------------------------------
pqOutputWidget::pqOutputWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqOutputWidget::pqInternals(this))
{
  // Setup Qt message pattern
  qSetMessagePattern("%{type}: In %{file}, line %{line}\n%{type}: %{message}");

  pqInternals& internals = (*this->Internals);

  this->connect(
    internals.Ui.showFullMessagesCheckBox, SIGNAL(toggled(bool)), SLOT(showFullMessages(bool)));
  this->connect(internals.Ui.saveButton, SIGNAL(clicked()), SLOT(saveToFile()));
  this->connect(internals.Ui.copyButton, SIGNAL(clicked()), SLOT(copyToClipboard()));

  // Tell VTK to forward all messages.
  vtkOutputWindow::SetInstance(internals.VTKOutputWindow.Get());

  // Install the message handler
  MessageHandler::install(this);

  this->setSettingsKey("pqOutputWidget");
}

//-----------------------------------------------------------------------------
pqOutputWidget::~pqOutputWidget()
{
  if (vtkOutputWindow::GetInstance() == this->Internals->VTKOutputWindow.Get())
  {
    vtkOutputWindow::SetInstance(nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqOutputWidget::suppress(const QStringList& substrs)
{
  this->Internals->suppress(substrs);
}

//-----------------------------------------------------------------------------
void pqOutputWidget::saveToFile()
{
  QString text = this->Internals->Ui.consoleWidget->text();
  pqFileDialog fileDialog(NULL, pqCoreUtilities::mainWidget(), "Save output", QString(),
    "Text Files (*.txt);;All Files (*)");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // Canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();
  QByteArray filename_ba = filename.toLocal8Bit();
  std::ofstream fileStream;
  fileStream.open(filename_ba.data());
  if (fileStream.is_open())
  {
    fileStream << text.toStdString();
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
void pqOutputWidget::copyToClipboard()
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  clipboard->setText(this->Internals->Ui.consoleWidget->text());
}

//-----------------------------------------------------------------------------
void pqOutputWidget::clear()
{
  pqInternals& internals = (*this->Internals);
  internals.clear();
}

//-----------------------------------------------------------------------------
bool pqOutputWidget::displayMessage(const QString& message, QtMsgType type)
{
  QString tmessage = message.trimmed();
  if (!this->suppress(tmessage, type))
  {
    this->Internals->displayMessageInConsole(message, type);
    QString summary = this->extractSummary(tmessage, type);
    this->Internals->addMessageToTree(message, type, summary);

    Q_EMIT this->messageDisplayed(message, type);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqOutputWidget::suppress(const QString& message, QtMsgType mtype)
{
  return this->Internals->suppress(message, mtype);
}

//-----------------------------------------------------------------------------
QString pqOutputWidget::extractSummary(const QString& message, QtMsgType)
{
  // check if python traceback, if so, simply return the last line as the
  // summary.
  if (message.startsWith("traceback", Qt::CaseInsensitive))
  {
    return message.section('\n', -1);
  }

  QRegExp vtkMessage("^(?:error|warning|debug|generic warning): In (.*), line (\\d+)\n[^:]*:(.*)$",
    Qt::CaseInsensitive);
  if (vtkMessage.exactMatch(message))
  {
    QString summary = vtkMessage.cap(3);
    summary.replace('\n', ' ');
    return summary;
  }

  // if couldn't extract summary in a known form, just return the first line.
  return message.left(message.indexOf('\n'));
}

//-----------------------------------------------------------------------------
void pqOutputWidget::showFullMessages(bool val)
{
  pqInternals& internals = (*this->Internals);
  internals.Ui.showFullMessagesCheckBox->setChecked(val);
  internals.Ui.stackedWidget->setCurrentIndex(val ? 1 : 0);
  internals.saveSettings();
}

//-----------------------------------------------------------------------------
void pqOutputWidget::setSettingsKey(const QString& key)
{
  pqInternals& internals = (*this->Internals);
  internals.setSettingsKey(key);
}

//-----------------------------------------------------------------------------
const QString& pqOutputWidget::settingsKey() const
{
  const pqInternals& internals = (*this->Internals);
  return internals.settingsKey();
}

//-----------------------------------------------------------------------------
void pqOutputWidget::setFontSize(int fontSize)
{
  this->Internals->setFontSize(fontSize);
}
