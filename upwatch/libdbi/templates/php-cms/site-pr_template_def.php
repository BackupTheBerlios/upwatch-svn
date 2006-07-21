<?php

  remember_location();

include "cms/db.php";
include "cms/forms.php";
include "cms/pr_template_def.rec";

  $cms = new DB_CMS;

include "site-pr_template_defshow.php";

  if (!cms_is_employee()) {
    return;
  }

  $q = "select history.id, created, contact, history.type, lookup, lookupid, value, " .
       "       co.name as contacttext, cr.name as creatortext " .
       "from   history, contact co, contact cr " .
       "where  co.id = history.contact and cr.id = history.creator " .
       "       and lookup = 'pr_template_def' and lookupid = '$id' ";

  if (isset($sort)) {
    $q .= " order by " . $sort;
    if (isset($sortdir)) {
      $q .= " " . $sortdir;
    }
  } else {
    $q .= " order by created desc";
  }
  $cms->query($q . " limit 100");

include "historyshow.php";


?>
