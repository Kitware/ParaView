/*=========================================================================

   Program: ParaView
   Module:    pqFormBuilder.cxx

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

#include "pqFormBuilder.h"

#include <QtDesigner/QDesignerCustomWidgetInterface>
#include <QtDesigner/QDesignerCustomWidgetCollectionInterface>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QWidget>

pqFormBuilder::pqFormBuilder(QObject* o) 
  : QUiLoader(o)
{
// If a plugin needs to load a library to locate the UI file, it should have
// already been loaded. This is loading other plugins the directories specified
// with weird side-effects.
//  // search plugins in application directory
//  this->addPluginPath(QCoreApplication::applicationDirPath());
//#ifdef Q_WS_MAC
//  this->addPluginPath(QCoreApplication::applicationDirPath() + "/../../../");
//#endif //Q_WS_MAC
}

pqFormBuilder::~pqFormBuilder()
{
}

QWidget* pqFormBuilder::createWidget(const QString& className, 
                      QWidget* p, 
                      const QString& name)
{
  QWidget* w = NULL;
  
  // search static plugins for ui widgets
  // would be nice if Qt did this for us
  foreach(QObject* o, QPluginLoader::staticInstances())
    {
    QList<QDesignerCustomWidgetInterface*> factories;
    QDesignerCustomWidgetInterface* factory =
      qobject_cast<QDesignerCustomWidgetInterface*>(o);
    QDesignerCustomWidgetCollectionInterface* factoryCollection =
      qobject_cast<QDesignerCustomWidgetCollectionInterface*>(o);
    if(factory)
      {
      factories << factory;
      }
    else if(factoryCollection)
      {
      factories = factoryCollection->customWidgets();
      }
    QList<QDesignerCustomWidgetInterface*>::iterator iter;
    for(iter = factories.begin(); w == NULL && iter != factories.end(); ++iter)
      {
      if((*iter)->name() == className)
        {
        w = (*iter)->createWidget(p);
        w->setObjectName(name);
        }
      }
    }
  if(!w)
    {
    w = QUiLoader::createWidget(className, p, name);
    }
  return w;
}

