<?php
$path = parse_url($_SERVER['REQUEST_URI'], PHP_URL_PATH);

// Provide mod_rewrite like functionality. If a php file in the root directory
// is explicitely requested then load the file, otherwise load index.php and
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
