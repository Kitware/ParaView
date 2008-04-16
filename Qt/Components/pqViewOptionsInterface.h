/*=========================================================================

   Program: ParaView
   Module:    pqViewOptionsInterface.h

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

#ifndef _pqViewOptionsInterface_h
#define _pqViewOptionsInterface_h

#include <QtPlugin>
class pqActiveViewOptions;
class pqOptionsContainer;

/// interface class for plugins that create view options pages
class pqViewOptionsInterface
{
public:
  /// destructor
  virtual ~pqViewOptionsInterface() {}

  /// returns a list of view types that this interface provides options for  
  virtual QStringList viewTypes() const = 0;

  /// return an options object for the active view.
  /// this is used when there are options that are specific to an instance of a
  /// view
  virtual pqActiveViewOptions* createActiveViewOptions(const QString& type, QObject* parent) = 0;
  
  /// return an options object for global view options
  /// this is used when there are options that apply to all instance of a view
  virtual pqOptionsContainer* createGlobalViewOptions(const QString& type, QWidget* parent) = 0;

};

Q_DECLARE_INTERFACE(pqViewOptionsInterface, "com.kitware/paraview/viewoptions")

#endif

