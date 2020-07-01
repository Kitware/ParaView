/*=========================================================================

   Program: ParaView
   Module:  pqExtractGenerator.cxx

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
#include "pqExtractGenerator.h"

#include "pqApplicationCore.h"
#include "pqOutputPort.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMExtractsController.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QPointer>

class pqExtractGenerator::pqInternals
{
public:
  QPointer<pqServerManagerModelItem> Producer;
};

//-----------------------------------------------------------------------------
pqExtractGenerator::pqExtractGenerator(const QString& group, const QString& name,
  vtkSMProxy* smproxy, pqServer* server, QObject* parentObject)
  : Superclass(group, name, smproxy, server, parentObject)
  , Internals(new pqExtractGenerator::pqInternals())
{
  Q_ASSERT(smproxy != nullptr && smproxy->GetProperty("Producer"));

  auto qtconnector = this->getConnector();
  qtconnector->Connect(
    smproxy->GetProperty("Producer"), vtkCommand::ModifiedEvent, this, SLOT(producerChanged()));
}

//-----------------------------------------------------------------------------
pqExtractGenerator::~pqExtractGenerator()
{
}

//-----------------------------------------------------------------------------
void pqExtractGenerator::initialize()
{
  this->Superclass::initialize();
  this->producerChanged();
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqExtractGenerator::producer() const
{
  auto& internals = (*this->Internals);
  return internals.Producer;
}

//-----------------------------------------------------------------------------
bool pqExtractGenerator::isImageExtractGenerator() const
{
  auto& internals = (*this->Internals);
  return qobject_cast<pqView*>(internals.Producer) != nullptr;
}

//-----------------------------------------------------------------------------
bool pqExtractGenerator::isDataExtractGenerator() const
{
  auto& internals = (*this->Internals);
  return qobject_cast<pqOutputPort*>(internals.Producer) != nullptr;
}

//-----------------------------------------------------------------------------
void pqExtractGenerator::producerChanged()
{
  auto& internals = (*this->Internals);
  auto smmmodel = pqApplicationCore::instance()->getServerManagerModel();
  vtkNew<vtkSMExtractsController> controller;

  auto pproxy = controller->GetInputForGenerator(this->getProxy());
  auto smitem = smmmodel->findItem<pqServerManagerModelItem*>(pproxy);

  if (pproxy != nullptr && smitem == nullptr)
  {
    qCritical("A proxy was added as input to the extract generator without "
              "having previously registered it with the proxy manager. "
              "This is not supported.");
  }

  auto olditem = internals.Producer.data();
  if (smitem != olditem)
  {
    internals.Producer = smitem;
    Q_EMIT this->producerRemoved(olditem, this);
    Q_EMIT this->producerAdded(smitem, this);
  }
}
