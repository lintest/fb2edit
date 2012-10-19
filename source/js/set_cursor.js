window.scrollTo(0,this.offsetTop);
var range = document.createRange();
range.setStart(this,0);
range.setEnd(this,0);
var selection = window.getSelection();
selection.removeAllRanges();
selection.addRange(range);
range.collapse(true);
range.select();
