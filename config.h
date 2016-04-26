/*
 * pg config.h git 04/25/2016 boobah <anon>
 * ix <arcetera@openmailbox.org>
 */

typedef const unsigned int bool;

#define BLANK  CSI "30m~" CSI "m" /* string shown on each line after EOF */
#define K_DOWN 'j'                /* vi-style scroll-down key            */
#define K_UP   'k'                /* vi-style scroll-up   key            */

bool b_errorbell = 0;             /* ring terminal bell on errors        */
