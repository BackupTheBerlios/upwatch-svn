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
"yellowmiss"    => array("pr_template_def", "yellowmiss", EDIT, OPTIONAL),
"redmiss"       => array("pr_template_def", "redmiss",    EDIT, OPTIONAL),
"color"         => array("pr_template_def", "color",      EDIT, WITHLINK),
);

$extra_fields = array("servertext");

  $cms = new DB_CMS;

  while  ($__act == "submit") { 
    cms_leave_if_canceled();
    foreach($HTTP_POST_VARS as $key => $value) {
      $fields[$key] = $HTTP_POST_VARS[$key];
    }
    if (!cms_fieldcheck($form_fields, $fields)) break;
    $cms->query("begin");
    cms_update($cms, $form_fields, "pr_template_def", "id", $fields);
    $changed_fields = cms_changed_fields($form_fields, $fields);
    foreach ($changed_fields as $value) {
      $oldvar = "org" . $value;
      $q = sprintf("insert into history set created = NOW(), creator = '%s', contact = '%s', " .
                   "type = 'CHANGED', lookup = 'pr_template_def', lookupid = '%s', value = '%s'",
                   $sess->u_id, $fields['contact'],
                   $id, mysql_escape_string($value . " from '" . $fields[$oldvar] . "' to '" . $fields[$value] . "'"));
      echo $q;
      $cms->query($q);
    }
    $cms->query("commit");
    previous_location();
  }
  if  (!isset($__act)) { 
    $cms->query("select pr_template_def.*, server.id, server.name as servertext, purchase.contact " .
                "from   pr_template_def, server, purchase " .
                "where  pr_template_def.id = $id and server.id = pr_template_def.server and purchase.id = server.purchase");
    if ($cms->num_rows() == 0) {
      $err_msg = "pr_template_def with id = $id not found";
      log_msg(__FILE__, __LINE__, $err_msg);
      Location("../internal_error.php?msg=$err_msg");
    }
    $cms->next_record();

    foreach ($form_fields as $key => $value) {
      $fields[$key] = $cms->f($key);
    }
    foreach ($extra_fields as $value) {
      $fields[$value] = $cms->f($value);
    }
    $contact = $cms->f('contact');
  }

  title(_("Wijzig Ping Probe") . " " . $fields['name']);

  cms_err_msg();

  form_start("form", "POST");
  html_input("hidden", "id", $id, 8);
  html_input("hidden", "contact", $contact, 8);
  table(); 
  foreach ($form_fields as $key => $value) {
    tr();
      td(); cms_showfielddesc($form_fields, $key);// the textual name of the field
      td(); cms_showfield($form_fields, $key, $fields);
  }
  save();
  endtable();
  form_end();

?>

