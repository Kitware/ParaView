proc vtkKWColorPresetSelectorEntryPoint {parent win} {

  set app [$parent GetApplication] 

  # -----------------------------------------------------------------------

  # Create the color transfer function that will be modified by
  # our choice of preset

  vtkColorTransferFunction cpsel_func

  # Create a color transfer function editor to show how the
  # color transfer function is affected by our choices

  vtkKWColorTransferFunctionEditor cpsel_tfunc_editor
  cpsel_tfunc_editor SetParent $parent
  cpsel_tfunc_editor Create $app
  cpsel_tfunc_editor SetBorderWidth 2
  cpsel_tfunc_editor SetReliefToGroove
  cpsel_tfunc_editor ExpandCanvasWidthOff
  cpsel_tfunc_editor SetCanvasWidth 200
  cpsel_tfunc_editor SetCanvasHeight 30
  cpsel_tfunc_editor SetPadX 2
  cpsel_tfunc_editor SetPadY 2
  cpsel_tfunc_editor ShowParameterRangeOff
  cpsel_tfunc_editor ShowParameterEntryOff
  cpsel_tfunc_editor ShowRangeLabelOff
  cpsel_tfunc_editor ShowColorSpaceOptionMenuOff
  cpsel_tfunc_editor ReadOnlyOn
  cpsel_tfunc_editor SetColorTransferFunction cpsel_func
  cpsel_tfunc_editor SetBalloonHelpString \
    "A color transfer function editor to demonstrate how the color\
    transfer function is affected by our choice of preset"

  # -----------------------------------------------------------------------

  # Create a color preset selector

  vtkKWColorPresetSelector cpsel1
  cpsel1 SetParent $parent
  cpsel1 Create $app
  cpsel1 SetColorTransferFunction cpsel_func
  cpsel1 SetPresetSelectedCommand cpsel_tfunc_editor "Update"
  cpsel1 SetBalloonHelpString \
    "A set of predefined color presets. Select one of them to apply the\
    preset to a color transfer function vtkColorTransferFunction"

  pack [cpsel1 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another color preset selector only the solid color

  vtkKWColorPresetSelector cpsel2
  cpsel2 SetParent $parent
  cpsel2 Create $app
  cpsel2 SetLabelText "Solid Color Presets:"
  cpsel2 HideGradientPresetsOn
  cpsel2 SetColorTransferFunction cpsel_func
  cpsel2 SetPresetSelectedCommand cpsel_tfunc_editor "Update"
  cpsel2 SetBalloonHelpString \
    "A set of predefined color presets. Let's hide the gradient presets."

  pack [cpsel2 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  # -----------------------------------------------------------------------

  # Create another color preset selector custom colors

  vtkKWColorPresetSelector cpsel3
  cpsel3 SetParent $parent
  cpsel3 Create $app
  cpsel3 SetLabelPositionToRight
  cpsel3 SetColorTransferFunction cpsel_func
  cpsel3 SetPresetSelectedCommand cpsel_tfunc_editor "Update"
  cpsel3 RemoveAllPresets
  cpsel3 SetPreviewSize [expr [cpsel3 GetPreviewSize]  * 2]
  cpsel3 SetLabelText "Custom Color Presets"
  cpsel3 AddSolidRGBPreset "Gray" 0.3 0.3 0.3
  cpsel3 AddSolidHSVPreset "Yellow" 0.15 1.0 1.0
  cpsel3 AddGradientRGBPreset \
    "Blue to Red" 0.0 0.0 1.0 1.0 0.0 0.0
  cpsel3 AddGradientHSVPreset \
    "Yellow to Magenta" 0.15 1.0 1.0 0.83 1.0 1.0
  cpsel3 SetBalloonHelpString \
    "A set of color presets. Just ours. Double the size of the preview and\
    put the label on the right."

  pack [cpsel3 GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 2

  pack [cpsel_tfunc_editor GetWidgetName] -side top -anchor nw -expand n -padx 2 -pady 20

  return "TypeVTK"
}

proc vtkKWColorPresetSelectorFinalizePoint {} {
  cpsel_tfunc_editor Delete
  cpsel1 Delete
  cpsel2 Delete
  cpsel3 Delete
  cpsel_func Delete
}

