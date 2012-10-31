(f = function(node) {
    if (node.nodeName === "#text") {
        handler.onTxt(node.data);
    } else if (node.nodeName === "#comment") {
        handler.onCom(node.data);
    } else {
        var atts = node.attributes;
        var count = atts.length;
        for (var i = 0; i < count; i++) handler.onAttr(atts[i].name, atts[i].value);
        handler.onNew(node.nodeName);
        for (var n = node.firstChild; n !== null; n = n.nextSibling) f(n);
        handler.onEnd(node.nodeName);
    }
})(this);
