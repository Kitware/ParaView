#include "SQLDatabaseTableSourcePanel.h"

#include "ui_SQLDatabaseTableSourcePanel.h"

#include <pqApplicationCore.h>
#include <pqPropertyHelper.h>
#include <pqProxy.h>
#include <pqSettings.h>

#include "vtkQtTableView.h"
#include "vtkSmartPointer.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkStdString.h"
#include "vtkStringArray.h"
#include <vtkSMProxy.h>
#include "vtkVariant.h"

#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QTableView>
#include <QtSql/QSqlDatabase>

#include <iostream>
#include <set>
#include <vtksys/SystemTools.hxx>

////////////////////////////////////////////////////////////////////////////////////
// SQLDatabaseTableSourcePanel::implementation

class SQLDatabaseTableSourcePanel::implementation
{
public:
  implementation()
  {
    this->DatabaseModel = new QStandardItemModel();
    this->Database = 0;
    this->TablePreviewResults = vtkSmartPointer<vtkRowQueryToTable>::New();
    this->TablePreview = vtkSmartPointer<vtkQtTableView>::New();
  }

  ~implementation()
  {
    if(this->Database)
      {
      this->Database->Delete();
      }
    delete this->DatabaseModel;
  }

  vtkSmartPointer<vtkQtTableView> TablePreview;
  vtkSmartPointer<vtkRowQueryToTable> TablePreviewResults;
  vtkSQLDatabase* Database;
  QStandardItemModel *DatabaseModel;
  Ui::SQLDatabaseTableSourcePanel Widgets;
};

SQLDatabaseTableSourcePanel::SQLDatabaseTableSourcePanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p),
  Implementation(new implementation())
{
  this->Implementation->Widgets.setupUi(this);

  this->Implementation->Widgets.databaseInfoGroupBox->layout()->addWidget(this->Implementation->TablePreview->GetWidget());
  this->Implementation->Widgets.databasePreview->setModel(this->Implementation->DatabaseModel);
  this->Implementation->Widgets.databasePreview->header()->hide();

  dynamic_cast<QTableView*>(this->Implementation->TablePreview->GetWidget())->setAlternatingRowColors(true);
  dynamic_cast<QTableView*>(this->Implementation->TablePreview->GetWidget())->setSortingEnabled(true);

  this->Implementation->Widgets.databaseType->addItem("psql");
  this->Implementation->Widgets.databaseType->addItem("mysql");
  this->Implementation->Widgets.databaseType->addItem("sqlite");
  this->Implementation->Widgets.databaseType->addItem("odbc");

  this->Implementation->Widgets.databaseFile->setForceSingleFile(true);

  // First populate database controls using settings
  this->loadDefaultSettings();

  // Now override these with any initial server manager state.
  this->loadInitialState();

  this->Implementation->Widgets.sqliteGroup->setVisible(this->Implementation->Widgets.databaseType->currentText() == "sqlite" ? 1 : 0);
  this->Implementation->Widgets.mysqlGroup->setVisible(this->Implementation->Widgets.databaseType->currentText() == "sqlite" ? 0 : 1);

  QObject::connect(this->Implementation->Widgets.databaseType, SIGNAL(currentIndexChanged(const QString&)), 
                  this, SLOT(onDatabaseTypeChanged(const QString&)));
  QObject::connect(this->Implementation->Widgets.databaseFile, SIGNAL(filenameChanged(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.databaseName, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.databaseUser, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.databasePassword, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.databaseHost, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.databasePort, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.query, SIGNAL(textChanged()), this, SLOT(setModified()));
//  QObject::connect(this->Implementation->Widgets.directed, SIGNAL(stateChanged(int)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.generatePedigreeIds, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.pedigreeIdArrayName, SIGNAL(textEdited(const QString&)), this, SLOT(setModified()));
  QObject::connect(this->Implementation->Widgets.getDatabaseInfo, SIGNAL(pressed()), this, SLOT(slotDatabaseInfo()));
  QObject::connect(this->Implementation->Widgets.databasePreview, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slotTableInfo(const QModelIndex &)));

}

SQLDatabaseTableSourcePanel::~SQLDatabaseTableSourcePanel()
{
  delete this->Implementation;
}

void SQLDatabaseTableSourcePanel::onDatabaseTypeChanged(const QString &databaseType)
{
  this->Implementation->Widgets.sqliteGroup->setVisible(databaseType == "sqlite" ? 1 : 0);
  this->Implementation->Widgets.mysqlGroup->setVisible(databaseType == "sqlite" ? 0 : 1);
  this->setModified();
}

void SQLDatabaseTableSourcePanel::slotDatabaseInfo()
{
  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  QString url;
  if(this->Implementation->Widgets.databaseType->currentText() == "sqlite")
    {
    url = this->Implementation->Widgets.databaseType->currentText() + QString("://") +
      this->Implementation->Widgets.databaseFile->singleFilename();
    }
  else
    {
    url = this->Implementation->Widgets.databaseType->currentText() + QString("://");
    if ( this->Implementation->Widgets.databaseUser->text().size() )
      { // only add "user[':'password]'@'" when username is present
      url += this->Implementation->Widgets.databaseUser->text();
      url += QString("@");
      }
    url += this->Implementation->Widgets.databaseHost->text();
    if ( this->Implementation->Widgets.databasePort->text().size() )
      { // Only add "':'port" when a port number is specified.
      url += QString(":") + this->Implementation->Widgets.databasePort->text();
      }
    url += QString("/") + this->Implementation->Widgets.databaseName->text();
    }

  if(this->Implementation->Database)
    this->Implementation->Database->Delete();

  this->Implementation->Database = vtkSQLDatabase::CreateFromURL(url.toAscii().data());
  if(!this->Implementation->Database)
    {
    return;
    }

  if(!this->Implementation->Database->Open(this->Implementation->Widgets.databasePassword->text().toAscii().data()))
    {
    return;
    }

  vtkStringArray *tableList = this->Implementation->Database->GetTables();
  int numTables = tableList->GetNumberOfValues();
  vtkSQLQuery *query = this->Implementation->Database->GetQueryInstance();
  QString queryString;
  
  // Clear out any old stuff
  this->Implementation->DatabaseModel->clear();
  
  // Check for MySQL databases (count(*) is very slow on mysql)
  bool mySQL = false;
  if (!strcmp(this->Implementation->Database->GetDatabaseType(),"QMYSQL")) 
    {
    mySQL = true;
    query->SetQuery("show table status");
    query->Execute();
    }
     
  // Populate the table info model
  QStandardItem *parentItem = this->Implementation->DatabaseModel->invisibleRootItem();
  for (int i=0; i<numTables; ++i)
    {
    // Get the table name
    QString tableName = QString(tableList->GetValue(i));
    
    // Get the number of rows in the table 
    int rowCount;
    if (!mySQL)
      {
      queryString.sprintf("Select count(*) from %s", tableName.toAscii().data());
      query->SetQuery(queryString.toAscii());
      query->Execute();
      query->NextRow();
      rowCount = query->DataValue(0).ToInt();
      }
    else
      {
      query->NextRow();
      rowCount = query->DataValue(4).ToInt();
      } 
    
    // Now add an item with two roles:
    // table name with count (for display)
    // and just table name to be used in 'auto-queries'
    QString tableInfo;
    tableInfo.sprintf("%s (%d)",tableName.toAscii().data(), rowCount);
    QStandardItem *item = new QStandardItem();
    item->setData(tableInfo, Qt::DisplayRole);
    item->setData(tableName, Qt::UserRole);  
    parentItem->appendRow(item);
    }
    
  // Okay now for each table populate the 
  // records within that table
  for (int i=0; i<numTables; ++i)
    { 
    // Get the table name
    QString tableName = QString(tableList->GetValue(i));
    
    // Okay get the item pointer for this table
    QStandardItem *item = parentItem->child(i);
    
    // Query executed just to get types
    if (!strcmp(this->Implementation->Database->GetDatabaseType(),"QOCI"))
    { 
    queryString.sprintf("Select * from %s where rownum < 1", tableName.toAscii().data());
    }
  else
    {
    queryString.sprintf("Select * from %s limit 1", tableName.toAscii().data());
    }
    query->SetQuery(queryString.toAscii());
    query->Execute();
    query->NextRow();
    
    // Table records
    vtkStringArray *recordList = this->Implementation->Database->GetRecord(tableName.toAscii());
   
    for (int j=0; j<recordList->GetNumberOfValues(); ++j)
      {
      QString recordName (recordList->GetValue(j));
      
      // Get the field type
      int fieldType = query->GetFieldType(j); 
      
      // Now add an item with two roles:
      // record name with type (for display)
      // and just record name to be used in 'auto-queries'
      QString typeString, recordInfo;
      typeString = vtkImageScalarTypeNameMacro(fieldType); 
      recordInfo.sprintf("%s [%s]",recordName.toAscii().data(), 
                                   typeString.toAscii().data());
      QStandardItem *recordItem = new QStandardItem;
      recordItem->setData(recordInfo, Qt::DisplayRole);
      recordItem->setData(recordName, Qt::UserRole);  
      item->appendRow(recordItem);
      }
    } 
    
  // Okay done with query so delete
  query->Delete();

}

void SQLDatabaseTableSourcePanel::slotTableInfo(const QModelIndex &index) 
{ 
  QString tableName;
  QString recordName;
  
  // Get the parent of this index
  QModelIndex parent = index.parent();
  
  // Do I have a parent?
  if (parent.isValid())
    {
    // I must be a record within a table
    tableName = parent.data(Qt::UserRole).toString();
    recordName = index.data(Qt::UserRole).toString();
    }
  else
    {
    // I am a table
    tableName = index.data(Qt::UserRole).toString();
    }
  
  // If there isn't a record name show all records
  QString showRecord;
  if (recordName.isEmpty())
    {
    showRecord = "*";
    }
  else
    {
    showRecord = recordName;
    }
  
  // Make the query to see the first 50 item in this table
  QString queryText;
  if (!strcmp(this->Implementation->Database->GetDatabaseType(),"QOCI"))
    { 
    queryText.sprintf("Select %s from %s where rownum < 50",showRecord.toAscii().data(),
                                             tableName.toAscii().data());
    }
  else
    {
    queryText.sprintf("Select %s from %s limit 50",showRecord.toAscii().data(),
                                             tableName.toAscii().data());
    }
  
  vtkSQLQuery *query = this->Implementation->Database->GetQueryInstance();
  query->SetQuery(queryText.toAscii().data());

  // Show a preview of this table (first 50 rows)
  // Give the query to the table reader
  this->Implementation->TablePreviewResults->SetQuery(query);
  this->Implementation->TablePreviewResults->Update();
  
  // Check for query error
  if (query->HasError())
    {
    return;
    }

  // Now hand off table to the results view
  this->Implementation->TablePreview->SetRepresentationFromInputConnection(
    this->Implementation->TablePreviewResults->GetOutputPort());
  
  // Okay done with query so delete
  query->Delete();
}

void SQLDatabaseTableSourcePanel::accept()
{
  vtkSMProxy* const proxy = this->referenceProxy()->getProxy();

  // The URL format for SQL databases is a true URL of the form:
  //   'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
  QString url;
  if(this->Implementation->Widgets.databaseType->currentText() == "sqlite")
    {
    url = this->Implementation->Widgets.databaseType->currentText() + QString("://") +
      this->Implementation->Widgets.databaseFile->singleFilename();
    }
  else
    {
    url = this->Implementation->Widgets.databaseType->currentText() + QString("://");
    if ( this->Implementation->Widgets.databaseUser->text().size() )
      { // only add "user[':'password]'@'" when username is present
      url += this->Implementation->Widgets.databaseUser->text();
      url += QString("@");
      }
    url += this->Implementation->Widgets.databaseHost->text();
    if ( this->Implementation->Widgets.databasePort->text().size() )
      { // Only add "':'port" when a port number is specified.
      url += QString(":") + this->Implementation->Widgets.databasePort->text();
      }
    url += QString("/") + this->Implementation->Widgets.databaseName->text();
    }

  pqPropertyHelper(proxy, "URL").Set(url);
  pqPropertyHelper(proxy, "Password").Set(this->Implementation->Widgets.databasePassword->text());
  pqPropertyHelper(proxy, "Query").Set(this->Implementation->Widgets.query->toPlainText());
  pqPropertyHelper(proxy, "PedigreeIdArrayName").Set(this->Implementation->Widgets.pedigreeIdArrayName->text());
  vtkSMPropertyHelper(proxy, "GeneratePedigreeIds").Set(this->Implementation->Widgets.generatePedigreeIds->currentIndex());

//  vtkSMPropertyHelper(proxy, "Directed").set(this->Implementation->Widgets.directed->isChecked());
 
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
  this->Implementation->Widgets.databaseType->setCurrentIndex(this->Implementation->Widgets.databaseType->findText(settings->value("DatabaseType", "sqlite").toString()));
  this->Implementation->Widgets.databaseFile->setSingleFilename(settings->value("DatabaseFile").toString());
  this->Implementation->Widgets.databaseName->setText(settings->value("DatabaseName").toString());
  this->Implementation->Widgets.databaseUser->setText(settings->value("DatabaseUser").toString());
  this->Implementation->Widgets.databaseHost->setText(settings->value("DatabaseHost").toString());
  this->Implementation->Widgets.databasePort->setText(settings->value("DatabasePort").toString());
  settings->endGroup();
}

void SQLDatabaseTableSourcePanel::saveDefaultSettings()
{
  pqSettings *settings = pqApplicationCore::instance()->settings();

  settings->beginGroup("OpenDatabase");
  settings->setValue("DatabaseType", this->Implementation->Widgets.databaseType->currentText());
  settings->setValue("DatabaseFile", this->Implementation->Widgets.databaseFile->singleFilename());
  settings->setValue("DatabaseName", this->Implementation->Widgets.databaseName->text());
  settings->setValue("DatabaseUser", this->Implementation->Widgets.databaseUser->text());
  settings->setValue("DatabaseHost", this->Implementation->Widgets.databaseHost->text());
  settings->setValue("DatabasePort", this->Implementation->Widgets.databasePort->text());
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
        //this->Implementation->Widgets.databaseFile->setFilename(tempfile);
        }
      }

    // Okay now for all the other database types get more detailed info
    if ( vtksys::SystemTools::ParseURL( url.toAscii().data(), protocol, username,
                                          unused, hostname, dataport, database) )
      {
      this->Implementation->Widgets.databaseType->setCurrentIndex(this->Implementation->Widgets.databaseType->findText(protocol.c_str()));
      this->Implementation->Widgets.databaseName->setText(database.c_str());
      this->Implementation->Widgets.databaseUser->setText(username.c_str());
      this->Implementation->Widgets.databaseHost->setText(hostname.c_str());
      this->Implementation->Widgets.databasePort->setText(dataport.c_str());
      }
    }

  this->Implementation->Widgets.databasePassword->setText(pqPropertyHelper(proxy, "Password").GetAsString());

  const QString query = pqPropertyHelper(proxy, "Query").GetAsString();
  if(!query.isEmpty())
    {
    this->Implementation->Widgets.query->setText(query);
    }

  this->Implementation->Widgets.pedigreeIdArrayName->setText(pqPropertyHelper(proxy, "PedigreeIdArrayName").GetAsString());
  this->Implementation->Widgets.generatePedigreeIds->setCurrentIndex(pqPropertyHelper(proxy, "GeneratePedigreeIds").GetAsInt());
}
