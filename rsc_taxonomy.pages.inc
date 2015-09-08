<?php


/**
 * Displays all nodes associated with a term.
 */
function rsc_taxonomy_term_page($term) {

  // prevent menu from setting different title
  drupal_set_title($term->name);

  // Add RSS feed
  drupal_add_feed('taxonomy/term/' . $term->tid . '/feed', 'RSS - ' . $term->name);

  // TODO: replace this view with rsc_taxonomy_get_menu_markup ?
  
  // Load a table of subcategories and number of articles in each
  // TODO: cache subs & nids!!
  $subs = rsc_taxonomy_subs_count(taxonomy_get_tree($term->vid, $term->tid, 1));
  
  // Get the module settings
  $settings = variable_get('rsc_taxonomy_settings', null);

  // determine whether this listing should be sorted by weight:
  $sort = false;
  if (!empty($settings['sort_enable_field'])) {
    
    // Get the value of the first item in this field instance for the current term
    $items = field_get_items('taxonomy_term', $term, $settings['sort_enable_field']);
    if (!empty($items)) {
      $item = array_shift($items);
      
      // If the field value evaluates to true, then sorting is enabled.
      if ($item['value']) {
        $sort = true;
      }
    }
  }

  // Get a paginated list of node ids
  $n_nodes_per_page = isset($settings['nodes_per_page']) ? (int) $settings['nodes_per_page'] : 10;
  $nids = rsc_taxonomy_nids($term->tid, true, $n_nodes_per_page, $sort);

  // Get the renderable taxonomy term
  $term_view_mode = 'full';
  $langcode = $GLOBALS['language_content']->language;
  // NB Always call this, even if no description, because other modules need hook_taxonomy_term_view_alter and hook_entity_view_alter
  $term_renderarray = taxonomy_term_view($term, $term_view_mode, $langcode);

  // start building the page
  $build = array();

  // Add term heading if the term has a description
  if (!empty($term->description)) {
    $build['term_heading'] = array(
      '#prefix' => '<div class="term-listing-heading">',
      '#suffix' => '</div>',
      'term' => $term_renderarray,
      '#weight' => 1
    );
  }

  // Add a table of subterms if there are subterms
  if ($subs) {
    $build['subterms'] = array(
      'subterms' => array(
        '#theme' => 'table',
        '#header' => array(
          array(
            'data'  => t("Category"),
            'class' => 'cat',
          ),
          array(
            'data'  => t("#&nbsp;Sub&shy;cate&shy;gories"),
            'class' => 'sub',
          ),
          array(
            'data'  => t("#&nbsp;Articles"),
            'class' => 'art',
          ),
        ),
        '#rows' => $subs,
        '#attributes' => array(
          'id' => array('taxonomy-subterms-map')
        ),
      ),
      '#weight' => 2
    );
  }

  // Add a list of nodes if there are nodes
  if ($nids) {
    $build['nodes'] = array(
      '#prefix' => '<div class="taxonomy-nodes-wrapper">',
      '#suffix' => '</div>',
      'nodes' => node_view_multiple(node_load_multiple($nids)),
      '#weight' => 3
    );
  }

  // Add a message if there is nothing to see
  if (!($nids || $subs)) {
    $build['nothing'] = array(
      '#markup' => '<strong>'.t('There is currently no content in this category.').'</strong>',
      '#weight' => 3
    );
  }

  // Put a pager at the top of the page
  $build['pager_top'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('pager-top'),
    ),
    '#weight' => -5,
    'pager' => array(
      '#theme' => 'pager',
    ),
  );

  // Put a pager at the bottom of the page
  $build['pager_bottom'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('pager-bottom'),
    ),
    '#weight' => 5,
    'pager' => array(
      '#theme' => 'pager',
    ),
  );

  // Keep modules that expect $entity->content to exist happy
  // otherwise this causes errors with implementation field_conditional_state_entity_view()
  $term->content = $build;

  // Allow modules to make their own additions to the term.
  module_invoke_all('taxonomy_term_view', $term, $term_view_mode, $langcode);
  module_invoke_all('entity_view', $term, 'taxonomy_term', $term_view_mode, $langcode);

  return $build;
}


function rsc_taxonomy_subs_count($terms) {
  $rows = array();
  foreach ($terms as $term) {
    $sub = count(taxonomy_get_children($term->tid)); // get number of subcategories for term
    if (!$sub) {
      $sub = '&ndash;';
    }

    $art = 0;
    $art += count(taxonomy_select_nodes($term->tid,false,false)); // get number of articles that fall directly under this term
    $subtree = taxonomy_get_tree($term->vid, $term->tid); // get a flat "tree" of subcategories
    foreach($subtree as $subcat) {
      $art += count(taxonomy_select_nodes($subcat->tid,false,false)); // get number of articles that fall directly under this term
    }
    if ($art==0) {
      $art = '&ndash;';
    }

    $rows[] = array(
      'data' => array(
        "cat" => array(
          'data'  => l($term->name, "taxonomy/term/". ($term->tid) ),
          'class' => 'cat',
        ),
        "sub" => array(
          'data'  => $sub,
          'class' => 'sub',
        ),
        "art" => array(
          'data'  => $art,
          'class' => 'art',
        ),
      ),
      'no_striping' => true,
    );
    
  }
  return $rows;
}


function rsc_taxonomy_nids($tid, $pager = true, $limit = false, $weighten_nids = false) {
  $weightfield = variable_get('rsc_taxonomy_sort_weight_field',null);
  if ($weighten_nids && !empty($weightfield)) {
    //copy-pasted from somewhere. No idea how this works... (yet)
    if (!variable_get('taxonomy_maintain_index_table', true)) {
      return array();
    }
    $query = db_select('taxonomy_index', 't');
    $query->addTag('node_access');
    $query->leftJoin("field_data_$weightfield", 'w', 't.nid = w.entity_id');
    $query->condition('t.tid', $tid);
    if ($pager) {
      $count_query = clone $query;
      $count_query->addExpression('COUNT(t.nid)');

      $query = $query->extend('PagerDefault');
      if ($limit !== false) {
        $query = $query->limit($limit);
      }
      $query->setCountQuery($count_query);
    } else {
      if ($limit !== false) {
        $query->range(0, $limit);
      }
    }
    $query->addField('t', 'nid');
    $query->addField('t', 'tid');
    $query->orderBy("w.{$weightfield}_value");
    $nids = $query->execute()->fetchCol();
  } else {
    $nids = taxonomy_select_nodes($tid, $pager, $limit);
  }
  return $nids;
}

