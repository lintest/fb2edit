(f = function(node){
    var tag = node.tagName;
    if (tag === 'BODY') return '';
    if (tag === 'DIV') tag = node.getAttribute('CLASS');
    return f(node.parentNode) + '/' + tag;
})(document.getSelection().baseNode.parentNode);

