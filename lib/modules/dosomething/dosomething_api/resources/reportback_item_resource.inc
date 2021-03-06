<?php

function _reportback_item_resource_definition() {
  $reportback_item_resource = [];
  $reportback_item_resource['reportback-items'] = [
    'operations' => [

      'retrieve' => [
        'help' => 'Retrieve a specified reportback item.',
        'file' => [
          'type' => 'inc',
          'module' => 'dosomething_api',
          'name' => 'resources/reportback_item_resource',
        ],
        'callback' => '_reportback_item_resource_retrieve',
        'args' => [
          [
            'name' => 'fid',
            'description' => 'The fid of the reportback item to retrieve.',
            'optional' => FALSE,
            'type' => 'int',
            'source' => array('path' => 0),
          ],
        ],
        'access callback' => '_reportback_item_resource_access',
        'access arguments' => array('view'),
      ],

      'index' => [
        'help' => 'List all reportbacks items.',
        'file' => [
          'type' => 'inc',
          'module' => 'dosomething_api',
          'name' => 'resources/reportback_item_resource'
        ],
        'callback' => '_reportback_item_resource_index',
        'args' => [
          [
            'name' => 'campaigns',
            'description' => 'The ids of specified campaigns to get reportback items.',
            'optional' => TRUE,
            'type' => 'string',
            'source' => array('param' => 'campaigns'),
            'default value' => NULL,
          ],
          [
            'name' => 'exclude',
            'description' => 'Exclude reportbacks from results based on specific ids.',
            'optional' => TRUE,
            'type' => 'string',
            'source' => ['param' => 'exclude'],
            'default value' => NULL,
          ],
          [
            'name' => 'status',
            'description' => 'Comma delimited list of reportback statuses to collect reportback items for.',
            'optional' => TRUE,
            'type' => 'string',
            'source' => array('param' => 'status'),
            'default value' => 'promoted,approved',
          ],
          [
            'name' => 'count',
            'description' => 'The number of reportback items to retrieve.',
            'optional' => TRUE,
            'type' => 'string',
            'source' => array('param' => 'count'),
            'default value' => 25,
          ],
          [
            'name' => 'random',
            'description' => 'Boolean to indicate whether to retrieve reportback items in random order.',
            'optional' => TRUE,
            'type' => 'boolean',
            'source' => array('param' => 'random'),
            'default value' => FALSE,
          ],
          [
            'name' => 'page',
            'description' => 'The zero-based index of the page to get, defaults to 0.',
            'optional' => TRUE,
            'type' => 'int',
            'source' => array('param' => 'page'),
            'default value' => 1,
          ],
          [
            'name' => 'load_user',
            'description' => 'Flag to indicate whether to make call to northstar to retrieve full user data.',
            'optional' => TRUE,
            'type' => 'boolean',
            'source' => array('param' => 'load_user'),
            'default value' => FALSE,
          ],
          [
            'name' => 'as_user',
            'description' => 'Override the current global user',
            'optional' => TRUE,
            'type' => 'string',
            'source' => ['param' => 'as_user'],
            'default_value' => NULL,
          ],
        ],
        'access callback' => '_reportback_item_resource_access',
        'access arguments' => array('index'),
      ],

    ],

  ];

  return $reportback_item_resource;
}


/**
 * Determine whether the current user can access campaign resource.
 *
 * @param string $op
 *
 * @return bool
 */
function _reportback_item_resource_access($op) {
  if ($op === 'view') {
    return TRUE;
  }

  if ($op === 'index') {
    return TRUE;
  }

  return FALSE;
}

/**
 * Retrieve and show index list response of reportbacks requested.
 *
 * @param  string  $campaigns
 * @param  string  $exclude
 * @param  string  $status
 * @param  int     $count
 * @param  bool    $random
 * @param  int     $page
 * @param  bool    $load_user
 * @param  string  $as_user
 * @return array
 */
function _reportback_item_resource_index($campaigns, $exclude, $status, $count, $random, $page, $load_user, $as_user) {
  global $user;

  $parameters = [
    'campaigns' => $campaigns,
    'exclude' => $exclude,
    'status' => $status,
    'count' => $count,
    'random' => $random,
    'page' => $page,
    'load_user' => $load_user,
    'as_user' => $as_user,
  ];

  // Fetch reportback items from Rogue.
  $response = dosomething_rogue_client()->getAllReportbacks([
    'filter' => [
      'campaign_id' => $campaigns,
      'exclude' => $exclude,
      'status' => 'accepted',
    ],
    // If `?as_user` provided with UID or Northstar ID, use that. Otherwise, send logged-in user's NS ID.
    'as_user' => dosomething_user_convert_to_northstar_id($as_user ? $as_user : $user->uid),
    'limit' => $count,
    'page' => $page,
  ]);

  $response['data'] = collect($response['data'])->map(function ($item) use ($load_user) {
    // Add campaign to each item:
    try {
      $campaign = Campaign::get($item['campaign']['id']);
      $item['campaign'] = $campaign;
    } catch (Exception $e) {
      // Oh well!
    }

    // If requested, add full user to each item:
    if ($load_user) {
      $profile = dosomething_northstar_get_user($item['user']['id']);

      $item['user'] = [
        'id' => $profile->id,
        'drupal_id' => $profile->drupal_id,
        'first_name' => $profile->first_name,
        'last_initial' => $profile->last_initial,
        'photo' => $profile->photo,
        'country' => $profile->country,
      ];
    }

    return $item;
  });

  return $response;
}


function _reportback_item_resource_retrieve($fid) {
  $reportbackItems = new ReportbackItemTransformer;
  return $reportbackItems->show($fid);
}
