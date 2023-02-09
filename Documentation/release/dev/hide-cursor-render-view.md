##  Allow to hide cursor in render views

You can now hide the cursor in render views through pqRenderView API.
You can also use the `HideCursor` dedicated XML hint.

How to use it :

    <RenderViewProxy ...>
      ...
      <Hints>
        <HideCursor/>
      </Hints>
    </RenderViewProxy>
