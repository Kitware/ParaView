/*=========================================================================

   Program: ParaView
   Module:    pqViewSettingsManager.h

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
#ifndef __pqViewSettingsManager_h 
#define __pqViewSettingsManager_h

#include "pqActiveViewOptionsManager.h"
#include "pqApplicationComponentsExport.h"

/// pqViewSettingsManager is a temporary class used by
/// pqActiveViewSettingsReaction to manage the view options using
/// pqActiveViewOptionsManager. Ideally, I'd like to deprecate
/// pqActiveViewOptionsManager for something with fewer cross dependencies.
class PQAPPLICATIONCOMPONENTS_EXPORT pqViewSettingsManager : public pqActiveViewOptionsManager
{
  Q_OBJECT
  typedef pqActiveViewOptionsManager Superclass;
public:
  pqViewSettingsManager(QObject* parent=0);

protected slots:
  void pluginLoaded(QObject* plugin_interface);

private:
  Q_DISABLE_COPY(pqViewSettingsManager)
};

#endif


