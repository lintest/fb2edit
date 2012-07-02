var startNode = document.getSelection().anchorNode;
// create a reverse function for jQuery
$.fn.reverse = [].reverse; 
var hierarchy = $( startNode ).parents().reverse().add( $( startNode ) ) ;
hierarchy.map(function () { 
    if ( undefined !== $( this ).parent().get( 0 ).tagName )
	{
		var first_part = $( this ).parent().get( 0 ).tagName + "=";
		return first_part + jQuery.inArray( this, $( this ).parent().contents() );
	}
}).get().join(",");
