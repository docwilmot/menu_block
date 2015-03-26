<?php
/**
 * @file
 * Provides infrequently used functions and hooks for menu_block.
 */

/**
 * Implements hook_block_info().
 */
function _menu_block_block_info() {
  $block['menu_block']['info'] = 'Menu Block';
  $block['menu_block']['description'] = 'Menu Block';
  return $block;
}

/**
 * Return the title of the block.
 *
 * @param $config
 *   array The configuration of the menu block.
 * @return
 *   string The title of the block.
 */
function _menu_block_format_title($config) {
  // If an administrative title is specified, use it.
  if (!empty($config['admin_title'])) {
    return check_plain($config['admin_title']);
  }
  $menus = menu_block_get_all_menus();
  $menus[MENU_TREE__CURRENT_PAGE_MENU] = t('Current menu');
  if (empty($config['menu_name'])) {
    $title = t('Unconfigured menu block');
  }
  elseif (!isset($menus[$config['menu_name']])) {
    $title = t('Deleted/missing menu @menu', array('@menu' => $config['menu_name']));
  }
  else {
    // Show the configured levels in the block info
    $replacements = array(
      '@menu_name' => $menus[$config['menu_name']],
      '@level1' => $config['level'],
      '@level2' => $config['level'] + $config['depth'] - 1,
    );
    if ($config['parent_mlid']) {
      $parent_item = menu_link_load($config['parent_mlid']);
      $replacements['@menu_name'] = $parent_item['title'];
    }
    if ($config['follow']) {
      $title = t('@menu_name (active menu item)', $replacements);
    }
    elseif ($config['depth'] == 1) {
      $title = t('@menu_name (level @level1)', $replacements);
    }
    elseif ($config['depth']) {
      if ($config['expanded']) {
        $title = t('@menu_name (expanded levels @level1-@level2)', $replacements);
      }
      else {
        $title = t('@menu_name (levels @level1-@level2)', $replacements);
      }
    }
    else {
      if ($config['expanded']) {
        $title = t('@menu_name (expanded levels @level1+)', $replacements);
      }
      else {
        $title = t('@menu_name (levels @level1+)', $replacements);
      }
    }
  }
  return $title;
}

/**
 * Implements hook_block_configure().
 */
function _menu_block_block_configure($delta = '', $settings = array()) {
  // Create a pseudo form state.
  // $form_state = array('values' => menu_block_get_config($delta, $settings));
  // Merge in the default configuration.
  if(empty($settings)) {
    $settings = menu_block_default_config();
  }
  $form_state = array('values' => $settings);
  return menu_block_configure_form(array(), $form_state);
}

/**
 * Returns the configuration form for a menu tree.
 *
 * @param $form_state
 *   array An associated array of configuration options should be present in the
 *   'values' key. If none are given, default configuration is assumed.
 * @return
 *   array The form in Form API format.
 */
function menu_block_configure_form($form, &$form_state) {
  $config = $form_state['values'];
  // Get the config from the form state.
  if (!empty($form_state['values'])) {
    if (!empty($config['parent'])) {
      list($config['menu_name'], $config['parent_mlid']) = explode(':', $config['parent']);
    }
  }

  // Build the standard form.
  $form['#attached']['js'][] = drupal_get_path('module', 'menu_block') . '/menu-block.js';
  $form['#attached']['css'][] = drupal_get_path('module', 'menu_block') . '/menu-block.admin.css';
  $form['#attached']['library'][] = array('system', 'ui.button');

  $form['menu-block-wrapper-start'] = array(
    '#markup' => '<div id="menu-block-settings">',
    '#weight' => -30,
  );
  $form['display_options'] = array(
    '#type' => 'radios',
    '#title' => t('Display'),
    '#default_value' => 'basic',
    '#options' => array(
      'basic' => t('Basic options'),
      'advanced' => t('Advanced options'),
    ),
    '#attributes' => array('class' => array('clearfix')),
    '#weight' => -29,
  );
  $form['title_link'] = array(
    '#type' => 'checkbox',
    '#title' => t('Block title as link'),
    '#default_value' => $config['title_link'],
    '#description' => t('Make the default block title a link to that menu item. An overridden block title will not be a link.'),
    '#states' => array(
      'visible' => array(
        ":input[name='title_display']" => array('!value' => 'none'),
      ),
    ),
  );
  $form['admin_title'] = array(
    '#type' => 'textfield',
    '#default_value' => $config['admin_title'],
    '#title' => t('Administrative title'),
    '#description' => t('This title will be used administratively to identify this block. If blank, the regular title will be used.'),
  );
  $menus = menu_block_get_all_menus();
  $form['menu_name'] = array(
    '#type' => 'select',
    '#title' => t('Menu'),
    '#default_value' => $config['menu_name'],
    '#options' => $menus,
    '#description' => t('The preferred menus used by <em>&lt;the menu selected by the page&gt;</em> can be customized on the <a href="!url">Menu block settings page</a>.', array('!url' => url('admin/config/user-interface/menu-block'))),
    '#attributes' => array('class' => array('menu-block-menu-name')),
  );
  $form['level'] = array(
    '#type' => 'select',
    '#title' => t('Starting level'),
    '#default_value' => $config['level'],
    '#options' => array(
      '1' => t('1st level (primary)'),
      '2' => t('2nd level (secondary)'),
      '3' => t('3rd level (tertiary)'),
      '4' => t('4th level'),
      '5' => t('5th level'),
      '6' => t('6th level'),
      '7' => t('7th level'),
      '8' => t('8th level'),
      '9' => t('9th level'),
    ),
    '#description' => t('Blocks that start with the 1st level will always be visible. Blocks that start with the 2nd level or deeper will only be visible when the trail to the active menu item passes though the block’s starting level.'),
  );
  // The value of "follow" in the database/config array is either FALSE or the
  // value of the "follow_parent" form element.
  if ($follow = $config['follow']) {
    $follow_parent = $follow;
    $follow = 1;
  }
  else {
    $follow_parent = 'active';
  }
  $form['follow'] = array(
    '#type' => 'checkbox',
    '#title' => t('Make the starting level follow the active menu item.'),
    '#default_value' => $follow,
    '#description' => t('If the active menu item is deeper than the level specified above, the starting level will follow the active menu item. Otherwise, the starting level of the tree will remain fixed.'),
    '#element_validate' => array('menu_block_configure_form_follow_validate'),
  );
  $form['follow_parent'] = array(
    '#type' => 'select',
    '#title' => t('Starting level will be'),
    '#default_value' => $follow_parent,
    '#options' => array(
      'active' => t('Active menu item'),
      'child' => t('Children of active menu item'),
    ),
    '#description' => t('When following the active menu item, specify if the starting level should be the active menu item or its children.'),
    '#states' => array(
      'visible' => array(
        ":input[name='block_settings[follow]']" => array('checked' => TRUE),
      ),
    ),
  );
  $form['depth'] = array(
    '#type' => 'select',
    '#title' => t('Maximum depth'),
    '#default_value' => $config['depth'],
    '#options' => array(
      '1' => '1',
      '2' => '2',
      '3' => '3',
      '4' => '4',
      '5' => '5',
      '6' => '6',
      '7' => '7',
      '8' => '8',
      '9' => '9',
      '0' => t('Unlimited'),
    ),
    '#description' => t('From the starting level, specify the maximum depth of the menu tree.'),
  );
  $form['expanded'] = array(
    '#type' => 'checkbox',
    '#title' => t('<strong>Expand all children</strong> of this tree.'),
    '#default_value' => $config['expanded'],
  );
  $form['sort'] = array(
    '#type' => 'checkbox',
    '#title' => t('<strong>Sort</strong> menu tree by the active menu item’s trail.'),
    '#default_value' => $config['sort'],
    '#description' => t('Sort each item in the active trail to the top of its level. When used on a deep or wide menu tree, the active menu item’s children will be easier to see when the page is reloaded.'),
  );
  $form['parent'] = array(
    '#type' => 'select',
    '#title' => t('Fixed parent item'),
    '#default_value' => $config['menu_name'] . ':' . $config['parent_mlid'],
    '#options' => menu_parent_options($menus, array('mlid' => 0)),
    '#description' => t('Alter the “starting level” and “maximum depth” options to be relative to the fixed parent item. The tree of links will only contain children of the selected menu item.'),
    '#attributes' => array('class' => array('menu-block-parent-mlid')),
    '#element_validate' => array('menu_block_configure_form_parent_validate'),
  );
  $form['parent']['#options'][MENU_TREE__CURRENT_PAGE_MENU . ':0'] = '<' . t('the menu selected by the page') . '>';
  $form['menu-block-wrapper-close'] = array('#markup' => '</div>');

  // Set visibility of advanced options.
  foreach (array('title_link', 'follow', 'follow_parent', 'expanded', 'sort', 'parent') as $key) {
    $form[$key]['#states']['visible'][':input[name="block_settings[display_options]"]'] = array('value' => 'advanced');
  }
  if ($config['title_link'] || $follow || $config['expanded'] || $config['sort'] || $config['parent_mlid']) {
    $form['display_options']['#default_value'] = 'advanced';
  }
    $form['#validate'][] = 'node_search_validatev';

  return $form;
}
 
/**
 * Menu callback: admin settings form.
 *
 * @return
 *   The settings form used by Menu block.
 */
function menu_block_admin_settings_form($form, &$form_state) {
  // Retrieve core's menus.
  $menus = menu_get_menus();
  // Retrieve all the menu names provided by hook_menu_block_get_sort_menus().
  $menus = array_merge($menus, module_invoke_all('menu_block_get_sort_menus'));
  asort($menus);

  // Load stored configuration.
  $menu_order = config_get('menu_block.settings', 'menu_block_menu_order');
  // Remove any menus no longer in the list of all menus.
  foreach (array_keys($menu_order) as $menu) {
    if (!isset($menus[$menu])) {
      unset($menu_order[$menu]);
    }
  }

  // Merge the saved configuration with any un-configured menus.
  $all_menus = $menu_order + $menus;

  $form['heading'] = array(
    '#markup' => '<p>' . t('If a block is configured to use <em>"the menu selected by the page"</em>, the block will be generated from the first "available" menu that contains a link to the page.') . '</p>',
  );

  // Orderable list of menu selections.
  $form['menu_order'] = array(
    '#tree' => TRUE,
    '#theme' => 'menu_block_menu_order',
  );

  $order = 0;
  $total_menus = count($all_menus);
  foreach (array_keys($all_menus) as $menu_name) {
    $form['menu_order'][$menu_name] = array(
      'title' => array(
        '#markup' => check_plain($menus[$menu_name]),
      ),
      'available' => array(
        '#type' => 'checkbox',
        '#attributes' => array('title' => t('Select from the @menu_name menu', array('@menu_name' => $menus[$menu_name]))),
        '#default_value' => isset($menu_order[$menu_name]),
      ),
      'weight' => array(
        '#type' => 'weight',
        '#default_value' => $order - $total_menus,
        '#delta' => $total_menus,
        '#id' => 'edit-menu-block-menus-' . $menu_name,
      ),
    );
    $order++;
  }

  $form['footer_note'] = array(
    '#markup' => '<p>' . t('The above list will <em>not</em> affect menu blocks that are configured to use a specific menu.') . '</p>',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

/**
 * Form submission handler.
 */
function menu_block_admin_settings_form_submit($form, &$form_state) {
  $menu_order = array();
  foreach ($form_state['values']['menu_order'] as $menu_name => $row) {
    if ($row['available']) {
      // Add available menu and its weight to list.
      $menu_order[$menu_name] = (int) $row['weight'];
    }
  }
  // Sort the keys by the weight stored in the value.
  asort($menu_order);
  foreach ($menu_order as $menu_name => $weight) {
    // Now that the array is sorted, the weight is redundant data.
    $menu_order[$menu_name] = '';
  }
  config_set('menu_block.settings', 'menu_block_menu_order', $menu_order);
  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Theme a drag-to-reorder table of menu selection checkboxes.
 */
function theme_menu_block_menu_order($variables) {
  $element = $variables['element'];
  drupal_add_tabledrag('menu-block-menus', 'order', 'sibling', 'menu-weight');

  $variables = array(
    'header' => array(
      t('Menu'),
      t('Available'),
      t('Weight'),
    ),
    'rows' => array(),
    'attributes' => array('id' => 'menu-block-menus'),
  );

  // Generate table of draggable menu names.
  foreach (element_children($element) as $menu_name) {
    $element[$menu_name]['weight']['#attributes']['class'] = array('menu-weight');
    $variables['rows'][] = array(
      'data' => array(
        drupal_render($element[$menu_name]['title']),
        drupal_render($element[$menu_name]['available']),
        drupal_render($element[$menu_name]['weight']),
      ),
      'class' => array('draggable'),
    );
  }

  return theme('table', $variables);
}
