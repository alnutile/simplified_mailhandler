<div class="box8 vdt-title-area">
    <h1><?php print $variables['params']['title']; ?></h1>
</div>
<div class="container-full tool-node">
    <div class='loader-vdt box2'></div>
    <div class="container-full virtual-design-tool-container node">
        <div class="main-display box6-custom">
            <div class="large-image">
							<?php print $variables['params']['largeImage']; ?>
            </div>
            <div class="orientation-bucket">
							<?php print $variables['params']['orientation_assets']; ?>
            </div>
        </div>



        <div class='right-tool box5-custom'>
            <div class="box3-custom textures-colors">
                <label class="texture-label-vdt box3-custom">Texture:<?php print 	$variables['params']['textures_title']; ?></label>
                <div class="textures full-width"><?php print $variables['params']['textures']; ?></div>

                <label class="colors box3-custom">Color:<?php print 	$variables['params']['color_title']; ?></label>
                <div class="colors full-width"><?php print $variables['params']['colors']; ?></div>
            </div>
        </div>
        <div class="box12-custom orients-shapes">
            <div class="shapes box5-custom">
							<?php print $variables['params']['shapes']; ?>
            </div>

            <div class="orientation-bar box5-custom pull-left">
							<?php print $variables['params']['orientations']; ?>
            </div>
        </div>

        <div class="see-more box8-custom">
            <strong>Want to see more?</strong> Visit the <?php print $variables['params']['link_and_name']; ?> product page to order samples, view larger swatches, and browse the product showcase.
        </div>
    </div>

</div>

<div class="box3-custom vd-product-list">
    <div class="vd-product-list-inner">
        <div class="vd-product-list-desc">
            <div class="title">Select a Product</div>
            <div class="subtitle">Use the list below to select a product to view in the virtual design tool.</div>
        </div>
        <div class="vd-product-scroll">
            <div class="vd-product-scroll-inner">
							<?php print $variables['params']['list']; ?>
            </div>
        </div>
    </div>
</div>
