<?php


/**
 * Form builder for the rsc_taxonomy settings form
 */
function rsc_taxonomy_settings() {
  $form = array();

  /*
   * Construct two lists:
   * - All fields that have instances on taxonomy terms
   * - All fields that have instances on nodes
   */
  $fieldmap = field_info_field_map();
  $taxonomyfields = array(NULL=>"--none--",);
  $nodefields = array(NULL=>"--none--",);
  foreach($fieldmap as $fieldname => $fieldinfo) {
    if (!empty($fieldinfo['bundles']['taxonomy_term'])) {
      $taxonomyfields[$fieldname] = $fieldname; 
    }
    if (!empty($fieldinfo['bundles']['node'])) {
      $nodefields[$fieldname] = $fieldname;
    }
  }

  $form['rsc_taxonomy_sort_enable_field'] = array(
    '#title'         => t('Field used to enable/disable sorting on taxonomy pages:'),
    '#type'          => 'select',
    '#multiple'      => FALSE,
    '#required'      => FALSE,
    '#options'       => $taxonomyfields,
    '#default_value' => variable_get('rsc_taxonomy_sort_enable_field',NULL),
  );
  
  $form['rsc_taxonomy_sort_weight_field'] = array(
    '#title'         => t('Field used for node weight on sorted taxonomy pages:'),
    '#type'          => 'select',
    '#multiple'      => FALSE,
    '#required'      => FALSE,
    '#options'       => $nodefields,
    '#default_value' => variable_get('rsc_taxonomy_sort_weight_field',NULL),
  );
  
  return system_settings_form($form);
}

