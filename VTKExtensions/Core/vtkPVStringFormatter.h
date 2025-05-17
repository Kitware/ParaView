// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

  using char_type = fmt::format_context::char_type;

  /**
   * Implements a named argument
   */
  struct vtkNamedArgument
  {
    enum class ValueType
    {
      // single values
      NONE,
      INT,
      UNSIGNED,
      LONG_LONG,
      UNSIGNED_LONG_LONG,
      BOOL,
      CHAR,
      FLOAT,
      DOUBLE,
      LONG_DOUBLE,
      STRING,
      TIME_POINT,
      // vector values
      DOUBLE_VECTOR
    };
    struct Value
    {
      ValueType Type;
      union
      {
        // single values
        int Int;
        unsigned Unsigned;
        long long LongLong;
        unsigned long long UnsignedLongLong;
        bool Bool;
        char_type Char;
        float Float;
        double Double;
        long double LongDouble;
        std::basic_string<char_type> String;
        std::chrono::time_point<std::chrono::system_clock> TimePoint;

        // vector values
        std::vector<double> DoubleVector;
      };

      Value()
        : Type(ValueType::NONE)
      {
      }

      Value(int value)
        : Type(ValueType::INT)
        , Int(value)
      {
      }

      Value(unsigned value)
        : Type(ValueType::UNSIGNED)
        , Unsigned(value)
      {
      }

      Value(long long value)
        : Type(ValueType::LONG_LONG)
        , LongLong(value)
      {
      }

      Value(unsigned long long value)
        : Type(ValueType::UNSIGNED_LONG_LONG)
        , UnsignedLongLong(value)
      {
      }

      Value(bool value)
        : Type(ValueType::BOOL)
        , Bool(value)
      {
      }

      Value(char_type value)
        : Type(ValueType::CHAR)
        , Char(value)
      {
      }

      Value(float value)
        : Type(ValueType::FLOAT)
        , Float(value)
      {
      }

      Value(double value)
        : Type(ValueType::DOUBLE)
        , Double(value)
      {
      }

      Value(long double value)
        : Type(ValueType::LONG_DOUBLE)
        , LongDouble(value)
      {
      }

      Value(const char_type* value)
        : Type(ValueType::STRING)
        , String(value)
      {
      }

      Value(const std::basic_string<char_type>& value)
        : Type(ValueType::STRING)
        , String(value)
      {
      }

      Value(const std::chrono::time_point<std::chrono::system_clock>& value)
        : Type(ValueType::TIME_POINT)
        , TimePoint(value)
      {
      }

      Value(const std::vector<double>& values)
        : Type(ValueType::DOUBLE_VECTOR)
        , DoubleVector(values)
      {
      }

      Value(const Value& value)
      {
        this->Type = value.Type;
        switch (value.Type)
        {
          case ValueType::INT:
            this->Int = value.Int;
            break;
          case ValueType::UNSIGNED:
            this->Unsigned = value.Unsigned;
            break;
          case ValueType::LONG_LONG:
            this->LongLong = value.LongLong;
            break;
          case ValueType::UNSIGNED_LONG_LONG:
            this->UnsignedLongLong = value.UnsignedLongLong;
            break;
          case ValueType::BOOL:
            this->Bool = value.Bool;
            break;
          case ValueType::CHAR:
            this->Char = value.Char;
            break;
          case ValueType::FLOAT:
            this->Float = value.Float;
            break;
          case ValueType::DOUBLE:
            this->Double = value.Double;
            break;
          case ValueType::LONG_DOUBLE:
            this->LongDouble = value.LongDouble;
            break;
          case ValueType::STRING:
            new (&this->String) std::basic_string<char_type>(value.String);
            break;
          case ValueType::TIME_POINT:
            this->TimePoint = value.TimePoint;
            break;
          case ValueType::DOUBLE_VECTOR:
            new (&this->DoubleVector) std::vector<double>(value.DoubleVector);
            break;
          default:
            break;
        }
      }

      ~Value()
      {
        switch (this->Type)
        {
          case ValueType::STRING:
            this->String.~basic_string();
            break;
          case ValueType::DOUBLE_VECTOR:
            this->DoubleVector.~vector();
            break;
          default:
            break;
        }
      }
    };

    std::basic_string<char_type> Name;
    Value Value;

    vtkNamedArgument() = default;

    template <typename DataType>
    vtkNamedArgument(const std::basic_string<char_type>& name, const DataType& value)
      : Name(name)
      , Value(value)
    {
    }

    ~vtkNamedArgument() = default;
  };

  /**
   * Implements a scope of arguments
   */
  class vtkArgumentScope
  {
  private:
    std::vector<vtkNamedArgument> Arguments;

  public:
    vtkArgumentScope() = default;

    vtkArgumentScope(const vtkArgumentScope& other)
    {
      this->Arguments.reserve(other.Arguments.size());
      for (const auto& arg : other.Arguments)
      {
        this->Arguments.emplace_back(arg);
      }
    }

    /**
     * Adds an named argument as an fmt::arg() object.
     * If argument already exists, it's ignored.
     */
    template <typename T>
    void AddArg(const fmt::detail::named_arg<char_type, T>& fmtArg)
    {
      bool argNotFound = std::find_if(this->Arguments.begin(), this->Arguments.end(),
                           [&fmtArg](const vtkNamedArgument& arg)
                           { return arg.Name == fmtArg.name; }) == this->Arguments.end();
      // if argument was not found
      if (argNotFound)
      {
        vtkNamedArgument newArg(fmtArg.name, fmtArg.value);
        this->Arguments.push_back(newArg);
      }
      else // else print message
      {
        vtkLogF(TRACE, "Argument %s already exists. Try to add another one.", fmtArg.name);
      }
    }

    /**
     * Gets arguments' info (name and type).
     */
    std::basic_string<char_type> GetArgInfo() const
    {
      std::basic_stringstream<char_type> argInfo;
      for (const auto& arg : this->Arguments)
      {
        argInfo << "\tName: " << arg.Name;
        argInfo << "\tType: ";
        switch (arg.Value.Type)
        {
          case vtkNamedArgument::ValueType::INT:
            argInfo << "int";
            break;
          case vtkNamedArgument::ValueType::UNSIGNED:
            argInfo << "unsigned";
            break;
          case vtkNamedArgument::ValueType::LONG_LONG:
            argInfo << "long long";
            break;
          case vtkNamedArgument::ValueType::UNSIGNED_LONG_LONG:
            argInfo << "unsigned long long";
            break;
          case vtkNamedArgument::ValueType::BOOL:
            argInfo << "bool";
            break;
          case vtkNamedArgument::ValueType::CHAR:
            argInfo << "char";
            break;
          case vtkNamedArgument::ValueType::FLOAT:
            argInfo << "float";
            break;
          case vtkNamedArgument::ValueType::DOUBLE:
            argInfo << "double";
            break;
          case vtkNamedArgument::ValueType::LONG_DOUBLE:
            argInfo << "long double";
            break;
          case vtkNamedArgument::ValueType::STRING:
            argInfo << "std::string";
            break;
          case vtkNamedArgument::ValueType::TIME_POINT:
            argInfo << "std::chrono::time_point<std::chrono::system_clock>";
            break;
          case vtkNamedArgument::ValueType::DOUBLE_VECTOR:
            argInfo << "std::vector<double>";
            break;
          default:
            argInfo << "unknown";
            break;
        }
        argInfo << "\n";
      }
      return argInfo.str();
    }

    /**
     * Builds ang Gets dynamic_format_arg_store from arguments.
     */
    fmt::dynamic_format_arg_store<fmt::format_context> GetArgs() const
    {
      fmt::dynamic_format_arg_store<fmt::format_context> args;
      for (const auto& arg : this->Arguments)
      {
        switch (arg.Value.Type)
        {
          case vtkNamedArgument::ValueType::INT:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Int));
            break;
          case vtkNamedArgument::ValueType::UNSIGNED:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Unsigned));
            break;
          case vtkNamedArgument::ValueType::LONG_LONG:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.LongLong));
            break;
          case vtkNamedArgument::ValueType::UNSIGNED_LONG_LONG:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.UnsignedLongLong));
            break;
          case vtkNamedArgument::ValueType::BOOL:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Bool));
            break;
          case vtkNamedArgument::ValueType::CHAR:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Char));
            break;
          case vtkNamedArgument::ValueType::FLOAT:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Float));
            break;
          case vtkNamedArgument::ValueType::DOUBLE:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.Double));
            break;
          case vtkNamedArgument::ValueType::LONG_DOUBLE:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.LongDouble));
            break;
          case vtkNamedArgument::ValueType::STRING:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.String));
            break;
          case vtkNamedArgument::ValueType::TIME_POINT:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.TimePoint));
            break;
          case vtkNamedArgument::ValueType::DOUBLE_VECTOR:
            args.push_back(fmt::arg(arg.Name.c_str(), arg.Value.DoubleVector));
            break;
          default:
            break;
        }
      }
      return args;
    }

    /**
     * Erases all arguments.
     */
    void clear() { this->Arguments.clear(); }
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
