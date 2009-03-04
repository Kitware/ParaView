#include "SQLDatabaseGraphSourcePanel.h"

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

SQLDatabaseGraphSourcePanel::SQLDatabaseGraphSourcePanel(pqProxy* proxy, QWidget* p) :
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
  QObject::connect(this->Widgets.edgeQuery, SIGNAL(textChanged()), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexQuery, SIGNAL(textChanged()), this, SLOT(setModified()));

  QObject::connect(this->Widgets.Directed, SIGNAL(stateChanged(int)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexField1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexField4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexDomain1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexDomain4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.vertexHidden1, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden2, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden3, SIGNAL(clicked(bool)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.vertexHidden4, SIGNAL(clicked(bool)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeSource1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeSource4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgeTarget1, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget2, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget3, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.edgeTarget4, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));

  QObject::connect(this->Widgets.edgePedigreeIdArrayName, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Widgets.generateEdgePedigreeIds, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
}

void SQLDatabaseGraphSourcePanel::onDatabaseTypeChanged(const QString &databaseType)
{
  this->Widgets.sqliteGroup->setVisible(databaseType == "sqlite" ? 1 : 0);
  this->Widgets.mysqlGroup->setVisible(databaseType == "sqlite" ? 0 : 1);
  this->setModified();
}

void SQLDatabaseGraphSourcePanel::accept()
{
/*
  const QStringList link_edges = this->Widgets.edgeList->text().split(',',QString::SkipEmptyParts);
  if(link_edges.size() % 2)
    {
    QMessageBox::critical(this, "Table-to-graph", "You must provide an even number of fields.");
    return;
    }
*/
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
      { // only add "user'@'" when username is present
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
  pqPropertyHelper(proxy, "EdgeQuery").Set(this->Widgets.edgeQuery->toPlainText());
  pqPropertyHelper(proxy, "VertexQuery").Set(this->Widgets.vertexQuery->toPlainText());

  QStringList link_vertices;
  if(!this->Widgets.vertexField1->text().isEmpty() && !this->Widgets.vertexDomain1->text().isEmpty())
    link_vertices << this->Widgets.vertexField1->text() << this->Widgets.vertexDomain1->text() << (this->Widgets.vertexHidden1->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField2->text().isEmpty() && !this->Widgets.vertexDomain2->text().isEmpty())
    link_vertices << this->Widgets.vertexField2->text() << this->Widgets.vertexDomain2->text() << (this->Widgets.vertexHidden2->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField3->text().isEmpty() && !this->Widgets.vertexDomain3->text().isEmpty())
    link_vertices << this->Widgets.vertexField3->text() << this->Widgets.vertexDomain3->text() << (this->Widgets.vertexHidden3->isChecked() ? "1" : "0");
  if(!this->Widgets.vertexField4->text().isEmpty() && !this->Widgets.vertexDomain4->text().isEmpty())
    link_vertices << this->Widgets.vertexField4->text() << this->Widgets.vertexDomain4->text() << (this->Widgets.vertexHidden4->isChecked() ? "1" : "0");

  QStringList link_edges;
  if(!this->Widgets.edgeSource1->text().isEmpty() && !this->Widgets.edgeTarget1->text().isEmpty())
    link_edges << this->Widgets.edgeSource1->text() << this->Widgets.edgeTarget1->text();
  if(!this->Widgets.edgeSource2->text().isEmpty() && !this->Widgets.edgeTarget2->text().isEmpty())
    link_edges << this->Widgets.edgeSource2->text() << this->Widgets.edgeTarget2->text();
  if(!this->Widgets.edgeSource3->text().isEmpty() && !this->Widgets.edgeTarget3->text().isEmpty())
    link_edges << this->Widgets.edgeSource3->text() << this->Widgets.edgeTarget3->text();
  if(!this->Widgets.edgeSource4->text().isEmpty() && !this->Widgets.edgeTarget4->text().isEmpty())
    link_edges << this->Widgets.edgeSource4->text() << this->Widgets.edgeTarget4->text();

  pqPropertyHelper(proxy, "LinkVertices").Set(link_vertices);
  pqPropertyHelper(proxy, "LinkEdges").Set(link_edges);
  pqPropertyHelper(proxy, "EdgePedigreeIdArrayName").Set(this->Widgets.edgePedigreeIdArrayName->text());
  vtkSMPropertyHelper(proxy, "GenerateEdgePedigreeIds").Set(this->Widgets.generateEdgePedigreeIds->currentIndex());

  vtkSMPropertyHelper(proxy, "Directed").Set(this->Widgets.Directed->isChecked());
 
  proxy->UpdateVTKObjects();

  this->saveDefaultSettings();

  Superclass::accept();    
}

void SQLDatabaseGraphSourcePanel::reset()
{
  Superclass::reset();
}

void SQLDatabaseGraphSourcePanel::loadDefaultSettings()
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

void SQLDatabaseGraphSourcePanel::saveDefaultSettings()
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


void SQLDatabaseGraphSourcePanel::loadInitialState()
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

  const QString edgeQuery = pqPropertyHelper(proxy, "EdgeQuery").GetAsString();
  if(!edgeQuery.isEmpty())
    {
    this->Widgets.edgeQuery->setText(edgeQuery);
    }

  const QString vertexQuery = pqPropertyHelper(proxy, "VertexQuery").GetAsString();
  if(!vertexQuery.isEmpty())
    {
    this->Widgets.vertexQuery->setText(vertexQuery);
    }


  const QStringList link_vertices = pqPropertyHelper(proxy, "LinkVertices").GetAsStringList();
  if(link_vertices.size() > 2)
    {
    this->Widgets.vertexField1->setText(link_vertices[0]);
    this->Widgets.vertexDomain1->setText(link_vertices[1]);
    this->Widgets.vertexHidden1->setChecked(link_vertices[2] == "1");
    }
  if(link_vertices.size() > 5)
    {
    this->Widgets.vertexField2->setText(link_vertices[3]);
    this->Widgets.vertexDomain2->setText(link_vertices[4]);
    this->Widgets.vertexHidden2->setChecked(link_vertices[5] == "1");
    }
  if(link_vertices.size() > 8)
    {
    this->Widgets.vertexField3->setText(link_vertices[6]);
    this->Widgets.vertexDomain3->setText(link_vertices[7]);
    this->Widgets.vertexHidden3->setChecked(link_vertices[8] == "1");
    }
  if(link_vertices.size() > 11)
    {
    this->Widgets.vertexField4->setText(link_vertices[9]);
    this->Widgets.vertexDomain4->setText(link_vertices[10]);
    this->Widgets.vertexHidden4->setChecked(link_vertices[11] == "1");
    }

  const QStringList link_edges = pqPropertyHelper(proxy, "LinkEdges").GetAsStringList();
  if(link_edges.size() > 1)
    {
    this->Widgets.edgeSource1->setText(link_edges[0]);
    this->Widgets.edgeTarget1->setText(link_edges[1]);
    }
  if(link_edges.size() > 3)
    {
    this->Widgets.edgeSource2->setText(link_edges[2]);
    this->Widgets.edgeTarget2->setText(link_edges[3]);
    }
  if(link_edges.size() > 5)
    {
    this->Widgets.edgeSource3->setText(link_edges[4]);
    this->Widgets.edgeTarget3->setText(link_edges[5]);
    }
  if(link_edges.size() > 7)
    {
    this->Widgets.edgeSource4->setText(link_edges[6]);
    this->Widgets.edgeTarget4->setText(link_edges[7]);
    }

  this->Widgets.edgePedigreeIdArrayName->setText(pqPropertyHelper(proxy, "EdgePedigreeIdArrayName").GetAsString());
  this->Widgets.generateEdgePedigreeIds->setCurrentIndex(pqPropertyHelper(proxy, "GenerateEdgePedigreeIds").GetAsInt());
}
