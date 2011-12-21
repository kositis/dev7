<?php print $doctype; ?>
<html lang="<?php print $language->language; ?>" dir="<?php print $language->dir; ?>"<?php print $rdf_namespaces; ?>>
<head<?php print $rdf_profile; ?>>
<?php print $head; ?>
<title><?php print $head_title; ?></title>
<?php print $styles; ?>
<? if($is_front): ?>
<link rel="stylesheet" href="<?php global $base_path; print $base_path.path_to_theme(); ?>/front-style.css" type="text/css" media="all" />
<? endif; ?>
<?php print $scripts; ?>
<!--[if lt IE 9]>
<link rel="stylesheet" href="<?php print $base_path.path_to_theme(); ?>/ie_common.css" type="text/css" media="all" />
<script src="http://html5shiv.googlecode.com/svn/trunk/html5.js"></script>
<![endif]-->
</head>
<body class="<?php print $classes; ?>"<?php print $attributes; ?>>
  <div id="skip-link">
    <a href="#main-content" class="element-invisible element-focusable"><?php print t('Skip to main content'); ?></a>
  </div>
  <?php print $page_top; ?>
  <?php print $page; ?>
  <?php print $page_bottom; ?>
</body>
</html>