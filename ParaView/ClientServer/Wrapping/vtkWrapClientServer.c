/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapClientServer.c
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
#include <stdio.h>
#include <string.h>
#include "vtkParse.h"

int numberOfWrappedFunctions = 0;
FunctionInfo *wrappedFunctions[1000];
extern FunctionInfo *currentFunction;


void output_temp(FILE *fp, int i, int aType, char *Id, int count)
{
  /* ignore void */
  if (((aType % 10) == 2)&&(!((aType%1000)/100)))
    {
    return;
    }
  
  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && (aType%2000 >= 1000))
    {
    fprintf(fp,"    const ");
    }
  else
    {
    fprintf(fp,"    ");
    }

  if ((aType%100)/10 == 1)
    {
    fprintf(fp,"unsigned ");
    }

  switch (aType%10)
    {
    case 1:   fprintf(fp,"float  "); break;
    case 7:   fprintf(fp,"double "); break;
    case 4:   fprintf(fp,"int    "); break;
    case 5:   fprintf(fp,"short  "); break;
    case 6:   fprintf(fp,"long   "); break;
    case 2:     fprintf(fp,"void   "); break;
    case 3:     fprintf(fp,"char   "); break;
    case 9:     fprintf(fp,"%s ",Id); break;
    case 8: return;
    }

  /* handle array arguements */
  if (count > 1)
    {
    fprintf(fp,"temp%i[%i];\n",i,count);
    return;
    }
  
  switch ((aType%1000)/100)
    {
    case 1: fprintf(fp, " *"); break; /* act " &" */
    case 2: fprintf(fp, "&&"); break;
    case 3: fprintf(fp, " *"); break;
    case 4: fprintf(fp, "&*"); break;
    case 5: fprintf(fp, "*&"); break;
    case 7: fprintf(fp, "**"); break;
    default: fprintf(fp,"  "); break;
    }
  
  fprintf(fp,"temp%i",i);
  fprintf(fp,";\n");
}

void checkClientServerType(int i, int aType, char *Id, int count, FILE *fp)
{
  static char tmp[256];
  int is_array = 0;
  int n_size = 0;

  (void)Id;
  
  tmp[0] = '\0';
  
  if (aType == 309)
    {
    fprintf(fp,"&&\n      msg->ArgumentTypes[%i] == vtkClientServerStream::vtk_object_pointer ",i+2);
    return;
    }
  
  if (aType%1000 == 303)
    {
    fprintf(fp,"&&\n      msg->ArgumentTypes[%i] == vtkClientServerStream::string_value ",i+2);
    return;
    }

  if ((aType%100)/10 == 1)
    {
    strcat(tmp,"unsigned_");
    }
  
  switch (aType%10)
    {
    case 1: strcat(tmp,"float"); n_size = sizeof(float); break;
    case 7: strcat(tmp,"double"); n_size = sizeof(double); break;
    case 4: strcat(tmp,"int"); n_size = sizeof(int); break;
    case 5: strcat(tmp,"short");n_size = sizeof(short);  break;
    case 6: strcat(tmp,"long"); n_size = sizeof(long); break;
    case 2: strcat(tmp,"void"); break;
    case 3: strcat(tmp,"char"); n_size = sizeof(char); break;
    case 9: strcat(tmp,"id"); break;
    }

  switch ((aType%1000)/100)
    {
    case 0: strcat(tmp,"_value"); break;
    case 3: strcat(tmp,"_array"); is_array = 1; break; 
/*    case 2: fprintf(fp, "&&"); break; */
/*    case 3: fprintf(fp, " *"); break; */
/*    case 4: fprintf(fp, "&*"); break; */
/*    case 5: fprintf(fp, "*&"); break; */
/*    case 7: fprintf(fp, "**"); break; */
    default: break;
    }

  /* handle array arguements */
  if (count > 1)
    {
/*    strcat(tmp,"_array"); */
    }

  fprintf(fp," &&\n      msg->ArgumentTypes[%i] == vtkClientServerStream::%s",i+2,tmp);
  if (is_array)
    {
    fprintf(fp," &&\n      msg->ArgumentSizes[%i] == %i",
            i+2, n_size*count);
    }
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE *fp)
{
  /* use the hint */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 307:  
    case 304: case 305: case 306: 
    case 313: case 314: case 315: case 316:
      fprintf(fp,"    *resultStream << vtkClientServerStream::Reply << vtkClientServerStream::InsertArray(temp%i,%i) << vtkClientServerStream::End;\n", MAX_ARGS, currentFunction->HintSize);
      break;
    }
}

void return_result(FILE *fp)
{
  switch (currentFunction->ReturnType%1000)
    {
    case 2:
      break;
    case 1: case 3: case 4: case 5: case 6: case 7: 
    case 13: case 14: case 15: case 16:
      fprintf(fp,"    *resultStream << vtkClientServerStream::Reply << temp%i << vtkClientServerStream::End;\n",
              MAX_ARGS); 
      break;
    case 303:
      fprintf(fp,"    if (temp%i)\n      {\n      *resultStream << vtkClientServerStream::Reply << temp%i << vtkClientServerStream::End;\n",MAX_ARGS,MAX_ARGS); 
      fprintf(fp,"      }\n");
      break;
    case 109:
    case 309:  
      fprintf(fp,"    *resultStream << vtkClientServerStream::Reply << (vtkObjectBase *)temp%i << vtkClientServerStream::End;\n",MAX_ARGS);
      break;

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case 301: case 307:
    case 304: case 305: case 306:
    case 313: case 314: case 315: case 316:      
      use_hints(fp);
      break;
    default:
      fprintf(fp,"    Tcl_SetResult(interp, (char *) \"unable to return result.\", TCL_VOLATILE);\n");
      break;
    }
}

void get_args(FILE *fp, int i)
{
  int j;
  int start_arg = 2;
  
  /* what arg do we start with */
  for (j = 0; j < i; j++)
    {
    start_arg = start_arg + 
      (currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
    }
  
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 10) == 2)&&
      (!((currentFunction->ArgTypes[i]%1000)/100)))
    {
    return;
    }
  
  switch (currentFunction->ArgTypes[i]%1000)
    {
    case 1:  
      fprintf(fp,"    temp%i = *(float *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;
    case 7:
      fprintf(fp,"    temp%i = *(double *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 4:
      fprintf(fp,"    temp%i = *(int *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 5:
      fprintf(fp,"    temp%i = *(short *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 6:
      fprintf(fp,"    temp%i = *(long *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 3:
      fprintf(fp,"    temp%i = *(char *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 13:
      fprintf(fp,"    temp%i = *(unsigned char *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 14:
      fprintf(fp,"    temp%i = *(unsigned int *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 15:
      fprintf(fp,"    temp%i = *(unsigned short *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 16:
      fprintf(fp,"    temp%i = *(unsigned long *)msg->Arguments[%i];\n", i, 
              start_arg);
      break;      
    case 303:
      fprintf(fp,"    temp%i = vtkClientServerInterpreter::GetString(msg,%i);\n",
              i,start_arg);
      break;
    case 109:
    case 309:
      fprintf(fp,
              "    temp%i = (%s *)(arlu->GetObjectFromMessage(msg,%i,1));\n",i,currentFunction->ArgClasses[i],start_arg);
      break;
    case 2:    
    case 9:
      break;
    default:
      if (currentFunction->ArgCounts[i] > 1)
        {
        switch (currentFunction->ArgTypes[i]%100)
          {
          case 1: case 7:
          case 4: case 5: case 6: 
          case 13: case 14: case 15: case 16: 
            fprintf(fp,"    memcpy(temp%i,msg->Arguments[%i],msg->ArgumentSizes[%i]);\n", 
                    i, start_arg, start_arg);
            break;
          }
        }
      
    }
}

void free_args(FILE *fp, int i)
{
  int j;
  int start_arg = 2;
  
  /* what arg do we start with */
  for (j = 0; j < i; j++)
    {
    start_arg = start_arg + 
      (currentFunction->ArgCounts[j] ? currentFunction->ArgCounts[j] : 1);
    }
  
  /* ignore void */
  if (((currentFunction->ArgTypes[i] % 10) == 2)&&
      (!((currentFunction->ArgTypes[i]%1000)/100)))
    {
    return;
    }
  
  switch (currentFunction->ArgTypes[i]%1000)
    {
    case 303:
      fprintf(fp,"    delete [] temp%i;\n",i);
      break;
    }
}

void outputFunction(FILE *fp, FileInfo *data)
{
  int i;
  int args_ok = 1;
 
  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator || 
      currentFunction->ArrayFailure ||
      !currentFunction->IsPublic ||
      !currentFunction->Name) 
    {
    return;
    }
  
  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
    {
    if ((currentFunction->ArgTypes[i]%10) == 8) args_ok = 0;
    /* if its a pointer arg make sure we have the ArgCount */
    if ((currentFunction->ArgTypes[i]%1000 >= 100) &&
        (currentFunction->ArgTypes[i]%1000 != 303)&&
        (currentFunction->ArgTypes[i]%1000 != 309)&&
        (currentFunction->ArgTypes[i]%1000 != 109)) 
      {
      if (currentFunction->NumberOfArguments > 1 ||
          !currentFunction->ArgCounts[i])
        {
        args_ok = 0;
        }
      }
    if ((currentFunction->ArgTypes[i]%100 >= 10)&&
        (currentFunction->ArgTypes[i] != 13)&&
        (currentFunction->ArgTypes[i] != 14)&&
        (currentFunction->ArgTypes[i] != 15)&&
        (currentFunction->ArgTypes[i] != 16)) 
      {
      args_ok = 0;
      }
    }

  /* if it returns an unknown class we cannot wrap it */
  if ((currentFunction->ReturnType%10) == 8) 
    {
    args_ok = 0;
    }

  if (((currentFunction->ReturnType%1000)/100 != 3)&&
      ((currentFunction->ReturnType%1000)/100 != 1)&&
      ((currentFunction->ReturnType%1000)/100)) 
    {
    args_ok = 0;
    }
  if (currentFunction->NumberOfArguments && 
      (currentFunction->ArgTypes[0] == 5000)) 
    {
    args_ok = 0;
    }

  /* we can't handle void * return types */
  if ((currentFunction->ReturnType%1000) == 302) 
    {
    args_ok = 0;
    }
  
  /* watch out for functions that dont have enough info */
  switch (currentFunction->ReturnType%1000)
    {
    case 301: case 307:
    case 304: case 305: case 306:
    case 313: case 314: case 315: case 316:
      args_ok = currentFunction->HaveHint;
      break;
    }
  
  /* if the args are OK and it is not a constructor or destructor */
  if (args_ok && 
      strcmp(data->ClassName,currentFunction->Name) &&
      strcmp(data->ClassName,currentFunction->Name + 1))
    {
    fprintf(fp,"  if (!strcmp(\"%s\",method) && msg->NumberOfArguments == %i ",
            currentFunction->Name, currentFunction->NumberOfArguments + 2);

    /* check the arg types as well */
    for (i = 0; i < currentFunction->NumberOfArguments; ++i)
      {
      checkClientServerType(i, currentFunction->ArgTypes[i],
                   currentFunction->ArgClasses[i], 
                   currentFunction->ArgCounts[i], fp);
      }
    fprintf(fp,")\n    {\n");
    
    /* process the args */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      output_temp(fp, i, currentFunction->ArgTypes[i],
                  currentFunction->ArgClasses[i], 
                  currentFunction->ArgCounts[i]);
      }
    output_temp(fp, MAX_ARGS,currentFunction->ReturnType,
                currentFunction->ReturnClass, 0);
    
    /* now get the required args from the stack */
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      get_args(fp,i);
      }
    
    switch (currentFunction->ReturnType%1000)
      {
      case 2:
        fprintf(fp,"    op->%s(",currentFunction->Name);
        break;
      case 109:
        fprintf(fp,"    temp%i = &(op)->%s(",MAX_ARGS,currentFunction->Name);
        break;
      default:
        fprintf(fp,"    temp%i = (op)->%s(",MAX_ARGS,currentFunction->Name);
      }
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      if (i)
        {
        fprintf(fp,",");
        }
      if (currentFunction->ArgTypes[i] == 109)
        {
        fprintf(fp,"*(temp%i)",i);
        }
      else
        {
        fprintf(fp,"temp%i",i);
        }
      }
    fprintf(fp,");\n");
    return_result(fp);
    for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
      free_args(fp,i);
      }
    fprintf(fp,"    return 0;\n");
    fprintf(fp,"    }\n");
    
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
    }

#if 0
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"  if (!strcmp(\"AddProgressObserver\",method) && msg->NumberOfArguments == 3 &&\n");
    fprintf(fp,"      msg->ArgumentTypes[2] == vtkClietnServerStream::string_value)\n");
    fprintf(fp,"    {\n");
    fprintf(fp,"    vtkClientServerProgressObserver *apo = vtkClientServerProgressObserver::New();\n");
    fprintf(fp,"    vtkObject* obj = arlu->GetObjectFromMessage(msg, 0, 1);\n");
    fprintf(fp,"    apo->SetFilterID(arlu->GetIDFromObject(obj));\n");
    fprintf(fp,"    apo->SetClientServerUtil(arlu);\n");
    fprintf(fp,"    char *temp0 = vtkClientServerInterpreter::GetString(msg,2);\n");
    fprintf(fp,"    op->AddObserver(temp0,apo);\n");
    fprintf(fp,"    apo->Delete();\n");
    fprintf(fp,"    delete [] temp0;\n");
    fprintf(fp,"    return 0;\n");
    fprintf(fp,"    }\n");
    }
#endif
}

/* print the parsed structures */
void vtkParseOutput(FILE *fp, FileInfo *data)
{
  int i;
  
  fprintf(fp,"// ClientServer wrapper for %s object\n//\n",data->ClassName);
  fprintf(fp,"#include \"vtkSystemIncludes.h\"\n");
  fprintf(fp,"#include \"%s.h\"\n",data->ClassName);
  fprintf(fp,"#include \"vtkClientServerInterpreter.h\"\n");
  fprintf(fp,"#include \"vtkClientServerStream.h\"\n\n");
  fprintf(fp,"#include \"vtkClientServerMessage.h\"\n\n");
  fprintf(fp,"#include \"vtkClientServerArrayInformation.h\"\n\n");
#if 0
  if (!strcmp("vtkObject",data->ClassName))
    {
    fprintf(fp,"#include \"vtkClientServerProgressObserver.h\"\n\n");
    }
#endif
  if (data->IsConcrete)
    {
    fprintf(fp,"\nvtkObjectBase *%sNewCommand()\n{\n",data->ClassName);
    fprintf(fp,"  return %s::New();\n}\n\n",data->ClassName);
    }
  
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
      {
      fprintf(fp,"int %sCommand(vtkClientServerInterpreter *, vtkObjectBase *, const char *, vtkClientServerMessage *, vtkClientServerStream* resultStream);\n",data->SuperClasses[i]);
      }
    }
  
  fprintf(fp,"\nint VTKClientServer_EXPORT %sCommand(vtkClientServerInterpreter *arlu, vtkObjectBase *ob, const char *method, vtkClientServerMessage *msg, vtkClientServerStream* resultStream)\n{\n",data->ClassName);
  
  if(strcmp(data->ClassName, "vtkObjectBase") == 0)
    {
    fprintf(fp,"  %s *op = ob;\n", 
            data->ClassName);
    }
  else
    {
    fprintf(fp,"  %s *op = %s::SafeDownCast(ob);\n", 
            data->ClassName, data->ClassName);
    }
  

  /*fprintf(fp,"  vtkClientServerStream resultStream;\n");*/

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    currentFunction = data->Functions + i;
    outputFunction(fp, data);
    }
  
  /* try superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    fprintf(fp,"\n  if (%sCommand(arlu, op,method,msg,resultStream) == 0)\n",
              data->SuperClasses[i]);
    fprintf(fp,"    {\n    return 0;\n    }\n");
    }
  
  fprintf(fp,"  vtkGenericWarningMacro(\"Object type: %s, could not find requested method: \" << method << \"\\nor the method was called with incorrect arguments.\\n\");\n", data->ClassName);
  fprintf(fp,"  return 1;\n}\n");
}
