import numpy as np
from scipy.interpolate import interp1d


class Renderer:
    """Cinema CIS Renderer

    The Renderer.renderer() renders a CIS image into the final composited and/or
    shadowed image. Currently, it support image composition through depth-buffer
    and applies "simulated" shadow using (pre-computed) shadow map. NaN values
    in images and depth buffers are considered as identification of "background"
    pixels and will be colored with imageview.background. Renderer() returns a
    tuple of final RGB image, represented as a 3D numpy array and depth buffer
    represented as a 2D numpy array.
    """

    def __init__(self):
        return

    # Paste buffer 'src' to buffer 'dest' at the 'offset' assuming dest is
    # large enough.
    @staticmethod
    def paste(dest, src, offset):
        ends = np.array(offset) + np.array(src.shape[0:1])
        dest[offset[0]:ends[0], offset[1]:ends[1], :] = src
        return dest

    # Color a scalar value buffer 'scalars' by the 'colormap'
    @staticmethod
    def color(scalars, colormap):
        cmap_fn, values = Renderer.make_rgb_colormap(colormap)
        # rescale scalars to be within the range of the colormap
        if np.nanmin(scalars) == np.nanmax(scalars):
            # prevent divide by zero when scalars are the same values.
            scalars = values.min()
        else:
            scalars = (scalars - np.nanmin(scalars)) / \
                      (np.nanmax(scalars) - np.nanmin(scalars))
            scalars = values.min() + scalars * (values.max() - values.min())
        return cmap_fn(scalars)

    @staticmethod
    def make_rgb_colormap(colormap):
        assert colormap['colorspace'] == 'rgb'
        points = colormap['points']
        values = np.zeros(len(points))
        rgbs = np.zeros((len(points), 3))
        for i in range(len(points)):
            values[i] = points[i]['x']
            rgbs[i, 0] = points[i]['r']
            rgbs[i, 1] = points[i]['g']
            rgbs[i, 2] = points[i]['b']
        cmap_fn = interp1d(values, rgbs, axis=0)
        return cmap_fn, values

    @staticmethod
    def blend(dest, src, mask):
        dest[mask] = src[mask]

    @staticmethod
    def depth_composite(dest_color, dest_z, src_color, src_z):
        mask = np.nan_to_num(dest_z, nan=np.inf) > \
               np.nan_to_num(src_z, nan=np.inf)
        Renderer.blend(dest_color, src_color, mask)
        Renderer.blend(dest_z, src_z, mask)

    @staticmethod
    def render(iview):

        # FXIME: this assumes RGB rather than RGBA color
        canvas = np.full((iview.dims[0], iview.dims[1], 3), iview.background,
                         float)
        depth = np.full((iview.dims[0], iview.dims[1]), np.inf, float)

        layers = iview.get_layer_data()
        for name, layer in layers.items():
            # TODO: fix this loop
            # The imageview object is intended to provide a way of iterating 
            # over active layers, but at present this has a bug in it. For
            # this release, we simply query to determine if the layer is active
            # and that is correct.
            if iview.is_active_layer(name):
                data = layer.channel.data
                background = np.full((data.shape[0], data.shape[1], 3),
                                     iview.background, float)
                foreground = Renderer.color(data, layer.channel.colormap)
                if iview.use_shadow:
                    foreground = foreground * \
                                 layer.shadow.data[:, :, np.newaxis]
                Renderer.blend(foreground, background, np.isnan(data))

                rectangle = (
                    slice(layer.offset[0], layer.offset[0] + data.shape[0]),
                    slice(layer.offset[1], layer.offset[1] + data.shape[1]))
                if iview.use_depth:
                    Renderer.depth_composite(canvas[rectangle],
                                             depth[rectangle],
                                             foreground, layer.depth.data)
                else:
                    canvas = Renderer.paste(canvas, foreground, layer.offset)

        # iview.dims is in row by column rather than x-dims by y-dims,
        # we need to transpose the two axes.
        return canvas.transpose((1, 0, 2)), depth.T
