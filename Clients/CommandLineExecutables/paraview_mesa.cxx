/*=========================================================================

Program:   ParaView
Module:    paraview_mesa.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtksys/Process.h>
#include <vtksys/SystemTools.hxx>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

static char const* const mesa_relative_libdir = MESA_RELATIVE_LIBDIR;

#if defined(_WIN32)
static char const* const library_env = "PATH";
#elif defined(__APPLE__)
static char const* const library_env = "DYLD_LIBRARY_PATH";
#else
static char const* const library_env = "LD_LIBRARY_PATH";
#endif

#if defined(_WIN32)
static char const path_separator = ';';
#else
static char const path_separator = ':';
#endif

static char const* const tools[] = {
#ifdef HAVE_pvserver
  "pvserver",
#endif
#ifdef HAVE_pvdataserver
  "pvdataserver",
#endif
#ifdef HAVE_pvrenderserver
  "pvrenderserver",
#endif
#ifdef HAVE_pvpython
  "pvpython",
#endif
#ifdef HAVE_pvbatch
  "pvbatch",
#endif
#ifdef HAVE_paraview
  "paraview",
#endif
  nullptr
};

static char const* const backends[] = { "llvmpipe", "swr", nullptr };

void available(char const* name, char const* const* arr)
{
  std::cerr << "Available " << name << ":" << std::endl;
  char const* const* current = arr;
  while (*current)
  {
    std::cerr << "    " << *current << std::endl;
    ++current;
  }
}

void usage(char const* prog)
{
  std::cerr << prog << " <tool> [--backend <backend>] [--print] [--help] -- <tool args>"
            << std::endl
            << std::endl;

  available("tools", tools);
  std::cerr << std::endl;

  available("backends", backends);
}

void error(char const* msg, char const* arg)
{
  if (arg)
  {
    std::cerr << "error: " << msg << arg << std::endl;
  }
  else
  {
    std::cerr << "error: " << msg << std::endl;
  }
}

void set_mesa_env(bool print, char const* var, const char* value)
{
  std::string const setting = std::string(var) + "=" + value;
  vtksys::SystemTools::PutEnv(setting);
  if (print)
  {
    std::cerr << setting << std::endl;
  }
}

std::string current_exe_dir(char const* prog)
{
  std::string exe_dir;
#if defined(_WIN32) && !defined(__CYGWIN__)
  (void)prog; // ignore this on windows
  wchar_t modulepath[_MAX_PATH];
  ::GetModuleFileNameW(NULL, modulepath, sizeof(modulepath));
  std::string path = vtksys::Encoding::ToNarrow(modulepath);
  std::string realPath = vtksys::SystemTools::GetRealPathResolvingWindowsSubst(path, NULL);
  if (realPath.empty())
  {
    realPath = path;
  }
  exe_dir = vtksys::SystemTools::GetFilenamePath(realPath);
#elif defined(__APPLE__)
  (void)prog; // ignore this on OS X
#define CM_EXE_PATH_LOCAL_SIZE 16384
  char exe_path_local[CM_EXE_PATH_LOCAL_SIZE];
#if defined(MAC_OS_X_VERSION_10_3) && !defined(MAC_OS_X_VERSION_10_4)
  unsigned long exe_path_size = CM_EXE_PATH_LOCAL_SIZE;
#else
  uint32_t exe_path_size = CM_EXE_PATH_LOCAL_SIZE;
#endif
#undef CM_EXE_PATH_LOCAL_SIZE
  char* exe_path = exe_path_local;
  if (_NSGetExecutablePath(exe_path, &exe_path_size) < 0)
  {
    exe_path = static_cast<char*>(malloc(exe_path_size));
    _NSGetExecutablePath(exe_path, &exe_path_size);
  }
  exe_dir = vtksys::SystemTools::GetFilenamePath(vtksys::SystemTools::GetRealPath(exe_path));
  if (exe_path != exe_path_local)
  {
    free(exe_path);
  }
#else
  std::string errorMsg;
  std::string exe;
  if (vtksys::SystemTools::FindProgramPath(prog, exe, errorMsg))
  {
    // remove symlinks
    exe = vtksys::SystemTools::GetRealPath(exe);
    exe_dir = vtksys::SystemTools::GetFilenamePath(exe);
  }
  else
  {
    // ???
  }
#endif

  return exe_dir;
}

int main(int argc, char* argv[])
{
  char const* backend = nullptr;
  char const* tool = nullptr;
  bool in_tool_args = false;
  bool print = false;
  std::vector<char const*> args;

  if (argc == 1)
  {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  for (int i = 1; i < argc; ++i)
  {
    char const* arg = argv[i];

    if (in_tool_args)
    {
      args.push_back(arg);
      continue;
    }

    if (!strcmp(arg, "--help"))
    {
      usage(argv[0]);
      return EXIT_FAILURE;
    }
    else if (!strcmp(arg, "--print"))
    {
      print = true;
    }
    else if (!strcmp(arg, "--backend"))
    {
      ++i;
      if (argc <= i)
      {
        error("--backend requires an argument", nullptr);
        return EXIT_FAILURE;
      }
      if (backend)
      {
        error("--backend may only be specified once", nullptr);
        return EXIT_FAILURE;
      }

      backend = argv[i];

      // Check that the backend argument is valid.
      bool have_backend = false;
      char const* const* current_backend = backends;
      while (*current_backend)
      {
        if (!strcmp(*current_backend, backend))
        {
          have_backend = true;
          break;
        }
        ++current_backend;
      }

      if (!have_backend)
      {
        error("unknown backend: ", backend);
        available("backends", backends);
        return EXIT_FAILURE;
      }
    }
    else if (!strcmp(arg, "--"))
    {
      in_tool_args = true;
    }
    else if (!strncmp(arg, "--", 2))
    {
      error("unknown flag: ", arg);
      return EXIT_FAILURE;
    }
    else if (tool)
    {
      error("only one tool may be specified", nullptr);
      return EXIT_FAILURE;
    }
    else
    {
      tool = arg;

      // Check that the tool argument is valid.
      bool have_tool = false;
      char const* const* current_tool = tools;
      while (*current_tool)
      {
        if (!strcmp(*current_tool, tool))
        {
          have_tool = true;
          break;
        }
        ++current_tool;
      }

      if (!have_tool)
      {
        error("unknown tool: ", tool);
        available("tools", tools);
        return EXIT_FAILURE;
      }
    }
  }

  if (!tool)
  {
    error("no tool specified", nullptr);
    available("tools", tools);
    return EXIT_FAILURE;
  }

  // Set up the environment to use Mesa.
  std::string const exe_dir = current_exe_dir(argv[0]);
  std::string const mesa_libdir = exe_dir + "/" + mesa_relative_libdir;
  char const* cur_value = vtksys::SystemTools::GetEnv(library_env);
  std::string new_library_env;
  // Unset -> use just the new path.
  if (!cur_value)
  {
    new_library_env = mesa_libdir;
  }
  // Empty -> use just the new path.
  else if (!*cur_value)
  {
    new_library_env = mesa_libdir;
  }
  // Prepend it with the separator.
  else
  {
    new_library_env = mesa_libdir + path_separator + cur_value;
  }
  set_mesa_env(print, library_env, mesa_libdir.c_str());
  if (backend)
  {
    set_mesa_env(print, "GALLIUM_DRIVER", backend);
  }

  // Build the command line for the tool.
  std::string const tool_path = exe_dir + "/" + tool;
  // Insert the program to run as argv[0].
  args.insert(args.begin(), tool_path.c_str());
  // Append a NULL at the end of the array.
  args.push_back(nullptr);

  // Run the tool with the arguments given.
  auto proc = vtksysProcess_New();
  vtksysProcess_SetCommand(proc, args.data());
  vtksysProcess_SetPipeShared(proc, vtksysProcess_Pipe_STDOUT, 1);
  vtksysProcess_SetPipeShared(proc, vtksysProcess_Pipe_STDERR, 1);
  vtksysProcess_Execute(proc);
  vtksysProcess_WaitForExit(proc, nullptr);

  // Extract the result of the command.
  int const state = vtksysProcess_GetState(proc);
  int ret = EXIT_FAILURE;
  if (state == vtksysProcess_State_Exited)
  {
    ret = vtksysProcess_GetExitValue(proc);
  }
  else if (state == vtksysProcess_State_Exception)
  {
    const char* exception_str = vtksysProcess_GetExceptionString(proc);
    error("exception occurred: ", exception_str);
  }
  else if (state == vtksysProcess_State_Error)
  {
    const char* error_str = vtksysProcess_GetErrorString(proc);
    error("process error: ", error_str);
  }
  else if (state == vtksysProcess_State_Expired)
  {
    error("timeout error", nullptr);
  }

  // Cleanup.
  vtksysProcess_Delete(proc);

  return ret;
}
