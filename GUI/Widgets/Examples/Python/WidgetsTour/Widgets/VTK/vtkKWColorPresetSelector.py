from kwwidgets import vtkKWColorPresetSelector
from kwwidgets import vtkKWColorTransferFunctionEditor
from vtk import vtkColorTransferFunction
from kwwidgets import vtkKWApplication
from kwwidgets import vtkKWWindow



def vtkKWColorPresetSelectorEntryPoint(parent, win):

    app = parent.GetApplication()
    
    # -----------------------------------------------------------------------
    
    # Create the color transfer function that will be modified by
    # our choice of preset
    
    cpsel_func = vtkColorTransferFunction()
    
    # Create a color transfer function editor to show how the
    # color transfer function is affected by our choices
    
    cpsel_tfunc_editor = vtkKWColorTransferFunctionEditor()
    cpsel_tfunc_editor.SetParent(parent)
    cpsel_tfunc_editor.Create()
    cpsel_tfunc_editor.SetBorderWidth(2)
    cpsel_tfunc_editor.SetReliefToGroove()
    cpsel_tfunc_editor.ExpandCanvasWidthOff()
    cpsel_tfunc_editor.SetCanvasWidth(200)
    cpsel_tfunc_editor.SetCanvasHeight(30)
    cpsel_tfunc_editor.SetPadX(2)
    cpsel_tfunc_editor.SetPadY(2)
    cpsel_tfunc_editor.ParameterRangeVisibilityOff()
    cpsel_tfunc_editor.ParameterEntryVisibilityOff()
    cpsel_tfunc_editor.ParameterRangeLabelVisibilityOff()
    cpsel_tfunc_editor.ColorSpaceOptionMenuVisibilityOff()
    cpsel_tfunc_editor.ReadOnlyOn()
    cpsel_tfunc_editor.SetColorTransferFunction(cpsel_func)
    cpsel_tfunc_editor.SetBalloonHelpString(
        "A color transfer function editor to demonstrate how the color "
        "transfer function is affected by our choice of preset")
    
    # -----------------------------------------------------------------------
    
    app.Script(
        "proc update_editor {name} {%s Update}", cpsel_tfunc_editor.GetTclName())
    
    # Create a color preset selector
    
    cpsel1 = vtkKWColorPresetSelector()
    cpsel1.SetParent(parent)
    cpsel1.Create()
    cpsel1.SetColorTransferFunction(cpsel_func)
    cpsel1.SetPresetSelectedCommand(None, "update_editor")
    cpsel1.SetBalloonHelpString(
        "A set of predefined color presets. Select one of them to apply the "
        "preset to a color transfer function (vtkColorTransferFunction)")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        cpsel1.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another color preset selector, only the solid color
    
    cpsel2 = vtkKWColorPresetSelector()
    cpsel2.SetParent(parent)
    cpsel2.Create()
    cpsel2.SetLabelText("Solid Color Presets:")
    cpsel2.GradientPresetsVisibilityOff()
    cpsel2.SetColorTransferFunction(cpsel_func)
    cpsel2.SetPresetSelectedCommand(None, "update_editor")
    cpsel2.SetBalloonHelpString(
        "A set of predefined color presets. Let's hide the gradient presets.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        cpsel2.GetWidgetName())
    
    # -----------------------------------------------------------------------
    
    # Create another color preset selector, custom colors
    
    cpsel3 = vtkKWColorPresetSelector()
    cpsel3.SetParent(parent)
    cpsel3.Create()
    cpsel3.SetLabelPositionToRight()
    cpsel3.SetColorTransferFunction(cpsel_func)
    cpsel3.SetPresetSelectedCommand(None, "update_editor")
    cpsel3.RemoveAllPresets()
    cpsel3.SetPreviewSize(cpsel3.GetPreviewSize() * 2)
    cpsel3.SetLabelText("Custom Color Presets")
    cpsel3.AddSolidRGBPreset("Gray", 0.3, 0.3, 0.3)
    cpsel3.AddSolidHSVPreset("Yellow", 0.15, 1.0, 1.0)
    cpsel3.AddGradientRGBPreset(
        "Blue to Red", 0.0, 0.0, 1.0, 1.0, 0.0, 0.0)
    cpsel3.AddGradientHSVPreset(
        "Yellow to Magenta", 0.15, 1.0, 1.0, 0.83, 1.0, 1.0)
    cpsel3.SetBalloonHelpString(
        "A set of color presets. Just ours. Double the size of the preview and "
        "put the label on the right.")
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 2",
        cpsel3.GetWidgetName())
    
    app.Script(
        "pack %s -side top -anchor nw -expand n -padx 2 -pady 20",
        cpsel_tfunc_editor.GetWidgetName())
    
    
    
    return "TypeVTK"
