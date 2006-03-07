/*=========================================================================

  Module:    vtkKWTkUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTkUtilities - class that supports basic Tk functions
// .SECTION Description
// vtkKWTkUtilities provides methods to perform common Tk operations.

#ifndef __vtkKWTkUtilities_h
#define __vtkKWTkUtilities_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.

#if defined(__hpux) && !defined(HAS_STDARG)
#define HAS_STDARG
#endif

#include <stdarg.h> // Needed for "va_list" argument of EstimateFormatLength.

class vtkKWWidget;
class vtkKWCoreWidget;
class vtkKWApplication;
class vtkKWIcon;
struct Tcl_Interp;

class KWWidgets_EXPORT vtkKWTkUtilities : public vtkObject
{
public:
  static vtkKWTkUtilities* New();
  vtkTypeRevisionMacro(vtkKWTkUtilities,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the Tcl name of a VTK object
  static const char* GetTclNameFromPointer(
    Tcl_Interp *interp, vtkObject *obj);
  static const char* GetTclNameFromPointer(
    vtkKWApplication *app, vtkObject *obj);
    
  // Description:
  // Evaluate a Tcl string. The string is passed to printf() first (as format
  // specifier) along with the remaining arguments.
  // The second prototype can be used by similar variable arguments method: it
  // needs to walk through the var_args list twice though. The only
  // portable way to do this is to pass two copies of the list's start
  // pointer.
  // Convenience methods are provided to specify a vtkKWApplication
  // instead of the Tcl interpreter.
  // Return a pointer to the Tcl interpreter result buffer.
  //BTX
  static const char* EvaluateString(
    Tcl_Interp *interp, const char *format, ...);
  static const char* EvaluateString(
    vtkKWApplication *app, const char *format, ...);
  //ETX
  static const char* EvaluateStringFromArgs(
    Tcl_Interp *interp, const char *format, 
    va_list var_args1, va_list var_args2);
  static const char* EvaluateStringFromArgs(
    vtkKWApplication *app, const char *format, 
    va_list var_args1, va_list var_args2);
  static const char* EvaluateSimpleString(
    Tcl_Interp *interp, const char *str);
  static const char* EvaluateSimpleString(
    vtkKWApplication *app, const char *str);

  // Description:
  // Convenience method that can be used to create a Tcl callback command.
  // The 'command' argument is a pointer to the command to be created.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // Note that 'command' is allocated automatically using the 'new' 
  // operator. If it is not NULL, it is deallocated first using 'delete []'.
  static void CreateObjectMethodCommand(
    vtkKWApplication *app, 
    char **command, vtkObject *object, const char *method);

  // Description:
  // Get the RGB components that correspond to 'color' (say, #223344)
  // in the widget given by 'widget' (say, .foo.bar). Color may be specified
  // in any of the forms acceptable for a Tk color option.
  // A convenience method is provided to query a vtkKWWidget directly.
  static void GetRGBColor(Tcl_Interp *interp,
                          const char *widget, 
                          const char *color, 
                          double *r, double *g, double *b);
  static void GetRGBColor(vtkKWWidget *widget, 
                          const char *color, 
                          double *r, double *g, double *b);

  // Description:
  // Get the RGB components that correspond to the color 'option'
  // (say -bg, -fg, etc.) of the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  static void GetOptionColor(Tcl_Interp *interp,
                             const char *widget, 
                             const char *option, 
                             double *r, double *g, double *b);
  static void GetOptionColor(vtkKWWidget *widget,
                             const char *option, 
                             double *r, double *g, double *b);
  static double* GetOptionColor(vtkKWWidget *widget,
                                const char *option);
  
  // Description:
  // Set the RGB components of the color 'option'
  // (say -bg, -fg, etc.) of the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  static void SetOptionColor(Tcl_Interp *interp,
                             const char *widget, 
                             const char *option, 
                             double r, double g, double b);
  static void SetOptionColor(vtkKWWidget *widget,
                             const char *option, 
                             double r, double g, double b);
  
  // Description:
  // Query user for color using a Tk color dialog
  // Convenience methods are provided to specify a vtkKWApplication
  // instead of the Tcl interpreter. 
  // Return 1 on success, 0 otherwise.
  static int QueryUserForColor(Tcl_Interp *interp,
                               const char *dialog_parent,
                               const char *dialog_title,
                               double in_r, double in_g, double in_b,
                               double *out_r, double *out_g, double *out_b);
  static int QueryUserForColor(vtkKWApplication *app,
                               const char *dialog_parent,
                               const char *dialog_title,
                               double in_r, double in_g, double in_b,
                               double *out_r, double *out_g, double *out_b);

  // Description:
  // Get the geometry of a widget given by 'widget' (say, .foo.bar).
  // The geometry is the width, height and position of the widget. 
  // Any of them can be a NULL pointer, they will be safely ignored.
  // Return 1 on success, 0 otherwise.
  // A convenience method is provided to query a vtkKWWidget directly.
  static int GetGeometry(Tcl_Interp *interp,
                         const char *widget, 
                         int *width, int *height, int *x, int *y);
  static int GetGeometry(vtkKWWidget *widget,
                         int *width, int *height, int *x, int *y);

  // Description:
  // Check if a pair of screen coordinates (x, y) are within the area defined
  // by the widget given by 'widget' (say, .foo.bar).
  // Return 1 if inside, 0 otherwise.
  // A convenience method is provided to query a vtkKWWidget directly.
  // ContainsCoordinatesForSpecificType will check if 'widget' is of a
  // specific type (or a subclass of that type), and if not will inspect
  // its children. It will return the widget that contains the coordinates
  // or NULL if not found.
  static int ContainsCoordinates(Tcl_Interp *interp,
                                 const char *widget, 
                                 int x, int y);
  static int ContainsCoordinates(vtkKWWidget *widget,
                                 int x, int y);
  static vtkKWWidget* ContainsCoordinatesForSpecificType(
    vtkKWWidget *widget, int x, int y, const char *classname);
  
  // Description:
  // Update a Tk photo given by its name 'photo_name' using pixels stored in
  // 'pixels' and structured as a 'width' x 'height' x 'pixel_size' (number
  // of bytes per pixel, 3 for RGB for example).
  // If 'buffer_length' is 0, compute it automatically by multiplying
  // 'pixel_size', 'width' and 'height' together.
  // If UPDATE_PHOTO_OPTION_FLIP_V is set in 'update_option', flip the image
  // buffer vertically.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  // Return 1 on success, 0 otherwise.
  //BTX
  enum 
  { 
    UpdatePhotoOptionFlipVertical = 1
  };
  //ETX
  static int UpdatePhoto(Tcl_Interp *interp,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         int update_options = 0);
  static int UpdatePhoto(vtkKWApplication *app,
                         const char *photo_name,
                         const unsigned char *pixels, 
                         int width, int height,
                         int pixel_size,
                         unsigned long buffer_length = 0,
                         int update_options = 0);

  // Description:
  // Update a Tk photo given by its name 'photo_name' using pixels stored in
  // the icon 'icon'. 
  static int UpdatePhotoFromIcon(vtkKWApplication *app,
                                 const char *photo_name,
                                 vtkKWIcon *icon,
                                 int update_options = 0);
  static int UpdatePhotoFromPredefinedIcon(vtkKWApplication *app,
                                           const char *photo_name,
                                           int icon_index,
                                           int update_options = 0);

  // Description:
  // Update a Tk photo given by its name 'photo_name' using pixels stored in
  // 'pixels' and structured as a 'width' x 'height' x 'pixel_size' (number
  // of bytes per pixel, 3 for RGB for example).
  // If a file 'file_name'.png is found in 'directory' or 
  // 'directory/Resources' then an attempt is made to update the photo using
  // this file. If no file is found, the remaining parameters are used
  // to update the photo by calling UpdatePhoto().
  // As a convenience, if 'photo_name' is NULL, 'file_name' is used instead.
  // Note that only the PNG file format is supported so far (do not provide
  // the .png extension to 'file_name').
  // Return 1 on success, 0 otherwise.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int UpdateOrLoadPhoto(Tcl_Interp *interp,
                               const char *photo_name,
                               const char *file_name,
                               const char *directory,
                               const unsigned char *pixels, 
                               int width, int height,
                               int pixel_size,
                               unsigned long buffer_length = 0);
  static int UpdateOrLoadPhoto(vtkKWApplication *app,
                               const char *photo_name,
                               const char *file_name,
                               const char *directory,
                               const unsigned char *pixels, 
                               int width, int height,
                               int pixel_size,
                               unsigned long buffer_length = 0);

  // Description:
  // Specifies an image to display in a widget. Typically, if the image
  // is specified then it overrides other options that specify a bitmap or
  // textual value to display in the widget.
  // Set the image option using pixel data. The parameters are the same
  // as the one used in UpdatePhoto().
  // An image is created and associated to the Tk -image option or 
  // image_option if not NULL (ex: -selectimage).
  static void SetImageOptionToPixels(
    vtkKWCoreWidget *widget,
    const unsigned char *pixels, 
    int width, int height, 
    int pixel_size = 4,
    unsigned long buffer_length = 0,
    const char *image_option = 0);

  // Description:
  // Query if a Tk photo given by its name 'photo_name' exists.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int FindPhoto(Tcl_Interp *interp, const char *photo_name);
  static int FindPhoto(vtkKWApplication *app, const char *photo_name);

  // Description:
  // Get the height of a Tk photo given by its name 'photo_name'.
  // If the photo does not exist, return 0 and issue a warning.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  // A convenience method is provided to specify a vtkKWWidget this photo
  // has been assigned to using the -image Tk option.
  static int GetPhotoHeight(Tcl_Interp *interp, const char *photo_name);
  static int GetPhotoHeight(vtkKWApplication *app, const char *photo_name);
  static int GetPhotoHeight(vtkKWWidget *widget);

  // Description:
  // Get the width of a Tk photo given by its name 'photo_name'.
  // If the photo does not exist, return 0 and issue a warning.
  // A convenience method is provided to specify the vtkKWApplication this
  // photo belongs to, instead of the Tcl interpreter.
  static int GetPhotoWidth(Tcl_Interp *interp, const char *photo_name);
  static int GetPhotoWidth(vtkKWApplication *app, const char *photo_name);

  // Description:
  // Change the weight attribute of a Tk font specification given by 'font'.
  // The new font specification is copied to 'new_font'. 
  // It is up to the caller to allocate enough space in 'new_font'.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontWeightToBold(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontWeightToNormal(
    Tcl_Interp *interp, const char *font, char *new_font);

  // Description:
  // Change the weight attribute of a 'widget' -font option.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontWeightToBold(Tcl_Interp *interp, const char *widget);
  static int ChangeFontWeightToBold(vtkKWWidget *widget);
  static int ChangeFontWeightToNormal(Tcl_Interp *interp, const char *widget);
  static int ChangeFontWeightToNormal(vtkKWWidget *widget);

  // Description:
  // Change the slant attribute of a Tk font specification given by 'font'.
  // The new font specification is copied to 'new_font'. 
  // It is up to the caller to allocate enough space in 'new_font'.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontSlantToItalic(
    Tcl_Interp *interp, const char *font, char *new_font);
  static int ChangeFontSlantToRoman(
    Tcl_Interp *interp, const char *font, char *new_font);

  // Description:
  // Change the slant attribute of a 'widget' -font option.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int ChangeFontSlantToItalic(Tcl_Interp *interp, const char *widget);
  static int ChangeFontSlantToItalic(vtkKWWidget *widget);
  static int ChangeFontSlantToRoman(Tcl_Interp *interp, const char *widget);
  static int ChangeFontSlantToRoman(vtkKWWidget *widget);

  // Description:
  // Get the number of colums and rows defined in the grid layout of
  // the widget given by 'widget' (say, .foo.bar).
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetGridSize(Tcl_Interp *interp,
                         const char *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);
  static int GetGridSize(vtkKWWidget *widget,
                         int *nb_of_cols,
                         int *nb_of_rows);

  // Description:
  // Get the grid position (column, row) of the widget given by 'widget'
  // (say, .foo.bar).
  // We assume that the current widget layout is a Tk grid.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetPositionInGrid(Tcl_Interp *interp,
                                     const char *widget,
                                     int *col,
                                     int *row);
  static int GetWidgetPositionInGrid(vtkKWWidget *widget,
                                     int *col,
                                     int *row);

  // Description:
  // Get the bounding box size (width, height) of the slaves packed in the
  // widget given by 'widget' (say, .foo.bar), i.e. the largest width
  // and height of the slaves packed in the widget, including padding options.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetSlavesBoundingBoxInPack(Tcl_Interp *interp,
                                        const char *widget,
                                        int *width,
                                        int *height);
  static int GetSlavesBoundingBoxInPack(vtkKWWidget *widget,
                                        int *width,
                                        int *height);

  // Description:
  // Get the horizontal position 'x' in pixels of a slave widget given by
  // 'slave' (say .foo.bar.sl) in the widget given by 'widget' (say .foo.bar).
  // This can be used in case 'winfo x' does not work because the widget
  // has not been mapped yet.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query vtkKWWidget(s) directly.
  // Return 1 on success, 0 otherwise.
  static int GetSlaveHorizontalPositionInPack(Tcl_Interp *interp,
                                              const char *widget,
                                              const char *slave,
                                              int *x);
  static int GetSlaveHorizontalPositionInPack(vtkKWWidget *widget,
                                              vtkKWWidget *slave,
                                              int *x);

  // Description:
  // Get the padding values of the widget given by 'widget' (say .foo.bar)
  // in its layout.
  // We assume that the current widget layout is a Tk pack.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetPaddingInPack(Tcl_Interp *interp,
                                    const char *widget,
                                    int *ipadx,
                                    int *ipady,
                                    int *padx,
                                    int *pady);

  // Description:
  // Get the container a widget given by 'widget' (say .foo.bar) is packed in.
  // This is similar to the Tk -in pack option.
  // Write the container widget name to the output stream 'in'.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int GetMasterInPack(Tcl_Interp *interp,
                             const char *widget,
                             ostream &in);
  static int GetMasterInPack(vtkKWWidget *widget,
                             ostream &in);

  // Description:
  // Get the column widths of a grid (i.e. a master widget that has been grid).
  // If 'allocate' is true, the resulting array (col_widths) is allocated
  // by the function to match the number of columns.
  // The function iterates over cells to request the width of
  // each slave (winfo reqwidth).
  static int GetGridColumnWidths(Tcl_Interp *interp,
                                 const char *widget,
                                 int *nb_of_cols,
                                 int **col_widths,
                                 int allocate = 0);

  // Description:
  // Synchronize the columns minimum size of different widgets that have
  // been grid. If 'factors' is non-null, it is used as an array of
  // multiplication factor to apply to each column minimum size.
  // If 'weights' is non-null, it is used as an array of weight
  // to apply to each column through columnconfigure -weight.
  static int SynchroniseGridsColumnMinimumSize(Tcl_Interp *interp,
                                               int nb_of_widgets,
                                               const char **widgets,
                                               const float *factors = 0,
                                               const int *weights = 0);

  // Description:
  // Synchronize the width of a set of labels given by an array
  // of 'nb_of_widgets' widgets stored in 'widgets'. The maximum size of
  // the labels is found and assigned to each label. 
  // Additionally it will apply the 'options' to/ each widget (if any).
  // A convenience method is provided to specify the vtkKWApplication these
  // widgets belongs to, instead of the Tcl interpreter.
  // Return 1 on success, 0 otherwise.
  static int SynchroniseLabelsMaximumWidth(Tcl_Interp *interp,
                                           int nb_of_widgets,
                                           const char **widgets,
                                           const char *options = 0);
  static int SynchroniseLabelsMaximumWidth(vtkKWApplication *app,
                                           int nb_of_widgets,
                                           const char **widgets,
                                           const char *options = 0);

  // Description:
  // Store the slaves packed in the widget given by 'widget' (say, .foo.bar)
  // in the array 'slaves'. This array is  allocated automatically.
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return the number of slaves.
  static int GetSlavesInPack(Tcl_Interp *interp,
                             const char *widget,
                             char ***slaves);
  static int GetSlavesInPack(vtkKWWidget *widget,
                             char ***slaves);

  // Description:
  // Browse all the slaves of the widget given by 'widget' (say, .foo.bar)
  // and store the slave packed before 'slave' in 'previous_slave', and the
  // slave packed after 'slave' in 'next_slave'
  // We assume that the current widget layout is a Tk pack.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 if 'slave' was found, 0 otherwise
  static int GetPreviousAndNextSlaveInPack(Tcl_Interp *interp,
                                           const char *widget,
                                           const char *slave,
                                           ostream &previous_slave,
                                           ostream &next_slave);
  static int GetPreviousAndNextSlaveInPack(vtkKWWidget *widget,
                                           vtkKWWidget *slave,
                                           ostream &previous_slave,
                                           ostream &next_slave);
  // Description:
  // Take screendump of the widget given by 'widget' (say, .foo.bar) and store
  // it into a png file given by 'fname'.
  // A convenience method is provided to query a vtkKWWidget directly.
  // Return 1 on success, 0 otherwise.
  static int TakeScreenDump(Tcl_Interp *interp,
                            const char *wname, 
                            const char *fname, 
                            int top = 0, int bottom = 0, 
                            int left = 0, int right = 0);
  static int TakeScreenDump(vtkKWWidget *widget,
                            const char *fname, 
                            int top = 0, int bottom = 0, 
                            int left = 0, int right = 0);

  // Description:
  // Set widget's toplevel mouse cursor.
  // Provide a NULL or empty cursor to reset it to default.
  static int SetTopLevelMouseCursor(Tcl_Interp *interp,
                                    const char *widget,
                                    const char *cursor);
  static int SetTopLevelMouseCursor(vtkKWWidget *widget,
                                    const char *cursor);

  // Description:
  // Return 1 if window is a toplevel, 0 otherwise
  static int IsTopLevel(Tcl_Interp *interp,
                        const char *widget);
  static int IsTopLevel(vtkKWWidget *widget);

  // Description:
  // Withdraw toplevel
  static void WithdrawTopLevel(Tcl_Interp *interp,
                              const char *widget);
  static void WithdrawTopLevel(vtkKWWidget *widget);

  // Description:
  // If a Tcl script file is currently being evaluated (i.e. there is a call
  // to Tcl_EvalFile active or there is an active invocation of the source 
  // command), then this command returns the name of the innermost file
  // being processed.
  static const char *GetCurrentScript(Tcl_Interp *interp);
  static const char *GetCurrentScript(vtkKWApplication *app);

  // Description:
  // Create a timer handler, i.e. arranges for a command to be executed
  // exactly once 'ms' milliseconds later, or when the application is idle,
  // i.e. the next time the event loop is entered and there are no events to
  // process.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // Returns a string identifier that can be used to cancel the timer.
  static const char* CreateTimerHandler(
    vtkKWApplication *app, unsigned long ms, 
    vtkObject *object, const char *method);
  static const char* CreateIdleTimerHandler(
    vtkKWApplication *app, vtkObject *object, const char *method);

  // Description:
  // Cancel one or all event handlers, i.e. cancel all delayed command that
  // were registered using the 'after' command or CreateTimerHandler methods.
  static void CancelTimerHandler(Tcl_Interp *interp, const char *id);
  static void CancelTimerHandler(vtkKWApplication *app, const char *id);
  static void CancelAllTimerHandlers(Tcl_Interp *interp);
  static void CancelAllTimerHandlers(vtkKWApplication *app);

  // Description:
  // Rings the bell on the display of the application's main window
  static void Bell(Tcl_Interp *interp);
  static void Bell(vtkKWApplication *app);

  // Description:
  // Process/update pending events. This command is used to bring the 
  // application "up to date" by entering the event loop repeatedly until
  // all pending events (including idle callbacks) have been processed. 
  static void ProcessPendingEvents(Tcl_Interp *interp);
  static void ProcessPendingEvents(vtkKWApplication *app);

  // Description:
  // Process/update idle tasks. This causes operations that are normally 
  // deferred, such as display updates and window layout calculations, to be
  // performed immediately. 
  static void ProcessIdleTasks(Tcl_Interp *interp);
  static void ProcessIdleTasks(vtkKWApplication *app);

  // Description:
  // Get the coordinates of the mouse pointer in the screen widget is in.
  // Return 1 on success, 0 otherwise.
  static int GetMousePointerCoordinates(
    Tcl_Interp *interp, const char *widget, int *x, int *y);
  static int GetMousePointerCoordinates(
    vtkKWWidget *widget, int *x, int *y);

  // Description:
  // Get the coordinates of the upper-left corner of widget in its screen.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetCoordinates(
    Tcl_Interp *interp, const char *widget, int *x, int *y);
  static int GetWidgetCoordinates(
    vtkKWWidget *widget, int *x, int *y);

  // Description:
  // Get the relative coordinates of the upper-left corner of widget in its
  // widget's parent.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetRelativeCoordinates(
    Tcl_Interp *interp, const char *widget, int *x, int *y);
  static int GetWidgetRelativeCoordinates(
    vtkKWWidget *widget, int *x, int *y);

  // Description:
  // Get the width and height of widget in its screen.
  // When a window is first created its width will be 1 pixel; the width will
  // eventually be changed by a geometry manager to fulfill the window's needs.
  // If you need the true width immediately after creating a widget, invoke
  // ProcessPendingEvents to force the geometry manager to arrange it, or use
  // GetWidgetRequestedSize to get the window's requested size instead of its
  // actual size. 
  // Return 1 on success, 0 otherwise.
  static int GetWidgetSize(
    Tcl_Interp *interp, const char *widget, int *w, int *h);
  static int GetWidgetSize(
    vtkKWWidget *widget, int *w, int *h);

  // Description:
  // Get the requested width and height of widget in its screen.
  // This is the value used by window's geometry manager to compute its
  // geometry.
  // Return 1 on success, 0 otherwise.
  static int GetWidgetRequestedSize(
    Tcl_Interp *interp, const char *widget, int *w, int *h);
  static int GetWidgetRequestedSize(
    vtkKWWidget *widget, int *w, int *h);

  // Description:
  // Get the widget class (i.e. Tk type).
  static const char* GetWidgetClass(
    Tcl_Interp *interp, const char *widget);
  static const char* GetWidgetClass(
    vtkKWWidget *widget);

  // Description:
  // Get the width and height (in pixels) of the screen the widget is in.
  // Return 1 on success, 0 otherwise.
  static int GetScreenSize(
    Tcl_Interp *interp, const char *widget, int *w, int *h);
  static int GetScreenSize(
    vtkKWWidget *widget, int *w, int *h);

  // Description:
  // Get windowing system.
  // Returns the current Tk windowing system, one of x11 (X11-based), 
  // win32 (MS Windows), classic (Mac OS Classic), or aqua (Mac OS X Aqua). 
  static const char* GetWindowingSystem(vtkKWApplication *app);
  static const char* GetWindowingSystem(Tcl_Interp *interp);

protected:
  vtkKWTkUtilities() {};
  ~vtkKWTkUtilities() {};

  //BTX  
  //ETX

  static int ChangeFontWeight(Tcl_Interp *interp, const char *widget, int);
  static int ChangeFontWeight(Tcl_Interp *interp, 
                              const char *font, char *new_font, int);
  static int ChangeFontSlant(Tcl_Interp *interp, const char *widget, int);
  static int ChangeFontSlant(Tcl_Interp *interp, 
                             const char *font, char *new_font, int);

  static const char* EvaluateStringFromArgsInternal(
    Tcl_Interp *interp, vtkObject *obj, const char *format, 
    va_list var_args1, va_list var_args2);
  static const char* EvaluateSimpleStringInternal(
    Tcl_Interp *interp, vtkObject *obj, const char *str);

private:
  vtkKWTkUtilities(const vtkKWTkUtilities&); // Not implemented
  void operator=(const vtkKWTkUtilities&); // Not implemented
};

#endif

