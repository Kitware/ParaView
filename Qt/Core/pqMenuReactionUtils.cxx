// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMenuReactionUtils.h"

#include "pqApplicationCore.h"
#include "vtkSMDataTypeDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"

#include <QCoreApplication>
#include <QStringList>
#include <vector>

namespace pqMenuReactionUtils
{

vtkSMInputProperty* getInputProperty(vtkSMProxy* proxy)
{
  // if "Input" is present, we return that, otherwise the "first"
  // vtkSMInputProperty encountered is returned.

  vtkSMInputProperty* prop = vtkSMInputProperty::SafeDownCast(proxy->GetProperty("Input"));
  vtkSMPropertyIterator* propIter = proxy->NewPropertyIterator();
  for (propIter->Begin(); !prop && !propIter->IsAtEnd(); propIter->Next())
  {
    prop = vtkSMInputProperty::SafeDownCast(propIter->GetProperty());
  }

  propIter->Delete();
  return prop;
}

QString getDomainDisplayText(vtkSMDomain* domain)
{
  if (auto dtd = vtkSMDataTypeDomain::SafeDownCast(domain))
  {
    return QString(dtd->GetDomainDescription().c_str());
  }
  else if (domain->IsA("vtkSMInputArrayDomain"))
  {
    vtkSMInputArrayDomain* iad = static_cast<vtkSMInputArrayDomain*>(domain);
    QString txt = (iad->GetAttributeType() == vtkSMInputArrayDomain::ANY
        ? QCoreApplication::translate("pqMenuReactionUtils", "Requires an attribute array")
        : QCoreApplication::translate("pqMenuReactionUtils", "Requires a %1 attribute array")
            .arg(iad->GetAttributeTypeAsString()));
    std::vector<int> numbersOfComponents = iad->GetAcceptableNumbersOfComponents();
    if (!numbersOfComponents.empty())
    {
      QString compDetails =
        QCoreApplication::translate("pqMenuReactionUtils", " with %1 component(s)");
      for (unsigned int i = 0; i < numbersOfComponents.size(); i++)
      {
        if (i == numbersOfComponents.size() - 1)
        {
          compDetails = compDetails.arg(numbersOfComponents[i]);
        }
        else
        {
          compDetails = compDetails.arg(numbersOfComponents[i]);
          compDetails += QCoreApplication::translate("pqMenuReactionUtils", " or %1");
        }
      }
      txt += compDetails;
    }
    return txt;
  }
  return QCoreApplication::translate("pqMenuReactionUtils", "Requirements not met");
}
}
