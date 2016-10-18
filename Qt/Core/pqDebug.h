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
#ifndef pqDebug_h
#define pqDebug_h

#include "pqCoreModule.h"
#include <QDebug>

/**
* pqDebugType provides a mechanism for application to define categories for
* debugging various components of the UI. By passing an appropriate name for
* the envVariable in the constructor, the debug output will only be
* generated when that variable is set.
* For example, if you want to put out debug messages when an environment
* variable MY_DEBUG_FOO is defined, you can do the following:
* \code{.cpp}
*   pqDebug("MY_DEBUG_FOO") << "This will be printed when MY_DEBUG_FOO "
*                           << "is defined.";
* \endcode
*/
class PQCORE_EXPORT pqDebugType
{
protected:
  bool Enabled;

public:
  pqDebugType(const QString& envVariable = QString());

  virtual ~pqDebugType();

  // Casting operator that returns true when the envVariable is set,
  // false otherwise.
  virtual operator bool() const { return this->Enabled; }
};

PQCORE_EXPORT QDebug pqDebug(const pqDebugType& type = pqDebugType());
inline QDebug pqDebug(const QString& type)
{
  return pqDebug(pqDebugType(type));
}

#include <string>
inline QDebug& operator<<(QDebug debug, const std::string& stdstring)
{
  debug << stdstring.c_str();
  return debug.maybeSpace();
}

#endif
