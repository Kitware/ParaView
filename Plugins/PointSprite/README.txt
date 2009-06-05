###########################################################################
#                                                                         #
#                        Point Sprite Plugin                              #
#                                                                         #
###########################################################################

The point sprite plugin enables a new presentation type : PointSprite

This presentation extract all points from the input, and draw them as point sprites.

#####    Display Panel    #####

The "RenderMode" parameter modify the way point sprites are drawn : 
  "SimplePoint" draws antialiased points
  "Texture" draws a point sprite and apply a texture to it.
  "Sphere" draws a sphere by raytracing it.
  
Two preset textures can be choosen from :
  Sphere is a splat the looks like a sphere
  Blur is a texture with alpha component (gaussian shape)
The user can load any texture in. 

The "MaxPixelSize" parameter is here as a security : trying to draw thousand of point sprites filling the whole screen usually crashes the session.

The "RadiusMode" can be :
  ConstantRadius : you can set the constant radius in object space
  or you can choose any point-centered array to use as radius parameter. Edit the radius transfer function as you like.
  
The "OpacityMode" can be :
  ConstantOpacity : in this case, the global opacity parameter is used.
  Or you can choose any point-centerd array to use a opacity parameter. Edit the opacity transfer function as you like. The result will be multiplied by the global opacity.
   
#####   Tranfer Function Editor    #####

The transfer function editors can be used to modify how the choosen array is mapped to the parameter (either radius or opacity)

The "UseScalarRange" parameter forces the scalar range to the range of the active scalar.
The "Proportional" parameter force the radius to be proportional to the scalar. The proportional factor specifies the ratio.
Several preset transfer functions are proposed : Zero, linear ramp, one or inverse linear ramp.
With the freeform editor, the tranfer function can be edited to any form.
With the gaussian transfer function editor, the transfer function is the max of a set of gaussian.

#####    Known Bugs     #####

Translucency : the "Sphere" mode do not support depth peeling. It is also much faster to use sorting with the point sprites instead of using depth peeling on hundred of layers. It is then strongly recommended to disable depth peeling when using point sprites.
  
Lighting : the points sprites do not work with the lighting kit. you should use a head light.

Textures : this plugins automatically preload two textures when any presentaion is created. It does not reload any texture if it detects that they are already loaded. 
But when reloading a state file (.pvsm), the textures seems to be loaded after the representations, and ParaView does not try to remove duplicate textures, a texture duplicate is then created.

#####    Workarounds     #####   

Depth Sorting : 
  when using a texture with alpha component that is always fully transparent or fully opaque (0 or 255), the vtkDepthSortPainter understand that it does not actually need to sort particles. This code should go in the vtkActor::HasTranslucentGeometry.
  teh vtkDepthSortPainter also look at the alpha component in the color array (the result is cached so that it is not recomputed at each frame if nothing changed), beacaus an alpha component might have been created int he vtkTwoScalarsToColorsPainter if the opacity editor is used.

Opacity transfer function : when the user activates the opacity transfer function, if the global opacity is 1, it is set the 1 - 0.0001 so that the actor knows that it will be translucent. The opacity is restored when disabling the opacity transfer function.

#####     VTK Patches    #####

vtkUniformVariables : added a "Merge" method to add variables coming from  another vtkUniformVariables to the current set of uniforms.
vtkOpenGLProperty : rewrite the "Render" method so that the ShaderProgram is the result of the merge of the renderer's shader program and the prop's shader program. The prop shader program is only used if the "Shading" parameter is on. The uniform variables coming from the two shader programs are merged.
  

    
