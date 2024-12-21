"""
This example assumes a venv with the following dependencies:

   $ pip install trame trame-vuetify trame-vtk

This can be executed as follow:

   $ pvpython --venv .venv -m paraview.simple.example.trame

This is an adaptation of what was produced in [this blog][1] for [VTK 9.4][2]

   [1] https://www.kitware.com/vtk-9-4-a-step-closer-to-the-ways-of-python/
   [2] https://github.com/Kitware/trame/blob/master/examples/blogs/vtk-9.4/pipeline.py

"""

from paraview import simple

try:
    from trame.app import get_server
    from trame.ui.vuetify3 import VAppLayout
    from trame.widgets import html, paraview as pv_widgets, vuetify3 as v3
    from trame.decorators import TrameApp, change
except ImportError:
    import sys

    print(
        "\nThis example requires a virtual environment compatible with your ParaView Python"
        "\nwhich contains the trame dependencies."
        "\n"
        "\nTo setup such environment, you can do the following:"
        "\n"
        "\n  $ python3.x -m venv .venv"
        "\n  $ source .venv/bin/activate"
        "\n  $ pip install trame trame-vuetify trame-vtk"
        "\n"
        "\nThen run it like"
        "\n"
        "\n  $ pvpython --venv .venv -m paraview.simple.example.trame"
        "\n"
    )

    sys.exit(1)


def setup_pv():
    # Sources
    sphere = simple.Sphere(
        Radius=0.25,
        ThetaResolution=16,
        PhiResolution=16,
    )
    cone = simple.Cone(
        Radius=0.1,
        Height=0.2,
        Resolution=30,
        Capping=False,
        Direction=(-1, 0, 0),
    )

    # Pipeline
    with sphere:
        simple.SurfaceNormals(ComputeCellNormals=1)
        simple.CellCenters()
        simple.Glyph(
            GlyphType=cone,
            OrientationArray="Normals",
            GlyphMode="All Points",
        )
        noise = simple.PerlinNoise()
        noise.ImplicitFunction.Frequency = (10, 10, 10)

        # Last filter as pipeline
        pipeline = simple.GetActiveSource()

    # Rendering
    with pipeline:
        representation = simple.Show()
        representation.ColorBy(("POINTS", "PerlinNoise"))
        simple.Render()

    return representation.GetView(), sphere, cone, pipeline, noise.ImplicitFunction


# -----------------------------------------------------------------------------
# GUI helpers
# -----------------------------------------------------------------------------


class TitleWithStatistic(v3.VCardTitle):
    def __init__(self, name, title, width):
        super().__init__(title, classes="d-flex align-center")

        with self:
            v3.VSpacer()
            with v3.VChip(
                size="small",
                variant="outlined",
                style=f"width: {width}rem;",
            ):
                v3.VIcon("mdi-dots-triangle", start=True)
                html.Span(f"{{{{ {name}_points.toLocaleString() }}}}")
            with v3.VChip(
                size="small",
                variant="outlined",
                classes="ml-2",
                style=f"width: {width}rem;",
                v_show=f"{name}_points !== {name}_cells",
            ):
                v3.VIcon("mdi-triangle-outline", start=True)
                html.Span(f"{{{{ {name}_cells.toLocaleString() }}}}")


def slider(title, name, default_value, min_value, max_value, step_value):
    v3.VLabel(f"{ title }: {{{{ {name} }}}}")
    v3.VSlider(
        v_model=(name, default_value),
        min=min_value,
        step=step_value,
        max=max_value,
        hide_details=True,
    )


# -----------------------------------------------------------------------------
# Application
# -----------------------------------------------------------------------------


@TrameApp()
class Viewer:
    def __init__(self, server=None):
        self.view, self.sphere, self.cone, self.pipeline, self.field = setup_pv()
        self.server = get_server(server)
        self._build_ui()

    @property
    def state(self):
        return self.server.state

    @property
    def ctrl(self):
        return self.server.controller

    @change("cone_resolution", "cone_height", "cone_radius")
    def update_cone(self, cone_resolution, cone_height, cone_radius, **_):
        self.cone.Set(
            Resolution=cone_resolution,
            Height=cone_height,
            Radius=cone_radius,
        )

        # Execute filter for output extraction
        cone_info = self.cone.GetInformation()
        output_info = self.pipeline.GetInformation()

        # Update UI with new statistics
        self.state.update(
            {
                "cone_points": cone_info.number_of_points,
                "cone_cells": cone_info.number_of_cells,
                "total_points": output_info.number_of_points,
                "total_cells": output_info.number_of_cells,
            }
        )

        self.ctrl.view_update()

    @change("sphere_resolution")
    def update_sphere(self, sphere_resolution, **_):
        self.sphere.Set(
            ThetaResolution=sphere_resolution,
            PhiResolution=sphere_resolution,
        )

        # Execute filter for output extraction
        sphere_info = self.sphere.GetInformation()
        output_info = self.pipeline.GetInformation()

        # Update UI with new statistics
        self.state.update(
            {
                "sphere_points": sphere_info.number_of_points,
                "sphere_cells": sphere_info.number_of_cells,
                "total_points": output_info.number_of_points,
                "total_cells": output_info.number_of_cells,
            }
        )
        self.ctrl.view_update()

    @change("preset")
    def update_color_preset(self, preset, **_):
        simple.AssignFieldToColorPreset("PerlinNoise", preset)
        self.ctrl.view_update()

    @change("noise_frequency")
    def update_field(self, noise_frequency, **_):
        self.field.Frequency = (noise_frequency, noise_frequency, noise_frequency)
        self.ctrl.view_update()

    def _build_ui(self):
        with VAppLayout(self.server, fill_height=True) as self.ui:
            with v3.VCard(
                style="z-index: 1;",
                classes="position-absolute w-33 top-0 left-0 ma-4",
            ):
                # Sphere
                TitleWithStatistic("sphere", "Sphere", 4)
                v3.VDivider()
                with v3.VCardText():
                    slider("Resolution", "sphere_resolution", 16, 8, 32, 1)

                # Cone
                v3.VDivider()
                TitleWithStatistic("cone", "Cone", 3)
                v3.VDivider()
                with v3.VCardText():
                    slider("Resolution", "cone_resolution", 30, 3, 24, 1)
                    slider("Height", "cone_height", 0.2, 0.01, 0.5, 0.01)
                    slider("Radius", "cone_radius", 0.1, 0.01, 0.2, 0.01)

                # Colors
                v3.VDivider()
                v3.VCardTitle("Color", classes="d-flex align-center")
                v3.VDivider()
                with v3.VCardText():
                    v3.VSelect(
                        v_model=("preset", "Fast"),
                        items=("presets", simple.ListColorPresetNames()),
                        density="compact",
                        hide_details=True,
                        prepend_inner_icon="mdi-palette",
                        classes="mx-n4",
                        flat=True,
                        variant="solo",
                    )
                    slider("Frequency", "noise_frequency", 10, 1, 20, 0.25)

                # Result
                v3.VDivider()
                TitleWithStatistic("total", "Result", 5)

            with pv_widgets.VtkRemoteView(self.view, interactive_ratio=1) as view:
                self.ctrl.view_update = view.update
                self.ctrl.view_reset_camera = view.reset_camera


def main():
    app = Viewer()
    app.server.start()


if __name__ == "__main__":
    main()
