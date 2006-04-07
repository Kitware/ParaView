/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqObjectNaming.h"

#include <QSet>
#include <QtDebug>

bool pqObjectNaming::Validate(QObject& Parent)
{
  const QString name = Parent.objectName();
  if(name.isEmpty())
    {
    qWarning() << "Unnamed root widget\n";
    return false;
    }
    
  return pqObjectNaming::Validate(Parent, "/" + name);
}

bool pqObjectNaming::Validate(QObject& Parent, const QString& Path)
{
  qDebug() << Path << "\n";

  bool result = true;
  
  QSet<QString> names;
  const QObjectList children = Parent.children();
  for(int i = 0; i != children.size(); ++i)
    {
    QObject* child = children[i];
    const QString name = child->objectName();
    if(name.isEmpty())
      {
      qWarning() << Path << " - unnamed widget: " << child << "\n";
      result = false;
      }
    else
      {
      if(names.contains(name))
        {
        qWarning() << Path << " - duplicate widget name [" << name << "]: " << child << "\n";
        result = false;
        }
        
      names.insert(name);
      
      if(!pqObjectNaming::Validate(*child, Path + "/" + name))
        {
        result = false;
        }
      }
    }
  
  return result;
}
