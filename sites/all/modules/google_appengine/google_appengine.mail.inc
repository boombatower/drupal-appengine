<?php

/**
 * @file
 * Drupal mail system implementation for Google App Engine.
 */

use google\appengine\api\mail\Message;

class GoogleAppenegineMailSystem implements MailSystemInterface {
  /**
   * An instance of DefaultMailSystem for use in format().
   *
   * @var DefaultMailSystem
   */
  private $defaultMailSystem;

  /**
   * @see google\appengine\api\mail\BaseMessage
   */
  public static $allowedHeaders = [
    'auto-submitted', 'in-reply-to', 'list-id', 'list-unsubscribe',
    'on-behalf-of', 'references', 'resent-date', 'resent-from', 'resent-to',
  ];

  /**
   * Instantiate the GAE mail system.
   */
  public function __construct() {
    $this->defaultMailSystem = new DefaultMailSystem();
  }

  /**
   * @see DefaultMailSystem::format()
   */
  public function format(array $message) {
    return $this->defaultMailSystem->format($message);
  }

  /**
   * Send a message using the GAE interface.
   *
   * @see MailSystemInterface::mail()
   */
  public function mail(array $message) {
    $this->filterHeaders($message['headers']);

    // Fill GAE Message class with values form $message array.
    try {
      $gae = new Message();
      $gae->setSender($message['from']);
      $gae->addHeaderArray($message['headers']);
      $gae->addTo($message['to']);
      $gae->setSubject($message['subject']);

      $line_endings = variable_get('mail_line_endings', MAIL_LINE_ENDINGS);
      $gae->setHtmlBody(
        preg_replace('@\r?\n@', $line_endings, $message['body']));
    }
    catch (InvalidArgumentException $e) {
      watchdog('google_appengine', $e->getMessage(), [], WATCHDOG_ERROR);
      return FALSE;
    }

    // Send the message.
    try {
      $gae->send();
    } catch (Exception $e) {
      watchdog('google_appengine', $e->getMessage(), [], WATCHDOG_ERROR);
      return FALSE;
    }

    return TRUE;
  }

  /**
   * Remove headers that are not in the whitelist provided by GAE.
   *
   * @param $headers
   *   An associative array of headers which will be filtered.
   */
  public static function filterHeaders(array &$headers) {
    // Lower-case all incoming headers and remove those not in the whitelist.
    $headers = array_change_key_case($headers);
    $headers = array_intersect_key($headers,
      array_flip(static::$allowedHeaders));
  }
}
