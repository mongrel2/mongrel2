function toggleBranches() {
  if (!document.styleSheets) return;
  var rules = new Array();
  if (document.styleSheets[0].cssRules)
    rules = document.styleSheets[0].cssRules;
  else if (document.styleSheets[0].rules)
    rules = document.styleSheets[0].rules;

  for (var i=0; i<rules.length; ++i) {
    if (rules[i].selectorText == "span.branchGroup") {
      var s = rules[i].style;
      s.display = s.display != "" ? "" : "none";
    }
  }
}

function toggleBranchesKeyHandler(e) {
    if (document.all)    { e = window.event; }
    if (e.which == 98 || e.keyCode == 98)
        toggleBranches();
}

document.onkeypress = toggleBranchesKeyHandler;
