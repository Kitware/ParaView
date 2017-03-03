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
#include "pqSettings.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

#include <QPointer>
#include <QStandardItemModel>
#include <QStringList>
#include <QStyle>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
class QMessageLogContext
{
public:
  QMessageLogContext() {}
  ~QMessageLogContext() {}
};
#endif

namespace OutputWidgetInternals
{

template <class T>
class ScopedSetter
{
private:
  T& Ref;
  T OldValue;

public:
  ScopedSetter(T& ref, const T& newvalue)
    : Ref(ref)
    , OldValue(ref)
  {
    this->Ref = newvalue;
  }
  ~ScopedSetter() { this->Ref = this->OldValue; }
private:
  ScopedSetter(const ScopedSetter&) VTK_DELETE_FUNCTION;
  void operator=(const ScopedSetter&) VTK_DELETE_FUNCTION;
};

/// Used when pqOutputWidget is registered with vtkOutputWindow as the default
/// output window for VTK messages.
class OutputWindow : public vtkOutputWindow
{
public:
  static OutputWindow* New();
  vtkTypeMacro(OutputWindow, vtkOutputWindow);

  void SetWidget(pqOutputWidget* widget) { this->Widget = widget; }

  void DisplayText(const char* msg) VTK_OVERRIDE
  {
    bool display = true;
    if (this->Widget)
    {
      display = this->Widget->displayMessage(msg, this->CurrentMessageType);
    }
    if (display)
    {
      cout << msg;
      // Ideally, we'd simply call superclass. However there's a bad interaction
      // between pqProgressManager, vtkPVProgressHandler and support for
      // server-side messages that leads this to be an infinite recursion. We
      // will fix that separately as that's beyond the scope of this changeset.
      // this->Superclass::DisplayText(msg);
    }
  }

  void DisplayErrorText(const char* msg) VTK_OVERRIDE
  {
    ScopedSetter<pqOutputWidget::MessageTypes> a(this->CurrentMessageType, pqOutputWidget::ERROR);
    this->Superclass::DisplayErrorText(msg); // this calls DisplayText();
  }

  void DisplayWarningText(const char* msg) VTK_OVERRIDE
  {
    ScopedSetter<pqOutputWidget::MessageTypes> a(this->CurrentMessageType, pqOutputWidget::WARNING);
    this->Superclass::DisplayWarningText(msg); // this calls DisplayText();
  }

  void DisplayGenericWarningText(const char* msg) VTK_OVERRIDE
  {
    ScopedSetter<pqOutputWidget::MessageTypes> a(this->CurrentMessageType, pqOutputWidget::WARNING);
    this->Superclass::DisplayGenericWarningText(msg); // this calls DisplayText();
  }

  void DisplayDebugText(const char* msg) VTK_OVERRIDE
  {
    ScopedSetter<pqOutputWidget::MessageTypes> a(this->CurrentMessageType, pqOutputWidget::DEBUG);
    this->Superclass::DisplayDebugText(msg); // this calls DisplayText();
  }

protected:
  OutputWindow()
    : CurrentMessageType(pqOutputWidget::MESSAGE)
  {
    this->PromptUserOff();
  }
  ~OutputWindow() {}

  pqOutputWidget::MessageTypes CurrentMessageType;
  QPointer<pqOutputWidget> Widget;

private:
  OutputWindow(const OutputWindow&) VTK_DELETE_FUNCTION;
  void operator=(const OutputWindow&) VTK_DELETE_FUNCTION;
};
vtkStandardNewMacro(OutputWindow);

void MessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
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

#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
      case QtInfoMsg:
        vtkWindow->DisplayText(localMsg.constData());
        break;
#endif

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

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
void MsgHandler(QtMsgType type, const char* cmsg)
{
  MessageHandler(type, QMessageLogContext(), QString(cmsg));
}
#endif // endif (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
}

class pqOutputWidget::pqInternals
{
  pqOutputWidget* Parent;
  static const int COLUMN_COUNT = 1;
  static const int COLUMN_DATA = 0;

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
    this->SuppressedStrings
      << "QEventDispatcherUNIX::unregisterTimer"
      << "looking for 'HistogramView"
      << "(looking for 'XYPlot"
      << "Unrecognised OpenGL version"
      /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
       * repeated menu actions in the menus. */
      << "DBusMenuExporterPrivate"
      << "DBusMenuExporterDBus"
      /* This error appears in Qt 5.6 on Mac OS X 10.11.1 (and maybe others) */
      << "QNSView mouseDragged: Internal mouse button tracking invalid"
      << "Unrecognised OpenGL version"
      /* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
       * repeated menu actions in the menus. */
      << "DBusMenuExporterPrivate"
      << "DBusMenuExporterDBus"
      /* Skip XCB errors coming from Qt 5 tests. */
      << "QXcbConnection: XCB";
  }

  void displayMessageInConsole(const QString& message, pqOutputWidget::MessageTypes type)
  {
    QTextCharFormat originalFormat = this->Ui.consoleWidget->getFormat();
    QTextCharFormat curFormat(originalFormat);
    curFormat.setForeground(this->foregroundColor(type));
    curFormat.clearBackground();
    this->Ui.consoleWidget->setFormat(curFormat);
    this->Ui.consoleWidget->printString(message);
    this->Ui.consoleWidget->setFormat(originalFormat);
  }

  void addMessageToTree(
    const QString& message, pqOutputWidget::MessageTypes type, const QString& summary)
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

    QStandardItem* messageItem = new QStandardItem(tr(message));
    messageItem->setFlags(messageItem->flags() ^ Qt::ItemIsEditable);
    messageItem->setForeground(this->foregroundColor(type));

    QList<QStandardItem*> items;
    items << messageItem << new QStandardItem();
    summaryItem->appendRow(items);
    items.clear();

    items << summaryItem << new QStandardItem();
    rootItem->appendRow(items);
  }

  void clear()
  {
    this->Model->clear();
    this->Ui.consoleWidget->clear();
    this->Model->setColumnCount(2);
    this->Ui.treeView->header()->moveSection(COLUMN_COUNT, COLUMN_DATA);
  }

  QIcon icon(pqOutputWidget::MessageTypes type)
  {
    switch (type)
    {
      case DEBUG:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxInformation);

      case ERROR:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxCritical);

      case WARNING:
        return this->Parent->style()->standardIcon(QStyle::SP_MessageBoxWarning);

      case MESSAGE:
      default:
        return QIcon();
    }
  }

  QColor foregroundColor(pqOutputWidget::MessageTypes type)
  {
    switch (type)
    {
      case MESSAGE:
      case DEBUG:
        return QColor(Qt::darkGreen);

      case ERROR:
      case WARNING:
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

private:
  QString tr(const QString& sourceText) const
  {
    return QApplication::translate("pqOutputWidget", sourceText.toUtf8().data());
  }
  QString SettingsKey;
};

//-----------------------------------------------------------------------------
void pqOutputWidget::installQMessageHandler()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
  qInstallMessageHandler(OutputWidgetInternals::MessageHandler);
#else
  qInstallMsgHandler(OutputWidgetInternals::MsgHandler);
#endif
}

//-----------------------------------------------------------------------------
pqOutputWidget::pqOutputWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , Internals(new pqOutputWidget::pqInternals(this))
{
  pqInternals& internals = (*this->Internals);

  this->connect(
    internals.Ui.showFullMessagesCheckBox, SIGNAL(toggled(bool)), SLOT(showFullMessages(bool)));

  // Tell VTK to forward all messages.
  vtkOutputWindow::SetInstance(internals.VTKOutputWindow.Get());

  // Tell Qt to forward us all messages.
  pqOutputWidget::installQMessageHandler();

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
  pqInternals& internals = (*this->Internals);
  internals.SuppressedStrings.append(substrs);
}

//-----------------------------------------------------------------------------
void pqOutputWidget::clear()
{
  pqInternals& internals = (*this->Internals);
  internals.clear();
}

//-----------------------------------------------------------------------------
bool pqOutputWidget::displayMessage(const QString& message, MessageTypes type)
{
  QString tmessage = message.trimmed();
  if (!this->suppress(tmessage, type))
  {
    this->Internals->displayMessageInConsole(message, type);
    QString summary = this->extractSummary(tmessage, type);
    this->Internals->addMessageToTree(message, type, summary);

    emit this->messageDisplayed(message, type);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool pqOutputWidget::suppress(const QString& message, MessageTypes)
{
  pqInternals& internals = (*this->Internals);
  foreach (const QString& substr, internals.SuppressedStrings)
  {
    if (message.contains(substr))
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
QString pqOutputWidget::extractSummary(const QString& message, MessageTypes)
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
  return message;
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
