Proxy Documentation Formatting             {#ProxyDocumentationFormatting}
==============================

This page describes formatting options for proxy documentation in
different ParaView versions (since we started tracking these after
version 4.2).

Changes in 4.3
--------------

###Formatting options are added for proxy documentation###
Documentation for proxies (and for input properties for proxies)
accepts reStructured text (RST) formatting options. Supported options
are: bold, italic, unordered lists, and paragraphs. Nested unordered
lists are not supported. Note that the text enclosed between
**Documentation** tags has to be alligned at column 0, (space is
significant in RST documents) and that we do not accept empty lines
between items in an unordered list. Formatted output will be displayed
in ParaView online help, ParaView Python documentation, and tooltips
displayed in ParaView client. See the **Calculator** and **Glyph**
filters for examples on how to format other filters. Next we show the
calculator filter proxy documentation which includes formatting
options for bold, paragraphs and an unordered list.

    <Documentation>
    The Calculator filter computes a new data array or new point
    coordinates as a function of existing scalar or vector arrays. If
    point-centered arrays are used in the computation of a new data array,
    the resulting array will also be point-centered. Similarly,
    computations using cell-centered arrays will produce a new
    cell-centered array. If the function is computing point coordinates,
    the result of the function must be a three-component vector.

    The Calculator interface operates similarly to a scientific
    calculator. In creating the function to evaluate, the standard order
    of operations applies. Each of the calculator functions is described
    below. Unless otherwise noted, enclose the operand in parentheses
    using the ( and ) buttons.

    - Clear: Erase the current function (displayed in the read-only text
      box above the calculator buttons).
    - /: Divide one scalar by another. The operands for this function are
      not required to be enclosed in parentheses.
    - *: Multiply two scalars, or multiply a vector by a scalar (scalar multiple).
      The operands for this function are not required to be enclosed in parentheses.
    - -: Negate a scalar or vector (unary minus), or subtract one scalar or vector
      from another. The operands for this function are not required to be enclosed
      in parentheses.
    - +: Add two scalars or two vectors. The operands for this function are not
      required to be enclosed in parentheses.
    - sin: Compute the sine of a scalar. cos: Compute the cosine of a scalar.
    - tan: Compute the tangent of a scalar.
    - asin: Compute the arcsine of a scalar.
    - acos: Compute the arccosine of a scalar.
    - atan: Compute the arctangent of a scalar.
    - sinh: Compute the hyperbolic sine of a scalar.
    - cosh: Compute the hyperbolic cosine of a scalar.
    - tanh: Compute the hyperbolic tangent of a scalar.
    - min: Compute minimum of two scalars.
    - max: Compute maximum of two scalars.
    - x^y: Raise one scalar to the power of another scalar. The operands for
      this function are not required to be enclosed in parentheses.
    - sqrt: Compute the square root of a scalar.
    - e^x: Raise e to the power of a scalar.
    - log: Compute the logarithm of a scalar (deprecated. same as log10).
    - log10: Compute the logarithm of a scalar to the base 10.
    - ln: Compute the logarithm of a scalar to the base 'e'.
    - ceil: Compute the ceiling of a scalar. floor: Compute the floor of a scalar.
    - abs: Compute the absolute value of a scalar.
    - v1.v2: Compute the dot product of two vectors. The operands for this
      function are not required to be enclosed in parentheses.
    - cross: Compute cross product of two vectors.
    - mag: Compute the magnitude of a vector.
    - norm: Normalize a vector.

    The operands are described below. The digits 0 - 9 and the decimal
    point are used to enter constant scalar values. **iHat**, **jHat**,
    and **kHat** are vector constants representing unit vectors in the X,
    Y, and Z directions, respectively. The scalars menu lists the names of
    the scalar arrays and the components of the vector arrays of either
    the point-centered or cell-centered data.  The vectors menu lists the
    names of the point-centered or cell-centered vector arrays. The
    function will be computed for each point (or cell) using the scalar or
    vector value of the array at that point (or cell). The filter operates
    on any type of data set, but the input data set must have at least one
    scalar or vector array. The arrays can be either point-centered or
    cell-centered. The Calculator filter's output is of the same data set
    type as the input.
    </Documentation>
