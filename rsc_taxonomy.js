// JavaScript should be made compatible with libraries other than jQuery by
// wrapping it with an "anonymous closure". See:
// - https://drupal.org/node/1446420
// - http://www.adequatelygood.com/2010/3/JavaScript-Module-Pattern-In-Depth
// To understand behaviors, see https://drupal.org/node/756722#behaviors
(function ($, Drupal, window, document, undefined) {

Drupal.behaviors.rsc_taxonomy = {
  attach: function(context, settings) {

    // find the menus on the page (assuming all blocks made by this module are menus)
    $(".block-rsc-taxonomy > div.content").each(function(index){

      // Get the current menu's outer div
      var menu_div = $(this);

      // Get the vid of the vocabulary that this menu is based on
      var vid = menu_div.attr("vid");

      // Determine the url to load the menu from if necessary
      var menu_url = Drupal.settings.basePath + "rsc_taxonomy_menu/" + vid;

      // Determine what we call our stuff in localstorage
      var markup_id = "rsc_taxonomy_menu_markup_" + vid;
      var modified_id = "rsc_taxonomy_menu_modified_" + vid;

      // Show that something is happening!
      menu_div.html("Loading ...");

      // check if localstorage is supported by this browser
      var support = false;
      try {
        if ('localStorage' in window && window['localStorage'] !== null) {
          support = true
        }
      } catch (e) {}

      // if localstorage is supported, try to load the menu from the localstorage
      if (support) {

        var markup = localStorage[markup_id];
        var modified = localStorage[modified_id];

        if (markup && modified && (menu_div.attr("modified") == modified)) {
          //console.log("cache hit");

          // add the menu to the DOM
          menu_div.html(markup);

        } else {
          //console.log("cache miss");

          // request the menu from the server using ajax
          $.ajax({
            url: menu_url,
            type: 'get',
            dataType: 'html',
            async: true,
            success: function(data) {
              markup = data;

              // save the menu to localstorage for next time
              localStorage[markup_id] = markup;
              localStorage[modified_id] = menu_div.attr("modified");

              // add the menu to the DOM
              menu_div.html(markup);
            }
          });

        }

      } else {
        // localstorage is not supported. Just get the menu from the server.
        menu_div.load(menu_url);
      }

    })

  }
};


})(jQuery, Drupal, this, this.document);
