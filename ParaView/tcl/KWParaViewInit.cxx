#include "vtkTclUtil.h"
int vtkInteractorStyleCameraCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleCameraNewCommand();
int vtkInteractorStyleGridExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleGridExtentNewCommand();
int vtkInteractorStyleImageExtentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleImageExtentNewCommand();
int vtkInteractorStylePlaneCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStylePlaneNewCommand();
int vtkInteractorStylePlaneSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStylePlaneSourceNewCommand();
int vtkInteractorStyleSphereCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkInteractorStyleSphereNewCommand();
int vtkKWRenderViewCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkKWRenderViewNewCommand();
int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVApplicationNewCommand();
int vtkPVImagePlaneComponentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImagePlaneComponentNewCommand();
int vtkPVRenderSlaveCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRenderSlaveNewCommand();
int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRenderViewNewCommand();
int vtkPVSlaveCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSlaveNewCommand();
int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVWindowNewCommand();

extern Tcl_HashTable vtkInstanceLookup;
extern Tcl_HashTable vtkPointerLookup;
extern Tcl_HashTable vtkCommandLookup;
extern void vtkTclDeleteObjectFromHash(void *);
extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);


extern "C" {int VTK_EXPORT Vtkkwparaviewtcl_SafeInit(Tcl_Interp *interp);}



extern "C" {int VTK_EXPORT Vtkkwparaviewtcl_Init(Tcl_Interp *interp);}



extern void vtkTclGenericDeleteObject(ClientData cd);



int VTK_EXPORT Vtkkwparaviewtcl_SafeInit(Tcl_Interp *interp)
{
  return Vtkkwparaviewtcl_Init(interp);
}


int VTK_EXPORT Vtkkwparaviewtcl_Init(Tcl_Interp *interp)
{
  vtkTclCreateNew(interp,"vtkInteractorStyleCamera", vtkInteractorStyleCameraNewCommand,
                  vtkInteractorStyleCameraCommand);
  vtkTclCreateNew(interp,"vtkInteractorStyleGridExtent", vtkInteractorStyleGridExtentNewCommand,
                  vtkInteractorStyleGridExtentCommand);
  vtkTclCreateNew(interp,"vtkInteractorStyleImageExtent", vtkInteractorStyleImageExtentNewCommand,
                  vtkInteractorStyleImageExtentCommand);
  vtkTclCreateNew(interp,"vtkInteractorStylePlane", vtkInteractorStylePlaneNewCommand,
                  vtkInteractorStylePlaneCommand);
  vtkTclCreateNew(interp,"vtkInteractorStylePlaneSource", vtkInteractorStylePlaneSourceNewCommand,
                  vtkInteractorStylePlaneSourceCommand);
  vtkTclCreateNew(interp,"vtkInteractorStyleSphere", vtkInteractorStyleSphereNewCommand,
                  vtkInteractorStyleSphereCommand);
  vtkTclCreateNew(interp,"vtkKWRenderView", vtkKWRenderViewNewCommand,
                  vtkKWRenderViewCommand);
  vtkTclCreateNew(interp,"vtkPVApplication", vtkPVApplicationNewCommand,
                  vtkPVApplicationCommand);
  vtkTclCreateNew(interp,"vtkPVImagePlaneComponent", vtkPVImagePlaneComponentNewCommand,
                  vtkPVImagePlaneComponentCommand);
  vtkTclCreateNew(interp,"vtkPVRenderSlave", vtkPVRenderSlaveNewCommand,
                  vtkPVRenderSlaveCommand);
  vtkTclCreateNew(interp,"vtkPVRenderView", vtkPVRenderViewNewCommand,
                  vtkPVRenderViewCommand);
  vtkTclCreateNew(interp,"vtkPVSlave", vtkPVSlaveNewCommand,
                  vtkPVSlaveCommand);
  vtkTclCreateNew(interp,"vtkPVWindow", vtkPVWindowNewCommand,
                  vtkPVWindowCommand);
  return TCL_OK;
}
