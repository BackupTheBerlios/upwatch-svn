<?php

$form_fields = array(
"server"        => array("pr_template_def", "server",     SHOW, OPTIONAL|WITHLINK),
"address"       => array("pr_template_def", "address",    SHOW, OPTIONAL),
"freq"          => array("pr_template_def", "freq",       SHOW, OPTIONAL),
"count"         => array("pr_template_def", "count",      SHOW, OPTIONAL),
"yellowmiss"    => array("pr_template_def", "yellowmiss", SHOW, OPTIONAL),
"redmiss"       => array("pr_template_def", "redmiss",    SHOW, OPTIONAL),
"color"         => array("pr_template_def", "color",      SHOW, WITHLINK),
"stattime"      => array("pr_template_def", "stattime",   SHOW, OPTIONAL),
"expires"       => array("pr_template_def", "expires",    SHOW, OPTIONAL),
"message"       => array("pr_template_def", "message",    SHOW, OPTIONAL),
);

$extra_fields = array("servertext");

  if (!cms_is_employee()) {
    unset($form_fields["color"]);
  }

  $cms = new DB_CMS;

  $cms->query("select pr_template_def.*, server.id, server.name as servertext " .
              "from   pr_template_def, server " .
              "where  pr_template_def.id = $id and server.id = pr_template_def.server");
  if ($cms->num_rows() == 0) {
    log_msg(__FILE__, __LINE__, "pr_template_def with id = $id not found");
    Location("index.php");
  }
  $cms->next_record();
  $contact = $cms->f('contact');
  if (!cms_represents($contact, R_ANY)) {
    log_msg(__FILE__, __LINE__, "Access denied");
    Location("../access_denied.php");
  }
  remember_location();

  if (!cms_represents($contact, R_TECH)) {
    unset($form_fields["password"]);
    unset($form_fields["routerpwd"]);
  }

  foreach ($form_fields as $key => $value) {
    $fields[$key] = $cms->f($key);
  }
  foreach ($extra_fields as $value) {
    $fields[$value] = $cms->f($value);
  }
 
  form_start("notused");
  table();
  tr(false);
    td("class=\"tbl-even\" align=\"left\""); title(_("Probe template") . " " . $cms->f('servertext'));
    td("class=\"tbl-even\" align=\"right\""); 
      probe_dropdown('template', $id);
  endtable();
  form_end();

  table();
  foreach ($form_fields as $key => $value) {
    tr();
      td(); cms_showfielddesc($form_fields, $key);// the textual name of the field
      td(); cms_showfield($form_fields, $key, $fields);
  }
  tr();
    td("class=\"tbl-even\" colspan=\"2\"");
    td("class=\"tbl-even\" colspan=\"2\"");
 
  if (cms_is_employee()) {
    tr();
      td("class=\"tbl-even\" align=\"center\"");
        button("edit-pr_template_def.gif", _("Edit Ping Probe"), "site-edit-pr_template_def.php?id=$id");
      td("class=\"tbl-even\" align=\"center\"");
  }
  endtable();

?>
