<?php
/**
 * @file
 * Documents hooks provided by this module.
 *
 * Copyright 2009-11 by Jim Berry ("solotandem", http://drupal.org/user/240748)
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Declares a filter menu plugin.
 *
 * This hook allows contributed modules to declare additional filter menu
 * functionality supplied by their module.
 *
 * @return
 *   An associative array with the following elements:
 *   - 'name': The plugin name (often the module name).
 *   - 'desc': A description of the plugin functionality.
 */
function hook_tf_template_info() {
  return array(
    'name' => 'context',
    'desc' => 'Shows terms related in some way to the base term.',
  );
}

/**
 * Returns a list of menu items to display in the section.
 *
 * This hook allows contributed modules to supply a custom array of menu items
 * to the section factory function. If the menu items have a template_settings
 * attribute, then that will be copied into the section data.
 *
 * @see hook_tf_section_alter()
 * @see hook_tf_block_alter()
 *
 * @param array $section_info
 *   Section information.
 * @param array $block_info
 *   Block information.
 *
 * @return array
 *   Section items.
 */
function hook_tf_section($section_info, $block_info) {
  $terms = array();
  $vid = $section_info['vid'];
  $tids = $block_info['url_tids'];

  $items = array();
  if (count($terms) > 0) {
    $items = taxonomy_filter_section_items($terms, $section_info, $block_info, $context);
  }
  return $items;
}

/**
 * Alters a list of term IDs to include in the link for a menu item.
 *
 * @param array $link_tids
 *   A list of term IDs.
 * @param array $item_tid
 *   The term ID of the current item.
 * @param string $context
 *   Not used.
 */
function hook_tf_link_tids_alter(&$link_tids, $item_tid, $context) {
  if ($context == 'base terms') {
    $link_tids = array($item_tid);
  }
}

/**
 * Alters a menu item to be displayed in the section.
 *
 * The menu item is an associative array with the following elements:
 * - 'title': The item title.
 * - 'item_attributes': Associative array of HTML attributes for the menu item,
 *     such as 'class' and 'style'.
 * - 'link_attributes': Associative array of HTML attributes for the item link,
 *     such as 'href', 'class', 'title', and 'style'.
 * - 'info': An associative array with the following elements:
 *   - 'item_tid': The term ID represented by the menu item.
 *   - 'link_tids': Array of term IDs in the link for the menu item.
 *
 * @see hook_tf_section_alter()
 * @see hook_tf_block_alter()
 *
 * @param array $item
 *   Filter menu item.
 * @param array $section_info
 *   Section information.
 * @param array $block_info
 *   Block information.
 * @param string $context
 *   Not used.
 */
function hook_tf_item_alter(&$item, $section_info, $block_info, $context = NULL) {
  if ($context != 'base terms') {
    if (in_array($item['info']['item_tid'], $block_info['url_tids'])) {
      $item['link_attributes']['class'][] = 'selected';
    }
  }
}

/**
 * Alters a filter menu section.
 *
 * This hook allows contributed modules to alter a list of menu items to be
 * displayed in the section. [and other section metadata]
 *
 * The section item is an associative array with the following elements:
 * - 'title': The section title.
 * - 'items': Array of menu items.
 * - 'info': An associative array with the following elements:
 *   - 'section_title': The section title.
 *   - 'vid': Vocabulary ID for menu items in the section,
 *   - 'menu_id': The ID of the menu.
 *   - 'module': The module used to generate the menu items for the section.
 *   - 'section_settings': Array of settings related to the menu.
 *   - 'class': Class name to be added to HTML element.
 *
 * @see hook_tf_block_alter()
 *
 * @param array $section
 *   Section item including section information and menu items.
 * @param array $block_info
 *   Block information.
 */
function hook_tf_section_alter(&$section, $block_info) {
  if ($section['info']['module'] == 'tf_cloud') {
    $results = array();
    foreach ($section['items'] as $item) {
      $results[] = $item['info']['tf_count'];
    }
    $thresholds = _tf_cloud_calc_thresholds($results);
    foreach ($section['items'] as &$item) {
      $size = _tf_cloud_calc_font_size($item['info']['tf_count'], $thresholds);
      $item['info']['tf_cloud'] = $size;
      $item['item_attributes']['class'][] = 'size' . $size;
    }
  }
}

/**
 * Alters a filter menu block.
 *
 * The block item is an associative array with the following elements:
 * - 'title': The block title.
 * - 'sections': Array of section items.
 * - 'info': An associative array with the following elements:
 *   - 'url_tids': Array of term IDs in the current page URL.
 *   - 'url_depth': The depth of terms to display in each section of the block.
 *
 * @param array $block
 *   Block item including block information and section items.
 */
function hook_tf_block_alter(&$block) {
  $add_css = FALSE;
  unset($block['sections']['base_terms']);
  foreach ($block['sections'] as &$section) {
    $settings = $section['info']['section_settings']['tf_multi'];
    if (isset($settings['apply_multi']) && $settings['apply_multi']) {
      $section['info']['class'][] = 'tf_multi';
      $add_css = TRUE;
    }
  }
  if ($add_css) {
    drupal_add_css(drupal_get_path('module', 'tf_multi') . '/tf_multi.css');
  }
}

/**
 * @} End of "addtogroup hooks".
 */

/**
 * Sample functions.
 *
 * To use the sample functions included in this api file:
 * - Copy and paste the sample functions to a file in your module.
 * - Replace "your_module_name" with your actual module name.
 * - Add code to the hook function.
 */

/**
 * Implements hook_tf_template_info().
 */
function your_module_name_tf_template_info() {
  return array(
    'name' => 'your_module_name',
    'desc' => 'This module displays menu items in an unusual way.',
  );
}

/**
 * Implements hook_tf_section().
 */
function your_module_name_tf_section($section_info, $block_info) {
  $terms = array();
  $vid = $section_info['vid'];
  $tids = $block_info['url_tids'];

  $items = array();
  if (count($terms) > 0) {
    $items = taxonomy_filter_section_items($terms, $section_info, $block_info, $context);
  }
  return $items;
}

/**
 * Implements hook_tf_link_tids_alter().
 */
function your_module_name_tf_link_tids_alter(&$link_tids, $item_tid, $context) {
  if ($context == 'your_module_name') {
    $link_tids = array($item_tid);
  }
}

/**
 * Implements hook_tf_item_alter().
 */
function your_module_name_tf_item_alter(&$item, $section_info, $block_info, $context = NULL) {
  if ($context == 'your_module_name') {
    if (in_array($item['info']['item_tid'], $block_info['url_tids'])) {
      $item['link_attributes']['class'][] = 'selected';
    }
  }
}

/**
 * Implements hook_tf_section_alter().
 */
function your_module_name_tf_section_alter(&$section, $block_info) {
  if ($section['info']['module'] == 'your_module_name') {
    foreach ($section['items'] as $item) {
      // Do something.
    }
  }
}

/**
 * Implements hook_tf_block_alter().
 */
function your_module_name_tf_block_alter(&$block) {
  $add_css = FALSE;
 $block['title'] = t('My special block title');
  foreach ($block['sections'] as &$section) {
    // Do something.
  }
  if ($add_css) {
    drupal_add_css(drupal_get_path('module', 'your_module_name') . '/tf_special.css');
  }
}
