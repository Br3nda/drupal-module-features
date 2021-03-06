<?php
// $Id: features.drush.inc,v 1.1 2009/03/13 21:59:23 yhahn Exp $

/**
 * @file
 *   Feature module drush integration.
 */

/**
 * Implementation of hook_drush_command().
 * 
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function features_drush_command() {
  $items = array();

  $items['status features'] = array(
    'callback' => 'features_command_list',
    'description' => "List all the available features for your site.",
  );
  $items['features export'] = array(
    'callback' => 'features_command_export',
    'description' => "Export a feature from your site into a module.",
    'arguments' => array(
      'feature' => 'Feature name to export / update.',
    ),
  );

  return $items;
}

/**
 * Implementation of hook_drush_help().
 */
function example_drush_help($section) {
  switch ($section) {
    case 'drush:state list':
      return dt("List all the available features for your site.");
    case 'drush:features export':
      return dt("Export a feature from your site into a module.");
  }
}

/**
 * Get a list of all feature modules.
 */
function features_command_list() {
  $rows = array(array(dt('Name'), dt('Enabled/Disabled'), dt('From'), dt('Timestamp')));
  foreach (features_get_modules() as $k => $m) {
    if ($m->info['feature']) {
      $rows[] = array(
        $m->name,
        $m->status ? dt('Enabled') : dt('Disabled'),
        $m->info['feature_uri'],
        format_date($m->info['feature_timestamp']));
    }
  }
  drush_print_table($rows, 2, true);
}

/**
 * Create a feature module based on a list of contexts.
 */
function features_command_export() {
  $args = func_get_args();

  if (count($args) == 1) {
    // Assume that the user intends to create a module with the same name as the
    // "value" of the context.
    $context = array_shift($args);
    _features_command_export(array($context));
  }
  elseif (count($args) > 1) {
    // Assume that the user intends to create a new module based on a list of 
    // contexts. First argument is assumed to be the name.
    $name = array_shift($args);
    _features_command_export($args, $name);
  }
  else {
    // By default just show contexts that are available.
    $rows = array(array(dt('Available contexts')));
    foreach (context_enabled_contexts() as $k => $c) {
      $rows[] = array("{$c->namespace}-{$c->attribute}-{$c->value}");
    }
    drush_print_table($rows, 2, true);
  }
}

/**
 * Write a module to the site dir.
 *
 * @param $requests
 *   An array of the context requested in this export.
 * @param $name 
 *  Optional. The name for the exported module, it omitted will be generated
 *  from the first listed context.
 */
function _features_command_export($requests, $name = null) {
  module_load_include('inc', 'features', 'features.export');
  $contexts = context_enabled_contexts();
  $export = array();
  foreach ($requests as $c) {
    if (!isset($contexts[$c])) {
      drush_die(dt('Couldn\'t generate module based on that context, which doesn\'t seem to exist'));
    }
    if (!isset($meta)) {
      $meta = array(
        'name' => ($name ? $name :$contexts[$c]->value),
        'description' => $contexts[$c]->description,
        'filename' => ($name ? $name :$contexts[$c]->value),
      );
    }
    $stub = array(
      'context' => array("{$contexts[$c]->namespace}-{$contexts[$c]->attribute}-{$contexts[$c]->value}"),
    );
    $e = features_populate($stub, $module_name);
    $export = array_merge_recursive($export, $e);
  }

  list($info, $module) = features_export_render($export, $meta);

  // Currently you cannot establish your own options, so this won't work as
  // a hook needs to be added. See drush_get_options().
  $dest = drush_get_option(array('d', 'destination'), 'all');

  if ($root = drush_locate_root()) {
    $directory = "$root/sites/$dest/modules/{$meta[filename]}";
    if (is_dir($directory)) {
      drush_print(dt('Module appears to already exist in !dir', array('!dir' => $directory)));
      if(!drush_confirm(dt('Do you really want to continue?'))) {
        drush_die('Aborting.');
      }
    }
    else {
      drush_op('mkdir', $directory);
    }
    drush_op('file_put_contents', $directory. '/'. $meta['filename'] . '.info', $info);
    drush_op('file_put_contents', $directory. '/'. $meta['filename'] . '.module', $module);
    drush_print(dt("Created module: !module in !directory", array('!module' => $meta['filename'], '!directory' => $directory)));
  }
  else{
    drush_die(dt('Couldn\'t locate site root'));
  }
}