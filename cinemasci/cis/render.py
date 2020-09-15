from PIL import Image, ImageDraw

class render():

    def __init__(self):
        return

    def render(self, cis, iname, lnames, cnames):
        image = cis.get_image(iname)
        imode = "RGB"

        im = Image.new(mode=imode, size=(cis.dims[0], cis.dims[1]))

        # for now, assume only variables (no shadow, depth, etc.)
        for layer in lnames: 
            l = image.get_layer(layer)
            if l:
                shape = [(0,0),(l.dims[0], l.dims[1])]
                limage = Image.new(mode=imode, size=(l.dims[0], l.dims[1]))
                lidraw = ImageDraw.Draw(limage)
                lidraw.rectangle(shape, fill="#ffff33", outline="red")
                im.paste(limage, l.offset)

        return im

