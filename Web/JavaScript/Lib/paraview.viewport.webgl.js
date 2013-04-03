/**
 * ParaViewWeb JavaScript Library.
 *
 * This module extend the ParaView viewport to add support for WebGL rendering.
 *
 * @class paraview.viewport.webgl
 */
(function (GLOBAL, $) {
    var module = {},
    RENDERER_CSS = {
        "position": "absolute",
        "top"     : "0px",
        "left"    : "0px",
        "right"   : "0px",
        "bottom"  : "0px",
        "z-index" : "0"
    },
    RENDERER_CSS_2D = {
        "z-index" : "1"
    },
    RENDERER_CSS_3D = {
        "z-index" : "0"
    },
    DEFAULT_OPTIONS = {
        /**
         * @member pv.ViewPortConfig
         * @property {Boolean} keepServerInSynch
         * This will force network communication to the server to update the
         * camera position on the server as well.
         * This is useful when collaboration is involved and several users
         * are looking at the same scene.
         *
         * Default: false
         */
        keepServerInSynch: false
    },
    FACTORY_KEY = 'webgl',
    FACTORY = {
        'builder': createGeometryDeliveryRenderer,
        'options': DEFAULT_OPTIONS,
        'stats': {
            'webgl-fps': {
                label: 'Framerate:',
                type: 'time',
                convert: function(value) {
                    if(value === 0) {
                        return 0;
                    }
                    return Math.floor(1000 / value);
                }
            },
            'webgl-nb-objects': {
                label: 'Number of 3D objects:',
                type: 'value',
                convert: NoOp
            },
            'webgl-fetch-scene': {
                label: 'Fetch scene (ms):',
                type: 'time',
                convert: NoOp
            },
            'webgl-fetch-object': {
                label: 'Fetch object (ms):',
                type: 'time',
                convert: NoOp
            }
        }
    },
    DEFAULT_SHADERS = {},
    mvMatrixStack = [];


    // ----------------------------------------------------------------------

    function NoOp(a) {
        return a;
    }

    // ----------------------------------------------------------------------
    // Initialize the Shaders
    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-fs"] = {
        type: "x-shader/x-fragment",
        code: "\
            #ifdef GL_ES\n\
            precision highp float;\n\
            #endif\n\
            uniform bool uIsLine;\
            varying vec4 vColor;\
            varying vec4 vTransformedNormal;\
            varying vec4 vPosition;\
            void main(void) {\
                float directionalLightWeighting1 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, 1.0)), 0.0); \
                float directionalLightWeighting2 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, -1.0)), 0.0);\
                vec3 lightWeighting = max(vec3(1.0, 1.0, 1.0) * directionalLightWeighting1, vec3(1.0, 1.0, 1.0) * directionalLightWeighting2);\
                if (uIsLine == false){\
                  gl_FragColor = vec4(vColor.rgb * lightWeighting, vColor.a);\
                } else {\
                  gl_FragColor = vColor*vec4(1.0, 1.0, 1.0, 1.0);\
                }\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-vs"] = {
        type: "x-shader/x-vertex",
        code: "\
            attribute vec3 aVertexPosition;\
            attribute vec4 aVertexColor;\
            attribute vec3 aVertexNormal;\
            uniform mat4 uMVMatrix;\
            uniform mat4 uPMatrix;\
            uniform mat4 uNMatrix;\
            varying vec4 vColor;\
            varying vec4 vPosition;\
            varying vec4 vTransformedNormal;\
            void main(void) {\
                vPosition = uMVMatrix * vec4(aVertexPosition, 1.0);\
                gl_Position = uPMatrix * vPosition;\
                vTransformedNormal = uNMatrix * vec4(aVertexNormal, 1.0);\
                vColor = aVertexColor;\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-fs-Point"] = {
        type: "x-shader/x-fragment",
        code: "\
            #ifdef GL_ES\n\
            precision highp float;\n\
            #endif\n\
            varying vec4 vColor;\
            void main(void) {\
                gl_FragColor = vColor;\
            }"
    };

    // ----------------------------------------------------------------------

    DEFAULT_SHADERS["shader-vs-Point"] = {
        type: "x-shader/x-vertex",
        code: "\
            attribute vec3 aVertexPosition;\
            attribute vec4 aVertexColor;\
            uniform mat4 uMVMatrix;\
            uniform mat4 uPMatrix;\
            uniform mat4 uNMatrix;\
            uniform float uPointSize;\
            varying vec4 vColor;\
            void main(void) {\
                vec4 pos = uMVMatrix * vec4(aVertexPosition, 1.0);\
                gl_Position = uPMatrix * pos;\
                vColor = aVertexColor*vec4(1.0, 1.0, 1.0, 1.0);\
                gl_PointSize = uPointSize;\
            }"
    };

    // ----------------------------------------------------------------------

    function getShader(gl, id) {
        try {
            var jsonShader = DEFAULT_SHADERS[id], shader = null;

            // Allocate shader
            if(jsonShader.type === "x-shader/x-fragment") {
                shader = gl.createShader(gl.FRAGMENT_SHADER);
            } else if(jsonShader.type === "x-shader/x-vertex") {
                shader = gl.createShader(gl.VERTEX_SHADER);
            } else {
                return null;
            }

            // Set code and compile
            gl.shaderSource(shader, jsonShader.code);
            gl.compileShader(shader);

            // Check compilation
            if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
                alert(gl.getShaderInfoLog(shader));
                return null;
            }

            return shader;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function initializeShader(gl, shaderProgram, pointShaderProgram) {
        try {

            var vertexShader = getShader(gl, 'shader-vs'),
            fragmentShader   = getShader(gl, 'shader-fs'),
            pointFragShader  = getShader(gl, 'shader-fs-Point'),
            pointVertShader  = getShader(gl, 'shader-vs-Point');

            // Initialize program
            gl.attachShader(shaderProgram, vertexShader);
            gl.attachShader(shaderProgram, fragmentShader);
            gl.linkProgram(shaderProgram);
            if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
                alert("Could not initialise shaders");
            }

            gl.attachShader(pointShaderProgram, pointVertShader);
            gl.attachShader(pointShaderProgram, pointFragShader);
            gl.linkProgram(pointShaderProgram);
            if (!gl.getProgramParameter(pointShaderProgram, gl.LINK_STATUS)) {
                alert("Could not initialise the point shaders");
            }

            gl.useProgram(pointShaderProgram);
            pointShaderProgram.vertexPositionAttribute = gl.getAttribLocation(pointShaderProgram, "aVertexPosition");
            gl.enableVertexAttribArray(pointShaderProgram.vertexPositionAttribute);
            pointShaderProgram.vertexColorAttribute = gl.getAttribLocation(pointShaderProgram, "aVertexColor");
            gl.enableVertexAttribArray(pointShaderProgram.vertexColorAttribute);
            pointShaderProgram.pMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uPMatrix");
            pointShaderProgram.mvMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uMVMatrix");
            pointShaderProgram.nMatrixUniform = gl.getUniformLocation(pointShaderProgram, "uNMatrix");
            pointShaderProgram.uPointSize = gl.getUniformLocation(pointShaderProgram, "uPointSize");

            gl.useProgram(shaderProgram);
            shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
            gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
            shaderProgram.vertexColorAttribute = gl.getAttribLocation(shaderProgram, "aVertexColor");
            gl.enableVertexAttribArray(shaderProgram.vertexColorAttribute);
            shaderProgram.vertexNormalAttribute = gl.getAttribLocation(shaderProgram, "aVertexNormal");
            gl.enableVertexAttribArray(shaderProgram.vertexNormalAttribute);
            shaderProgram.pMatrixUniform = gl.getUniformLocation(shaderProgram, "uPMatrix");
            shaderProgram.mvMatrixUniform = gl.getUniformLocation(shaderProgram, "uMVMatrix");
            shaderProgram.nMatrixUniform = gl.getUniformLocation(shaderProgram, "uNMatrix");
            shaderProgram.uIsLine = gl.getUniformLocation(shaderProgram, "uIsLine");
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------
    // GL rendering metods
    // ----------------------------------------------------------------------

    function setMatrixUniforms(renderingContext, shaderProgram) {
        var mvMatrixInv = mat4.create(),
        normal = mat4.create();

        mat4.inverse(renderingContext.mvMatrix, mvMatrixInv);
        mat4.transpose(mvMatrixInv, normal);

        renderingContext.gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, renderingContext.pMatrix);
        renderingContext.gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, renderingContext.mvMatrix);
        if(shaderProgram.nMatrixUniform != null) renderingContext.gl.uniformMatrix4fv(shaderProgram.nMatrixUniform, false, normal);
    }

    // ----------------------------------------------------------------------

    function renderMesh(sceneJSON, renderingContext) {
        try {
            var obj = this, cameraRot, inverse, objTransp, icenter;

            renderingContext.gl.useProgram(renderingContext.shaderProgram);

            renderingContext.gl.uniform1i(renderingContext.shaderProgram.uIsLine, false);

            cameraRot = mat4.toRotationMat(renderingContext.mvMatrix);
            mat4.transpose(cameraRot);
            inverse = mat4.create();
            mat4.inverse(cameraRot, inverse);
            objTransp = mat4.create(obj.matrix);
            mat4.transpose(objTransp);

            icenter = [-sceneJSON.Center[0], -sceneJSON.Center[1], -sceneJSON.Center[2]];

            mvPushMatrix(renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, renderingContext.transform.translation);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);

            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, sceneJSON.Center);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0){
                mat4.scale( renderingContext.mvMatrix, [renderingContext.transform.scale, renderingContext.transform.scale, renderingContext.transform.scale],renderingContext.mvMatrix);
            }
            mat4.multiply(renderingContext.mvMatrix, renderingContext.transform.rotation, renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, icenter);

            renderingContext.transform.rotation2 = renderingContext.mvMatrix;

            mat4.multiply(renderingContext.mvMatrix, objTransp, renderingContext.mvMatrix);

            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.vbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexPositionAttribute, obj.vbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.nbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.cbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);

            setMatrixUniforms(renderingContext, renderingContext.shaderProgram);

            renderingContext.gl.drawElements(renderingContext.gl.TRIANGLES, obj.numberOfIndex, renderingContext.gl.UNSIGNED_SHORT, 0);
            renderingContext.mvMatrix = mvPopMatrix();
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderLine(sceneJSON, renderingContext) {
        try {
            var obj = this, cameraRot, inverse, objTransp, icenter;

            renderingContext.gl.useProgram(renderingContext.shaderProgram);

            renderingContext.gl.enable(renderingContext.gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
            renderingContext.gl.polygonOffset(-1.0, -1.0);

            renderingContext.gl.uniform1i(renderingContext.shaderProgram.uIsLine, true);

            cameraRot = mat4.toRotationMat(renderingContext.mvMatrix);
            mat4.transpose(cameraRot);
            inverse = mat4.create();
            mat4.inverse(cameraRot, inverse);
            objTransp = mat4.create(obj.matrix);
            mat4.transpose(objTransp);

            icenter = [-sceneJSON.Center[0], -sceneJSON.Center[1], -sceneJSON.Center[2]];

            mvPushMatrix(renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, renderingContext.transform.translation);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);

            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, sceneJSON.Center);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.scale(renderingContext.mvMatrix, [renderingContext.transform.scale, renderingContext.transform.scale, renderingContext.transform.scale], renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, renderingContext.transform.rotation, renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, icenter);

            renderingContext.transform.rotation2 = renderingContext.mvMatrix;

            mat4.multiply(renderingContext.mvMatrix, objTransp, renderingContext.mvMatrix);

            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.lbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexPositionAttribute, obj.lbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.nbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.cbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);

            setMatrixUniforms(renderingContext, renderingContext.shaderProgram);

            renderingContext.gl.drawElements(renderingContext.gl.LINES, obj.numberOfIndex, renderingContext.gl.UNSIGNED_SHORT, 0);
            renderingContext.mvMatrix = mvPopMatrix();

            renderingContext.gl.disable(renderingContext.gl.POLYGON_OFFSET_FILL);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderPoints(sceneJSON, renderingContext) {
        try {
            var obj = this, cameraRot, inverse, objTransp, icenter;

            renderingContext.gl.useProgram(renderingContext.pointShaderProgram);

            renderingContext.gl.enable(renderingContext.gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
            renderingContext.gl.polygonOffset(-1.0, -1.0);

            renderingContext.gl.uniform1f(renderingContext.pointShaderProgram.uPointSize, 2.0);//Wendel

            cameraRot = mat4.toRotationMat(renderingContext.mvMatrix);
            mat4.transpose(cameraRot);
            inverse = mat4.create();
            mat4.inverse(cameraRot, inverse);
            objTransp = mat4.create(obj.matrix);
            mat4.transpose(objTransp);

            icenter = [-sceneJSON.Center[0], -sceneJSON.Center[1], -sceneJSON.Center[2]];

            mvPushMatrix(renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, renderingContext.transform.translation);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);

            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, sceneJSON.Center);
            mat4.multiply(renderingContext.mvMatrix, cameraRot, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.scale(renderingContext.mvMatrix, [renderingContext.transform.scale, renderingContext.transform.scale, renderingContext.transform.scale], renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, renderingContext.transform.rotation, renderingContext.mvMatrix);
            mat4.multiply(renderingContext.mvMatrix, inverse, renderingContext.mvMatrix);
            if(obj.layer == 0) mat4.translate(renderingContext.mvMatrix, icenter);

            renderingContext.transfrom.rotation2 = renderingContext.mvMatrix;

            mat4.multiply(renderingContext.mvMatrix, objTransp, renderingContext.mvMatrix);

            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.pbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.pointShaderProgram.vertexPositionAttribute, obj.pbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);
            renderingContext.gl.bindBuffer(renderingContext.gl.ARRAY_BUFFER, obj.cbuff);
            renderingContext.gl.vertexAttribPointer(renderingContext.pointShaderProgram.vertexColorAttribute, obj.cbuff.itemSize, renderingContext.gl.FLOAT, false, 0, 0);

            setMatrixUniforms(renderingContext, renderingContext.pointShaderProgram);

            renderingContext.gl.drawArrays(renderingContext.gl.POINTS, 0, obj.numberOfPoints);
            renderingContext.mvMatrix = mvPopMatrix();

            renderingContext.gl.disable(renderingContext.gl.POLYGON_OFFSET_FILL);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderColorMap(sceneJSON, renderingContext) {
        try {
            var obj = this, ctx = renderingContext.ctx2d, range, txt, color, c, v,
            size, pos, dx, dy, realSize, textSizeX, textSizeY, grad,
            width = renderingContext.container.width(),
            height = renderingContext.container.height();

            range = [obj.colors[0][0], obj.colors[obj.colors.length-1][0]];
            size = [obj.size[0]*width, obj.size[1]*height];
            pos = [obj.position[0]*width, (1-obj.position[1])*height];
            pos[1] = pos[1]-size[1];
            dx = size[0]/size[1];
            dy = size[1]/size[0];
            realSize = size;

            textSizeX = Math.round(height/35);
            textSizeY = Math.round(height/23);
            if (obj.orientation == 1){
                size[0] = size[0]*dy/25;
                size[1] = size[1]-(2*textSizeY);
            } else {
                size[0] = size[0];
                size[1] = size[1]*dx/25;
            }

            // Draw Gradient
            if(obj.orientation == 1){
                pos[1] += 2*textSizeY;
                grad = ctx.createLinearGradient(pos[0], pos[1], pos[0], pos[1]+size[1]);
            } else {
                pos[1] += 2*textSizeY;
                grad = ctx.createLinearGradient(pos[0], pos[1], pos[0]+size[0], pos[1]);
            }
            if ((range[1]-range[0]) == 0){
                color = 'rgba(' + obj.colors[0][1] + ',' + obj.colors[0][2] + ',' + obj.colors[0][3] + ',1)';
                grad.addColorStop(0, color);
                grad.addColorStop(1, color);
            } else {
                for(c=0; c<obj.colors.length; c++){
                    v = ((obj.colors[c][0]-range[0])/(range[1]-range[0]));
                    if (obj.orientation == 1) v=1-v;
                    color = 'rgba(' + obj.colors[c][1] + ',' + obj.colors[c][2] + ',' + obj.colors[c][3] + ',1)';
                    grad.addColorStop(v, color);
                }
            }
            ctx.fillStyle = grad;
            ctx.fillRect(pos[0], pos[1], size[0], size[1]);
            // Draw Range Labels
            range[0] = Math.round(range[0]*1000)/1000;
            range[1] = Math.round(range[1]*1000)/1000;
            ctx.fillStyle = 'white';
            ctx.font = textSizeY + 'px sans-serif';
            ctx.txtBaseline = 'ideographic';
            if (obj.orientation == 1){
                ctx.fillText(range[1], pos[0], pos[1]-5);
                ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
            } else {
                ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
                txt = range[1].toString();
                ctx.fillText(range[1], pos[0]+size[0]-((txt.length-1)*textSizeX), pos[1]+size[1]+textSizeY);
            }
            // Draw Title
            ctx.fillStyle = 'white';
            ctx.font = textSizeY + 'px sans-serif';
            ctx.txtBaseline = 'ideographic';
            if (obj.orientation == 1) ctx.fillText(obj.title, pos[0]+(obj.size[0]*width)/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
            else ctx.fillText(obj.title, pos[0]+size[0]/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function renderBackground(sceneJSON, renderingContext) {
        try {
            var background = this, gl = renderingContext.gl, shaderProgram = renderingContext.shaderProgram;

            gl.useProgram(renderingContext.shaderProgram);
            gl.uniform1i(renderingContext.shaderProgram.uIsLine, false);

            mat4.translate(renderingContext.mvMatrix, [0.0, 0.0, -1.0]);

            gl.bindBuffer(gl.ARRAY_BUFFER, background.vbuff);
            gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, background.vbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, background.nbuff);
            gl.vertexAttribPointer(shaderProgram.vertexNormalAttribute, background.nbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ARRAY_BUFFER, background.cbuff);
            gl.vertexAttribPointer(shaderProgram.vertexColorAttribute, background.cbuff.itemSize, gl.FLOAT, false, 0, 0);
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, background.ibuff);

            setMatrixUniforms(renderingContext, shaderProgram);

            gl.drawElements(gl.TRIANGLES, background.numberOfIndex, gl.UNSIGNED_SHORT, 0);
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------
    // 3D object handler
    // ----------------------------------------------------------------------

    function create3DObjectHandler() {
        var objectIndex = {}, displayList = {}, sceneJSON;

        // ------------------------------------------------------------------

        function getKey(object) {
            return object.id + '_' + object.md5;
        }

        // ------------------------------------------------------------------

        function getLayerDisplayList(layer) {
            var key = String(layer);
            if(!displayList.hasOwnProperty(key)) {
                displayList[key] = {
                    transparent: [],
                    solid: []
                };
            }
            return displayList[key];
        }

        // ------------------------------------------------------------------
        function render(displayList, renderingContext) {
            var i, k, key, array;
            for(i in displayList) {
                key = displayList[i];
                if(objectIndex.hasOwnProperty(key)) {
                    array = objectIndex[key];
                    for(k in array) {
                        array[k].render(sceneJSON, renderingContext);
                    }
                }
            }
            return displayList.length;
        }

        // ------------------------------------------------------------------

        return {
            registerObject: function(object) {
                var key = getKey(object), idx;
                if(!objectIndex.hasOwnProperty(key)) {
                    objectIndex[key] = [];
                } else {
                    // Make sure is not already in
                    for(idx in objectIndex[key]) {
                        if(objectIndex[key][idx].part === object.part) {
                            return;
                        }
                    }
                }

                // Add it
                objectIndex[key].push(object);
            },

            // --------------------------------------------------------------

            updateDisplayList: function(scene) {
                // Reset displayList
                displayList = {}, sceneJSON = scene;

                // Create display lists
                for(var idx in sceneJSON.Objects) {
                    var currentObject = sceneJSON.Objects[idx],
                    displayListKey = currentObject.hasTransparency ? 'transparent' : 'solid',
                    key = getKey(currentObject);

                    getLayerDisplayList(currentObject.layer)[displayListKey].push(key);
                }
            },

            // --------------------------------------------------------------

            renderTransparent: function(layer, renderingContext) {
                var displayList = getLayerDisplayList(layer).transparent;
                return render(displayList, renderingContext);
            },

            // --------------------------------------------------------------

            renderSolid: function(layer, renderingContext) {
                var displayList = getLayerDisplayList(layer).solid;
                return render(displayList, renderingContext);
            },

            // --------------------------------------------------------------

            fetchMissingObjects: function(fetchMethod) {
                var fetch = fetchMethod, idx, part;
                for(idx in sceneJSON.Objects) {
                    var currentObject = sceneJSON.Objects[idx],
                    key = getKey(currentObject);
                    if(!objectIndex.hasOwnProperty(key)) {
                        // Request all the pieces
                        for(part = 1; part <= currentObject.parts; part++) {
                            fetch(currentObject, part);
                        }
                    }
                }
            },

            // --------------------------------------------------------------

            garbageCollect: function() {
                var refCount = {};
                for(var key in objectIndex) {
                    refCount[key] = 0;
                }

                // Start registering display list
                for(var layer in displayList) {
                    var array = displayList[layer].solid.concat(displayList[layer].transparent);
                    for(var idx in array) {
                        if(refCount.hasOwnProperty(array[idx])) {
                            refCount[array[idx]]++;
                        }
                    }
                }

                // Remove entry with no reference
                for(var key in refCount) {
                    if(refCount[key] === 0) {
                        delete objectIndex[key];
                    }
                }
            }

        }
    }

    // ----------------------------------------------------------------------
    // GL object creation
    // ----------------------------------------------------------------------

    function get4ByteNumber(binaryArray, cursor) {
        return (binaryArray[cursor++]) + (binaryArray[cursor++] << 8) + (binaryArray[cursor++] << 16) + (binaryArray[cursor++] << 24);
    }

    // ----------------------------------------------------------------------

    function buildBackground(gl, color1, color2) {
        try {
            if (typeof(gl) == "undefined") return;

            var background = {
                vertices: new Float32Array([-1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0]),
                colors: new Float32Array([
                    color1[0], color1[1], color1[2], 1.0,
                    color1[0], color1[1], color1[2], 1.0,
                    color2[0], color2[1], color2[2], 1.0,
                    color2[0], color2[1], color2[2], 1.0]),
                index: new Uint16Array([0, 1, 2, 0, 2, 3]),
                normals: new Float32Array([0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0]),
                numberOfIndex: 6
            };

            //Create Buffers
            background.vbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.vbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.vertices, gl.STATIC_DRAW);
            background.vbuff.itemSize = 3;
            background.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.normals, gl.STATIC_DRAW);
            background.nbuff.itemSize = 3;
            background.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, background.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, background.colors, gl.STATIC_DRAW);
            background.cbuff.itemSize = 4;
            background.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, background.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, background.index, gl.STREAM_DRAW);

            // bind render method
            background.render = renderBackground;

            return background;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processWireframe(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Extract points
            obj.numberOfPoints = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Points
            size = obj.numberOfPoints * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.points = new Float32Array(tmpArray.buffer);

            // Generating Normals
            size = obj.numberOfPoints * 3;
            tmpArray = new Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = 0.0;
            }
            obj.normals = new Float32Array(tmpArray);

            // Getting Colors
            size = obj.numberOfPoints * 4;
            tmpArray = new Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;;
            }
            obj.colors = new Float32Array(tmpArray);

            // Extract the number of index
            obj.numberOfIndex = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Index
            size = obj.numberOfIndex * 2;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.index = new Uint16Array(tmpArray.buffer);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Creating Buffers
            obj.lbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.lbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.points, gl.STATIC_DRAW);
            obj.lbuff.itemSize = 3;

            obj.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.normals, gl.STATIC_DRAW);
            obj.nbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            obj.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, obj.index, gl.STREAM_DRAW);

            // Bind render method
            obj.render = renderLine;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processSurfaceMesh(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Extract number of vertices
            obj.numberOfVertices = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Vertices
            size = obj.numberOfVertices * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.vertices = new Float32Array(tmpArray.buffer);

            // Getting Normals
            size = obj.numberOfVertices * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.normals = new Float32Array(tmpArray.buffer);

            // Getting Colors
            tmpArray = [];
            size = obj.numberOfVertices * 4;
            for(i=0; i < size; i++) {
                tmpArray[i] =  binaryArray[cursor++] / 255.0;
            }
            obj.colors = new Float32Array(tmpArray);

            // Get number of index
            obj.numberOfIndex = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Index
            size = obj.numberOfIndex * 2;
            tmpArray  = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.index = new Uint16Array(tmpArray.buffer);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Getting TCoord
            obj.tcoord = null;

            // Create Buffers
            obj.vbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.vbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.vertices, gl.STATIC_DRAW);
            obj.vbuff.itemSize = 3;

            obj.nbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.nbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.normals, gl.STATIC_DRAW);
            obj.nbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            obj.ibuff = gl.createBuffer();
            gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
            gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, obj.index, gl.STREAM_DRAW);

            // Bind render method
            obj.render = renderMesh;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processColorMap(gl, obj, nbColor, binaryArray, cursor) {
        try {
            var tmpArray, size, xrgb, i, c;

            // Set number of colors
            obj.numOfColors = nbColor;

            // Getting Position
            size = 2 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.position = new Float32Array(tmpArray.buffer);

            // Getting Size
            size = 2 * 4;
            tmpArray = new Int8Array(2*4);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.size = new Float32Array(tmpArray.buffer);

            //Getting Colors
            obj.colors = [];
            for(c=0; c < obj.numOfColors; c++){
                tmpArray = new Int8Array(4);
                for(i=0; i < 4; i++) {
                    tmpArray[i] = binaryArray[cursor++];
                }
                xrgb = [
                new Float32Array(tmpArray.buffer)[0],
                binaryArray[cursor++],
                binaryArray[cursor++],
                binaryArray[cursor++]
                ];
                obj.colors[c] = xrgb;
            }

            obj.orientation = binaryArray[cursor++];
            obj.numOfLabels = binaryArray[cursor++];
            obj.title = "";
            while(cursor < binaryArray.length) {
                obj.title += String.fromCharCode(binaryArray[cursor++]);
            }

            // Bind render method
            obj.render = renderColorMap;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function processPointSet(gl, obj, binaryArray, cursor) {
        try {
            var tmpArray, size, i;

            // Get number of points
            obj.numberOfPoints = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Getting Points
            size = obj.numberOfPoints * 4 * 3;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++];
            }
            obj.points = new Float32Array(tmpArray.buffer);

            // Getting Colors
            size = obj.numberOfPoints * 4;
            tmpArray = [];
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;
            }
            obj.colors = new Float32Array(tmpArray);

            // Getting Matrix
            size = 16 * 4;
            tmpArray = new Int8Array(size);
            for(i=0; i < size; i++) {
                tmpArray[i] = binaryArray[cursor++]/255.0;
            }
            obj.matrix = new Float32Array(tmpArray.buffer);

            // Creating Buffers
            obj.pbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.pbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.points, gl.STATIC_DRAW);
            obj.pbuff.itemSize = 3;

            obj.cbuff = gl.createBuffer();
            gl.bindBuffer(gl.ARRAY_BUFFER, obj.cbuff);
            gl.bufferData(gl.ARRAY_BUFFER, obj.colors, gl.STATIC_DRAW);
            obj.cbuff.itemSize = 4;

            // Bind render method
            obj.render = renderPoints;
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function initializeObject(gl, obj) {
        try {
            var binaryArray = [], cursor = 0, size, type;

            // Convert char to byte
            for(var i in obj.data) {
                binaryArray.push(obj.data.charCodeAt(i) & 0xff);
            }

            // Extract size (4 bytes)
            size = get4ByteNumber(binaryArray, cursor);
            cursor += 4;

            // Extract object type
            type = String.fromCharCode(binaryArray[cursor++]);
            obj.type = type;

            // Extract raw data
            if (type == 'L'){
                processWireframe(gl, obj, binaryArray, cursor);
            } else if (type == 'M'){
                processSurfaceMesh(gl, obj, binaryArray, cursor);
            } else if (type == 'C'){
                processColorMap(gl, obj, size, binaryArray, cursor);
            } else if (type == 'P'){
                processPointSet(gl, obj, binaryArray, cursor);
            }
        } catch(error) {
            console.log(error);
        }
    }

    // ----------------------------------------------------------------------

    function mvPushMatrix(m) {
        var copy = mat4.create();
        mat4.set(m, copy);
        mvMatrixStack.push(copy);
    }

    // ----------------------------------------------------------------------

    function mvPopMatrix() {
        if (mvMatrixStack.length == 0) {
            throw "Invalid popMatrix!";
        }
        return mvMatrixStack.pop();
    }

    // ----------------------------------------------------------------------
    // Geometry Delivery renderer - factory method
    // ----------------------------------------------------------------------

    function createGeometryDeliveryRenderer(domElement) {
        var container = $(domElement),
        options = $.extend({}, DEFAULT_OPTIONS, container.data('config')),
        session = options.session,
        divContainer = GLOBAL.document.createElement('div'),
        canvas2D = GLOBAL.document.createElement('canvas'),
        canvas3D = GLOBAL.document.createElement('canvas'),
        ctx2d = canvas2D.getContext('2d'),
        gl = canvas3D.getContext("experimental-webgl") || canvas3D.getContext("webgl"),
        shaderProgram = gl.createProgram(),
        pointShaderProgram = gl.createProgram(),
        renderer = $(divContainer).addClass(FACTORY_KEY).css(RENDERER_CSS).append($(canvas2D).css(RENDERER_CSS).css(RENDERER_CSS_2D)).append($(canvas3D).css(RENDERER_CSS).css(RENDERER_CSS_3D)),
        sceneJSON = null,
        objectHandler = create3DObjectHandler(),
        mouseHandling = {
            button: null,
            lastX: 0,
            lastY: 0
        },
        renderingContext = {
            container: container,
            gl: gl,
            ctx2d: ctx2d,
            shaderProgram: shaderProgram,
            pointShaderProgram: pointShaderProgram,
            mvMatrix: mat4.create(),
            pMatrix: mat4.create(),
            transform: {
                translation: [0.0, 0.0, 0.0],
                rotation: mat4.create(),
                rotation2: mat4.create(),
                scale: 1.0
            }
        },
        oldLookAt = [0,0,0,0,0,0,0,0,0],
        camera = {
            lookAt: [0,0,0,0,1,0,0,0,1],
            z_dir: [],
            up: [],
            right: []
        },
        background = null;

        // Init rotation to be identity
        mat4.identity(renderingContext.mvMatrix);
        mat4.identity(renderingContext.pMatrix);
        mat4.identity(renderingContext.transform.rotation);
        mat4.identity(renderingContext.transform.rotation2);

        // Helper functions -------------------------------------------------

        function fetchScene() {
            container.trigger({
                type: 'stats',
                stat_id: 'webgl-fetch-scene',
                stat_value: 0
            });
            session.call("pv:getSceneMetaData", Number(options.view)).then(function(data) {
                sceneJSON = JSON.parse(data);
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fetch-scene',
                    stat_value: 1
                });
                updateScene();
            });
        }

        // ------------------------------------------------------------------

        function fetchObject(sceneObject, part) {
            try {
                var viewId = Number(options.view),
                newObject;

                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fetch-object',
                    stat_value: 0
                });
                session.call("pv:getWebGLData", viewId, sceneObject.id, part).then(function(data) {
                    try {
                        // decode base64
                        data = atob(data);
                        container.trigger({
                            type: 'stats',
                            stat_id: 'webgl-fetch-object',
                            stat_value: 1
                        });

                        newObject = {
                            md5: sceneObject.md5,
                            part: part,
                            vid: viewId,
                            id: sceneObject.id,
                            data: data,
                            hasTransparency: sceneObject.hasTransparency,
                            layer: sceneObject.layer,
                            render: function(){}
                        };

                        // Process object
                        initializeObject(gl, newObject);

                        // Register it for rendering
                        objectHandler.registerObject(newObject);

                        // Redraw the scene
                        drawScene();
                    } catch(error) {
                        console.log(error);
                    }
                });
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function drawScene() {
            try {
                if (sceneJSON == null){
                    return;
                }
                var currentTime = new Date().getTime(), deltaT,
                width = renderer.width(), height = renderer.height(),
                layer, localRenderer, localWidth, localHeight, localX, localY,
                nbObjects = 0;

                // Update frame rate
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 0
                });

                // Update viewport size
                ctx2d.canvas.width = width;
                ctx2d.canvas.height = height;
                gl.canvas.width = width;
                gl.canvas.height = height;
                gl.viewportWidth = width;
                gl.viewportHeight = height;

                // Clear 3D context
                gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
                gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

                mat4.ortho(-1.0, 1.0, -1.0, 1.0, 1.0, 1000000.0, renderingContext.pMatrix);
                mat4.identity(renderingContext.mvMatrix);

                // Draw background
                gl.disable(gl.DEPTH_TEST);
                if(background != null) {
                    background.render(sceneJSON, renderingContext);
                }
                gl.enable(gl.DEPTH_TEST);

                // Clear 2D overlay canvas
                ctx2d.clearRect(0, 0, width, height);

                // Render each layer on top of each other (Starting with the background one)
                for(layer = sceneJSON.Renderers.length - 1; layer >= 0; layer--) {
                    localRenderer = sceneJSON.Renderers[layer];
                    localWidth = localRenderer.size[0] - localRenderer.origin[0];
                    localHeight = localRenderer.size[1] - localRenderer.origin[1];

                    // Convert % to pixel based
                    localWidth *= width;
                    localHeight *= height;
                    localX = localRenderer.origin[0] * width;
                    localY = localRenderer.origin[1] * height;
                    localX = (localX < 0) ? 0 : localX;
                    localY = (localY < 0) ? 0 : localY;

                    // Setup viewport
                    gl.viewport(localX, localY, localWidth, localHeight);

                    mat4.perspective(localRenderer.LookAt[0], width/height, 0.1, 1000000.0, renderingContext.pMatrix);
                    mat4.identity(renderingContext.mvMatrix);
                    mat4.lookAt(
                        [localRenderer.LookAt[7], localRenderer.LookAt[8], localRenderer.LookAt[9]],
                        [localRenderer.LookAt[1], localRenderer.LookAt[2], localRenderer.LookAt[3]],
                        [localRenderer.LookAt[4], localRenderer.LookAt[5], localRenderer.LookAt[6]],
                        renderingContext.mvMatrix);

                    // Render non-transparent objects for the current layer
                    nbObjects += objectHandler.renderSolid(layer, renderingContext);

                    // Now render transparent objects
                    gl.enable(gl.BLEND);                //Enable transparency
                    gl.enable(gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
                    gl.polygonOffset(-1.0, -1.0);

                    nbObjects += objectHandler.renderTransparent(layer, renderingContext);

                    gl.disable(gl.POLYGON_OFFSET_FILL);
                    gl.disable(gl.BLEND);
                }

                // Update frame rate
                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-fps',
                    stat_value: 1
                });

                container.trigger({
                    type: 'stats',
                    stat_id: 'webgl-nb-objects',
                    stat_value: nbObjects
                });
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function updateScene() {
            try{
                if(sceneJSON === null || typeof(sceneJSON) === "undefined") {
                    return;
                }

                // Local variables
                var bgColor1 = [0,0,0], bgColor2 = [0,0,0];

                // Update Background (Layer 0)
                for(var idx = 0; idx < sceneJSON.Renderers.length; idx++) {
                    if(sceneJSON.Renderers[idx].layer == 0) {
                        camera.lookAt = sceneJSON.Renderers[idx].LookAt;
                        bgColor1 = bgColor2 = sceneJSON.Renderers[idx].Background1;
                        if(typeof(sceneJSON.Renderers[idx].Background2) != "undefined") {
                            bgColor2 = sceneJSON.Renderers[idx].Background2;
                        }
                    }
                }
                background = buildBackground(gl, bgColor1, bgColor2);

                // Handle camera position
                if (JSON.stringify(oldLookAt) != JSON.stringify(camera.lookAt)) {
                    renderingContext.transform.translation = [0.0, 0.0, 0.0];
                    renderingContext.transform.scale = 1.0;
                    mat4.identity(renderingContext.transform.rotation);

                    camera.up = [camera.lookAt[4], camera.lookAt[5], camera.lookAt[6]];
                    camera.z_dir = [
                    camera.lookAt[1]-camera.lookAt[7],
                    camera.lookAt[2]-camera.lookAt[8],
                    camera.lookAt[3]-camera.lookAt[9]];
                    vec3.normalize(camera.z_dir, camera.z_dir);
                    vec3.cross(camera.z_dir, camera.up, camera.right);
                }
                oldLookAt = camera.lookAt;

                // Update the list of object to render
                objectHandler.updateDisplayList(sceneJSON);

                // Fetch the object that we are missing
                objectHandler.fetchMissingObjects(fetchObject);

                // Draw scene
                drawScene();
            } catch(error) {
                console.log(error);
            }
        }

        // ------------------------------------------------------------------

        function updateCamera() {
            if(sceneJSON === null || typeof(sceneJSON) === "undefined") {
                return;
            }

            var pos = [camera.lookAt[7], camera.lookAt[8], camera.lookAt[9]],
            up  = [camera.lookAt[4], camera.lookAt[5], camera.lookAt[6]],
            fp  = [camera.lookAt[1], camera.lookAt[2], camera.lookAt[3]],
            tt  = [renderingContext.transform.translation[0], renderingContext.transform.translation[1], 0.0],
            center = [sceneJSON.Center[0], sceneJSON.Center[1], sceneJSON.Center[2]],
            cameraRot, inverse, inv,
            viewId = Number(options.view);

            cameraRot = mat4.toRotationMat(renderingContext.mvMatrix);
            mat4.transpose(cameraRot);
            inverse = mat4.create();
            mat4.inverse(cameraRot, inverse);

            inv = mat4.create();
            mat4.identity(inv);
            mat4.multiply(inv, cameraRot, inv);
            mat4.scale(inv, [renderingContext.transform.scale, renderingContext.transform.scale, renderingContext.transform.scale], inv);
            mat4.multiply(inv, renderingContext.transform.rotation, inv);
            mat4.multiply(inv, inverse, inv);

            mat4.inverse(inv, inv);
            fp = vec3.subtract(fp, center, fp);
            pos = vec3.subtract(pos, center, pos);
            mat4.multiplyVec3(inv, fp, fp);
            mat4.multiplyVec3(inv, pos, pos);
            mat4.multiplyVec3(inv, up, up);
            fp = vec3.add(fp, center, fp);
            pos = vec3.add(pos, center, pos);
            vec3.normalize(up, up);

            tt2 = [0, 0, 0];
            tt2[0] += tt[0]*camera.right[0];
            tt2[1] += tt[0]*camera.right[1];
            tt2[2] += tt[0]*camera.right[2];
            tt2[0] += tt[1]*camera.up[0];
            tt2[1] += tt[1]*camera.up[1];
            tt2[2] += tt[1]*camera.up[2];

            vec3.subtract(pos, tt2, pos);
            vec3.subtract(fp , tt2, fp);

        // this.session.call("pv:updateCamera", viewId, fp, up, pos);
        }

        // ------------------------------------------------------------------
        // Add renderer into the DOM
        container.append(renderer);

        // ------------------------------------------------------------------
        // Add viewport listener
        container.bind('invalidateScene', function() {
            if(renderer.hasClass('active')){
                fetchScene();
            }
        }).bind('render', function(){
            if(renderer.hasClass('active')){
                drawScene();
            }
        }).bind('mouse', function(event){
            if(renderer.hasClass('active')){
                event.preventDefault();

                if(event.action === 'down') {
                    mouseHandling.button = event.current_button;
                    mouseHandling.lastX = event.clientX;
                    mouseHandling.lastY = event.clientY;
                } else if (event.action === 'up') {
                    mouseHandling.button = null;
                } else if (event.action === 'move' && mouseHandling.button != null) {
                    var newX = event.clientX,
                    newY = event.clientY,
                    deltaX = newX - mouseHandling.lastMouseX,
                    deltaY = newY - mouseHandling.lastMouseY,
                    rX, rY, mx, my, z, pan;

                    if (mouseHandling.button == 0){
                        rX = deltaX/50.0;
                        rY = deltaY/50.0;
                        mx = mat4.create();
                        mat4.identity(mx);
                        mat4.rotate(mx, rX, [0, 1, 0]);
                        my = mat4.create();
                        mat4.identity(my);
                        mat4.rotate(my, rY, [1, 0, 0]);
                        mat4.multiply(mx, my, mx);
                        mat4.multiply(mx, renderingContext.transform.rotation, renderingContext.transform.rotation);
                    } else if (event.button == 1){
                        z = Math.abs(sceneJSON.Renderers[0].LookAt[9]-sceneJSON.Renderers[0].LookAt[3]);
                        pan = z/renderingContext.transform.scale;
                        this.translation[0] += pan*deltaX/1500.0;
                        this.translation[1] -= pan*deltaY/1500.0;
                    } else if (event.button == 2){
                    //renderingContext.transform.scale += renderingContext.transform.scale*(deltaY)/200.0;
                    } else {
                    //renderingContext.transform.scale += renderingContext.transform.scale*(deltaY)/200.0;
                    }
                    mouseHandling.lastX = newX;
                    mouseHandling.lastX = newY;
                    drawScene();
                }
            }
        }).bind('active', function(){
            if(renderer.hasClass('active')){
                // Setup GL context
                gl.viewportWidth = renderer.width();
                gl.viewportHeight = renderer.height();

                gl.clearColor(0.0, 0.0, 0.0, 1.0);
                gl.clearDepth(1.0);
                gl.enable(gl.DEPTH_TEST);
                gl.depthFunc(gl.LEQUAL);

                gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

                initializeShader(gl, shaderProgram, pointShaderProgram);

                // Ready to render data
                fetchScene();
                updateCamera();
                drawScene();
            }
        });
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
    if (GLOBAL.WebGLRenderingContext && typeof(vec3) != "undefined" && typeof(mat4) != "undefined") {
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