<?php

/**
 * @file
 *   Administrative settings for searching.
 */

/**
 * Helper function for empty search configuration.
 */
function _apachesolr_search_browse_form($default_value) {
  $description = t('This is what is shown when the user enters an empty search, or removes all filters from an active search.') . ' ';
  if (!module_exists('facetapi')) {
    $description .= t('<strong>Filters will not be shown until you enable Facet API module.</strong>');
  }
  else {
    $description .= t('Remember to enable filters on the <a href="!filterslink">enabled filters page</a> and assign blocks to regions on the <a href="!blocklink">block settings page</a>', array(
      '!filterslink' => url('admin/config/search/apachesolr/facets'),
      '!blocklink' => url('admin/structure/block'),
    ));
  }
  return array(
    '#type' => 'radios',
    '#title' => t('Behavior on empty search'),
    '#options' => array(
      'none' => t("Show search box"),
      'browse'  => t("Show enabled filters' blocks under the search box"),
      'blocks'  => t("Show enabled filters' blocks in their configured regions"),
      'results' => t("Show enabled filters' blocks in their configured regions and first page of all available results"),
    ),
    '#default_value' => $default_value,
    '#description' => $description,
  );
}

/**
 * Menu callback for the overview page showing custom search pages.
 */
function apachesolr_search_page_list_page() {
  $build = array();

  // Build the sortable table header.
  $header = array(
    'label' => array('data' => t('Name'), 'field' => 's.label'),
    'path' => array('data' => t('Path'), 'field' => 's.search_path'),
    'environment' => array('data' => t('Search environment')),
    'operations' => array('data' => t('Operations'), 'colspan' => '2'),
  );

  // Initializes the search page query.
  $query = db_select('apachesolr_search_page', 's')
    ->fields('s', array('label', 'page_id', 'page_title', 'env_id', 'description', 'search_path', 'settings'))
    ->extend('PagerDefault')
    ->extend('TableSort')
    ->limit(20)
    ->orderByHeader($header);

  $rows = array();
  $rows['core_search'] = array();

  foreach ($query->execute() as $record) {
    $row = array();

    $label = check_plain($record->label);
    if ($record->page_id == 'core_search') {
      $label .= ' ' . t('<em>(integrates with core <a href="!search_admin">Search module</a>)</em>', array('!search_admin' => url('admin/config/search/settings')));
      // Special case - always uses the default environment.
      $record->env_id = apachesolr_default_environment();
    }

    $row[] = $label;

    $row[] = array(
      'data' => array(
        '#type' => 'link',
        '#title' => $record->search_path,
        '#href' => $record->search_path,
      ),
    );

    $environment = apachesolr_environment_load($record->env_id);
    $row[] = $environment ? check_plain($environment['name']) : check_plain(t('<Disabled>'));

    $settings = unserialize($record->settings);
    // Operations
    $row[] = array('data' => l(t('Edit'), 'admin/config/search/apachesolr/search-pages/' . $record->page_id . '/edit'));
    if (!isset($settings['apachesolr_search_not_removable'])) {
      $row[] = array('data' => l(t('Delete'), 'admin/config/search/apachesolr/search-pages/' . $record->page_id . '/delete'));
    }
    else {
      $row[] = '';
    }

    $rows[$record->page_id] = $row;
  }

  $build['list'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => array_values($rows),
    '#empty' => t('No available search pages.'),
  );
  $build['pager'] = array(
    '#theme' => 'pager',
    '#quantity' => 20,
    '#weight' => 10,
  );

  return $build;
}

/**
 * Menu callback/form-builder for the form to create or edit a search page.
 */
function apachesolr_search_page_settings_form($form, &$form_state, $search_page = NULL) {
  $environments = apachesolr_load_all_environments();
  $options = array('' => t('<Disabled>'));
  foreach ($environments as $id => $environment) {
    $options[$id] = $environment['name'];
  }
  // Validate the env_id.
  if (!empty($search_page->env_id) && !apachesolr_environment_load($search_page->env_id)) {
    $search_page->env_id = '';
  }

  // Initializes form with common settings.


  $form['search_page'] = array(
      '#type' => 'value',
      '#value' => $search_page,
  );

  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#description' => '',
    '#required' => TRUE,
    '#size' => 30,
    '#maxlength' => 32,
    '#default_value' => !empty($search_page->label) ? $search_page->label : '',
    '#description' => t('The human-readable name of the search page configuration.'),
  );

  $form['page_id'] = array(
    '#type' => 'machine_name',
    '#maxlength' => 32,
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'apachesolr_search_page_exists',
      'source' => array('label'),
    ),
    '#description' => '',
    '#default_value' => !empty($search_page->page_id) ? $search_page->page_id : '',
    '#disabled' => !empty($search_page),
    '#description' => t('A unique machine-readable identifier for the search page configuration. It must only contain lowercase letters, numbers, and underscores.'),
  );

  $form['description_enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Description'),
    '#default_value' => !empty($search_page->description)  ? TRUE : FALSE
  );

  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Provide description'),
    '#title_display' => 'invisible',
    '#size' => 64,
    '#default_value' => !empty($search_page->description) ? $search_page->description : '',
    '#dependency' => array(
      'edit-description-enable' => array(1),
    ),
  );

  $form['info'] = array(
    '#title' => t('Search Page Information'),
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
  );

  $core_search = FALSE;
  if(!empty($search_page->page_id) && ($search_page->page_id == 'core_search')) {
    $core_search = TRUE;
  }
  if ($core_search) {
    $description = t('This page always uses the current default search environment');
  }
  else {
    $description = t('The environment that is used by this search page. If no environment is selected, this page will be disabled.');
  }

  $form['info']['env_id'] = array(
    '#title' => t('Search environment'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => !empty($search_page->env_id) ? $search_page->env_id : '',
    '#disabled' => $core_search,
    '#description' => $description,
  );

  $form['info']['page_title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#description' => '',
    '#default_value' => !empty($search_page->page_title) ? $search_page->page_title : '',
  );

  $search_types = apachesolr_load_all_search_types();
  $options = array('custom' => t('Custom Field'));
  foreach ($search_types as $id => $search_type) {
    $options[$id] = $search_type['name'];
  }

  $form['info']['search_type'] = array(
    '#title' => t('Search Type'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => !empty($search_page->settings['apachesolr_search_search_type']) ? $search_page->settings['apachesolr_search_search_type'] : '',
    '#access' => !$core_search,
    '#description' => t('Use this only when filtering on a value from the search path.
      For example, select Taxonomy Term to filter on a term ID (search/taxonomy/%).'),
    '#ajax' => array(
      'callback' => 'apachesolr_search_ajax_search_page_default',
      'wrapper' => 'dynamic-search-page',
      'method' => 'replace',
    ),
  );

  // Token element validate is added to validate the specific
  // tokens that are allowed
  $form['info']['search_path'] = array(
    '#title' => t('Path'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#description' => t('For example: search/my-search-page. Search keywords will appear at the end of the path.'),
    '#default_value' => !empty($search_page->search_path) ? $search_page->search_path : '',
  );
  if (!$core_search) {
    $form['info']['search_path']['#description'] .= ' ' . t('You can use one % to make the search page dynamic.');
  }

  $form['info']['custom_filter_enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Custom Filter'),
    '#default_value' => !empty($search_page->settings['fq'])  ? TRUE : FALSE
  );

  $form['info']['filters'] = array(
    '#title' => t('Custom filters'),
    '#type' => 'textfield',
    '#required' => FALSE,
    '#maxlength' => 255,
    '#description' => t('A comma-separated list of lucene filter queries to apply by default.'),
    '#default_value' => !empty($search_page->settings['fq'])  ? implode(', ', $search_page->settings['fq']) : '',
    '#dependency' => array(
      'edit-custom-filter-enable' => array(1),
      'edit-search-type' => array('custom'),
    ),
  );
  if (!$core_search) {
    $form['info']['filters']['#description'] .= ' ' . t('E.g. "bundle:blog, is_uid:(1 OR 2 OR %). % will be replaced by the value of % in the path"');
  }

  $form['advanced'] = array(
    '#title' => t('Advanced Search Page Options'),
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // Results per page per search page
  $default_value = isset($search_page->settings['apachesolr_search_per_page']) ? $search_page->settings['apachesolr_search_per_page'] : '10';
  $form['advanced']['apachesolr_search_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Results per page'),
    '#description' => t('Select how many items will be displayed on one page of the search result.'),
    '#options' => drupal_map_assoc(array(5, 10, 20, 30, 40, 50, 60, 80, 100)),
    '#default_value' => $default_value,
  );

  // Enable/disable spellcheck on pages
  $default_value = isset($search_page->settings['apachesolr_search_spellcheck']) ? $search_page->settings['apachesolr_search_spellcheck'] : TRUE;
  $form['advanced']['apachesolr_search_spellcheck'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable spell check'),
    '#description' => t('Display "Did you mean … ?" above search results.'),
    '#default_value' => $default_value,
  );

  // Enable/disable search form on search page (replaced by a block perhaps)
  $default_value = isset($search_page->settings['apachesolr_search_search_box']) ? $search_page->settings['apachesolr_search_search_box'] : TRUE;
  $form['advanced']['apachesolr_search_search_box'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable the search box on the page'),
    '#description' => t('Display a search box on the page.'),
    '#access' => !$core_search,
    '#default_value' => $default_value,
  );

  // Use the main search page setting as the default for new pages.
  $default_value = isset($search_page->settings['apachesolr_search_browse']) ? $search_page->settings['apachesolr_search_browse'] : 'browse';
  $form['advanced']['apachesolr_search_browse'] = _apachesolr_search_browse_form($default_value);

  // Button for the corresponding actions
  $form['actions'] = array(
    '#type' => 'actions',
  );

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/search/apachesolr/search-pages',
  );

  $form['#submit'][] = 'apachesolr_search_page_settings_form_submit';

  return $form;
}

/**
 * Callback element needs only select the portion of the form to be updated.
 * Since #ajax['callback'] return can be HTML or a renderable array (or an
 * array of commands), we can just return a piece of the form.
 */
function apachesolr_search_ajax_search_page_default($form, $form_state, $search_page = NULL) {

  $search_page = $form_state['values']['search_page'];
  $search_types = apachesolr_load_all_search_types();

  // Helping with sensible defaults for the search path
  $default_search_path = '';

  if (!empty($form_state['values']['search_type']) && $form_state['values']['search_type'] != 'custom') {
    $default_search_path = $search_types[$form_state['values']['search_type']]['default menu'];
    $form['setup']['search_path']['#value'] = $default_search_path;
  }

  // Helping with sensible defaults for the search title
  $default_search_title = '';

  if (!empty($form_state['values']['page_title']) && $form_state['values']['search_type'] != 'custom') {
    $default_search_title_callback = $search_types[$form_state['values']['search_type']]['title callback'];
    $default_search_title = $default_search_title_callback();
    $form['setup']['page_title']['#value'] = $default_search_title;
  }
  return $form['setup'];
}

function apachesolr_search_page_settings_form_validate($form, &$form_state) {
  // Performs basic validation of the menu path.
  if (url_is_external($form_state['values']['search_path'])) {
    form_set_error('search_path', t('Path must be local.'));
  }
  $form_state['values']['search_path'] = trim($form_state['values']['search_path'], '/');
  if (empty($form_state['values']['search_path'])) {
    form_set_error('search_path', t('Path required.'));
  }
  if ($form_state['values']['page_id'] == 'core_search') {
    if (!preg_match('@^search/[^/%]+$@', $form_state['values']['search_path'])) {
      form_set_error('search_path', t('The core Search page path must start with search/ and only have one /'));
    }
  }
  elseif (count(explode('%', $form_state['values']['search_path'])) > 2) {
    form_set_error('search_path', t('Only one % placeholder is allowed.'));
  }
}

/**
 * Processes apachesolr_search_page_settings_form form submissions.
 */
function apachesolr_search_page_settings_form_submit($form, &$form_state) {
  $settings = array();
  $settings['fq'] = array();
  if ($form_state['values']['filters']) {
    foreach(explode(',', $form_state['values']['filters']) as $string) {
      $string = trim($string);
      // Minimal validation.  ':' must exist and can't be the 1st char..
      if (strpos($string, ':')) {
        $settings['fq'][] = $string;
      }
    }
  }
  $settings['apachesolr_search_search_type'] = $form_state['values']['search_type'];
  $settings['apachesolr_search_search_box'] = $form_state['values']['apachesolr_search_search_box'];
  $settings['apachesolr_search_per_page'] = $form_state['values']['apachesolr_search_per_page'];
  $settings['apachesolr_search_browse'] = $form_state['values']['apachesolr_search_browse'];
  $settings['apachesolr_search_spellcheck'] = $form_state['values']['apachesolr_search_spellcheck'];

  if (isset($form_state['values']['search_page']->settings)) {
    $settings = array_merge($form_state['values']['search_page']->settings, $settings);
  }

  $search_page = new stdClass();
  $search_page->page_id = $form_state['values']['page_id'];
  $search_page->label = $form_state['values']['label'];
  $search_page->description = $form_state['values']['description'];
  $search_page->env_id = $form_state['values']['env_id'];
  $search_page->search_path = $form_state['values']['search_path'];
  $search_page->page_title = $form_state['values']['page_title'];
  $search_page->settings = $settings;
  apachesolr_search_page_save($search_page);

  // Saves our values in the database, sets redirect path on success.
  drupal_set_message(t('The configuration options have been saved for %page.', array('%page' => $form_state['values']['label'])));
  $form_state['redirect'] = 'admin/config/search/apachesolr/search-pages';

  // Menu rebuild needed to pick up search path.
  menu_rebuild();
}

/**
 * Deletes a single search page configuration.
 */
function apachesolr_search_delete_search_page_confirm($form, &$form_state, $search_page) {

  // Sets values required for deletion.
  $form['page_id'] = array('#type' => 'value', '#value' => $search_page->page_id);
  $form['label'] = array('#type' => 'value', '#value' => $search_page->label);

  // Sets the message, or the title of the page.
  $message = t(
    'Are you sure you want to delete the %label search page configuration?',
    array('%label' => $form['label']['#value'])
  );

  // Builds caption.
  $caption = '<p>';
  $caption .= t(
    'The %label search page configuration will be deleted.',
    array('%label' => $form['label']['#value'])
  );
  $caption .= '</p>';
  $caption .= '<p><strong>' . t('This action cannot be undone.') . '</strong></p>';

  // Finalizes and returns the confirmation form.
  $return_path = 'admin/config/search/apachesolr/search-pages';
  $button_text = t('Delete page');
  if (!isset($search_page->settings['apachesolr_search_not_removable'])) {
    return confirm_form($form, $message, $return_path, $caption, $button_text);
  }
  else {
    // Maybe this should be solved somehow else
    drupal_access_denied();
  }
}

/**
 * Process content type delete confirm submissions.
 */
function apachesolr_search_delete_search_page_confirm_submit($form, &$form_state) {
  // Deletes the index configuration settings.
  // @todo Invoke a hook that allows backends and indexers to delete their stuff.
  db_delete('apachesolr_search_page')
    ->condition('page_id', $form_state['values']['page_id'])
    ->execute();

  // Sets message, logs action.
  drupal_set_message(t(
    'The %label search page configuration has been deleted.',
    array('%label' => $form_state['values']['label'])
  ));
  watchdog('apachesolr_search', 'Deleted search page configuration "@page_id".', array('@page_id' => $form_state['values']['page_id']), WATCHDOG_NOTICE);

  // Rebuilds the menu cache.
  menu_rebuild();

  // Returns back to search page list page.
  $form_state['redirect'] = 'admin/config/search/apachesolr/search-pages';
}


/**
 * Menu callback - the settings form.
 */
function apachesolr_search_settings_page($env_id = NULL) {
  if (empty($env_id)) {
    $env_id = apachesolr_default_environment();
  }

  // Try to fetch the schema fields.
  try {
    $solr = apachesolr_get_solr($env_id);
    $fields = $solr->getFields();
    $output = array(
      'apachesolr_environment' => array(
        '#theme' => 'apachesolr_settings_title',
        '#env_id' => $env_id,
      ),
      'apachesolr_search_settings_form' => drupal_get_form('apachesolr_search_settings_form', $fields, $env_id)
    );
  }
  catch (Exception $e) {
    watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
    drupal_set_message(nl2br(check_plain($e->getMessage())), 'warning');
    $output = t('Cannot get information about the fields in the index at this time.');
  }
  return $output;
}

/**
 * Menu callback - boosts settings form.
 */
function apachesolr_boost_settings_page($env_id = NULL) {
  if (empty($env_id)) {
    $env_id = apachesolr_default_environment();
  }

  // Initializes output with information about which environment's setting we are
  // editing, as it is otherwise not transparent to the end user.
  $output = array(
    'apachesolr_environment' => array(
      '#theme' => 'apachesolr_settings_title',
      '#env_id' => $env_id,
    ),
  );

  // Adds content bias and type boost / exclusion forms.
  $output['apachesolr_search_bias_form'] = drupal_get_form('apachesolr_search_bias_form', $env_id);
  $output['apachesolr_search_type_boost_form'] = drupal_get_form('apachesolr_search_type_boost_form', $env_id);
  return $output;
}

/**
 * Form builder function to set date, comment, etc biases.
 */
function apachesolr_search_bias_form($form, &$form_state, $env_id) {

  $date_settings = apachesolr_environment_variable_get($env_id, 'apachesolr_search_date_boost', '0:0');
  $comment_settings = apachesolr_environment_variable_get($env_id, 'apachesolr_search_comment_boost', '0:0');
  $changed_settings = apachesolr_environment_variable_get($env_id, 'apachesolr_search_changed_boost', '0:0');
  $sticky_boost = apachesolr_environment_variable_get($env_id, 'apachesolr_search_sticky_boost', '0');
  $promote_boost = apachesolr_environment_variable_get($env_id, 'apachesolr_search_promote_boost', '0');

  $options = array(
    '10:2000.0' => '10',
    '8:1000.0' => '9',
    '8:700.0' => '8',
    '8:500.0' => '7',
    '4:300.0' => '6',
    '4:200.0' => '5',
    '4:150.0' => '4',
    '2:150.0' => '3',
    '2:100.0' => '2',
    '1:100.0' => '1',
    '0:0' => t('Ignore'),
  );

  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Ignore');

  $form['biasing'] = array(
    '#type' => 'fieldset',
    '#title' => t('Result biasing'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Give bias to certain properties when ordering the search results. Any value except <em>Ignore</em> will increase the score of the given type in search results. Choose <em>Ignore</em> to ignore any given property.'),
  );
  $form['biasing']['apachesolr_search_sticky_boost'] = array(
    '#type' => 'select',
    '#options' => $weights,
    '#title' => t("Sticky at top of lists"),
    '#default_value' => $sticky_boost,
    '#description' => t("Select additional bias to give to nodes that are set to be 'Sticky at top of lists'."),
  );
  $form['biasing']['apachesolr_search_promote_boost'] = array(
    '#type' => 'select',
    '#options' => $weights,
    '#title' => t("Promoted to home page"),
    '#default_value' => $promote_boost,
    '#description' => t("Select additional bias to give to nodes that are set to be 'Promoted to home page'."),
  );
  $form['biasing']['apachesolr_search_date_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More recently created"),
    '#default_value' => $date_settings,
    '#description' => t('This setting will change the result scoring so that nodes created more recently may appear before those with higher keyword matching.'),
  );
  $form['biasing']['apachesolr_search_comment_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More comments"),
    '#default_value' => $comment_settings,
    '#description' => t('This setting will change the result scoring so that nodes with more comments may appear before those with higher keyword matching.'),
  );
  $form['biasing']['apachesolr_search_changed_boost'] = array(
    '#type' => 'select',
    '#options' => $options,
    '#title' => t("More recent comments"),
    '#default_value' => $changed_settings,
    '#description' => t('This setting will change the result scoring so that nodes with the most recent comments (or most recent updates to the node itself) may appear before those with higher keyword matching.'),
  );

  return apachesolr_settings_form($form, $env_id);
}

/**
 * Form builder function to set query field weights.
 */
function apachesolr_search_settings_form($form, &$form_state, $fields, $env_id) {
  // get the current weights
  $defaults = array(
    'content' => '1.0',
    'ts_comments' => '0.5',
    'tos_content_extra' => '0.1',
    'label' => '5.0',
    'tos_name' => '3.0',
    'taxonomy_names' => '2.0',
    'tags_h1' => '5.0',
    'tags_h2_h3' => '3.0',
    'tags_h4_h5_h6' => '2.0',
    'tags_inline' => '1.0',
    'tags_a' => '0',
  );
  $qf = apachesolr_environment_variable_get($env_id, 'apachesolr_search_query_fields', $defaults);
  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Omit');
  if (!$qf) {
    $qf = $defaults;
  }
  if ($fields) {
    $form['apachesolr_search_query_fields'] = array(
      '#type' => 'fieldset',
      '#title' => t('Field biases'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
      '#description' => t('Specify here which fields are more important when searching. Give a field a greater numeric value to make it more important. If you omit a field, it will not be searched.'),
    );
    foreach ($fields as $field_name => $field) {
      // Only indexed feids are searchable.
      if ($field->schema{0} == 'I') {
        // By default we only show text fields.  Use hook_form_alter to change.
        $form['apachesolr_search_query_fields'][$field_name] = array(
          '#access' => $field->type == 'text',
          '#type' => 'select',
          '#options' => $weights,
          '#title' => apachesolr_field_name_map($field_name),
          '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : '0',
        );
      }
    }
    // Make sure all the default fields are included, even if they have
    // no indexed content.
    foreach ($defaults as $field_name => $weight) {
      $form['apachesolr_search_query_fields'][$field_name] = array(
        '#type' => 'select',
        '#options' => $weights,
        '#title' => apachesolr_field_name_map($field_name),
        '#default_value' => isset($qf[$field_name]) ? $qf[$field_name] : $defaults[$field_name],
      );
    }

    ksort($form['apachesolr_search_query_fields']);
  }

  return apachesolr_settings_form($form, $env_id);
}

/**
 * Form builder function to set query type weights.
 */
function apachesolr_search_type_boost_form($form, &$form_state, $env_id) {

  $form['apachesolr_search_type_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Type biasing and exclusion'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['apachesolr_search_type_settings']['apachesolr_search_type_boosts'] = array(
    '#type' => 'item',
    '#description' => t("Specify here which node types should get a higher relevancy score in searches. Any value except <em>Ignore</em> will increase the score of the given type in search results."),
    '#tree' => TRUE,
  );

  $weights = drupal_map_assoc(array('21.0', '13.0', '8.0', '5.0', '3.0', '2.0', '1.0', '0.8', '0.5', '0.3', '0.2', '0.1'));
  $weights['0'] = t('Ignore');

  // Get the current boost values.
  $type_boosts = apachesolr_environment_variable_get($env_id, 'apachesolr_search_type_boosts', array());
  $names = node_type_get_names();

  foreach ($names as $type => $name) {
    $form['apachesolr_search_type_settings']['apachesolr_search_type_boosts'][$type] = array(
      '#type' => 'select',
      '#title' => t('%type type content bias', array('%type' => $name)),
      '#options' => $weights,
      '#default_value' => isset($type_boosts[$type]) ? $type_boosts[$type] : 0,
    );
  }

  $form['apachesolr_search_type_settings']['apachesolr_search_excluded_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Types to exclude from the search index'),
    '#options' => $names,
    '#default_value' => apachesolr_environment_variable_get($env_id, 'apachesolr_search_excluded_types', array()),
    '#description' => t("Specify here which node types should be totally excluded from the search index. Content excluded from the index will never appear in any search results."),
  );
  $form['#original_excluded_types'] =  $form['apachesolr_search_type_settings']['apachesolr_search_excluded_types']['#default_value'];

  $form['#submit'][] = 'apachesolr_search_type_boost_form_submit';

  return apachesolr_settings_form($form, $env_id);
}

/**
 * Submit callback for apachesolr_search_type_boost_form().
 *
 * This is called before system_settings_form_submit().
 */
function apachesolr_search_type_boost_form_submit($form, &$form_state) {
  $old_excluded_types = $form['#original_excluded_types'];
  $new_excluded_types = $form_state['values']['apachesolr_search_excluded_types'];
  // Check whether we are resetting the values.
  if ($form_state['clicked_button']['#value'] == t('Reset to defaults')) {
    $new_excluded_types = array();
  }

  foreach ($new_excluded_types as $type => $excluded) {
    // Remove newly omitted node types.
    // Note - we omit a check on empty($old_excluded_types[$type]) so that
    // the admin can re-submit this page if the delete operation fails.
    if (!empty($new_excluded_types[$type])) {
      try {
        $solr = apachesolr_get_solr($form['#env_id']);
        $solr->deleteByQuery("bundle:$type");
        apachesolr_index_set_last_updated(REQUEST_TIME);
      }
      catch (Exception $e) {
        watchdog('Apache Solr', nl2br(check_plain($e->getMessage())), NULL, WATCHDOG_ERROR);
        drupal_set_message(t('The Apache Solr search engine is not available. Please contact your site administrator.'), 'error');
      }
    }
  }

  foreach ($old_excluded_types as $type => $excluded) {
    // Set no longer omitted node types for reindexing.
    if (empty($new_excluded_types[$type]) && !empty($old_excluded_types[$type])) {
      $nids = db_select('node')->fields('node', array('nid'))->condition('type', $type);
      db_update('apachesolr_search_node')->fields(array('changed' => REQUEST_TIME))->condition('nid', $nids, 'IN')->execute();
    }
  }
}
