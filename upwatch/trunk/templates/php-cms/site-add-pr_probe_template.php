<?php

  if (!cms_is_employee()) {
    log_msg(__FILE__, __LINE__, "Access denied");
    Location("../access_denied.php");
  }

include "cms/db.php";
include "cms/forms.php";
include "cms/pr_template_def.rec";
 
$form_fields = array(
"server"        => array("pr_template_def", "server",     EDIT, OPTIONAL|CHOOSER),
"address"       => array("pr_template_def", "address",    EDIT, OPTIONAL),
"freq"          => array("pr_template_def", "freq",       EDIT, OPTIONAL),
"count"         => array("pr_template_def", "count",      EDIT, OPTIONAL),
"yellowtime"    => array("pr_template_def", "yellowtime", EDIT, OPTIONAL),
"redtime"       => array("pr_template_def", "redtime",    EDIT, OPTIONAL),
);

  $cms = new DB_CMS;

  while ($__act == "submit") {
    cms_leave_if_canceled();
    foreach($HTTP_GET_VARS as $key => $value) {
      $fields[$key] = $HTTP_GET_VARS[$key];
    }
    if (!cms_fieldcheck($form_fields, $fields)) break;

    $cms->query("begin");

    if ($fields['contact'] < 1) { $fields['contact'] = 1; }
    $q = sprintf("insert into pr_probe_template set server = '%s', address = '%s', freq = '%s', " . 
                 "redtime = '%s', yellowtime = '%s'",
                 $fields['server'], $fields['address'], $fields['freq'],
                  $fields['redtime'],  $fields['yellowtime']);
    $cms->query($q);
    $pr_probe_templateid = $cms->insert_id();

    $newstate = 'running';

    cms_add_history($fields['contact'], "CREATE", "pr_probe_template", $pr_probe_templateid, 
                    $fields['setup'] . " for " . $fields['contacttext']);

    $cms->query("commit");
    previous_location();
    break;
  }
  if  (!isset($__act)) {
    $fields['contact'] = $contact;
    $fields['server'] = $server;
    $fields['freq'] = 1;
    $fields['yellowtime'] = 1;
    $fields['redtime'] = 3;
  }
  list($contacttext, $dummy) = get_name($contact);
  $fields['name'] = $contacttext;

  title(_("New Ping Probe for") . " " . $contacttext);

  cms_err_msg();
  form_start("form");
  html_input("hidden", "id", $id, 8);
  html_input("hidden", "contact", $contact);
  html_input("hidden", "contacttext", $contacttext);
  if ($taskhead > 1) {
    html_input("hidden", "createtask", 'no');
    html_input("hidden", "taskhead", $taskhead);
  }
  table();
  foreach ($form_fields as $key => $value) {
    tr();
      td(); cms_showfielddesc($form_fields, $key);// the textual name of the field
      td(); cms_showfield($form_fields, $key, $fields);
  }
  add();
  endtable();
  form_end();
 
?>

