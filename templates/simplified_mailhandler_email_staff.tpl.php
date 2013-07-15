<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title><?php print $variables['params']['title']; ?></title>
  <style type="text/css">

    /***********
    Originally based on The MailChimp Reset from Fabio Carneiro, MailChimp User Experience Design
    More info and templates on Github: https://github.com/mailchimp/Email-Blueprints
    http://www.mailchimp.com &amp; http://www.fabio-carneiro.com

    INLINE: Yes.
    ***********/
    /* Client-specific Styles */
    #outlook a {padding:0;} /* Force Outlook to provide a "view in browser" menu link. */
    body{width:100% !important; -webkit-text-size-adjust:100%; -ms-text-size-adjust:100%; margin:0; padding:0;}
    /* Prevent Webkit and Windows Mobile platforms from changing default font sizes, while not breaking desktop design. */
    .ExternalClass {width:100%;} /* Force Hotmail to display emails at full width */
    .ExternalClass, .ExternalClass p, .ExternalClass span, .ExternalClass font, .ExternalClass td, .ExternalClass div {line-height: 100%;} /* Force Hotmail to display normal line spacing.  More on that: http://www.emailonacid.com/forum/viewthread/43/ */
    #backgroundTable {margin:0; padding:0; width:100% !important; line-height: 100% !important;}
    /* End reset */

    /* Some sensible defaults for images
    1. "-ms-interpolation-mode: bicubic" works to help ie properly resize images in IE. (if you are resizing them using the width and height attributes)
    2. "border:none" removes border when linking images.
    3. Updated the common Gmail/Hotmail image display fix: Gmail and Hotmail unwantedly adds in an extra space below images when using non IE browsers. You may not always want all of your images to be block elements. Apply the "image_fix" class to any image you need to fix.

    Bring inline: Yes.
    */
    img {outline:none; text-decoration:none; -ms-interpolation-mode: bicubic;}
    a img {border:none;}
    .image_fix {display:block;}

    /** Yahoo paragraph fix: removes the proper spacing or the paragraph (p) tag. To correct we set the top/bottom margin to 1em in the head of the document. Simple fix with little effect on other styling. NOTE: It is also common to use two breaks instead of the paragraph tag but I think this way is cleaner and more semantic. NOTE: This example recommends 1em. More info on setting web defaults: http://www.w3.org/TR/CSS21/sample.html or http://meiert.com/en/blog/20070922/user-agent-style-sheets/

    Bring inline: Yes.
    **/
    p {margin: 1em 0;}

    /** Hotmail header color reset: Hotmail replaces your header color styles with a green color on H2, H3, H4, H5, and H6 tags. In this example, the color is reset to black for a non-linked header, blue for a linked header, red for an active header (limited support), and purple for a visited header (limited support).  Replace with your choice of color. The !important is really what is overriding Hotmail's styling. Hotmail also sets the H1 and H2 tags to the same size.

    Bring inline: Yes.
    **/
    h1, h2, h3, h4, h5, h6 {color: black !important;}

    h1 a, h2 a, h3 a, h4 a, h5 a, h6 a {color: blue !important;}

    h1 a:active, h2 a:active,  h3 a:active, h4 a:active, h5 a:active, h6 a:active {
      color: red !important; /* Preferably not the same color as the normal header link color.  There is limited support for psuedo classes in email clients, this was added just for good measure. */
     }

    h1 a:visited, h2 a:visited,  h3 a:visited, h4 a:visited, h5 a:visited, h6 a:visited {
      color: purple !important; /* Preferably not the same color as the normal header link color. There is limited support for psuedo classes in email clients, this was added just for good measure. */
    }

    /** Outlook 07, 10 Padding issue: These "newer" versions of Outlook add some padding around table cells potentially throwing off your perfectly pixeled table.  The issue can cause added space and also throw off borders completely.  Use this fix in your header or inline to safely fix your table woes.

    More info: http://www.ianhoar.com/2008/04/29/outlook-2007-borders-and-1px-padding-on-table-cells/
    http://www.campaignmonitor.com/blog/post/3392/1px-borders-padding-on-table-cells-in-outlook-07/

    H/T @edmelly

    Bring inline: No.
    **/
    table td {border-collapse: collapse;}

    /** Remove spacing around Outlook 07, 10 tables

    More info : http://www.campaignmonitor.com/blog/post/3694/removing-spacing-from-around-tables-in-outlook-2007-and-2010/

    Bring inline: Yes
    **/
    table { border-collapse:collapse; mso-table-lspace:0pt; mso-table-rspace:0pt; }

    /* Styling your links has become much simpler with the new Yahoo.  In fact, it falls in line with the main credo of styling in email, bring your styles inline.  Your link colors will be uniform across clients when brought inline.

    Bring inline: Yes. */
    a {color: #4169e1;}

    /* Or to go the gold star route...
    a:link { color: orange; }
    a:visited { color: blue; }
    a:hover { color: green; }
    */

  </style>
</head>
<body>
  <!-- Wrapper/Container Table: Use a wrapper table to control the width and the background color consistently of your email. Use this approach instead of setting attributes on the body tag. -->
  <table cellpadding="0" cellspacing="0" border="0" id="backgroundTable">
  <tr>
    <td>

    <!-- Tables are the most common way to format your email consistently. Set your table widths inside cells and in most cases reset cellpadding, cellspacing, and border to zero. Use nested tables as a way to space effectively in your message. -->
    <table cellpadding="0" cellspacing="0" border="0" align="center">
      <tr>
        <td width="600" valign="top">
            <?php
                print '((( Reply ABOVE this LINE to POST a COMMENT )))';
                print $variables['params']['header'];
            ?>
        </td>
      </tr>
      <tr>
        <td width="600" valign="top">
          <?php
            print $variables['params']['body'];
            print "</br>";
            print $variables['params']['files'];
          ?>
        </td>       
      </tr>
      <tr>
        <td width="600" valign="top">
          <?php print $variables['params']['footer']; ?>
						<br>
					<?php print $variables['params']['staff_link']; ?>
        </td>
      </tr>
    </table>

    <!-- End example table -->

    </td>
  </tr>
  </table>
  <!-- End of wrapper table -->
</body>
</html>