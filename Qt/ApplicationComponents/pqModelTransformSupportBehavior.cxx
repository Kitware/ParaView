// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqModelTransformSupportBehavior.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkTuple.h"

#include <QtDebug>

#include <cassert>

namespace
{
static vtkSMSourceProxy* FindVisibleProducerWithChangeOfBasisMatrix(pqView* view)
{
  Q_FOREACH (pqRepresentation* repr, view->getRepresentations())
  {
    pqDataRepresentation* drepr = qobject_cast<pqDataRepresentation*>(repr);
    if (!drepr || !drepr->isVisible())
    {
      continue;
    }

    vtkPVDataInformation* info = drepr->getInputDataInformation();
    vtkPVArrayInformation* cobm =
      info->GetArrayInformation("ChangeOfBasisMatrix", vtkDataObject::FIELD);
    vtkPVArrayInformation* bbimc = cobm
      ? info->GetArrayInformation("BoundingBoxInModelCoordinates", vtkDataObject::FIELD)
      : nullptr;
    if (cobm && bbimc)
    {
      return vtkSMSourceProxy::SafeDownCast(drepr->getInput()->getProxy());
    }
  }
  return nullptr;
}

/// Changes the title property iff its value wasn't explicitly changed by the user.
/// If value is nullptr, we restore the property default else the property is set to
/// the specified value.
void SafeSetAxisTitle(vtkSMProxy* gridAxis, const char* pname, const char* value)
{
  vtkSMProperty* prop = gridAxis->GetProperty(pname);
  assert(prop);

  vtkSMPropertyHelper helper(prop);

  QString key = QString("MTSBAutoTitle.%1").arg(pname);
  if (prop->IsValueDefault() ||
    (gridAxis->HasAnnotation(key.toUtf8().data()) &&
      strcmp(gridAxis->GetAnnotation(key.toUtf8().data()), helper.GetAsString()) == 0))
  {
    if (value)
    {
      helper.Set(value);
      gridAxis->SetAnnotation(key.toUtf8().data(), value);
    }
    else
    {
      prop->ResetToDefault();
      gridAxis->RemoveAnnotation(key.toUtf8().data());
    }
  }
}
}

//-----------------------------------------------------------------------------
pqModelTransformSupportBehavior::pqModelTransformSupportBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(viewAdded(pqView*)), SLOT(viewAdded(pqView*)));
  Q_FOREACH (pqView* view, smmodel->findItems<pqView*>())
  {
    this->viewAdded(view);
  }
}

//-----------------------------------------------------------------------------
pqModelTransformSupportBehavior::~pqModelTransformSupportBehavior() = default;

//-----------------------------------------------------------------------------
void pqModelTransformSupportBehavior::viewAdded(pqView* view)
{
  if (view)
  {
    this->connect(view, SIGNAL(updateDataEvent()), SLOT(viewUpdated()));
  }
}

//-----------------------------------------------------------------------------
void pqModelTransformSupportBehavior::viewUpdated()
{
  pqView* view = qobject_cast<pqView*>(this->sender());
  assert(view);

  // Check if there is any data source visible in the view that has a
  // ChangeOfBasisMatrix and BoundingBoxInModelCoordinates specified.
  if (vtkSMSourceProxy* producer = ::FindVisibleProducerWithChangeOfBasisMatrix(view))
  {
    this->enableModelTransform(view, producer);
  }
  else
  {
    this->disableModelTransform(view);
  }
}

//-----------------------------------------------------------------------------
void pqModelTransformSupportBehavior::enableModelTransform(pqView* view, vtkSMSourceProxy* producer)
{
  bool are_titles_valid;
  vtkTuple<std::string, 3> titles = this->getAxisTitles(producer, 0, &are_titles_valid);

  if (vtkSMProxy* gridAxes3DActor =
        vtkSMPropertyHelper(view->getProxy(), "AxesGrid", /*quiet*/ true).GetAsProxy())
  {
    vtkSMPropertyHelper(gridAxes3DActor, "UseModelTransform").Set(1);
    vtkTuple<double, 16> cobm = this->getChangeOfBasisMatrix(producer);
    vtkTuple<double, 6> bounds = this->getBoundingBoxInModelCoordinates(producer);
    vtkSMPropertyHelper(gridAxes3DActor, "ModelTransformMatrix").Set(cobm.GetData(), 16);
    vtkSMPropertyHelper(gridAxes3DActor, "ModelBounds").Set(bounds.GetData(), 6);

    if (are_titles_valid)
    {
      ::SafeSetAxisTitle(gridAxes3DActor, "XTitle", titles[0].c_str());
      ::SafeSetAxisTitle(gridAxes3DActor, "YTitle", titles[1].c_str());
      ::SafeSetAxisTitle(gridAxes3DActor, "ZTitle", titles[2].c_str());
    }
    else
    {
      // clear data-dependent axis titles.
      ::SafeSetAxisTitle(gridAxes3DActor, "XTitle", nullptr);
      ::SafeSetAxisTitle(gridAxes3DActor, "YTitle", nullptr);
      ::SafeSetAxisTitle(gridAxes3DActor, "ZTitle", nullptr);
    }
    gridAxes3DActor->UpdateVTKObjects();
  }

  if (are_titles_valid)
  {
    vtkSMProxy* viewProxy = view->getProxy();
    vtkSMPropertyHelper(viewProxy, "XTitle", /*quiet*/ true).Set(titles[0].c_str());
    vtkSMPropertyHelper(viewProxy, "YTitle", /*quiet*/ true).Set(titles[1].c_str());
    vtkSMPropertyHelper(viewProxy, "ZTitle", /*quiet*/ true).Set(titles[2].c_str());
    viewProxy->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
void pqModelTransformSupportBehavior::disableModelTransform(pqView* view)
{
  if (vtkSMProxy* gridAxes3DActor =
        vtkSMPropertyHelper(view->getProxy(), "AxesGrid", /*quiet*/ true).GetAsProxy())
  {
    vtkSMPropertyHelper helper(gridAxes3DActor, "UseModelTransform");
    if (helper.GetAsInt() != 0)
    {
      helper.Set(0);
    }
    ::SafeSetAxisTitle(gridAxes3DActor, "XTitle", nullptr);
    ::SafeSetAxisTitle(gridAxes3DActor, "YTitle", nullptr);
    ::SafeSetAxisTitle(gridAxes3DActor, "ZTitle", nullptr);
    gridAxes3DActor->UpdateVTKObjects();
  }
}

//-----------------------------------------------------------------------------
namespace
{
template <class T, int size>
vtkTuple<T, size> GetValues(const char* aname, vtkSMSourceProxy* producer, int port, bool* pisvalid)
{
  bool dummy;
  pisvalid = pisvalid ? pisvalid : &dummy;
  *pisvalid = false;

  vtkTuple<T, size> value;
  vtkPVDataInformation* dinfo = producer->GetDataInformation(port);
  if (vtkPVArrayInformation* ainfo =
        (dinfo ? dinfo->GetArrayInformation(aname, vtkDataObject::FIELD) : nullptr))
  {
    if (ainfo->GetNumberOfComponents() == size)
    {
      *pisvalid = true;
      for (int cc = 0; cc < size; cc++)
      {
        value[cc] = ainfo->GetComponentRange(cc)[0];
      }
    }
  }
  return value;
}
}

//-----------------------------------------------------------------------------
vtkTuple<double, 16> pqModelTransformSupportBehavior::getChangeOfBasisMatrix(
  vtkSMSourceProxy* producer, int port, bool* pisvalid)
{
  return GetValues<double, 16>("ChangeOfBasisMatrix", producer, port, pisvalid);
}

//-----------------------------------------------------------------------------
vtkTuple<double, 6> pqModelTransformSupportBehavior::getBoundingBoxInModelCoordinates(
  vtkSMSourceProxy* producer, int port, bool* pisvalid)
{
  return GetValues<double, 6>("BoundingBoxInModelCoordinates", producer, port, pisvalid);
}

//-----------------------------------------------------------------------------
vtkTuple<std::string, 3> pqModelTransformSupportBehavior::getAxisTitles(
  vtkSMSourceProxy* producer, int port, bool* pisvalid)
{
  bool dummy;
  pisvalid = pisvalid ? pisvalid : &dummy;
  *pisvalid = false;

  vtkTuple<std::string, 3> value;
  vtkPVDataInformation* dinfo = producer->GetDataInformation(port);
  if (!dinfo)
  {
    return value;
  }

  vtkPVArrayInformation* xtitle = dinfo->GetArrayInformation("AxisTitleForX", vtkDataObject::FIELD);
  vtkPVArrayInformation* ytitle = dinfo->GetArrayInformation("AxisTitleForY", vtkDataObject::FIELD);
  vtkPVArrayInformation* ztitle = dinfo->GetArrayInformation("AxisTitleForZ", vtkDataObject::FIELD);
  if ((xtitle && xtitle->GetComponentName(0) == nullptr) ||
    (ytitle && ytitle->GetComponentName(0) == nullptr) ||
    (ztitle && ztitle->GetComponentName(0) == nullptr))
  {
    qCritical() << "Mechanisms for specifying axis titles have changed. "
                   "Please contact the ParaView developers for more info.";
    return value;
  }

  *pisvalid = xtitle || ytitle || ztitle;
  if (xtitle && xtitle->GetComponentName(0))
  {
    value[0] = xtitle->GetComponentName(0);
  }
  if (ytitle && ytitle->GetComponentName(0))
  {
    value[1] = ytitle->GetComponentName(0);
  }
  if (ztitle && ztitle->GetComponentName(0))
  {
    value[2] = ztitle->GetComponentName(0);
  }
  return value;
}
