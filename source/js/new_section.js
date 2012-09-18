(function() {
   if(window.getSelection().rangeCount){ 
      var selection = window.getSelection()
      range = selection.getRangeAt(0)
      
      start = range.startContainer
      end = range.endContainer
      root = range.commonAncestorContainer

      if(start.nodeName.toLowerCase() == "body") return null
      if(start.nodeName == "#text") start = start.parentNode
      if(end.nodeName == "#text") end = end.parentNode

      if(start == end) root = start

      var range = document.createRange();
      range.setStartBefore(start);
      range.setEndAfter(end);

      var newNode = document.createElement("div");
      newNode.className = "section";
      range.surroundContents(newNode);
      
      return newNode;
    }
})()
