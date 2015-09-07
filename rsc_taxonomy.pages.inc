<?php

/**
 * Menu callback; displays all nodes associated with a term.
 */
function rsc_taxonomy_term_page($term) {
  drupal_set_title($term->name); // prevent menu from setting different title
  drupal_add_feed('taxonomy/term/' . $term->tid . '/feed', 'RSS - ' . $term->name);

  // TODO: cache subs & nids!!
  $subs = rsc_taxonomy_subs_count(taxonomy_get_tree($term->vid, $term->tid, 1));
  $fname = variable_get('rsc_taxonomy_sort_enable_field', NULL);
  $npp = 10; // number of nodes per page TODO: make this configurable
  
  // determine whether this listing should be sorted by weight:
  $sort = FALSE;
  if (!empty($fname)) {
    $items = field_get_items('taxonomy_term', $term, $fname);
    if (!empty($items)) {
      $item = array_shift($items);
      if ($item['value']) {
        $sort = TRUE;
      }
    }
  }
  
  $nids = rsc_taxonomy_nids($term->tid, TRUE, $npp, $sort);

  $build = array();

  $view_mode = 'full';
  $langcode = $GLOBALS['language_content']->language;

  // NB Always call this, even if no description, because other modules need hook_taxonomy_term_view_alter and hook_entity_view_alter
  $term_renderarray = taxonomy_term_view($term, $view_mode, $langcode);

  // Add term heading if the term has a description
  if (!empty($term->description)) {
    $build['term_heading'] = array(
      '#prefix' => '<div class="term-listing-heading">',
      '#suffix' => '</div>',
      'term' => $term_renderarray,
      '#weight' => 1
    );
  }

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
  if ($nids) {
    $build['nodes'] = array(
      '#prefix' => '<div class="taxonomy-nodes-wrapper">',
      '#suffix' => '</div>',
      'nodes' => node_view_multiple(node_load_multiple($nids)),
      '#weight' => 3
    );
  }
  if (!($nids || $subs)) { // nothing to see
    $build['nothing'] = array(
      '#markup' => '<strong>'.t('There is currently no content in this category.').'</strong>',
      '#weight' => 3
    );
  }

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
  module_invoke_all('taxonomy_term_view', $term, $view_mode, $langcode);
  module_invoke_all('entity_view', $term, 'taxonomy_term', $view_mode, $langcode);

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
    $art += count(taxonomy_select_nodes($term->tid,FALSE,FALSE)); // get number of articles that fall directly under this term
    $subtree = taxonomy_get_tree($term->vid, $term->tid); // get a flat "tree" of subcategories
    foreach($subtree as $subcat) {
      $art += count(taxonomy_select_nodes($subcat->tid,FALSE,FALSE)); // get number of articles that fall directly under this term
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


function rsc_taxonomy_nids($tid, $pager = TRUE, $limit = FALSE, $weighten_nids = FALSE) {
  $weightfield = variable_get('rsc_taxonomy_sort_weight_field',NULL);
  if ($weighten_nids && !empty($weightfield)) {
    //copy-pasted from somewhere. No idea how this works... (yet)
    if (!variable_get('taxonomy_maintain_index_table', TRUE)) {
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
      if ($limit !== FALSE) {
        $query = $query->limit($limit);
      }
      $query->setCountQuery($count_query);
    } else {
      if ($limit !== FALSE) {
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
