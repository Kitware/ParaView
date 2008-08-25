/*=========================================================================

   Program: ParaView
   Module:    pqSourcesMenuManager.cxx

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

========================================================================*/
#include "pqSourcesMenuManager.h"

// Server Manager Includes.
#include "vtkSMProxyManager.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"

// Qt Includes.
#include <QMenu>
#include <QAction>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"

//-----------------------------------------------------------------------------
pqSourcesMenuManager::pqSourcesMenuManager(QMenu* _menu): Superclass(_menu)
{
}

//-----------------------------------------------------------------------------
pqSourcesMenuManager::~pqSourcesMenuManager()
{
}

//-----------------------------------------------------------------------------
void pqSourcesMenuManager::setEnabled(bool enable)
{
  QList<QAction*> cur_actions = this->menu()->actions();
  foreach (QAction* action, cur_actions)
    {
    action->setEnabled(enable);
    }
}
  
//-----------------------------------------------------------------------------
bool pqSourcesMenuManager::filter(const QString& name)
{
  bool showit = false;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = pxm->GetPrototypeProxy(
    this->xmlGroup().toAscii().data(), name.toAscii().data());
  pqObjectBuilder* ob = pqApplicationCore::instance()->getObjectBuilder();
  if(ob->getFileNamePropertyName(prototype).isEmpty())
    {
    showit = true;
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
            showit = true;
            }
          }
        }
      }
    }
  return showit;
}


