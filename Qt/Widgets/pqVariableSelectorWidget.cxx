/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "pqVariableSelectorWidget.h"

#include <QComboBox>
#include <QHBoxLayout>

pqVariableSelectorWidget::pqVariableSelectorWidget( QWidget *p ) 
  : QWidget( p )
{
  this->Layout  = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(6);

  this->Type = new QComboBox( this );
  this->Type->setObjectName("VarType");
  this->Type->setMinimumSize( QSize( 50, 20 ) );
  this->Type->insertItem(TYPE_CELL,  "CELL");
  this->Type->insertItem(TYPE_NODE, "POINT");
  this->Type->setCurrentIndex( TYPE_CELL );
  this->PrevType = "CELL";

  this->Name[TYPE_CELL] = new QComboBox( this );
  this->Name[TYPE_CELL]->setObjectName("CellVars");
  this->Name[TYPE_CELL]->setMinimumSize( QSize( 150, 20 ) );
  this->Name[TYPE_NODE] = new QComboBox( this );
  this->Name[TYPE_NODE]->setObjectName("NodeVars");
  this->Name[TYPE_NODE]->setMinimumSize( QSize( 150, 20 ) );
  this->PrevName[TYPE_CELL] = "";
  this->PrevName[TYPE_NODE] = "";

  // this->Spacer = new QSpacerItem( 10, 10, QSizePolicy::Expanding, 
      // QSizePolicy::Minimum );

  this->Layout->setMargin( 0 );
  this->Layout->setSpacing( 1 );
  this->Layout->addWidget( this->Type );
  this->Layout->addWidget( this->Name[TYPE_CELL] );
  this->Layout->addWidget( this->Name[TYPE_NODE] );
  // this->Layout->addItem( this->Spacer );
  this->Name[TYPE_NODE]->hide();

  connect( Type, 
      SIGNAL( activated( const QString & ) ),  
      SLOT( typeActivated( const QString & ) ) );
  connect( Name[TYPE_CELL], 
      SIGNAL( activated( const QString & ) ),  
      SLOT( nameActivated( const QString & ) ) );
  connect( Name[TYPE_NODE], 
      SIGNAL( activated( const QString & ) ),  
      SLOT( nameActivated( const QString & ) ) );
}

pqVariableSelectorWidget::~pqVariableSelectorWidget()
{
  delete this->Layout;
  this->Layout = NULL;
  delete this->Type;
  this->Type = NULL;
  delete this->Name[TYPE_CELL];
  this->Name[TYPE_CELL] = NULL;
  delete this->Name[TYPE_NODE];
  this->Name[TYPE_NODE] = NULL;
}

void pqVariableSelectorWidget::typeActivated( const QString &type ) 
{
  if ( type != this->PrevType )
    {
    // show the correct names
    if ( type == "CELL" ) 
      {
      this->Name[TYPE_CELL]->show();
      this->Name[TYPE_NODE]->hide();
      }
    else if ( type == "POINT" )
      {
      this->Name[TYPE_CELL]->hide();
      this->Name[TYPE_NODE]->show();
      }
    this->PrevType = type;
    emit varTypeChanged( type ); 
    }
}

void pqVariableSelectorWidget::nameActivated( const QString &name )
{
  int nameID = TYPE_CELL;
  if ( this->Name[TYPE_NODE]->isVisible() )
    {
    nameID = TYPE_NODE;
    }
  if ( QString::compare( name, this->PrevName[nameID] ) )
    {
    this->PrevName[nameID] = name;
    emit varNameChanged( name ); 
    }
}

int pqVariableSelectorWidget::findVarType( const QString &type )
{ 
  int result = -1;

  for ( int i = 0; i < this->Type->count(); ++i ) 
    {
    if ( type == this->Type->itemText( i ) ) 
      {
      result = i; 
      break;
      }
    }

  return result;
}

int pqVariableSelectorWidget::findVarName( pqVariableSelectorWidget::VariableType type, const QString &name )
{ 
  int result = -1;

  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    for ( int i = 0; i < this->Name[type]->count(); ++i ) 
      {
      if ( name == this->Name[type]->itemText( i ) ) 
        {
        result = i; 
        break;
        }
      }
    }

  return result;
}

QString pqVariableSelectorWidget::getCurVarType() const
{
  return this->Type->currentText();
}

pqVariableSelectorWidget::VariableType pqVariableSelectorWidget::getCurVarTypeID() const
{
  return static_cast<pqVariableSelectorWidget::VariableType>(this->Type->currentIndex());
}

QString pqVariableSelectorWidget::getCurVarName() const
{
  QString type = this->getCurVarType();
  if ( type == "CELL" )
    {
    return Name[TYPE_CELL]->currentText();
    }
  else
    {
    return Name[TYPE_NODE]->currentText();
    }
}

QString pqVariableSelectorWidget::getCurVarName( pqVariableSelectorWidget::VariableType type ) const
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    return Name[type]->currentText();
    }
  else
    {
    return "error";
    }
}

void pqVariableSelectorWidget::getVarTypes( QStringList &names )
{
  names.clear();
  for ( int i = 0; i < this->Type->count(); ++i ) 
    {
    names.push_back( this->Type->itemText( i ) );
    }
}

void pqVariableSelectorWidget::getVarNames( pqVariableSelectorWidget::VariableType type, QStringList &names )
{
  names.clear();
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    for ( int i = 0; i < this->Name[type]->count(); ++i ) 
      {
      names.push_back( this->Name[type]->itemText( i ) );
      }
    }
}

void pqVariableSelectorWidget::clearVarNames( pqVariableSelectorWidget::VariableType type )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    this->Name[type]->clear();
    }
}

void pqVariableSelectorWidget::setVarNames( pqVariableSelectorWidget::VariableType type, const QStringList &names )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    clearVarNames( type );
    this->Name[type]->insertItems(0, names );
    }
}

void pqVariableSelectorWidget::addVarNames( pqVariableSelectorWidget::VariableType type, const QStringList &names )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    QStringList::ConstIterator cur = names.begin();
    QStringList::ConstIterator end = names.end();
    for ( ; cur != end; cur++ )
      {
      this->addVarName( type, *cur );
      }
    }
}

void pqVariableSelectorWidget::removeVarNames( pqVariableSelectorWidget::VariableType type, const QStringList &names )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    QStringList::ConstIterator cur = names.begin();
    QStringList::ConstIterator end = names.end();
    for ( ; cur != end; cur++ )
      {
      this->removeVarName( type, *cur );
      }
    }
}

bool pqVariableSelectorWidget::addVarName( pqVariableSelectorWidget::VariableType type, const QString &name )
{
  bool result = false;

  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    int id = findVarName( type, name );
    if ( id == -1 )
      {
      this->Name[type]->insertItem( this->Name[type]->count(), name );
      result = true;
      }
    }

  return result;
}

bool pqVariableSelectorWidget::removeVarName( pqVariableSelectorWidget::VariableType type, const QString &name )
{
  bool result = false;

  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    int id = findVarName( type, name );
    if ( id != -1 )
      {
      this->Name[type]->removeItem( id );
      result = true;
      }
    }

  return result;
}

QComboBox *pqVariableSelectorWidget::getVarTypeCombo() 
{ 
  return this->Type; 
}

QComboBox *pqVariableSelectorWidget::getVarNameCombo( pqVariableSelectorWidget::VariableType type )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    return this->Name[type];
    }
  else
    {
    return NULL;
    }
}

void pqVariableSelectorWidget::clearVarTypes() 
{ 
  this->Type->clear(); 
}

void pqVariableSelectorWidget::clearVarNames()
{
  this->Name[TYPE_CELL]->clear();
  this->Name[TYPE_NODE]->clear();
}


void pqVariableSelectorWidget::setCurVarType( pqVariableSelectorWidget::VariableType type )
{
  if ( type == TYPE_CELL )
    {
    this->Type->setCurrentIndex( type );
    this->Name[TYPE_CELL]->show();
    this->Name[TYPE_NODE]->hide();
    }
  else if ( type == TYPE_NODE )
    {
    this->Type->setCurrentIndex( type );
    this->Name[TYPE_CELL]->hide();
    this->Name[TYPE_NODE]->show();
    }
}

void pqVariableSelectorWidget::setCurVarName( const QString &name ) 
{
  int id = this->findVarName( this->getCurVarTypeID(), name );
  this->Name[this->getCurVarTypeID()]->setCurrentIndex( id );
}

void pqVariableSelectorWidget::update()
{
}


// commented out -- should use VTK print methods, not cout.
/*
void pqVariableSelectorWidget::printVarTypes()
{
  cout << "Var Types" << endl;
  for ( int i = 0; i < this->Type->count(); ++i ) 
    {
    cout << this->Type->text( i ) << endl;
    }
  cout << endl;
}

void pqVariableSelectorWidget::printVarNames()
{
  printVarNames( TYPE_CELL );
  printVarNames( TYPE_NODE );
}

void pqVariableSelectorWidget::printVarNames( pqVariableSelectorWidget::VariableType type )
{
  if ( ( type == TYPE_CELL ) || ( type == TYPE_NODE ) )
    {
    if ( type == TYPE_CELL )
      {
      cout << "Cell Var Names" << endl;
      }
    else
      {
      cout << "Node Var Names" << endl;
      }
    for ( int i = 0; i < this->Name[type]->count(); ++i ) 
      {
      cout << this->Name[type]->text( i ) << endl;
      }
    cout << endl;
    }
}

void pqVariableSelectorWidget::TestPlan()
{
  pqVariableSelectorWidget widget;
  QStringList cell[2];
  QStringList node[2];
  QString name;
  for ( int i=0;i<2;i++ )
    {
    for ( int j=0;j<4;j++ )
      {
      name = QString( "cell:%1:%2" ).arg( i ).arg( j );
      cell[i].push_back( name );
      name = QString( "node:%1:%2" ).arg( i ).arg( j );
      node[i].push_back( name );
      }
    }

  cout << "Starting pqVariableSelectorWidget TestPlan" << endl;
  widget.printVarTypes();
  widget.printVarNames();

  cout << "TEST: addVarName, trying to add duplicates" << endl;
  name = "cell:first";
  widget.addVarName( TYPE_CELL, name );
  name = "cell:second";
    // testing that duplicates are not inserted
  widget.addVarName( TYPE_CELL, name );
  widget.addVarName( TYPE_CELL, name );
  // node
  name = "node:first";
  widget.addVarName( TYPE_NODE, name );
  name = "node:second";
    // testing that duplicates are not inserted
  widget.addVarName( TYPE_NODE, name );
  widget.addVarName( TYPE_NODE, name );

  widget.printVarNames();


  // testing remove
  cout << "TEST: remove from node and cell of node:second" << endl;
  name = "node:second";
  widget.removeVarName( TYPE_CELL, name );
  widget.removeVarName( TYPE_NODE, name );
  widget.printVarNames();

  // testing clear
  cout << "TEST: testing clearVarNames" << endl;
  widget.clearVarNames(); 
  widget.printVarNames();

  // string list 
  cout << "TEST: testing setVarNames" << endl;
  widget.setVarNames( TYPE_CELL, cell[0] );
  widget.setVarNames( TYPE_NODE, node[0] );
  widget.printVarNames();
  widget.setVarNames( TYPE_CELL, cell[1] );
  widget.setVarNames( TYPE_NODE, node[1] );
  widget.printVarNames();

  QStringList names;
  cout << "TEST: testing getVarTypes" << endl;
  widget.getVarTypes( names );
  QStringList::Iterator cur = names.begin();
  QStringList::Iterator end = names.end();
  for ( ; cur != end; cur++ )
    {
    cout << *cur << " ";
    }
  cout << endl;
  cout << endl;

  cout << "TEST: testing getVarNames" << endl;
  widget.getVarNames( TYPE_CELL, names );
  cur = names.begin();
  end = names.end();
  for ( ; cur != end; cur++ )
    {
    cout << *cur << " ";
    }
  cout << endl;
  widget.getVarNames( TYPE_NODE, names );
  cur = names.begin();
  end = names.end();
  for ( ; cur != end; cur++ )
    {
    cout << *cur << " ";
    }
  cout << endl;
  cout << endl;

  cout << "TEST: getCurVarType, getCurVarName" << endl;
  name = widget.getCurVarType();
  cout << "Type: " << name << endl;
  name = widget.getCurVarName();
  cout << "Name: " << name << endl;
}

*/
