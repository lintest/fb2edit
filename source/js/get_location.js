// create a reverse function for jQuery
$.fn.reverse = [].reverse; 
var hierarchy = $(element).parents().reverse().add($(element));
hierarchy.map(function () { 
    var parent = $(this).parent();
    if (undefined !== parent.get( 0 ).tagName) {
        return parent.get(0).tagName + "=" + parent.children().index(this);
    }
}).get().join(",");
