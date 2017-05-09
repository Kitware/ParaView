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
#ifndef pqServerConfiguration_h
#define pqServerConfiguration_h

#include "pqCoreModule.h"
#include "vtkSmartPointer.h"
#include <QObject>

class vtkPVXMLElement;
class pqServerResource;
class vtkIndent;

/**
* pqServerConfiguration corresponds to a server connection configuration.
* These are typically read from pvsc files.
*/
class PQCORE_EXPORT pqServerConfiguration
{
public:
  pqServerConfiguration();
  pqServerConfiguration(vtkPVXMLElement* xml);
  ~pqServerConfiguration();

  /**
  * Get/Set whether the configuration is mutable. This variable is not
  * serialized.
  */
  bool isMutable() const { return this->Mutable; }
  void setMutable(bool val) { this->Mutable = val; }

  /**
  * Get/Set the name for the configuration.
  */
  void setName(const QString& name);
  QString name() const;

  /**
  * Returns true if the name for this configuration is the default one i.e.
  * the one that gets set when none is specified. Useful to determine "empty"
  * configurations.
  */
  bool isNameDefault() const;

  /**
  * Get/Set the URI that describes the server scheme, hostname(s) and port(s).
  */
  pqServerResource resource() const;
  void setResource(const pqServerResource&);
  void setResource(const QString&);

  /**
  * Types of start
  */
  enum StartupType
  {
    INVALID,
    MANUAL,
    COMMAND
  };

  /**
  * returns the startup type for this configuration. There are 3 types of
  * startup: manual, simple-command and custom-command.
  */
  StartupType startupType() const;

  /**
  * If startupType() == COMMAND, then this method can be used to obtain
  * the command for the startup. Note that this does not include any
  * information options etc. that may be specified in the startup.
  */
  QString command(double& timeout, double& delay) const;

  /**
  * changes the startup type to manual.
  */
  void setStartupToManual();

  /**
  * changes the startup type to command.
  */
  void setStartupToCommand(double timeout, double delay, const QString& command);

  /**
  * serialize to a string.
  */
  QString toString(vtkIndent indent) const;

  /**
  * Create a new clone (deep copying the vtkPVXMLElement).
  */
  pqServerConfiguration clone() const;

  /**
  * returns the \<Options\> element, if any.
  */
  vtkPVXMLElement* optionsXML() const;

  /**
  * returns the \<Hints\> element, if any.
  */
  vtkPVXMLElement* hintsXML() const;

protected:
  vtkPVXMLElement* startupXML() const;

private:
  void constructor(vtkPVXMLElement*);
  bool Mutable;
  vtkSmartPointer<vtkPVXMLElement> XML;
};

#endif
