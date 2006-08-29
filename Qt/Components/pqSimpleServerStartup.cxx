/*=========================================================================

   Program: ParaView
   Module:    pqSimpleServerStartup.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqEditServerStartupDialog.h"
#include "pqServerStartupDialog.h"
#include "pqSimpleServerStartup.h"

#include <pqApplicationCore.h>
#include <pqCommandServerStartup.h>
#include <pqCreateServerStartupDialog.h>
#include <pqEditServerStartupDialog.h>
#include <pqServer.h>
#include <pqServerBrowser.h>
#include <pqServerManagerModel.h>
#include <pqServerResource.h>
#include <pqServerStartupContext.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>

#include <vtkProcessModule.h>
#include <vtkProcessModuleConnectionManager.h>
#include <vtkMath.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QtDebug>

#include <vtkstd/map>

//////////////////////////////////////////////////////////////////////////////
// pqSimpleServerStartup::pqImplementation

class pqSimpleServerStartup::pqImplementation
{
public:
  pqImplementation() :
    Startup(0),
    StartupContext(0),
    StartupDialog(0),
    ReverseConnectionID(0)
  {
    this->Timer.setInterval(10);
  }
  
  ~pqImplementation()
  {
    this->reset();
  }

  void reset()
  {
    this->Startup = 0;
    this->Timer.stop();

    delete this->StartupContext;
    this->StartupContext = 0;

    delete this->StartupDialog;
    this->StartupDialog = 0;
    
    this->ReverseConnectionID = 0;
    this->Options.clear();
    this->Server = pqServerResource();
  }

  /// Stores a reference to the startup configuration to be used
  pqServerStartup* Startup;
  /// Used to check the reverse-connection status periodically
  QTimer Timer;
  /// Used to track the progress of a startup as it executes
  pqServerStartupContext* StartupContext;
  /// Modal dialog used to display progress during startup
  pqServerStartupDialog* StartupDialog;
  /// Connection identifier returned by the server manager during reverse-connection startup
  int ReverseConnectionID;
  /// Stores options defined by the user prior to startup
  pqServerStartup::OptionsT Options;
  /** Stores a complete description of the server to be started
  (differs from the startup server if the user has chosen nonstandard ports */
  pqServerResource Server;
};

//////////////////////////////////////////////////////////////////////////////
// pqSimpleServerStartup

pqSimpleServerStartup::pqSimpleServerStartup(QObject* p) :
  Superclass(p),
  Implementation(new pqImplementation())
{
  QObject::connect(
    &this->Implementation->Timer,
    SIGNAL(timeout()),
    this,
    SLOT(monitorReverseConnections()));
}

pqSimpleServerStartup::~pqSimpleServerStartup()
{
  delete this->Implementation;
}

void pqSimpleServerStartup::startServer(pqServerStartup& startup)
{
  // Get the named startup ...
  this->Implementation->Startup = &startup;
  this->Implementation->Server = startup.getServer();
      
  // Prompt the user for runtime server arguments ...
  if(!this->promptRuntimeArguments())
    {
    this->cancelled();
    return;
    }    

  // Branch based on the connection type - builtin, forward, or reverse ...
  if(startup.getServer().scheme() == "builtin")
    {
    this->startBuiltinConnection();
    }
  else if(startup.getServer().scheme() == "cs" || startup.getServer().scheme() == "cdsrs")
    {
    this->startForwardConnection();
    }
  else if(startup.getServer().scheme() == "csrc" || startup.getServer().scheme() == "cdsrsrc")
    {
    this->startReverseConnection();
    }
  else
    {
    qCritical() << "Unknown server scheme: " << startup.getServer().scheme();
    this->failed();
    }
}

void pqSimpleServerStartup::startServer(
  pqServerStartups& server_startups,
  pqSettings& settings,
  const pqServerResource& server)
{
  // There may be zero, one, or more-than-one startup already configured for this server
  const pqServerStartups::StartupsT startups =
    server_startups.getStartups(server);
    
  // No startup, yet, so prompt the user to create one ...
  if(0 == startups.size())
    {
    pqCreateServerStartupDialog create_server_dialog(server);
    if(QDialog::Accepted == create_server_dialog.exec())
      {
      pqEditServerStartupDialog edit_server_dialog(
        server_startups,
        create_server_dialog.getName(),
        create_server_dialog.getServer());
      if(QDialog::Accepted == edit_server_dialog.exec())
        {
        server_startups.save(settings);
        if(pqServerStartup* const startup = server_startups.getStartup(
          create_server_dialog.getName()))
          {
          this->startServer(*startup);
          }
        }
      }
    }
  // Exactly one startup, so just use it already ...
  else if(1 == startups.size())
    {
    if(pqServerStartup* const startup = server_startups.getStartup(startups[0]))
      {
      this->startServer(*startup);
      }
    }
  // More than one startup, so prompt the user to pick one ...
  else
    {
    pqServerBrowser dialog(server_startups, settings);
    dialog.setMessage(QString(tr("Pick the configuration for starting %1")).arg(server.schemeHosts().toString()));
    
    if(QDialog::Accepted == dialog.exec())
      {
      if(dialog.getSelectedServer())
        {
        this->startServer(*dialog.getSelectedServer());
        }
      }
    else
      {
      this->cancelled();
      }
    }
}

void pqSimpleServerStartup::cancelled()
{
  this->Implementation->reset();
  emit this->serverCancelled();
}

void pqSimpleServerStartup::failed()
{
  this->Implementation->reset();
  emit this->serverFailed();
}

void pqSimpleServerStartup::started(pqServer* server)
{
  this->Implementation->reset();
  emit this->serverStarted(server);
}

bool pqSimpleServerStartup::promptRuntimeArguments()
{
  // Never prompt for the builtin server
  const pqServerResource& server = this->Implementation->Server;
  if(server.scheme() == "builtin")
    {
    return true;
    }

  QDialog dialog;
  dialog.setWindowTitle("Start " + this->Implementation->Server.toString());
  
  QGridLayout* const layout = new QGridLayout();
  dialog.setLayout(layout);

  const int label_column = 0;
  const int widget_column = 1;
  
  typedef vtkstd::map<QString, QWidget*> widgets_t;
  widgets_t widgets;
  
  vtkstd::map<QString, QString> true_values;
  vtkstd::map<QString, QString> false_values;
  
  // Prompt for port numbers ...
  if(server.scheme() == "cs" || server.scheme() == "csrc")
    {
    layout->addWidget(new QLabel("Port:"), 0, label_column, Qt::AlignLeft | Qt::AlignVCenter);

    QSpinBox* const widget = new QSpinBox();
    widget->setRange(1, 65535);
    widget->setValue(11111);
    layout->addWidget(widget, 0, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
    widgets["PV_SERVER_PORT"] = widget;
    }
  else if(server.scheme() == "cdsrs" || server.scheme() == "cdsrsrc")
    {
    layout->addWidget(new QLabel("Data Server Port:"), 0, label_column, Qt::AlignLeft | Qt::AlignVCenter);
    QSpinBox* const data_widget = new QSpinBox();
    data_widget->setRange(1, 65535);
    data_widget->setValue(11111);
    layout->addWidget(data_widget, 0, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
    widgets["PV_DATA_SERVER_PORT"] = data_widget;

    layout->addWidget(new QLabel("Render Server Port:"), 1, label_column, Qt::AlignLeft | Qt::AlignVCenter);
    QSpinBox* const render_widget = new QSpinBox();
    render_widget->setRange(1, 65535);
    render_widget->setValue(22221);
    layout->addWidget(render_widget, 1, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
    widgets["PV_RENDER_SERVER_PORT"] = render_widget;
    }  

  // Prompt for configured options ...  
  QDomDocument xml = this->Implementation->Startup->getConfiguration();
  QDomElement xml_command_startup = xml.firstChildElement("CommandStartup");
  if(!xml_command_startup.isNull())
    {
    QDomElement xml_options = xml_command_startup.firstChildElement("Options");
    if(!xml_options.isNull())
      {
      for(QDomNode xml_option = xml_options.firstChild(); !xml_option.isNull(); xml_option = xml_option.nextSibling())
        {
        if(xml_option.isElement() && xml_option.toElement().tagName() == "Option")
          {
          const QString option_name = xml_option.toElement().attribute("name");
          const QString option_label = xml_option.toElement().attribute("label");
          const bool option_readonly = xml_option.toElement().attribute("readonly") == "true";

          QDomNode xml_type = xml_option.firstChildElement();
          if(xml_type.isElement())
            {
            const int row = layout->rowCount();
            
            layout->addWidget(new QLabel(option_label + ":"), row, label_column, Qt::AlignLeft | Qt::AlignVCenter);
            
            if(xml_type.toElement().tagName() == "Range")
              {
              const QString range_type = xml_type.toElement().attribute("type");
              if(range_type == "int")
                {
                const QString widget_min = xml_type.toElement().attribute("min");
                const QString widget_max = xml_type.toElement().attribute("max");
                const QString widget_step = xml_type.toElement().attribute("step");
                const QString widget_default = xml_type.toElement().attribute("default");

                QSpinBox* const widget = new QSpinBox();
                widget->setMinimum(widget_min.toInt());
                widget->setMaximum(widget_max.toInt());
                widget->setSingleStep(widget_step.toInt());
                if(widget_default == "random")
                  {
                  widget->setValue(static_cast<int>(
                    vtkMath::Random(widget_min.toInt(), widget_max.toInt())));
                  }
                else
                  {
                  widget->setValue(widget_default.toInt());
                  }
                widget->setEnabled(!option_readonly);
                
                layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
                widgets[option_name] = widget;
                }
              else if(range_type == "double")
                {
                const QString widget_min = xml_type.toElement().attribute("min");
                const QString widget_max = xml_type.toElement().attribute("max");
                const QString widget_step = xml_type.toElement().attribute("step");
                const QString widget_precision = xml_type.toElement().attribute("precision");
                const QString widget_default = xml_type.toElement().attribute("default");

                QDoubleSpinBox* const widget = new QDoubleSpinBox();
                widget->setMinimum(widget_min.toDouble());
                widget->setMaximum(widget_max.toDouble());
                widget->setSingleStep(widget_step.toDouble());
                widget->setDecimals(widget_precision.toInt());
                widget->setValue(widget_default.toDouble());
                widget->setEnabled(!option_readonly);
                
                layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
                widgets[option_name] = widget;
                }
              }
            else if(xml_type.toElement().tagName() == "String")
              {
              const QString widget_default = xml_type.toElement().attribute("default");
              QLineEdit* const widget = new QLineEdit(widget_default);
              widget->setEnabled(!option_readonly);
              layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
              widgets[option_name] = widget;
              }
            else if(xml_type.toElement().tagName() == "Boolean")
              {
              const QString widget_true = xml_type.toElement().attribute("true");
              const QString widget_false = xml_type.toElement().attribute("false");
              const QString widget_default = xml_type.toElement().attribute("default");
              
              QCheckBox* const widget = new QCheckBox();
              widget->setChecked(widget_default == "true");
              widget->setEnabled(!option_readonly);
              
              layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
              widgets[option_name] = widget;
              true_values[option_name] = widget_true;
              false_values[option_name] = widget_false;
              }
            else if(xml_type.toElement().tagName() == "Enumeration")
              {
              const QString widget_default = xml_type.toElement().attribute("default");
              QComboBox* const widget = new QComboBox();

              for(QDomNode xml_enumeration = xml_type.firstChild(); !xml_enumeration.isNull(); xml_enumeration = xml_enumeration.nextSibling())
                {
                if(xml_enumeration.isElement() && xml_enumeration.toElement().tagName() == "Entry") 
                  {
                  const QString xml_value = xml_enumeration.toElement().attribute("value");
                  const QString xml_label = xml_enumeration.toElement().attribute("label");
                  widget->addItem(xml_label, xml_value);
                  }
                }
              
              widget->setCurrentIndex(widget->findData(widget_default));
              widget->setEnabled(!option_readonly);
              
              layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
              widgets[option_name] = widget;
              }
            }
          }
        }
      }
    }
    
  layout->setRowStretch(layout->rowCount(), 1);
  
  QHBoxLayout* const button_layout = new QHBoxLayout();
  button_layout->addStretch(1);
  
  QPushButton* const ok_button = new QPushButton(tr("Connect"));
  QObject::connect(ok_button, SIGNAL(clicked()), &dialog, SLOT(accept()));
  button_layout->addWidget(ok_button);
  
  QPushButton* const cancel_button = new QPushButton(tr("Cancel"));
  QObject::connect(cancel_button, SIGNAL(clicked()), &dialog, SLOT(reject()));
  button_layout->addWidget(cancel_button);
  
  layout->addLayout(button_layout, layout->rowCount(), 0, 1, 2);
  
  if(QDialog::Accepted != dialog.exec())
    return false;

  for(widgets_t::const_iterator option = widgets.begin(); option != widgets.end(); ++option)
    {
    const QString option_name = option->first;
    QWidget* const option_widget = option->second;
    
    if(QSpinBox* const widget1 = qobject_cast<QSpinBox*>(option_widget))
      {
      this->Implementation->Options[option_name] = QString::number(widget1->value());
      }
    else if(QDoubleSpinBox* const widget2 = qobject_cast<QDoubleSpinBox*>(option_widget))
      {
      this->Implementation->Options[option_name] = QString::number(widget2->value());
      }
    else if(QLineEdit* const widget3 = qobject_cast<QLineEdit*>(option_widget))
      {
      this->Implementation->Options[option_name] = widget3->text();
      }
    else if(QCheckBox* const widget4 = qobject_cast<QCheckBox*>(option_widget))
      {
      this->Implementation->Options[option_name] = widget4->isChecked() ? true_values[option_name] : false_values[option_name];
      }
    else if(QComboBox* const widget5 = qobject_cast<QComboBox*>(option_widget))
      {
      this->Implementation->Options[option_name] = widget5->itemData(widget5->currentIndex()).toString();
      }
    }

  if(!this->Implementation->Options["PV_SERVER_PORT"].isEmpty())
    {
    this->Implementation->Server.setPort(this->Implementation->Options["PV_SERVER_PORT"].toInt());
    }
  if(!this->Implementation->Options["PV_DATA_SERVER_PORT"].isEmpty())
    {
    this->Implementation->Server.setDataServerPort(this->Implementation->Options["PV_DATA_SERVER_PORT"].toInt());
    }
  if(!this->Implementation->Options["PV_RENDER_SERVER_PORT"].isEmpty())
    {
    this->Implementation->Server.setRenderServerPort(this->Implementation->Options["PV_RENDER_SERVER_PORT"].toInt());
    }

  return true;
}

void pqSimpleServerStartup::startBuiltinConnection()
{
  if(pqServer* const existing_server =
    pqApplicationCore::instance()->getServerManagerModel()->getServer(
      this->Implementation->Server))
    {
    this->started(existing_server);
    return;
    }

  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server);
  this->Implementation->StartupDialog->show();

  pqServer* const server = pqApplicationCore::instance()->createServer(
    pqServerResource("builtin:"));

  this->Implementation->StartupDialog->hide();
    
  if(server)
    {
    this->started(server);
    }
  else
    {
    this->failed();
    }
}

void pqSimpleServerStartup::startForwardConnection()
{
  if(pqServer* const existing_server =
    pqApplicationCore::instance()->getServerManagerModel()->getServer(
      this->Implementation->Server))
    {
    this->started(existing_server);
    return;
    }

  this->Implementation->StartupContext = new pqServerStartupContext();
  
  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server);
  this->Implementation->StartupDialog->show();
  
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(succeeded()),
    this,
    SLOT(forwardConnectServer()));
    
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(succeeded()),
    this->Implementation->StartupDialog,
    SLOT(hide()));
    
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(failed()),
    this,
    SLOT(failed()));
  
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(failed()),
    this->Implementation->StartupDialog,
    SLOT(hide()));
  
  this->Implementation->Startup->execute(
    this->Implementation->Options,
    *this->Implementation->StartupContext);
}

void pqSimpleServerStartup::forwardConnectServer()
{
  if(pqServer* const server = pqApplicationCore::instance()->createServer(
    this->Implementation->Server))
    {
    this->started(server);
    }
  else
    {
    this->failed();
    }
}

void pqSimpleServerStartup::startReverseConnection()
{
  if(pqServer* const existing_server =
    pqApplicationCore::instance()->getServerManagerModel()->getServer(
      this->Implementation->Server))
    {
    this->started(existing_server);
    return;
    }

  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(finishReverseConnection(pqServer*)));
  
  if(this->Implementation->Server.scheme() == "csrc")
    {
    process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.port(11111));
    }
  else if(this->Implementation->Server.scheme() == "cdsrsrc")
    {
    int dsid, rsid;
    process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.dataServerPort(11111),
      this->Implementation->Server.renderServerPort(22221),
      dsid,
      rsid);
    }
    
  this->Implementation->StartupContext = new pqServerStartupContext();

  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server);
  this->Implementation->StartupDialog->show();

  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(succeeded()),
    &this->Implementation->Timer,
    SLOT(start()));
    
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(failed()),
    this,
    SLOT(failed()));
  
  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(failed()),
    this->Implementation->StartupDialog,
    SLOT(hide()));

  QObject::connect(
    this->Implementation->StartupContext,
    SIGNAL(failed()),
    &this->Implementation->Timer,
    SLOT(stop()));

  this->Implementation->Startup->execute(
    this->Implementation->Options,
    *this->Implementation->StartupContext);
}

void pqSimpleServerStartup::monitorReverseConnections()
{
  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  if(-1 == process_module->MonitorConnections(10))
    {
    this->Implementation->Timer.stop();
    this->Implementation->StartupDialog->hide();
    this->failed();
    }
}

void pqSimpleServerStartup::finishReverseConnection(pqServer* server)
{
  QObject::disconnect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(finishReverseConnection(pqServer*)));

  server->setResource(this->Implementation->Server);

  this->Implementation->Timer.stop();
  this->Implementation->StartupDialog->hide();
  this->started(server);
}
