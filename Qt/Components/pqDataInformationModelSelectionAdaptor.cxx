/*=========================================================================

   Program: ParaView
   Module:    pqDataInformationModelSelectionAdaptor.cxx

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
#include "pqDataInformationModelSelectionAdaptor.h"

// Qt includes.
#include <QItemSelectionModel>
#include <QtDebug>

// ParaView includes.
#include "pqDataInformationModel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServerManagerSelectionModel.h"

//-----------------------------------------------------------------------------
pqDataInformationModelSelectionAdaptor::pqDataInformationModelSelectionAdaptor(
  QItemSelectionModel* qModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* _parent/*=0*/)
: pqSelectionAdaptor(qModel, smSelectionModel, _parent)
{
  const QAbstractItemModel* model = this->getQModel();

  if (!qobject_cast<const pqDataInformationModel*>(model))
    {
    qDebug() << "QItemSelectionModel must be a selection model for "
      " pqDataInformationModel.";
    return;
    }
}

//-----------------------------------------------------------------------------
pqDataInformationModelSelectionAdaptor::~pqDataInformationModelSelectionAdaptor()
{
}

//-----------------------------------------------------------------------------
QModelIndex pqDataInformationModelSelectionAdaptor::mapFromSMModel(
    pqServerManagerModelItem* item) const
{
  const pqDataInformationModel* pM = qobject_cast<const pqDataInformationModel*>(
    this->getQModel());
  
  pqOutputPort* outputPort = qobject_cast<pqOutputPort*>(item);
  if (outputPort)
    {
    return pM->getIndexFor(outputPort);
    }

  pqPipelineSource* src = qobject_cast<pqPipelineSource*>(item);
  return pM->getIndexFor(src? src->getOutputPort(0) : 0);
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqDataInformationModelSelectionAdaptor::mapToSMModel(
    const QModelIndex& index) const
{
  const pqDataInformationModel* pM = qobject_cast<const pqDataInformationModel*>(
    this->getQModel());
  return pM->getItemFor(index);
}

//-----------------------------------------------------------------------------
QItemSelectionModel::SelectionFlag 
pqDataInformationModelSelectionAdaptor::qtSelectionFlags() const
{
  return QItemSelectionModel::Rows;
}

//-----------------------------------------------------------------------------

