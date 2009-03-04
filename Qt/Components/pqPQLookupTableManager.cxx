/*=========================================================================

   Program: ParaView
   Module:    pqPQLookupTableManager.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqPQLookupTableManager.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
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
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"

#include <vtksys/ios/sstream>

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

    vtkIdType ConnectionID;
    QString Arrayname;
    int NumberOfComponents;
    };

  typedef QMap<Key, QPointer<pqScalarsToColors> > MapOfLUT;
  MapOfLUT LookupTables;
  vtkSmartPointer<vtkPVXMLElement> DefaultLUTElement;

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

  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings && settings->contains(DEFAULT_LOOKUPTABLE_SETTING_KEY()))
    {
    vtkPVXMLParser* parser = vtkPVXMLParser::New();
    if (parser->Parse(
        settings->value(DEFAULT_LOOKUPTABLE_SETTING_KEY()).toString().toAscii().data()))
      {
      this->Internal->DefaultLUTElement = parser->GetRootElement();
      }
    parser->Delete();
    }
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
bool pqPQLookupTableManager::getLookupTableProperties(
  pqScalarsToColors* lut,
  QString& arrayname, int &numComponents, int &component)
{
  pqInternal::Key key = this->Internal->LookupTables.key(lut);
  if (!key.Arrayname.isEmpty())
    {
    arrayname = key.Arrayname;
    numComponents = key.NumberOfComponents;
    component = lut->getVectorMode() == pqScalarsToColors::MAGNITUDE? -1:
      lut->getVectorComponent();
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
void pqPQLookupTableManager::saveAsDefault(pqScalarsToColors* lut)
{
  if (!lut)
    {
    qCritical() << "Cannot save empty lookup table as default.";
    return;
    }

  vtkSMProxy* lutProxy = lut->getProxy();

  // Remove "ScalarRangeInitialized" property, since we want the lookup table to
  // rescale when it is assciated with a new array.
  bool old_value = pqSMAdaptor::getElementProperty(
    lutProxy->GetProperty("ScalarRangeInitialized")).toBool();
  pqSMAdaptor::setElementProperty(
    lutProxy->GetProperty("ScalarRangeInitialized"), false);
  this->Internal->DefaultLUTElement.TakeReference(lutProxy->SaveState(0));
  pqSMAdaptor::setElementProperty(
    lutProxy->GetProperty("ScalarRangeInitialized"), old_value);


  vtksys_ios::ostringstream stream;
  this->Internal->DefaultLUTElement->PrintXML(stream, vtkIndent());

  pqApplicationCore* core = pqApplicationCore::instance();
  pqSettings* settings = core->settings();
  if (settings)
    {
    settings->setValue(pqPQLookupTableManager::DEFAULT_LOOKUPTABLE_SETTING_KEY(),
      stream.str().c_str());
    }
}

//-----------------------------------------------------------------------------
void pqPQLookupTableManager::setDefaultState(vtkSMProxy* lutProxy)
{
  // Setup default LUT to go from Cool to Warm.
  QList<QVariant> values;
  values << 0.0 << 0.1381 << 0.2411 << 0.7091
         << 1.0 << 0.6728 << 0.1408 << 0.1266;
  pqSMAdaptor::setMultipleElementProperty(
    lutProxy->GetProperty("RGBPoints"), values);
  pqSMAdaptor::setEnumerationProperty(
    lutProxy->GetProperty("ColorSpace"), "Diverging");
  pqSMAdaptor::setEnumerationProperty(
    lutProxy->GetProperty("VectorMode"), "Magnitude");

  if (this->Internal->DefaultLUTElement)
    {
    lutProxy->LoadState(this->Internal->DefaultLUTElement, NULL);
    }

  lutProxy->UpdateVTKObjects();
  lutProxy->InvokeCommand("Build");
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
  this->setDefaultState(lutProxy);

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
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QList<pqPipelineRepresentation*> reprs =
    smmodel->findItems<pqPipelineRepresentation*>();
  foreach(pqPipelineRepresentation* repr, reprs)
    {
    repr->updateLookupTableScalarRange();
    }
}
