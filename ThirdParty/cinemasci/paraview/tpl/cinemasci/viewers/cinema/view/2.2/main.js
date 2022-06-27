var emptyImage   = 'cinema/lib/2.2/img/empty.png';
var databaseList = 'cinema/view/2.2/databases.json';
var databaseListType = 'json'
var AssetName = "FILE"

// first, check for an overwrite to the AssetName 
loadAttributes(function(response) 
{
    if (response != "") {
        jsondata = JSON.parse(response);
        if ("assetname" in jsondata) {
            AssetName = jsondata.assetname
        }
    }
    // note that others could be passed from the server
    // at present, they aren't, to simplify testing
});

// override attributes from a local URL, if present
var url = window.location.href;
var urlArgs = url.split('?');

if (urlArgs.length > 1) {
    var urlArgPairs = urlArgs[1].split('&');

    // go through all the pairs, and if you find something you expect, deal with it
    for (var i in urlArgPairs) {
        var kvpair = urlArgPairs[i].split('=');

        // now look for the values you expect, and do something with them
        //
        // Note that this will not properly handle the case in which both parameters
        // are set by url attributes 
        if (kvpair[0] == 'databases') {
            databaseList = kvpair[1];
        } else if (kvpair[0] == 'assetname') {
            AssetName = kvpair[1];
        }
    }
}
// end override attributes

function loadAttributes(callback)
{
    var xobj = new XMLHttpRequest();
    xobj.overrideMimeType("application/json");
    xobj.open('GET', "cinema/view/2.2/cinema_attributes.json", true); // Replace 'my_data' with the path to your file
    xobj.onreadystatechange = function () {
        if (xobj.readyState == 4 && (xobj.status == 0 || xobj.status == 200)) {
            // Required use of an anonymous callback as .open will NOT return a value but simply returns undefined in asynchronous mode
            callback(xobj.responseText);
        }
    };
    xobj.send(null);
}

function loadData(callback) 
{
    var xobj = new XMLHttpRequest();
    xobj.overrideMimeType("application/json");
    xobj.open('GET', databaseList, true); // Replace 'my_data' with the path to your file
    xobj.onreadystatechange = function () {
        if (xobj.readyState == 4 && (xobj.status == 0 || xobj.status == 200)) {
            // Required use of an anonymous callback as .open will NOT return a value but simply returns undefined in asynchronous mode
            callback(xobj.responseText);
        }
    };
    xobj.send(null);
}


function onlyUnique(value, index, self) 
{
    return self.indexOf(value) === index;
}

function getLookupKey(keys, query) 
{
    var key = "";
    keys.forEach(function(item, index) {
        key += query[item] + "_";
    });

    return key;
}


// Initialize Dropdown list from JSON
var actual_JSON;

databaseListType = db_get_database_list_type( databaseList )
if (databaseListType == "json") {
    loadData(function(response) 
    {
        // Parse JSON string into object
        actual_JSON = JSON.parse(response);

        var dbs = [];
        for (i = 0; i < actual_JSON.length; i++) 
        {
            dbs.push( actual_JSON[i].database_name );
        }

        var sel = document.getElementById('dbList');
        for(var i=0; i <actual_JSON.length; i++) {
            var opt = document.createElement('option');
            opt.innerHTML = dbs[i];
            opt.value = i;
            sel.appendChild(opt);
        }

        // Fill in DB
        optionSelected();
    });
} else {
    // convert a database into a json data structure
    actual_JSON = db_create_view_database_list( databaseList );

    var dbs = [];
    for (i = 0; i < actual_JSON.length; i++) 
    {
        dbs.push( actual_JSON[i].database_name );
    }

    var sel = document.getElementById('dbList');
    for(var i=0; i <actual_JSON.length; i++) {
        var opt = document.createElement('option');
        opt.innerHTML = dbs[i];
        opt.value = i;
        sel.appendChild(opt);
    }

    // Fill in DB
    optionSelected();
}


function optionSelected()
{
    // Clear image area
    var div = document.getElementById("imageArea"); 
    while(div.firstChild) { 
        div.removeChild(div.firstChild); 
    } 


    // Clear slider area
    var div = document.getElementById("sliderContainer"); 
    while(div.firstChild) { 
        div.removeChild(div.firstChild); 
    } 

    
    // Get selected option
    var e = document.getElementById("dbList");
    var value = e.options[e.selectedIndex].value;
    var text = e.options[e.selectedIndex].text;
    

    // Parse the json
    var datasettoLoad = [];
    var datasetNamestoLoad = [];
    for (i=0; i<actual_JSON.length; i++) 
    {   
        if (i == parseInt(value))
        {
            for (j=0; j<actual_JSON[i].datasets.length; j++) 
            {
                datasettoLoad.push( actual_JSON[i].datasets[j].location );
                datasetNamestoLoad.push( actual_JSON[i].datasets[j].name );
            }
        }
    }

    var dataSets = datasettoLoad;
    // START: add code to manage databases to view
    // END:   add code to manage databases to view

    var dataResults = null;
    var dataValues = null;
    var query = {};
    var images = [];
    var numImages = 0;

    var q = d3.queue();
    dataSets.forEach(function(item, index) {
        q.defer(d3.csv, item + "/data.csv");
    });
    q.awaitAll(function(error, results) {
        var values = {};
        results.forEach(function(item, index) {
            var keys = Object.keys(item[0]).filter(function(d) {
                return !isNaN(+item[0][d]);
            });

            var lookup = {};

            item.forEach(function(result, index) {
                keys.forEach(function(key, index) {
                    if (!(key in values)) {
                        values[key] = [];
                    }

                    values[key].push(result[key]);
                });

                lookup[getLookupKey(keys, result)] = result;
            })

            results[index] = {keys: keys, results: results[index], lookup: lookup};
        });

        var keys = Object.keys(values);
        keys.forEach(function(key, index) {
            values[key] = values[key].filter(onlyUnique);
            values[key] = values[key].sort(function(a, b){return (+a)-(+b)});
        });

        doneLoading(results, values);
    });

    function updateResults() 
    {
        // Called when sliders are moved
        dataResults.forEach(function(result, index) {
            var imgSrcKey = getLookupKey(result.keys, query);
            if (imgSrcKey in result.lookup) {
                var imgSrc = dataSets[index] + "/" +result.lookup[imgSrcKey][AssetName];
                images[index].img.attr("src", imgSrc);
            }
            else {
                images[index].img.attr("src", emptyImage);
            }
        });
    }

    function doneLoading(results, values) 
    {
        var imageSizeSlider = d3.select("#imageSize")
            .property("value", (100/dataSets.length)-1)
            .on("input", function() {
                for (i = 0; i < numImages; i++) {
                    var selectedItem = "#image_div_" + i.toString();
                    d3.select(selectedItem).style("width","" + imageSizeSlider.node().value + "%")
                }
            });

        dataResults = results;
        dataValues = values;

        var keys = Object.keys(values);
        keys.forEach(function(key, index) {
            var sliderDiv = d3.select("#sliderContainer").append("div");
            sliderDiv.append("div")
                .html(key)
                .style("padding", "5px");
            var slider = sliderDiv.append("div")
                .append("input")
                    .attr("type","range")
                    .attr("class","slidecontainer")
                    .attr("min", "0")
                    .attr("max", values[key].length - 1)
                    .property("value", "0")
                    .style("float", "left");
            var sliderText = sliderDiv.append("input")
                    .attr("type","text")
                    .attr("value", values[key][0])
                    .style("width", "40px")
                    .on("input", function() {
                        query[key] = this.value;
                        var index = values[key].indexOf(this.value);
                        slider.property("value", index);
                        updateResults();
                    });

            slider.on("input", function() {
                    var val = values[key][+this.value];
                    sliderText.property("value", val);
                    query[key] = val;
                    updateResults();
                });

            query[key] = values[key][0];
        });


        dataResults.forEach(function(result, index) {
            console.log("index:", index);
            console.log("val:",imageSizeSlider.node().value);
            numImages = numImages + 1
            var imgSrcKey = getLookupKey(result.keys, query);

            var img = d3.select("#imageArea")
                .append("div")
                    .attr("id", "image_div_" + index.toString())
                    .style("width","" + imageSizeSlider.node().value + "%")
                    .style("text-align","center")
                    .style("margin","5px")
                    .text(datasetNamestoLoad[index])

                .append("img")
                    .attr("class", "cinemaImage")
                    .attr("src", emptyImage)
                    .style("width","100%")

                    .on("load", function() {
                          if (this.src != emptyImage) { 
                            images[index].height = this.height;
                        }
                    })
                    .on("error", function() {
                        this.src=emptyImage;
                        this.height = images[index].height;
                    });

            images.push({img: img});

            if (imgSrcKey in result.lookup) {
                var imgSrc = dataSets[index] + "/" + result.lookup[imgSrcKey][AssetName];
                images[index].img.attr("src", imgSrc);
            }
            else {
                images[index].img.attr("src", emptyImage);
            }
        });
    }
}
