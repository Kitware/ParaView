/*=========================================================================

   Program: ParaView
   Module:    pqFiltersMenuReaction.cxx

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
#include "pqMenuReactionUtils.h"

#include "vtkSMDataTypeDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"

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
        ? QString("Requires an attribute array")
        : QString("Requires a %1 attribute array").arg(iad->GetAttributeTypeAsString()));
    std::vector<int> numbersOfComponents = iad->GetAcceptableNumbersOfComponents();
    if (numbersOfComponents.size() > 0)
    {
      txt += QString(" with ");
      for (unsigned int i = 0; i < numbersOfComponents.size(); i++)
      {
        if (i == numbersOfComponents.size() - 1)
        {
          txt += QString("%1 ").arg(numbersOfComponents[i]);
        }
        else
        {
          txt += QString("%1 or ").arg(numbersOfComponents[i]);
        }
      }
      txt += QString("component(s)");
    }
    return txt;
  }
  return QString("Requirements not met");
}
}
