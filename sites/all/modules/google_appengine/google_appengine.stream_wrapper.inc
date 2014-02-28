<?php

/**
 * @file
 * Drupal stream wrapper implementation for Google Cloud Storage.
 *
 * Given that CloudStorageStreamWrapper class is final it cannot be extended to
 * provide the functionality required by Drupal. Instead
 * CloudStorageStreamWrapperTrait is defined to wrap the stream wrapper and
 * allow the trait to be extended a reused.
 *
 * Since the already provided class functions the trait does not need to be
 * instantiated and used for the basic stream wrapper. A number of classes and
 * traits are defined to allow the stream wrapper to be used at any level and in
 * a variety of ways. Much of this code is intended to be useful outside of
 * Drupal and perhaps upstream in the original stream wrapper.
 *
 * A summary of the hierarchy provided is presented below.
 *
 * trait CloudStorageStreamWrapperTrait
 *
 * class CloudStorageBucketlessStreamWrapper
 *     implements StreamWrapperInterface
 *   use CloudStorageStreamWrapperTrait
 *
 *
 * trait DrupalCloudStorageStreamWrapperTrait
 *
 * class DrupalCloudStorageStreamWrapper
 *     implements DrupalStreamWrapperInterface
 *   use CloudStorageStreamWrapperTrait;
 *   use DrupalCloudStorageStreamWrapperTrait;
 *
 * class DrupalCloudStorageBucketlessStreamWrapper
 *     extends CloudStorageBucketlessStreamWrapper
 *     implements DrupalStreamWrapperInterface
 *   use DrupalCloudStorageStreamWrapperTrait;
 *
 *
 * abstract class DrupalLocalCloudStorageStreamWrapper
 *     extends DrupalCloudStorageBucketlessStreamWrapper
 *
 * class DrupalPublicCloudStorageStreamWrapper
 *     extends DrupalLocalCloudStorageStreamWrapper
 * class DrupalPrivateCloudStorageStreamWrapper
 *     extends DrupalLocalCloudStorageStreamWrapper
 * class DrupalTemporaryCloudStorageStreamWrapper
 *     extends DrupalLocalCloudStorageStreamWrapper
 */

use google\appengine\ext\cloud_storage_streams\CloudStorageStreamWrapper;

/**
 * Basic wrapper trait around CloudStorageStreamWrapper provided by GAE.
 *
 * Forwards all calls to CloudStorageStreamWrapper, but keeps track of URI,
 * bucket, and object. Incoming URIs can also be altered by overriding
 * alterUri() method. The URI property will still contain the original URI
 * as that will be the URI understood from outside. The altered URI purely
 * passed through to the underlying CloudStorageStreamWrapper.
 *
 * If instantiated as a class the trait would function identically to built in
 * stream wrapper provided by GAE.
 *
 * The CloudStorageStreamWrapper implements the StreamWrapperInterface, but
 * since PHP itself does not define a formal interface the class does not
 * explicitly implement the interface.
 *
 * @see CloudStorageStreamWrapper
 */
trait CloudStorageStreamWrapperTrait {
  /**
   * Instance of CloudStorageStreamWrapper to which calls will be forwarded.
   *
   * @var Resource
   */
  protected $stream;

  /**
   * Instance URI (stream).
   *
   * A stream is referenced as "scheme://bucket/object".
   *
   * @var string
   */
  protected $uri;

  /**
   * Google Cloud Storage bucket.
   *
   * @var string
   */
  protected $bucket;

  /**
   * Google Cloud Storage object (file or directory).
   *
   * @var string
   */
  protected $object;

  /**
   * Instantiate cloud storage wrapper.
   */
  public function __construct() {
    $this->stream = new CloudStorageStreamWrapper();
  }

  public function stream_open($uri, $mode, $options, &$opened_url) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->stream_open($uri, $mode, $options, $opened_url);
  }

  public function stream_close() {
    return $this->stream->stream_close();
  }

  public function stream_lock($operation) {
    return $this->stream->stream_lock($operation);
  }

  public function stream_read($count) {
    return $this->stream->stream_read($count);
  }

  public function stream_write($data) {
    return $this->stream->stream_write($data);
  }

  public function stream_eof() {
    return $this->stream->stream_eof();
  }

  public function stream_seek($offset, $whence) {
    return $this->stream->stream_seek($offset, $whence);
  }

  public function stream_flush() {
    return $this->stream->stream_flush();
  }

  public function stream_tell() {
    return $this->stream->stream_tell();
  }

  public function stream_stat() {
    return $this->stream->stream_stat();
  }

  public function unlink($uri) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->unlink($uri);
  }

  public function rename($from_uri, $to_uri) {
    $from_uri = $this->alterUri($from_uri);
    $to_uri = $this->alterUri($to_uri);
    return $this->stream->rename($from_uri, $to_uri);
  }

  public function mkdir($uri, $mode, $options) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->mkdir($uri, $mode, $options);
  }

  public function rmdir($uri, $options) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->rmdir($uri, $options);
  }

  public function url_stat($uri, $flags) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->url_stat($uri, $flags);
  }

  public function dir_opendir($uri, $options) {
    $this->setUri($uri);
    $uri = $this->alterUri($uri);
    return $this->stream->dir_opendir($uri, $options);
  }

  public function dir_readdir() {
    return $this->stream->dir_readdir();
  }

  public function dir_rewinddir() {
    return $this->stream->dir_rewinddir();
  }

  public function dir_closedir() {
    return $this->stream->dir_closedir();
  }

  /**
   * Set the absolute stream resource URI.
   *
   * Generally, only called by the factory method and internally.
   *
   * Since alterUri() is invoked the bucket and object will reflect the real
   * values instead of the stream wrapper URI. For example if foo://bar is
   * passed as the URI and alterUri() returns foo://bucket/dir/bar then
   * getBucket() will return 'bucket' and getObject() will return 'dir/bar'
   * while getUri() will return foo://bar.
   *
   * @param string $uri
   *   A string containing the URI that should be used for this instance.
   *
   * @see GoogleCloudStorageStreamWrapperTrait::setBucket()
   * @see GoogleCloudStorageStreamWrapperTrait::setObject()
   */
  public function setUri($uri) {
    $this->uri = $uri;

    if ($uri_parts = parse_url($this->alterUri($uri))) {
      $this->setBucket($uri_parts['host']);
      if (isset($uri_parts['path'])) {
        $this->setObject($uri_parts['path']);
      }
    }
  }

  /**
   * @return string
   *   The stream resource URI.
   */
  public function getUri() {
    return $this->uri;
  }

  /**
   * Set the Google Cloud Storage bucket.
   *
   * @param string $bucket
   *   The Google Cloud Storage bucket.
   */
  protected function setBucket($bucket) {
    $this->bucket = $bucket;
  }

  /**
   * @return string
   *   The Google Cloud Storage bucket.
   *
   * @see GoogleCloudStorageStreamWrapperTrait::setUri()
   */
  public function getBucket() {
    return $this->bucket;
  }

  /**
   * Set Google Cloud Storage object (should match URI).
   *
   * @param string $object
   *   Google Cloud Storage object (path to file or directory).
   *
   * @see GoogleCloudStorageStreamWrapperTrait::setUri()
   */
  protected function setObject($object) {
    // Remove erroneous leading or trailing, forward-slashes and backslashes.
    $this->object = trim($object, '\/');
  }

  /**
   * @return string
   *   The Google Cloud Storage object.
   */
  public function getObject() {
    return $this->object;
  }

  /**
   * Alter incoming URI before it is passed to underlying stream wrapper.
   *
   * @param string $uri
   *   URI to alter.
   */
  protected function alterUri($uri) {
    return $uri;
  }
}

/**
 * Provide a bucketless stream wrapper that does not require the bucket in URI.
 *
 * The altered form is "scheme://object" and the bucket is taken from
 * google_appengine_default_storage_bucket().
 */
class CloudStorageBucketlessStreamWrapper implements StreamWrapperInterface {
  use CloudStorageStreamWrapperTrait {
    __construct as __constructTrait;
  }

  /**
   * Initialize the trait and set the bucket to default.
   */
  public function __construct() {
    static::__constructTrait();

    $this->setBucket(google_appengine_default_storage_bucket());
  }

  /**
   * Alter the URI to include the bucket property and set scheme to 'gs'.
   */
  protected function alterUri($uri) {
    if ($scheme = explode('://', $uri, 2)[0]) {
      return str_replace($scheme . '://',
        'gs://' . $this->getBucket() . '/', $uri);
    }
    return $uri;
  }
}

/**
 * Implements additional methods required by DrupalStreamWrapperInterface.
 *
 * The setUri() and getUri() methods are not included since they are defined
 * CloudStorageStreamWrapperTrait and will cause a conflict when both traits are
 * used together.
 *
 * @see DrupalStreamWrapperInterface
 */
trait DrupalCloudStorageStreamWrapperTrait {
  /**
   * Use the bucket as a CNAME domain when creating external URL.
   *
   * Default value is taken from Drupal variable
   * 'google_appengine_default_storage_cname'.
   *
   * @var boolean
   */
  protected $cname;

  /**
   * Returns a web accessible URL for the resource.
   *
   * @return string
   *   A string containing a web accessible URL for the resource.
   */
  public function getExternalUrl() {
    // TODO Support mapping per bucket mapping for CNAME.
    if (!isset($this->cname)) {
      $this->cname = (bool) variable_get('google_appengine_default_storage_cname');
    }

    // When using a CNAME redirect the bucket should be the domain of the CNAME
    // record to which the requests will be sent.
    $format = $this->cname ?
      'http://%s/%s' :
      'https://%s.storage.googleapis.com/%s';

    // The googleapis.com URL will only work if the ACL has been set to
    // 'public-read' or equivalent. This method can be overridden to point to
    // custom menu handlers for implementing ACL checks and such.
    // TODO Replace with CloudStorageTools method call once it exists (pending).
    return sprintf($format, $this->getBucket(), $this->getObject());
  }

  /**
   * Returns the MIME type of the resource.
   *
   * @param string $uri
   *   The URI, path, or filename.
   * @param array $mapping
   *   An optional map of extensions to their mimetypes, in the form:
   *    - 'mimetypes': a list of mimetypes, keyed by an identifier,
   *    - 'extensions': the mapping itself, an associative array in which
   *      the key is the extension and the value is the mimetype identifier.
   *
   * @return string
   *   A string containing the MIME type of the resource.
   *
   * @see DrupalLocalStreamWrapper::getMimeType()
   */
  public static function getMimeType($uri, $mapping = NULL) {
    return DrupalLocalStreamWrapper::getMimeType($uri, $mapping);
  }

  /**
   * Set the MIME type in the stream context.
   *
   * Should be invoked during stream_open() since directory handling calls and
   * DrupalStreamWrapperInterface calls do not need the MIME information.
   * Unfortunately, since this is a trait the stream_open() method defined in
   * DrupalCloudStorageStreamWrapperTrait cannot be overridden and must be done
   * in the wrapper classes that use the traits.
   */
  protected function setStreamContextMimeType() {
    // Modify an existing stream context if found, otherwise start anew.
    if (!empty($this->stream->context)) {
      $context = stream_context_get_options($this->stream->context);
    }
    else {
      $context = [];
    }
    $context['gs']['Content-Type'] = static::getMimeType($this->getUri());
    $this->stream->context = stream_context_create($context);
  }

  /**
   * Changes permissions of the resource.
   *
   * @param int $mode
   *   Integer value for the permissions. Consult PHP chmod() documentation
   *   for more information.
   *
   * @return boolean
   *   Returns TRUE on success or FALSE on failure.
   */
  public function chmod($mode) {
    // TODO Translate mode into Google Cloud Storage ACL.
    // Required to be TRUE to function properly.
    return TRUE;
  }

  /**
   * Returns canonical, absolute path of the resource.
   *
   * @return boolean|string
   *   A string with absolute pathname on success (implemented
   *   by core wrappers), or FALSE on failure or if the registered
   *   wrapper does not provide an implementation.
   */
  public function realpath() {
    return FALSE;
  }

  /**
   * Gets the name of the directory from a given path.
   *
   * @param string $uri
   *   An optional URI.
   *
   * @return string
   *   A string containing the directory name, or FALSE if not applicable.
   *
   * @see drupal_dirname()
   * @see DrupalLocalStreamWrapper::dirname()
   */
  public function dirname($uri = NULL) {
    list($scheme, $target) = explode('://', $uri, 2);
    $dirname = dirname($target);

    if ($dirname == '.') {
      $dirname = '';
    }

    return $scheme . '://' . $dirname;
  }
}

/**
 * Provide Drupal stream wrapper for Google Cloud Storage.
 *
 * Overrides the default gs://bucket/object stream wrapper.
 *
 * @see google_appengine_stream_wrappers()
 */
class DrupalCloudStorageStreamWrapper implements DrupalStreamWrapperInterface {
  use CloudStorageStreamWrapperTrait {
    // Cannot use parent to access stream_open(), so rename and call.
    stream_open as stream_open_trait;
  }
  use DrupalCloudStorageStreamWrapperTrait;

  // Cannot be overridden by DrupalCloudStorageStreamWrapperTrait.
  public function stream_open($uri, $mode, $options, &$opened_url) {
    $this->setUri($uri);
    $this->setStreamContextMimeType();
    return $this->stream_open_trait($uri, $mode, $options, $opened_url);
  }
}

/**
 * Provide Drupal stream wrapper for bucketless Google Cloud Storage.
 *
 * Defaultly provided as gsb://object stream wrapper.
 *
 * Does not use CloudStorageStreamWrapperTrait since
 * CloudStorageBucketlessStreamWrapper does and provides bucketless feature.
 *
 * @see google_appengine_stream_wrappers()
 */
class DrupalCloudStorageBucketlessStreamWrapper extends CloudStorageBucketlessStreamWrapper implements DrupalStreamWrapperInterface {
  use DrupalCloudStorageStreamWrapperTrait;

  // Cannot be overridden by DrupalCloudStorageStreamWrapperTrait.
  public function stream_open($uri, $mode, $options, &$opened_url) {
    $this->setUri($uri);
    $this->setStreamContextMimeType();
    return parent::stream_open($uri, $mode, $options, $opened_url);
  }
}

/**
 * Abstract base class for Drupal local stream wrapper replacements.
 *
 * Local storage replacements all use the bucketless stream wrapper since the
 * local filesystem does not have the concept of a bucket and thus provides an
 * equivalent interface.
 */
abstract class DrupalLocalCloudStorageStreamWrapper extends DrupalCloudStorageBucketlessStreamWrapper {
  /**
   * Gets the path that the wrapper is responsible for.
   *
   * @return
   *   A string specifying the path.
   */
  abstract function getDirectoryPath();

  /**
   * Alter the URI to not only include the bucket, but also the directory.
   *
   * @see DrupalCloudStorageBucketlessStreamWrapper::alterUri()
   */
  protected function alterUri($uri) {
    if ($scheme = explode('://', $uri, 2)[0]) {
      return str_replace($scheme . '://', 'gs://' . $this->getBucket() . '/' .
        $this->getDirectoryPath() . '/', $uri);
    }
    return $uri;
  }
}

/**
 * Provide Google Cloud Storage replacement for public files wrapper.
 *
 * @see DrupalPublicStreamWrapper
 */
class DrupalPublicCloudStorageStreamWrapper extends DrupalLocalCloudStorageStreamWrapper {
  public function __construct() {
    parent::__construct();

    // Set the default stream context to be publicly readable so the files can
    // be served directed from GCS instead of having to be read through Drupal.
    $context = [
      'gs' => [
        'acl' => 'public-read',
      ],
    ];
    $this->stream->context = stream_context_create($context);
  }

  /**
   * @see DrupalPublicStreamWrapper::getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('file_public_path', conf_path() . '/files');
  }

  /**
   * Provide a external URL using added public file handler.
   *
   * Use the public serving URL when running on App Engine, otherwise use a
   * Drupal handler when running on dev server. Since the files cannot be served
   * directly as they would if stored locally a handler is necessary in order to
   * read through the Google Cloud storage stream wrapper. In the future this
   * will hopefully be unnecessary since dev appserver will provide a GCS
   * serving handler.
   *
   * Special case aggregated css and js which is served via the method
   * set in the Drupal variable 'google_appengine_aggregate'.
   */
  public function getExternalUrl() {
    if (google_appengine_is_dev_server()) {
      return $this->getExternalUrlDrupalProxy();
    }

    if (preg_match('%^sites/.*/files/(css|js)%', $this->getObject())) {
      $aggregate = variable_get('google_appengine_aggregate', 'proxy');
      if ($aggregate == 'static') {
        return url($this->getObject(), ['absolute' => TRUE]);
      }
      if ($aggregate == 'proxy') {
        return $this->getExternalUrlDrupalProxy();
      }
      // GCS fall through to standard public serving URL.
    }

    // Serve non-aggregated public files normally via GCS directly.
    return parent::getExternalUrl();
  }

  /**
   * @return string
   *   The external URL for accessing the resource via Drupal menu router.
   */
  protected function getExternalUrlDrupalProxy() {
    // Remove the the added directory path [getDirectoryPath()] from the object
    // when serving via the public file router provided by this module since the
    // router is aware of the stream wrapper and will end up adding the path
    // back. This is the same way core private file router works.
    $path = ltrim(str_replace($this->getDirectoryPath(), '', $this->getObject()), '/');
    return url('system/public/' . $path, ['absolute' => TRUE]);
  }
}

/**
 * Provide Google Cloud Storage replacement for private files wrapper.
 *
 * @see DrupalPrivateStreamWrapper
 */
class DrupalPrivateCloudStorageStreamWrapper extends DrupalLocalCloudStorageStreamWrapper {
  /**
   * @see DrupalPrivateStreamWrapper::getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('file_private_path', '');
  }

  /**
   * @see DrupalPrivateStreamWrapper::getExternalUrl()
   */
  public function getExternalUrl() {
    // TODO Support Google Cloud Storage private ACLs to allow direct download.
    $path = str_replace('\\', '/', $this->getObject());
    return url('system/files/' . $path, ['absolute' => TRUE]);
  }
}

/**
 * Provide Google Cloud Storage replacement for temporary files wrapper.
 *
 * @see DrupalTemporaryStreamWrapper
 */
class DrupalTemporaryCloudStorageStreamWrapper extends DrupalLocalCloudStorageStreamWrapper {
  /**
   * @see DrupalTemporaryStreamWrapper::getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('file_temporary_path', file_directory_temp());
  }

  /**
   * @see DrupalTemporaryStreamWrapper::getExternalUrl()
   */
  public function getExternalUrl() {
    $path = str_replace('\\', '/', $this->getObject());
    return url('system/temporary/' . $path, ['absolute' => TRUE]);
  }
}
