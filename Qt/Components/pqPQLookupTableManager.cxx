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

#include "vtkProcessModule.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <QList>
#include <QMap>
#include <QPointer>
#include <QRegExp>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel2.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqPQLookupTableManager::pqInternal
{
public:
  class Key
    {
  public:
    Key()
      {
      this->ConnectionID = 0;
      this->Arrayname = "";
      this->NumberOfComponents = 0;
      }
    Key(vtkIdType cid, QString arrayname, int num)
      {
      this->ConnectionID = cid;
      this->Arrayname = arrayname;
      this->NumberOfComponents = num;
      }

    bool operator<(const Key& k) const
      {
      if (this->NumberOfComponents == k.NumberOfComponents)
        {
        if (this->ConnectionID == k.ConnectionID)
          {
          return this->Arrayname < k.Arrayname;
          }
        return (this->ConnectionID < k.ConnectionID);
        }
      return (this->NumberOfComponents < k.NumberOfComponents); 
      }

  private:
    vtkIdType ConnectionID;
    QString Arrayname;
    int NumberOfComponents;
    };

  typedef QMap<Key, QPointer<pqScalarsToColors> > MapOfLUT;
  MapOfLUT LookupTables;

  QString getRegistrationName(const QString& arrayname, 
    int number_of_components, int vtkNotUsed(component)) const
    {
    return QString::number(number_of_components) + "." + arrayname;
    }

  Key getKey(vtkIdType cid, const QString& registration_name)
    {
    QRegExp rex ("(\\d+)\\.(.+)");
    if (rex.exactMatch(registration_name))
      {
      int number_of_components = QVariant(rex.cap(1)).toInt();
      QString arrayname = rex.cap(2);
      return Key(cid, arrayname, number_of_components);
      }
    return Key();
    }
};

//-----------------------------------------------------------------------------
pqPQLookupTableManager::pqPQLookupTableManager(QObject* _p)
  : pqLookupTableManager(_p)
{
  this->Internal = new pqInternal;
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
  pqInternal::Key key = 
    this->Internal->getKey(lut->getServer()->GetConnectionID(),
      registration_name);
  if (!this->Internal->LookupTables.contains(key))
    {
    this->Internal->LookupTables[key] = lut;
    }
}

//-----------------------------------------------------------------------------
void pqPQLookupTableManager::onRemoveLookupTable(pqScalarsToColors* lut)
{
  pqInternal::MapOfLUT::iterator iter = 
    this->Internal->LookupTables.begin();
  for (; iter != this->Internal->LookupTables.end(); )
    {
    if (iter.value() == lut)
      {
      iter = this->Internal->LookupTables.erase(iter);
      }
    else
      {
      ++iter;
      }
    }
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqPQLookupTableManager::getLookupTable(pqServer* server,
  const QString& arrayname,  int number_of_components, int component)
{
  pqInternal::Key key(
    server->GetConnectionID(), arrayname, number_of_components);

  if (this->Internal->LookupTables.contains(key))
    {
    return this->Internal->LookupTables[key];
    }

  // Create a new lookuptable.
  return this->createLookupTable(
    server, arrayname, number_of_components, component);
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqPQLookupTableManager::createLookupTable(pqServer* server,
  const QString& arrayname, int number_of_components, int component)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* lutProxy = 
    pxm->NewProxy("lookup_tables", "PVLookupTable");
  lutProxy->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  lutProxy->SetConnectionID(server->GetConnectionID());
  QString name = this->Internal->getRegistrationName(
    arrayname, number_of_components, component);
  // This will lead to the creation of pqScalarsToColors object
  // which this class will be intimated of (onAddLookupTable)
  // and our internal DS will be updated.
  pxm->RegisterProxy("lookup_tables", name.toAscii().data(), lutProxy);
  lutProxy->Delete();

  // Setup default LUT to go from Blue to Red.
  QList<QVariant> values;
  values << 0.0 << 0.0 << 0.0 << 1.0
    << 1.0 << 1.0 << 0.0 << 0.0;

  pqSMAdaptor::setMultipleElementProperty(
    lutProxy->GetProperty("RGBPoints"), values);
  pqSMAdaptor::setEnumerationProperty(
    lutProxy->GetProperty("ColorSpace"), "HSV");
  pqSMAdaptor::setEnumerationProperty(
    lutProxy->GetProperty("VectorMode"), "Magnitude");
  lutProxy->UpdateVTKObjects();
  lutProxy->InvokeCommand("Build");

  if (number_of_components >= 1)
    {
    vtkSMIntRangeDomain* componentsDomain = 
      vtkSMIntRangeDomain::SafeDownCast(
        lutProxy->GetProperty("VectorComponent")->GetDomain("range"));
    componentsDomain->AddMaximum(0, (number_of_components-1));
    }
  pqInternal::Key key(
    server->GetConnectionID(), arrayname, number_of_components);
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
  pqServerManagerModel2* smmodel = core->getServerManagerModel2();
  
  QList<pqPipelineRepresentation*> reprs = 
    smmodel->findItems<pqPipelineRepresentation*>();
  foreach(pqPipelineRepresentation* repr, reprs)
    {
    repr->updateLookupTableScalarRange();
    }
}
