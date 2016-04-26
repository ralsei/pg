/*
 * pg config.h git 04/25/2016 boobah <anon>
 * ix <arcetera@openmailbox.org>
 */

typedef const unsigned int bool;

#define BLANK  CSI "30m~" CSI "m" /* string shown on each line after EOF */
bool b_errorbell = 0;             /* ring terminal bell on errors        */

/*
 * keys to move up with vi binds;
 * change depending on your keyboard
 * layout if you use dvorak or colemak et al
 */
static const char viup = 'k'; /* up key   */
static const char vidn = 'j'; /* down key */
