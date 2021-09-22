/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringFormatter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVStringFormatter
 * @brief Utility class used for string formatting
 *
 * vtkPVStringFormatter is a utility class that defines the API for formatting strings
 * using a stack of argument scopes. Scopes can be created by defining arguments using
 * the fmt library. A string will always be formatted using the top argument scope in
 * the scope stack.
 */

#ifndef vtkPVStringFormatter_h
#define vtkPVStringFormatter_h

#include "vtkLogger.h"
#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro
#include <algorithm>
#include <memory>
#include <sstream>
#include <stack>

// clang-format off
#include <vtk_fmt.h> // needed for `fmt`
#include VTK_FMT(fmt/args.h)
#include VTK_FMT(fmt/chrono.h)
#include VTK_FMT(fmt/core.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVStringFormatter : public vtkObject
{
public:
  static vtkPVStringFormatter* New();
  vtkTypeMacro(vtkPVStringFormatter, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This subclass should ONLY be used to enable automatic push/pop of argument scopes
   * in the same scope of code.
   */
  class TraceScope
  {
  public:
    template <typename... Args>
    TraceScope(Args&&... args)
    {
      vtkPVStringFormatter::PushScope(args...);
    }

    template <typename... Args>
    TraceScope(const char* scopeName, Args&&... args)
    {
      vtkPVStringFormatter::PushScope(scopeName, args...);
    }

    ~TraceScope() { vtkPVStringFormatter::PopScope(); }
  };

  /**
   * Pushes arguments using fmt::arg(name, arg) to add a new scope to the scope stack.
   * The new scope has as a baseline the top scope of the scope stack.
   */
  template <typename... Args>
  static void PushScope(Args&&... args)
  {
    std::shared_ptr<vtkArgumentScope> newScope;

    if (vtkPVStringFormatter::ScopeStack.empty()) // check if stack is empty
    {
      newScope = std::make_shared<vtkArgumentScope>();
    }
    else // else use the top scope as a baseline for the new scope
    {
      newScope = std::make_shared<vtkArgumentScope>(*vtkPVStringFormatter::ScopeStack.top());
    }
    vtkPVStringFormatter::Push(*newScope, args...);
    vtkPVStringFormatter::ScopeStack.push(newScope);
  }

  /**
   * Pushes arguments using a scope name and arguments in the form of fmt::arg(name, arg)
   * to add a new scope to the scope stack.
   * E.g. if PushScope("TEST", fmt::arg("test1", test)) is used then the scope the scope will have
   * the following arguments: TEST_test1 and test1,
   * The new scope has as a baseline the top scope of the scope stack.
   *
   * @note If you want to call this function, scope name MUST be const char*, otherwise
   * the implementation of this function without the scope name will be invoked.
   */
  template <typename... Args>
  static void PushScope(const char* scopeName, Args&&... args)
  {
    std::shared_ptr<vtkArgumentScope> newScope;

    if (vtkPVStringFormatter::ScopeStack.empty()) // check if stack is empty
    {
      newScope = std::make_shared<vtkArgumentScope>();
    }
    else // else use the top scope as a baseline for the new scope
    {
      newScope = std::make_shared<vtkArgumentScope>(*vtkPVStringFormatter::ScopeStack.top());
    }
    vtkPVStringFormatter::Push(*newScope, scopeName, args...);
    vtkPVStringFormatter::ScopeStack.push(newScope);
  }

  /**
   * Pops the top scope of the scope stack.
   */
  static void PopScope();

  /**
   * Given a formattable string, use the top scope of the argument stack,
   * and return the formatted string.
   */
  static std::string Format(const std::string& formattableString);

protected:
  vtkPVStringFormatter();
  ~vtkPVStringFormatter() override;

private:
  vtkPVStringFormatter(const vtkPVStringFormatter&) = delete;
  void operator=(const vtkPVStringFormatter&) = delete;

  /**
   * Implements a scope of arguments
   */
  class vtkArgumentScope
  {
  private:
    fmt::dynamic_format_arg_store<fmt::format_context> Args;
    std::vector<std::string> ArgNames;

  public:
    vtkArgumentScope() = default;

    vtkArgumentScope(const vtkArgumentScope& args)
      : Args(args.Args)
      , ArgNames(args.ArgNames)
    {
    }

    /**
     * Adds an named argument as an fmt::arg() object.
     * If argument already exists, it's ignored.
     */
    template <typename T>
    void AddArg(const T& namedArg)
    {
      std::string argName = namedArg.name;
      bool argNotFound =
        std::find(this->ArgNames.begin(), this->ArgNames.end(), argName) == this->ArgNames.end();
      // if argument was not found
      if (argNotFound)
      {
        this->Args.push_back(namedArg);
        this->ArgNames.push_back(argName);
      }
      else // else print warning
      {
        vtkLogF(WARNING, "Argument %s already exists. Try to add another one.", argName.c_str());
      }
    }

    /**
     * Gets arguments' names.
     */
    std::string GetArgInfo() const
    {
      std::stringstream argInfo;
      for (size_t i = 0; i < this->ArgNames.size(); ++i)
      {
        argInfo << "\tName: " << this->ArgNames[i] << "\n";
      }
      return argInfo.str();
    }

    /**
     * Gets arguments
     */
    const fmt::dynamic_format_arg_store<fmt::format_context>& GetArgs() const { return this->Args; }

    /**
     * Erases all arguments.
     */
    void clear()
    {
      this->Args.clear();
      this->ArgNames.clear();
    }
  };

  /**
   * Gets information about the argument names of the top scope of the scope stack.
   */
  static std::string GetArgInfo();

  /**
   * Default case of an argument push to an argument scope.
   */
  static void Push(vtkArgumentScope& vtkNotUsed(scope)) {}

  /**
   * Push an argument into an argument scope.
   */
  template <typename T0, typename... TArgs>
  static void Push(vtkArgumentScope& scope, T0& arg0, TArgs&... args)
  {
    scope.AddArg(arg0);
    vtkPVStringFormatter::Push(scope, args...);
  }

  /**
   * Default case of an argument push to an argument scope with a name.
   */
  static void Push(vtkArgumentScope& vtkNotUsed(scope), const char* vtkNotUsed(scopeName)) {}

  /**
   * Push an argument into an argument scope with a name.
   */
  template <typename T0, typename... TArgs>
  static void Push(vtkArgumentScope& scope, const char* scopeName, T0& arg0, TArgs&... args)
  {
    auto scopeBasedArgName = std::string(scopeName) + "_" + arg0.name;
    scope.AddArg(fmt::arg(scopeBasedArgName.c_str(), arg0.value));
    scope.AddArg(arg0);
    vtkPVStringFormatter::Push(scope, scopeName, args...);
  }

  static std::stack<std::shared_ptr<vtkArgumentScope>> ScopeStack;
};

#define PV_STRING_FORMATTER_SCOPE_0(x, y) x##y
#define PV_STRING_FORMATTER_SCOPE_1(x, y) PV_STRING_FORMATTER_SCOPE_0(x, y)
#define PV_STRING_FORMATTER_SCOPE(...)                                                             \
  vtkPVStringFormatter::TraceScope PV_STRING_FORMATTER_SCOPE_1(_trace_item, __LINE__)(__VA_ARGS__)
#define PV_STRING_FORMATTER_NAMED_SCOPE(NAME, ...)                                                 \
  vtkPVStringFormatter::TraceScope PV_STRING_FORMATTER_SCOPE_1(_trace_item, __LINE__)(             \
    NAME, __VA_ARGS__)

#endif
