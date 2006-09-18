/*=========================================================================

   Program: ParaView
   Module:    pqPQLookupTableManager.cxx

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
#include "pqPQLookupTableManager.h"

#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <QList>
#include <QMap>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"
#include "pqPipelineDisplay.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"

//-----------------------------------------------------------------------------
class pqPQLookupTableManagerInternal
{
public:
  class Key
    {
  public:
    Key()
      {
      this->ConnectionID = 0;
      this->Arrayname = "";
      }
    Key(vtkIdType cid, QString arrayname)
      {
      this->ConnectionID = cid;
      this->Arrayname = arrayname;
      }

    bool operator<(const Key& k) const
      {
      if (this->ConnectionID == k.ConnectionID)
        {
        return this->Arrayname < k.Arrayname;
        }
      return (this->ConnectionID < k.ConnectionID);
      }

  private:
    vtkIdType ConnectionID;
    QString Arrayname;
    };

  typedef QMap<Key, QPointer<pqScalarsToColors> > MapOfLUT;
  MapOfLUT LookupTables;

  QString getRegistrationName(const QString& arrayname, 
    int vtkNotUsed(component)) const
    {
    return arrayname;
    }

  Key getKey(vtkIdType cid, const QString& registration_name)
    {
    QString arrayname = registration_name;
    if (arrayname != "")
      {
      return Key(cid, arrayname);
      }
    return Key();
    }
};

//-----------------------------------------------------------------------------
pqPQLookupTableManager::pqPQLookupTableManager(QObject* _p)
  : pqLookupTableManager(_p)
{
  this->Internal = new pqPQLookupTableManagerInternal;
}

//-----------------------------------------------------------------------------
pqPQLookupTableManager::~pqPQLookupTableManager()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqPQLookupTableManager::onAddLookupTable(pqScalarsToColors* lut)
{
  QString registration_name = lut->getSMName();
  pqPQLookupTableManagerInternal::Key key = 
    this->Internal->getKey(lut->getServer()->GetConnectionID(),
      registration_name);
  if (!this->Internal->LookupTables.contains(key))
    {
    this->Internal->LookupTables[key] = lut;
    }
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqPQLookupTableManager::getLookupTable(pqServer* server,
  const QString& arrayname,  int component)
{
  pqPQLookupTableManagerInternal::Key key(
    server->GetConnectionID(), arrayname);

  if (this->Internal->LookupTables.contains(key))
    {
    return this->Internal->LookupTables[key];
    }

  // Create a new lookuptable.
  return this->createLookupTable(server, arrayname, component);
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqPQLookupTableManager::createLookupTable(pqServer* server,
  const QString& arrayname, int component)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* lutProxy = 
    pxm->NewProxy("lookup_tables", "PVLookupTable");
  lutProxy->SetConnectionID(server->GetConnectionID());
  QString name = this->Internal->getRegistrationName(arrayname, component);
  // This will lead to the creation of pqScalarsToColors object
  // which this class will be intimated of (onAddLookupTable)
  // and our internal DS will be updated.
  pxm->RegisterProxy("lookup_tables", name.toStdString().c_str(), lutProxy);
  lutProxy->Delete();

  // Setup default LUT to go from Blue to Red.
  QList<QVariant> values;
  values << 0.0 << 0.0 << 0.0 << 1.0
    << 1.0 << 1.0 << 0.0 << 0.0;

  pqSMAdaptor::setMultipleElementProperty(
    lutProxy->GetProperty("RGBPoints"), values);
  pqSMAdaptor::setEnumerationProperty(
    lutProxy->GetProperty("ColorSpace"), "HSV");
  lutProxy->UpdateVTKObjects();
  lutProxy->InvokeCommand("Build");
  pqPQLookupTableManagerInternal::Key key(
    server->GetConnectionID(), arrayname);
  if (!this->Internal->LookupTables.contains(key))
    {
    qDebug() << "Creation of LUT failed!" ;
    return 0;
    }

  return this->Internal->LookupTables[key];
}

//-----------------------------------------------------------------------------
void pqPQLookupTableManager::updateLookupTableScalarRanges()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = pqServerManagerModel::instance();
  
  QList<pqPipelineDisplay*> displays = smmodel->getPipelineDisplays(0);
  foreach(pqPipelineDisplay* display, displays)
    {
    display->updateLookupTableScalarRange();
    }

}
