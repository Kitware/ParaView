import ipywidgets as widgets
from cinemasci.cis.renderer import Renderer

class ParamSet():

    def __init__(self, params):
        self.params = params

    def update(self, params):
        for p in params:
            self.params[p] = params[p]

class ParamSlider():

    def __init__(self, params): 
        self.label  = widgets.Label(params["name"], style={'description_width': 'initial'})
        self.slider = widgets.IntSlider()
        self.HBox   = widgets.HBox([self.label, self.slider])
        display(self.HBox)

class ParamSliders():
    def __init__(self, params):
        self.sliders = {}

        # don't do this yet
        # for p in params: 
        #     self.sliders[p] = ParamSlider({"name": p})

class CISHistogramViewer():

    def __init__(self, cisview):
        self.aspect      = "equal"
        self.size        = (10, 10)
        self.cisview     = cisview
        self.sliders     = ParamSliders(cisview.get_cdb_parameters())
        self.title       = ""
        self.left_label  = ""
        self.right_label = ""
        self.figsize     = (1, 1)
        self.widths      = [1, 1]
        self.height      = [1]
        self.num_bins    = 50
        return

    @property
    def num_bins(self):
        return self._num_bins

    @num_bins.setter
    def num_bins(self, value):
        self._num_bins = value
    
    @property
    def height(self):
        return self._height

    @height.setter
    def height(self, value):
        self._height = value

    @property
    def widths(self):
        return self._widths

    @widths.setter
    def widths(self, value):
        self._widths = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value

    @property
    def right_label(self):
        return self._right_label

    @right_label.setter
    def right_label(self, value):
        self._right_label = value

    @property
    def left_label(self):
        return self._left_label

    @left_label.setter
    def left_label(self, value):
        self._left_label = value

    @property
    def title(self):
        return self._title

    @title.setter
    def title(self, value):
        self._title = value

    @property
    def aspect(self):
        return self._aspect

    @aspect.setter
    def aspect(self, value):
        self._aspect = value

    @property
    def size(self):
        return self._size

    @size.setter
    def size(self, value):
        self._size = value

    def display(self, iview, alayer):
        import numpy
        from matplotlib import pyplot as plt

        # update the input data
        iview.update()

        layers = iview.get_layer_data()
        cdata = layers[alayer].channel.data

        fig = plt.figure(figsize=self.size, constrained_layout=True)
        spec = fig.add_gridspec(ncols=2, nrows=1, width_ratios=self.widths, height_ratios=self.height)
        fig.suptitle(self.title)

        # render the image view
        (image, depth) = Renderer.render(iview)

        # display the rendered image
        import warnings
        ax = fig.add_subplot(spec[0,0])
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            ax.axis('off')
            ax.set_title(self.left_label)
            ax.imshow(image, aspect='equal')

        ax = fig.add_subplot(spec[0, 1])
        ax.hist(cdata[~numpy.isnan(cdata)], self.num_bins)
        ax.set_title(self.right_label)



