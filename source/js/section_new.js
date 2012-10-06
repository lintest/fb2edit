f=function(elem,tag,start,end){
start=$(elem).children().get(start);
end=$(elem).children().get(end);
var range=document.createRange();
range.setStartBefore(start);
range.setEndAfter(end);
var newNode=document.createElement(tag);
range.surroundContents(newNode);
range.setStartBefore(start);
range.setEndBefore(start);
var selection=window.getSelection();
selection.removeAllRanges();
selection.addRange(range);
};
