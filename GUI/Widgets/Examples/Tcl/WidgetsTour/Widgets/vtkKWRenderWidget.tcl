package require vtkio
package require vtkrendering

proc vtkKWRenderWidgetEntryPoint {parent win} {

    set app [$parent GetApplication]

    # -----------------------------------------------------------------------

    # Create a render widget

    vtkKWRenderWidget rw
    rw SetParent $parent
    rw Create $app

    pack [rw GetWidgetName] -side top -expand y -fill both -padx 0 -pady 0

    # -----------------------------------------------------------------------

    # Switch to trackball style, it's nicer

    [[[rw GetRenderWindow] GetInteractor] GetInteractorStyle] SetCurrentStyleToTrackballCamera

    # Create a 3D object reader

    vtkXMLPolyDataReader reader
    reader SetFileName [file join [file dirname [info script]] ".." ".." Data teapot.vtp]

    # Create the mapper and actor

    vtkPolyDataMapper mapper
    mapper SetInputConnection [reader GetOutputPort]

    vtkActor actor
    actor SetMapper mapper

    # Add the actor to the scene

    rw AddProp actor
    rw ResetCamera

    return 3
}

proc vtkKWRenderWidgetFinalizePoint {} {
    rw Delete
    reader Delete
    mapper Delete
    actor Delete
}