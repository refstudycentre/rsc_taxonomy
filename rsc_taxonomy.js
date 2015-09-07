// JavaScript should be made compatible with libraries other than jQuery by
// wrapping it with an "anonymous closure". See:
// - https://drupal.org/node/1446420
// - http://www.adequatelygood.com/2010/3/JavaScript-Module-Pattern-In-Depth
// To understand behaviors, see https://drupal.org/node/756722#behaviors
(function ($, Drupal, window, document, undefined) {

Drupal.behaviors.rsc_taxonomy = {
  attach: function(context, settings) {

    // find the library menu on the page
    var div = $("#block-rsc-taxonomy-library-menu > div.content");
    div.html("Loading ...");

    // check if localstorage is supported by this browser
    var support = false;
    try {
      if ('localStorage' in window && window['localStorage'] !== null) {
        support = true
      }
    } catch (e) {}

    // if localstorage is supported, try to load the menu from the localstorage
    if (support) {
      
      var markup = localStorage["rsc_taxonomy_library_menu_markup"];
      var modified = localStorage["rsc_taxonomy_library_menu_modified"];

      if (markup && modified && (div.attr("modified") == modified)) {
        // alert("cache hit");
      } else {
        // alert("cache miss");

        // request the menu from the server using ajax
        $.ajax({
          url: "/rsc_taxonomy_library_menu",
          type: 'get',
          dataType: 'html',
          async: false,
          success: function(data) {
              markup = data;
          } 
        });

        // save the menu to localstorage for next time
        localStorage["rsc_taxonomy_library_menu_markup"] = markup;
        localStorage["rsc_taxonomy_library_menu_modified"] = div.attr("modified");

      }

      // add the menu to the DOM
      div.html(markup);
      
    } else {
      // localstorage is not supported. Just get the menu from the server.
      div.load("/rsc_taxonomy_library_menu");
    }
      
  }
};


})(jQuery, Drupal, this, this.document);
