/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWFindPath.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkKWFindPath.h"
#include "vtkObjectFactory.h"
#if defined(_MSC_VER) || defined(__BORLANDC__)
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#define PATHDELIMITER ';'
#else
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#define PATHDELIMITER ':'
#endif

#define vtkStrDup(x) \
   strcpy(new char[strlen(x)+1], x)

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define Getcwd(path,len) _getcwd(path,len)
#else
#define Getcwd(path,len) getcwd(path,len)
#endif

#if defined(_MSC_VER)
inline int Stat(const char *path)
{
  struct _stat buffer;
  return (_stat(path, &buffer) != -1);
}
#else
inline int Stat(const char *path)
{
  struct stat buffer;
  return (stat(path, &buffer) != -1);
}
#endif

vtkStandardNewMacro( vtkKWFindPath );

vtkKWFindPath::vtkKWFindPath()
{
  this->ExecutableName          = 0;
  this->ExecutablePath          = 0;
  this->AbsolutePath            = 0;
  this->LastFindPath            = 0;
  this->CurrentWorkingDirectory = 0;
}

vtkKWFindPath::~vtkKWFindPath()
{
  this->SetExecutableName(0);
  this->SetExecutablePath(0);
  this->SetAbsolutePath(0);
  this->SetLastFindPath(0);
  this->SetCurrentWorkingDirectory(0);
}

void vtkKWFindPath::Initialize(const char* argv0)
{
  if ( this->GetExecutablePath() )
    {
    vtkWarningMacro("Already initialized");
    return;
    }
  char buffer[1024];
  const char *progname = argv0;
  char *str = 0;  
  int found = 0;
  if ( ( str = strrchr( argv0, '\\' ) ) )
    {
    progname = str+1;
    found = 1;
    }
  else if ( ( str = strrchr( argv0, '/' ) ) )
    {
    progname = str+1;
    found = 1;
    }
  this->SetExecutableName(progname);
  Getcwd(buffer, 1024);
  this->SetCurrentWorkingDirectory(buffer);

  if ( found )
    {
    char *npath = vtkStrDup(argv0);
    npath[ strlen(argv0) - strlen(this->ExecutableName) ] = 0;
    if ( npath[strlen(npath)-1] == '\\' || npath[strlen(npath)-1] == '/' )
      {
      npath[strlen(npath)-1] = 0;
      }    
    if ( strlen(npath) > 0 )
      {
      //cout << "Directory: " << npath << endl;      
      const char *rpath = this->GetAbsolutePath(npath);
      if ( rpath )
	{	
	sprintf(buffer, "%s/%s", rpath, this->ExecutableName);	
	if ( Stat(buffer) )
	  {
	  cout << "Found in directory: " << rpath << endl;
	  this->SetExecutablePath(rpath);
	  }
	else
	  {
	  cout << "Error stating: " << buffer << ": " << strerror(0) << endl;
	  }
	}
      }
    delete [] npath;
    }
  else
    {
    sprintf(buffer, "%s/%s", this->GetCurrentWorkingDirectory(), 
	    this->ExecutableName);	
    if ( Stat (buffer) )
      {
      this->SetExecutablePath(this->GetCurrentWorkingDirectory());
      cout << "Found in local directory: " 
	   << this->GetCurrentWorkingDirectory() << endl;
      }     
    }

  if ( !this->GetExecutablePath() )
    {
    char* envpath = vtkStrDup(getenv("PATH"));
    if ( envpath )
      {
      istrstream is(envpath);
      char fullfile[1024];
      while(is.good())
	{
	is.getline(buffer, 1024, PATHDELIMITER);
	sprintf(fullfile, "%s/%s", buffer, progname);	
	//cout << "Entry: " << fullfile << endl;
	if ( Stat(fullfile) )
	  {
	  cout << "Found: " << fullfile << endl;
	  const char *realpath = this->GetAbsolutePath(buffer);
	  if ( realpath )
	    {
	    cout << "Realpath: " << realpath << endl;
	    this->SetExecutablePath(realpath);
	    }
	  }
	}
      }
    //cout << "Env PATH: " << envpath << endl;
    delete envpath;
    }
  cout << "Executable path: " << this->GetExecutablePath() << endl;
}

int vtkKWFindPath::ChangeDirectory(const char* path)
{
#if defined(_MSC_VER)
  return !_chdir(path);
#else
  return !chdir(path);
#endif 
}

const char* vtkKWFindPath::GetAbsolutePath(const char* path)
{
  int res = 0;
  char oldpath[1024];
  this->SetAbsolutePath(0);
  Getcwd(oldpath, 1024);
  if ( this->ChangeDirectory(path) )
    {
    char buffer[1024];
    Getcwd(buffer, 1024);
    this->SetAbsolutePath(buffer);
    res = 1;
    }
  this->ChangeDirectory(oldpath);
  return this->AbsolutePath; 
}

const char* vtkKWFindPath::FindDirectory(const char* directory)
{
  if ( !this->GetExecutablePath() )
    {
    vtkWarningMacro("Not yet initialized");
    return 0;
    }
  char buffer[1024];
  sprintf(buffer, "%s/%s", this->GetExecutablePath(), directory);
  this->SetLastFindPath(this->GetAbsolutePath(buffer));
  if ( this->LastFindPath )
    {
    return this->LastFindPath;
    }
  return 0;
}
