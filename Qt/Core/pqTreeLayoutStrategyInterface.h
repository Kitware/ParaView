/*=========================================================================

   Program: ParaView
   Module:    pqTreeLayoutStrategyInterface.h

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

#ifndef _pqTreeLayoutStrategyInterface_h
#define _pqTreeLayoutStrategyInterface_h

#include "pqCoreModule.h"
#include <QStringList>
#include <QtPlugin>

class vtkAreaLayoutStrategy;

/**
* interface class for plugins that create view modules
*/
class PQCORE_EXPORT pqTreeLayoutStrategyInterface
{
public:
  /**
  * destructor
  */
  pqTreeLayoutStrategyInterface();
  virtual ~pqTreeLayoutStrategyInterface();

  /**
  * Return a list of layout strategies supported by this interface
  */
  virtual QStringList treeLayoutStrategies() const = 0;

  virtual vtkAreaLayoutStrategy* getTreeLayoutStrategy(const QString& layoutStrategy) = 0;
};

Q_DECLARE_INTERFACE(pqTreeLayoutStrategyInterface, "com.kitware/paraview/treeLayoutStrategy")

#endif
