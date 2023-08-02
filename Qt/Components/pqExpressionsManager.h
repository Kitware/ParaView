// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
