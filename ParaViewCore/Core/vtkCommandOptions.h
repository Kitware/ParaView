/*=========================================================================

  Program:   ParaView
  Module:    vtkCommandOptions.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCommandOptions
 * @brief   ParaView options storage
 *
 * An object of this class represents a storage for ParaView options
 *
 * These options can be retrieved during run-time, set using configuration file
 * or using Command Line Arguments.
*/

#ifndef vtkCommandOptions_h
#define vtkCommandOptions_h

#include "vtkObject.h"
#include "vtkPVCoreModule.h" // needed for export macro

class vtkCommandOptionsInternal;
class vtkCommandOptionsXMLParser;

class VTKPVCORE_EXPORT vtkCommandOptions : public vtkObject
{
public:
  static vtkCommandOptions* New();
  vtkTypeMacro(vtkCommandOptions, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int Parse(int argc, const char* const argv[]);
  void GetRemainingArguments(int* argc, char** argv[]);

  enum
  {
    EVERYBODY = 0,
    XMLONLY = 0x1
  };

  const char* GetHelp();

  //@{
  /**
   * Was help selected?
   */
  vtkGetMacro(HelpSelected, int);
  vtkSetMacro(HelpSelected, int);
  //@}

  /**
   * Set/Get the type of the process for this set of options.
   * data-server, render-server, combined-server or client.
   */
  int GetProcessType() { return this->ProcessType; }
  void SetProcessType(int p) { this->ProcessType = p; }

  //@{
  /**
   * In case of unknown argument, set this variable with the unknown argument.
   */
  vtkGetStringMacro(UnknownArgument);
  //@}

  //@{
  /**
   * Get the error message if Parse returned 0.
   */
  vtkGetStringMacro(ErrorMessage);
  //@}

  /**
   * Get argv[0]
   */
  const char* GetArgv0();

  //@{
  /**
   * Get full path of executable (based on Argv0)
   */
  vtkGetStringMacro(ApplicationPath);
  //@}

  /**
   * Get the index of the last argument parsed.
   */
  int GetLastArgument();

  /**
   * Pass in the name and the attributes for all tags that are not Options.
   * If it returns 1, then it is successful, and 0 if it failed.
   */
  virtual int ParseExtraXMLTag(const char*, const char**) { return 1; }

protected:
  /**
   * Default constructor.
   */
  vtkCommandOptions();

  /**
   * Destructor.
   */
  ~vtkCommandOptions() override;

  /**
   * Prototype for callbacks.
   */
  typedef int (*CallbackType)(const char* argument, const char* value, void* call_data);

  //@{
  /**
   * Add a command line option.  For each argument added there is a long
   * version --long and a short version -l, a help string, and a variable
   * that is set to the value of the option.  The types can be int, char*, or
   * boolean (set to 1 of option is present).  Also deprecated arguments can
   * be added with only a help string.  The help string should say that the
   * argument is deprecated and suggest the alternative argument to use.
   * Each option can specify in a bit flag int the processes that the option
   * is valid for, the default is to be valid for all paraview processes.
   */
  void AddBooleanArgument(
    const char* longarg, const char* shortarg, int* var, const char* help, int type = EVERYBODY);
  void AddDeprecatedArgument(
    const char* longarg, const char* shortarg, const char* help, int type = EVERYBODY);
  void AddArgument(
    const char* longarg, const char* shortarg, int* var, const char* help, int type = EVERYBODY);
  void AddArgument(
    const char* longarg, const char* shortarg, char** var, const char* help, int type = EVERYBODY);
  //@}

  void AddCallback(const char* longarg, const char* shortarg, CallbackType callback,
    void* call_data, const char* help, int type = EVERYBODY);

  /**
   * Initialize arguments.
   */
  virtual void Initialize();

  /**
   * After parsing, process extra option dependencies.
   */
  virtual int PostProcess(int argc, const char* const* argv);

  /**
   * This method is called when wrong argument is found. If it returns 0, then
   * the parsing will fail.
   */
  virtual int WrongArgument(const char* argument);

  /**
   * This method is called when a deprecated argument is found. If it returns 0, then
   * the parsing will fail.
   */
  virtual int DeprecatedArgument(const char* argument);

  /**
   * This method loads the paraview config file.  The command line
   * will override any of the values in this file, but all options can
   * be in the file.
   */
  int LoadXMLConfigFile(const char*);

  vtkSetStringMacro(UnknownArgument);

  vtkSetStringMacro(ErrorMessage);

  // Options:
  vtkSetStringMacro(XMLConfigFile);

  void CleanArgcArgv();

  vtkSetStringMacro(ApplicationPath);
  void ComputeApplicationPath();

  vtkCommandOptionsXMLParser* XMLParser;

private:
  int Argc;
  char** Argv;
  int HelpSelected;
  char* UnknownArgument;
  char* ErrorMessage;
  char* XMLConfigFile;
  char* ApplicationPath;
  int ProcessType; // data-server, render-server, combined-server, client

  vtkCommandOptions(const vtkCommandOptions&) = delete;
  void operator=(const vtkCommandOptions&) = delete;

  vtkCommandOptionsInternal* Internals;

  static int UnknownArgumentHandler(const char* argument, void* call_data);
  static int DeprecatedArgumentHandler(const char* argument, const char* value, void* call_data);
};

#endif
