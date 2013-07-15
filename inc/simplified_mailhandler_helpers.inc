<?php


/**
 * Return the imap connections and
 * number of messages
 * @param username
 * @param password
 * @param host
 * @param port
 * @param array of args eg array('notls', 'imap', 'novalidate-cert')
 * @box box to check
 */
class SimplifiedConnect {
	public $username;
	public $password;
	public $host;
	public $port;
	public $args;
	public $imap;


	public function imapConnect($username, $password, $host, $port, array $args, $box = 'INBOX') {
		$args = implode('/', $args);
		$mailserver = '{' . $host . ':' . $port . '/' . $args . '}'.$box.'';

		//Catch Error Here
		$imap = imap_open($mailserver, $username, $password);
		//No Email
		//No Connection
		if($imap === FALSE) {
			watchdog('simplified_mailhandler', 'Can not connect!', array(), WATCHDOG_ALERT, $link = NULL);
			return FALSE;
			//throw new Exception('Connection Failed');
		} else {
			return $imap;
		}
	}
	public function numConnections($imap) {
		return imap_num_msg($imap);
	}

}


/**
 * Quick check if user is in the system
 */
class SimplifiedUserIsInSystem {
	public $email;

	public static function userInSystem($email) {

		$uid = db_query("SELECT uid FROM users WHERE mail LIKE :mail", array(':mail' => $email))->fetchField();

		if(!empty($uid)) {
			$user = user_load($uid);
			return $user;
		} else {
			watchdog('simplified_mailhandler', t("User with email %email not in system.", array('%email' => $email)));
			return FALSE;
		}
	}

}


/**
 * Quick check to see if there are even attachments
 */
class SimplifiedMailHandlerCheckAttachment {

	public static function checkAttachment($imap, $count, $imapUid) {
		$mailStruct = imap_fetchstructure($imap, $count);
		$attachments = getAttachments($imap, $count, $mailStruct, "", $imapUid);
		return $attachments;
	}

}


/**
 * Make Attachments
 * Into Drupal Files
 */
function makeFilesFromAttachmentsForNode( &$node, $attachments ) {
	foreach($attachments as $key => $value) {

		if(array_key_exists('name', $value)) {

			$filename = $value['name'];
			$message = $value['message'];
			file_put_contents('/tmp/' . $filename, $message);
			$file = file_save_data($message, 'public://' . $filename);
			//Pass back the file object to the node object
			$node->field_files['und'][$key]['fid'] = $file->fid;
			$node->field_files['und'][$key]['display'] = '1';
			$node->field_files['und'][$key]['width'] = '';
			$node->field_files['und'][$key]['height'] = '';
			$node->field_files['und'][$key]['description'] = $filename;
			$node->field_files['und'][$key]['upload'] = '';
			$node->field_files['und'][$key]['alt'] = $filename;
			$node->field_files['und'][$key]['title'] = $filename;

		} else {
			return makeFilesFromAttachmentsForNode( $node, $value );
		}

	}
	return $node;
}


/**
 * Make Attachments
 * Into Drupal Files
 * @comment saved comment object
 * @attachments array from getAttachments
 */
function makeFilesFromAttachmentsForComments( $comment, $attachments ) {
	$delta = 0;

	foreach($attachments[0] as $key => $value) {

		if(array_key_exists('name', $value)) {
			$filename = $value['name'];
			$message = $value['message'];
			file_put_contents('/tmp/' . $filename, $message);
			$file = file_save_data($message, 'public://' . $filename);

			//From here attach the attachments
			$fields = array(
				'entity_type' => 'comment',
				'bundle' => 'comment_node_task',
				'deleted' => 0,
				'entity_id' => $comment->cid,
				'revision_id' => $comment->cid,
				'language' => 'und',
				'delta' => $delta,
				'field_attachment_fid' => $file->fid,
				'field_attachment_display' => 1,
				'field_attachment_description' => '',
			);
			db_insert('field_data_field_attachment')
				->fields($fields)
				->execute();

			db_insert('field_revision_field_attachment')
				->fields($fields)
				->execute();

			db_insert('file_usage')
				->fields(array(
				'fid' => $file->fid,
				'module' => 'file',
				'type' => 'comment',
				'id' => $comment->cid,
				'count' => 1,
			))
				->execute();

			$delta++;
		}
	}
	//Quick clear
	//Else it did not show
	field_cache_clear();
}

/**
 * Run at cron and testing
 */
function SimplifiedMailHandlerProcessor() {
	$newConneciton = new SimplifiedConnect;
	$connectionParams = explode('|', CONNECTION_PARAMS);
	$imap = $newConneciton->imapConnect(USERNAME, PASSWORD, HOST, PORT, $connectionParams);

	$numMessages = $newConneciton->numConnections($imap);
	//If messages need to process them
	if( $numMessages > 0 ) {
		for ($count = 1; $count <= ($numMessages); $count++) {
			$header = imap_header($imap, $count);
			$imapUid = imap_uid($imap, $count);
			//Who is it from?
			$final_email_from = sm_final_email_from($header);
			$userObject = SimplifiedUserIsInSystem::userInSystem($final_email_from);

			if($userObject !== FALSE) {
				//User in the system now what?
				$body = getBodyPlain($imapUid, $imap);
				//Now to see if this is a new ticket or a reply
				if(strpos($body, 'Reply ABOVE this LINE to POST a COMMENT') === FALSE) {
					//Must be new...
					$makeNode = simple_mailhander_task_maker($header, $userObject, $imapUid, $imap, $count);
					//Now send the new node to the user
					//This now happens during the node creation
					//$newMails = new SimplifiedMailHandlerNotify($makeNode->title, $makeNode->nid, $makeNode->uid, FROM, 'post');
					//$newMails->sendToStaff();
					//If node is made move it to Processed
					$moveNode = new SimplifiedMailHandlerMoveMail();
					if($makeNode->nid) {
						//Subscribe user
						$subscribe = subscribe_author_to_task_from_email($userObject, $makeNode->nid);
						//Move Email
						$moveNode->moveMail(
							$imap, $count, 'Processed', WATCHDOG_ERROR,
							'Email %title Processed', array('%title' => $header->Subject)
						);
					} else {
						$message = t('Problem processing email titled %title', array('%title' => $header->Subject));
						$moveNode->moveMail(
							$imap, $count, 'Error', WATCHDOG_ERROR,
							$message,
							array()
						);
						sm_notify_on_state('Problem processing email...', $message);
					}

				} else {
					//So this is a reply so we need to make a Comment
					//Thread or not to thread.
					$base_url = BASE;
					$node_or_comment = sm_get_post_id($body);

					if($node_or_comment['type'] == 'comment') {
						$thread_parent = $node_or_comment['id'];
						$makeComment = simple_mailhander_comment_maker($header, $userObject, $imapUid, $imap, $count, $thread_parent, $thread_parent);
						//hook_comment_load does this
						//$newMails = new SimplifiedMailHandlerNotify($makeComment->subject, $makeComment->cid, $makeComment->uid, FROM, 'comment');
						//$newMails->sendToStaff();
					} else {
						//Must be a first comment to a node/email
						$get_comment_nid = $node_or_comment['id'];
						$makeComment = simple_mailhander_comment_maker($header, $userObject, $imapUid, $imap, $count, "", $get_comment_nid);
						//hook_comment_load does this
						//$newMails = new SimplifiedMailHandlerNotify($makeComment->subject, $makeComment->cid, $makeComment->uid, FROM, 'comment');
						//$newMails->sendToStaff();
					}
					//If node is made move it to Processed
					$moveComment = new SimplifiedMailHandlerMoveMail();
					if($makeComment->cid) {
						$moveComment->moveMail(
							$imap, $count, 'Processed', WATCHDOG_NOTICE,
							'Email %title Processed', array('%title' => $header->Subject)
						);
					} else {
						$message = t('Problem processing email titled %title', array('%title' => $header->Subject));
						$moveComment->moveMail(
							$imap, $count, 'Error', WATCHDOG_ERROR,
							$message,
							array()
						);
						sm_notify_on_state('Problem processing email', $message);
					}
				}
			} else {
				//Move to Not-Processed
				//since no user exists
				//let staff manage it.
				$moveMail = new SimplifiedMailHandlerMoveMail();
				$moveMail->moveMail(
					$imap, $count, 'NotProcessed', WATCHDOG_NOTICE,
					'User @user not in system', array('@user' => $final_email_from, '@title' => $header->Subject)
				);
				$message = t('User @user not in system', array('@user' => $final_email_from, '@title' => $header->Subject));
				sm_notify_on_state('Email NotProcessed...', $message);
			}
		}
		//Clean up after the above
		@imap_expunge($imap);
		@imap_close($imap);
	} else {
		$date = date('m-d-y h:i:s', REQUEST_TIME);
		watchdog('simplified mailhandler', 'No Emails @ %date', array('%date' => $date), WATCHDOG_NOTICE, $link = NULL);
	}

}

/**
 * Recursively Check Attachments
 * http://www.phpclasses.org/package/2934-PHP-Retrieve-and-parse-messages-from-a-POP3-mailbox.html
 */
function getAttachments($imap, $mailNum, $part, $partNum = NULL, $imapUid) {
	$attachments = array();

	if (isset($part->parts)) {
		foreach ($part->parts as $key => $subpart) {
			if($partNum != "") {
				$newPartNum = $partNum . "." . ($key + 1);
			}
			else {
				$newPartNum = ($key+1);
			}
			$result = getAttachments($imap, $mailNum, $subpart,
				$newPartNum, $imapUid);
			if (count($result) != 0) {
				array_push($attachments, $result);
			}
		}
	}
	else if (isset($part->disposition)) {
		if ($part->disposition == "ATTACHMENT" || $part->disposition == "INLINE") {

			$partStruct = imap_bodystruct($imap, $mailNum,
				$partNum);
			$message = imap_fetchbody($imap, $imapUid, $partNum, FT_UID);
			$message = getdecodevalue($message,$part->type);

			$attachmentDetails = array(
				"name"    => $part->dparameters[0]->value,
				"partNum" => $partNum,
				"enc"     => $partStruct->encoding,
				"message" => $message
			);
			//watchdog('Attachments line 314', print_r($attachmentDetails,1));
			return $attachmentDetails;
		}
	}
//Non Gmail Attachments
//else if (isset($part->ifdisposition)) {
//			if ($part->ifdisposition == "ATTACHMENT" || $part->ifdisposition == "INLINE") {
//
//				$partStruct = imap_bodystruct($imap, $mailNum,
//					$partNum);
//				$message = imap_fetchbody($imap, $imapUid, $partNum, FT_UID);
//				$message = getdecodevalue($message,$part->type);
//
//				$attachmentDetails = array(
//					"name"    => $part->ifdparameters[0]->value,
//					"partNum" => $partNum,
//					"enc"     => $partStruct->encoding,
//					"message" => $message
//				);
//				watchdog('Attachments line 331', print_r($attachmentDetails,1));
//				return $attachmentDetails;
//			}
//		}
	//watchdog('Attachments line 3335', print_r($part, 1));
	return $attachments;
}

/**
 * http://amitkgaur.blogspot.com/2010/07/download-gmail-attachment-in-php.html
 */
function getdecodevalue($msg,$bodyType) {
	if($bodyType==0 || $bodyType==1) {
		$msg = imap_base64($msg);
	}
	if($bodyType==2) {
		$msg = imap_binary($msg);
	}
	if($bodyType==3 || $bodyType==5) {
		$msg = imap_base64($msg);
	}
	if($bodyType==4) {
		$msg = imap_qprint($msg);
	}
	return $msg;
}

/**
 * Move Mail to Folders
 */
class SimplifiedMailHandlerMoveMail {
	public $imap;
	public $count;
	public $mailBox;
	public $severity;
	public $message;
	public $message_args;

	public function moveMail($imap, $count, $mailBox, $severity, $message, $message_args) {
		@imap_mail_move($imap, $count, $mailBox);
		watchdog('simplified_mailhandler', $message, $message_args, $severity, $link = NULL);
	}

}


/**
 * http://phpmaster.com/exploring-phps-imap-library-2/
 */


function getBodyPlain($uid, $imap) {
	$body = get_part($imap, $uid, "TEXT/PLAIN");
	return $body;
}

function getBodyHTML($uid, $imap) {
	$body = get_part($imap, $uid, "TEXT/HTML");
	return $body;
}

function getBodyDynamic($uid, $imap) {
	$body = get_part($imap, $uid, "TEXT/HTML");
	// if HTML body is empty, try getting text body
	if ($body == "") {
		$body = get_part($imap, $uid, "TEXT/PLAIN");
	}
	return $body;
}

/**
 * http://phpmaster.com/exploring-phps-imap-library-2/
 */
function get_part($imap, $uid, $mimetype, $structure = false, $partNumber = false) {
	if (!$structure) {
		$structure = imap_fetchstructure($imap, $uid, FT_UID);
	}
	if ($structure) {
		if ($mimetype == get_mime_type($structure)) {
			if (!$partNumber) {
				$partNumber = 1;
			}
			$text = imap_fetchbody($imap, $uid, $partNumber, FT_UID);
			switch ($structure->encoding) {
				case 3: return imap_base64($text);
				case 4: return imap_qprint($text);
				default: return $text;
			}
		}

		// multipart
		if ($structure->type == 1) {
			foreach ($structure->parts as $index => $subStruct) {
				$prefix = "";
				if ($partNumber) {
					$prefix = $partNumber . ".";
				}
				$data = get_part($imap, $uid, $mimetype, $subStruct, $prefix . ($index + 1));
				if ($data) {
					return $data;
				}
			}
		}
	}
	return false;
}

/**
 * http://phpmaster.com/exploring-phps-imap-library-2/
 */
function get_mime_type($structure) {
	$primaryMimetype = array("TEXT", "MULTIPART", "MESSAGE", "APPLICATION", "AUDIO", "IMAGE", "VIDEO", "OTHER");

	if ($structure->subtype) {
		return $primaryMimetype[(int)$structure->type] . "/" . $structure->subtype;
	}
	return "TEXT/PLAIN";
}


/**
 * Make the task node from the email
 * Passing too many args here
 * @todo reduce args
 * @param $header from the email rather not process it again
 * @param $userObject rather not process it again
 * @param $imapUid rather not....
 * @param $imap using it to get the body
 * @param $count to help in the loop use 0 if just a one off use
 */
function simple_mailhander_task_maker($header, $userObject, $imapUid, $imap, $count){

	$node = new stdClass();
	$node->type = 'task';
	node_object_prepare($node);
	$node->uid = $userObject->uid;
	$node->name = $userObject->name;

	$title = check_plain($header->Subject);
	$node->title = $title;
	$node->language = 'en';

	//  This will not work as is

	$body = getBodyPlain($imapUid, $imap);

	//Filters for cleaning out message goop
	//Getting Warning: htmlspecialchars(): Invalid multibyte sequence i
	//replace body with 'test' and it works.
	//$body = htmlspecialchars($body, ENT_QUOTES, 'UTF-8');
	$body = check_markup($body, 'full_html');
	$body = utf8_encode($body);
	$node->body['und'][0]['value'] = $body;
	$node->body['und'][0]['summary'] = '';
	$node->body['und'][0]['format'] = 'full_html';

	$attachments = SimplifiedMailHandlerCheckAttachment::checkAttachment($imap, $count, $imapUid);
	if(count($attachments)) {
		makeFilesFromAttachmentsForNode($node, $attachments);
	}

	$node->status = 1;
	$node->promote = 0;
	$node->revision = $userObject->uid;

	$node->changed = $_SERVER['REQUEST_TIME'];
	$node->created = $_SERVER['REQUEST_TIME'];
	//try{
	node_submit($node);
	node_save($node);
	//} catch (Exception $e)  {
	//watchdog('simplified_mailhandler', 'Error Line 470 simplified_mailhandler_helpers.inc ' . $e->getMessage());
	//}
	if($node->nid) {
		return $node;
	} else {
		throw new Exception('Node not made');
	}

}


/**
 * Make the task comment from the email
 * Passing too many args here
 * @todo reduce args
 * @param $header from the email rather not process it again
 * @param $userObject rather not process it again
 * @param $imapUid rather not....
 * @param $imap using it to get the body
 * @param $count to help in the loop use 0 if just a one off use
 * @thread_parent_cid does it have a parent cid
 * @comment_nid if no to above then make attach it to nid
 */
function simple_mailhander_comment_maker($header, $userObject, $imapUid, $imap, $count, $thread_parent_cid = "NOPARENT", $comment_nid = FALSE){
	watchdog('Parent Cid', print_r($thread_parent_cid, 1));
	watchdog('Comment Nid', print_r($comment_nid, 1));
	if($thread_parent_cid >= 1) {
		$cid = db_query("SELECT cid, nid FROM comment WHERE cid = :threadparent", array(':threadparent' => $thread_parent_cid))->fetchAll();
		$cid_nid = $cid[0]->nid;
		$cid_pid = $cid[0]->cid;
	} else {
		$cid_nid = $comment_nid;
		$cid_pid = 0;
	}

	$author = $userObject->uid;

	$date = date('U', strtotime($header->date));
	$body = getBodyPlain($imapUid, $imap);
	//Strip body of the
	//Reply text so we only get the new text
	$replyText = strpos($body, '((( Reply ABOVE this LINE to POST a COMMENT )))');

	//Get the part of the body before this
	//Trim a bit more off for the ending date
	$replyText = $replyText - 65;
	$bodySub = substr($body, 0, $replyText);

	$bodySub = utf8_encode($bodySub);
	//Filters for cleaning out message goop
	$body = check_markup($bodySub, 'full_html');
	$comment = new stdClass();
	$comment->cid = 0;
	$comment->subject = $header->Subject;
	$comment->pid = $cid_pid;
	$comment->uid = $author;
	$comment->nid = $cid_nid;
	$comment->mail = $userObject->mail;
	$comment->name = $userObject->name;
	$comment->hostname = '127.0.0.1';
	$comment->created = $date;
	$comment->date = $date;
	$comment->is_anonymous = 0;
	$comment->status = 1;
	$comment->language = LANGUAGE_NONE;
	$comment->comment_body[$comment->language][0]['format'] = 'full_html';

	$comment->comment_body[$comment->language][0]['value'] = $body;
	try {
		comment_submit($comment);
		comment_save($comment);
	} catch (Exception $e)  {
		watchdog('simplified_mailhandler', 'Error Line 531 simplified_mailhandler_helpers.inc' . $e->getMessage());
	}
	$attachments = SimplifiedMailHandlerCheckAttachment::checkAttachment($imap, $count, $imapUid);
	if(count($attachments)) {
		makeFilesFromAttachmentsForComments($comment, $attachments);
	}

	return $comment;
}


/**
 * Move email back to inbox for testing
 */
function simplified_mailhandler_move_back() {
	//Move them all back to INBOX
	$newConneciton = new SimplifiedConnect;
	$connectionParams = explode('|', CONNECTION_PARAMS);
	$imap = $newConneciton->imapConnect(USERNAME, PASSWORD, HOST, PORT, $connectionParams, 'Processed');
	$numMessages = $newConneciton->numConnections($imap);
	for ($count = 1; $count <= ($numMessages); $count++) {
		$header = imap_header($imap, $count);
		$imapUid = imap_uid($imap, $count);
		$userObject = SimplifiedUserIsInSystem::userInSystem($header->senderaddress);
		$move_node = new SimplifiedMailHandlerMoveMail();
		$move_node->moveMail(
			$imap, $count, 'INBOX', WATCHDOG_INFO,
			'Email %title Processed', array('%title' => $header->Subject)
		);

	}
	@imap_expunge($imap);
	$newConneciton = new SimplifiedConnect;
	$connectionParams = explode('|', CONNECTION_PARAMS);
	$imap = $newConneciton->imapConnect(USERNAME, PASSWORD, HOST, PORT, $connectionParams, 'NotProcessed');
	$numMessages = $newConneciton->numConnections($imap);
	for ($count = 1; $count <= ($numMessages); $count++) {
		$header = imap_header($imap, $count);
		$imapUid = imap_uid($imap, $count);
		$userObject = SimplifiedUserIsInSystem::userInSystem($header->senderaddress);
		$move_node = new SimplifiedMailHandlerMoveMail();
		$move_node->moveMail(
			$imap, $count, 'INBOX', WATCHDOG_INFO,
			'Email %title Processed', array('%title' => $header->Subject)
		);

	}
	@imap_expunge($imap);
}



/**
 * Subscribe Author to their own posts on email/node creation.
 */
function subscribe_author_to_task_from_email($userObject, $nid) {


	$fields = array(
		'pid' => NULL,
		'type' => 'node',
		'entity_id' => $nid,
		'who' => serialize(array($userObject->uid)),
		'time' => REQUEST_TIME,
	);

	$subscription = db_insert('simplified_mailhandler_message', $fields)
		->fields($fields)
		->execute();
	return $subscription;
}


/**
 * Using this to get the id of the node or comment we are working on
 * Seems here though my post_id work is stripped?
 */
function simplified_mailhandler_html_tpl($content) {
	//Chech if node or comment
	$files = FALSE;
	$node_or_comment = sm_get_post_id($content);
	if($node_or_comment['type'] == 'node') {
		$node_nid = $node_or_comment['id'];
		if($node_nid !== FALSE) {
			$output = sm_get_node_body_html($node_nid);
			$output .= '</br>';
			$output .= sm_get_files_html($node_nid);
			$output .= '</br>';
			$output .= '[post_id:'.$node_nid.']';
			$output .= '</br>';

			return $output;
		}

	} elseif($node_or_comment['type'] == 'comment') {
		$comment_cid = $node_or_comment['id'];
		if($comment_cid !== FALSE) {
			$output = sm_get_cid_body_html($comment_cid);
			$output .= '</br>';
			$output .= sm_get_cid_files_html($comment_cid);
			$output .= '</br>';
			$output .= '[post_id:'.$comment_cid.']';
			$output .= '</br>';

			return $output;
		}
	} else {
		return FALSE;
	}
}


/**
 * Using this to get the id of the node or comment we are working on
 */
function simplified_mailhandler_plain_tpl($content) {
	//Chech if node or comment
	$files = FALSE;
	$node_or_comment = sm_get_post_id($content);
	if($node_or_comment['type'] == 'node') {

		$node_nid = $node_or_comment['id'];
		if($node_nid !== FALSE) {
			$output = sm_get_node_body_html($node_nid);
			$output .= '\n';
			$output .= sm_get_files($node_nid);
			$output .= '[post_id:'.$node_nid.']';
			$output .= '\n';
			return $output;
		}

	} elseif($node_or_comment['type'] == 'comment') {
		$comment_cid = $node_or_comment['id'];
		if($comment_cid !== FALSE) {
			$output = sm_get_cid_body($comment_cid);
			$output .= '\n';
			$output .= sm_get_cid_files($comment_cid);
			$output .= '[post_id:'.$comment_cid.']';
			$output .= '\n';
			return $output;
		}
		//$output = sm_get_node_body_from_comment($node_nid);
	} else {
		return FALSE;
	}
	//return $files
}


function sm_get_post_id($content){
	$post_id = array();
	$starts = strpos($content, "[post_id");

	$ends = strpos($content, "]") - $starts;
	$post_id = substr($content, $starts, $ends);
	$post_id = explode(':', $post_id);

	if($starts !== FALSE) {
		return array('type' => 'node', 'id' => $post_id[1]);
	} else {
		//Must be a comment
		$comment_id = sm_get_comment_id($content);
		if(is_array($comment_id)) {
			return $comment_id;
		} else {
			watchdog('simplified mailhandler', 'Post ID not found in this message !message', array('!message' => $content));
			return FALSE;
		}
	}
}

/**
 * Function:sm_get_comment_id
 * Check for the comment token placed in comments
 *
 * @param $content
 * The body of the meila
 */

function sm_get_comment_id($content) {
	$post_id = array();
	$starts = strpos($content, "[comment_id");
	$ends = strpos($content, "]") - $starts;
	$post_id = substr($content, $starts, $ends);
	$post_id = explode(':', $post_id);

	if($starts !== FALSE) {
		return array('type' => 'comment', 'id' => $post_id[1]);
	} else {
		return false;
	}
}

function sm_get_node_title($nid) {
	return db_query("SELECT title from node WHERE nid = :nid", array(':nid' => $nid))->fetchField();
}


function sm_get_comment_title($cid) {
	return db_query("SELECT subject from comment WHERE cid = :cid", array(':cid' => $cid))->fetchField();
}

function sm_get_node_body($nid) {
	$body = 'No Message';
	$body = db_query("SELECT body_value FROM field_data_body WHERE entity_id = :nid", array(':nid' => $nid))->fetchField();
	return htmlspecialchars($body, ENT_QUOTES, 'UTF-8');
}

function sm_get_node_body_html($nid) {
	$body = 'No Message';
	$body = db_query("SELECT body_value FROM field_data_body WHERE entity_id = :nid", array(':nid' => $nid))->fetchField();
	return $body;
}

function sm_get_cid_body($cid) {
	$body = 'No Message';
	$body = db_query("SELECT comment_body_value FROM field_data_comment_body WHERE entity_id = :cid", array(':cid' => $cid))->fetchField();
	return $body;
}

function sm_get_cid_body_html($cid) {
	$body = 'No Message';
	$body = db_query("SELECT comment_body_value FROM field_data_comment_body WHERE entity_id = :cid", array(':cid' => $cid))->fetchField();
	return $body;
}

function sm_get_nid_from_cid($cid) {
	$nid = '0';
	$nid = db_query("SELECT nid FROM comment WHERE cid = :cid", array(':cid' => $cid))->fetchField();
	return $nid;
}

function sm_get_site_link_from_comment($cid) {
	$link = '';
	$query = db_query("SELECT cid, pid, nid FROM comment WHERE cid = :cid", array(':cid' => $cid))->execute();
	foreach($query as $key) {
		$link = link('Task Link', $GLOBALS['base_url'] . '/node/' . $key->nid);
	}
	return $link;
}

function sm_get_files($nid) {
	$query = db_select('field_data_field_files', 'files');
	$query->fields('files', array('field_files_fid', 'entity_id'));
	$query->innerJoin('file_managed', 'managed', 'managed.fid = field_files_fid');
	$query->fields('managed', array('uri', 'filename'));
	$query->condition('files.entity_id', $nid, '=');
	$results = $query->execute();
	if($results){
		$output = "Related Files \n";
		foreach($results as $row) {
			$path = file_create_url($row->uri);
			$output .= "$path \n";
		}
		return $output;
	} else {
		return FALSE;
	}
}

function sm_get_files_html($nid) {
	if(is_numeric($nid)) {
		$query = db_select('field_data_field_files', 'files');
		$query->fields('files', array('field_files_fid', 'entity_id'));
		$query->innerJoin('file_managed', 'managed', 'managed.fid = field_files_fid');
		$query->fields('managed', array('uri', 'filename'));
		$query->condition('files.entity_id', $nid, '=');
		$results = $query->execute();
		$files = array();
		$output = "<h2>Related Files</h2>";
		foreach($results as $row) {
			$path = file_create_url($row->uri);
			$files[] = "<a href=\"$path\">{$row->filename}</a>";
		}
		if(count($files) > 0) {
			$output .= theme('item_list', array('items' => $files));
			return $output;
		} else {
			return FALSE;
		}
	}
}

function sm_get_cid_files($cid) {
	if(is_numeric($cid)) {
		$query = db_select('field_data_field_attachment', 'files');
		$query->fields('files', array('field_attachment_fid', 'entity_id'));
		$query->innerJoin('file_managed', 'managed', 'managed.fid = field_attachment_fid');
		$query->fields('managed', array('uri', 'filename'));
		$query->condition('files.entity_id', $cid, '=');
		$results = $query->execute();
		$output = '';
		foreach($results as $row) {
			$path = file_create_url($row->uri);
			$output .= "$path \n";
		}
		if(!empty($output)) {
			return "Related Files \n" . $output;
		} else {
			return FALSE;
		}
	}
}


function sm_get_cid_files_html($cid) {
	$files = array();
	$query = db_select('field_data_field_attachment', 'files');
	$query->fields('files', array('field_attachment_fid', 'entity_id'));
	$query->innerJoin('file_managed', 'managed', 'managed.fid = field_attachment_fid');
	$query->fields('managed', array('uri', 'filename'));
	$query->condition('files.entity_id', $cid, '=');
	$results = $query->execute();
	$output = "<h2>Related Files</h2>";
	foreach($results as $row) {
		$path = file_create_url($row->uri);
		$files[] = "<a href=\"$path\">{$row->filename}</a>";
	}
	if(count($files)) {
		$output .= theme('item_list', array('items' => $files));
		return $output;
	} else {
		return FALSE;
	}
}

/**
 * Parse the email to get just the
 * address since that is what we need
 * for say finding user in system
 */
function sm_final_email_from($header) {
	$final_email_from = '';

	if(strpos($header->senderaddress, '<') !== FALSE) {
		$start_email = strpos($header->senderaddress, '<') + 1;
		$end_email = strpos($header->senderaddress, '>') - $start_email;
		$final_email_from = substr($header->senderaddress, $start_email, $end_email);
	} else {
		$final_email_from = $header->senderaddress;
	}

	return $final_email_from;

}



/**
 * @vars could be the Expection error
 */
function sm_notify_on_state($subject, $vars = NULL) {
	$subject_plus = '[Ticket System Issue] ' . $subject;
	$message = "Watchdog logs may have some info <a href='"  . BASE .">watchdog</a>";
	if($vars) {
		$message .= $vars;
	}
	$mails = array(
		'al@freshjones.com',
	);

	foreach($mails as $to) {
		try {
			mail($to, $subject_plus, $message);
		} catch (Expection $e) {
			watchdog('simplified_mailhandler', 'Not good, tried to email for help and could not even do that.');
		}
	}
}

class SimplifiedMailHandlerNotify {
	public $entity_id;
	public $subject;
	public $staffEmailsArray;
	public $headers;
	public $from;
	public $type; //node or comment
	public $who;
	public $author;

	function __construct($subject, $entity_id, $who, $from, $type) {
		$this->type = $type;
		$this->body = theme('simplified_mailhandler_email_staff', array('entity_id' => $entity_id, 'type' => $type));
		$this->from = $from;
		$this->who = $who;
		$this->author = simplifiedMailHandlerHelpers::getAuthorUidAndEmail($entity_id, $type);
		$this->subject = $subject;
		$this->staffEmailsArray = self::getStaffToNotify();
		$this->headers = self::makeHeader();
	}

	function getStaffToNotify() {
		$staffEmailsArray = simplifiedMailHandlerHelpers::getEmailsOfRole(4);
		return $staffEmailsArray;
	}

	function makeHeader() {
		$headers  = 'MIME-Version: 1.0' . "\r\n";
		$headers .= 'Content-type: text/html; charset=iso-8859-1' . "\r\n";
		$headers .= "From: {$this->from}" . "\r\n" .
			"Reply-To: {$this->from}" . "\r\n" .
			'X-Mailer: PHP/' . phpversion();
		return $headers;
	}

	function sendToStaff() {
		$body = $this->body;
		$subject = $this->subject;
		$headers = $this->headers;
		foreach($this->staffEmailsArray as $uid => $to) {
			//No need to notify the maker of the node
			if($uid != $this->who) {
				$send = '';
				//To
				//Subject
				//Message
				//Additional Headers may need \n
				//Additional Parameters Optionsal
				//<link http://php.net/manual/en/function.mail.php>
				$send = mail($to, $subject, $body, $headers);
				if($send === TRUE) {
					watchdog('simplifed mailhandler', "Message to $to Sent");
				} else {
					watchdog('simplifed mailhandler', "Message to $to *NOT* Sent");
				}
			}
		}
	}
	/**
	 * @param who should be an array with the email as the value
	 */
	function sendToNodeRelatedPeople() {
		$body = $this->body;
		$subject = $this->subject;
		$headers = $this->headers;
		$who = $this->who;
		foreach($who as $uid => $to) {

			//No need to notify the maker of the node
			if(isset($this->author[$uid])) {
				//Quick check to see if it is the author
				if($this->author[$uid]  != $to) {
					self::sendIt($to, $subject, $body, $headers);
				}
			} else {
				//not set so not too author?
				self::sendIt($to, $subject, $body, $headers);
			}
		}
	}

	function sendIt($to, $subject, $body, $headers) {
		$send = '';
		$send = mail($to, $subject, $body, $headers);
		if($send === TRUE) {
			watchdog('simplifed mailhandler', "Message to $to Sent");
		} else {
			watchdog('simplifed mailhandler', "Message to $to *NOT* Sent");
		}
	}

}

class SimplifiedMailHandlerSaveSendRecord {
	public $entityId;
	public $entityType;
	public $whoSerialized; //send not serialized
	public $unixTime;

	function __construct($entityId, $entityType, $whoSerialized, $unixTime) {
		$this->entityId = $entityId;
		$this->entityType = $entityType;
		$this->whoSerialized = serialize($whoSerialized);
		$this->unixTime = $unixTime;
	}

	// Notice that we are ignoring any revision information using $node->nid
	function saveIt() {
		return db_insert('simplified_mailhandler_message')
			->fields(array(
			'pid' => NULL,
			'entity_id' => $this->entityId,
			'type' => $this->entityType,
			'who' => $this->whoSerialized,
			'time' => $this->unixTime,
		))
			->execute();
	}

}


/**
 * Function SimplifiedMailHandlerNotifyNodeEdit
 *
 * Set who to notify when making a node
 */
class SimplifiedMailHandlerNotifyNodeEdit {
	public $staffArray;
	public $staffArrayOptionList;
	public $node_nid;

	function __construct($nodeObject) {
		$this->node_nid = $nodeObject->nid;
		$this->staffArrayOptionList = simplifiedMailHandlerHelpers::makeRoleOptionList(4);
	}

}

/**
 * Function SimplifiedMailHandlerNotifyNodeView
 *
 * View the notified when looking at a node
 */
class SimplifiedMailHandlerNotifyNodeView {
	public $whoToNotifyUids;
	public $whoToNotifyEmails;
	public $nodeObjectNid;
	public $notify;
	public $usersEmail;
	public $role;
	public $who;
	public $emails;
	public $whoArray;

	function __construct($nodeObject) {
		$this->nodeObjectNid = $nodeObject->nid;
		$this->whoToNotifyUids = simplifiedMailHandlerHelpers::getWhoToNotifyUids($nodeObject->nid);
		$this->whoToNotifyEmails = simplifiedMailHandlerHelpers::whoToNotifyEmails($nodeObject->nid);
	}

}

class simplifiedMailHandlerHelpers {
	public $nid;

	static function getEmailsOfRole($role){
		$usersEmail = db_query("SELECT u.uid, mail FROM users_roles r
                       INNER JOIN users u ON u.uid = r.uid
                       WHERE r.rid = :role", array(':role' => $role))->fetchAllKeyed();
		return $usersEmail;
	}

	static function getUidArrayOfRoles($role){
		$usersUid = db_query("SELECT u.uid, u.uid FROM users_roles r
                       INNER JOIN users u ON u.uid = r.uid
                       WHERE r.rid = :role", array(':role' => $role))->fetchAllKeyed();
		return $usersUid;
	}

	/**
	 * see who the node is set foreach
	 */
	static function queryForWhoForNodeNid($nid){
		$who = db_query("SELECT who FROM simplified_mailhandler_message where entity_id = :entity_id", array(':entity_id' => $nid))->fetchField();
		$who = unserialize($who);
		return $who;
	}

	static function whoToNotifyEmails($nid){
		$emails = array();
		$whoToNotifyUids = self::queryForWhoForNodeNid($nid);
		//dpm($whoToNotifyUids);
		if(is_array($whoToNotifyUids)) {
 			foreach($whoToNotifyUids as $uid) {
				//This below code was to stop a white screen when looking
				//at node 255
				//		if(is_array($whoToNotifyUids['#values']->whoToNotifyUids)) {
				//			foreach($whoToNotifyUids['#values']->whoToNotifyUids as $uid) {
				//Get Email
				$emails[$uid] = db_query("SELECT mail FROM users WHERE uid = :uid", array(':uid' => $uid))->fetchField();
			}
		}
		return $emails;
	}

	static function getWhoToNotifyUids($nid) {
		$whoArray = self::queryForWhoForNodeNid($nid);
		return $whoArray;
	}

	/**
	 * Make an options list for roles
	 */
	static function makeRoleOptionList($role) {
		$usersEmail = db_query("SELECT u.uid, mail FROM users_roles r
                     INNER JOIN users u ON u.uid = r.uid
                     WHERE r.rid = :role", array(':role' => $role))->fetchAllKeyed();
		return $usersEmail;
	}

	/**
	 * @param array of users ids
	 */
	static function notifyThePeople($subject, $entity_id, $who, $from, $type) {
		$notify = new SimplifiedMailHandlerNotify($subject, $entity_id, $who, $from, $type);
		$notify->sendToNodeRelatedPeople();
	}

	/**
	 * @param type post or comment
	 * @return the array uid => email
	 */
	static function getAuthorUidAndEmail($enity_id, $type) {
		if($type == 'post') {
			return db_query("SELECT u.uid, mail FROM users u
                         INNER JOIN node n ON n.uid = u.uid
                         WHERE n.nid = :nid", array(':nid' => $enity_id))->fetchAllKeyed();
		}
		if($type == 'comment') {
			return db_query("SELECT u.uid, u.mail FROM users u
                         INNER JOIN comment c ON c.uid = u.uid
                         WHERE c.cid = :cid", array(':cid' => $enity_id))->fetchAllKeyed();
		}
	}
}