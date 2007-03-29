/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkManagerModel.cxx

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
#include "pqLookmarkManagerModel.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QStringList>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "pqLookmarkModel.h"

#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
class pqLookmarkManagerModelInternal 
{
public:

  typedef QList<QPointer<pqLookmarkModel> > ListOfLookmarks;
  ListOfLookmarks  Lookmarks;
  pqSettings *Settings;
};


//-----------------------------------------------------------------------------
pqLookmarkManagerModel::pqLookmarkManagerModel(QObject* _parent /*=NULL*/):
  QObject(_parent)
{
  this->Internal = new pqLookmarkManagerModelInternal();

  // Restore the contents of the lookmark model from a previous ParaView session, if any.
  this->importLookmarksFromSettings();
}

//-----------------------------------------------------------------------------
pqLookmarkManagerModel::~pqLookmarkManagerModel()
{
  this->exportAllLookmarksToSettings();
  foreach (pqLookmarkModel* lookmark, this->Internal->Lookmarks)
    {
    if (lookmark)
      {
      delete lookmark;
      }
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QString pqLookmarkManagerModel::getAllLookmarksSerialized() const
{
  QList<pqLookmarkModel*> list;
  foreach (pqLookmarkModel* lookmark, this->Internal->Lookmarks)
    {
    if (lookmark)
      {
      list.push_back(lookmark);
      }
    }
  return this->getLookmarksSerialized(list);
}

QString pqLookmarkManagerModel::getLookmarksSerialized(const QList<pqLookmarkModel*> &lookmarks) const
{
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("LookmarkDefinitionFile");

  //QList<pqLookmarkModel*>::const_iterator iter = lookmarks.begin();

  foreach (pqLookmarkModel* lookmark, lookmarks)
    {
    vtkPVXMLElement *lmkElem = vtkPVXMLElement::New();
    lmkElem->SetName("LookmarkDefinition");
    lookmark->saveState(lmkElem);
    root->AddNestedElement(lmkElem);
    lmkElem->Delete();
    }

  vtksys_ios::ostringstream os;
  root->PrintXML(os,vtkIndent(0));
  QString modelState = os.str().c_str();
  root->Delete();

  return modelState;
}


//-----------------------------------------------------------------------------
QList<pqLookmarkModel*> pqLookmarkManagerModel::getAllLookmarks() const
{
  QList<pqLookmarkModel*> list;
  foreach (pqLookmarkModel* lookmark, this->Internal->Lookmarks)
    {
    if (lookmark)
      {
      list.push_back(lookmark);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
pqLookmarkModel* pqLookmarkManagerModel::getLookmark(int index) const
{
  if(index<0 || index>=this->Internal->Lookmarks.size())
    {
    return 0;
    }

  return this->Internal->Lookmarks[index];
}

//-----------------------------------------------------------------------------
pqLookmarkModel* pqLookmarkManagerModel::getLookmark(const QString &name) const
{
  foreach (pqLookmarkModel* lookmark, this->Internal->Lookmarks)
    {
    if (lookmark && lookmark->getName()==name)
      {
      return lookmark;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
int pqLookmarkManagerModel::getNumberOfLookmarks()
{
  return this->Internal->Lookmarks.size();
}

void pqLookmarkManagerModel::importLookmarksFromSettings()
{
  this->Internal->Settings = pqApplicationCore::instance()->settings();
  QString key = "Lookmarks";
  if(!this->Internal->Settings->contains(key))
    {
    return;
    }

  QString state = this->Internal->Settings->value(key).toString();

  if(state.isNull())
    {
    return;
    }

  char *charArray = new char[state.size()];
  const QChar *ptr = state.unicode();
  int j;
  // This is a hack for converting the QString to a char*. None of qstring's conversion methods were working.
  for(j=0; j<state.size(); j++)
    {
    charArray[j] = (char)ptr->toAscii();
    ptr++;
    if(ptr->isNull())
      {
      break;
      }
    }
  istrstream *is = new istrstream(charArray,j+1);

  vtkPVXMLParser *parser = vtkPVXMLParser::New();
  parser->SetStream(is);
  parser->Parse();
  vtkPVXMLElement *root = parser->GetRootElement();
  if(root)
    {
    int i = 0;
    vtkPVXMLElement *lookmark;
    while( (lookmark = root->GetNestedElement(i++)) )
      {
      pqLookmarkModel *lmkModel = new pqLookmarkModel(lookmark);
      this->addLookmark(lmkModel);
      //delete lmkModel;
      }
    }

  parser->Delete();
  delete [] charArray;
  delete is;
}

//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::addLookmark(pqLookmarkModel *lookmark)
{
  if(lookmark->getName().isEmpty() || lookmark->getName().isNull())
    {
    qDebug() << "Lookmark requires a name.";
    return;
    }
  if(lookmark->getState().isNull() || lookmark->getState().isEmpty())
    {
    qDebug() << "Lookmark requires server manager state.";
    return;
    }
  
  this->Internal->Lookmarks.push_back(lookmark);

  // listen for modified events
  QObject::connect(lookmark,SIGNAL(modified(pqLookmarkModel*)),this,SIGNAL(lookmarkModified(pqLookmarkModel*)));
  QObject::connect(lookmark,SIGNAL(nameChanged(const QString&,const QString&)),this,SIGNAL(lookmarkNameChanged(const QString&,const QString&)));

  QString name = lookmark->getName();
  QImage icon = lookmark->getIcon();
  emit this->lookmarkAdded(name,icon);
  emit this->lookmarkAdded(lookmark);
}


//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::removeLookmark(pqLookmarkModel *lookmark)
{
  QString lmkName = lookmark->getName();
  this->Internal->Lookmarks.removeAll(lookmark);
  delete lookmark;

  emit this->lookmarkRemoved(lmkName);
}


//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::removeLookmark(const QString &name)
{
  for(int i=0;i<this->Internal->Lookmarks.size();i++)
    {
    pqLookmarkModel *lookmark = this->Internal->Lookmarks[i];
    if (lookmark->getName()==name)
      {
      this->removeLookmark(lookmark);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::removeLookmarks(const QList<pqLookmarkModel*> &lookmarks)
{
  QList<pqLookmarkModel*>::const_iterator iter;
  QList<QString> names;
  for(iter=lookmarks.begin(); iter!=lookmarks.end(); iter++)
    {
    names.push_back((*iter)->getName());
    }
  QList<QString>::iterator iter2;
  for(iter2=names.begin(); iter2!=names.end(); iter2++)
    {
    this->removeLookmark(*iter2);
    }
}

//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::removeAllLookmarks()
{
  // I do this so a signal will be emitted for each removed lookmark
  this->removeLookmarks(this->getAllLookmarks());
}



//-----------------------------------------------------------------------------
void pqLookmarkManagerModel::importLookmarksFromFiles(const QStringList &files)
{
  // Clear the current selection. The new lookmark definitions
  // will be selected as they're added.
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  vtkPVXMLElement *lookmark;
  vtkPVXMLElement *root;

  QStringList::ConstIterator iter = files.begin();
  for( ; iter != files.end(); ++iter)
    {
    // Make sure name is unique among lookmarks
    parser->SetFileName((*iter).toAscii().data());
    parser->Parse();
    root = parser->GetRootElement();
    if (!root)
      {
      continue;
      }
    unsigned int numElems = root->GetNumberOfNestedElements();
    for (unsigned int i=0; i<numElems; i++)
      {
      vtkPVXMLElement* currentElement = root->GetNestedElement(i);
      if (currentElement->GetName() &&
          strcmp(currentElement->GetName(), "LookmarkDefinition") == 0)
        {
        const char* name = currentElement->GetAttribute("Name");
        if (name)
          {
          QString newname = this->getUnusedLookmarkName(QString(name));
          currentElement->SetAttribute("Name",newname.toAscii().data());
          }
        }
      }

    int j = 0;
    while( (lookmark = root->GetNestedElement(j++)) )
      {
      this->addLookmark(new pqLookmarkModel(lookmark));
      }
    }
  parser->Delete();
}



//----------------------------------------------------------------------------
QString pqLookmarkManagerModel::getUnusedLookmarkName(const QString &name)
{
  QString tempName = name;
  int counter = 1;
  while(this->getLookmark(tempName))
    {
    tempName = QString(name + " (" + QString::number(counter) + ")");
    counter++;
    }

  return tempName;
}


void pqLookmarkManagerModel::exportAllLookmarksToSettings()
{
  // Store the contents of the lookmarks browser for a subsequent ParaView session
  //pqSettings* settings = pqApplicationCore::instance()->settings();
  this->Internal->Settings->setValue("Lookmarks", this->getAllLookmarksSerialized());
}


void pqLookmarkManagerModel::exportAllLookmarksToFiles(const QStringList &files)
{
  QStringList::ConstIterator jter = files.begin();
  for( ; jter != files.end(); ++jter)
    {
    ofstream os((*jter).toAscii().data(), ios::out);
    os << this->getAllLookmarksSerialized().toAscii().data();
    }
}

void pqLookmarkManagerModel::exportLookmarksToFiles(const QList<pqLookmarkModel*> &lookmarks, const QStringList &files)
{
  QStringList::ConstIterator jter = files.begin();
  for( ; jter != files.end(); ++jter)
    {
    ofstream os((*jter).toAscii().data(), ios::out);
    os << this->getLookmarksSerialized(lookmarks).toAscii().data();
    }
}

void pqLookmarkManagerModel::loadLookmark(pqServer *server, pqGenericViewModule *view, QList<pqPipelineSource*> *sources, pqLookmarkModel *lookmark)
{
  if(!server || !lookmark)
    {
    return;
    }

  this->loadLookmark(server, view, sources, lookmark->getName());
}

void pqLookmarkManagerModel::loadLookmark(pqServer *server, pqGenericViewModule *view, QList<pqPipelineSource*> *sources, const QString &name)
{
  foreach (pqLookmarkModel* tempLmk, this->Internal->Lookmarks)
    {
    if (tempLmk->getName()==name)
      {
      tempLmk->load(server,sources,view);
      emit this->lookmarkLoaded(tempLmk);
      }
    }
}
