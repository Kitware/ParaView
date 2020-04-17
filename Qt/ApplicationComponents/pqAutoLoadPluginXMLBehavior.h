/*=========================================================================

   Program: ParaView
   Module:    pqAutoLoadPluginXMLBehavior.h

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
#ifndef pqAutoLoadPluginXMLBehavior_h
#define pqAutoLoadPluginXMLBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QSet>

/**
* @ingroup Behaviors
*
* ParaView plugins can load gui configuration xmls eg. xmls for defining the
* filters menu, readers etc. This behavior ensures that as soon as such
* plugins are loaded if they provide any XMLs in the ":/.{*}/ParaViewResources/"
* resource location, then such xmls are parsed and an attempt is made to load
* them (by calling pqApplicationCore::loadConfiguration()).
*
* Note: {} is to keep the *+/ from ending the comment block.
*
* Without going into too much detail, if you want your application to
* automatically load GUI configuration XMLs from plugins, instantiate this
* behavior.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqAutoLoadPluginXMLBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqAutoLoadPluginXMLBehavior(QObject* parent = 0);

protected Q_SLOTS:
  void updateResources();

private:
  Q_DISABLE_COPY(pqAutoLoadPluginXMLBehavior)
  QSet<QString> PreviouslyParsedResources;
  QSet<QString> PreviouslyParsedPlugins;
};

#endif
