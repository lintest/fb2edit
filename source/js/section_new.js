f=function(elem,start, end){
start=$(elem).children().get(start);
end=$(elem).children().get(end);
var range=document.createRange();
range.setStartBefore(start);
range.setEndAfter(end);
var newNode=document.createElement("div");
newNode.className="section";
range.surroundContents(newNode);
range.setEndBefore(start);
var selection=window.getSelection();
selection.removeAllRanges();
selection.addRange(range);
};
