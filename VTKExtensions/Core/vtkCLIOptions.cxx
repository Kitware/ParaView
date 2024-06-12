// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCLIOptions.h"

#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <algorithm>
#include <cassert>
#include <vtk_cli11.h>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

namespace
{
// This is fairly naive. It's not smart enough to detect 'quoted words'
// and treat them as a single word. However, works reasonably okay for our
// use-case.
std::string FormatParagraph(size_t indent, const std::string& text)
{
  static const size_t width = vtksys::SystemTools::GetTerminalWidth();
  if (width < (indent + 10))
  {
    // too narrow; don't bother.
    return text;
  }

  std::ostringstream str;
  const size_t target = width - indent;
  for (const auto& line : vtksys::SystemTools::SplitString(text, '\n'))
  {
    size_t count = 0;
    for (const auto& word : vtksys::SystemTools::SplitString(line, ' '))
    {
      if ((count + word.length() + 5) > target)
      {
        str << "\n";
        count = 0;
      }
      str << word << " ";
      count += word.length() + 1;
    }
    str << "\n";
  }
  return str.str();
}

class PVFormatter : public CLI::Formatter
{
  using Superclass = CLI::Formatter;

public:
  // overridden to add a new line before each group.
  std::string make_expanded(const CLI::App* sub) const override
  {
    return "\n" + Superclass::make_expanded(sub);
  }

  // overridden to ensure good word wrapping for description text.
  std::string make_option_desc(const CLI::Option* option) const override
  {
    auto txt = this->Superclass::make_option_desc(option);
    const size_t indent = this->get_column_width();
    return FormatParagraph(indent, txt);
  }

  std::string make_description(const CLI::App* app) const override
  {
    auto txt = this->Superclass::make_description(app);
    return FormatParagraph(0, txt);
  }

  std::string make_usage(const CLI::App* app, std::string name) const override
  {
    auto usage = this->Superclass::make_usage(app, name);
    if (app->get_parent() == nullptr)
    {
      usage +=
        "\n[General Guidelines]\n\n"
        "Values for options can be specified either with a space (' ') or an equal-to sign ('='). "
        "Thus, '--option=value' and '--option value' are equivalent.\n\n"
        "Multi-valued options can be specified by providing the option multiple times or "
        "separating "
        "the values by a comma (','). Thus, '--moption=foo,bar' and '--moption=foo --moption=bar' "
        "are equivalent.\n\n"
        "Short-named options with more than one character name are no longer supported, and should "
        "simply be specified as long-named option by adding an additional '-'. Thus, use '--dr' "
        "instead of "
        "'-dr'.\n\n"
        "Some options are described in this help text with the format '--moption TYPE:name' where "
        "'TYPE' specifies what kind of value is expected for the option 'name'. Type 'ENUM' means "
        "there "
        "are several possible predefined values for the option. Type 'TEXT' means that the "
        "filename option is any text string.\n\n"
        "Numeric specifiers 'INT' and 'FLOAT' in this help text indicate the expected option value "
        "type of "
        "some options, either an integer or floating point number.\n\n";
    }
    return FormatParagraph(0, usage);
  }
};
}

class vtkCLIOptions::vtkInternals
{
public:
  std::shared_ptr<CLI::App> App;
  vtkInternals()
    : App(new CLI::App())
  {
    // initialize defaults.
    this->App->formatter(std::make_shared<PVFormatter>());
    // ensures that `,` is treated as multi-option separator.
    this->App->option_defaults()->delimiter(',');
    this->App->prefix_command(true);
    this->App->allow_extras(true);
  }
};

vtkStandardNewMacro(vtkCLIOptions);
//----------------------------------------------------------------------------
vtkCLIOptions::vtkCLIOptions()
  : Internals(new vtkCLIOptions::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkCLIOptions::~vtkCLIOptions() = default;

//----------------------------------------------------------------------------
vtkCLIOptions* vtkCLIOptions::New(const std::shared_ptr<CLI::App>& app)
{
  auto result = vtkCLIOptions::New();
  if (app)
  {
    result->Internals->App = app;
  }
  return result;
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetAllowExtras(bool val)
{
  auto& internals = (*this->Internals);
  internals.App->allow_extras(val);
}

//----------------------------------------------------------------------------
bool vtkCLIOptions::GetAllowExtras() const
{
  auto& internals = (*this->Internals);
  return internals.App->get_allow_extras();
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetStopOnUnrecognizedArgument(bool val)
{
  auto& internals = (*this->Internals);
  /**
   * stop parsing after the first unrecognized command / option.
   */
  internals.App->prefix_command(!val);
}

//----------------------------------------------------------------------------
bool vtkCLIOptions::GetStopOnUnrecognizedArgument() const
{
  auto& internals = (*this->Internals);
  return !internals.App->get_prefix_command();
}

//----------------------------------------------------------------------------
CLI::App* vtkCLIOptions::GetCLI11App() const
{
  auto& internals = (*this->Internals);
  return internals.App.get();
}

//----------------------------------------------------------------------------
const char* vtkCLIOptions::GetHelp() const
{
  auto& internals = (*this->Internals);
  this->Help = internals.App->help();
  return this->Help.empty() ? nullptr : this->Help.c_str();
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetName(const char* name)
{
  auto& internals = (*this->Internals);
  internals.App->name(name ? name : "");
}

//----------------------------------------------------------------------------
const char* vtkCLIOptions::GetName() const
{
  auto& internals = (*this->Internals);
  return internals.App->get_name().c_str();
}

//----------------------------------------------------------------------------
void vtkCLIOptions::SetDescription(const char* desc)
{
  auto& internals = (*this->Internals);
  internals.App->description(desc ? desc : "");
}

//----------------------------------------------------------------------------
const char* vtkCLIOptions::GetDescription() const
{
  auto& internals = (*this->Internals);
  this->Description = internals.App->get_description();
  return this->Description.empty() ? nullptr : this->Description.c_str();
}

//----------------------------------------------------------------------------
bool vtkCLIOptions::Parse(int& argc, char**& argv)
{
  this->ExtraArguments.clear();
  this->LastErrorMessage.clear();
  this->HelpRequested = false;

  auto controller = vtkMultiProcessController::GetGlobalController();

  auto& internals = (*this->Internals);
  auto& app = (*internals.App);
  if (app.get_name().empty())
  {
    app.name(argv[0]);
  }

  const bool generateWarnings =
    this->GenerateWarnings && controller && (controller->GetLocalProcessId() == 0);

  bool encounteredPositionalSeparator = false;
  std::vector<std::string> args;
  args.reserve(argc - 1);

  vtksys::RegularExpression badShortName("^-[a-zA-Z0-9][a-zA-Z0-9]+(=.*)?$");
  vtksys::RegularExpression splitShortName("^(-[a-zA-Z0-9])=(.*)$");
  for (int cc = 1; cc < argc; ++cc)
  {
    auto arg = std::string(argv[cc]);
    if (!encounteredPositionalSeparator && this->HandleLegacyArgumentFormats &&
      badShortName.find(arg))
    {
      // Previously, we supported args like '-dr'. These are now converted to
      // '--dr'. This piece of code handles that automatically.
      if (generateWarnings)
      {
        vtkLogF(WARNING, "'%s' is deprecated. Use '-%s' instead", arg.c_str(), arg.c_str());
      }
      arg.insert(arg.begin(), '-');
    }
    else if (!encounteredPositionalSeparator && this->HandleLegacyArgumentFormats &&
      splitShortName.find(arg))
    {
      // Previously `-l=foo` was supported. CLI does not support `=` for short
      // names. We temporarily handle it.
      if (generateWarnings)
      {
        vtkLogF(WARNING, "short option names no longer support '='; use space ' ' instead for '%s'",
          arg.c_str());
      }
      args.emplace_back(splitShortName.match(1));
      arg = splitShortName.match(2);
    }
    else if (arg == "--")
    {
      // ensures that we stop auto-converting args after the first positional
      // separator is encountered.
      encounteredPositionalSeparator = true;
    }

    args.emplace_back(std::move(arg));
  }

  try
  {
    // CLI::App::parse(...) takes a reversed vector, so we do that.
    std::reverse(args.begin(), args.end());
    internals.App->parse(args);
    std::reverse(args.begin(), args.end());
    auto iter = args.begin();
    if (iter != args.end() && *iter == "--")
    {
      // remove positional separator, if found.
      ++iter;
    }

    std::copy(iter, args.end(), std::back_inserter(this->ExtraArguments));
  }
  catch (const CLI::CallForHelp&)
  {
    this->HelpRequested = true;
  }
  catch (const CLI::CallForAllHelp&)
  {
    this->HelpRequested = true;
  }
  catch (const CLI::ParseError& error)
  {
    this->LastErrorMessage = error.what();
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
std::string vtkCLIOptions::GetUsage() const
{
  const auto& app = (*this->Internals->App);
  auto formatter = app.get_formatter();
  assert(formatter);
  return dynamic_cast<CLI::Formatter*>(formatter.get())->make_usage(&app, app.get_name());
}

//----------------------------------------------------------------------------
void vtkCLIOptions::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HandleLegacyArgumentFormats: " << this->HandleLegacyArgumentFormats << endl;
  os << indent << "GenerateWarnings: " << this->GenerateWarnings << endl;
  os << indent << "HelpRequested: " << this->HelpRequested << endl;
}
