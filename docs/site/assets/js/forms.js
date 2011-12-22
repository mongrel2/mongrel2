/*
 * HTML5 Forms Chapter JavaScript Library
 * http://thecssninja.com/javascript/H5F
 *
 * Copyright (c) 2010 Ryan Seddon - http://thecssninja.com/
 * Dual-licensed under the BSD and MIT licenses.
 * http://thecssninja.com/H5F/license.txt
 */

var H5F = H5F || {};

(function(d){
    
    var field = d.createElement("input"),
        emailPatt = new RegExp("^[a-z0-9_.%+-]+@[0-9a-z.-]+\\.[a-z.]{2,6}$","i"), 
        urlPatt = new RegExp("[a-z][-\.+a-z]*:\/\/","i"),
        nodes = new RegExp("^(input|select|textarea)$","i"),
        usrPatt, curEvt, args, custMsg = "";
    
    H5F.setup = function(form,settings) {
        var isCollection = !form.nodeType || false;
        
        var opts = {
            validClass : "valid",
            invalidClass : "error",
            requiredClass : "required",
            placeholderClass : "placeholder"
        };

        if(typeof settings == "object") {
            for (var i in opts) {
                if(typeof settings[i] == "undefined") { settings[i] = opts[i]; }
            }
        }
        
        args = settings || opts;
        
        if(isCollection) {
            for(var k=0,len=form.length;k<len;k++) {
                H5F.validation(form[k]);
            }
        } else {
            H5F.validation(form);
        }
    };
    
    H5F.validation = function(form) {
        var f = form.elements,
            flen = f.length,
            isRequired;
        
        H5F.listen(form,"invalid",H5F.checkField,true);
        H5F.listen(form,"blur",H5F.checkField,true);
        H5F.listen(form,"input",H5F.checkField,true);
        H5F.listen(form,"keyup",H5F.checkField,true);
        H5F.listen(form,"focus",H5F.checkField,true);
        
        if(!H5F.support()) { 
            form.checkValidity = function() { return H5F.checkValidity(form); };
            
            while(flen--) {
                isRequired = !!(f[flen].attributes["required"]);
                // Firefox includes fieldsets inside elements nodelist so we filter it out.
                if(f[flen].nodeName !== "FIELDSET") {
                    H5F.validity(f[flen]); // Add validity object to field
                }
            }
        }
    };
    H5F.validity = function(el) {
        var elem = el,
            missing = H5F.valueMissing(elem),
            type = elem.getAttribute("type"),
            pattern = elem.getAttribute("pattern"),
            placeholder = elem.getAttribute("placeholder"),
            isType = /^(email|url)$/i,
            evt = /^(input|keyup)$/i,
            fType = ((isType.test(type)) ? type : ((pattern) ? pattern : false)),
            patt = H5F.pattern(elem,fType),
            step = H5F.range(elem,"step"),
            min = H5F.range(elem,"min"),
            max = H5F.range(elem,"max"),
            customError = (custMsg !== "");
        
        elem.checkValidity = function() { return H5F.checkValidity(elem); };
        elem.setCustomValidity = function(msg) { H5F.setCustomValidity.call(elem,msg); };
        elem.validationMessage = custMsg;
        
        elem.validity = {
            valueMissing: missing,
            patternMismatch: patt,
            rangeUnderflow: min,
            rangeOverflow: max,
            stepMismatch: step,
            customError: customError,
            valid: (!missing && !patt && !step && !min && !max && !customError)
        };
        
        if(placeholder && !evt.test(curEvt)) { H5F.placeholder(elem); }
    };
    H5F.checkField = function (e) {
        var el = H5F.getTarget(e) || e, // checkValidity method passes element not event
            events = /^(input|keyup|focusin|focus)$/i,
            ignoredTypes = /^(submit|image|button|reset)$/i,
            checkForm = true;
        
        if(nodes.test(el.nodeName) && !(ignoredTypes.test(el.type) || ignoredTypes.test(el.nodeName))) {
            curEvt = e.type;
            if(!H5F.support()) { H5F.validity(el); }
            
            if(el.validity.valid) {
                H5F.removeClass(el,[args.invalidClass,args.requiredClass]);
                H5F.addClass(el,args.validClass);
            } else if(!events.test(curEvt)) {
                if(el.validity.valueMissing) {
                    H5F.removeClass(el,[args.invalidClass,args.validClass]);
                    H5F.addClass(el,args.requiredClass);
                } else {
                    H5F.removeClass(el,[args.validClass,args.requiredClass]);
                    H5F.addClass(el,args.invalidClass);
                }
            } else if(el.validity.valueMissing) {
                H5F.removeClass(el,[args.requiredClass,args.invalidClass,args.validClass]);
            }
            if(curEvt === "input" && checkForm) {
                // If input is triggered remove the keyup event
                H5F.unlisten(el.form,"keyup",H5F.checkField,true);
                checkForm = false;
            }
        }
    };
    H5F.checkValidity = function (el) {
        var f, ff, isRequired, hasPattern, invalid = false;
        
        if(el.nodeName === "FORM") {
            f = el.elements;
            
            for(var i = 0,len = f.length;i < len;i++) {
                ff = f[i];
                
                isRequired = !!(ff.attributes["required"]);
                hasPattern = !!(ff.attributes["pattern"]);
                
                if(ff.nodeName !== "FIELDSET" && (isRequired || hasPattern)) {
                    H5F.checkField(ff);
                    if(!ff.validity.valid && !invalid) {
                        ff.focus();
                        invalid = true;
                    }
                }
            }
            return !invalid;
        } else {
            H5F.checkField(el);
            return el.validity.valid;
        }
    };
    H5F.setCustomValidity = function (msg) {
        var el = this;
            custMsg = msg;
            
        el.validationMessage = custMsg;
    };
    
    H5F.support = function() {
        return (H5F.isHostMethod(field,"validity") && H5F.isHostMethod(field,"checkValidity"));
    };

    // Create helper methods to emulate attributes in older browsers
    H5F.pattern = function(el, type) {
        if(type === "email") {
            return !emailPatt.test(el.value);
        } else if(type === "url") {
            return !urlPatt.test(el.value);
        } else if(!type) {
            return false;
        } else {
            var placeholder = el.getAttribute("placeholder"),
                val = el.value;
            
            usrPatt = new RegExp('^(?:' + type + ')$');
            
            if(val === placeholder) {    
                return true;
            } else if(val === "") {
                return false;
            } else {
                return !usrPatt.test(el.value);
            }
        }
    };
    H5F.placeholder = function(el) {
        var placeholder = el.getAttribute("placeholder"),
            focus = /^(focus|focusin|submit)$/i,
            node = /^(input|textarea)$/i,
            ignoredType = /^password$/i,
            isNative = !!("placeholder" in field);
        
        if(!isNative && node.test(el.nodeName) && !ignoredType.test(el.type)) {
            if(el.value === "" && !focus.test(curEvt)) {
                el.value = placeholder;
                H5F.listen(el.form,'submit', function () {
                  curEvt = 'submit';
                  H5F.placeholder(el);
                }, true);
                H5F.addClass(el,args.placeholderClass);
            } else if(el.value === placeholder && focus.test(curEvt)) {
                el.value = "";
                H5F.removeClass(el,args.placeholderClass);
            }
        }
    };
    H5F.range = function(el,type) {
        // Emulate min, max and step
        var min = parseInt(el.getAttribute("min"),10) || 0,
            max = parseInt(el.getAttribute("max"),10) || false,
            step = parseInt(el.getAttribute("step"),10) || 1,
            val = parseInt(el.value,10),
            mismatch = (val-min)%step;
        
        if(!H5F.valueMissing(el) && !isNaN(val)) {
            if(type === "step") {
                return (el.getAttribute("step")) ? (mismatch !== 0) : false;
            } else if(type === "min") {
                return (el.getAttribute("min")) ? (val < min) : false;
            } else if(type === "max") {
                return (el.getAttribute("max")) ? (val > max) : false;
            } 
        } else if(el.getAttribute("type") === "number") { 
            return true;
        } else {
            return false;
        }
    };
    H5F.required = function(el) {
        var required = !!(el.attributes["required"]);
        
        return (required) ? H5F.valueMissing(el) : false;
    };
    H5F.valueMissing = function(el) {
        var placeholder = el.getAttribute("placeholder"),
            isRequired = !!(el.attributes["required"]);
        return !!(isRequired && (el.value === "" || el.value === placeholder));
    };
    
    /* Util methods */
    H5F.listen = function (node,type,fn,capture) {
        if(H5F.isHostMethod(window,"addEventListener")) {
            /* FF & Other Browsers */
            node.addEventListener( type, fn, capture );
        } else if(H5F.isHostMethod(window,"attachEvent") && typeof window.event !== "undefined") {
            /* Internet Explorer way */
            if(type === "blur") {
                type = "focusout";
            } else if(type === "focus") {
                type = "focusin";
            }
            node.attachEvent( "on" + type, fn );
        }
    };
    H5F.unlisten = function (node,type,fn,capture) {
        if(H5F.isHostMethod(window,"removeEventListener")) {
            /* FF & Other Browsers */
            node.removeEventListener( type, fn, capture );
        } else if(H5F.isHostMethod(window,"detachEvent") && typeof window.event !== "undefined") {
            /* Internet Explorer way */
            node.detachEvent( "on" + type, fn );
        }
    };
    H5F.preventActions = function (evt) {
        evt = evt || window.event;
        
        if(evt.stopPropagation && evt.preventDefault) {
            evt.stopPropagation();
            evt.preventDefault();
        } else {
            evt.cancelBubble = true;
            evt.returnValue = false;
        }
    };
    H5F.getTarget = function (evt) {
        evt = evt || window.event;
        return evt.target || evt.srcElement;
    };
    H5F.addClass = function (e,c) {
        var re;
        if (!e.className) {
            e.className = c;
        }
        else {
            re = new RegExp('(^|\\s)' + c + '(\\s|$)');
            if (!re.test(e.className)) { e.className += ' ' + c; }
        }
    };
    H5F.removeClass = function (e,c) {
        var re, m, arr = (typeof c === "object") ? c.length : 1, len = arr;
        if (e.className) {
            if (e.className == c) {
                e.className = '';
            }
            else {        
                while(arr--) {
                    re = new RegExp('(^|\\s)' + ((len > 1) ? c[arr] : c) + '(\\s|$)');
                    m = e.className.match(re);
                    if (m && m.length == 3) { e.className = e.className.replace(re, (m[1] && m[2])?' ':''); }
                }
            }
        }
    };
    H5F.isHostMethod = function(o, m) {
        var t = typeof o[m], reFeaturedMethod = new RegExp('^function|object$', 'i');
        return !!((reFeaturedMethod.test(t) && o[m]) || t == 'unknown');
    };

})(document);