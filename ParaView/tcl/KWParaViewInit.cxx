#include "vtkTclUtil.h"
int vtkColorByProcessCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkColorByProcessNewCommand();
int vtkDummyRenderWindowInteractorCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkDummyRenderWindowInteractorNewCommand();
int vtkGetRemoteGhostCellsCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkGetRemoteGhostCellsNewCommand();
int vtkImageOutlineFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkImageOutlineFilterNewCommand();
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
int vtkParallelDecimateCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkParallelDecimateNewCommand();
int vtkPVActorCompositeCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVActorCompositeNewCommand();
int vtkPVAnimationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVAnimationNewCommand();
int vtkPVApplicationCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVApplicationNewCommand();
int vtkPVAssignmentCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVAssignmentNewCommand();
int vtkPVCommandListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVCommandListNewCommand();
int vtkPVCutterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVCutterNewCommand();
int vtkPVDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataNewCommand();
int vtkPVDataSetToDataSetFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataSetToDataSetFilterNewCommand();
int vtkPVDataSetToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVDataSetToPolyDataFilterNewCommand();
int vtkPVExtentTranslatorCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVExtentTranslatorNewCommand();
int vtkPVGetRemoteGhostCellsCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVGetRemoteGhostCellsNewCommand();
int vtkPVGlyph3DCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVGlyph3DNewCommand();
int vtkPVImageDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageDataNewCommand();
int vtkPVImageClipCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageClipNewCommand();
int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageSliceNewCommand();
int vtkPVImageSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageSourceNewCommand();
int vtkPVImageToImageFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVImageToImageFilterNewCommand();
int vtkPVMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVMenuButtonNewCommand();
int vtkPVParallelDecimateCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVParallelDecimateNewCommand();
int vtkPVPolyDataCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataNewCommand();
int vtkPVPolyDataSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataSourceNewCommand();
int vtkPVPolyDataToPolyDataFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVPolyDataToPolyDataFilterNewCommand();
int vtkPVRenderViewCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRenderViewNewCommand();
int vtkPVRunTimeContourCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVRunTimeContourNewCommand();
int vtkPVScalarBarCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVScalarBarNewCommand();
int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSelectionListNewCommand();
int vtkPVSourceCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceNewCommand();
int vtkPVSourceCollectionCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceCollectionNewCommand();
int vtkPVSourceListCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVSourceListNewCommand();
int vtkPVWindowCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkPVWindowNewCommand();
int vtkRunTimeContourCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkRunTimeContourNewCommand();
int vtkSingleContourFilterCommand(ClientData cd, Tcl_Interp *interp,
             int argc, char *argv[]);
ClientData vtkSingleContourFilterNewCommand();

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
  vtkTclCreateNew(interp,"vtkColorByProcess", vtkColorByProcessNewCommand,
                  vtkColorByProcessCommand);
  vtkTclCreateNew(interp,"vtkDummyRenderWindowInteractor", vtkDummyRenderWindowInteractorNewCommand,
                  vtkDummyRenderWindowInteractorCommand);
  vtkTclCreateNew(interp,"vtkGetRemoteGhostCells", vtkGetRemoteGhostCellsNewCommand,
                  vtkGetRemoteGhostCellsCommand);
  vtkTclCreateNew(interp,"vtkImageOutlineFilter", vtkImageOutlineFilterNewCommand,
                  vtkImageOutlineFilterCommand);
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
  vtkTclCreateNew(interp,"vtkParallelDecimate", vtkParallelDecimateNewCommand,
                  vtkParallelDecimateCommand);
  vtkTclCreateNew(interp,"vtkPVActorComposite", vtkPVActorCompositeNewCommand,
                  vtkPVActorCompositeCommand);
  vtkTclCreateNew(interp,"vtkPVAnimation", vtkPVAnimationNewCommand,
                  vtkPVAnimationCommand);
  vtkTclCreateNew(interp,"vtkPVApplication", vtkPVApplicationNewCommand,
                  vtkPVApplicationCommand);
  vtkTclCreateNew(interp,"vtkPVAssignment", vtkPVAssignmentNewCommand,
                  vtkPVAssignmentCommand);
  vtkTclCreateNew(interp,"vtkPVCommandList", vtkPVCommandListNewCommand,
                  vtkPVCommandListCommand);
  vtkTclCreateNew(interp,"vtkPVCutter", vtkPVCutterNewCommand,
                  vtkPVCutterCommand);
  vtkTclCreateNew(interp,"vtkPVData", vtkPVDataNewCommand,
                  vtkPVDataCommand);
  vtkTclCreateNew(interp,"vtkPVDataSetToDataSetFilter", vtkPVDataSetToDataSetFilterNewCommand,
                  vtkPVDataSetToDataSetFilterCommand);
  vtkTclCreateNew(interp,"vtkPVDataSetToPolyDataFilter", vtkPVDataSetToPolyDataFilterNewCommand,
                  vtkPVDataSetToPolyDataFilterCommand);
  vtkTclCreateNew(interp,"vtkPVExtentTranslator", vtkPVExtentTranslatorNewCommand,
                  vtkPVExtentTranslatorCommand);
  vtkTclCreateNew(interp,"vtkPVGetRemoteGhostCells", vtkPVGetRemoteGhostCellsNewCommand,
                  vtkPVGetRemoteGhostCellsCommand);
  vtkTclCreateNew(interp,"vtkPVGlyph3D", vtkPVGlyph3DNewCommand,
                  vtkPVGlyph3DCommand);
  vtkTclCreateNew(interp,"vtkPVImageData", vtkPVImageDataNewCommand,
                  vtkPVImageDataCommand);
  vtkTclCreateNew(interp,"vtkPVImageClip", vtkPVImageClipNewCommand,
                  vtkPVImageClipCommand);
  vtkTclCreateNew(interp,"vtkPVImageSlice", vtkPVImageSliceNewCommand,
                  vtkPVImageSliceCommand);
  vtkTclCreateNew(interp,"vtkPVImageSource", vtkPVImageSourceNewCommand,
                  vtkPVImageSourceCommand);
  vtkTclCreateNew(interp,"vtkPVImageToImageFilter", vtkPVImageToImageFilterNewCommand,
                  vtkPVImageToImageFilterCommand);
  vtkTclCreateNew(interp,"vtkPVMenuButton", vtkPVMenuButtonNewCommand,
                  vtkPVMenuButtonCommand);
  vtkTclCreateNew(interp,"vtkPVParallelDecimate", vtkPVParallelDecimateNewCommand,
                  vtkPVParallelDecimateCommand);
  vtkTclCreateNew(interp,"vtkPVPolyData", vtkPVPolyDataNewCommand,
                  vtkPVPolyDataCommand);
  vtkTclCreateNew(interp,"vtkPVPolyDataSource", vtkPVPolyDataSourceNewCommand,
                  vtkPVPolyDataSourceCommand);
  vtkTclCreateNew(interp,"vtkPVPolyDataToPolyDataFilter", vtkPVPolyDataToPolyDataFilterNewCommand,
                  vtkPVPolyDataToPolyDataFilterCommand);
  vtkTclCreateNew(interp,"vtkPVRenderView", vtkPVRenderViewNewCommand,
                  vtkPVRenderViewCommand);
  vtkTclCreateNew(interp,"vtkPVRunTimeContour", vtkPVRunTimeContourNewCommand,
                  vtkPVRunTimeContourCommand);
  vtkTclCreateNew(interp,"vtkPVScalarBar", vtkPVScalarBarNewCommand,
                  vtkPVScalarBarCommand);
  vtkTclCreateNew(interp,"vtkPVSelectionList", vtkPVSelectionListNewCommand,
                  vtkPVSelectionListCommand);
  vtkTclCreateNew(interp,"vtkPVSource", vtkPVSourceNewCommand,
                  vtkPVSourceCommand);
  vtkTclCreateNew(interp,"vtkPVSourceCollection", vtkPVSourceCollectionNewCommand,
                  vtkPVSourceCollectionCommand);
  vtkTclCreateNew(interp,"vtkPVSourceList", vtkPVSourceListNewCommand,
                  vtkPVSourceListCommand);
  vtkTclCreateNew(interp,"vtkPVWindow", vtkPVWindowNewCommand,
                  vtkPVWindowCommand);
  vtkTclCreateNew(interp,"vtkRunTimeContour", vtkRunTimeContourNewCommand,
                  vtkRunTimeContourCommand);
  vtkTclCreateNew(interp,"vtkSingleContourFilter", vtkSingleContourFilterNewCommand,
                  vtkSingleContourFilterCommand);
  return TCL_OK;
}
