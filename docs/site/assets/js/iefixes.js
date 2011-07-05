// run jQuery in no conflict mode to prevent from conflicts with another libraries that use $ dollar symbol
jQuery.noConflict();

// This function will be called by content slider when necessarry. It somehow fixes text rendering for IE.
(function($){
	$.fn.fixTextFilter = function(){
		return this.each(function(){
			this.style.removeAttribute('filter');
			var bg = $(this).parents('.inner-wrap').parent().css('background-color');
		});
	}
})(jQuery);