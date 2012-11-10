var selection = document.getSelection();
var anchorNode = selection.anchorNode;
var focusNode = selection.focusNode;
(f = function(node) {
    if (node.nodeName === "#text") {
        if (anchorNode === node) handler.onAnchor(selection.anchorOffset);
        if (focusNode === node) handler.onFocus(selection.focusOffset);
        handler.onTxt(node.data);
    } else if (node.nodeName === "#comment") {
        handler.onCom(node.data);
    } else {
        if (node.nodeName !== "#document") {
            var atts = node.attributes;
            var count = atts.length;
            for (var i = 0; i < count; i++) handler.onAttr(atts[i].name, atts[i].value);
        }
        handler.onNew(node.nodeName);
        for (var n = node.firstChild; n !== null; n = n.nextSibling) f(n);
        handler.onEnd(node.nodeName);
    }
})(document);
