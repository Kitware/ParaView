# TODO: Change this to take the array name and number of components. Register
# the lt under the name ncomp.array_name
def MakeBlueToRedLT(min, max):
    """
    Create a LookupTable that go from blue to red using the scalar range
    provided by the min and max arguments.

    :param min: Minimum range value.
    :type min: float
    :param max: Maximum range value.
    :type max: float
    :return: the blue-to-red lookup table
    :rtype: Lookup table proxy.
    """
    # Define RGB points. These are tuples of 4 values. First one is
    # the scalar values, the other 3 the RGB values.
    rgbPoints = [min, 0, 0, 1, max, 1, 0, 0]
    return CreateLookupTable(RGBPoints=rgbPoints, ColorSpace="HSV")
