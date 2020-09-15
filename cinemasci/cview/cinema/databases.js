
//
// inspect a string to determine what type it contains 
//
function db_get_database_list_type( dbs ) {
    result = ""

    dbs = dbs.trim()

    ext = db_get_file_extension( dbs )

    if (ext == "json") {
        result = "json"
    } else if (ext == "cdb") {
        result = "cdb"
    } else {
        result = "unknown"
        // TODO error if it's neither type
    }

    return result
}

//
// get the file name from a path string
//
function db_get_file_name(filename) {
    nameArray = filename.split('/');
    return nameArray[nameArray.length - 1];
}

//
// get the file extension from a path string
//
function db_get_file_extension(filename)
{
  var ext = /^.+\.([^.]+)$/.exec(filename);
  return ext == null ? "" : ext[1];
}

//
// assuming that we have been passed a list of cinema database names
// construct the proper data structures
//
function db_create_view_database_list( dbstring ) {
    dbs = dbstring.split(",")
    cdbname = db_get_file_name(dbs[0])
    json_string = `[{ "database_name": "${cdbname}", "datasets": [ `

    dbs.forEach(function (item) {
        cdbname = db_get_file_name(item)
        json_string += `{ "name" : "${cdbname}", "location" : "${item}" },`
    })

    // remove the last comma
    json_string = json_string.substring(0, json_string.length - 1);
    // close the string
    json_string += "]}]"
    return JSON.parse( json_string )
}


//
// assuming that we have been passed a list of cinema database names
// construct the proper data structures
//
function db_create_explorer_database_list( dbstring ) {
    dbs = dbstring.split(",")
    json_string = "["
    dbs.forEach(function (item, index) {
        cdbname = db_get_file_name(item)
        json_string += `{ "name" : "${cdbname}", "directory" : "${item}" },`
    })
    // remove the last comma
    json_string = json_string.substring(0, json_string.length - 1);
    // close the string
    json_string += "]"
    return JSON.parse( json_string )
}
