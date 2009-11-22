/*=========================================================================

   Program: ParaView
   Module:    pqListNewProxyDefinitionsBehavior.cxx

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
#include "pqListNewProxyDefinitionsBehavior.h"

#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPluginManager.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServerManagerObserver.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyDefinitionIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkPVXMLElement.h"

#include <QStringList>

static bool HasInput(const char* xmlgroup, const char* xmlname)
{
  vtkSMProxy* prototype =
    vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy(xmlgroup, xmlname);
  if (!prototype)
    {
    return false;
    }
  if (vtkSMInputProperty::SafeDownCast(prototype->GetProperty("Input")))
    {
    return true;
    }

  vtkSmartPointer<vtkSMPropertyIterator> propIter;
  propIter.TakeReference(prototype->NewPropertyIterator());
  for (propIter->Begin(); !propIter->IsAtEnd(); propIter->Next())
    {
    if (vtkSMInputProperty::SafeDownCast(propIter->GetProperty()))
      {
      return true;
      }
    }
  return false;
}

static bool HasShowHint(const char* xmlgroup, const char* xmlname)
{
  vtkSMProxy* prototype =
    vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy(xmlgroup, xmlname);
  if (!prototype)
    {
    return false;
    }
  // check for the show option in the hints
  vtkPVXMLElement* hints = prototype->GetHints();
  if(hints)
    {
    unsigned int numHints = hints->GetNumberOfNestedElements();
    for (unsigned int i = 0; i < numHints; i++)
      {
      vtkPVXMLElement *element = hints->GetNestedElement(i);
      if (QString("Property") == element->GetName())
        {
        QString propertyName = element->GetAttribute("name");
        int showProperty;
        if (element->GetScalarAttribute("show", &showProperty))
          {
          if (showProperty)
            {
            return true;
            }
          }
        }
      }
    }
  return false;
}

//-----------------------------------------------------------------------------
static bool IsReader(const char* xmlgroup, const char* xmlname)
{
  vtkSMProxy* prototype =
    vtkSMProxyManager::GetProxyManager()->GetPrototypeProxy(xmlgroup, xmlname);
  if (!prototype)
    {
    return false;
    }
  return !pqObjectBuilder::getFileNamePropertyName(prototype).isEmpty();
}

//-----------------------------------------------------------------------------
pqListNewProxyDefinitionsBehavior::pqListNewProxyDefinitionsBehavior(
  pqListNewProxyDefinitionsBehavior::eMode mode,
  const QString& xmlgroup,
  pqProxyGroupMenuManager* menuManager):
  Superclass(menuManager)
{
  Q_ASSERT(menuManager != NULL);

  this->Mode = mode;
  this->XMLGroup = xmlgroup;
  this->MenuManager = menuManager;

  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
    SIGNAL(serverManagerExtensionLoaded()),
    this, SLOT(update()));
  QObject::connect(pqApplicationCore::instance()->getServerManagerObserver(),
    SIGNAL(compoundProxyDefinitionRegistered(QString)),
    this, SLOT(update()));
  this->update();
}

//-----------------------------------------------------------------------------
void pqListNewProxyDefinitionsBehavior::update()
{
  bool something_added = false;
  bool add_new = (this->AlreadySeenSet.size() != 0);
  vtkSMProxyDefinitionIterator* iter = vtkSMProxyDefinitionIterator::New();
  for (iter->Begin(this->XMLGroup.toAscii().data()); !iter->IsAtEnd();
    iter->Next())
    {
    QString key = iter->GetKey();
    bool is_custom_filter = iter->IsCustom();
    if ( (add_new || is_custom_filter) && !this->AlreadySeenSet.contains(key) )
      {
      bool has_input = ::HasInput(iter->GetGroup(), iter->GetKey());
      if ( (this->Mode == SOURCES && !has_input && 
          (!::IsReader(iter->GetGroup(), iter->GetKey()) ||
           ::HasShowHint(iter->GetGroup(), iter->GetKey()))) ||
        (this->Mode == FILTERS && has_input) ||
        this->Mode == ANY)
        {
        this->MenuManager->addProxy(this->XMLGroup, key);
        something_added = true;
        }
      }
    this->AlreadySeenSet.insert(key);
    }
  iter->Delete();
  if (something_added)
    {
    this->MenuManager->populateMenu();
    }
}

