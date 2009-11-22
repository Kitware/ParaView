/*=========================================================================

   Program: ParaView
   Module:    pqParaViewBehaviors.h

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
#ifndef __pqParaViewBehaviors_h 
#define __pqParaViewBehaviors_h

#include <QObject>
#include "pqApplicationComponentsExport.h"

class QMainWindow;

/// @defgroup Behaviors ParaView Behaviors
/// Behaviors are classes that manage certain behaviors in the application. 
/// Developers should simply instantiate behaviors if the expect that
/// behavior in their client.


/// @ingroup Behaviors
/// pqParaViewBehaviors creates all the behaviors used by ParaView. If your
/// client is merely a branded version of ParaView, then you may want to simply
/// use this behavior.
class PQAPPLICATIONCOMPONENTS_EXPORT pqParaViewBehaviors : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqParaViewBehaviors(QMainWindow* window, QObject* parent=0);

private:
  Q_DISABLE_COPY(pqParaViewBehaviors)
};

#endif


