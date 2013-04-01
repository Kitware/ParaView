/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend the ParaView viewport to add support for WebGL rendering.
 *
 * @class paraview.viewport.webgl
 */
(function (GLOBAL, $) {
    var module = {},
    DEFAULT_OPTIONS = {},
    FACTORY_KEY = 'webgl',
    FACTORY = {
        'builder': createGeometryDeliveryRenderer,
        'options': DEFAULT_OPTIONS
    };

    // ----------------------------------------------------------------------
    // Geometry Delivery renderer - factory method
    // ----------------------------------------------------------------------

    function createGeometryDeliveryRenderer(domElement) {
        console.log('createGeometryDeliveryRenderer');
        console.log('WebGL renderer created...');
    }

    // ----------------------------------------------------------------------
    // Init paraview module if needed
    // ----------------------------------------------------------------------
    if (GLOBAL.hasOwnProperty("paraview")) {
        module = GLOBAL.paraview || {};
    } else {
        GLOBAL.paraview = module;
    }

    // ----------------------------------------------------------------------
    // Extend the viewport factory - ONLY IF WEBGL IS SUPPORTED
    // ----------------------------------------------------------------------
    if (GLOBAL.WebGLRenderingContext) {
        var canvas = GLOBAL.document.createElement('canvas'),
        gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
        if(gl) {
            // WebGL is supported
            if(!module.hasOwnProperty('ViewportFactory')) {
                module['ViewportFactory'] = {};
            }
            module.ViewportFactory[FACTORY_KEY] = FACTORY;
        }
    }
}(window, jQuery));