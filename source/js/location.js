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
