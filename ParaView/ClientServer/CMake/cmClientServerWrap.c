/*=========================================================================

  Program:   Visualization Toolkit
  Module:    cmClientServerWrap.c
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
/* this is a CMake loadable command to wrap vtk objects into Tcl */

#include "cmCPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct 
{
  char *LibraryName;
  int NumberWrapped;
  char **SourceFiles;
  char **HeaderFiles;
} cmVTKWrapClientServerData;

/* this routine creates the init file */
static void CreateInitFile(cmLoadedCommandInfo *info,
                           void *mf, const char *libName, 
                           int numConcrete, const char **concrete) 
{
  /* we have to make sure that the name is the correct case */
  char *kitName = info->CAPI->Capitalized(libName);
  int i;
  char *tempOutputFile;  
  char *outFileName = 
    (char *)malloc(strlen(info->CAPI->GetCurrentOutputDirectory(mf)) + 
                   strlen(libName) + 10);
  FILE *fout;
  
  sprintf(outFileName,"%s/%sInit.cxx",
          info->CAPI->GetCurrentOutputDirectory(mf), libName);
  
  tempOutputFile = (char *)malloc(strlen(outFileName) + 5);
  sprintf(tempOutputFile,"%s.tmp",outFileName);
  fout = fopen(tempOutputFile,"w");
  if (!fout)
    {
    return;
    }
  
  fprintf(fout,"#include \"vtkClientServerInterpreter.h\"\n");
  fprintf(fout,"\n\nstatic int %s_NewInstance(vtkClientServerInterpreter *arlu, const char *type, vtkClientServerID id);\n",kitName);
  for (i = 0; i < numConcrete; i++)
    {
    fprintf(fout,"int %sCommand(vtkClientServerInterpreter *, vtkObjectBase *, const char *, const vtkClientServerStream&, vtkClientServerStream& resultStrem);\n",concrete[i]);
    fprintf(fout,"vtkObjectBase *%sNewCommand();\n",concrete[i]);
    }
  
  fprintf(fout,"\n\nvoid VTK_EXPORT %s_Initialize(vtkClientServerInterpreter *arlu)\n{\n",kitName);
  fprintf(fout,"\n  arlu->AddNewInstanceFunction( %s_NewInstance);\n", kitName);
  for (i = 0; i < numConcrete; i++)
    {
    fprintf(fout,
            "  arlu->AddCommandFunction(\"%s\",%sCommand);\n"
            ,concrete[i], concrete[i]);
    }
  fprintf(fout,"}\n");

  /* the main declaration */
  fprintf(fout,"\n\nstatic int %s_NewInstance(vtkClientServerInterpreter *arlu, const char *type, vtkClientServerID id)\n{\n",kitName);
  
  for (i = 0; i < numConcrete; i++)
    {
    fprintf(fout,
            "    if (!strcmp(\"%s\",type))\n"
            "      {\n"
            "      vtkObjectBase *ptr = %sNewCommand();\n"
            "      arlu->NewInstance(ptr,id);\n"
            "      return 1;\n"
            "      }\n"
            ,concrete[i], concrete[i]);
    }
  fprintf(fout,
          "  return 0;\n}\n");
  fclose(fout);

  /* copy the file if different */
  info->CAPI->CopyFileIfDifferent(tempOutputFile, outFileName);
  info->CAPI->RemoveFile(tempOutputFile);
}

/* do almost everything in the initial pass */
static int InitialPass(void *inf, void *mf, int argc, char *argv[])
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  int i;
  int newArgc;
  char **newArgv;
  char **sources = 0;
  int numSources = 0;
  int numConcrete = 0;
  char **concrete = 0;
  int numWrapped = 0;
  cmVTKWrapClientServerData *cdata = 
    (cmVTKWrapClientServerData *)malloc(sizeof(cmVTKWrapClientServerData));
  
  if(argc < 2)
    {
    info->CAPI->SetError(info, "called with incorrect number of arguments");
    return 0;
    }
  
  info->CAPI->ExpandSourceListArguments(mf, argc, argv, 
                                        &newArgc, &newArgv, 2);
  
  /* extract the sources and commands parameters */
  sources = (char **)malloc(sizeof(char *)*newArgc);
  concrete = (char **)malloc(sizeof(char *)*newArgc);
  cdata->SourceFiles = (char **)malloc(sizeof(char *)*newArgc);
  cdata->HeaderFiles = (char **)malloc(sizeof(char *)*newArgc);
  
  for(i = 1; i < newArgc; ++i)
    {   
    sources[numSources] = newArgv[i];
    numSources++;
    }
  
  /* get the list of classes for this library */
  if (numSources)
    {
    /* what is the current source dir */
    const char *cdir = info->CAPI->GetCurrentDirectory(mf);
    int sourceListSize = 0;
    char *sourceListValue = 0;
    void *cfile = 0;
    char *newName;

    /* was the list already populated */
    const char *def = info->CAPI->GetDefinition(mf, sources[0]);

    /* Calculate size of source list.  */
    /* Start with list of source files.  */
    sourceListSize = info->CAPI->GetTotalArgumentSize(newArgc,newArgv);
    /* Add enough to extend the name of each class. */
    sourceListSize += numSources*strlen("ClientServer.cxx");
    /* Add enough to include the def and init file.  */
    sourceListSize += def?strlen(def):0;
    sourceListSize += strlen(";Init.cxx");

    /* Allocate and initialize the source list.  */
    sourceListValue = (char *)malloc(sourceListSize);
    if (def)
      {
      sprintf(sourceListValue,"%s;%sInit.cxx",def,argv[0]);
      }
    else
      {
      sprintf(sourceListValue,"%sInit.cxx",argv[0]);
      }
    
    for(i = 1; i < numSources; ++i)
      {   
      void *curr = info->CAPI->GetSource(mf,sources[i]);
      
      /* if we should wrap the class */
      if (!curr || 
          !info->CAPI->SourceFileGetPropertyAsBool(curr,"WRAP_EXCLUDE"))
        {
        void *file = info->CAPI->CreateSourceFile();
        char *srcName;
        char *pathName;
        char *hname;
        int addLength;
        
        addLength = strlen("ClientServer")+1;
        srcName = info->CAPI->GetFilenameWithoutExtension(sources[i]);
        pathName = info->CAPI->GetFilenamePath(sources[i]);
        if (curr)
          {
          int abst = info->CAPI->SourceFileGetPropertyAsBool(curr,"ABSTRACT");
          info->CAPI->SourceFileSetProperty(file,"ABSTRACT",
                                            (abst ? "1" : "0"));
          
          if (!abst)
            {
            concrete[numConcrete] = strdup(srcName);
            numConcrete++;
            }
         }
        else
          {
          concrete[numConcrete] = strdup(srcName);
          numConcrete++;
          }
        newName = (char *)malloc(strlen(srcName)+addLength);
        sprintf(newName,"%sClientServer",srcName);
        info->CAPI->SourceFileSetName2(file, newName, 
                                       info->CAPI->GetCurrentOutputDirectory(mf),
                                       "cxx",0);
        
        if (strlen(pathName) > 1)
          {
          hname = (char *)malloc(strlen(pathName) + strlen(srcName) + addLength);
          sprintf(hname,"%s/%s.h",pathName,srcName);
          }
        else
          {
          hname = (char *)malloc(strlen(cdir) + strlen(srcName) + addLength);
          sprintf(hname,"%s/%s.h",cdir,srcName);
          }
        /* add starting depends */
        info->CAPI->SourceFileAddDepend(file,hname);
        info->CAPI->AddSource(mf,file);
        cdata->SourceFiles[numWrapped] = file;
        cdata->HeaderFiles[numWrapped] = hname;
        numWrapped++;
        strcat(sourceListValue,";");
        strcat(sourceListValue,newName);
        strcat(sourceListValue,".cxx");        
        free(newName);
        }
      }
    /* add the init file */
    cfile = info->CAPI->CreateSourceFile();
    info->CAPI->SourceFileSetProperty(cfile,"ABSTRACT","0");
    newName = (char *)malloc(strlen(argv[0]) + 5);
    sprintf(newName,"%sInit",argv[0]);
    CreateInitFile(info,mf,argv[0],numConcrete,concrete);
    info->CAPI->SourceFileSetName2(cfile, newName, 
                                   info->CAPI->GetCurrentOutputDirectory(mf),
                                   "cxx",0);
    free(newName);
    info->CAPI->AddSource(mf,cfile);
    info->CAPI->AddDefinition(mf, sources[0], sourceListValue);  
    }

  /* store key data in the CLientData for the final pass */
  cdata->NumberWrapped = numWrapped;
  cdata->LibraryName = strdup(argv[0]);
  info->CAPI->SetClientData(info,cdata);
  
  free(sources);
  for(i=0;i<numConcrete; i++)
    {
    free(concrete[i]);
    }
  free(concrete);
  info->CAPI->FreeArguments(newArgc, newArgv);

  return 1;
}


static void FinalPass(void *inf, void *mf) 
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  /* get our client data from initial pass */
  cmVTKWrapClientServerData *cdata = 
    (cmVTKWrapClientServerData *)info->CAPI->GetClientData(info);
  
  /* first we add the rules for all the .h to ClientServer.cxx files */
  const char *wtcl = "${VTK_WRAP_ClientServer_EXE}";
  const char *hints = info->CAPI->GetDefinition(mf,"VTK_WRAP_HINTS");
  const char *args[4];
  const char *depends[2];
  int i;
  int numDepends, numArgs;
  const char *cdir = info->CAPI->GetCurrentDirectory(mf);
  
  /* wrap all the .h files */
  depends[0] = wtcl;
  numDepends = 1;
  if (hints)
    {
    depends[1] = hints;
    numDepends++;
    }
  for(i = 0; i < cdata->NumberWrapped; i++)
    {
    char *res;
    const char *srcName = info->CAPI->SourceFileGetSourceName(cdata->SourceFiles[i]);
    args[0] = cdata->HeaderFiles[i];
    numArgs = 1;
    if (hints)
      {
      args[1] = hints;
      numArgs++;
      }
    args[numArgs] = 
      (info->CAPI->SourceFileGetPropertyAsBool(cdata->SourceFiles[i],"ABSTRACT") ?"0" :"1");
    numArgs++;
    res = (char *)malloc(strlen(info->CAPI->GetCurrentOutputDirectory(mf)) + 
                         strlen(srcName) + 6);
    sprintf(res,"%s/%s.cxx",info->CAPI->GetCurrentOutputDirectory(mf),srcName);
    args[numArgs] = res;
    numArgs++;
    info->CAPI->AddCustomCommand(mf, args[0],
                       wtcl, numArgs, args, numDepends, depends, 
                       1, &res, cdata->LibraryName);
    free(res);
    }
}

static void Destructor(void *inf) 
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  /* get our client data from initial pass */
  cmVTKWrapClientServerData *cdata = 
    (cmVTKWrapClientServerData *)info->CAPI->GetClientData(info);
  if (cdata)
    {
    info->CAPI->FreeArguments(cdata->NumberWrapped, cdata->SourceFiles);
    info->CAPI->FreeArguments(cdata->NumberWrapped, cdata->HeaderFiles);
    free(cdata->LibraryName);
    free(cdata);
    }
}

static const char* GetTerseDocumentation() 
{
  return "Create ClientServer Wrappers for VTK classes.";
}

static const char* GetFullDocumentation()
{
  return
    "VTK_WRAP_ClientServer (resultingLibraryName SourceListName SourceLists )";
}

void CM_PLUGIN_EXPORT VTK_WRAP_ClientServerInit(cmLoadedCommandInfo *info)
{
  info->InitialPass = InitialPass;
  info->FinalPass = FinalPass;
  info->Destructor = Destructor;
  info->m_Inherited = 0;
  info->GetTerseDocumentation = GetTerseDocumentation;
  info->GetFullDocumentation = GetFullDocumentation;  
  info->Name = "VTK_WRAP_ClientServer";
}
