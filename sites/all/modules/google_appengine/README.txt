Optimize your Drupal installation for the Google App Engine platform.

- App Engine mail service

    Implements Drupal MailSystemInterface to make use of the App Engine mail
    service. The system email address will be used as the default sender
    address and must be authorized to send mail. To configure the address visit
    admin/config/system/site-information and for details on App Engine mail
    service see https://developers.google.com/appengine/docs/php/mail/.

- Cloud Storage

    Implements DrupalStreamWrapperInterface and overrides the three stream
    wrappers provided by Drupal core (private://, public://, temporary://). The
    storage bucket must be configured (see below) in order for the GCS
    integration to function properly. The standard mechanisms for controlling
    the file system setup (admin/config/media/file-system) can be used and
    file fields can be stored within one of the default stream wrappers.

    File MIME types are determined by DrupalLocalStreamWrapper::getMimeType()
    which consults file_mimetype_mapping() for a mapping of extensions to MIME
    types. The type is included in the stream context when writing files to GCS
    and as such the file will be served with the assigned MIME type.

- Task Queues

    Implements DrupalQueueInterface to be backed by App Engine Push Queues. The
    PHP implementation provided by App Engine does not currently support Pull
    Queues which limits the use. Instead of being able to back all Drupal queues
    only those defined by hook_cron_queue_info() can be backed by App Engine
    since they are implemented in a manor compatible with Push Queues. Enable
    the 'App Engine - Push Task Queues' module which will swap the
    implementation for all empty cron queues. If a cron queue has items simply
    run cron until it is empty and then clear cache and the implementation will
    be swapped. To know if you hit this case simply check watchdog log.

- Drupal core patch (root/core.patch)

    In order to have Drupal run properly on Google App Engine a few changes
    need to be made to Drupal core. Those changes can be found in
    root/core.patch which is managed in
    https://github.com/boombatower/drupal-appengine (7.x-appengine branch) and
    rebased on top of Drupal core updates. There are three other files contained
    within root/ directory that need to be placed in the Drupal root which are
    contain within the patch.

    - Add app.yaml to root which provides basic information about the app to
      Google App Engine so that it can invoke Drupal properly.
    - Alters drupal_move_uploaded_file() in includes/file.inc to support newly
      uploaded files from the $_FILES array being referenced via a stream
      wrapper. In the case of App Engine all uploaded files are uploaded through
      the GCS proxy, hosted on GCS, and thus start with gs://. The change is
      should be generally useful and has been rolled as a core patch in
      https://drupal.org/node/2114885.
    - Alters file_upload_max_size() in includes/file.inc to only check PHP ini
      setting 'upload_max_filesize' instead of also checking 'post_max_size'
      which is normally relevant, but in the case of App Engine is not since all
      uploads are sents through GCS proxy and are thus not affected by app
      instance post limits.
    - Alters drupal_tempnam() in includes/file.inc to manually simulate
      tempnam() since it is currently not supported by App Engine.
    - Alters system_file_system_settings() in modules/system/system.admin.inc
      to include #wrapper_scheme property to be picked up by
      system_check_directory() in modules/system/system.module. Given that the
      current code voids using the stream wrappers this is technically a bug and
      is a candidate for being fixed in Drupal core as well.
    - Alters system_requirements() in modules/system/system.install to skip the
      directory check since the GCS integration will not be loaded until the App
      Engine module is enabled.
    - Add php.ini to root which enables some php functions used by Drupal and
      turns on output buffering.
    - Adds wrapper.php to root which simulates Apache mod_rewrite like behavior.

- Drupal core overrides

  - drupal_http_request() is overriden using the variable
    'drupal_http_request_function' to call a function defined in the App Engine
    module. The altered drupal_http_request() works without requiring socket
    support. The changes are maintain by rolling core/drupal_http_request.patch
    forward with core changes.
  - Cron may be overriden to use the App Engine cron mechanism. Please see
    cron.yaml (patched into root of Drupal) for details on how to configure. For
    further details on App Engine's cron mechanism see the documentation,
    https://developers.google.com/appengine/docs/php/config/cron.

INITIAL SETUP
=============

All the changes can be applied using root/core.patch manually, using the
included drush make file (recommended, see https://github.com/drush-ops/drush),
or the entire tree with everything included downloaded from github.

  https://github.com/boombatower/drupal-appengine/releases (pick latest release)

If Drush make is preferred use either the drupal-full.make which includes other
recommended modules or drupal.make which includes core and this module.

  drush make http://drupalcode.org/project/google_appengine.git/blob_plain/refs/heads/7.x-1.x:/root/drupal-full.make
  drush make http://drupalcode.org/project/google_appengine.git/blob_plain/refs/heads/7.x-1.x:/root/drupal.make

Or, applied manually as follows.

  wget http://drupalcode.org/project/google_appengine.git/blob_plain/refs/heads/7.x-1.x:/root/core.patch
  git apply core.patch.

Otherwise, download Drupal core App Engine repository directly and place the
Drupal module inside.

  git clone --branch 7.x-appengine https://github.com/boombatower/drupal-appengine

DEVELOPMENT SERVER
==================

While building out your site you can use the development server to simulate an
App Engine environment on your local machine. Once the site is running locally
it can be uploaded to App Engine and the database imported to Cloud SQL.

See the instructions at the following URL for details.
  https://developers.google.com/appengine/docs/php/tools/devserver

SETTINGS.PHP
============

When developing locally and uploading to App Engine it can be annoying to have
to change the settings.php file to reflect the different database connection
details needed in the two environments. As such it is recommended to use a
conditional statement as shown below in settings.php.

if(strpos($_SERVER['SERVER_SOFTWARE'], 'Google App Engine') !== false) {
  // App Engine database credentials.
  $databases['default']['default'] = array(
    'database' => '{DATABASE}',
    'username' => 'root',
    'password' => '',
    'unix_socket' => '/cloudsql/{SOME_PROJECT}:{DATABASE}',
    'port' => '',
    'driver' => 'mysql',
    'prefix' => '',
  );
}
else {
  // Local database credentials.
  $databases['default']['default'] = array(
    'database' => '{DATABASE}',
    'username' => '{USERNAME}',
    'password' => '{PASSWORD}',
    'host' => 'localhost',
    'port' => '',
    'driver' => 'mysql',
    'prefix' => '',
  );
}

The following settings are also available for controlling the App Engine
integration.

$conf['google_appengine_default_storage_bucket'] = '';
$conf['google_appengine_default_storage_cname'] = FALSE;
$conf['google_appengine_aggregate'] = 'proxy'; // (static|proxy|gcs)

These settings may also be changed from the web interface at the following
paths.

- admin/config/media/file-system
- admin/config/development/performance

CONFIGURE GOOGLE CLOUD STORAGE (GCS) BUCKET
===========================================

In order to start using GCS the default bucket needs to be configured. Either
visit admin/config/media/file-system page or configure
'google_appengine_default_storage_bucket' in the site settings.php file.

See the documentation at the following URL for details.
  https://developers.google.com/appengine/docs/php/googlestorage

UPLOADING APP
=============

See the instructions at the following URL for details.
  https://developers.google.com/appengine/docs/php/tools/uploadinganapp

AGGREGATE CSS & JS
==================

Since a local writable file-system is not available on Google App Engine, for
various reasons, the ability for Drupal to aggregate CSS and JS into combined
files is restricted. There are three possible choices.

- Directly from static files (recommended, but requires proper setup)

    Serving from static files requires that the aggregate files be uploaded
    with the app. There are a couple of ways to achieve this some of which are
    better than others.

    - Build site locally using the development server and generate the files
      locally. During upload the aggregate files will be present and included
      with app.
    - Upload app and generate the files while running on App Engine and written
      to GCS. Download the files locally into the app and re-upload. This method
      means that your app may serve with out-of-date CSS or JS until you
      re-upload which can cause all sort of issues.

      gsutil makes it easy to download the css and js files from GCS.
        https://developers.google.com/storage/docs/gsutil

      Run the following with the relevant values filled in.

      ./gsutil cp -R gs://{BUCKET}/sites/default/files/css {~/path/to/drupal}/sites/default/files/
      ./gsutil cp -R gs://{BUCKET}/sites/default/files/js {~/path/to/drupal}/sites/default/files/

- From GCS using Drupal router as proxy (default)

    By default, aggregate files are served via a Drupal router which acts as a
    GCS proxy. The proxy should always work without any additional
    configuration, but this will consume instance hours for serving static
    aggregate resources.

- Directly from GCS

    Serving directly from GCS does not require uploading static files with the
    app, but can cause difficulties since resources referenced from CSS will
    need to be uploaded to GCS as well (or referenced using an absolute URL).
    Also note that that the CSS and JS files will be served from a different
    domain which may cause complications.

SUPPORT
=======

- For general App Engine (PHP) support please visit:
  http://stackoverflow.com/questions/tagged/google-app-engine+php
- For issues specific to this Drupal module please visit:
  https://drupal.org/project/issues/google_appengine
