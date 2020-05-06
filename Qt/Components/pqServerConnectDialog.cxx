/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqServerConnectDialog.h"
#include "ui_pqServerConnectDialog.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqServerConfiguration.h"
#include "pqServerConfigurationCollection.h"
#include "pqServerConfigurationImporter.h"
#include "pqServerResource.h"
#include "pqSettings.h"
#include "vtkSetGet.h"

#include <QAuthenticator>
#include <QFormLayout>
#include <QMessageBox>
#include <QNetworkReply>
#include <QRegExp>
#include <QSyntaxHighlighter>
#include <QTextStream>
#include <QUrl>

#include <cassert>

namespace
{
enum
{
  CLIENT_SERVER = 0,
  CLIENT_SERVER_REVERSE_CONNECT = 1,
  CLIENT_DATA_SERVER_RENDER_SERVER = 2,
  CLIENT_DATA_SERVER_RENDER_SERVER_REVERSE_CONNECT = 3
};

QString getPVSCSourcesFromSettings()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  return settings
    ->value("PVSC_SOURCES", QString("# Enter list of URLs to obtain server configurations from.\n"
                                    "# Syntax:\n"
                                    "#    pvsc <url> <userfriendly-name>\n\n"
                                    "# Official Kitware Server Configurations\n"
                                    "pvsc http://www.paraview.org/files/pvsc Kitware Inc.\n"))
    .toString();
}

class SourcesSyntaxHighlighter : public QSyntaxHighlighter
{
  QTextCharFormat KeywordFormat;
  QTextCharFormat CommentFormat;
  QTextCharFormat URLFormat;
  QTextCharFormat NameFormat;
  QTextCharFormat ErrorFormat;

public:
  SourcesSyntaxHighlighter(QTextDocument* parentObject = 0)
    : QSyntaxHighlighter(parentObject)
  {
    this->KeywordFormat.setForeground(Qt::darkBlue);
    this->CommentFormat.setForeground(Qt::darkGreen);
    this->URLFormat.setForeground(Qt::blue);
    this->URLFormat.setFontItalic(true);
    this->URLFormat.setForeground(Qt::black);
    this->NameFormat.setFontWeight(QFont::Bold);
    this->ErrorFormat.setForeground(Qt::red);
  }

protected:
  void highlightBlock(const QString& text) override
  {
    // is comment.
    static QRegExp comment("#[^\n]*");
    static QRegExp keyword_line1("^\\s*(pvsc)\\s+");
    static QRegExp keyword_line2("^\\s*(pvsc)\\s+(\\S+)\\s*");
    static QRegExp keyword_line3("^\\s*(pvsc)\\s+(\\S+)\\s+(.+)");

    if (comment.indexIn(text) >= 0)
    {
      this->setFormat(0, comment.matchedLength(), this->CommentFormat);
    }
    else if (keyword_line3.indexIn(text) >= 0)
    {
      this->setFormat(keyword_line3.pos(1), keyword_line3.cap(1).size(), this->KeywordFormat);
      this->setFormat(keyword_line3.pos(2), keyword_line3.cap(2).size(), this->URLFormat);
      this->setFormat(keyword_line3.pos(3), keyword_line3.cap(3).size(), this->NameFormat);
    }
    else if (keyword_line2.indexIn(text) >= 0)
    {
      this->setFormat(keyword_line2.pos(1), keyword_line2.cap(1).size(), this->KeywordFormat);
      this->setFormat(keyword_line2.pos(2), keyword_line2.cap(2).size(), this->URLFormat);
    }
    else if (keyword_line1.indexIn(text) >= 0)
    {
      this->setFormat(keyword_line1.pos(1), keyword_line1.cap(1).size(), this->KeywordFormat);
    }
    else
    {
      this->setFormat(0, text.size(), this->ErrorFormat);
    }
  }
};
}

class pqServerConnectDialog::pqInternals : public Ui::pqServerConnectDialog
{
public:
  QList<pqServerConfiguration> Configurations;
  pqServerResource Selector;
  pqServerConfiguration ToConnect;
  pqServerConfigurationImporter Importer;

  QString OriginalName;
  pqServerConfiguration ActiveConfiguration;
};

//-----------------------------------------------------------------------------
pqServerConnectDialog::pqServerConnectDialog(
  QWidget* parentObject /*=0*/, const pqServerResource& selector /*=pqServerResource()*/)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);
  this->Internals->Selector = selector;

  this->Internals->servers->horizontalHeader()->setObjectName("horz_header");
  // this->Internals->servers->horizontalHeader()->setResizeMode(0,
  //  QHeaderView::ResizeToContents);

  QObject::connect(&pqApplicationCore::instance()->serverConfigurations(), SIGNAL(changed()), this,
    SLOT(updateConfigurations()));

  QObject::connect(this->Internals->servers, SIGNAL(currentCellChanged(int, int, int, int)), this,
    SLOT(onServerSelected()));

  QObject::connect(
    this->Internals->servers, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(connect()));

  QObject::connect(this->Internals->addServer, SIGNAL(clicked()), this, SLOT(addServer()));

  QObject::connect(this->Internals->editServer, SIGNAL(clicked()), this, SLOT(editServer()));

  QObject::connect(
    this->Internals->name, SIGNAL(textChanged(const QString&)), this, SLOT(onNameChanged()));

  QObject::connect(
    this->Internals->type, SIGNAL(currentIndexChanged(int)), this, SLOT(updateServerType()));

  QObject::connect(this->Internals->cancelButton, SIGNAL(clicked()), this, SLOT(goToFirstPage()));
  QObject::connect(
    this->Internals->editServer2ButtonBox, SIGNAL(rejected()), this, SLOT(goToFirstPage()));
  QObject::connect(this->Internals->fetchCancel, SIGNAL(clicked()), this, SLOT(goToFirstPage()));

  QObject::connect(
    this->Internals->okButton, SIGNAL(clicked()), this, SLOT(acceptConfigurationPage1()));

  QObject::connect(this->Internals->editServer2ButtonBox, SIGNAL(accepted()), this,
    SLOT(acceptConfigurationPage2()));

  QObject::connect(this->Internals->deleteServer, SIGNAL(clicked()), this, SLOT(deleteServer()));

  QObject::connect(this, SIGNAL(serverAdded()), this, SLOT(updateButtons()));
  QObject::connect(this, SIGNAL(serverDeleted()), this, SLOT(updateButtons()));

  QObject::connect(this->Internals->connect, SIGNAL(clicked()), this, SLOT(connect()));

  QObject::connect(this->Internals->load, SIGNAL(clicked()), this, SLOT(loadServers()));

  QObject::connect(this->Internals->save, SIGNAL(clicked()), this, SLOT(saveServers()));

  QObject::connect(this->Internals->stackedWidget, SIGNAL(currentChanged(int)), this,
    SLOT(updateDialogTitle(int)));

  QObject::connect(this->Internals->fetchServers, SIGNAL(clicked()), this, SLOT(fetchServers()));

  QObject::connect(this->Internals->editSources, SIGNAL(clicked()), this, SLOT(editSources()));

  QObject::connect(
    this->Internals->editSourcesButtonBox, SIGNAL(accepted()), this, SLOT(saveSourcesList()));
  QObject::connect(
    this->Internals->editSourcesButtonBox, SIGNAL(rejected()), this, SLOT(cancelEditSources()));

  // setup pqServerConfigurationImporter connections.
  QObject::connect(&this->Internals->Importer,
    SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), this,
    SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));

  QObject::connect(&this->Internals->Importer, SIGNAL(incrementalUpdate()), this,
    SLOT(updateImportableConfigurations()));

  QObject::connect(&this->Internals->Importer, SIGNAL(message(const QString&)), this,
    SLOT(importError(const QString&)));

  QObject::connect(this->Internals->importServersTable, SIGNAL(itemSelectionChanged()), this,
    SLOT(importServersSelectionChanged()));

  QObject::connect(this->Internals->importSelected, SIGNAL(clicked()), this, SLOT(importServers()));

  new SourcesSyntaxHighlighter(this->Internals->editSourcesText->document());

  this->Internals->stackedWidget->setCurrentIndex(0);
  this->updateDialogTitle(0);

  this->updateConfigurations();
}

//-----------------------------------------------------------------------------
pqServerConnectDialog::~pqServerConnectDialog()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::updateDialogTitle(int page_number)
{
  switch (page_number)
  {
    case 1:
      this->setWindowTitle("Edit Server Configuration");
      break;

    case 2:
      this->setWindowTitle("Edit Server Launch Configuration");
      break;

    case 3:
      this->setWindowTitle("Fetch Server Configurations");
      break;

    case 4:
      this->setWindowTitle("Edit Server Configuration Sources");
      break;

    case 0:
    default:
      this->setWindowTitle("Choose Server Configuration");
      break;
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::updateConfigurations()
{
  this->Internals->Configurations = this->Internals->Selector.scheme().isEmpty()
    ? pqApplicationCore::instance()->serverConfigurations().configurations()
    : pqApplicationCore::instance()->serverConfigurations().configurations(
        this->Internals->Selector);

  // Add list-items for every configuration.
  bool old = this->Internals->servers->blockSignals(true);
  this->Internals->servers->setRowCount(0);
  this->Internals->servers->setSortingEnabled(false);
  this->Internals->servers->setRowCount(this->Internals->Configurations.size());
  int original_index = 0;
  foreach (const pqServerConfiguration& config, this->Internals->Configurations)
  {
    QTableWidgetItem* item1 = new QTableWidgetItem(config.name());
    QTableWidgetItem* item2 = new QTableWidgetItem(config.resource().toURI());

    // setup tooltips.
    item1->setToolTip(item1->text());
    item2->setToolTip(item2->text());

    // original_index helps us find the item after sorting.
    item1->setData(Qt::UserRole, original_index);
    item2->setData(Qt::UserRole, original_index);

    this->Internals->servers->setItem(original_index, 0, item1);
    this->Internals->servers->setItem(original_index, 1, item2);
    original_index++;
  }
  this->Internals->servers->setSortingEnabled(true);
  this->Internals->servers->blockSignals(old);
  if (this->Internals->Configurations.size() > 0)
  {
    this->Internals->servers->setCurrentCell(0, 0);
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::onServerSelected()
{
  assert(this->Internals->servers->rowCount() == this->Internals->Configurations.size());
  this->updateButtons();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::addServer()
{
  this->editConfiguration(pqServerConfiguration());
  Q_EMIT serverAdded();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::updateButtons()
{
  // Handle the case where there are no servers.
  if (this->Internals->servers->rowCount() == 0)
  {
    this->Internals->editServer->setEnabled(false);
    this->Internals->deleteServer->setEnabled(false);
    this->Internals->connect->setEnabled(false);
    this->Internals->timeoutLabel->setVisible(false);
    this->Internals->timeoutSpinBox->setVisible(false);
    return;
  }

  int row = this->Internals->servers->currentRow();
  if (row >= 0)
  {
    // Convert the row number to original index (since servers can be sorted).
    int original_index = this->Internals->servers->item(row, 0)->data(Qt::UserRole).toInt();

    bool is_mutable = false;
    bool isReverse = false;
    if (original_index >= 0 && original_index < this->Internals->servers->rowCount())
    {
      is_mutable = this->Internals->Configurations[original_index].isMutable();
      isReverse = this->Internals->Configurations[original_index].resource().isReverse();
    }
    this->Internals->editServer->setEnabled(is_mutable);
    this->Internals->deleteServer->setEnabled(is_mutable);
    this->Internals->connect->setEnabled(true);
    this->Internals->timeoutLabel->setVisible(!isReverse);
    this->Internals->timeoutSpinBox->setVisible(!isReverse);
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::editServer()
{
  int row = this->Internals->servers->currentRow();
  assert(row >= 0 && row < this->Internals->servers->rowCount());

  // covert the row number to original index (since servers can be sorted).
  int original_index = this->Internals->servers->item(row, 0)->data(Qt::UserRole).toInt();

  assert(original_index >= 0 && original_index < this->Internals->Configurations.size());

  this->editConfiguration(this->Internals->Configurations[original_index]);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::editConfiguration(const pqServerConfiguration& configuration)
{
  // ensure that we are not editing non-mutable configurations by mistake.
  assert(configuration.isMutable());

  this->Internals->ActiveConfiguration = configuration.clone();
  this->Internals->OriginalName = configuration.name();
  this->Internals->stackedWidget->setCurrentIndex(1);

  // set the default widget values.
  this->Internals->name->setText(configuration.name());
  this->Internals->host->setText("localhost");
  this->Internals->port->setValue(11111);
  this->Internals->dataServerHost->setText("localhost");
  this->Internals->dataServerPort->setValue(11111);
  this->Internals->renderServerHost->setText("localhost");
  this->Internals->renderServerPort->setValue(22221);

  // Update the interface based on the values in \c configuration.
  int type = CLIENT_SERVER;
  QString scheme = configuration.resource().scheme();
  // don't allow the user to change the name when editing an existing
  // configuration.
  this->Internals->name->setEnabled(scheme.isEmpty());
  if (scheme == "cs")
  {
    this->Internals->host->setText(configuration.resource().host());
    this->Internals->port->setValue(configuration.resource().port(11111));
  }
  else if (scheme == "csrc")
  {
    type = CLIENT_SERVER_REVERSE_CONNECT;
    this->Internals->port->setValue(configuration.resource().port(11111));

    // set the host the the remote server name is correct, even if it not used for connecting.
    this->Internals->host->setText(configuration.resource().host());
  }
  else if (scheme == "cdsrs")
  {
    type = CLIENT_DATA_SERVER_RENDER_SERVER;
    this->Internals->dataServerHost->setText(configuration.resource().dataServerHost());
    this->Internals->dataServerPort->setValue(configuration.resource().dataServerPort(11111));
    this->Internals->renderServerHost->setText(configuration.resource().renderServerHost());
    this->Internals->renderServerPort->setValue(configuration.resource().renderServerPort(22221));
  }
  else if (scheme == "cdsrsrc")
  {
    type = CLIENT_DATA_SERVER_RENDER_SERVER_REVERSE_CONNECT;
    this->Internals->dataServerPort->setValue(configuration.resource().dataServerPort(11111));
    this->Internals->renderServerPort->setValue(configuration.resource().renderServerPort(22221));

    // set the host the the remote server name is correct, even if it not used for connecting.
    this->Internals->dataServerHost->setText(configuration.resource().dataServerHost());
    this->Internals->renderServerHost->setText(configuration.resource().renderServerHost());
  }
  this->Internals->type->setCurrentIndex(type);
  this->updateServerType();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::onNameChanged()
{
  bool acceptable = true;
  QString current_name = this->Internals->name->text();
  if (current_name != this->Internals->OriginalName)
  {
    foreach (const pqServerConfiguration& config, this->Internals->Configurations)
    {
      if (config.name() == current_name)
      {
        acceptable = false;
        break;
      }
    }
  }
  else if (current_name.trimmed().isEmpty() || current_name == "unknown")
  {
    acceptable = false;
  }
  this->Internals->okButton->setEnabled(acceptable);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::updateServerType()
{
  this->Internals->hostLabel->setVisible(false);
  this->Internals->host->setVisible(false);
  this->Internals->portLabel->setVisible(false);
  this->Internals->port->setVisible(false);
  this->Internals->renderServerHostLabel->setVisible(false);
  this->Internals->renderServerHost->setVisible(false);
  this->Internals->dataServerHostLabel->setVisible(false);
  this->Internals->dataServerHost->setVisible(false);
  this->Internals->renderServerPortLabel->setVisible(false);
  this->Internals->renderServerPort->setVisible(false);
  this->Internals->dataServerPortLabel->setVisible(false);
  this->Internals->dataServerPort->setVisible(false);

  switch (this->Internals->type->currentIndex())
  {
    case CLIENT_SERVER:
      this->Internals->hostLabel->setVisible(true);
      this->Internals->host->setVisible(true);
      VTK_FALLTHROUGH;
    // break; << -- don't break

    case CLIENT_SERVER_REVERSE_CONNECT:
      this->Internals->portLabel->setVisible(true);
      this->Internals->port->setVisible(true);
      break;

    case CLIENT_DATA_SERVER_RENDER_SERVER:
      this->Internals->renderServerHostLabel->setVisible(true);
      this->Internals->renderServerHost->setVisible(true);
      this->Internals->dataServerHostLabel->setVisible(true);
      this->Internals->dataServerHost->setVisible(true);
      VTK_FALLTHROUGH;
    // break; << -- don't break

    case CLIENT_DATA_SERVER_RENDER_SERVER_REVERSE_CONNECT:
      this->Internals->renderServerPortLabel->setVisible(true);
      this->Internals->renderServerPort->setVisible(true);
      this->Internals->dataServerPortLabel->setVisible(true);
      this->Internals->dataServerPort->setVisible(true);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::acceptConfigurationPage1()
{
  // based on the user settings, update this->Internals->ActiveConfiguration.

  pqServerConfiguration& config = this->Internals->ActiveConfiguration;

  // note: name doesn't cannot be changed when a server config is being edited.
  config.setName(this->Internals->name->text());

  pqServerResource resource = config.resource();

  switch (this->Internals->type->currentIndex())
  {
    case CLIENT_SERVER:
      resource.setScheme("cs");
      resource.setHost(this->Internals->host->text());
      resource.setPort(this->Internals->port->value());
      break;

    case CLIENT_SERVER_REVERSE_CONNECT:
      resource.setScheme("csrc");
      resource.setHost(this->Internals->host->text());
      resource.setPort(this->Internals->port->value());
      break;

    case CLIENT_DATA_SERVER_RENDER_SERVER:
      resource.setScheme("cdsrs");
      resource.setDataServerHost(this->Internals->dataServerHost->text());
      resource.setDataServerPort(this->Internals->dataServerPort->value());
      resource.setRenderServerHost(this->Internals->renderServerHost->text());
      resource.setRenderServerPort(this->Internals->renderServerPort->value());
      break;

    case CLIENT_DATA_SERVER_RENDER_SERVER_REVERSE_CONNECT:
      resource.setScheme("cdsrsrc");
      resource.setDataServerHost(this->Internals->dataServerHost->text());
      resource.setDataServerPort(this->Internals->dataServerPort->value());
      resource.setRenderServerHost(this->Internals->renderServerHost->text());
      resource.setRenderServerPort(this->Internals->renderServerPort->value());
      break;

    default:
      abort();
  }

  config.setResource(resource);

  // go to next page.
  this->editServerStartup();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::editServerStartup()
{
  // This is the page where the user edits the command to run for launching the
  // server processes, if any.
  this->Internals->stackedWidget->setCurrentIndex(2);
  this->Internals->startup_type->setEnabled(true);

  pqServerConfiguration& config = this->Internals->ActiveConfiguration;
  switch (config.startupType())
  {
    case pqServerConfiguration::COMMAND:
    {
      double delay, timeout;
      this->Internals->startup_type->setCurrentIndex(1);
      this->Internals->commandLine->setText(config.execCommand(timeout, delay));
      this->Internals->delay->setValue(delay);
    }
    break;

    case pqServerConfiguration::MANUAL:
    default:
      this->Internals->startup_type->setCurrentIndex(0);
      break;
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::acceptConfigurationPage2()
{
  pqServerConfiguration& config = this->Internals->ActiveConfiguration;
  switch (this->Internals->startup_type->currentIndex())
  {
    case 0:
      config.setStartupToManual();
      break;

    case 1:
      config.setStartupToCommand(
        0, this->Internals->delay->value(), this->Internals->commandLine->toPlainText());
      break;
  }

  // Save this configuration.
  pqApplicationCore::instance()->serverConfigurations().removeConfiguration(
    this->Internals->OriginalName);
  pqApplicationCore::instance()->serverConfigurations().addConfiguration(
    this->Internals->ActiveConfiguration);
  pqApplicationCore::instance()->serverConfigurations().saveNow();

  // Now, make this newly edited configuration the selected one.
  QList<QTableWidgetItem*> items = this->Internals->servers->findItems(
    this->Internals->ActiveConfiguration.name(), Qt::MatchFixedString);
  if (items.size() > 0)
  {
    this->Internals->servers->setCurrentItem(items[0]);
  }

  this->goToFirstPage();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::goToFirstPage()
{
  this->Internals->ActiveConfiguration = pqServerConfiguration();
  this->Internals->OriginalName = QString();

  this->Internals->stackedWidget->setCurrentIndex(0);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::deleteServer()
{
  int row = this->Internals->servers->currentRow();
  assert(row >= 0 && row < this->Internals->servers->rowCount());

  // covert the row number to original index (since servers can be sorted).
  int original_index = this->Internals->servers->item(row, 0)->data(Qt::UserRole).toInt();

  assert(original_index >= 0 && original_index < this->Internals->Configurations.size());

  const pqServerConfiguration& config = this->Internals->Configurations[original_index];
  if (QMessageBox::question(this, "Delete Server Configuration",
        QString("Are you sure you want to delete \"%1\"?").arg(config.name()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
  {
    pqApplicationCore::instance()->serverConfigurations().removeConfiguration(config.name());
    pqApplicationCore::instance()->serverConfigurations().saveNow();
  }

  Q_EMIT serverDeleted();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::saveServers()
{
  QString filters;
  filters += "ParaView server configuration file (*.pvsc)";

  pqFileDialog dialog(NULL, this, tr("Save Server Configuration File"), QString(), filters);
  dialog.setObjectName("SaveServerConfigurationDialog");
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (dialog.exec() == QDialog::Accepted)
  {
    pqApplicationCore::instance()->serverConfigurations().save(
      dialog.getSelectedFiles()[0], /*only_mutable=*/false);
  }
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::loadServers()
{
  QString filters;
  filters += "ParaView server configuration file (*.pvsc)";
  filters += ";;All files (*)";

  pqFileDialog dialog(NULL, this, tr("Load Server Configuration File"), QString(), filters);
  dialog.setObjectName("LoadServerConfigurationDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted)
  {
    pqApplicationCore::instance()->serverConfigurations().load(
      dialog.getSelectedFiles()[0], /*mutable_configs=*/true);
  }
  pqApplicationCore::instance()->serverConfigurations().saveNow();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::connect()
{
  int row = this->Internals->servers->currentRow();
  assert(row >= 0 && row < this->Internals->servers->rowCount());

  // covert the row number to original index (since servers can be sorted).
  int original_index = this->Internals->servers->item(row, 0)->data(Qt::UserRole).toInt();

  assert(original_index >= 0 && original_index < this->Internals->Configurations.size());

  this->Internals->ToConnect = this->Internals->Configurations[original_index];
  this->Internals->ToConnect.setConnectionTimeout(this->Internals->timeoutSpinBox->value());
  this->accept();
}

//-----------------------------------------------------------------------------
const pqServerConfiguration& pqServerConnectDialog::configurationToConnect() const
{
  return this->Internals->ToConnect;
}

//-----------------------------------------------------------------------------
bool pqServerConnectDialog::selectServer(pqServerConfiguration& selected_configuration,
  QWidget* dialogParent /*=NULL*/, const pqServerResource& selector /*=pqServerResource()*/)
{
  // see if only 1 server matched the selector (if valid). In that case, no
  // need to popup the dialog.
  if (!selector.scheme().isEmpty())
  {
    QList<pqServerConfiguration> configs =
      pqApplicationCore::instance()->serverConfigurations().configurations(selector);
    if (configs.size() == 1)
    {
      selected_configuration = configs[0];
      return true;
    }
    else if (configs.size() == 0)
    {
      // Ne configs found, still add resource so config can be used somehow
      selected_configuration.setResource(selector);
      return true;
    }
  }

  pqServerConnectDialog dialog(dialogParent, selector);
  if (dialog.exec() == QDialog::Accepted)
  {
    selected_configuration = dialog.configurationToConnect();
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::editSources()
{
  this->Internals->stackedWidget->setCurrentIndex(4);
  QString pvsc_sources = getPVSCSourcesFromSettings();
  this->Internals->editSourcesText->setPlainText(pvsc_sources);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::cancelEditSources()
{
  // simply go to previous page, no need to fetch sources again.
  this->Internals->stackedWidget->setCurrentIndex(3);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::saveSourcesList()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("PVSC_SOURCES", this->Internals->editSourcesText->toPlainText());
  this->fetchServers();
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::fetchServers()
{
  this->Internals->stackedWidget->setCurrentIndex(3);
  this->Internals->Importer.clearSources();

  QString pvsc_sources = getPVSCSourcesFromSettings();

  QRegExp regExp("pvsc\\s+([^\\s]+)\\s+(.+)");
  QTextStream stream(&pvsc_sources, QIODevice::ReadOnly);
  foreach (const QString& line, stream.readAll().split("\n", QString::SkipEmptyParts))
  {
    QString cleaned_line = line.trimmed();
    if (regExp.exactMatch(cleaned_line))
    {
      this->Internals->Importer.addSource(regExp.cap(2), regExp.cap(1));
    }
  }

  QDialog dialog(this);
  QFormLayout* flayout = new QFormLayout();
  dialog.setLayout(flayout);
  dialog.setWindowTitle("Fetching configurations ...");

  QDialogButtonBox* buttons =
    new QDialogButtonBox(QDialogButtonBox::Abort, Qt::Horizontal, &dialog);
  flayout->addRow(buttons);
  QObject::connect(buttons, SIGNAL(rejected()), &this->Internals->Importer, SLOT(abortFetch()));
  dialog.show();
  dialog.raise();
  dialog.activateWindow();
  this->Internals->Importer.fetchConfigurations();
}

//-----------------------------------------------------------------------------
/// called when the importer needs authentication from the user.
void pqServerConnectDialog::authenticationRequired(
  QNetworkReply* reply, QAuthenticator* authenticator)
{
  QDialog dialog(this);
  QFormLayout* flayout = new QFormLayout();
  dialog.setLayout(flayout);

  dialog.setWindowTitle("Authenticate Connection");
  QLabel* label =
    new QLabel(QString("%1 at %2").arg(authenticator->realm()).arg(reply->url().host()), &dialog);
  QLineEdit* username = new QLineEdit(reply->url().userName(), &dialog);
  QLineEdit* password = new QLineEdit(reply->url().password(), &dialog);
  QPushButton* okButton = new QPushButton("Accept");
  QObject::connect(okButton, SIGNAL(clicked()), &dialog, SLOT(accept()));
  password->setEchoMode(QLineEdit::Password);
  flayout->addRow(label);
  flayout->addRow("Username", username);
  flayout->addRow("Password", password);
  dialog.adjustSize();
  if (dialog.exec() == QDialog::Accepted)
  {
    authenticator->setUser(username->text());
    authenticator->setPassword(password->text());
  }
}

//-----------------------------------------------------------------------------
/// called update importable configs.
void pqServerConnectDialog::updateImportableConfigurations()
{
  const QList<pqServerConfigurationImporter::Item>& items =
    this->Internals->Importer.configurations();

  // remove old items.
  this->Internals->importServersTable->setRowCount(0);
  this->Internals->importServersTable->setRowCount(items.size());
  this->Internals->importServersTable->setSortingEnabled(false);
  int original_index = 0;
  foreach (const pqServerConfigurationImporter::Item& item, items)
  {
    QTableWidgetItem* item1 = new QTableWidgetItem(item.Configuration.name());
    QTableWidgetItem* item2 = new QTableWidgetItem("");
    QTableWidgetItem* item3 = new QTableWidgetItem(item.SourceName);

    // setup tooltips.
    item1->setToolTip(item1->text());
    item2->setToolTip(item2->text());
    item3->setToolTip(item3->text());

    // original_index helps us find the item after sorting.
    item1->setData(Qt::UserRole, original_index);
    item2->setData(Qt::UserRole, original_index);
    item3->setData(Qt::UserRole, original_index);

    this->Internals->importServersTable->setItem(original_index, 0, item1);
    this->Internals->importServersTable->setItem(original_index, 1, item2);
    this->Internals->importServersTable->setItem(original_index, 2, item3);
    original_index++;
  }
  this->Internals->importServersTable->setSortingEnabled(true);
}

//-----------------------------------------------------------------------------
/// called to report error from importer.
void pqServerConnectDialog::importError(const QString& /*message*/)
{
  // QMessageBox::information(
  //   this, tr("Configuration Fetch Failed"), message);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::importServersSelectionChanged()
{
  this->Internals->importSelected->setEnabled(
    this->Internals->importServersTable->selectedItems().size() > 0);
}

//-----------------------------------------------------------------------------
void pqServerConnectDialog::importServers()
{
  QList<QTableWidgetItem*> items = this->Internals->importServersTable->selectedItems();
  QSet<int> indexes;
  foreach (QTableWidgetItem* item, items)
  {
    indexes.insert(item->data(Qt::UserRole).toInt());
  }

  pqServerConfigurationCollection& collection =
    pqApplicationCore::instance()->serverConfigurations();

  foreach (int index, indexes)
  {
    collection.addConfiguration(this->Internals->Importer.configurations()[index].Configuration);
  }

  this->goToFirstPage();
}
