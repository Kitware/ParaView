/*=========================================================================

Program: ParaView
Module:    pqExpressionsManager.h

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

#ifndef pqExpressionsManager_h
#define pqExpressionsManager_h

#include "pqComponentsModule.h"

#include <memory> // for unique_ptr

#include <QString> // for QString

class PQCOMPONENTS_EXPORT pqExpressionsManager
{
public:
  pqExpressionsManager() = default;
  ~pqExpressionsManager() = default;

  /**
   * Data structure to handle an expression and its group name.
   */
  struct pqExpression
  {
    pqExpression() = default;

    pqExpression(const QString& group, const QString& value)
      : Group(group)
      , Name("")
      , Value(value)
    {
    }

    pqExpression(const QString& group, const QString& name, const QString& value)
      : Group(group)
      , Name(name)
      , Value(value)
    {
    }

    bool operator==(const pqExpression& other) const
    {
      return this->Value == other.Value && this->Group == other.Group;
    }

    // allow to sort by Group then Name then Value
    bool operator<(const pqExpression& other) const
    {
      int compare = this->Group.compare(other.Group, Qt::CaseInsensitive);
      if (compare != 0)
      {
        return compare < 0;
      }

      compare = this->Name.compare(other.Name, Qt::CaseInsensitive);
      if (compare != 0)
      {
        if (this->Name.isEmpty())
        {
          return false;
        }
        if (other.Name.isEmpty())
        {
          return true;
        }

        return compare < 0;
      }

      compare = this->Value.compare(other.Value, Qt::CaseInsensitive);
      if (compare != 0)
      {
        return compare < 0;
      }

      return false;
    }

    QString Group;
    QString Name;
    QString Value;
  };

  /**
   * Read and parse settings to return the list of available expressions.
   */
  static QList<pqExpression> getExpressionsFromSettings();
  static QList<pqExpression> getExpressionsFromSettings(const QString& group);
  /**
   * Add the expression under the given group and store it in the settings.
   * Preserve existent expressions.
   * Return true if the expression was correctly added.
   * Return false if an error occured or if expression was already present.
   */
  static bool addExpressionToSettings(const QString& group, const QString& value);
  /**
   * Store the given list to the settings. Replace previous settings.
   */
  static void storeToSettings(const QList<pqExpression>& expressions);

  /**
   * Name of QSettings group containing the expressions.
   */
  static constexpr const char* SETTINGS_GROUP() { return "ExpressionsManager"; }
  /**
   * Name of QSettings key containing the expression concatenated in one string
   */
  static constexpr const char* SETTINGS_KEY() { return "Expressions"; }
  /**
   * Group for classic expressions, like Calculator.
   */
  static constexpr const char* EXPRESSION_GROUP() { return "Expression"; }
  /**
   * Group for python expressions, like Python Calculator.
   */
  static constexpr const char* PYTHON_EXPRESSION_GROUP() { return "Python"; }
};

#endif
