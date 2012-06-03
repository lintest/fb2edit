(f = function(node) {
    if (node.nodeName === "#text") {
        handler.onTxt(node.data);
    } else {
        handler.onNew(node.nodeName);
        var atts = node.attributes;
        var count = atts.length;
        for (var i = 0; i < count; i++) handler.attr(atts[i].name, atts[i].value);
        for (var n = node.firstChild; n !== null; n = n.nextSibling) f(n);
        handler.onEnd(node.nodeName);
    }
})(document.body);
