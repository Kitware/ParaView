/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqRecentlyUsedResourcesList.h"

#include "pqSettings.h"

#include <QStringList>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqRecentlyUsedResourcesList::pqRecentlyUsedResourcesList(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqRecentlyUsedResourcesList::~pqRecentlyUsedResourcesList()
{
}

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::add(const pqServerResource& resource)
{
  // Remove any existing resources that match the resource we're about to add ...
  // Note: we consider a resource a "match" if it has the same host(s) and path;
  // we ignore scheme and port(s)
  for (int cc = 0; cc < this->ResourceList.size(); cc++)
  {
    if (this->ResourceList[cc].hostPath() == resource.hostPath())
    {
      this->ResourceList.removeAt(cc);
      cc--;
    }
  }

  this->ResourceList.prepend(resource);

  const int max_length = 30;
  while (this->ResourceList.size() > max_length)
  {
    this->ResourceList.removeAt(max_length);
  }

  emit this->changed();
}

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::load(pqSettings& settings)
{
  const QStringList resources = settings.value("RecentlyUsedResourcesList").toStringList();
  this->ResourceList.clear();
  for (int i = resources.size() - 1; i >= 0; --i)
  {
    this->add(pqServerResource(resources[i]));
  }
}

//-----------------------------------------------------------------------------
void pqRecentlyUsedResourcesList::save(pqSettings& settings) const
{
  QStringList resources;
  QList<pqServerResource>::const_iterator iter;
  for (iter = this->ResourceList.begin(); iter != this->ResourceList.end(); ++iter)
  {
    resources.push_back(iter->serializeString());
  }
  settings.setValue("RecentlyUsedResourcesList", resources);
}
