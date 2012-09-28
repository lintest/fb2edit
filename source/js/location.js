function location(e){
$.fn.reverse = [].reverse;
var h=$(e).parents().reverse().add($(e));
var f=function(){
 var p=$(this).parent();
 var t=p.get(0).tagName;
 if(t===undefined)return;
 return t+"="+p.children().index(this);
};
return h.map(f).get().join(",");
};

function locator(node){
if (node === undefined) return "undefined";
return (f = function(node){
	if (node.tagName === "HTML") return "$('html')";
	var child = $(node);
	var parent = child.parent();
    var prefix = f(node.parentNode);
	if (node.nodeName === "#text") {
        return prefix + ".contents()" + ".eq(" + parent.contents().index(node) + ")";
	} else {
        return prefix + ".children()" + ".eq(" + parent.children().index(node) + ")";
	}
})(node) + ".first()";
};
