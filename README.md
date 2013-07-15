simplified_mailhandler
======================

Made about December 2012 to help centralize emails as tickets.

Part of the Project Manager suite. Needed a more concise system to manage incoming emails and attachments.

Turn off the Mail Comment and Mail Node Feed modules
admin/config/messaging/method/mailSet the who and name
But leave mail comment enabled for a module.
Watchdog needs to be on and will log all activity including issues.

http://local.mh.com/admin/config/messaging/mailcomment

http://local.mh.com/admin/config/messaging/method/mail

There is a settings.php file to put you email settings in.
This way dev and live can have separate settings.
Just make sure git ignore or deploy ignores.



Workflow setup

Staff role is set to receive any new emails of type Tasks 

But if staff is not set this way as a new one comes in the can unsubscribe to it.
***Maybe better is they are notified but not subscribe till they subscribe else all staff are on all threads?***


Client
SHOULD NOT be allowed to subscribe to Task content type
They only are notifed about Tasks they make are Staff set "notify" to.
