<?php
function pllkc_preprocess_page(&$vars, $hook) {
  if (isset($vars['node'])) {
  // If the node type is "blog" the template suggestion will be "page--blog.tpl.php".
   $vars['theme_hook_suggestions'][] = 'page__'. str_replace('_', '--', $vars['node']->type);
  }  
}

function pllkc_form_alter(&$form, &$form_state, $form_id) {

  if ($form_id == 'search_block_form') {
  	/*
    $form['search_block_form']['#title'] = t('Search'); // Change the text on the label element
    $form['search_block_form']['#title_display'] = 'invisible'; // Toggle label visibilty
    */
    $form['search_block_form']['#size'] = 40;  // define size of the textfield
    // $form['search_block_form']['#value'] = t('Search this site'); // Set a default value for the textfield
    $form['actions']['submit']['#value'] = t('Go'); // Change the text on the submit button
    /*
    $form['search_block_form']['#attributes']['onblur'] = "if (this.value == '') {this.value = 'Search this site';}";
    $form['search_block_form']['#attributes']['onfocus'] = "if (this.value == 'Search this site') {this.value = '';}";
    */
  
  }
}

