#include "SQLDatabaseTableSourcePanel.h"

#include <pqApplicationCore.h>
#include <pqPropertyHelper.h>
#include <pqProxy.h>
#include <pqSettings.h>

#include "vtkStdString.h"
#include <vtkSMProxy.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QtSql/QSqlDatabase>

#include <iostream>
#include <set>
#include <vtksys/SystemTools.hxx>

SQLDatabaseTableSourcePanel::SQLDatabaseTableSourcePanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  this->Widgets.databaseType->addItem("psql");
  this->Widgets.databaseType->addItem("mysql");
  this->Widgets.databaseType->addItem("sqlite");

  this->Widgets.databaseFile->setForceSingleFile(true);

  // First populate database controls using settings
  this->loadDefaultSettings();

  // Now override these with any initial server manager state.
  this->loadInitialState();

  this->Widgets.sqliteGroup->setVisible(this->Widgets.databaseType->currentText() == "sqlite" ? 1 : 0);
  this->Widgets.mysqlGroup->setVisible(this->Widgets.databaseType->currentText() == "sqlite" ? 0 : 1);

  QObject::connect(this->Widgets.databaseType, SIGNAL(currentIndexChanged(const QString&)), 
                  this, SLOT(onDatabaseTypeChanged(const QString&)));
  QObject::connect(this->Widgets.databaseFile, SIGNAL(filenameChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.databaseName, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.databaseUser, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.databasePassword, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.databaseHost, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.databasePort, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.query, SIGNAL(textChanged()), this, SLOT(setModified()));
//  QObject::connect(this->Widgets.directed, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.generatePedigreeIds, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.pedigreeIdArrayName, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
}

void SQLDatabaseTableSourcePanel::onDatabaseTypeChanged(const QString &databaseType)
{
  this->Widgets.sqliteGroup->setVisible(databaseType == "sqlite" ? 1 : 0);
  this->Widgets.mysqlGroup->setVisible(databaseType == "sqlite" ? 0 : 1);
  this->setModified();
}

void SQLDatabaseTableSourcePanel::accept()
{
  vtkSMProxy* const proxy = this->referenceProxy()->getProxy();

  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  QString url;
  if(this->Widgets.databaseType->currentText() == "sqlite")
    {
    url = this->Widgets.databaseType->currentText() + QString("://") +
      this->Widgets.databaseFile->singleFilename();
    }
  else
    {
    url = this->Widgets.databaseType->currentText() + QString("://");
    if ( this->Widgets.databaseUser->text().size() )
      { // only add "user[':'password]'@'" when username is present
      url += this->Widgets.databaseUser->text();
      url += QString("@");
      }
    url += this->Widgets.databaseHost->text();
    if ( this->Widgets.databasePort->text().size() )
      { // Only add "':'port" when a port number is specified.
      url += QString(":") + this->Widgets.databasePort->text();
      }
    url += QString("/") + this->Widgets.databaseName->text();
    }

  pqPropertyHelper(proxy, "URL").Set(url);
  pqPropertyHelper(proxy, "Password").Set(this->Widgets.databasePassword->text());
  pqPropertyHelper(proxy, "Query").Set(this->Widgets.query->toPlainText());
  pqPropertyHelper(proxy, "PedigreeIdArrayName").Set(this->Widgets.pedigreeIdArrayName->text());
  vtkSMPropertyHelper(proxy, "GeneratePedigreeIds").Set(this->Widgets.generatePedigreeIds->currentIndex());

//  vtkSMPropertyHelper(proxy, "Directed").set(this->Widgets.directed->isChecked());
 
  proxy->UpdateVTKObjects();

  this->saveDefaultSettings();

  Superclass::accept();    
}

void SQLDatabaseTableSourcePanel::reset()
{
  Superclass::reset();
}

void SQLDatabaseTableSourcePanel::loadDefaultSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();
  
  settings->beginGroup("OpenDatabase");
  this->Widgets.databaseType->setCurrentIndex(this->Widgets.databaseType->findText(settings->value("DatabaseType", "sqlite").toString()));
  this->Widgets.databaseFile->setSingleFilename(settings->value("DatabaseFile").toString());
  this->Widgets.databaseName->setText(settings->value("DatabaseName").toString());
  this->Widgets.databaseUser->setText(settings->value("DatabaseUser").toString());
  this->Widgets.databaseHost->setText(settings->value("DatabaseHost").toString());
  this->Widgets.databasePort->setText(settings->value("DatabasePort").toString());
  settings->endGroup();
}

void SQLDatabaseTableSourcePanel::saveDefaultSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("OpenDatabase");
  settings->setValue("DatabaseType", this->Widgets.databaseType->currentText());
  settings->setValue("DatabaseFile", this->Widgets.databaseFile->singleFilename());
  settings->setValue("DatabaseName", this->Widgets.databaseName->text());
  settings->setValue("DatabaseUser", this->Widgets.databaseUser->text());
  settings->setValue("DatabaseHost", this->Widgets.databaseHost->text());
  settings->setValue("DatabasePort", this->Widgets.databasePort->text());
  settings->endGroup();
}


void SQLDatabaseTableSourcePanel::loadInitialState()
{
  vtkSMProxy *proxy = this->referenceProxy()->getProxy();

  const QString url = pqPropertyHelper(proxy, "URL").GetAsString();
  if(!url.isEmpty())
    {
    vtkstd::string protocol;
    vtkstd::string username; 
    vtkstd::string unused;
    vtkstd::string hostname; 
    vtkstd::string dataport; 
    vtkstd::string database;
    vtkstd::string dataglom;
    
    // SQLite is a bit special so lets get that out of the way :)
    if ( vtksys::SystemTools::ParseURLProtocol( url.toAscii().data(), protocol, dataglom))
      {
      if ( protocol == "sqlite" )
        {
        // write the data to a temp file and use that as the filename?
        //this->Widgets.databaseFile->setFilename(tempfile);
        }
      }

    // Okay now for all the other database types get more detailed info
    if ( vtksys::SystemTools::ParseURL( url.toAscii().data(), protocol, username,
                                          unused, hostname, dataport, database) )
      {
      this->Widgets.databaseType->setCurrentIndex(this->Widgets.databaseType->findText(protocol.c_str()));
      this->Widgets.databaseName->setText(database.c_str());
      this->Widgets.databaseUser->setText(username.c_str());
      this->Widgets.databaseHost->setText(hostname.c_str());
      this->Widgets.databasePort->setText(dataport.c_str());
      }
    }

  this->Widgets.databasePassword->setText(pqPropertyHelper(proxy, "Password").GetAsString());

  const QString query = pqPropertyHelper(proxy, "Query").GetAsString();
  if(!query.isEmpty())
    {
    this->Widgets.query->setText(query);
    }

  this->Widgets.pedigreeIdArrayName->setText(pqPropertyHelper(proxy, "PedigreeIdArrayName").GetAsString());
  this->Widgets.generatePedigreeIds->setCurrentIndex(pqPropertyHelper(proxy, "GeneratePedigreeIds").GetAsInt());
}
