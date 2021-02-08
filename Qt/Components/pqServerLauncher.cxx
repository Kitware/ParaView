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
#include "pqServerLauncher.h"
#include "ui_pqConnectIdDialog.h"
#include "ui_pqServerLauncherDialog.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileChooserWidget.h"
#include "pqObjectBuilder.h"
#include "pqServer.h"
#include "pqServerConfiguration.h"
#include "pqServerResource.h"
#include "pqSettings.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkNetworkAccessManager.h"
#include "vtkPVConfig.h"
#include "vtkPVOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkTimerLog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QtDebug>

#include <cassert>

//----------------------------------------------------------------------------
const QMetaObject* pqServerLauncher::DefaultServerLauncherType = NULL;
const QMetaObject* pqServerLauncher::setServerDefaultLauncherType(const QMetaObject* other)
{
  const QMetaObject* old = pqServerLauncher::DefaultServerLauncherType;
  pqServerLauncher::DefaultServerLauncherType = other;
  return old;
}

const QMetaObject* pqServerLauncher::defaultServerLauncherType()
{
  return pqServerLauncher::DefaultServerLauncherType;
}

pqServerLauncher* pqServerLauncher::newInstance(
  const pqServerConfiguration& configuration, QObject* parentObject)
{
  if (pqServerLauncher::DefaultServerLauncherType)
  {
    QObject* aObject = pqServerLauncher::DefaultServerLauncherType->newInstance(
      Q_ARG(const pqServerConfiguration&, configuration), Q_ARG(QObject*, parentObject));
    if (pqServerLauncher* aLauncher = qobject_cast<pqServerLauncher*>(aObject))
    {
      return aLauncher;
    }
    delete aObject;
  }
  return new pqServerLauncher(configuration, parentObject);
}

//----------------------------------------------------------------------------
namespace
{
/// pqWidget is used to make it easier to get and set values from different
/// types of widgets.
class pqWidget : public QObject
{
  QString PropertyName;

public:
  QWidget* Widget;
  bool ToSave;
  pqWidget()
    : Widget(NULL)
    , ToSave(false)
  {
  }
  pqWidget(QWidget* wdg, const QString& pname)
    : PropertyName(pname)
    , Widget(wdg)
    , ToSave(false)
  {
  }
  ~pqWidget() override {}

  virtual QVariant get() const
  {
    return this->Widget->property(this->PropertyName.toLocal8Bit().data());
  }
  virtual void set(const QVariant& value)
  {
    this->Widget->setProperty(this->PropertyName.toLocal8Bit().data(), value);
  }

private:
  Q_DISABLE_COPY(pqWidget)
};

class pqWidgetForComboBox : public pqWidget
{
public:
  pqWidgetForComboBox(QComboBox* widget)
    : pqWidget(widget, QString())
  {
  }

  QVariant get() const override
  {
    QComboBox* combobox = qobject_cast<QComboBox*>(this->Widget);
    return combobox->itemData(combobox->currentIndex());
  }

  void set(const QVariant& value) override
  {
    QComboBox* combobox = qobject_cast<QComboBox*>(this->Widget);
    combobox->setCurrentIndex(combobox->findData(value));
  }

private:
  Q_DISABLE_COPY(pqWidgetForComboBox)
};

class pqWidgetForCheckbox : public pqWidget
{
  QString TrueValue;
  QString FalseValue;

public:
  pqWidgetForCheckbox(QCheckBox* widget, const char* tval, const char* fval)
    : pqWidget(widget, QString())
    , TrueValue(tval)
    , FalseValue(fval)
  {
  }

  QVariant get() const override
  {
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(this->Widget);
    return checkbox->isChecked() ? this->TrueValue : this->FalseValue;
  }

  void set(const QVariant& value) override
  {
    QCheckBox* checkbox = qobject_cast<QCheckBox*>(this->Widget);
    checkbox->setChecked(value.toString() == this->TrueValue);
  }

private:
  Q_DISABLE_COPY(pqWidgetForCheckbox)
};

/// Returns pre-defined run-time environment. This includes the environment
/// of this application itself as well as some predefined values.
QProcessEnvironment getDefaultEnvironment(const pqServerConfiguration& configuration)
{
  pqServerResource resource = configuration.resource();

  // Get the process environment.
  QProcessEnvironment options = QProcessEnvironment::systemEnvironment();

  // Now append the pre-defined runtime environment to this.
  options.insert("PV_CLIENT_HOST", QHostInfo::localHostName());
  options.insert("PV_CONNECTION_URI", resource.toURI());
  options.insert("PV_CONNECTION_SCHEME", resource.scheme());
  options.insert("PV_VERSION_MAJOR", QString::number(PARAVIEW_VERSION_MAJOR));
  options.insert("PV_VERSION_MINOR", QString::number(PARAVIEW_VERSION_MINOR));
  options.insert("PV_VERSION_PATCH", QString::number(PARAVIEW_VERSION_PATCH));
  options.insert("PV_VERSION", PARAVIEW_VERSION);
  options.insert("PV_VERSION_FULL", PARAVIEW_VERSION_FULL);
  options.insert("PV_SERVER_HOST", resource.host());
  options.insert("PV_SERVER_PORT", QString::number(resource.port(11111)));
  options.insert("PV_SSH_PF_SERVER_PORT", configuration.portForwardingLocalPort());
  options.insert("PV_DATA_SERVER_HOST", resource.dataServerHost());
  options.insert("PV_DATA_SERVER_PORT", QString::number(resource.dataServerPort(11111)));
  options.insert("PV_RENDER_SERVER_HOST", resource.renderServerHost());
  options.insert("PV_RENDER_SERVER_PORT", QString::number(resource.renderServerPort(22221)));

#if defined(_WIN32)
  options.insert("PV_CLIENT_PLATFORM", "Windows");
#elif defined(__APPLE__)
  options.insert("PV_CLIENT_PLATFORM", "Apple");
#elif defined(__linux__)
  options.insert("PV_CLIENT_PLATFORM", "Linux");
#elif defined(__unix__)
  options.insert("PV_CLIENT_PLATFORM", "Unix");
#else
  options.insert("PV_CLIENT_PLATFORM", "Unknown");
#endif

  return options;
}

/// Processes the <Options /> XML defined in the server configuration to
/// update the dialog with widgets of right type with correct default values.
/// It uses application settings to obtain the default values, whenever
/// possible.
bool createWidgets(QMap<QString, pqWidget*>& widgets, QDialog& dialog,
  const pqServerConfiguration& configuration, QProcessEnvironment& options)
{
  vtkPVXMLElement* optionsXML = configuration.optionsXML();
  assert(optionsXML != NULL);

  QFormLayout* formLayout = new QFormLayout();
  dialog.setLayout(formLayout);
  dialog.setWindowTitle(QString("Connection Options for \"%1\"").arg(configuration.name()));

  pqSettings* settings = pqApplicationCore::instance()->settings();

  // Process the <Options/> to create dialog with widgets.
  for (unsigned int cc = 0; cc < optionsXML->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* node = optionsXML->GetNestedElement(cc);
    if (node->GetName() == NULL)
    {
      continue;
    }

    if (strcmp(node->GetName(), "Set") == 0)
    {
      options.insert(node->GetAttribute("name"), node->GetAttribute("value"));
    }
    else if (strcmp(node->GetName(), "Option") == 0)
    {
      vtkPVXMLElement* typeNode = node->GetNestedElement(0);
      if (typeNode == NULL || typeNode->GetName() == NULL)
      {
        continue;
      }

      const char* name = node->GetAttribute("name");
      const char* label = node->GetAttributeOrDefault("label", name);
      bool readonly = strcmp(node->GetAttributeOrDefault("readonly", "false"), "true") == 0;
      bool save = strcmp(node->GetAttributeOrDefault("save", "true"), "true") == 0;

      QString settingsKey = QString("SERVER_STARTUP/%1.%2").arg(configuration.name()).arg(name);

      bool default_is_random = false;
      QVariant default_value;
      if (typeNode->GetAttribute("default"))
      {
        default_is_random = (strcmp(typeNode->GetAttribute("default"), "random") == 0);
        default_value = QString(typeNode->GetAttribute("default"));

        // if default_is_random, save cannot be true.
        if (default_is_random)
        {
          save = false;
        }
      }

      // noise is a in the range [0, 1].
      double noise = 0.0;
      if (default_is_random)
      {
        // We need a seed that changes every execution. Get the
        // universal time as double and then add all the bytes
        // together to get a nice seed without causing any overflow.
        long rseed = 0;
        double atime = vtkTimerLog::GetUniversalTime() * 1000;
        char* tc = (char*)&atime;
        for (unsigned int ic = 0; ic < sizeof(double); ic++)
        {
          rseed += tc[ic];
        }
        vtkMath::RandomSeed(rseed);
        noise = vtkMath::Random();
      }

      // obtain default value from settings if available.
      if (save && settings->contains(settingsKey))
      {
        default_value = settings->value(settingsKey);
      }

      if (strcmp(typeNode->GetName(), "Range") == 0)
      {
        QString min = typeNode->GetAttributeOrDefault("min", "0");
        QString max = typeNode->GetAttributeOrDefault("max", "99999999999");
        QString step = typeNode->GetAttributeOrDefault("step", "1");
        QWidget* widget = NULL;
        if (strcmp(typeNode->GetAttributeOrDefault("type", "int"), "int") == 0)
        {
          widget = new QSpinBox(&dialog);
          if (default_is_random)
          {
            default_value = min.toInt() + (max.toInt() - min.toInt()) * noise;
          }
        }
        else // assume double.
        {
          widget = new QDoubleSpinBox(&dialog);
          if (default_is_random)
          {
            default_value = min.toDouble() + (max.toDouble() - min.toDouble()) * noise;
          }
        }
        widgets[name] = new pqWidget(widget, "value");
        widget->setProperty("minimum", QVariant(min));
        widget->setProperty("maximum", QVariant(max));
        widget->setProperty("singleStep", QVariant(step));
      }
      else if (strcmp(typeNode->GetName(), "String") == 0)
      {
        QLineEdit* widget = new QLineEdit(QString(), &dialog);
        widgets[name] = new pqWidget(widget, "text");
      }
      else if (strcmp(typeNode->GetName(), "Password") == 0)
      {
        QLineEdit* widget = new QLineEdit(QString(), &dialog);
        widget->setEchoMode(QLineEdit::Password);
        widgets[name] = new pqWidget(widget, "text");
      }
      else if (strcmp(typeNode->GetName(), "File") == 0)
      {
        pqFileChooserWidget* widget = new pqFileChooserWidget(&dialog);
        widget->setForceSingleFile(true);
        widgets[name] = new pqWidget(widget, "singleFilename");
      }
      else if (strcmp(typeNode->GetName(), "Boolean") == 0)
      {
        QCheckBox* checkbox = new QCheckBox(&dialog);
        const char* true_value = typeNode->GetAttributeOrDefault("true", "1");
        const char* false_value = typeNode->GetAttributeOrDefault("false", "0");
        widgets[name] = new pqWidgetForCheckbox(checkbox, true_value, false_value);
      }
      else if (strcmp(typeNode->GetName(), "Enumeration") == 0)
      {
        QComboBox* widget = new QComboBox(&dialog);
        for (unsigned int kk = 0; kk < typeNode->GetNumberOfNestedElements(); kk++)
        {
          vtkPVXMLElement* child = typeNode->GetNestedElement(kk);
          if (QString(child->GetName()) == "Entry")
          {
            QString xml_value = child->GetAttribute("value");
            QString xml_label =
              child->GetAttributeOrDefault("label", xml_value.toLocal8Bit().data());
            widget->addItem(xml_label, xml_value);
          }
        }
        widgets[name] = new pqWidgetForComboBox(widget);
      }
      else
      {
        qWarning() << "Ignoring unknown element '<" << typeNode->GetName()
                   << "/>' discovered under <Option/> element.";
        continue;
      }
      widgets[name]->setParent(&dialog);
      widgets[name]->ToSave = save;
      widgets[name]->set(default_value);
      widgets[name]->Widget->setEnabled(!readonly);
      widgets[name]->Widget->setObjectName(name);
      formLayout->addRow(label, widgets[name]->Widget);
    } // end of <Option />
    else if (strcmp(node->GetName(), "Switch") == 0)
    {
      continue; // Switch's are handled afterwords
    }
    else
    {
      qWarning() << "Ignoring unknown element '<" << node->GetName()
                 << "/>' discovered under <Options/> element.";
    }
  }

  QDialogButtonBox* buttonBox =
    new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
  QObject::connect(buttonBox, SIGNAL(accepted()), &dialog, SLOT(accept()));
  QObject::connect(buttonBox, SIGNAL(rejected()), &dialog, SLOT(reject()));
  formLayout->addRow(buttonBox);
  return true;
}

/// Update the QProcessEnvironment based on the values picked by the user on
/// the widgets. This function may change the resource uri for the \c
/// configuration itself if any of the GUI widgets changed the server-port
/// numbers.
void updateEnvironment(const QMap<QString, pqWidget*>& widgets,
  pqServerConfiguration& configuration, QProcessEnvironment& options)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  pqServerResource resource = configuration.resource();
  foreach (const pqWidget* item, widgets)
  {
    QString name = item->Widget->objectName();
    QVariant chosen_value = item->get();
    if (item->ToSave)
    {
      // save the chosen value in settings if requested.
      QString settingsKey = QString("SERVER_STARTUP/%1.%2").arg(configuration.name()).arg(name);
      settings->setValue(settingsKey, chosen_value);
    }
    options.insert(name, chosen_value.toString());

    // Some options can affect the server resource itself e.g. PV_SERVER_PORT etc.
    // So if those were changed using the config XML, we need to update the
    // resource.
    if (name == "PV_SERVER_PORT")
    {
      resource.setPort(chosen_value.toInt());
      configuration.setResource(resource);
    }
    else if (name == "PV_DATA_SERVER_PORT")
    {
      resource.setDataServerPort(chosen_value.toInt());
      configuration.setResource(resource);
    }
    else if (name == "PV_RENDER_SERVER_PORT")
    {
      resource.setRenderServerPort(chosen_value.toInt());
      configuration.setResource(resource);
    }
  }
}

/// Process <Switch />
void handleSwitchCases(const pqServerConfiguration& configuration, QProcessEnvironment& options)
{
  vtkPVXMLElement* optionsXML = configuration.optionsXML();
  for (unsigned int cc = 0; cc < optionsXML->GetNumberOfNestedElements(); cc++)
  {
    vtkPVXMLElement* switchXML = optionsXML->GetNestedElement(cc);
    if (!switchXML->GetName() || strcmp(switchXML->GetName(), "Switch") != 0)
    {
      continue;
    }
    const char* variable = switchXML->GetAttribute("name");
    if (!variable)
    {
      qWarning("Missing attribute 'name' in 'Switch' statement");
      continue;
    }
    if (!options.contains(variable))
    {
      qWarning() << "'Switch' statement has no effect since no variable named " << variable
                 << " is defined. ";
      continue;
    }
    QString value = options.value(variable);
    bool handled = false;
    for (unsigned int kk = 0; !handled && kk < switchXML->GetNumberOfNestedElements(); kk++)
    {
      vtkPVXMLElement* caseXML = switchXML->GetNestedElement(kk);
      if (!caseXML->GetName() || strcmp(caseXML->GetName(), "Case") != 0)
      {
        qWarning() << "'<Switch/> element can only contain <Case/> elements";
        continue;
      }
      const char* case_value = caseXML->GetAttribute("value");
      if (!case_value || value != case_value)
      {
        continue;
      }
      handled = true;
      for (unsigned int i = 0; i < caseXML->GetNumberOfNestedElements(); i++)
      {
        vtkPVXMLElement* setXML = caseXML->GetNestedElement(i);
        if (QString(setXML->GetName()) == "Set")
        {
          const char* option_name = setXML->GetAttributeOrDefault("name", "");
          const char* option_value = setXML->GetAttributeOrDefault("value", "");
          options.insert(option_name, option_value);
        }
        else
        {
          qWarning() << "'Case' element can only contain 'Set' elements as children and not '"
                     << setXML->GetName() << "'";
        }
      }
    }
    if (!handled)
    {
      qWarning() << "Case '" << value << "' not handled in 'Switch' for variable "
                                         "'"
                 << variable << "'";
    }
  }
}
}

class pqServerLauncher::pqInternals
{
public:
  pqServerConfiguration Configuration;
  QProcessEnvironment Options;
  QPointer<pqServer> Server;
  QMap<QString, pqWidget*> ActiveWidgets; // map to save the widgets in promptOptions().
};

//-----------------------------------------------------------------------------
pqServerLauncher::pqServerLauncher(
  const pqServerConfiguration& _configuration, QObject* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();

  // we create a clone so that we can change the values in place.
  this->Internals->Configuration = _configuration.clone();
}

//-----------------------------------------------------------------------------
pqServerLauncher::~pqServerLauncher()
{
  delete this->Internals;
  this->Internals = NULL;
}

//-----------------------------------------------------------------------------
pqServerConfiguration& pqServerLauncher::configuration() const
{
  return this->Internals->Configuration;
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::connectToServer()
{
  pqServerConfiguration::StartupType startupType = this->Internals->Configuration.startupType();
  if (startupType != pqServerConfiguration::MANUAL && startupType != pqServerConfiguration::COMMAND)
  {
    qCritical() << "Invalid server configuration."
                << "Cannot connect to server";
    return false;
  }

  // Check if there are any user-configurable parameters that we should obtain
  // from the user. promptOptions() returns false only when user hits cancel, in
  // which case the user is aborting connecting to the server.
  if (!this->promptOptions())
  {
    return false;
  }

  if (startupType == pqServerConfiguration::COMMAND)
  {
    if (this->isReverseConnection())
    {
      // in reverse connection, we don't launchServer() immediately, instead we
      // wait for the client to setup the "socket" before starting the server
      // process.
      QTimer::singleShot(0, this, SLOT(launchServerForReverseConnection()));
    }
    else
    {
      if (!this->launchServer(true))
      {
        return false;
      }
    }
  }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkNetworkAccessManager* nam = pm->GetNetworkAccessManager();
  vtkPVOptions* options = pm->GetOptions();
  QDialog dialog(pqCoreUtilities::mainWidget(), Qt::WindowStaysOnTopHint);
  Ui::pqConnectIdDialog ui;
  ui.setupUi(&dialog);
  ui.connectId->setMaximum(VTK_INT_MAX);
  ui.connectId->setValue(options->GetConnectID());

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  bool force = builder->forceWaitingForConnection(true);
  bool launched = false;
  while (!(launched = this->connectToPrelaunchedServer()) && nam->GetWrongConnectID() &&
    dialog.exec() == QDialog::Accepted)
  {
    options->SetConnectID(ui.connectId->value());
  }
  builder->forceWaitingForConnection(force);
  return launched;
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::connectToPrelaunchedServer()
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  QDialog dialog(pqCoreUtilities::mainWidget(), Qt::WindowStaysOnTopHint);
  QObject::connect(&dialog, SIGNAL(rejected()), builder, SLOT(abortPendingConnections()));

  Ui::pqServerLauncherDialog ui;
  ui.setupUi(&dialog);
  ui.message->setText(QString("Establishing connection to '%1' \n"
                              "Waiting for server to connect.")
                        .arg(this->Internals->Configuration.name()));
  dialog.setWindowTitle("Waiting for Server Connection");
  if (this->isReverseConnection())
  {
    // using reverse connect, popup the dialog.
    dialog.show();
    dialog.raise();
    dialog.activateWindow();
  }

  const pqServerResource& resource = this->Internals->Configuration.actualResource();
  this->Internals->Server =
    builder->createServer(resource, this->Internals->Configuration.connectionTimeout());
  return this->Internals->Server != NULL;
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::isReverseConnection() const
{
  const pqServerResource& resource = this->Internals->Configuration.actualResource();
  return (resource.scheme() == "csrc" || resource.scheme() == "cdsrsrc");
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::promptOptions()
{
  vtkPVXMLElement* optionsXML = this->Internals->Configuration.optionsXML();
  // Get the process environment.
  QProcessEnvironment& options = this->Internals->Options;
  // setup the options using the default environment, in any case.
  options = getDefaultEnvironment(this->Internals->Configuration);
  if (optionsXML == NULL)
  {
    return true;
  }

  QDialog dialog(pqCoreUtilities::mainWidget(), Qt::WindowStaysOnTopHint);

  // setup the dialog using the configuration's XML.
  QMap<QString, pqWidget*>& widgets = this->Internals->ActiveWidgets;
  ; // map to save the widgets.
  // note: all pqWidget instances created are set with parent as the dialog, so
  // we don't need to clean them up explicitly.
  createWidgets(widgets, dialog, this->Internals->Configuration, options);
  // give subclasses an opportunity to fine-tune the dialog.
  this->prepareDialogForPromptOptions(dialog);
  if (dialog.exec() != QDialog::Accepted)
  {
    widgets.clear();
    return false;
  }

  this->updateOptionsUsingUserSelections();

  // if options contains PV_CONNECT_ID. We need to update the pqOptions to
  // give it the correct connection-id.
  if (options.contains("PV_CONNECT_ID"))
  {
    vtkPVOptions* pvoptions = vtkProcessModule::GetProcessModule()->GetOptions();
    if (pvoptions)
    {
      pvoptions->SetConnectID(options.value("PV_CONNECT_ID").toInt());
    }
  }

  widgets.clear();
  // Now we have the environment filled up correctly.
  return true;
}

//-----------------------------------------------------------------------------
void pqServerLauncher::updateOptionsUsingUserSelections()
{
  if (this->Internals->ActiveWidgets.size() > 0)
  {
    /// now based on user-chosen values, update the options.
    updateEnvironment(
      this->Internals->ActiveWidgets, this->Internals->Configuration, this->Internals->Options);

    // Now that user entered options have been processes, handle the <Switch />
    // elements.  This has to happen after the Options have been updated with
    // user-selected values so that we can pick the right case.
    handleSwitchCases(this->Internals->Configuration, this->Internals->Options);
  }
}

//-----------------------------------------------------------------------------
void pqServerLauncher::launchServerForReverseConnection()
{
  if (!this->launchServer(false))
  {
    // server-launch failed, abort the "waiting for the server to connect" part
    // by letting the pqObjectBuilder know.
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    builder->abortPendingConnections();
  }
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::launchServer(bool show_status_dialog)
{
  // We need launch the server.
  double timeout, delay;
  QString command = this->Internals->Configuration.command(timeout, delay);
  if (command.isEmpty())
  {
    qCritical() << "Could not determine command to launch the server.";
    return false;
  }

  // Pop-up a dialog to tell the user that the server is being launched.
  QDialog dialog(pqCoreUtilities::mainWidget(), Qt::WindowStaysOnTopHint);
  Ui::pqServerLauncherDialog ui;
  ui.setupUi(&dialog);
  ui.cancel->hide();
  ui.message->setText(QString("Launching server '%1'").arg(this->Internals->Configuration.name()));
  if (show_status_dialog)
  {
    dialog.show();
    dialog.raise();
    dialog.activateWindow();
  }

  // replace all $FOO$ with values for QProcessEnvironment.
  QRegExp regex("\\$([^$ ]*)\\$");

  // Do string-substitution for the command line.
  while (regex.indexIn(command) > -1)
  {
    QString before = regex.cap(0);
    QString variable = regex.cap(1);
    QString after = this->Internals->Options.value(variable, variable);
    command.replace(before, after);
  }

  return this->processCommand(command, timeout, delay, &this->Internals->Options);
}

//-----------------------------------------------------------------------------
bool pqServerLauncher::processCommand(
  QString command, double timeout, double delay, const QProcessEnvironment* options)
{
  QProcess* process = new QProcess(pqApplicationCore::instance());

  if (options != NULL)
  {
    process->setProcessEnvironment(*options);
  }

  QObject::connect(process, SIGNAL(error(QProcess::ProcessError)), this,
    SLOT(processFailed(QProcess::ProcessError)));
  QObject::connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardError()));
  QObject::connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOutput()));

  process->start(command);

  // wait for process to start.
  // waitForStarted() may block until the process starts. That is generally a short
  // span of time, hence we don't worry about it too much.
  if (process->waitForStarted(timeout > 0. ? static_cast<int>(timeout * 1000.) : -1) == false)
  {
    qCritical() << "Command launch timed out.";
    process->kill();
    delete process;
    return false;
  }

  if (delay == -1)
  {
    // Wait for process to be finished
    while (process->state() == QProcess::Running)
    {
      process->waitForFinished(100);
    }
  }
  else
  {
    // wait for delay before attempting to connect to the server.
    pqEventDispatcher::processEventsAndWait(static_cast<int>(delay * 1000));
  }

  // Check process state
  if (process->state() != QProcess::Running)
  {
    if (process->exitStatus() != QProcess::NormalExit || process->exitCode() != 0)
    {
      // if the launched code exited with error, we consider that the process
      // failed. If the process quits with success, we
      // still assume that the script has launched the server successfully (it's
      // just treated as non-blocking).
      qCritical() << "Command aborted.";
      process->deleteLater();
      return false;
    }
    else
    {
      // process has completed, so delete it.
      process->deleteLater();
    }
  }
  else
  {
    // setup slot to delete the QProcess instance when the process exits.
    QObject::connect(
      process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqServerLauncher::processFailed(QProcess::ProcessError error_code)
{
  switch (error_code)
  {
    case QProcess::FailedToStart:
      qCritical() << "The process failed to start. Either the invoked program is missing, "
                     "or you may have insufficient permissions to invoke the program.";
      break;

    case QProcess::Crashed:
      qCritical() << "The process crashed some time after starting successfully.";
      break;

    default:
      qCritical() << "Process failed with error";
  }
}

//-----------------------------------------------------------------------------
pqServer* pqServerLauncher::connectedServer() const
{
  return this->Internals->Server;
}

//-----------------------------------------------------------------------------
void pqServerLauncher::readStandardOutput()
{
  QProcess* process = qobject_cast<QProcess*>(this->sender());
  if (process)
  {
    this->handleProcessStandardOutput(process->readAllStandardOutput());
    pqEventDispatcher::processEvents();
  }
}

//-----------------------------------------------------------------------------
void pqServerLauncher::readStandardError()
{
  QProcess* process = qobject_cast<QProcess*>(this->sender());
  if (process)
  {
    this->handleProcessErrorOutput(process->readAllStandardError());
    pqEventDispatcher::processEvents();
  }
}

//-----------------------------------------------------------------------------
void pqServerLauncher::handleProcessStandardOutput(const QByteArray& data)
{
  qDebug() << data.data();
}

//-----------------------------------------------------------------------------
void pqServerLauncher::handleProcessErrorOutput(const QByteArray& data)
{
  qCritical() << data.data();
}

//-----------------------------------------------------------------------------
QProcessEnvironment& pqServerLauncher::options() const
{
  return this->Internals->Options;
}
