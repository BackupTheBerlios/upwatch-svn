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

  $q = "select * from pr_template_hist where probe = '$id'";
  if (isset($sort)) {
    $q .= " order by " . $sort;
    if (isset($sortdir)) {
      $q .= " " . $sortdir;
    }
  } else {
    $q .= " order by stattime desc";
  }
  $cms->query($q . " limit 50");

include "site-pr_histshow.php";


?>
