<?php

use google\appengine\api\taskqueue\PushTask;

/**
 * Drupal queue implementation for App Engine Push Task Queues.
 *
 * Only implement createItem() since Push Task Queues do not support any direct
 * access to items within the queue. In the future it would seem to make sense
 * for the push queue implementation to extend the pull task queues
 * implementation and disable the non-supported operations. Since App Engine
 * does not currently provide a built-in implementation extending is not yet
 * done, but it would seem possible to use the google-api-php-client
 * Google_TaskqueueService.php if interested.
 */
class DrupalGoogleAppenginePushTaskQueue implements DrupalQueueInterface {
  /**
   * @var string
   */
  protected $name;

  /**
   * @param string $name
   *   The name of the queue.
   */
  public function __construct($name) {
    $this->name = $name;
  }

  public function createItem($data) {
    $task = new PushTask(
      '/system/queue/' . $this->name,
      ['data' => serialize($data)]);
    $task->add();
  }

  public function numberOfItems() {
    throw new RuntimeException('Push task queues do not support numberOfItems().');
  }

  public function claimItem($lease_time = 3600) {
    // Invoked by drupal_cron_run() so this must not throw exception.
    return FALSE;
  }

  public function deleteItem($item) {
    throw new RuntimeException('Push task queues do not support deleteItem().');
  }

  public function releaseItem($item) {
    throw new RuntimeException('Push task queues do not support releaseItem().');
  }

  public function createQueue() {
    // Intended to be implemented as a noop if queue already exists. Since
    // the App Engine 'default' queue is the only one supported and always
    // exists it is accurate to allow calling code to assume created.
  }

  public function deleteQueue() {
    throw new RuntimeException('Push task queues do not support deleteQueue().');
  }
}
