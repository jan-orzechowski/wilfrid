Module["print"] = function (text) { print_to_output(text); };
Module["onRuntimeInitialized"] = function () { Module.callMain(); }