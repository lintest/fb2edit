(function(root) {
    var selection = document.getSelection();
    var anchorNode = selection.anchorNode;
    var focusNode = selection.focusNode;
    var f = function(node) {
        if (node.nodeName === "#text") {
            handler.onTxt(node.data);
            if (anchorNode === node) handler.onAnchor(selection.anchorOffset);
            if (focusNode === node) handler.onFocus(selection.focusOffset);
        } else if (node.nodeName === "#comment") {
            handler.onCom(node.data);
        } else {
            var atts = node.attributes;
            var count = atts.length;
            for (var i = 0; i < count; ++i) handler.onAttr(atts[i].name, atts[i].value);
            handler.onNew(node.nodeName);
            for (var n = node.firstChild; n !== null; n = n.nextSibling) f(n);
            handler.onEnd(node.nodeName);
        }
    }
    handler.onNew(root.nodeName);
    for (var n = root.firstChild; n !== null; n = n.nextSibling) f(n);
    handler.onEnd(root.nodeName);
})(document);
