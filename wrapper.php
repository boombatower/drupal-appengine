<?php
/**
 * @file
 * Provide mod_rewrite like functionality and correct $_SERVER['SCRIPT_NAME'].
 *
 * Pass through requests for root php files and forward all other requests to
 * index.php with $_GET['q'] equal to path. In terms of how the requests will
 * seem please see the following examples.
 *
 * - /install.php: install.php
 * - /update.php?op=info: update.php?op=info
 * - /foo/bar: index.php?q=/foo/bar
 * - /: index.php?q=/
 */

$path = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);

// Provide mod_rewrite like functionality. If a php file in the root directory
// is explicitly requested then load the file, otherwise load index.php and
// set get variable 'q' to $_SERVER['REQUEST_URI'].
if (dirname($path) == '/' && pathinfo($path, PATHINFO_EXTENSION) == 'php') {
  $file = pathinfo($path, PATHINFO_BASENAME);
}
else {
  $file = 'index.php';

  // Provide mod_rewrite like functionality by using the path which excludes
  // any other part of the request query (ie. ignores ?foo=bar).
  $_GET['q'] = $path;
}

// Override the script name to simulate the behavior without wrapper.php.
// Ensure that $_SERVER['SCRIPT_NAME'] always begins with a / to be consistent
// with HTTP request and the value that is normally provided (not what GAE
// currently provides).
$_SERVER['SCRIPT_NAME'] = '/' . $file;
require $file;
