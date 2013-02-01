(function(GLOBAL,$){
    var REPRESENTATIONS = [
    'images/representations/hide-active.png', 
    'images/representations/outline-active.png',
    'images/representations/wireframe-active.png',
    'images/representations/surface-active.png',
    'images/representations/surfaceEdge-active.png'], 
    FILTER_ICONS = [
    'images/filters/server.png',
    'images/filters/Dataset.png',
    'images/filters/Clip.png',
    'images/filters/IsoSurface.png',
    'images/filters/Slice.png',
    'images/filters/StreamTracer.png',
    'images/filters/Threshold.png'
    ],
    FILTER_CLASS = [
    'server',
    'dataset',
    'clip',
    'iso',
    'slice',
    'stream',
    'threshold'
    ], pipeline = [ {
        data: "kitware.com",
        metadata:{
            id: 0,
            name: "kitware.com",
            icon: 0
        },
        children: [
        {
            data: "Can.ex2",
            state : "open",
            metadata: {
                id: 1234, 
                name: "Can.ex2",
                rep: 0,
                icon: 1,
                selectedArray: 'RTData',
                arrays: ['A', 'B', 'C', 'RTData'],
                bar: false
            }, 
            children: [ {
                data: "Clip",
                state : "open",
                metadata: {
                    id: 234, 
                    name: 'Clip',
                    rep: 1,
                    icon: 1,
                    selectedArray: 'RTData',
                    arrays: ['A', 'B', 'C', 'RTData'],
                    bar: false
                }, 
                children:[{
                    data:"Iso", 
                    state : "open",
                    metadata: {
                        id: 34, 
                        name: "Iso",
                        rep: 2,
                        icon: 2,
                        selectedArray: 'A',
                        arrays: ['A', 'B', 'C', 'RTData'],
                        bar: false
                    }
                }]
            }, {
                data: "Slice", 
                metadata: {
                    id: 4,
                    name: 'Slice',
                    rep: 3,
                    icon: 3,
                    selectedArray: 'C',
                    arrays: ['A', 'B', 'C', 'RTData'],
                    bar: false
                }
            }, {
                data:"Stream Tracer",
                metadata: {
                    id: 5, 
                    name:'Stream Tracer',
                    rep: 4,
                    icon: 4,
                    selectedArray: 'B',
                    arrays: ['A', 'B', 'C', 'RTData'],
                    bar: true
                }
            }, {
                data:"Threshold", 
                metadata: {
                    id: 6, 
                    name:'Threshold',
                    rep: 4,
                    icon: 5,
                    selectedArray: 'RTData',
                    arrays: ['A', 'B', 'C', 'RTData'],
                    bar: false
                }
            }]
        }]
        }],html = [], h = -1;
    
    
    function initialize() {
        $('.cancel').hide();
        $('.create-source-panel').hide();
        $('.itemPanel').hide();
        $('.root').show();
        $(".item").click(function(){
            var me = $(this); 
            if(me.attr("submenu")) {
                other = $("." + me.attr("submenu"));
                moveTo(me.parent(), other)
            }
        });
        $(".scalarbar").click(function(){
            //            var me = $(this); 
            //            me.toggleClass("active");
            var proxy = getProxy(Number($(".filter.name").attr("id")));
            proxy.metadata.bar = !proxy.metadata.bar;

            updateSelectedProxy(proxy);
        });
            
        $('.create').click(function(){
            $('.create').hide();
            $('.pipeline-tree').hide("slide",{}, 250, function(){
                $('.create-source-panel').show("slide",{}, 250);
                $('.cancel').show();
            });
                
                
        });
        $('.cancel').click(function(){
            $('.cancel').hide();
            $('.create-source-panel').hide("slide",{}, 250, function(){
                $('.pipeline-tree').show("slide",{}, 250);
                $('.create').show();
            });
        });
            
        function moveTo(before, after, direction) {
            var options = {};
            before.hide("slide", options, 250, function(){
                after.show("slide", options, 250);
            });
        }
            
        $(".toggle-control").click(function(){
            var controlPanel = $('.control-panel');
            if(controlPanel.hasClass("hide")) {
                controlPanel.show('slide',{}, 250, function(){
                    controlPanel.toggleClass("hide");
                });
            } else {
                controlPanel.hide('slide',{}, 250, function(){
                    controlPanel.toggleClass("hide");
                });
            }
        });
    }
    
    function updateFiler (selectedObject) {
        activeObject = selectedObject;
        var proxy = getProxy(selectedObject.data('id'));
        updateSelectedProxy(proxy);
    }
    
    function updateSelectedProxy (proxy) {
        $('.filter.representation').attr("src", REPRESENTATIONS[proxy.metadata.rep]);
        $('.filter.icon').attr("src", FILTER_ICONS[proxy.metadata.icon]);
        $('.filter.name').txt(proxy.metadata.name).attr('id', proxy.metadata.id);
        var arrays = proxy.metadata.arrays;
        var selector = $('.filter.array').empty();
        for(i in arrays) {
            $("<option></option>").text(arrays[i]).appendTo(selector);
        }
        $('.filter.array').val(proxy.metadata.selectedArray);
        if(proxy.metadata.bar) {
            $('.filter.scalarbar').addClass("active");            
        } else {
            $('.filter.scalarbar').removeClass("active");  
        }
    }
    
    function getProxy(id) {
        var result = findNode(id, pipeline);
        if(result.length === 1) {
            return result[0];
        }
        return null;
    }
    
    function findNode(id, list) {
        var result = []
        for(idx in list) {
            node = list[idx];
            if(node.metadata.id === id) {
                return [node];
            }
            if(node.hasOwnProperty("children")) {
                result = result.concat(findNode(id, node.children));
            }
        }
        return result;
    }
    
    
    function proxyToPipelineLine(proxy) {
        var htmlBuffer = [], idx = -1;
        if(proxy.metadata.id === 0) {
            return  proxy.metadata.name;  
        }
        htmlBuffer[++idx] = "<div class='proxy proxy-id-";
        htmlBuffer[++idx] = proxy.metadata.id;
        htmlBuffer[++idx] = "'>";
        htmlBuffer[++idx] = proxy.metadata.name
        htmlBuffer[++idx] = "<span class='proxy-control'>";
        htmlBuffer[++idx] = "<img class='representation' src='";
        htmlBuffer[++idx] = REPRESENTATIONS[proxy.metadata.rep]
        htmlBuffer[++idx] = "'/>";
        htmlBuffer[++idx] = "<select><option>A</option><option>B</option><option>C</option></select>";
        htmlBuffer[++idx] = "<img src='images/pqScalarBar16.png' class='scalarbar ";
        htmlBuffer[++idx] = (proxy.metadata.bar) ? "active" : "";
        htmlBuffer[++idx] = "'>";
        htmlBuffer[++idx] = "</span></div>";
        return htmlBuffer.join('');
    }
    
    function appendProxy(proxy, lastChild, root) {
        html[++h] = "<li class='";
        html[++h] = FILTER_CLASS[proxy.metadata.icon];
        if(lastChild) {
            html[++h] = " lastChild"
        } else if (root === false && proxy.hasOwnProperty("children") && proxy.children.length > 0) {
            html[++h] = " open"
        }
        html[++h] = "'>" ;
        html[++h] = proxyToPipelineLine(proxy);
        if(proxy.hasOwnProperty("children")) {
            html[++h] = "<ul>";
            console.log("Children of " + proxy.metadata.name + " " + proxy.children.length);
            var len = Number(proxy.children.length) - 1;
            for(idx in proxy.children) {
                console.log(proxy.children[idx].metadata.name + " " + idx + " " +len);
                appendProxy(proxy.children[idx], idx == len, false);
            }
            html[++h] = "</ul>";
        }
        html[++h] = "</li>";
    }
    
    
    function updatePipeline(pipeline) {
        html = [];
        h = -1;
        appendProxy(pipeline[0], false, true);
        
        var tree = $('.pipeline-tree')[0];
        tree.innerHTML = html.join('');
        
        $('.scalarbar').click(function(){
            var me = $(this);
            me.toggleClass('active');
        });
        
        function selectRepresentation() {
            this.src = 'images/representations/all.png';
            $(this).unbind('click').bind('click', chooseRepresentation);
        }
        
        function chooseRepresentation(e) {
            var me = $(this);
            var pos = (e.pageX - me.offset().left)/20;
            console.log(Math.floor(pos));
            this.src = REPRESENTATIONS[Math.floor(pos)]; 
            me.unbind('click').bind('click', selectRepresentation);
        }
        
        $('.representation').bind('click', selectRepresentation);
        
        $(".proxy").click(function(){
            $(".proxy").removeClass('active');
            $(".proxy-control").removeClass('active');
            $('.proxy-control',this).addClass('active').parent().addClass('active');
        });
    }

    initialize();
    updatePipeline(pipeline);
    
})(window, jQuery);