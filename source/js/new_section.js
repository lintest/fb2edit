(function(){
if(window.getSelection().rangeCount===0)return;
var selection=window.getSelection();
var range=selection.getRangeAt(0);
var root=range.commonAncestorContainer;
var start=range.startContainer;
var end=range.endContainer;
while (true) {
 if(root===null)return;
 tag=root.nodeName.toLowerCase();
 if(tag==="body")return;
 if(tag==="div"){
  type=root.className.toLowerCase();
  if(type==="body"||type==="section")break;
 }
 root = root.parentNode;
}
while(start.parentNode!==root) {
 if(start===null)return;
 start=start.parentNode;
}
while(end.parentNode!==root) {
 if(end===null)return;
 end=end.parentNode;
}
range=document.createRange();
range.setStartBefore(start);
range.setEndAfter(end);
var newNode=document.createElement("div");
newNode.className="section";
range.surroundContents(newNode);
range.setEndBefore(start);
selection.removeAllRanges();
selection.addRange(range);
})()
