var insert = function(parent) {
    var child = document.createElement('DIV');
    child.setAttribute("class", 'title');
    child.innerText = "title";
    parent.insertAdjacentElement("afterBegin",child);
    return child;
}

var section = function(parent) {
    if (parent.nodeName !== 'DIV') return 0;
    var attr = parent.getAttribute('CLASS').toLowerCase();
    if (attr === 'section') return 1;
    return 0;
}

var exists = function(parent) {
    var child = parent.firstChild;
    if (child === undefined) return 0;
    if (child.nodeName !== 'DIV') return 0;
    if (child.getAttribute('CLASS').toLowerCase() !== 'title') return 0;
    return 1;
}

var parent = document.getSelection().baseNode;

while (parent.nodeName !== 'BODY') {
  if (parent.nodeName === 'DIV') {
      if (!section(parent)) break;
      if (exists(parent)) break;
      insert(parent);
      break;
  }
  parent = parent.parentNode;
}
