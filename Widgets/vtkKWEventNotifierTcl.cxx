// tcl wrapper for vtkKWEventNotifier object
//
#include "vtkSystemIncludes.h"
#include "vtkKWEventNotifier.h"

#include "vtkTclUtil.h"

ClientData vtkKWEventNotifierNewCommand()
{
  vtkKWEventNotifier *temp = vtkKWEventNotifier::New();
  return ((ClientData)temp);
}

int vtkKWObjectCppCommand(vtkKWObject *op, Tcl_Interp *interp,
             int argc, char *argv[]);
int VTKTCL_EXPORT vtkKWEventNotifierCppCommand(vtkKWEventNotifier *op, Tcl_Interp *interp,
             int argc, char *argv[]);

int VTKTCL_EXPORT vtkKWEventNotifierCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[])
{
  if ((argc == 2)&&(!strcmp("Delete",argv[1]))&& !vtkTclInDelete())
    {
    Tcl_DeleteCommand(interp,argv[0]);
    return TCL_OK;
    }
   return vtkKWEventNotifierCppCommand((vtkKWEventNotifier *)cd,interp, argc, argv);
}

int VTKTCL_EXPORT vtkKWEventNotifierCppCommand(vtkKWEventNotifier *op, Tcl_Interp *interp,
             int argc, char *argv[])
{
  int    tempi;
  double tempd;
  static char temps[80];
  int    error;

  error = 0; error = error;
  tempi = 0; tempi = tempi;
  tempd = 0; tempd = tempd;
  temps[0] = 0; temps[0] = temps[0];

  if (argc < 2)
    {
    Tcl_SetResult(interp, "Could not find requested method.", TCL_VOLATILE);
    return TCL_ERROR;
    }
  if (!interp)
    {
    if (!strcmp("DoTypecasting",argv[0]))
      {
      if (!strcmp("vtkKWEventNotifier",argv[1]))
        {
        argv[2] = (char *)((void *)op);
        return TCL_OK;
        }
      if (vtkKWObjectCppCommand((vtkKWObject *)op,interp,argc,argv) == TCL_OK)
        {
        return TCL_OK;
        }
      }
    return TCL_ERROR;
    }

  if (!strcmp("GetSuperClassName",argv[1]))
    {
    Tcl_SetResult(interp,"vtkKWObject", TCL_VOLATILE);
    return TCL_OK;
    }

  if ((!strcmp("New",argv[1]))&&(argc == 2))
    {
    vtkKWEventNotifier  *temp20;
    int vtkKWEventNotifierCommand(ClientData, Tcl_Interp *, int, char *[]);
    error = 0;

    if (!error)
      {
      temp20 = (op)->New();
      vtkTclGetObjectFromPointer(interp,(void *)temp20,vtkKWEventNotifierCommand);
      return TCL_OK;
      }
    }
  if ((!strcmp("GetClassName",argv[1]))&&(argc == 2))
    {
    const char    *temp20;
    error = 0;

    if (!error)
      {
      temp20 = (op)->GetClassName();
      if (temp20)
        {
        Tcl_SetResult(interp, (char*)temp20, TCL_VOLATILE);
        }
      else
        {
        Tcl_ResetResult(interp);
        }
      return TCL_OK;
      }
    }
  if ((!strcmp("IsA",argv[1]))&&(argc == 3))
    {
    char    *temp0;
    int      temp20;
    error = 0;

    temp0 = argv[2];
    if (!error)
      {
      temp20 = (op)->IsA(temp0);
      char tempResult[1024];
      sprintf(tempResult,"%i",temp20);
      Tcl_SetResult(interp, tempResult, TCL_VOLATILE);
      return TCL_OK;
      }
    }
  if ((!strcmp("AddCallback",argv[1]))&&(argc == 6))
    {
    char    *temp0;
    vtkKWWindow  *temp1;
    vtkKWObject  *temp2;
    char    *temp3;
    error = 0;

    temp0 = argv[2];
    temp1 = (vtkKWWindow *)(vtkTclGetPointerFromObject(argv[3],"vtkKWWindow",interp,error));
    temp2 = (vtkKWObject *)(vtkTclGetPointerFromObject(argv[4],"vtkKWObject",interp,error));
    temp3 = argv[5];
    if (!error)
      {
      op->AddCallback(temp0,temp1,temp2,temp3);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("RemoveCallback",argv[1]))&&(argc == 6))
    {
    char    *temp0;
    vtkKWWindow  *temp1;
    vtkKWObject  *temp2;
    char    *temp3;
    error = 0;

    temp0 = argv[2];
    temp1 = (vtkKWWindow *)(vtkTclGetPointerFromObject(argv[3],"vtkKWWindow",interp,error));
    temp2 = (vtkKWObject *)(vtkTclGetPointerFromObject(argv[4],"vtkKWObject",interp,error));
    temp3 = argv[5];
    if (!error)
      {
      op->RemoveCallback(temp0,temp1,temp2,temp3);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }
  if ((!strcmp("InvokeCallbacks",argv[1]))&&(argc == 5))
    {
    char    *temp0;
    vtkKWWindow  *temp1;
    char    *temp2;
    error = 0;

    temp0 = argv[2];
    temp1 = (vtkKWWindow *)(vtkTclGetPointerFromObject(argv[3],"vtkKWWindow",interp,error));
    temp2 = argv[4];
    if (!error)
      {
      op->InvokeCallbacks(temp0,temp1,temp2);
      Tcl_ResetResult(interp);
      return TCL_OK;
      }
    }

  if (!strcmp("ListInstances",argv[1]))
    {
    vtkTclListInstances(interp,(ClientData)vtkKWEventNotifierCommand);
    return TCL_OK;
    }

  if (!strcmp("ListMethods",argv[1]))
    {
    vtkKWObjectCppCommand(op,interp,argc,argv);
    Tcl_AppendResult(interp,"Methods from vtkKWEventNotifier:\n",NULL);
    Tcl_AppendResult(interp,"  GetSuperClassName\n",NULL);
    Tcl_AppendResult(interp,"  New\n",NULL);
    Tcl_AppendResult(interp,"  GetClassName\n",NULL);
    Tcl_AppendResult(interp,"  IsA\t with 1 arg\n",NULL);
    Tcl_AppendResult(interp,"  AddCallback\t with 4 args\n",NULL);
    Tcl_AppendResult(interp,"  RemoveCallback\t with 4 args\n",NULL);
    Tcl_AppendResult(interp,"  InvokeCallbacks\t with 3 args\n",NULL);
    return TCL_OK;
    }

  if (vtkKWObjectCppCommand((vtkKWObject *)op,interp,argc,argv) == TCL_OK)
    {
    return TCL_OK;
    }

  if ((argc >= 2)&&(!strstr(interp->result,"Object named:")))
    {
    char temps2[256];
    sprintf(temps2,"Object named: %s, could not find requested method: %s\nor the method was called with incorrect arguments.\n",argv[0],argv[1]);
    Tcl_AppendResult(interp,temps2,NULL);
    }
  return TCL_ERROR;
}
