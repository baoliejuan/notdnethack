/*	SCCS Id: @(#)options.c	3.4	2003/11/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef OPTION_LISTS_ONLY	/* (AMIGA) external program for opt lists */
#include "config.h"
#include "objclass.h"
#include "flag.h"
NEARDATA struct flag flags;	/* provide linkage */
NEARDATA struct instance_flags iflags;	/* provide linkage */
#define static
#else
#include "hack.h"
#include "tcap.h"
#include "botl.h"
#include <ctype.h>
#endif
#include <errno.h>

#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif

#define WINTYPELEN 16

#ifdef DEFAULT_WC_TILED_MAP
#define PREFER_TILED TRUE
#else
#define PREFER_TILED FALSE
#endif

#ifdef CURSES_GRAPHICS
extern int curses_read_attrs(char *attrs);
#endif

/*
 *  NOTE:  If you add (or delete) an option, please update the short
 *  options help (option_help()), the long options help (dat/opthelp),
 *  and the current options setting display function (doset()),
 *  and also the Guidebooks.
 *
 *  The order matters.  If an option is a an initial substring of another
 *  option (e.g. time and timed_delay) the shorter one must come first.
 */

static struct Bool_Opt
{
	const char *name;
	boolean	*addr, initvalue;
	int optflags;
} boolopt[] = {
#ifdef AMIGA
	{"altmeta", &flags.altmeta, TRUE, DISP_IN_GAME},
#else
	{"altmeta", (boolean *)0, TRUE, DISP_IN_GAME},
#endif
	{"ascii_map",     &iflags.wc_ascii_map, !PREFER_TILED, SET_IN_GAME},	/*WC*/
#ifdef MFLOPPY
	{"asksavedisk", &flags.asksavedisk, FALSE, SET_IN_GAME},
#else
	{"asksavedisk", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"autodig", &flags.autodig, FALSE, SET_IN_GAME},
	{"autoopen", &iflags.autoopen, TRUE, SET_IN_GAME},
	{"autopickup", &flags.pickup, TRUE, SET_IN_GAME},
	{"apexception_regex", &iflags.ape_regex, FALSE,  SET_IN_FILE},
	{"autoquiver", &flags.autoquiver, FALSE, SET_IN_GAME},
	{"autounlock", &flags.autounlock, TRUE, SET_IN_GAME},
#if defined(MICRO) && !defined(AMIGA)
	{"BIOS", &iflags.BIOS, FALSE, SET_IN_FILE},
#else
	{"BIOS", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"botl_updates", &iflags.botl_updates, TRUE, SET_IN_GAME},
	{"bones", (boolean *)&iflags.bones, TRUE, SET_IN_GAME},
#ifdef INSURANCE
	{"checkpoint", &flags.ins_chkpt, TRUE, SET_IN_GAME},
#else
	{"checkpoint", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef MFLOPPY
	{"checkspace", &iflags.checkspace, TRUE, SET_IN_GAME},
#else
	{"checkspace", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef CURSES_GRAPHICS
	{"classic_status", &iflags.classic_status, TRUE, SET_IN_FILE},
	{"classic_colors", &iflags.classic_colors, TRUE, DISP_IN_GAME},
#endif
	{"cmdassist", &iflags.cmdassist, TRUE, SET_IN_GAME},
# if defined(MICRO) || defined(WIN32) || defined(CURSES_GRAPHICS)
	{"color",         &iflags.wc_color,TRUE, SET_IN_GAME},		/*WC*/
# else	/* systems that support multiple terminals, many monochrome */
	{"color",         &iflags.wc_color, FALSE, SET_IN_GAME},	/*WC*/
# endif
#ifdef CURSES_GRAPHICS
	{"cursesgraphics", &iflags.cursesgraphics, TRUE, SET_IN_GAME},
#else
	{"cursesgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#if defined(TERMLIB) && !defined(MAC_GRAPHICS_ENV)
	{"DECgraphics", &iflags.DECgraphics, FALSE, SET_IN_GAME},
#else
	{"DECgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"eight_bit_tty", &iflags.wc_eight_bit_input, FALSE, SET_IN_GAME},	/*WC*/
#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS)
	{"extmenu", &iflags.extmenu, FALSE, SET_IN_GAME},
#else
	{"extmenu", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef OPT_DISPMAP
	{"fast_map", &flags.fast_map, TRUE, SET_IN_GAME},
#else
	{"fast_map", (boolean *)0, TRUE, SET_IN_FILE},
#endif
	{"female", &flags.female, FALSE, DISP_IN_GAME},
	{"fixinv", &flags.invlet_constant, TRUE, SET_IN_GAME},
#ifdef AMIFLUSH
	{"flush", &flags.amiflush, FALSE, SET_IN_GAME},
#else
	{"flush", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"fullscreen", &iflags.wc2_fullscreen, FALSE, SET_IN_FILE},
	{"guicolor", &iflags.wc2_guicolor, TRUE, SET_IN_GAME},
	{"help", &flags.help, TRUE, SET_IN_GAME},
	{"hilite_pet",    &iflags.wc_hilite_pet, TRUE, SET_IN_GAME},	/*WC*/
#ifdef WIN32CON
	{"hilite_peaceful",    &iflags.wc_hilite_peaceful, FALSE, SET_IN_GAME},	/*WC*/
	{"hilite_detected",    &iflags.wc_hilite_detected, FALSE, SET_IN_GAME},	/*WC*/
	{"use_inverse",   &iflags.wc_inverse, TRUE, SET_IN_GAME},		/*WC*/
#else
	{"hilite_peaceful",    &iflags.wc_hilite_peaceful, TRUE, SET_IN_GAME},	/*WC*/
	{"hilite_detected",    &iflags.wc_hilite_detected, TRUE, SET_IN_GAME},	/*WC*/
	{"use_inverse",   &iflags.wc_inverse, FALSE, SET_IN_GAME},		/*WC*/
#endif
	{"hilite_hidden_stairs",    &iflags.hilite_hidden_stairs, TRUE, SET_IN_GAME},	/*WC*/
	{"hilite_obj_piles",    &iflags.hilite_obj_piles, FALSE, SET_IN_GAME},	/*WC*/
	{"default_template_hilite", &iflags.default_template_hilite, TRUE, SET_IN_FILE },
	{"dnethack_start_text",    &iflags.dnethack_start_text, TRUE, DISP_IN_GAME},
	{"artifact_descriptors",    &iflags.artifact_descriptors, FALSE, SET_IN_GAME},
	{"force_artifact_names",    &iflags.force_artifact_names, TRUE, SET_IN_GAME},
	{"role_obj_names",    &iflags.role_obj_names, TRUE, SET_IN_GAME},
	{"obscure_role_obj_names",    &iflags.obscure_role_obj_names, FALSE, SET_IN_GAME},
	{"dnethack_dungeon_colors",    &iflags.dnethack_dungeon_colors, TRUE, SET_IN_GAME},
	{"invweight",    &iflags.invweight, TRUE, SET_IN_GAME},
	{"hitpointbar", &iflags.hitpointbar, FALSE, SET_IN_GAME},
	{"hp_monitor", (boolean *)0, TRUE, SET_IN_FILE}, /* For backward compat, HP monitor patch */
	{"hp_notify", &iflags.hp_notify, FALSE, SET_IN_GAME},
#ifdef ASCIIGRAPH
	{"IBMgraphics", &iflags.IBMgraphics, FALSE, SET_IN_GAME},
#else
	{"IBMgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifndef MAC
	{"ignintr", &flags.ignintr, FALSE, SET_IN_GAME},
#else
	{"ignintr", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"item_use_menu", &iflags.item_use_menu, TRUE, SET_IN_GAME},
	{"large_font", &iflags.obsolete, FALSE, SET_IN_FILE},	/* OBSOLETE */
	{"legacy", &flags.legacy, TRUE, DISP_IN_GAME},
	{"lit_corridor", &flags.lit_corridor, TRUE, SET_IN_GAME},
	{"lootabc", &iflags.lootabc, FALSE, SET_IN_GAME},
#ifdef MAC_GRAPHICS_ENV
	{"Macgraphics", &iflags.MACgraphics, TRUE, SET_IN_GAME},
#else
	{"Macgraphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef MAIL
	{"mail", &flags.biff, TRUE, SET_IN_GAME},
#else
	{"mail", (boolean *)0, TRUE, SET_IN_FILE},
#endif
#ifdef MENU_COLOR
# ifdef MICRO
	{"menucolors", &iflags.use_menu_color, TRUE,  SET_IN_GAME},
# else
	{"menucolors", &iflags.use_menu_color, FALSE, SET_IN_GAME},
# endif
#else
	{"menucolors", (boolean *)0, FALSE, SET_IN_GAME},
#endif
	{"menu_glyphs", &iflags.use_menu_glyphs, FALSE, SET_IN_GAME},
#ifdef WIZARD
	/* for menu debugging only*/
	{"menu_tab_sep", &iflags.menu_tab_sep, FALSE, SET_IN_GAME},
#else
	{"menu_tab_sep", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"mod_turncount", &iflags.mod_turncount, FALSE, SET_IN_GAME},
#ifdef CURSES_GRAPHICS
	{"mouse_support", &iflags.wc_mouse_support, FALSE, DISP_IN_GAME},	/*WC*/
#else
	{"mouse_support", &iflags.wc_mouse_support, TRUE, DISP_IN_GAME},	/*WC*/
#endif
	{"msgtype_regex", &iflags.msgtype_regex, FALSE,  SET_IN_FILE},
#ifdef NEWS
	{"news", &iflags.news, TRUE, DISP_IN_GAME},
#else
	{"news", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"msg_wall_hits", &iflags.notice_walls, FALSE, SET_IN_GAME},
	{"block_forget_map", &iflags.no_forget_map, FALSE, SET_IN_GAME},
	{"null", &flags.null, TRUE, SET_IN_GAME},
	{"old_C_behaviour", &iflags.old_C_behaviour, FALSE, SET_IN_GAME},
#ifdef MAC
	{"page_wait", &flags.page_wait, TRUE, SET_IN_GAME},
#else
	{"page_wait", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"paranoid_self_cast", &iflags.paranoid_self_cast, FALSE, SET_IN_GAME},
	{"paranoid_hit", &iflags.paranoid_hit, FALSE, SET_IN_GAME},
	{"paranoid_quit", &iflags.paranoid_quit, FALSE, SET_IN_GAME},
	{"paranoid_remove", &iflags.paranoid_remove, FALSE, SET_IN_GAME},
	{"paranoid_swim", &iflags.paranoid_swim, TRUE, SET_IN_GAME},
	{"paranoid_wand_break", &iflags.paranoid_wand_break, TRUE, SET_IN_GAME},
	{"perm_invent", &flags.perm_invent, FALSE, SET_IN_GAME},
       {"pickup_thrown", &iflags.pickup_thrown, FALSE, SET_IN_GAME},
	{"polearm_old_style", &flags.standard_polearms, FALSE, SET_IN_GAME},
	{"polearm_peace_safe", &flags.peacesafe_polearms, TRUE, SET_IN_GAME},
	{"polearm_pet_safe", &flags.petsafe_polearms, TRUE, SET_IN_GAME},
	{"polearm_sequential_letters", &flags.relative_polearms, TRUE, SET_IN_GAME},
	{"popup_dialog",  &iflags.wc_popup_dialog, FALSE, SET_IN_GAME},	/*WC*/
	{"prayconfirm", &flags.prayconfirm, TRUE, SET_IN_GAME},
	{"preload_tiles", &iflags.wc_preload_tiles, TRUE, DISP_IN_GAME},	/*WC*/
	{"pushweapon", &flags.pushweapon, FALSE, SET_IN_GAME},
	{"querytype_regex", &iflags.querytype_regex, FALSE, SET_IN_FILE},
	{"quick_m_abilities", &iflags.quick_m_abilities, TRUE, SET_IN_GAME },
	{"quiver_fired", &iflags.quiver_fired, TRUE, SET_IN_GAME},
#ifdef QWERTZ
	{"qwertz_movement", &iflags.qwertz_movement, FALSE, SET_IN_GAME},
#endif
#if defined(MICRO) && !defined(AMIGA)
	{"rawio", &iflags.rawio, FALSE, DISP_IN_GAME},
#else
	{"rawio", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"rest_on_space", &flags.rest_on_space, FALSE, SET_IN_GAME},
	{"safe_pet", &flags.safe_dog, TRUE, SET_IN_GAME},
#ifdef WIZARD
	{"sanity_check", &iflags.sanity_check, FALSE, SET_IN_GAME},
#else
	{"sanity_check", (boolean *)0, FALSE, SET_IN_FILE},
#endif
#ifdef SHOW_BORN
	{"showborn", &iflags.show_born, FALSE, SET_IN_GAME},
#endif
	{"showbuc", &iflags.show_buc, FALSE, SET_IN_GAME},
#ifdef EXP_ON_BOTL
	{"showexp", &flags.showexp, FALSE, SET_IN_GAME},
#else
	{"showexp", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"show_obj_sym", &iflags.show_obj_sym, TRUE, SET_IN_GAME},
	{"showrace", &iflags.showrace, FALSE, SET_IN_GAME},
#ifdef REALTIME_ON_BOTL
        {"showrealtime", &iflags.showrealtime, FALSE, SET_IN_GAME},
#endif
#ifdef SCORE_ON_BOTL
	{"showscore", &flags.showscore, FALSE, SET_IN_GAME},
#else
	{"showscore", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"show_shop_prices", &iflags.show_shop_prices, TRUE, SET_IN_GAME},
	{"silent", &flags.silent, TRUE, SET_IN_GAME},
	{"softkeyboard", &iflags.wc2_softkeyboard, FALSE, SET_IN_FILE},
	{"sortpack", &flags.sortpack, TRUE, SET_IN_GAME},
	{"sound", &flags.soundok, TRUE, SET_IN_GAME},
	{"sparkle", &flags.sparkle, TRUE, SET_IN_GAME},
	{"standout", &flags.standout, FALSE, SET_IN_GAME},
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
        {"statuscolors", &iflags.use_status_colors, TRUE, SET_IN_GAME},
#else
        {"statuscolors", (boolean *)0, TRUE, SET_IN_GAME},
#endif
	{"splash_screen",     &iflags.wc_splash_screen, TRUE, DISP_IN_GAME},	/*WC*/
	{"suppress hurtness", &flags.suppress_hurtness, FALSE, SET_IN_GAME},
	{"tiled_map",     &iflags.wc_tiled_map, PREFER_TILED, DISP_IN_GAME},	/*WC*/
	{"time", &flags.time, FALSE, SET_IN_GAME},
#ifdef TIMED_DELAY
	{"timed_delay", &flags.nap, TRUE, SET_IN_GAME},
#else
	{"timed_delay", (boolean *)0, FALSE, SET_IN_GAME},
#endif
	{"tombstone",&flags.tombstone, TRUE, SET_IN_GAME},
	{"toptenwin",&flags.toptenwin, FALSE, SET_IN_GAME},
	{"travel", &iflags.travelcmd, TRUE, SET_IN_GAME},
#ifdef UTF8_GLYPHS
	{"UTF8graphics", &iflags.UTF8graphics, FALSE, SET_IN_GAME},
#else
	{"UTF8graphics", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"use_darkgray", &iflags.wc2_darkgray, TRUE, SET_IN_FILE},
#ifdef WIN32CON
	{"use_inverse",   &iflags.wc_inverse, TRUE, SET_IN_GAME},		/*WC*/
#else
	{"use_inverse",   &iflags.wc_inverse, FALSE, SET_IN_GAME},		/*WC*/
#endif
#ifdef WIN_EDGE
	{"win_edge", &iflags.win_edge, FALSE, SET_IN_GAME},
#else
	{"win_edge", (boolean *)0, TRUE, SET_IN_FILE},
#endif
	{"verbose", &flags.verbose, TRUE, SET_IN_GAME},
#ifdef USE_TILES
	{"vt_tiledata", &iflags.vt_nethack, FALSE, SET_IN_GAME},
#else
	{"vt_tiledata", (boolean *)0, FALSE, SET_IN_FILE},
#endif
	{"wraptext", &iflags.wc2_wraptext, FALSE, SET_IN_GAME},
	{(char *)0, (boolean *)0, FALSE, 0}
};

/* compound options, for option_help() and external programs like Amiga
 * frontend */
static struct Comp_Opt
{
	const char *name, *descr;
	int size;	/* for frontends and such allocating space --
			 * usually allowed size of data in game, but
			 * occasionally maximum reasonable size for
			 * typing when game maintains information in
			 * a different format */
	int optflags;
} compopt[] = {
	{ "align",    "your starting alignment (lawful, neutral, or chaotic)",
						8, DISP_IN_GAME },
	{ "align_message", "message window alignment", 20, DISP_IN_GAME }, 	/*WC*/
	{ "align_status", "status window alignment", 20, DISP_IN_GAME }, 	/*WC*/
	{ "altkeyhandler", "alternate key handler", 20, DISP_IN_GAME },
	{ "attack_mode", "attack, refrain or ask to attack monsters", 1,
		SET_IN_GAME },
	{ "boulder",  "the symbol to use for displaying boulders",
						1, SET_IN_GAME },
	{ "catname",  "the name of your (first) cat (e.g., catname:Tabby)",
						PL_PSIZ, DISP_IN_GAME },
	{ "chaos_quest",    "tournament chaos quest (temple, mithardir, or mordor)",
						10, DISP_IN_GAME },
	{ "delay_length", "the length of delays when rendering animation",
						sizeof("normal"), DISP_IN_GAME },
	{ "disclose", "the kinds of information to disclose at end of game",
						sizeof(flags.end_disclose) * 2,
						SET_IN_GAME },
	{ "dogname",  "the name of your (first) dog (e.g., dogname:Fang)",
						PL_PSIZ, DISP_IN_GAME },
#ifdef DUMP_LOG
	{ "dumpfile", "where to dump data (e.g., dumpfile:/tmp/dump.nh)",
#ifdef DUMP_FN
						PL_PSIZ, DISP_IN_GAME
#else
						PL_PSIZ, SET_IN_GAME
#endif
	},
#endif
	{ "dungeon",  "the symbols to use in drawing the dungeon map",
						MAXDCHARS+1, SET_IN_FILE },
	{ "effects",  "the symbols to use in drawing special effects",
						MAXECHARS+1, SET_IN_FILE },
	{ "font_map", "the font to use in the map window", 40, DISP_IN_GAME },	/*WC*/
	{ "font_menu", "the font to use in menus", 40, DISP_IN_GAME },		/*WC*/
	{ "font_message", "the font to use in the message window",
						40, DISP_IN_GAME },		/*WC*/
	{ "font_size_map", "the size of the map font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_menu", "the size of the menu font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_message", "the size of the message font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_status", "the size of the status font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_size_text", "the size of the text font", 20, DISP_IN_GAME },	/*WC*/
	{ "font_status", "the font to use in status window", 40, DISP_IN_GAME }, /*WC*/
	{ "font_text", "the font to use in text windows", 40, DISP_IN_GAME },	/*WC*/
	{ "fruit",    "the name of a fruit you enjoy eating",
						PL_FSIZ, SET_IN_GAME },
	{ "gender",   "your starting gender (male or female)",
						8, DISP_IN_GAME },
	{ "horsename", "the name of your (first) horse (e.g., horsename:Silver)",
						PL_PSIZ, DISP_IN_GAME },
	{ "inherited",  "the name of your chosen inherited artifact (e.g., inherited:Dirge)",
						PL_PSIZ, DISP_IN_GAME },
	{ "hp_notify_fmt", "hp_notify format string", 20, SET_IN_GAME },
	{ "map_mode", "map display mode under Windows", 20, DISP_IN_GAME },	/*WC*/
	{ "menucolor", "set menu colors", PL_PSIZ, SET_IN_FILE },
	{ "menustyle", "user interface for object selection",
						MENUTYPELEN, SET_IN_GAME },
	{ "menu_deselect_all", "deselect all items in a menu", 4, SET_IN_FILE },
	{ "menu_deselect_page", "deselect all items on this page of a menu",
						4, SET_IN_FILE },
	{ "menu_first_page", "jump to the first page in a menu",
						4, SET_IN_FILE },
	{ "menu_headings", "bold, inverse, or underline headings", 9, SET_IN_GAME },
	{ "menu_invert_all", "invert all items in a menu", 4, SET_IN_FILE },
	{ "menu_invert_page", "invert all items on this page of a menu",
						4, SET_IN_FILE },
	{ "menu_last_page", "jump to the last page in a menu", 4, SET_IN_FILE },
	{ "menu_next_page", "goto the next menu page", 4, SET_IN_FILE },
	{ "menu_previous_page", "goto the previous menu page", 4, SET_IN_FILE },
	{ "menu_search", "search for a menu item", 4, SET_IN_FILE },
	{ "menu_select_all", "select all items in a menu", 4, SET_IN_FILE },
	{ "menu_select_page", "select all items on this page of a menu",
						4, SET_IN_FILE },
	{ "monsters", "the symbols to use for monsters",
						MAXMCLASSES, SET_IN_FILE },
	{ "msghistory", "number of top line messages to save",
						5, DISP_IN_GAME },
# ifdef TTY_GRAPHICS
	{"msg_window", "the type of message window required",1, SET_IN_GAME},
# else
	{"msg_window", "the type of message window required", 1, SET_IN_FILE},
# endif
	{ "name",     "your character's name (e.g., name:Merlin-W)",
						PL_NSIZ, DISP_IN_GAME },
	{ "number_pad", "use the number pad", 1, SET_IN_GAME},
	{ "objects",  "the symbols to use for objects",
						MAXOCLASSES, SET_IN_FILE },
	{ "packorder", "the inventory order of the items in your pack",
						MAXOCLASSES, SET_IN_GAME },
#ifdef CHANGE_COLOR
	{ "palette",  "palette (00c/880/-fff is blue/yellow/reverse white)",
						15 , SET_IN_GAME },
# if defined(MAC)
	{ "hicolor",  "same as palette, only order is reversed",
						15, SET_IN_FILE },
# endif
#endif
	{ "petattr",  "attributes for highlighting pets", 12, SET_IN_FILE },
	{ "pettype",  "your preferred initial pet type", 4, DISP_IN_GAME },
	{ "pickup_burden",  "maximum burden picked up before prompt",
						20, SET_IN_GAME },
	{ "pickup_types", "types of objects to pick up automatically",
						MAXOCLASSES, SET_IN_GAME },
	{ "player_selection", "choose character via dialog or prompts",
						12, DISP_IN_GAME },
	{ "pokedex", "default details shown in the pokedex",
						32, SET_IN_GAME },
	{ "race",     "your starting race (e.g., Human, Elf)",
						PL_CSIZ, DISP_IN_GAME },
#ifdef CONVICT
	{ "ratname",  "the name of your (first) rat (e.g., ratname:Squeak)",
						PL_PSIZ, DISP_IN_GAME },
#endif /* CONVICT */
	{ "role",     "your starting role (e.g., Barbarian, Valkyrie)",
						PL_CSIZ, DISP_IN_GAME },
	{ "runmode", "display frequency when `running' or `travelling'",
						sizeof "teleport", SET_IN_GAME },
	{ "scores",   "the parts of the score list you wish to see",
						32, SET_IN_GAME },
	{ "scroll_amount", "amount to scroll map when scroll_margin is reached",
						20, DISP_IN_GAME }, /*WC*/
	{ "scroll_margin", "scroll map when this far from the edge", 20, DISP_IN_GAME }, /*WC*/
	{ "species",    "your starting species (e.g., poplar, blue)",
						PL_CSIZ, DISP_IN_GAME },
#ifdef SORTLOOT
	{ "sortloot", "sort object selection lists by description", 4, SET_IN_GAME },
#endif
#ifdef MSDOS
	{ "soundcard", "type of sound card to use", 20, SET_IN_FILE },
#endif
	{ "statuseffects", "status effects to show in the status line", 20, SET_IN_GAME },
	{ "statuslines", "number of status lines (2, 3, or 4)", 20, SET_IN_GAME },
	{ "suppress_alert", "suppress alerts about version-specific features",
						8, SET_IN_GAME },
	{ "tile_width", "width of tiles", 20, DISP_IN_GAME},	/*WC*/
	{ "tile_height", "height of tiles", 20, DISP_IN_GAME},	/*WC*/
	{ "tile_file", "name of tile file", 70, DISP_IN_GAME},	/*WC*/
	{ "traps",    "the symbols to use in drawing traps",
						MAXTCHARS+1, SET_IN_FILE },
	{ "travelplus", "maximum unknown-explore distance when traveling", 32, SET_IN_GAME},
	{ "vary_msgcount", "show more old messages at a time", 20, DISP_IN_GAME }, /*WC*/
#ifdef MSDOS
	{ "video",    "method of video updating", 20, SET_IN_FILE },
#endif
#ifdef VIDEOSHADES
	{ "videocolors", "color mappings for internal screen routines",
						40, DISP_IN_GAME },
	{ "videoshades", "gray shades to map to black/gray/white",
						32, DISP_IN_GAME },
#endif
#ifdef WIN32CON
	{"subkeyvalue", "override keystroke value", 7, SET_IN_FILE},
#endif
	{ "windowcolors",  "the foreground/background colors of windows",	/*WC*/
						80, DISP_IN_GAME },
	{ "windowtype", "windowing system to use", WINTYPELEN, DISP_IN_GAME },
	{ "wizlevelport", "wiz-mode ^V settings", 3, SET_IN_GAME},
	{ "wizcombatdebug", "wiz-mode combat debug options", 6, SET_IN_GAME },
	{ (char *)0, (char *)0, 0, 0 }
};

#ifdef OPTION_LISTS_ONLY
#undef static

#else	/* use rest of file */

static boolean need_redraw; /* for doset() */

#if defined(TOS) && defined(TEXTCOLOR)
extern boolean colors_changed;	/* in tos.c */
#endif

#ifdef VIDEOSHADES
extern char *shade[3];		  /* in sys/msdos/video.c */
extern char ttycolors[CLR_MAX];	  /* in sys/msdos/video.c */
#endif

static char def_inv_order[MAXOCLASSES] = {
	COIN_CLASS, AMULET_CLASS, WEAPON_CLASS, ARMOR_CLASS, BELT_CLASS, FOOD_CLASS,
	SCROLL_CLASS, TILE_CLASS, SPBOOK_CLASS, POTION_CLASS, RING_CLASS, WAND_CLASS,
	TOOL_CLASS, GEM_CLASS, ROCK_CLASS, BALL_CLASS, CHAIN_CLASS, BED_CLASS, SCOIN_CLASS, 0,
};

/*
 * Default menu manipulation command accelerators.  These may _not_ be:
 *
 *	+ a number - reserved for counts
 *	+ an upper or lower case US ASCII letter - used for accelerators
 *	+ ESC - reserved for escaping the menu
 *	+ NULL, CR or LF - reserved for commiting the selection(s).  NULL
 *	  is kind of odd, but the tty's xwaitforspace() will return it if
 *	  someone hits a <ret>.
 *	+ a default object class symbol - used for object class accelerators
 *
 * Standard letters (for now) are:
 *
 *		<  back 1 page
 *		>  forward 1 page
 *		^  first page
 *		|  last page
 *		:  search
 *
 *		page		all
 *		 ,    select	 .
 *		 \    deselect	 -
 *		 ~    invert	 @
 *
 * The command name list is duplicated in the compopt array.
 */
typedef struct {
    const char *name;
    char cmd;
} menu_cmd_t;

#define NUM_MENU_CMDS 11
static const menu_cmd_t default_menu_cmd_info[NUM_MENU_CMDS] = {
/* 0*/	{ "menu_first_page",	MENU_FIRST_PAGE },
	{ "menu_last_page",	MENU_LAST_PAGE },
	{ "menu_next_page",	MENU_NEXT_PAGE },
	{ "menu_previous_page",	MENU_PREVIOUS_PAGE },
	{ "menu_select_all",	MENU_SELECT_ALL },
/* 5*/	{ "menu_deselect_all",	MENU_UNSELECT_ALL },
	{ "menu_invert_all",	MENU_INVERT_ALL },
	{ "menu_select_page",	MENU_SELECT_PAGE },
	{ "menu_deselect_page",	MENU_UNSELECT_PAGE },
	{ "menu_invert_page",	MENU_INVERT_PAGE },
/*10*/	{ "menu_search",		MENU_SEARCH },
};

/*
 * Allow the user to map incoming characters to various menu commands.
 * The accelerator list must be a valid C string.
 */
#define MAX_MENU_MAPPED_CMDS 32	/* some number */
       char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS+1];	/* exported */
static char mapped_menu_op[MAX_MENU_MAPPED_CMDS+1];
static short n_menu_mapped = 0;


static boolean initial, from_file;

STATIC_DCL void FDECL(doset_add_menu, (winid,const char *,int));
STATIC_DCL void FDECL(nmcpy, (char *, const char *, int));
STATIC_DCL void FDECL(escapes, (const char *, char *));
STATIC_DCL void FDECL(rejectoption, (const char *));
STATIC_DCL void FDECL(badoption, (const char *));
STATIC_DCL char *FDECL(string_for_opt, (char *,BOOLEAN_P));
STATIC_DCL char *FDECL(string_for_env_opt, (const char *, char *,BOOLEAN_P));
STATIC_DCL void FDECL(bad_negation, (const char *,BOOLEAN_P));
STATIC_DCL int FDECL(change_inv_order, (char *));
STATIC_DCL void FDECL(oc_to_str, (char *, char *));
STATIC_DCL void FDECL(graphics_opts, (char *,const char *,int,int));
STATIC_DCL int FDECL(feature_alert_opts, (char *, const char *));
STATIC_DCL const char *FDECL(get_compopt_value, (const char *, char *));
STATIC_DCL boolean FDECL(special_handling, (const char *, BOOLEAN_P, BOOLEAN_P));
STATIC_DCL void FDECL(warning_opts, (char *,const char *));
STATIC_DCL void FDECL(duplicate_opt_detection, (const char *, int));

STATIC_OVL void FDECL(wc_set_font_name, (int, char *));
STATIC_OVL int FDECL(wc_set_window_colors, (char *));
STATIC_OVL boolean FDECL(is_wc_option, (const char *));
STATIC_OVL boolean FDECL(wc_supported, (const char *));
STATIC_OVL boolean FDECL(is_wc2_option, (const char *));
STATIC_OVL boolean FDECL(wc2_supported, (const char *));
#ifdef AUTOPICKUP_EXCEPTIONS
STATIC_DCL void FDECL(remove_autopickup_exception, (struct autopickup_exception *));
STATIC_OVL int FDECL(count_ape_maps, (int *, int *));
#endif

/* check whether a user-supplied option string is a proper leading
   substring of a particular option name; option string might have
   a colon or equals sign and arbitrary value appended to it */
boolean
match_optname(user_string, opt_name, min_length, val_allowed)
const char *user_string, *opt_name;
int min_length;
boolean val_allowed;
{
	int len = (int)strlen(user_string);

	if (val_allowed) {
	    const char *p = index(user_string, ':'),
		       *q = index(user_string, '=');

	    if (!p || (q && q < p)) p = q;
	    while(p && p > user_string && isspace(*(p-1))) p--;
	    if (p) len = (int)(p - user_string);
	}

	return (len >= min_length) && !strncmpi(opt_name, user_string, len);
}

/* most environment variables will eventually be printed in an error
 * message if they don't work, and most error message paths go through
 * BUFSZ buffers, which could be overflowed by a maliciously long
 * environment variable.  if a variable can legitimately be long, or
 * if it's put in a smaller buffer, the responsible code will have to
 * bounds-check itself.
 */
char *
nh_getenv(ev)
const char *ev;
{
	char *getev = getenv(ev);

	if (getev && strlen(getev) <= (BUFSZ / 2))
		return getev;
	else
		return (char *)0;
}

void
initoptions()
{
#ifndef MAC
	char *opts;
#endif
	int i;

	/* initialize the random number generator */
	setrandom();

	/* for detection of configfile options specified multiple times */
	iflags.opt_booldup = iflags.opt_compdup = (int *)0;
	
	for (i = 0; boolopt[i].name; i++) {
		if (boolopt[i].addr)
			*(boolopt[i].addr) = boolopt[i].initvalue;
	}
	flags.end_own = FALSE;
	flags.end_top = 3;
	flags.end_around = 2;
	iflags.travelplus = 0;
	iflags.runmode = RUN_LEAP;
        iflags.delay_length = RUN_STEP;
	iflags.pokedex = POKEDEX_SHOW_DEFAULT;
	iflags.wizlevelport = WIZLVLPORT_TRADITIONAL;
	iflags.wizcombatdebug = WIZCOMBATDEBUG_DMG | WIZCOMBATDEBUG_UVM;
	iflags.msg_history = 20;
#ifdef TTY_GRAPHICS
	iflags.prevmsg_window = 's';
#endif
	iflags.menu_headings = ATR_INVERSE;
	iflags.attack_mode = ATTACK_MODE_CHAT;
	iflags.statuseffects = ~0;
	iflags.statuslines = 4;

	/* Use negative indices to indicate not yet selected */
	flags.initrole = -1;
	flags.initrace = -1;
	flags.descendant = -1;
	flags.initgend = -1;
	flags.initalign = -1;
	flags.initspecies = -1;


	/* Set the default monster and object class symbols.  Don't use */
	/* memcpy() --- sizeof char != sizeof uchar on some machines.	*/
	for (i = 0; i < MAXOCLASSES; i++)
		oc_syms[i] = (uchar) def_oc_syms[i];
	for (i = 0; i < MAXMCLASSES; i++)
		monsyms[i] = (uchar) def_monsyms[i];
	for (i = 0; i < WARNCOUNT; i++)
		warnsyms[i] = def_warnsyms[i].sym;

#ifdef USER_DUNGEONCOLOR
	assign_colors((uchar *)0, 0, MAXPCHARS, 0);
#endif

/* FIXME: These should be integrated into objclass and permonst structs,
   but that invalidates saves */
	memset(objclass_unicode_codepoint, 0, sizeof(objclass_unicode_codepoint));
	memset(permonst_unicode_codepoint, 0, sizeof(permonst_unicode_codepoint));

	iflags.bouldersym = 0;
	iflags.travelcc.x = iflags.travelcc.y = -1;
	flags.warnlevel = 1;
	flags.warntypem = 0L;
	flags.warntypet = 0L;
	flags.warntypeb = 0L;
	flags.warntypeg = 0L;
	flags.warntypea = 0L;
	flags.warntypev = 0L;
	flags.montype = (long long int)0;

#ifdef SORTLOOT
	iflags.sortloot = 'n';
#endif

     /* assert( sizeof flags.inv_order == sizeof def_inv_order ); */
	(void)memcpy((genericptr_t)flags.inv_order,
		     (genericptr_t)def_inv_order, sizeof flags.inv_order);
	flags.pickup_types[0] = '\0';
	flags.pickup_burden = MOD_ENCUMBER;

	for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++)
		flags.end_disclose[i] = DISCLOSE_PROMPT_DEFAULT_NO;
	switch_graphics(ASCII_GRAPHICS);	/* set default characters */
#if defined(UNIX) && defined(TTY_GRAPHICS)
	/*
	 * Set defaults for some options depending on what we can
	 * detect about the environment's capabilities.
	 * This has to be done after the global initialization above
	 * and before reading user-specific initialization via
	 * config file/environment variable below.
	 */
	/* this detects the IBM-compatible console on most 386 boxes */
	if ((opts = nh_getenv("TERM")) && !strncmp(opts, "AT", 2)) {
		switch_graphics(IBM_GRAPHICS);
# ifdef TEXTCOLOR
		iflags.use_color = TRUE;
# endif
	}
#endif /* UNIX && TTY_GRAPHICS */
#if defined(UNIX) || defined(VMS)
# ifdef TTY_GRAPHICS
	/* detect whether a "vt" terminal can handle alternate charsets */
	if ((opts = nh_getenv("TERM")) &&
	    !strncmpi(opts, "vt", 2) && AS && AE &&
	    index(AS, '\016') && index(AE, '\017')) {
		switch_graphics(DEC_GRAPHICS);
	}
# endif
# ifdef HAVE_SETLOCALE
	/* try to detect if a utf-8 locale is supported */
	if (setlocale(LC_ALL, "") &&
	    (opts = setlocale(LC_CTYPE, NULL)) &&
	    ((strstri(opts, "utf8") != 0) || (strstri(opts, "utf-8") != 0))) {
	        switch_graphics(UTF8_GRAPHICS);
	}
# endif
#endif /* UNIX || VMS */

#ifdef MAC_GRAPHICS_ENV
	switch_graphics(MAC_GRAPHICS);
#endif /* MAC_GRAPHICS_ENV */
	flags.menu_style = MENU_FULL;

	/* since this is done before init_objects(), do partial init here */
	objects[SLIME_MOLD].oc_name_idx = SLIME_MOLD;
	nmcpy(pl_fruit, OBJ_NAME(objects[SLIME_MOLD]), PL_FSIZ);
#ifndef MAC
	opts = getenv("NETHACKOPTIONS");
	if (!opts) opts = getenv("HACKOPTIONS");
	if (opts) {
		if (*opts == '/' || *opts == '\\' || *opts == '@') {
			if (*opts == '@') opts++;	/* @filename */
			/* looks like a filename */
			if (strlen(opts) < BUFSZ/2)
			    read_config_file(opts);
		} else {
			read_config_file((char *)0);
			/* let the total length of options be long;
			 * parseoptions() will check each individually
			 */
			parseoptions(opts, TRUE, FALSE);
		}
	} else
#endif
		read_config_file((char *)0);
	if(iflags.default_template_hilite){
		if(!(iflags.monstertemplate[ZOMBIFIED-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[ZOMBIFIED-1].bg = CLR_GREEN;
			iflags.monstertemplate[ZOMBIFIED-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[SKELIFIED-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[SKELIFIED-1].bg = CLR_GREEN;
			iflags.monstertemplate[SKELIFIED-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[CRYSTALFIED-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[CRYSTALFIED-1].bg = CLR_GREEN;
			iflags.monstertemplate[CRYSTALFIED-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[FRACTURED-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[FRACTURED-1].bg = CLR_GREEN;
			iflags.monstertemplate[FRACTURED-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[TOMB_HERD-1].set&MONSTERTEMPLATE_FOREGROUND)){
			iflags.monstertemplate[TOMB_HERD-1].fg = CLR_GRAY;
		}
		if(!(iflags.monstertemplate[TOMB_HERD-1].set&MONSTERTEMPLATE_SYMBOL)){
			iflags.monstertemplate[TOMB_HERD-1].symbol = '`';
		}
		iflags.monstertemplate[TOMB_HERD-1].set |= MONSTERTEMPLATE_FOREGROUND|MONSTERTEMPLATE_SYMBOL;

		if(!(iflags.monstertemplate[YELLOW_TEMPLATE-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[YELLOW_TEMPLATE-1].bg = CLR_GREEN;
			iflags.monstertemplate[YELLOW_TEMPLATE-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[DREAM_LEECH-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[DREAM_LEECH-1].bg = CLR_GREEN;
			iflags.monstertemplate[DREAM_LEECH-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[MOLY_TEMPLATE-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[MOLY_TEMPLATE-1].bg = CLR_GREEN;
			iflags.monstertemplate[MOLY_TEMPLATE-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[SPORE_ZOMBIE-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[SPORE_ZOMBIE-1].bg = CLR_GREEN;
			iflags.monstertemplate[SPORE_ZOMBIE-1].set |= MONSTERTEMPLATE_BACKGROUND;
		}

		if(!(iflags.monstertemplate[CORDYCEPS-1].set&MONSTERTEMPLATE_SYMBOL)){
			iflags.monstertemplate[CORDYCEPS-1].symbol = 'F';
		}
		if(!(iflags.monstertemplate[CORDYCEPS-1].set&MONSTERTEMPLATE_BACKGROUND)){
			iflags.monstertemplate[CORDYCEPS-1].bg = CLR_GREEN;
		}
		iflags.monstertemplate[CORDYCEPS-1].set |= MONSTERTEMPLATE_BACKGROUND|MONSTERTEMPLATE_SYMBOL;
	}
	(void)fruitadd(pl_fruit);
	/* Remove "slime mold" from list of object names; this will	*/
	/* prevent it from being wished unless it's actually present	*/
	/* as a named (or default) fruit.  Wishing for "fruit" will	*/
	/* result in the player's preferred fruit [better than "\033"].	*/
	obj_descr[SLIME_MOLD].oc_name = "fruit";
	
        if (flags.lit_corridor && iflags.use_color) {
            showsyms[S_drkroom]=showsyms[S_litroom];
        } else {
            showsyms[S_drkroom]=showsyms[S_stone];
        }
	return;
}

STATIC_OVL void
nmcpy(dest, src, maxlen)
	char	*dest;
	const char *src;
	int	maxlen;
{
	int	count;

	for(count = 1; count < maxlen; count++) {
		if(*src == ',' || *src == '\0') break; /*exit on \0 terminator*/
		*dest++ = *src++;
	}
	*dest = 0;
}

/*
 * escapes: escape expansion for showsyms. C-style escapes understood include
 * \n, \b, \t, \r, \xnnn (hex), \onnn (octal), \nnn (decimal). The ^-prefix
 * for control characters is also understood, and \[mM] followed by any of the
 * previous forms or by a character has the effect of 'meta'-ing the value (so
 * that the alternate character set will be enabled).
 */
STATIC_OVL void
escapes(cp, tp)
const char	*cp;
char *tp;
{
    while (*cp)
    {
	int	cval = 0, meta = 0;

	if (*cp == '\\' && index("mM", cp[1])) {
		meta = 1;
		cp += 2;
	}
	if (*cp == '\\' && index("0123456789xXoO", cp[1]))
	{
	    const char *dp, *hex = "00112233445566778899aAbBcCdDeEfF";
	    int dcount = 0;

	    cp++;
	    if (*cp == 'x' || *cp == 'X')
		for (++cp; (dp = index(hex, *cp)) && (dcount++ < 2); cp++)
		    cval = (cval * 16) + (dp - hex) / 2;
	    else if (*cp == 'o' || *cp == 'O')
		for (++cp; (index("01234567",*cp)) && (dcount++ < 3); cp++)
		    cval = (cval * 8) + (*cp - '0');
	    else
		for (; (index("0123456789",*cp)) && (dcount++ < 3); cp++)
		    cval = (cval * 10) + (*cp - '0');
	}
	else if (*cp == '\\' && cp[1] != '\0')		/* C-style character escapes */
	{
	    switch (*++cp)
	    {
	    case '\\': cval = '\\'; break;
	    case 'n': cval = '\n'; break;
	    case 't': cval = '\t'; break;
	    case 'b': cval = '\b'; break;
	    case 'r': cval = '\r'; break;
	    default: cval = *cp;
	    }
	    cp++;
	}
	else if (*cp == '^' && cp[1] != '\0')		/* expand control-character syntax */
	{
	    cval = (*++cp & 0x1f);
	    cp++;
	}
	else if (*cp != '\0') {
	    cval = *cp++;
	} else {
	    cval = 0;
	    meta = 0;
	}
	if (meta)
	    cval |= 0x80;
	*tp++ = cval;
    }
    *tp = '\0';
}

STATIC_OVL void
rejectoption(optname)
const char *optname;
{
#ifdef MICRO
	pline("\"%s\" settable only from %s.", optname, configfile);
#else
	pline("%s can be set only from NETHACKOPTIONS or %s.", optname,
			configfile);
#endif
}

STATIC_OVL void
badoption(opts)
const char *opts;
{
	if (!initial) {
	    if (!strncmp(opts, "h", 1) || !strncmp(opts, "?", 1))
		option_help();
	    else
		pline("Bad syntax: %s.  Enter \"?g\" for help.", opts);
	    return;
	}
#ifdef MAC
	else return;
#endif

	if(from_file)
	    raw_printf("Bad syntax in OPTIONS in %s: %s.", configfile, opts);
	else
	    raw_printf("Bad syntax in NETHACKOPTIONS: %s.", opts);

	wait_synch();
}

STATIC_OVL char *
string_for_opt(opts, val_optional)
char *opts;
boolean val_optional;
{
	char *colon, *equals;

	colon = index(opts, ':');
	equals = index(opts, '=');
	if (!colon || (equals && equals < colon)) colon = equals;

	if (!colon || !*++colon) {
		if (!val_optional) badoption(opts);
		return (char *)0;
	}
	return colon;
}

STATIC_OVL char *
string_for_env_opt(optname, opts, val_optional)
const char *optname;
char *opts;
boolean val_optional;
{
	if(!initial) {
		rejectoption(optname);
		return (char *)0;
	}
	return string_for_opt(opts, val_optional);
}

STATIC_OVL void
bad_negation(optname, with_parameter)
const char *optname;
boolean with_parameter;
{
	pline_The("%s option may not %sbe negated.",
		optname,
		with_parameter ? "both have a value and " : "");
}

/*
 * Change the inventory order, using the given string as the new order.
 * Missing characters in the new order are filled in at the end from
 * the current inv_order, except for gold, which is forced to be first
 * if not explicitly present.
 *
 * This routine returns 1 unless there is a duplicate or bad char in
 * the string.
 */
STATIC_OVL int
change_inv_order(op)
char *op;
{
    int oc_sym, num;
    char *sp, buf[BUFSZ];

    num = 0;
#ifndef GOLDOBJ
    if (!index(op, GOLD_SYM))
	buf[num++] = COIN_CLASS;
#else
    /*  !!!! probably unnecessary with gold as normal inventory */
#endif

    for (sp = op; *sp; sp++) {
	oc_sym = def_char_to_objclass(*sp);
	/* reject bad or duplicate entries */
	if (oc_sym == MAXOCLASSES ||
		oc_sym == RANDOM_CLASS || oc_sym == ILLOBJ_CLASS ||
		!index(flags.inv_order, oc_sym) || index(sp+1, *sp))
	    return 0;
	/* retain good ones */
	buf[num++] = (char) oc_sym;
    }
    buf[num] = '\0';

    /* fill in any omitted classes, using previous ordering */
    for (sp = flags.inv_order; *sp; sp++)
	if (!index(buf, *sp)) {
	    buf[num++] = *sp;
	    buf[num] = '\0';	/* explicitly terminate for next index() */
	}

    Strcpy(flags.inv_order, buf);
    return 1;
}

STATIC_OVL void
graphics_opts(opts, optype, maxlen, offset)
register char *opts;
const char *optype;
int maxlen, offset;
{
	glyph_t translate[MAXPCHARS+1];
	int length, i;

	if (!(opts = string_for_env_opt(optype, opts, FALSE)))
		return;
	escapes(opts, opts);

	length = strlen(opts);
	if (length > maxlen) length = maxlen;
	/* match the form obtained from PC configuration files */
	for (i = 0; i < length; i++)
		translate[i] = (glyph_t) opts[i];
	assign_graphics(translate, length, maxlen, offset);
}

STATIC_OVL void
warning_opts(opts, optype)
register char *opts;
const char *optype;
{
	uchar translate[MAXPCHARS+1];
	int length, i;

	if (!(opts = string_for_env_opt(optype, opts, FALSE)))
		return;
	escapes(opts, opts);

	length = strlen(opts);
	if (length > WARNCOUNT) length = WARNCOUNT;
	/* match the form obtained from PC configuration files */
	for (i = 0; i < length; i++)
	     translate[i] = (((i < WARNCOUNT) && opts[i]) ?
			   (uchar) opts[i] : def_warnsyms[i].sym);
	assign_warnings(translate);
}

void
assign_warnings(graph_chars)
register uchar *graph_chars;
{
	int i;
	for (i = 0; i < WARNCOUNT; i++)
	    if (graph_chars[i]) warnsyms[i] = graph_chars[i];
}

STATIC_OVL int
feature_alert_opts(op, optn)
char *op;
const char *optn;
{
	char buf[BUFSZ];
	boolean rejectver = FALSE;
	unsigned long fnv = get_feature_notice_ver(op);		/* version.c */
	if (fnv == 0L) return 0;
	if (fnv > get_current_feature_ver())
		rejectver = TRUE;
	else
		flags.suppress_alert = fnv;
	if (rejectver) {
		if (!initial)
			You_cant("disable new feature alerts for future versions.");
		else {
			Sprintf(buf,
				"\n%s=%s Invalid reference to a future version ignored",
				optn, op);
			badoption(buf);
		}
		return 0;
	}
	if (!initial) {
		Sprintf(buf, "%lu.%lu.%lu", FEATURE_NOTICE_VER_MAJ,
			FEATURE_NOTICE_VER_MIN, FEATURE_NOTICE_VER_PATCH);
		pline("Feature change alerts disabled for NetHack %s features and prior.",
			buf);
	}
	return 1;
}

#if defined(STATUS_COLORS) && defined(TEXTCOLOR)

struct name_value {
    char *name;
    int value;
};

const struct name_value status_colornames[] = {
    { "black",      CLR_BLACK },
    { "red",        CLR_RED },
    { "green",      CLR_GREEN },
    { "brown",      CLR_BROWN },
    { "blue",       CLR_BLUE },
    { "magenta",    CLR_MAGENTA },
    { "cyan",       CLR_CYAN },
    { "gray",       CLR_GRAY },
    { "grey",       CLR_GRAY },
    { "orange",     CLR_ORANGE },
    { "lightgreen", CLR_BRIGHT_GREEN },
    { "yellow",     CLR_YELLOW },
    { "lightblue",  CLR_BRIGHT_BLUE },
    { "lightmagenta", CLR_BRIGHT_MAGENTA },
    { "lightcyan",  CLR_BRIGHT_CYAN },
    { "white",      CLR_WHITE },
    { NULL,         -1 }
};

const struct name_value status_attrnames[] = {
    { "none",      ATR_NONE },
    { "bold",      ATR_BOLD },
    { "dim",       ATR_DIM },
    { "underline", ATR_ULINE },
    { "blink",     ATR_BLINK },
    { "inverse",   ATR_INVERSE },
    { NULL,        -1 }
};

int
value_of_name(name, name_values)
     const char *name;
     const struct name_value *name_values;
{
    while (name_values->name && !strstri(name_values->name, name))
	++name_values;
    return name_values->value;
}

struct color_option
parse_color_option(start)
     char *start;
{
    struct color_option result = {NO_COLOR, 0};
    char last;
    char *end;
    int attr;

    for (end = start; *end != '&' && *end != '\0'; ++end);
    last = *end;
    *end = '\0';
    result.color = value_of_name(start, status_colornames);

    while (last == '&') {
	for (start = ++end; *end != '&' && *end != '\0'; ++end);
	last = *end;
	*end = '\0';
	attr = value_of_name(start, status_attrnames);
	if (attr >= 0)
	    result.attr_bits |= 1 << attr;
    }

    return result;
}

struct percent_color_option *hp_colors = NULL;
struct percent_color_option *pw_colors = NULL;
struct text_color_option *text_colors = NULL;

struct percent_color_option *
add_percent_option(new_option, list_head)
     struct percent_color_option *new_option;
     struct percent_color_option *list_head;
{
    if (list_head == NULL)
	return new_option;
    if (new_option->percentage <= list_head->percentage) {
	new_option->next = list_head;
	return new_option;
    }
    list_head->next = add_percent_option(new_option, list_head->next);
    return list_head;
}

boolean
parse_status_color_option(start)
     char *start;
{
    char *middle;

    while (*start && isspace(*start)) start++;
    for (middle = start; *middle != ':' && *middle != '=' && *middle != '\0'; ++middle);
    *middle++ = '\0';
    if (middle - start > 2 && (start[2] == '%' || start[2] == '.' || start[2] == '<' || start[2] == '>')) {
                struct percent_color_option *percent_color_option =
		    (struct percent_color_option *)alloc(sizeof(*percent_color_option));
                percent_color_option->next = NULL;
                percent_color_option->percentage = atoi(start + 3);
		switch (start[2]) {
		default:
		case '%': percent_color_option->statclrtype = STATCLR_TYPE_PERCENT; break;
		case '.': percent_color_option->statclrtype = STATCLR_TYPE_NUMBER_EQ; break;
		case '>': percent_color_option->statclrtype = STATCLR_TYPE_NUMBER_GT; break;
		case '<': percent_color_option->statclrtype = STATCLR_TYPE_NUMBER_LT; break;
		}
                percent_color_option->color_option = parse_color_option(middle);
                start[2] = '\0';
                if (percent_color_option->color_option.color >= 0
		    && percent_color_option->color_option.attr_bits >= 0) {
		    if (!strcmpi(start, "hp")) {
			hp_colors = add_percent_option(percent_color_option, hp_colors);
			return TRUE;
		    }
		    if (!strcmpi(start, "pw")) {
			pw_colors = add_percent_option(percent_color_option, pw_colors);
			return TRUE;
		    }
                }
                free(percent_color_option);
                return FALSE;
    } else {
	int length = strlen(start) + 1;
                struct text_color_option *text_color_option =
		    (struct text_color_option *)alloc(sizeof(*text_color_option));
                text_color_option->next = NULL;
                text_color_option->text = (char *)alloc(length);
                memcpy((char *)text_color_option->text, start, length);
                text_color_option->color_option = parse_color_option(middle);
                if (text_color_option->color_option.color >= 0
		    && text_color_option->color_option.attr_bits >= 0) {
		    text_color_option->next = text_colors;
		    text_colors = text_color_option;
		    return TRUE;
                }
                free((genericptr_t)text_color_option->text);
                free(text_color_option);
                return FALSE;
    }
}

boolean
parse_status_color_options(start)
     char *start;
{
    char last = ',';
    char *end = start - 1;
    boolean ok = TRUE;
    while (last == ',') {
	for (start = ++end; *end != ',' && *end != '\0'; ++end);
	last = *end;
	*end = '\0';
	ok = parse_status_color_option(start) && ok;
    }
    return ok;
}

#endif /* STATUS_COLORS */


struct rgb_color_option *setcolor_opts = NULL;
struct rgb_color_option *resetcolor_opts = NULL;

struct rgb_color_option * add_rgb_color_option(new_option, list_head)
     struct rgb_color_option *new_option;
     struct rgb_color_option *list_head;
{
    if (list_head == NULL)
		return new_option;

	new_option->next = list_head;
    return new_option;
}

boolean
parse_rgbcolor(char* cptr, boolean reset)
{
	int clr;

	char hpre[BUFSZ] = "0x";
	long hexcode = -1;
	int r, g, b, l = 0;

	if 		(!strncmpi(cptr, "black",		l=5))	clr = CLR_BLACK;
	else if (!strncmpi(cptr, "red",			l=3))	clr = CLR_RED;
	else if (!strncmpi(cptr, "green",	 	l=5))	clr = CLR_GREEN;
	else if (!strncmpi(cptr, "brown",		l=5))	clr = CLR_BROWN;
	else if (!strncmpi(cptr, "blue", 		l=4))	clr = CLR_BLUE;
	else if (!strncmpi(cptr, "magenta",		l=7))	clr = CLR_MAGENTA;
	else if (!strncmpi(cptr, "cyan",		l=4))	clr = CLR_CYAN;
	else if (!strncmpi(cptr, "gray", 		l=4))	clr = CLR_GRAY;
	else if (!strncmpi(cptr, "orange",		l=6))	clr = CLR_ORANGE;
	else if (!strncmpi(cptr, "brightgreen",	l=11))	clr = CLR_BRIGHT_GREEN;
	else if (!strncmpi(cptr, "lightgreen",	l=10))	clr = CLR_BRIGHT_GREEN;
	else if (!strncmpi(cptr, "yellow",		l=6))	clr = CLR_YELLOW;
	else if (!strncmpi(cptr, "brightblue",	l=10))	clr = CLR_BRIGHT_BLUE;
	else if (!strncmpi(cptr, "lightblue",	l=9))	clr = CLR_BRIGHT_BLUE;
	else if (!strncmpi(cptr, "brightmagenta",l=13))	clr = CLR_BRIGHT_MAGENTA;
	else if (!strncmpi(cptr, "lightmagenta",l=12))	clr = CLR_BRIGHT_MAGENTA;
	else if (!strncmpi(cptr, "brightcyan",	l=10))	clr = CLR_BRIGHT_CYAN;
	else if (!strncmpi(cptr, "lightcyan",	l=9))	clr = CLR_BRIGHT_CYAN;
	else if (!strncmpi(cptr, "white",		l=5))	clr = CLR_WHITE;
	else return FALSE;

	cptr += l+1;

	if (cptr[0] == '#'){
		cptr++;
		strcat(hpre, cptr);
		hexcode = strtol(cptr, NULL, 16);
		r = (int) (hexcode & 0xff0000) >> 16;
		g = (int) (hexcode & 0x00ff00) >> 8;
		b = (int) (hexcode & 0x0000ff);
	} else {
		r = (int) strtol(cptr, &cptr, 10);
		g = (int) strtol(cptr, &cptr, 10);
		b = (int) strtol(cptr, NULL,  10);
	}

	r = r * 1000 / 255;
	g = g * 1000 / 255;
	b = b * 1000 / 255;

	struct rgb_color_option *rgb_opt = (struct rgb_color_option *)alloc(sizeof(struct rgb_color_option));
	rgb_opt->color = clr;
	rgb_opt->r = r;
	rgb_opt->g = g;
	rgb_opt->b = b;
	rgb_opt->next = NULL;

	if (reset) resetcolor_opts = add_rgb_color_option(rgb_opt, resetcolor_opts);
	else setcolor_opts = add_rgb_color_option(rgb_opt, setcolor_opts);

    return TRUE;
}


boolean
parse_setcolor(char* cptr){
	return parse_rgbcolor(cptr, FALSE);
}

boolean
parse_resetcolor(char* cptr){
	return parse_rgbcolor(cptr, TRUE);
}


void
set_duplicate_opt_detection(on_or_off)
int on_or_off;
{
	int k, *optptr;
	if (on_or_off != 0) {
		/*-- ON --*/
		if (iflags.opt_booldup)
			impossible("iflags.opt_booldup already on (memory leak)");
		iflags.opt_booldup = (int *)alloc(SIZE(boolopt) * sizeof(int));
		optptr = iflags.opt_booldup;
		for (k = 0; k < SIZE(boolopt); ++k)
			*optptr++ = 0;
			
		if (iflags.opt_compdup)
			impossible("iflags.opt_compdup already on (memory leak)");
		iflags.opt_compdup = (int *)alloc(SIZE(compopt) * sizeof(int));
		optptr = iflags.opt_compdup;
		for (k = 0; k < SIZE(compopt); ++k)
			*optptr++ = 0;
	} else {
		/*-- OFF --*/
		if (iflags.opt_booldup) free((genericptr_t) iflags.opt_booldup);
		iflags.opt_booldup = (int *)0;
		if (iflags.opt_compdup) free((genericptr_t) iflags.opt_compdup);
		iflags.opt_compdup = (int *)0;
	} 
}

STATIC_OVL void
duplicate_opt_detection(opts, bool_or_comp)
const char *opts;
int bool_or_comp;	/* 0 == boolean option, 1 == compound */
{
	int i, *optptr;
#if defined(MAC)
	/* the Mac has trouble dealing with the output of messages while
	 * processing the config file.  That should get fixed one day.
	 * For now just return.
	 */
	return;
#endif
	if ((bool_or_comp == 0) && iflags.opt_booldup && initial && from_file) {
	    for (i = 0; boolopt[i].name; i++) {
		if (match_optname(opts, boolopt[i].name, 3, FALSE)) {
			optptr = iflags.opt_booldup + i;
			if (*optptr == 1) {
			    raw_printf(
				"\nWarning - Boolean option specified multiple times: %s.\n",
					opts);
			        wait_synch();
			}
			*optptr += 1;
			break; /* don't match multiple options */
		}
	    }
	} else if ((bool_or_comp == 1) && iflags.opt_compdup && initial && from_file) {
	    for (i = 0; compopt[i].name; i++) {
		if (match_optname(opts, compopt[i].name, strlen(compopt[i].name), TRUE)) {
			optptr = iflags.opt_compdup + i;
			if (*optptr == 1) {
			    raw_printf(
				"\nWarning - compound option specified multiple times: %s.\n",
					compopt[i].name);
			        wait_synch();
			}
			*optptr += 1;
			break; /* don't match multiple options */
		}
	    }
	}
}

static const struct {
   const char *name;
   const int color;
} colornames[] = {
   {"black", CLR_BLACK},
   {"red", CLR_RED},
   {"green", CLR_GREEN},
   {"brown", CLR_BROWN},
   {"blue", CLR_BLUE},
   {"magenta", CLR_MAGENTA},
   {"cyan", CLR_CYAN},
   {"gray", CLR_GRAY},
   {"grey", CLR_GRAY},
   {"orange", CLR_ORANGE},
   {"lightgreen", CLR_BRIGHT_GREEN},
   {"yellow", CLR_YELLOW},
   {"lightblue", CLR_BRIGHT_BLUE},
   {"lightmagenta", CLR_BRIGHT_MAGENTA},
   {"lightcyan", CLR_BRIGHT_CYAN},
   {"white", CLR_WHITE}
};

#ifdef MENU_COLOR
extern struct menucoloring *menu_colorings;

static const struct {
   const char *name;
   const int attr;
} attrnames[] = {
     {"none", ATR_NONE},
     {"bold", ATR_BOLD},
     {"dim", ATR_DIM},
     {"underline", ATR_ULINE},
     {"blink", ATR_BLINK},
     {"inverse", ATR_INVERSE}

};

/* parse '"regex_string"=color&attr' and add it to menucoloring */
boolean
add_menu_coloring(str)
char *str;
{
    int i, c = NO_COLOR, a = ATR_NONE;
    struct menucoloring *tmp;
    char *tmps, *cs = strchr(str, '=');
#ifdef MENU_COLOR_REGEX_POSIX
    int errnum;
    char errbuf[80];
#endif
    const char *err = (char *)0;

    if (!cs || !str) return FALSE;

    tmps = cs;
    tmps++;
    while (*tmps && isspace(*tmps)) tmps++;

    for (i = 0; i < SIZE(colornames); i++)
	if (strstri(tmps, colornames[i].name) == tmps) {
	    c = colornames[i].color;
	    break;
	}
    if ((i == SIZE(colornames)) && (*tmps >= '0' && *tmps <='9'))
	c = atoi(tmps);

    if (c > 15) return FALSE;

    tmps = strchr(str, '&');
    if (tmps) {
	tmps++;
	while (*tmps && isspace(*tmps)) tmps++;
	for (i = 0; i < SIZE(attrnames); i++)
	    if (strstri(tmps, attrnames[i].name) == tmps) {
		a = attrnames[i].attr;
		break;
	    }
	if ((i == SIZE(attrnames)) && (*tmps >= '0' && *tmps <='9'))
	    a = atoi(tmps);
    }

    *cs = '\0';
    tmps = str;
    if ((*tmps == '"') || (*tmps == '\'')) {
	cs--;
	while (isspace(*cs)) cs--;
	if (*cs == *tmps) {
	    *cs = '\0';
	    tmps++;
	}
    }

    tmp = (struct menucoloring *)alloc(sizeof(struct menucoloring));
#ifdef MENU_COLOR_REGEX
#ifdef MENU_COLOR_REGEX_POSIX
    errnum = regcomp(&tmp->match, tmps, REG_EXTENDED | REG_NOSUB);
    if (errnum != 0)
    {
	regerror(errnum, &tmp->match, errbuf, sizeof(errbuf));
	err = errbuf;
    }
#else
    tmp->match.translate = 0;
    tmp->match.fastmap = 0;
    tmp->match.buffer = 0;
    tmp->match.allocated = 0;
    tmp->match.regs_allocated = REGS_FIXED;
    err = re_compile_pattern(tmps, strlen(tmps), &tmp->match);
#endif
#else
    tmp->match = (char *)alloc(strlen(tmps)+1);
    (void) memcpy((genericptr_t)tmp->match, (genericptr_t)tmps, strlen(tmps)+1);
#endif
    if (err) {
	raw_printf("\nMenucolor regex error: %s\n", err);
	wait_synch();
	free(tmp);
	return FALSE;
    } else {
	tmp->next = menu_colorings;
	tmp->color = c;
	tmp->attr = a;
	menu_colorings = tmp;
	return TRUE;
    }
}
#endif /* MENU_COLOR */

/* parse '"monster name":color' and change monster info accordingly */
boolean
parse_monster_color(str)
     char *str;
{
    int i, c = NO_COLOR;
    char *tmps, *cs = strchr(str, ':');
    char buf[BUFSZ];
    int monster;

    if (!str) return FALSE;

    strncpy(buf, str, BUFSZ);
    cs = strchr(buf, ':');
    if (!cs) return FALSE;

    tmps = cs;
    tmps++;
    /* skip whitespace at start of string */
    while (*tmps && isspace(*tmps)) tmps++;

    /* determine color */
    for (i = 0; i < SIZE(colornames); i++)
	if (strstri(tmps, colornames[i].name) == tmps) {
	    c = colornames[i].color;
	    break;
	}
    if ((i == SIZE(colornames)) && (*tmps >= '0' && *tmps <='9'))
	c = atoi(tmps);

    if (c > 15) return FALSE;

    /* determine monster name */
    *cs = '\0';
    tmps = buf;
    if ((*tmps == '"') || (*tmps == '\'')) {
	cs--;
	while (isspace(*cs)) cs--;
	if (*cs == *tmps) {
	    *cs = '\0';
	    tmps++;
	}
    }

    monster = name_to_mon(tmps);
    if (monster > -1) {
	mons[monster].mcolor = c;
	return TRUE;
    } else {
	return FALSE;
    }
}

/* parse template:target:value and set corresponding in iflags */
boolean
parse_monster_template(str)
char * str;
{
    int i, c = NO_COLOR, template;
	char s;
	int type = 0;
    char *s_temp, *s_type, *s_val;
    char buf[BUFSZ];

    if (!str) return FALSE;

    strncpy(buf, str, BUFSZ);
	s_temp = buf;
    s_type = strchr(s_temp, ':');
    if (!s_type) return FALSE;
	else s_type++;
	s_val = strchr(s_type, ':');
	if (!s_val) return FALSE;
	else s_val++;

    /* skip whitespace at start of strings */
    while (*s_temp && isspace(*s_temp)) s_temp++;
	while (*s_type && isspace(*s_type)) s_type++;
	while (*s_val  && isspace(*s_val )) s_val++;

	/* determine template */
	if (strstri(s_temp, "zombi") == s_temp)
		template = ZOMBIFIED;
	else if (strstri(s_temp, "skel") == s_temp)
		template = SKELIFIED;
	else if ((strstri(s_temp, "cryst") == s_temp) || strstri(s_temp, "vitri") == s_temp)
		template = CRYSTALFIED;
	else if ((strstri(s_temp, "fracture") == s_temp) || (strstri(s_temp, "witness") == s_temp))
		template = FRACTURED;
	else if (strstri(s_temp, "vampir") == s_temp)
		template = VAMPIRIC;
	else if ((strstri(s_temp, "illuminated") == s_temp) || (strstri(s_temp, "shining") == s_temp))
		template = ILLUMINATED;
	else if (strstri(s_temp, "pseudo") == s_temp)
		template = PSEUDONATURAL;
	else if (strstri(s_temp, "tomb") == s_temp)
		template = TOMB_HERD;
	else if (strstri(s_temp, "yith") == s_temp)
		template = YITH;
	else if (strstri(s_temp, "crani") == s_temp)
		template = CRANIUM_RAT;
	else if (strstri(s_temp, "psurlon") == s_temp)
		template = PSURLON;
	else if (strstri(s_temp, "constellation") == s_temp)
		template = CONSTELLATION;
	else if (strstri(s_temp, "mistweaver") == s_temp)
		template = MISTWEAVER;
	else if (strstri(s_temp, "deloused") == s_temp)
		template = DELOUSED;
	else if (strstri(s_temp, "black web") == s_temp)
		template = M_BLACK_WEB;
	else if (strstri(s_temp, "great web") == s_temp)
		template = M_GREAT_WEB;
	else if (strstri(s_temp, "slime") == s_temp)
		template = SLIME_REMNANT;
	else if (strstri(s_temp, "yellow") == s_temp)
		template = YELLOW_TEMPLATE;
	else if (strstri(s_temp, "dream") == s_temp)
		template = DREAM_LEECH;
	else if (strstri(s_temp, "mad") == s_temp)
		template = MAD_TEMPLATE;
	else if (strstri(s_temp, "fallen") == s_temp)
		template = FALLEN_TEMPLATE;
	else if (strstri(s_temp, "world") == s_temp)
		template = WORLD_SHAPER;
	else if (strstri(s_temp, "mindless") == s_temp)
		template = MINDLESS;
	else if (strstri(s_temp, "poison") == s_temp)
		template = POISON_TEMPLATE;
	else if (strstri(s_temp, "moly") == s_temp)
		template = MOLY_TEMPLATE;
	else if (strstri(s_temp, "cordyceps") == s_temp)
		template = CORDYCEPS;
	else if (strstri(s_temp, "spore") == s_temp)
		template = SPORE_ZOMBIE;
	else
		return FALSE;

	/* determine type */
	if ((strstri(s_type, "fg") == s_type) ||
		(strstri(s_type, "fore") == s_type))
		type = MONSTERTEMPLATE_FOREGROUND;
	else if ((strstri(s_type, "bg") == s_type) ||
			(strstri(s_type, "back") == s_type))
		type = MONSTERTEMPLATE_BACKGROUND;
	else if (strstri(s_type, "sym") == s_type)
		type = MONSTERTEMPLATE_SYMBOL;
	else
		return FALSE;
    /* determine color, if type is forground or background */
	if (type == MONSTERTEMPLATE_BACKGROUND || type == MONSTERTEMPLATE_FOREGROUND) {
		for (i = 0; i < SIZE(colornames); i++)
		if (strstri(s_val, colornames[i].name) == s_val) {
			c = colornames[i].color;
			break;
		}
		if ((i == SIZE(colornames)) && (*s_val >= '0' && *s_val <='9'))
			c = atoi(s_val);
		if (c > 15) return FALSE;
	}
	/* read symbol, if type is symbol */
	else {
		if (!(*s_val)) return FALSE;
		s = *s_val;
	}
	switch(type) {
		case MONSTERTEMPLATE_FOREGROUND:
			iflags.monstertemplate[template - 1].fg = c;
			break;
		case MONSTERTEMPLATE_BACKGROUND:
			iflags.monstertemplate[template - 1].bg = c;
			break;
		case MONSTERTEMPLATE_SYMBOL:
			iflags.monstertemplate[template - 1].symbol = s;
			break;
	}
	iflags.monstertemplate[template - 1].set |= type;

	return TRUE;
}

/** Split up a string that matches name:value or 'name':value and
 * return name and value separately. */
static boolean
parse_extended_option(str, option_name, option_value)
const char *str;
char *option_name;	/**< Output string buffer for option name */
char *option_value;	/**< Output string buffer for option value */
{
	int i;
	char *tmps, *cs;
	char buf[BUFSZ];

	if (!str) return FALSE;

	strncpy(buf, str, BUFSZ);

	/* remove comment*/
	cs = strrchr(buf, '#');
	if (cs) *cs = '\0';

	/* trim whitespace at end of string */
	i = strlen(buf)-1;
	while (i>=0 && isspace(buf[i])) {
		buf[i--] = '\0';
	}

	/* extract value */
	cs = strchr(buf, ':');
	if (!cs) return FALSE;

	tmps = cs;
	tmps++;
	/* skip whitespace at start of string */
	while (*tmps && isspace(*tmps)) tmps++;

	strncpy(option_value, tmps, BUFSZ);

	/* extract option name */
	*cs = '\0';
	tmps = buf;
	if ((*tmps == '"') || (*tmps == '\'')) {
		cs--;
		while (isspace(*cs)) cs--;
		if (*cs == *tmps) {
			*cs = '\0';
			tmps++;
		}
	}

	strncpy(option_name, tmps, BUFSZ);

	return TRUE;
}

/** Parse a string as Unicode codepoint and return the numerical codepoint.
 * Valid codepoints are decimal numbers or U+FFFF and 0xFFFF for hexadecimal
 * values. */
int
parse_codepoint(codepoint)
char *codepoint;
{
	char *ptr, *endptr;
	int num=0, base;

	/* parse codepoint */
	if (!strncmpi(codepoint, "u+", 2) ||
	    !strncmpi(codepoint, "0x", 2)) {
		/* hexadecimal */
		ptr = &codepoint[2];
		base = 16;
	} else {
		/* decimal */
		ptr = &codepoint[0];
		base = 10;
	}
	errno = 0;
	num = strtol(ptr, &endptr, base);
	if (errno != 0 || *endptr != 0 || endptr == ptr) {
		return FALSE;
	}
	return num;
}

/** Parse '"monster name":unicode_codepoint' and change symbol in
 * monster list. */
boolean
parse_monster_symbol(str)
const char *str;
{
	char monster[BUFSZ];
	char codepoint[BUFSZ];
	int i, num=0;

	if (!parse_extended_option(str, monster, codepoint)) {
		return FALSE;
	}

	num = parse_codepoint(codepoint);
	if (num < 0) {
		return FALSE;
	}

	/* find monster */
	for (i=0; mons[i].mlet != 0; i++) {
		if (!strcmpi(monster, mons[i].mname)) {
			permonst_unicode_codepoint[i] = num;
			return TRUE;
		}
	}

	return FALSE;
}

/** Parse '"object name":unicode_codepoint' and change symbol in
 * object list. */
boolean
parse_object_symbol(str)
const char *str;
{
       char object[BUFSZ];
       char codepoint[BUFSZ];
       int i, num=0;

       if (!parse_extended_option(str, object, codepoint)) {
               return FALSE;
       }

       num = parse_codepoint(codepoint);
       if (num < 0) {
               return FALSE;
       }

       /* find object */
       for (i=0; obj_descr[i].oc_name || obj_descr[i].oc_descr; i++) {
               if ((obj_descr[i].oc_name && obj_descr[i].oc_descr) ||
                   (obj_descr[i].oc_descr)) {
                       /* Items with both descriptive and actual name or only
                        * descriptive name. */
                       if (!strcmpi(object, obj_descr[i].oc_descr)) {
                               objclass_unicode_codepoint[i] = num;
                               return TRUE;
                       }
               } else if (obj_descr[i].oc_name) {
                       /* items with only actual name like "carrot" */
                       if (!strcmpi(object, obj_descr[i].oc_name)) {
                               objclass_unicode_codepoint[i] = num;
                               return TRUE;
                       }
               }
       }
       return FALSE;
}


/** Parse '"dungeon feature":unicode_codepoint' and change symbol in
 * UTF8graphics. */
boolean
parse_symbol(str)
const char *str;
{
	char feature[BUFSZ];
	char codepoint[BUFSZ];
	int i, num;

	if (!parse_extended_option(str, feature, codepoint)) {
		return FALSE;
	}

	num = parse_codepoint(codepoint);
	if (num < 0) {
		return FALSE;
	}

	/* find dungeon feature */
	for (i=0; i < MAXPCHARS; i++) {
		if (!strcmpi(feature, symbol_names[i]) || !strcmpi(feature, defsyms[i].explanation)) {
			assign_utf8graphics_symbol(i, num);
			return TRUE;
		}
	}

	return FALSE;
}


void
parseoptions(opts, tinitial, tfrom_file)
register char *opts;
boolean tinitial, tfrom_file;
{
	register char *op;
	unsigned num;
	boolean negated;
	int i;
	const char *fullname;

	initial = tinitial;
	from_file = tfrom_file;
	if ((op = index(opts, ',')) != 0) {
		*op++ = 0;
		parseoptions(op, initial, from_file);
	}
	if (strlen(opts) > BUFSZ/2) {
		badoption("option too long");
		return;
	}

	/* strip leading and trailing white space */
	opts = stripspace(opts);

	if (!*opts) return;
	negated = FALSE;
	while ((*opts == '!') || !strncmpi(opts, "no", 2)) {
		if (*opts == '!') opts++; else opts += 2;
		negated = !negated;
	}

	/* variant spelling */

	if (match_optname(opts, "colour", 5, FALSE))
		Strcpy(opts, "color");	/* fortunately this isn't longer */

	if (!match_optname(opts, "subkeyvalue", 11, TRUE)) /* allow multiple */
	duplicate_opt_detection(opts, 1);	/* 1 means compound opts */

	/* special boolean options */

	if (match_optname(opts, "female", 3, FALSE)) {
		if(!initial && flags.female == negated)
			pline("That is not anatomically possible.");
		else
			flags.initgend = flags.female = !negated;
		return;
	}

	if (match_optname(opts, "male", 4, FALSE)) {
		if(!initial && flags.female != negated)
			pline("That is not anatomically possible.");
		else
			flags.initgend = flags.female = negated;
		return;
	}

#if defined(MICRO) && !defined(AMIGA)
	/* included for compatibility with old NetHack.cnf files */
	if (match_optname(opts, "IBM_", 4, FALSE)) {
		iflags.BIOS = !negated;
		return;
	}
#endif /* MICRO */

	/* compound options */

	fullname = "attack_mode";
	/* attack_mode:pacifist, chat, ask, or fight */
	if (match_optname(opts, fullname, 11, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
		} else if ((op = string_for_opt(opts, FALSE))) {
			int tmp = tolower(*op);
			switch (tmp) {
				case ATTACK_MODE_PACIFIST:
				case ATTACK_MODE_CHAT:
				case ATTACK_MODE_ASK:
				case ATTACK_MODE_FIGHT_ALL:
					iflags.attack_mode = tmp;
				break;
				default:
					badoption(opts);
					return;
			}
		}
		return;
	}

	fullname = "pokedex";
	if (match_optname(opts, fullname, 3, TRUE)) {
		int l = 0;
		boolean negative = FALSE;
		if (negated) {
			iflags.pokedex = POKEDEX_SHOW_DEFAULT;
		}
		else for (op = string_for_opt(opts, FALSE); op && *op; op += l) {
			while (*op == ' ' || *op == ',')
				op++;
			if (op[0] == '!') {
				negative = TRUE;
				op++;
			}
			else {
				negative = FALSE;
			}
#define ADD_REMOVE_SECTION(section)	if (!negative) iflags.pokedex |= (section); else iflags.pokedex &= ~(section)
			if      (!strncmpi(op, "stats",      l= 5))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_STATS);
			else if (!strncmpi(op, "generation", l=10))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_GENERATION);
			else if (!strncmpi(op, "weight",     l= 6))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_WEIGHT);
			else if (!strncmpi(op, "resists",    l= 7))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_RESISTS);
			else if (!strncmpi(op, "conveys",    l= 7))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_CONVEYS);
			else if (!strncmpi(op, "movement",   l= 8))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MM);
			else if (!strncmpi(op, "thinking",   l= 8))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MT);
			else if (!strncmpi(op, "biology",    l= 7))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MB);
			else if (!strncmpi(op, "mechanics",  l= 9))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MG);
			else if (!strncmpi(op, "race",       l= 4))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MA);
			else if (!strncmpi(op, "vision",     l= 6))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_MV);
			else if (!strncmpi(op, "attacks",    l= 7))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_ATTACKS);
			else if (!strncmpi(op, "summary",    l= 7))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_CRITICAL);
			else if (!strncmpi(op, "wards",    l= 5))
				ADD_REMOVE_SECTION(POKEDEX_SHOW_WARDS);
			else
				badoption(opts);
#undef ADD_REMOVE_SECTION
		}
		return;
	}
		

	fullname = "pettype";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if ((op = string_for_env_opt(fullname, opts, negated)) != 0) {
		    if (negated) bad_negation(fullname, TRUE);
		    else switch (*op) {
			case 'd':	/* dog */
			case 'D':
			    preferred_pet = 'd';
			    break;
			case 'c':	/* cat */
			case 'C':
			case 'f':	/* feline */
			case 'F':
			    preferred_pet = 'c';
			    break;
			case 'n':	/* no pet */
			case 'N':
			    preferred_pet = 'n';
			    break;
			default:
			    pline("Unrecognized pet type '%s'.", op);
			    break;
		    }
		} else if (negated) preferred_pet = 'n';
		return;
	}

	fullname = "catname";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(catname, op, PL_PSIZ);
		sanitizestr(catname);
		return;
	}

	fullname = "dogname";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(dogname, op, PL_PSIZ);
		sanitizestr(dogname);
		return;
	}

#ifdef DUMP_LOG
	fullname = "dumpfile";
	if (match_optname(opts, fullname, 3, TRUE)) {
#ifndef DUMP_FN
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_opt(opts, !tfrom_file)) != 0
			&& strlen(op) > 1)
			nmcpy(dump_fn, op, PL_PSIZ);
#endif
		return;
       }
#endif

	fullname = "horsename";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(horsename, op, PL_PSIZ);
		sanitizestr(horsename);
		return;
	}

#ifdef CONVICT
	fullname = "ratname";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(ratname, op, PL_PSIZ);
		sanitizestr(ratname);
		return;
	}
#endif /* CONVICT */

	fullname = "number_pad";
	if (match_optname(opts, fullname, 10, TRUE)) {
		boolean compat = (strlen(opts) <= 10);
		number_pad(iflags.num_pad ? 1 : 0);
		op = string_for_opt(opts, (compat || !initial));
		if (!op) {
		    if (compat || negated || initial) {
			/* for backwards compatibility, "number_pad" without a
			   value is a synonym for number_pad:1 */
			iflags.num_pad = !negated;
			if (iflags.num_pad) iflags.num_pad_mode = 0;
		    }
		    return;
		}
		if (negated) {
		    bad_negation("number_pad", TRUE);
		    return;
		}
		if (*op == '1' || *op == '2') {
			iflags.num_pad = 1;
			if (*op == '2') iflags.num_pad_mode = 1;
			else iflags.num_pad_mode = 0;
		} else if (*op == '0') {
			iflags.num_pad = 0;
			iflags.num_pad_mode = 0;
		} else badoption(opts);
		return;
	}

#ifdef QWERTZ
        fullname = "qwertz_movement";
        if (match_optname(opts, fullname, 6, FALSE)) {
                if (negated)
                        sdir=qykbd_dir;
                else
                        sdir=qzkbd_dir;
                iflags.qwertz_movement=!negated;
                return;
        }
#endif

	fullname = "runmode";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) {
			iflags.runmode = RUN_TPORT;
		} else if ((op = string_for_opt(opts, FALSE)) != 0) {
		    if (!strncmpi(op, "teleport", strlen(op)))
			iflags.runmode = RUN_TPORT;
		    else if (!strncmpi(op, "run", strlen(op)))
			iflags.runmode = RUN_LEAP;
		    else if (!strncmpi(op, "walk", strlen(op)))
			iflags.runmode = RUN_STEP;
		    else if (!strncmpi(op, "crawl", strlen(op)))
			iflags.runmode = RUN_CRAWL;
		    else
			badoption(opts);
		}
		return;
	}
	fullname = "delay_length";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
		    if (!strncmpi(op, "none", strlen(op)))
			iflags.delay_length = RUN_TPORT;
		    else if (!strncmpi(op, "short", strlen(op)))
			iflags.delay_length = RUN_LEAP;
		    else if (!strncmpi(op, "normal", strlen(op)))
			iflags.delay_length = RUN_STEP;
		    else
			badoption(opts);
		}
		return;
	}

	/* menucolor:"regex_string"=color */
	fullname = "menucolor";
	if (match_optname(opts, fullname, 9, TRUE)) {
#ifdef MENU_COLOR
	    if (negated) bad_negation(fullname, FALSE);
	    else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
		if (!add_menu_coloring(op))
		    badoption(opts);
#endif
	    return;
	}

	fullname = "hp_notify_fmt";
	if (match_optname(opts, fullname, sizeof("hp_notify_fmt")-1, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
			if (iflags.hp_notify_fmt) free(iflags.hp_notify_fmt);
			iflags.hp_notify_fmt = (char *)alloc(strlen(op) + 1);
			Strcpy(iflags.hp_notify_fmt, op);
		}
		return;
	}

	fullname = "msghistory";
	if (match_optname(opts, fullname, 3, TRUE)) {
		op = string_for_env_opt(fullname, opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.msg_history = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	fullname="msg_window";
	/* msg_window:single, combo, full or reversed */
	if (match_optname(opts, fullname, 4, TRUE)) {
	/* allow option to be silently ignored by non-tty ports */
#ifdef TTY_GRAPHICS
		int tmp;
		if (!(op = string_for_opt(opts, TRUE))) {
		    tmp = negated ? 's' : 'f';
		} else {
			  if (negated) {
			  	bad_negation(fullname, TRUE);
			  	return;
				  }
		    tmp = tolower(*op);
		}
		switch (tmp) {
			case 's':	/* single message history cycle (default if negated) */
				iflags.prevmsg_window = 's';
				break;
			case 'c':	/* combination: two singles, then full page reversed */
				iflags.prevmsg_window = 'c';
				break;
			case 'f':	/* full page (default if no opts) */
				iflags.prevmsg_window = 'f';
				break;
			case 'r':	/* full page (reversed) */
				iflags.prevmsg_window = 'r';
				break;
			default:
				badoption(opts);
		}
#endif
		return;
	}

	/* WINCAP
	 * setting font options  */
	fullname = "font";
	if (!strncmpi(opts, fullname, 4))
	{
		int wintype = -1;
		char *fontopts = opts + 4;

		if (!strncmpi(fontopts, "map", 3) ||
		    !strncmpi(fontopts, "_map", 4))
			wintype = NHW_MAP;
		else if (!strncmpi(fontopts, "message", 7) ||
			 !strncmpi(fontopts, "_message", 8))
			wintype = NHW_MESSAGE;
		else if (!strncmpi(fontopts, "text", 4) ||
			 !strncmpi(fontopts, "_text", 5))
			wintype = NHW_TEXT;			
		else if (!strncmpi(fontopts, "menu", 4) ||
			 !strncmpi(fontopts, "_menu", 5))
			wintype = NHW_MENU;
		else if (!strncmpi(fontopts, "status", 6) ||
			 !strncmpi(fontopts, "_status", 7))
			wintype = NHW_STATUS;
		else if (!strncmpi(fontopts, "_size", 5)) {
			if (!strncmpi(fontopts, "_size_map", 8))
				wintype = NHW_MAP;
			else if (!strncmpi(fontopts, "_size_message", 12))
				wintype = NHW_MESSAGE;
			else if (!strncmpi(fontopts, "_size_text", 9))
				wintype = NHW_TEXT;
			else if (!strncmpi(fontopts, "_size_menu", 9))
				wintype = NHW_MENU;
			else if (!strncmpi(fontopts, "_size_status", 11))
				wintype = NHW_STATUS;
			else {
				badoption(opts);
				return;
			}
			if (wintype > 0 && !negated &&
			    (op = string_for_opt(opts, FALSE)) != 0) {
			    switch(wintype)  {
			    	case NHW_MAP:
					iflags.wc_fontsiz_map = atoi(op);
					break;
			    	case NHW_MESSAGE:
					iflags.wc_fontsiz_message = atoi(op);
					break;
			    	case NHW_TEXT:
					iflags.wc_fontsiz_text = atoi(op);
					break;
			    	case NHW_MENU:
					iflags.wc_fontsiz_menu = atoi(op);
					break;
			    	case NHW_STATUS:
					iflags.wc_fontsiz_status = atoi(op);
					break;
			    }
			}
			return;
		} else {
			badoption(opts);
		}
		if (wintype > 0 &&
		    (op = string_for_opt(opts, FALSE)) != 0) {
			wc_set_font_name(wintype, op);
#ifdef MAC
			set_font_name (wintype, op);
#endif
			return;
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
#ifdef CHANGE_COLOR
	if (match_optname(opts, "palette", 3, TRUE)
# ifdef MAC
	    || match_optname(opts, "hicolor", 3, TRUE)
# endif
							) {
	    int color_number, color_incr;

# ifdef MAC
	    if (match_optname(opts, "hicolor", 3, TRUE)) {
		if (negated) {
		    bad_negation("hicolor", FALSE);
		    return;
		}
		color_number = CLR_MAX + 4;	/* HARDCODED inverse number */
		color_incr = -1;
	    } else {
# endif
		if (negated) {
		    bad_negation("palette", FALSE);
		    return;
		}
		color_number = 0;
		color_incr = 1;
# ifdef MAC
	    }
# endif
	    if ((op = string_for_opt(opts, FALSE)) != (char *)0) {
		char *pt = op;
		int cnt, tmp, reverse;
		long rgb;

		while (*pt && color_number >= 0) {
		    cnt = 3;
		    rgb = 0L;
		    if (*pt == '-') {
			reverse = 1;
			pt++;
		    } else {
			reverse = 0;
		    }
		    while (cnt-- > 0) {
			if (*pt && *pt != '/') {
# ifdef AMIGA
			    rgb <<= 4;
# else
			    rgb <<= 8;
# endif
			    tmp = *(pt++);
			    if (isalpha(tmp)) {
				tmp = (tmp + 9) & 0xf;	/* Assumes ASCII... */
			    } else {
				tmp &= 0xf;	/* Digits in ASCII too... */
			    }
# ifndef AMIGA
			    /* Add an extra so we fill f -> ff and 0 -> 00 */
			    rgb += tmp << 4;
# endif
			    rgb += tmp;
			}
		    }
		    if (*pt == '/') {
			pt++;
		    }
		    change_color(color_number, rgb, reverse);
		    color_number += color_incr;
		}
	    }
	    if (!initial) {
		need_redraw = TRUE;
	    }
	    return;
	}
#endif /* CHANGE_COLOR */

	if (match_optname(opts, "fruit", 2, TRUE)) {
		char empty_str = '\0';
		op = string_for_opt(opts, negated);
		if (negated) {
		    if (op) {
			bad_negation("fruit", TRUE);
			return;
		    }
		    op = &empty_str;
		    goto goodfruit;
		}
		if (!op) return;
		if (!initial) {
		    struct fruit *f;

		    num = 0;
		    for(f=ffruit; f; f=f->nextf) {
			if (!strcmp(op, f->fname)) goto goodfruit;
			num++;
		    }
		    if (num >= 100) {
			pline("Doing that so many times isn't very fruitful.");
			return;
		    }
		}
goodfruit:
		nmcpy(pl_fruit, op, PL_FSIZ);
		sanitizestr(pl_fruit);
	/* OBJ_NAME(objects[SLIME_MOLD]) won't work after initialization */
		if (!*pl_fruit)
		    nmcpy(pl_fruit, "slime mold", PL_FSIZ);
		if (!initial)
		    (void)fruitadd(pl_fruit);
		/* If initial, then initoptions is allowed to do it instead
		 * of here (initoptions always has to do it even if there's
		 * no fruit option at all.  Also, we don't want people
		 * setting multiple fruits in their options.)
		 */
		return;
	}

	/* graphics:string */
	fullname = "graphics";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXPCHARS, 0);
		return;
	}
	fullname = "dungeon";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXDCHARS, 0);
		return;
	}
	fullname = "traps";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else graphics_opts(opts, fullname, MAXTCHARS, MAXDCHARS);
		return;
	}
	fullname = "effects";
	if (match_optname(opts, fullname, 2, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else
		 graphics_opts(opts, fullname, MAXECHARS, MAXDCHARS+MAXTCHARS);
		return;
	}

	/* objects:string */
	fullname = "objects";
	if (match_optname(opts, fullname, 7, TRUE)) {
		int length;

		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
		if (!(opts = string_for_env_opt(fullname, opts, FALSE)))
			return;
		escapes(opts, opts);

		/*
		 * Override the default object class symbols.  The first
		 * object in the object class is the "random object".  I
		 * don't want to use 0 as an object class, so the "random
		 * object" is basically a place holder.
		 *
		 * The object class symbols have already been initialized in
		 * initoptions().
		 */
		length = strlen(opts);
		if (length >= MAXOCLASSES)
		    length = MAXOCLASSES-1;	/* don't count RANDOM_OBJECT */

		for (i = 0; i < length; i++)
		    oc_syms[i+1] = (uchar) opts[i];
		return;
	}

	/* monsters:string */
	fullname = "monsters";
	if (match_optname(opts, fullname, 8, TRUE)) {
		int length;

		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
		if (!(opts = string_for_env_opt(fullname, opts, FALSE)))
			return;
		escapes(opts, opts);

		/* Override default mon class symbols set in initoptions(). */
		length = strlen(opts);
		if (length >= MAXMCLASSES)
		    length = MAXMCLASSES-1;	/* mon class 0 unused */

		for (i = 0; i < length; i++)
		    monsyms[i+1] = (uchar) opts[i];
		return;
	}
	fullname = "warnings";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else warning_opts(opts, fullname);
		return;
	}
	/* boulder:symbol */
	fullname = "boulder";
	if (match_optname(opts, fullname, 7, TRUE)) {
		int clash = 0;
		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		}
/*		if (!(opts = string_for_env_opt(fullname, opts, FALSE))) */
		if (!(opts = string_for_opt(opts, FALSE)))
			return;
		escapes(opts, opts);
		if (def_char_to_monclass(opts[0]) != MAXMCLASSES)
			clash = 1;
		else if (opts[0] >= '1' && opts[0] <= '5')
			clash = 2;
		if (clash) {
			/* symbol chosen matches a used monster or warning
			   symbol which is not good - reject it*/
			pline(
		  "Badoption - boulder symbol '%c' conflicts with a %s symbol.",
				opts[0], (clash == 1) ? "monster" : "warning");
		} else {
			/*
			 * Override the default boulder symbol.
			 */
			iflags.bouldersym = (uchar) opts[0];
		}
		if (!initial) need_redraw = TRUE;
		return;
	}

	/* name:string */
	fullname = "name";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(plname, op, PL_NSIZ);
		return;
	}

	/* role:string or character:string */
	fullname = "role";
	if (match_optname(opts, fullname, 4, TRUE) ||
	    match_optname(opts, (fullname = "character"), 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initrole = str2role(op)) == ROLE_NONE)
				badoption(opts);
			else  /* Backwards compatibility */
				nmcpy(pl_character, op, PL_NSIZ);
		}
		return;
	}

	/* race:string */
	fullname = "race";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initrace = str2race(op)) == ROLE_NONE)
				badoption(opts);
			else /* Backwards compatibility */
				pl_race = *op;
		}
		return;
	}

	/* gender:string */
	fullname = "gender";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
			if ((flags.initgend = str2gend(op)) == ROLE_NONE)
				badoption(opts);
			else
				flags.female = flags.initgend;
		}
		return;
	}

	if (match_optname(opts, "descendant", 4, FALSE)) {
		if (negated) flags.descendant = 0;
		else flags.descendant = 1;
		return;
	}

	fullname = "inherited";
	if (match_optname(opts, fullname, 3, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			nmcpy(inherited, op, PL_PSIZ);
		sanitizestr(inherited);
		return;
	}

	/* altkeyhandler:string */
	fullname = "altkeyhandler";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_opt(opts, negated))) {
#ifdef WIN32CON
		    (void)strncpy(iflags.altkeyhandler, op, MAX_ALTKEYHANDLER - 5);
		    load_keyboard_handler();
#endif
		}
		return;
	}

	/* WINCAP
	 * align_status:[left|top|right|bottom] */
	fullname = "align_status";
	if (match_optname(opts, fullname, sizeof("align_status")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "left", sizeof("left")-1))
			iflags.wc_align_status = ALIGN_LEFT;
		    else if (!strncmpi (op, "top", sizeof("top")-1))
			iflags.wc_align_status = ALIGN_TOP;
		    else if (!strncmpi (op, "right", sizeof("right")-1))
			iflags.wc_align_status = ALIGN_RIGHT;
		    else if (!strncmpi (op, "bottom", sizeof("bottom")-1))
			iflags.wc_align_status = ALIGN_BOTTOM;
		    else
			badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * align_message:[left|top|right|bottom] */
	fullname = "align_message";
	if (match_optname(opts, fullname, sizeof("align_message")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "left", sizeof("left")-1))
			iflags.wc_align_message = ALIGN_LEFT;
		    else if (!strncmpi (op, "top", sizeof("top")-1))
			iflags.wc_align_message = ALIGN_TOP;
		    else if (!strncmpi (op, "right", sizeof("right")-1))
			iflags.wc_align_message = ALIGN_RIGHT;
		    else if (!strncmpi (op, "bottom", sizeof("bottom")-1))
			iflags.wc_align_message = ALIGN_BOTTOM;
		    else
			badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* align:string */
	fullname = "align";
	if (match_optname(opts, fullname, sizeof("align")-1, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			if ((flags.initalign = str2align(op)) == ROLE_NONE)
				badoption(opts);
		return;
	}

	fullname = "chaos_quest";
	if (match_optname(opts, fullname, sizeof("chaos_quest")-1, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0){
			//flags.chaosvar must be nonzero
			if(!strcmp(op, "mithardir"))
				flags.chaosvar = MITHARDIR+1;
			else if(!strcmp(op, "temple"))
				flags.chaosvar = TEMPLE_OF_CHAOS+1;
			else if(!strcmp(op, "mordor"))
				flags.chaosvar = MORDOR+1;
			else if(!strcmp(op, "random"))
				flags.chaosvar = 0;
			else
				badoption(opts);
		}
		return;
	}

	/* the order to list the pack */
	fullname = "packorder";
	if (match_optname(opts, fullname, 4, TRUE)) {
		if (negated) {
		    bad_negation(fullname, FALSE);
		    return;
		} else if (!(op = string_for_opt(opts, FALSE))) return;

		if (!change_inv_order(op))
			badoption(opts);
		return;
	}

	/* maximum burden picked up before prompt (Warren Cheung) */
	fullname = "pickup_burden";
	if (match_optname(opts, fullname, 8, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		} else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
		    switch (tolower(*op)) {
				/* Unencumbered */
				case 'u':
					flags.pickup_burden = UNENCUMBERED;
					break;
				/* Burdened (slight encumbrance) */
				case 'b':
					flags.pickup_burden = SLT_ENCUMBER;
					break;
				/* streSsed (moderate encumbrance) */
				case 's':
					flags.pickup_burden = MOD_ENCUMBER;
					break;
				/* straiNed (heavy encumbrance) */
				case 'n':
					flags.pickup_burden = HVY_ENCUMBER;
					break;
				/* OverTaxed (extreme encumbrance) */
				case 'o':
				case 't':
					flags.pickup_burden = EXT_ENCUMBER;
					break;
				/* overLoaded */
				case 'l':
					flags.pickup_burden = OVERLOADED;
					break;
				default:
				badoption(opts);
		    }
		}
		return;
	}

	/* types of objects to pick up automatically */
	if (match_optname(opts, "pickup_types", 8, TRUE)) {
		char ocl[MAXOCLASSES + 1], tbuf[MAXOCLASSES + 1],
		     qbuf[QBUFSZ], abuf[BUFSZ];
		int oc_sym;
		boolean badopt = FALSE, compat = (strlen(opts) <= 6), use_menu;

		oc_to_str(flags.pickup_types, tbuf);
		flags.pickup_types[0] = '\0';	/* all */
		op = string_for_opt(opts, (compat || !initial));
		if (!op) {
		    if (compat || negated || initial) {
			/* for backwards compatibility, "pickup" without a
			   value is a synonym for autopickup of all types
			   (and during initialization, we can't prompt yet) */
			flags.pickup = !negated;
			return;
		    }
		    oc_to_str(flags.inv_order, ocl);
		    use_menu = TRUE;
		    if (flags.menu_style == MENU_TRADITIONAL ||
			    flags.menu_style == MENU_COMBINATION) {
			use_menu = FALSE;
			Sprintf(qbuf, "New pickup_types: [%s am] (%s)",
				ocl, *tbuf ? tbuf : "all");
			getlin(qbuf, abuf);
			op = mungspaces(abuf);
			if (abuf[0] == '\0' || abuf[0] == '\033')
			    op = tbuf;		/* restore */
			else if (abuf[0] == 'm')
			    use_menu = TRUE;
		    }
		    if (use_menu) {
			(void) choose_classes_menu("Auto-Pickup what?", 1,
						   TRUE, ocl, tbuf);
			op = tbuf;
		    }
		}
		if (negated) {
		    bad_negation("pickup_types", TRUE);
		    return;
		}
		while (*op == ' ') op++;
		if (*op != 'a' && *op != 'A') {
		    num = 0;
		    while (*op) {
			oc_sym = def_char_to_objclass(*op);
			/* make sure all are valid obj symbols occuring once */
			if (oc_sym != MAXOCLASSES &&
			    !index(flags.pickup_types, oc_sym)) {
			    flags.pickup_types[num] = (char)oc_sym;
			    flags.pickup_types[++num] = '\0';
			} else
			    badopt = TRUE;
			op++;
		    }
		    if (badopt) badoption(opts);
		}
		return;
	}
	/* WINCAP
	 * player_selection: dialog | prompts */
	fullname = "player_selection";
	if (match_optname(opts, fullname, sizeof("player_selection")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "dialog", sizeof("dialog")-1))
			iflags.wc_player_selection = VIA_DIALOG;
		    else if (!strncmpi (op, "prompt", sizeof("prompt")-1))
			iflags.wc_player_selection = VIA_PROMPTS;
		    else
		    	badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	/* things to disclose at end of game */
	if (match_optname(opts, "disclose", 7, TRUE)) {
		/*
		 * The order that the end_disclore options are stored:
		 * inventory, attribs, vanquished, genocided, conduct
		 * There is an array in flags:
		 *	end_disclose[NUM_DISCLOSURE_OPT];
		 * with option settings for the each of the following:
		 * iagvc [see disclosure_options in decl.c]:
		 * Legal setting values in that array are:
		 *	DISCLOSE_PROMPT_DEFAULT_YES  ask with default answer yes
		 *	DISCLOSE_PROMPT_DEFAULT_NO   ask with default answer no
		 *	DISCLOSE_YES_WITHOUT_PROMPT  always disclose and don't ask
		 *	DISCLOSE_NO_WITHOUT_PROMPT   never disclose and don't ask
		 *
		 * Those setting values can be used in the option
		 * string as a prefix to get the desired behaviour.
		 *
		 * For backward compatibility, no prefix is required,
		 * and the presence of a i,a,g,v, or c without a prefix
		 * sets the corresponding value to DISCLOSE_YES_WITHOUT_PROMPT.
		 */
		boolean badopt = FALSE;
		int idx, prefix_val;

		op = string_for_opt(opts, TRUE);
		if (op && negated) {
			bad_negation("disclose", TRUE);
			return;
		}
		/* "disclose" without a value means "all with prompting"
		   and negated means "none without prompting" */
		if (!op || !strcmpi(op, "all") || !strcmpi(op, "none")) {
			if (op && !strcmpi(op, "none")) negated = TRUE;
			for (num = 0; num < NUM_DISCLOSURE_OPTIONS; num++)
			    flags.end_disclose[num] = negated ?
						DISCLOSE_NO_WITHOUT_PROMPT :
						DISCLOSE_PROMPT_DEFAULT_YES;
			return;
		}

		num = 0;
		prefix_val = -1;
		while (*op && num < sizeof flags.end_disclose - 1) {
			register char c, *dop;
			static char valid_settings[] = {
				DISCLOSE_PROMPT_DEFAULT_YES,
				DISCLOSE_PROMPT_DEFAULT_NO,
				DISCLOSE_YES_WITHOUT_PROMPT,
				DISCLOSE_NO_WITHOUT_PROMPT,
				'\0'
			};
			c = lowc(*op);
			if (c == 'k') c = 'v';	/* killed -> vanquished */
			dop = index(disclosure_options, c);
			if (dop) {
				idx = dop - disclosure_options;
				if (idx < 0 || idx > NUM_DISCLOSURE_OPTIONS - 1) {
				    impossible("bad disclosure index %d %c",
							idx, c);
				    continue;
				}
				if (prefix_val != -1) {
				    flags.end_disclose[idx] = prefix_val;
				    prefix_val = -1;
				} else
				    flags.end_disclose[idx] = DISCLOSE_YES_WITHOUT_PROMPT;
			} else if (index(valid_settings, c)) {
				prefix_val = c;
			} else if (c == ' ') {
				/* do nothing */
			} else
				badopt = TRUE;				
			op++;
		}
		if (badopt) badoption(opts);
		return;
	}

	/* scores:5t[op] 5a[round] o[wn] */
	if (match_optname(opts, "scores", 4, TRUE)) {
	    if (negated) {
		bad_negation("scores", FALSE);
		return;
	    }
	    if (!(op = string_for_opt(opts, FALSE))) return;

	    while (*op) {
		int inum = 1;

		if (digit(*op)) {
		    inum = atoi(op);
		    while (digit(*op)) op++;
		} else if (*op == '!') {
		    negated = !negated;
		    op++;
		}
		while (*op == ' ') op++;

		switch (*op) {
		 case 't':
		 case 'T':  flags.end_top = inum;
			    break;
		 case 'a':
		 case 'A':  flags.end_around = inum;
			    break;
		 case 'o':
		 case 'O':  flags.end_own = !negated;
			    break;
		 default:   badoption(opts);
			    return;
		}
		while (letter(*++op) || *op == ' ') continue;
		if (*op == '/') op++;
	    }
	    return;
	}
	
	fullname = "species";
	if (match_optname(opts, fullname, sizeof("species")-1, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
			if ((flags.initspecies = str2species(op)) == ROLE_NONE)
				badoption(opts);
		return;
	}

        fullname = "statuscolor";
        if (match_optname(opts, fullname, 11, TRUE)) {
#if defined(STATUS_COLORS) && defined(TEXTCOLOR)
	    if (negated) bad_negation(fullname, FALSE);
	    else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0)
		if (!parse_status_color_options(op))
		    badoption(opts);
#endif
            return;
        }


	fullname = "statuseffects";
	if (match_optname(opts, fullname, sizeof("statuseffects")-1, TRUE)) {
		int l = 0;
		boolean negative = FALSE;
		boolean error = FALSE;
		if (negated) {
			iflags.statuseffects = 0;
		}
		else for (op = string_for_opt(opts, FALSE); op && *op; op += l) {
			while (*op == ' ' || *op == ',')
				op++;
			if (op[0] == '!') {
				negative = TRUE;
				op++;
			}
			else {
				negative = FALSE;
			}
			if (!strncmpi(op, "all", l=3)) {
				iflags.statuseffects = negative ? 0 : ~0;
				continue;
			}
			boolean exists = FALSE;
			for (i = 0; i < SIZE(status_effects); i++) {
				struct status_effect status = status_effects[i];
				if (!strncmpi(op, status.name, l=strlen(status.name))) {
					exists = TRUE;
					if (!negative)
						iflags.statuseffects |= status.mask;
					else
						iflags.statuseffects &= ~status.mask;
					break;
				}
			}
			if (!exists) error = TRUE;
		}
		if (error) badoption(opts);
		return;
	}

	fullname = "statuslines";
	if (match_optname(opts, fullname, sizeof("statuslines")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if(!op){
			return;
		}
		if (negated) bad_negation(fullname, FALSE);
		else {
		    if (!initial)
		        need_redraw = TRUE;
		    iflags.statuslines = atoi(op);
		    if (iflags.statuslines < 2) {
		        iflags.statuslines = 2;
		        badoption(opts);
		    }
		}
		return;
	}


#ifdef SORTLOOT
	fullname = "sortloot";
	if (match_optname(opts, fullname, 4, TRUE)) {
		op = string_for_env_opt(fullname, opts, FALSE);
		if (op) {
			switch (tolower(*op)) {
                        case 'n':
                        case 'l':
                        case 'f': iflags.sortloot = tolower(*op);
				break;
                        default:  badoption(opts);
				return;
			}
		}
		return;
	}
#endif /* SORTLOOT */

	fullname = "suppress_alert";
	if (match_optname(opts, fullname, 4, TRUE)) {
		op = string_for_opt(opts, negated);
		if (negated) bad_negation(fullname, FALSE);
		else if (op) (void) feature_alert_opts(op,fullname);
		return;
	}
	
	fullname = "travelplus";
	if (match_optname(opts, fullname, 7, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.travelplus = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

#ifdef VIDEOSHADES
	/* videocolors:string */
	fullname = "videocolors";
	if (match_optname(opts, fullname, 6, TRUE) ||
	    match_optname(opts, "videocolours", 10, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_videocolors(opts))
			badoption(opts);
		return;
	}
	/* videoshades:string */
	fullname = "videoshades";
	if (match_optname(opts, fullname, 6, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_videoshades(opts))
			badoption(opts);
		return;
	}
#endif /* VIDEOSHADES */
#ifdef MSDOS
# ifdef NO_TERMS
	/* video:string -- must be after longer tests */
	fullname = "video";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_video(opts))
			badoption(opts);
		return;
	}
# endif /* NO_TERMS */
	/* soundcard:string -- careful not to match boolean 'sound' */
	fullname = "soundcard";
	if (match_optname(opts, fullname, 6, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!assign_soundcard(opts))
			badoption(opts);
		return;
	}
#endif /* MSDOS */

	/* WINCAP
	 * map_mode:[tiles|ascii4x6|ascii6x8|ascii8x8|ascii16x8|ascii7x12|ascii8x12|
			ascii16x12|ascii12x16|ascii10x18|fit_to_screen] */
	fullname = "map_mode";
	if (match_optname(opts, fullname, sizeof("map_mode")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    if (!strncmpi (op, "tiles", sizeof("tiles")-1))
			iflags.wc_map_mode = MAP_MODE_TILES;
		    else if (!strncmpi (op, "ascii4x6", sizeof("ascii4x6")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII4x6;
		    else if (!strncmpi (op, "ascii6x8", sizeof("ascii6x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII6x8;
		    else if (!strncmpi (op, "ascii8x8", sizeof("ascii8x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII8x8;
		    else if (!strncmpi (op, "ascii16x8", sizeof("ascii16x8")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII16x8;
		    else if (!strncmpi (op, "ascii7x12", sizeof("ascii7x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII7x12;
		    else if (!strncmpi (op, "ascii8x12", sizeof("ascii8x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII8x12;
		    else if (!strncmpi (op, "ascii16x12", sizeof("ascii16x12")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII16x12;
		    else if (!strncmpi (op, "ascii12x16", sizeof("ascii12x16")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII12x16;
		    else if (!strncmpi (op, "ascii10x18", sizeof("ascii10x18")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII10x18;
		    else if (!strncmpi (op, "fit_to_screen", sizeof("fit_to_screen")-1))
			iflags.wc_map_mode = MAP_MODE_ASCII_FIT_TO_SCREEN;
		    else
		    	badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * scroll_amount:nn */
	fullname = "scroll_amount";
	if (match_optname(opts, fullname, sizeof("scroll_amount")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_scroll_amount = negated ? 1 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * scroll_margin:nn */
	fullname = "scroll_margin";
	if (match_optname(opts, fullname, sizeof("scroll_margin")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_scroll_margin = negated ? 5 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	fullname = "subkeyvalue";
	if (match_optname(opts, fullname, 5, TRUE)) {
		if (negated) bad_negation(fullname, FALSE);
		else {
#if defined(WIN32CON)
			op = string_for_opt(opts, 0);
			map_subkeyvalue(op);
#endif
		}
		return;
	}
	/* WINCAP
	 * tile_width:nn */
	fullname = "tile_width";
	if (match_optname(opts, fullname, sizeof("tile_width")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_tile_width = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * tile_file:name */
	fullname = "tile_file";
	if (match_optname(opts, fullname, sizeof("tile_file")-1, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
			if (iflags.wc_tile_file) free(iflags.wc_tile_file);
			iflags.wc_tile_file = (char *)alloc(strlen(op) + 1);
			Strcpy(iflags.wc_tile_file, op);
		}
		return;
	}
	/* WINCAP
	 * tile_height:nn */
	fullname = "tile_height";
	if (match_optname(opts, fullname, sizeof("tile_height")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_tile_height = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	/* WINCAP
	 * vary_msgcount:nn */
	fullname = "vary_msgcount";
	if (match_optname(opts, fullname, sizeof("vary_msgcount")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wc_vary_msgcount = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	fullname = "windowtype";
	if (match_optname(opts, fullname, 3, TRUE)) {
	    if (negated) {
		bad_negation(fullname, FALSE);
		return;
	    } else if ((op = string_for_env_opt(fullname, opts, FALSE)) != 0) {
		char buf[WINTYPELEN];
		nmcpy(buf, op, WINTYPELEN);
		choose_windows(buf);
	    }
	    return;
	}
	fullname = "wizlevelport";
	if (match_optname(opts, fullname, 12, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wizlevelport = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}
	fullname = "wizcombatdebug";
	if (match_optname(opts, fullname, 14, TRUE)) {
		op = string_for_opt(opts, negated);
		if ((negated && !op) || (!negated && op)) {
			iflags.wizcombatdebug = negated ? 0 : atoi(op);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}

	/* WINCAP
	 * setting window colors
         * syntax: windowcolors=menu foregrnd/backgrnd text foregrnd/backgrnd
         */
	fullname = "windowcolors";
	if (match_optname(opts, fullname, 7, TRUE)) {
		if ((op = string_for_opt(opts, FALSE)) != 0) {
			if (!wc_set_window_colors(op))
				badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}


	/* WINCAP2
	 * term_cols:amount */
	fullname = "term_cols";
	if (match_optname(opts, fullname, sizeof("term_cols")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		iflags.wc2_term_cols = atoi(op);
		if (negated) bad_negation(fullname, FALSE);
		return;
	}

	/* WINCAP2
	 * term_rows:amount */
	fullname = "term_rows";
	if (match_optname(opts, fullname, sizeof("term_rows")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		iflags.wc2_term_rows = atoi(op);
		if (negated) bad_negation(fullname, FALSE);
		return;
	}


	/* WINCAP2
	 * petattr:string */
	fullname = "petattr";
	if (match_optname(opts, fullname, sizeof("petattr")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (op && !negated) {
		    iflags.wc2_petattr = curses_read_attrs(op);
		    if (!curses_read_attrs(op))
		    	badoption(opts);
		} else if (negated) bad_negation(fullname, TRUE);
		return;
	}


	/* WINCAP2
	 * windowborders:n */
	fullname = "windowborders";
	if (match_optname(opts, fullname, sizeof("windowborders")-1, TRUE)) {
		op = string_for_opt(opts, negated);
		if (negated && op) bad_negation(fullname, TRUE);
		else {
		    if (negated)
		        iflags.wc2_windowborders = 2; /* Off */
		    else if (!op)
		        iflags.wc2_windowborders = 1; /* On */
		    else    /* Value supplied */
		        iflags.wc2_windowborders = atoi(op);
		    if ((iflags.wc2_windowborders > 3) ||
		     (iflags.wc2_windowborders < 1)) {
		        iflags.wc2_windowborders = 0;
		        badoption(opts);
		    }
		}
		return;
	}

	/* menustyle:traditional or combo or full or partial */
	if (match_optname(opts, "menustyle", 4, TRUE)) {
		int tmp;
		boolean val_required = (strlen(opts) > 5 && !negated);

		if (!(op = string_for_opt(opts, !val_required))) {
		    if (val_required) return; /* string_for_opt gave feedback */
		    tmp = negated ? 'n' : 'f';
		} else {
		    tmp = tolower(*op);
		}
		switch (tmp) {
			case 'n':	/* none */
			case 't':	/* traditional */
				flags.menu_style = MENU_TRADITIONAL;
				break;
			case 'c':	/* combo: trad.class sel+menu */
				flags.menu_style = MENU_COMBINATION;
				break;
			case 'p':	/* partial: no class menu */
				flags.menu_style = MENU_PARTIAL;
				break;
			case 'f':	/* full: class menu + menu */
				flags.menu_style = MENU_FULL;
				break;
			default:
				badoption(opts);
		}
		return;
	}

	fullname = "menu_headings";
	if (match_optname(opts, fullname, 12, TRUE)) {
		if (negated) {
			bad_negation(fullname, FALSE);
			return;
		}
		else if (!(opts = string_for_env_opt(fullname, opts, FALSE))) {
			return;
		}
		if (!strcmpi(opts,"bold"))
			iflags.menu_headings = ATR_BOLD;
		else if (!strcmpi(opts,"inverse"))
			iflags.menu_headings = ATR_INVERSE;
		else if (!strcmpi(opts,"underline"))
			iflags.menu_headings = ATR_ULINE;
		else
			badoption(opts);
		return;
	}

	/* check for menu command mapping */
	for (i = 0; i < NUM_MENU_CMDS; i++) {
	    fullname = default_menu_cmd_info[i].name;
	    if (match_optname(opts, fullname, (int)strlen(fullname), TRUE)) {
		if (negated)
		    bad_negation(fullname, FALSE);
		else if ((op = string_for_opt(opts, FALSE)) != 0) {
		    int j;
		    char c, op_buf[BUFSZ];
		    boolean isbad = FALSE;

		    escapes(op, op_buf);
		    c = *op_buf;

		    if (c == 0 || c == '\r' || c == '\n' || c == '\033' ||
			    c == ' ' || digit(c) || (letter(c) && c != '@'))
			isbad = TRUE;
		    else	/* reject default object class symbols */
			for (j = 1; j < MAXOCLASSES; j++)
			    if (c == def_oc_syms[i]) {
				isbad = TRUE;
				break;
			    }

		    if (isbad)
			badoption(opts);
		    else
			add_menu_cmd_alias(c, default_menu_cmd_info[i].cmd);
		}
		return;
	    }
	}

	/* OK, if we still haven't recognized the option, check the boolean
	 * options list
	 */
	for (i = 0; boolopt[i].name; i++) {
		if (match_optname(opts, boolopt[i].name, 3, FALSE)) {
			/* options that don't exist */
			if (!boolopt[i].addr) {
			    if (!initial && !negated)
				pline_The("\"%s\" option is not available.",
					boolopt[i].name);
			    return;
			}
			/* options that must come from config file */
			if (!initial && (boolopt[i].optflags == SET_IN_FILE)) {
			    rejectoption(boolopt[i].name);
			    return;
			}

			 if (iflags.debug_fuzzer && !initial) {
                		/* don't randomly toggle this/these */
                		if (boolopt[i].addr == &flags.silent)
                    			return;
       			  }
		
	
			*(boolopt[i].addr) = !negated;

			duplicate_opt_detection(boolopt[i].name, 0);

#if defined(TERMLIB) || defined(ASCIIGRAPH) || defined(MAC_GRAPHICS_ENV) || defined(CURSES_GRAPHICS)
			if (FALSE
# ifdef TERMLIB
				 || (boolopt[i].addr) == &iflags.DECgraphics
# endif
# ifdef ASCIIGRAPH
				 || (boolopt[i].addr) == &iflags.IBMgraphics
# endif
# ifdef MAC_GRAPHICS_ENV
				 || (boolopt[i].addr) == &iflags.MACgraphics
# endif
# ifdef UTF8_GLYPHS
				 || (boolopt[i].addr) == &iflags.UTF8graphics
# endif
# ifdef CURSES_GRAPHICS
				 || (boolopt[i].addr) == &iflags.cursesgraphics
# endif
				) {
# ifdef REINCARNATION
			    if (!initial && Is_rogue_level(&u.uz))
				assign_rogue_graphics(FALSE);
# endif
			    need_redraw = TRUE;
# ifdef TERMLIB
			    if ((boolopt[i].addr) == &iflags.DECgraphics)
				switch_graphics(iflags.DECgraphics ?
						DEC_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef ASCIIGRAPH
			    if ((boolopt[i].addr) == &iflags.IBMgraphics)
				switch_graphics(iflags.IBMgraphics ?
						IBM_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef MAC_GRAPHICS_ENV
			    if ((boolopt[i].addr) == &iflags.MACgraphics)
				switch_graphics(iflags.MACgraphics ?
						MAC_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef UTF8_GLYPHS
			    if ((boolopt[i].addr) == &iflags.UTF8graphics)
				switch_graphics(iflags.UTF8graphics ?
						UTF8_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef CURSES_GRAPHICS
			    if ((boolopt[i].addr) == &iflags.cursesgraphics)
				switch_graphics(iflags.cursesgraphics ?
						CURS_GRAPHICS : ASCII_GRAPHICS);
# endif
# ifdef REINCARNATION
			    if (!initial && Is_rogue_level(&u.uz))
				assign_rogue_graphics(TRUE);
# endif
			}
#endif /* TERMLIB || ASCIIGRAPH || MAC_GRAPHICS_ENV */

			/* only do processing below if setting with doset() */
			if (initial) return;

			if ((boolopt[i].addr) == &flags.time
#ifdef EXP_ON_BOTL
			 || (boolopt[i].addr) == &flags.showexp
#endif
#ifdef SCORE_ON_BOTL
			 || (boolopt[i].addr) == &flags.showscore
#endif
			    )
			    flags.botl = TRUE;

			else if ((boolopt[i].addr) == &flags.invlet_constant) {
			    if (flags.invlet_constant) reassign();
			}
#ifdef LAN_MAIL
			else if ((boolopt[i].addr) == &flags.biff) {
			    if (flags.biff) lan_mail_init();
			    else lan_mail_finish();
			}
#endif
			else if ((boolopt[i].addr) == &flags.lit_corridor) {
			    /*
			     * All corridor squares seen via night vision or
			     * candles & lamps change.  Update them by calling
			     * newsym() on them.  Don't do this if we are
			     * initializing the options --- the vision system
			     * isn't set up yet.
			     */
			    vision_recalc(2);		/* shut down vision */
			    vision_full_recalc = 1;	/* delayed recalc */
			    if (iflags.use_color) need_redraw = TRUE;  /* darkroom refresh */
			}
			else if ((boolopt[i].addr) == &iflags.use_inverse ||
					(boolopt[i].addr) == &iflags.showrace ||
					(boolopt[i].addr) == &iflags.hilite_pet ||
					(boolopt[i].addr) == &iflags.wc2_guicolor) {
			    need_redraw = TRUE;
			}
#ifdef CURSES_GRAPHICS
			else if ((boolopt[i].addr) == &iflags.cursesgraphics) {
			    need_redraw = TRUE;
			}
#endif
#ifdef TEXTCOLOR
			else if ((boolopt[i].addr) == &iflags.use_color) {
			    need_redraw = TRUE;
# ifdef TOS
			    if ((boolopt[i].addr) == &iflags.use_color
				&& iflags.BIOS) {
				if (colors_changed)
				    restore_colors();
				else
				    set_colors();
			    }
# endif
			}
#endif

			return;
		}
	}

	/* out of valid options */
	badoption(opts);
}


static NEARDATA const char *menutype[] = {
	"traditional", "combination", "partial", "full"
};

static NEARDATA const char *burdentype[] = {
	"unencumbered", "burdened", "stressed",
	"strained", "overtaxed", "overloaded"
};

static NEARDATA const char *pokedexsections[] = {
	"stats", "generation", "weight", "resists", "conveys",
	"movement", "thinking", "biology", "mechanics", "race", "vision", "attacks"/*, "summary"*/
};

static NEARDATA const char *runmodes[] = {
	"teleport", "run", "walk", "crawl"
};
static NEARDATA const char *delay_lengths[] = {
	"none", "short", "normal"
};

#ifdef SORTLOOT
static NEARDATA const char *sortltype[] = {
	"none", "loot", "full"
};
#endif

/*
 * Convert the given string of object classes to a string of default object
 * symbols.
 */
STATIC_OVL void
oc_to_str(src,dest)
    char *src, *dest;
{
    int i;

    while ((i = (int) *src++) != 0) {
	if (i < 0 || i >= MAXOCLASSES)
	    impossible("oc_to_str:  illegal object class %d", i);
	else
	    *dest++ = def_oc_syms[i];
    }
    *dest = '\0';
}

/*
 * Add the given mapping to the menu command map list.  Always keep the
 * maps valid C strings.
 */
void
add_menu_cmd_alias(from_ch, to_ch)
    char from_ch, to_ch;
{
    if (n_menu_mapped >= MAX_MENU_MAPPED_CMDS)
	pline("out of menu map space.");
    else {
	mapped_menu_cmds[n_menu_mapped] = from_ch;
	mapped_menu_op[n_menu_mapped] = to_ch;
	n_menu_mapped++;
	mapped_menu_cmds[n_menu_mapped] = 0;
	mapped_menu_op[n_menu_mapped] = 0;
    }
}

/*
 * Map the given character to its corresponding menu command.  If it
 * doesn't match anything, just return the original.
 */
char
map_menu_cmd(ch)
    char ch;
{
    char *found = index(mapped_menu_cmds, ch);
    if (found) {
	int idx = found - mapped_menu_cmds;
	ch = mapped_menu_op[idx];
    }
    return ch;
}


#if defined(MICRO) || defined(MAC) || defined(WIN32)
# define OPTIONS_HEADING "OPTIONS"
#else
# define OPTIONS_HEADING "NETHACKOPTIONS"
#endif

static char fmtstr_doset_add_menu[] = "%s%-15s [%s]   "; 
static char fmtstr_doset_add_menu_tab[] = "%s\t[%s]";

STATIC_OVL void
doset_add_menu(win, option, indexoffset)
    winid win;			/* window to add to */
    const char *option;		/* option name */
    int indexoffset;		/* value to add to index in compopt[], or zero
				   if option cannot be changed */
{
    const char *value = "unknown";		/* current value */
    char buf[BUFSZ], buf2[BUFSZ];
    anything any;
    int i;

    any.a_void = 0;
    if (indexoffset == 0) {
	any.a_int = 0;
	value = get_compopt_value(option, buf2);
    } else {
	for (i=0; compopt[i].name; i++)
	    if (strcmp(option, compopt[i].name) == 0) break;

	if (compopt[i].name) {
	    any.a_int = i + 1 + indexoffset;
	    value = get_compopt_value(option, buf2);
	} else {
	    /* We are trying to add an option not found in compopt[].
	       This is almost certainly bad, but we'll let it through anyway
	       (with a zero value, so it can't be selected). */
	    any.a_int = 0;
	}
    }
    /* "    " replaces "a - " -- assumes menus follow that style */
    if (!iflags.menu_tab_sep)
	Sprintf(buf, fmtstr_doset_add_menu, any.a_int ? "" : "    ", option, value);
    else
	Sprintf(buf, fmtstr_doset_add_menu_tab, option, value);
    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
}

/* Changing options via menu by Per Liboriussen */
int
doset()
{
	char buf[BUFSZ], buf2[BUFSZ];
	int i, pass, boolcount, pick_cnt, pick_idx, opt_indx;
	boolean *bool_p;
	winid tmpwin;
	anything any;
	menu_item *pick_list;
	int indexoffset, startpass, endpass;
	boolean setinitial = FALSE, fromfile = FALSE;
	int biggest_name = 0;

	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);

	any.a_void = 0;
 add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "Booleans (selecting will toggle value):", MENU_UNSELECTED);
	any.a_int = 0;
	/* first list any other non-modifiable booleans, then modifiable ones */
	for (pass = 0; pass <= 1; pass++)
	    for (i = 0; boolopt[i].name; i++)
		if ((bool_p = boolopt[i].addr) != 0 &&
			((boolopt[i].optflags == DISP_IN_GAME && pass == 0) ||
			 (boolopt[i].optflags == SET_IN_GAME && pass == 1))) {
		    if (bool_p == &flags.female) continue;  /* obsolete */
#ifdef WIZARD
		    if (bool_p == &iflags.sanity_check && !wizard) continue;
		    if (bool_p == &iflags.menu_tab_sep && !wizard) continue;
#endif
		    if (is_wc_option(boolopt[i].name) &&
			!wc_supported(boolopt[i].name)) continue;
		    if (is_wc2_option(boolopt[i].name) &&
			!wc2_supported(boolopt[i].name)) continue;
		    any.a_int = (pass == 0) ? 0 : i + 1;
		    if (!iflags.menu_tab_sep)
			Sprintf(buf, "%s%-13s [%s]",
			    pass == 0 ? "    " : "",
			    boolopt[i].name, *bool_p ? "true" : "false");
 		    else
			Sprintf(buf, "%s\t[%s]",
			    boolopt[i].name, *bool_p ? "true" : "false");
		    add_menu(tmpwin, NO_GLYPH, &any, 0, 0,
			     ATR_NONE, buf, MENU_UNSELECTED);
		}

	boolcount = i;
	indexoffset = boolcount;
	any.a_void = 0;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
 add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "Compounds (selecting will prompt for new value):",
		 MENU_UNSELECTED);

	startpass = DISP_IN_GAME;
	endpass = SET_IN_GAME;

	/* spin through the options to find the biggest name
           and adjust the format string accordingly if needed */
	biggest_name = 0;
	for (i = 0; compopt[i].name; i++)
		if (compopt[i].optflags >= startpass && compopt[i].optflags <= endpass &&
		    strlen(compopt[i].name) > (unsigned) biggest_name)
			biggest_name = (int) strlen(compopt[i].name);
	if (biggest_name > 30) biggest_name = 30;
	if (!iflags.menu_tab_sep)
		Sprintf(fmtstr_doset_add_menu, "%%s%%-%ds [%%s]", biggest_name);
	
	/* deliberately put `name', `role', `race', `gender' first */
	doset_add_menu(tmpwin, "name", 0);
	doset_add_menu(tmpwin, "role", 0);
	doset_add_menu(tmpwin, "race", 0);
	doset_add_menu(tmpwin, "gender", 0);

	for (pass = startpass; pass <= endpass; pass++) 
	    for (i = 0; compopt[i].name; i++)
		if (compopt[i].optflags == pass) {
 		    	if (!strcmp(compopt[i].name, "name") ||
		    	    !strcmp(compopt[i].name, "role") ||
		    	    !strcmp(compopt[i].name, "race") ||
		    	    !strcmp(compopt[i].name, "gender"))
		    	    	continue;
		    	else if (is_wc_option(compopt[i].name) &&
					!wc_supported(compopt[i].name))
		    		continue;
		    	else if (is_wc2_option(compopt[i].name) &&
					!wc2_supported(compopt[i].name))
		    		continue;
				else if (!strcmp(compopt[i].name, "wizlevelport") && !wizard)
					continue;
				else if (!strcmp(compopt[i].name, "wizcombatdebug") && !wizard)
					continue;
		    	else
				doset_add_menu(tmpwin, compopt[i].name,
					(pass == DISP_IN_GAME) ? 0 : indexoffset);
		}
#ifdef AUTOPICKUP_EXCEPTIONS
	any.a_int = -1;
	Sprintf(buf, "autopickup exceptions (%d currently set)",
		count_ape_maps((int *)0, (int *)0));
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);

#endif /* AUTOPICKUP_EXCEPTIONS */
#ifdef PREFIXES_IN_USE
	any.a_void = 0;
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
		 "Variable playground locations:", MENU_UNSELECTED);
	for (i = 0; i < PREFIX_COUNT; i++)
		doset_add_menu(tmpwin, fqn_prefix_names[i], 0);
#endif
	end_menu(tmpwin, "Set what options?");
	need_redraw = FALSE;
	if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &pick_list)) > 0) {
	    /*
	     * Walk down the selection list and either invert the booleans
	     * or prompt for new values. In most cases, call parseoptions()
	     * to take care of options that require special attention, like
	     * redraws.
	     */
	    for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
		opt_indx = pick_list[pick_idx].item.a_int - 1;
#ifdef AUTOPICKUP_EXCEPTIONS
		if (opt_indx == -2) {
		    special_handling("autopickup_exception",
		    			setinitial, fromfile);
		} else
#endif
		if (opt_indx < boolcount) {
		    /* boolean option */
		    Sprintf(buf, "%s%s", *boolopt[opt_indx].addr ? "!" : "",
			    boolopt[opt_indx].name);
		    parseoptions(buf, setinitial, fromfile);
		    if (wc_supported(boolopt[opt_indx].name) ||
		    	wc2_supported(boolopt[opt_indx].name))
			preference_update(boolopt[opt_indx].name);
		} else {
		    /* compound option */
		    opt_indx -= boolcount;

		    if (!special_handling(compopt[opt_indx].name,
							setinitial, fromfile)) {
			Sprintf(buf, "Set %s to what?", compopt[opt_indx].name);
			getlin(buf, buf2);
			if (buf2[0] == '\033')
			    continue;
			Sprintf(buf, "%s:%s", compopt[opt_indx].name, buf2);
			/* pass the buck */
			parseoptions(buf, setinitial, fromfile);
		    }
		    if (wc_supported(compopt[opt_indx].name) ||
			wc2_supported(compopt[opt_indx].name))
			preference_update(compopt[opt_indx].name);
		}
	    }
	    free((genericptr_t)pick_list);
	    pick_list = (menu_item *)0;
	}

	destroy_nhwindow(tmpwin);
	if (need_redraw) {
	    if (flags.lit_corridor && iflags.use_color) {
		showsyms[S_drkroom]=showsyms[S_litroom];
	    } else {
		showsyms[S_drkroom]=showsyms[S_stone];
	    }
 	    (void) doredraw();
	}
	return MOVE_CANCELLED;
}

STATIC_OVL boolean
special_handling(optname, setinitial, setfromfile)
const char *optname;
boolean setinitial,setfromfile;
{
    winid tmpwin;
    anything any;
    int i;
    char buf[BUFSZ];
    boolean retval = FALSE;
    
    /* Special handling of menustyle, pickup_burden, pickup_types,
     * disclose, runmode, msg_window, menu_headings, number_pad and sortloot
	 * attack_mode, pokedex, wizlevelport, wizcombatdebug
#ifdef AUTOPICKUP_EXCEPTIONS
     * Also takes care of interactive autopickup_exception_handling changes.
#endif
     */
	if(!strcmp("attack_mode", optname)){
		menu_item *pick = (menu_item *) 0;
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		any = zeroany;
		any.a_char = ATTACK_MODE_PACIFIST;
		add_menu(tmpwin, NO_GLYPH, &any, ATTACK_MODE_PACIFIST, 0, 0,
			"pacifist: don't fight anything", MENU_UNSELECTED);
		any.a_char = ATTACK_MODE_CHAT;
		add_menu(tmpwin, NO_GLYPH, &any, ATTACK_MODE_CHAT, 0, 0,
			"chat: chat with peacefuls, fight hostiles",
			MENU_UNSELECTED);
		any.a_char = ATTACK_MODE_ASK;
		add_menu(tmpwin, NO_GLYPH, &any, ATTACK_MODE_ASK, 0, 0,
			"ask: ask to fight peacefuls", MENU_UNSELECTED);
		any.a_char = ATTACK_MODE_FIGHT_ALL;
		add_menu(tmpwin, NO_GLYPH, &any, ATTACK_MODE_FIGHT_ALL, 0, 0,
			"fightall: fight peacefuls and hostiles", MENU_UNSELECTED);
		end_menu(tmpwin, "Select attack_mode:");
		if (select_menu(tmpwin, PICK_ONE, &pick) > 0) {
			iflags.attack_mode = pick->item.a_char;
			free((genericptr_t) pick);
		}
		destroy_nhwindow(tmpwin);
    } else if (!strcmp("menustyle", optname)){
		const char *style_name;
		menu_item *style_pick = (menu_item *)0;
			tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(menutype); i++) {
			style_name = menutype[i];
				/* note: separate `style_name' variable used
			   to avoid an optimizer bug in VAX C V2.3 */
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, *style_name, 0,
				 ATR_NONE, style_name, MENU_UNSELECTED);
			}
		end_menu(tmpwin, "Select menustyle:");
		if (select_menu(tmpwin, PICK_ONE, &style_pick) > 0) {
			flags.menu_style = style_pick->item.a_int - 1;
			free((genericptr_t)style_pick);
			}
		destroy_nhwindow(tmpwin);
			retval = TRUE;
    } else if (!strcmp("pickup_burden", optname)) {
		const char *burden_name, *burden_letters = "ubsntl";
		menu_item *burden_pick = (menu_item *)0;
			tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(burdentype); i++) {
			burden_name = burdentype[i];
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, burden_letters[i], 0,
				 ATR_NONE, burden_name, MENU_UNSELECTED);
			}
		end_menu(tmpwin, "Select encumbrance level:");
		if (select_menu(tmpwin, PICK_ONE, &burden_pick) > 0) {
			flags.pickup_burden = burden_pick->item.a_int - 1;
			free((genericptr_t)burden_pick);
		}
		destroy_nhwindow(tmpwin);
		retval = TRUE;
    } else if (!strcmp("pickup_types", optname)) {
		/* parseoptions will prompt for the list of types */
		parseoptions(strcpy(buf, "pickup_types"), setinitial, setfromfile);
		retval = TRUE;
    } else if (!strcmp("disclose", optname)) {
		int pick_cnt, pick_idx, opt_idx;
		menu_item *disclosure_category_pick = (menu_item *)0;
		/*
		 * The order of disclose_names[]
			 * must correspond to disclosure_options in decl.h
			 */
		static const char *disclosure_names[] = {
			"inventory", "attributes", "vanquished", "genocides", "conduct"
		};
		int disc_cat[NUM_DISCLOSURE_OPTIONS];
		const char *disclosure_name;

			tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
			disclosure_name = disclosure_names[i];
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, disclosure_options[i], 0,
				 ATR_NONE, disclosure_name, MENU_UNSELECTED);
			disc_cat[i] = 0;
			}
		end_menu(tmpwin, "Change which disclosure options categories:");
		if ((pick_cnt = select_menu(tmpwin, PICK_ANY, &disclosure_category_pick)) > 0) {
			for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
			opt_idx = disclosure_category_pick[pick_idx].item.a_int - 1;
			disc_cat[opt_idx] = 1;
			}
			free((genericptr_t)disclosure_category_pick);
			disclosure_category_pick = (menu_item *)0;
		}
		destroy_nhwindow(tmpwin);

		for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
			if (disc_cat[i]) {
				char dbuf[BUFSZ];
			menu_item *disclosure_option_pick = (menu_item *)0;
			Sprintf(dbuf, "Disclosure options for %s:", disclosure_names[i]);
				tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			any.a_char = DISCLOSE_NO_WITHOUT_PROMPT;
			add_menu(tmpwin, NO_GLYPH, &any, 'a', 0,
				ATR_NONE,"Never disclose and don't prompt", MENU_UNSELECTED);
			any.a_void = 0;
			any.a_char = DISCLOSE_YES_WITHOUT_PROMPT;
			add_menu(tmpwin, NO_GLYPH, &any, 'b', 0,
				ATR_NONE,"Always disclose and don't prompt", MENU_UNSELECTED);
			any.a_void = 0;
			any.a_char = DISCLOSE_PROMPT_DEFAULT_NO;
			add_menu(tmpwin, NO_GLYPH, &any, 'c', 0,
				ATR_NONE,"Prompt and default answer to \"No\"", MENU_UNSELECTED);
			any.a_void = 0;
			any.a_char = DISCLOSE_PROMPT_DEFAULT_YES;
			add_menu(tmpwin, NO_GLYPH, &any, 'd', 0,
				ATR_NONE,"Prompt and default answer to \"Yes\"", MENU_UNSELECTED);
			end_menu(tmpwin, dbuf);
			if (select_menu(tmpwin, PICK_ONE, &disclosure_option_pick) > 0) {
				flags.end_disclose[i] = disclosure_option_pick->item.a_char;
				free((genericptr_t)disclosure_option_pick);
			}
			destroy_nhwindow(tmpwin);
			}
		}
		retval = TRUE;
    } else if (!strcmp("pokedex", optname)) {
		char buf[BUFSZ]; 
		menu_item *mode_pick = (menu_item *)0;
		boolean done = FALSE;

		while (!done){
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			for (i = 0; i < SIZE(pokedexsections); i++) {
				Sprintf(buf, "%-11s (%s)",pokedexsections[i], (iflags.pokedex & (1 << i)) ? "on" : "off");
				any.a_int = i + 1;
				add_menu(tmpwin, NO_GLYPH, &any, (char)((int)'a' + i), 0,
					ATR_NONE, buf, MENU_UNSELECTED);
			}
			end_menu(tmpwin, "Toggle pokedex sections on/off:");
			if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
				iflags.pokedex ^= (1 << (mode_pick->item.a_int - 1));
			}
			else
				done = TRUE;
			free((genericptr_t)mode_pick);
			destroy_nhwindow(tmpwin);
		}
		retval = TRUE;
	} else if (!strcmp(optname, "statuseffects")) {
		char buf[BUFSZ];
		menu_item *mode_pick = (menu_item *)0;
		boolean done = FALSE;
		/*
		 * Refresh statusline so shown status conditions don't
		 * end up desynced while in the menu. curses doesn't
		 * update the statusline during the menu at all and
		 * refreshing it each time is too slow because it
		 * redraws everything (including drawing the map and
		 * then overwriting it), so only do this for tty.
		 */
		boolean refresh_botl = !strcmp(windowprocs.name, "tty");

		if (refresh_botl) bot();
		while (!done){
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			for (i = 0; i < SIZE(status_effects); i++) {
				struct status_effect status = status_effects[i];
				/* a-z then A-Z */
				char letter = 'a' + i;
				if (letter > 'z')
					letter += 'A'-'z'-1;
				Sprintf(buf, "%-11s (%s)", status.name, iflags.statuseffects & status.mask ? "on" : "off");
				any.a_int = i + 1;
				add_menu(tmpwin, NO_GLYPH, &any, letter, 0,
					ATR_NONE, buf, MENU_UNSELECTED);
			}
			end_menu(tmpwin, "Toggle shown status effects on/off:");
			if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
				iflags.statuseffects ^= status_effects[mode_pick->item.a_int - 1].mask;
				if (refresh_botl) bot();
			}
			else
				done = TRUE;
			free((genericptr_t)mode_pick);
			destroy_nhwindow(tmpwin);
		}
		retval = TRUE;
	} else if (!strcmp("wizlevelport", optname)) {
		char buf[BUFSZ];
		menu_item *pick = (menu_item *) 0;
		boolean done = FALSE;
		while (!done) {
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			any = zeroany;

			any.a_int = WIZLVLPORT_TRADITIONAL + 1;
			Sprintf(buf, "%-33s %s", "traditional: pick level directly",
				iflags.wizlevelport == WIZLVLPORT_TRADITIONAL ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'a', 0, 0, buf, MENU_UNSELECTED);

			any.a_int = WIZLVLPORT_TWOMENU + 1;
			Sprintf(buf, "%-33s %s", "twomenu: pick branch, then level",
				iflags.wizlevelport & WIZLVLPORT_TWOMENU ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'b', 0, 0, buf, MENU_UNSELECTED);

			if (iflags.wizlevelport & WIZLVLPORT_TWOMENU) {
				any.a_int = WIZLVLPORT_BRANCHES_FIRST + 1;
				Sprintf(buf, "%-33s %s", " -> branches first",
					iflags.wizlevelport & WIZLVLPORT_BRANCHES_FIRST ? "(on)" : "(off)");
				add_menu(tmpwin, NO_GLYPH, &any, 'c', 0, 0, buf, MENU_UNSELECTED);

				any.a_int = WIZLVLPORT_SELECTED_DUNGEON + 1;
				Sprintf(buf, "%-33s %s", " -> tight scope",
					iflags.wizlevelport & WIZLVLPORT_SELECTED_DUNGEON ? "(on)" : "(off)");
				add_menu(tmpwin, NO_GLYPH, &any, 'd', 0, 0, buf, MENU_UNSELECTED);
			}

			end_menu(tmpwin, "Select wizlevelport:");
			if (select_menu(tmpwin, PICK_ONE, &pick) > 0) {
				int out = pick->item.a_int - 1;
				if (out == WIZLVLPORT_TRADITIONAL) {
					iflags.wizlevelport = WIZLVLPORT_TRADITIONAL;
				}
				else if (out == WIZLVLPORT_TWOMENU) {
					iflags.wizlevelport |= WIZLVLPORT_TWOMENU;
				}
				else {
					iflags.wizlevelport ^= out;
				}
			}
			else
				done = TRUE;
			free((genericptr_t) pick);
			destroy_nhwindow(tmpwin);
		}
		retval = TRUE;
	} else if (!strcmp("wizcombatdebug", optname)) {
		char buf[BUFSZ];
		menu_item *pick = (menu_item *) 0;
		boolean done = FALSE;
		while (!done) {
			tmpwin = create_nhwindow(NHW_MENU);
			start_menu(tmpwin);
			any = zeroany;

			any.a_int = WIZCOMBATDEBUG_NONE + 1;
			Sprintf(buf, "%-43s %s", "No extra info.",
				iflags.wizcombatdebug == WIZLVLPORT_TRADITIONAL ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'a', 0, 0, buf, MENU_UNSELECTED);

			any.a_int = WIZCOMBATDEBUG_DMG + 1;
			Sprintf(buf, "%-43s %s", "Show combat damage dealt.",
				iflags.wizcombatdebug & WIZCOMBATDEBUG_DMG ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'b', 0, 0, buf, MENU_UNSELECTED);

			any.a_int = WIZCOMBATDEBUG_FULLDMG + 1;
			Sprintf(buf, "%-43s %s", "Show combat damage dealt breakdown.",
				iflags.wizcombatdebug & WIZCOMBATDEBUG_FULLDMG ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'c', 0, 0, buf, MENU_UNSELECTED);

			any.a_int = WIZCOMBATDEBUG_ACCURACY + 1;
			Sprintf(buf, "%-43s %s", "Show combat accuracy.",
				iflags.wizcombatdebug & WIZCOMBATDEBUG_ACCURACY ? "(on)" : "(off)");
			add_menu(tmpwin, NO_GLYPH, &any, 'd', 0, 0, buf, MENU_UNSELECTED);

			if (iflags.wizcombatdebug != WIZCOMBATDEBUG_NONE) {
				any.a_int = WIZCOMBATDEBUG_UVM + 1;
				Sprintf(buf, "%-43s %s", "Show combat debug info for you v mon.",
					iflags.wizcombatdebug & WIZCOMBATDEBUG_UVM ? "(on)" : "(off)");
				add_menu(tmpwin, NO_GLYPH, &any, 'e', 0, 0, buf, MENU_UNSELECTED);
				any.a_int = WIZCOMBATDEBUG_MVU + 1;
				Sprintf(buf, "%-43s %s", "Show combat debug info for mon v you.",
					iflags.wizcombatdebug & WIZCOMBATDEBUG_MVU ? "(on)" : "(off)");
				add_menu(tmpwin, NO_GLYPH, &any, 'f', 0, 0, buf, MENU_UNSELECTED);
				any.a_int = WIZCOMBATDEBUG_MVM + 1;
				Sprintf(buf, "%-43s %s", "Show combat debug info for mon v mon.",
					iflags.wizcombatdebug & WIZCOMBATDEBUG_MVM ? "(on)" : "(off)");
				add_menu(tmpwin, NO_GLYPH, &any, 'g', 0, 0, buf, MENU_UNSELECTED);
			}

			end_menu(tmpwin, "Select wizcombatdebug:");
			if (select_menu(tmpwin, PICK_ONE, &pick) > 0) {
				int out = pick->item.a_int - 1;
				if (out == WIZCOMBATDEBUG_NONE) {
					iflags.wizcombatdebug = WIZCOMBATDEBUG_NONE;
				}
				else {
					iflags.wizcombatdebug ^= out;

					if ((iflags.wizcombatdebug ^ out) == WIZCOMBATDEBUG_NONE)
						iflags.wizcombatdebug |= WIZCOMBATDEBUG_UVM;

					if (!(iflags.wizcombatdebug & (WIZCOMBATDEBUG_UVM|WIZCOMBATDEBUG_MVU|WIZCOMBATDEBUG_MVM)))
						iflags.wizcombatdebug = WIZCOMBATDEBUG_NONE;
					if (!(iflags.wizcombatdebug & (WIZCOMBATDEBUG_DMG|WIZCOMBATDEBUG_FULLDMG|WIZCOMBATDEBUG_ACCURACY)))
						iflags.wizcombatdebug = WIZCOMBATDEBUG_NONE;
				}
			}
			else
				done = TRUE;
			free((genericptr_t) pick);
			destroy_nhwindow(tmpwin);
		}
		retval = TRUE;
	} else if (!strcmp("delay_length", optname)) {
		const char *mode_name;
		menu_item *mode_pick = (menu_item *)0;
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(delay_lengths); i++) {
			mode_name = delay_lengths[i];
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, *mode_name, 0,
				 ATR_NONE, mode_name, MENU_UNSELECTED);
		}
		end_menu(tmpwin, "Select delay length:");
		if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
			iflags.delay_length = mode_pick->item.a_int - 1;
			free((genericptr_t)mode_pick);
		}
		destroy_nhwindow(tmpwin);
		retval = TRUE;
	} else if (!strcmp("runmode", optname)) {
		const char *mode_name;
		menu_item *mode_pick = (menu_item *)0;
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(runmodes); i++) {
			mode_name = runmodes[i];
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, *mode_name, 0,
				 ATR_NONE, mode_name, MENU_UNSELECTED);
		}
		end_menu(tmpwin, "Select run/travel display mode:");
		if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
			iflags.runmode = mode_pick->item.a_int - 1;
			free((genericptr_t)mode_pick);
		}
		destroy_nhwindow(tmpwin);
		retval = TRUE;
    }
#ifdef TTY_GRAPHICS
    else if (!strcmp("msg_window", optname)) {
		/* by Christian W. Cooper */
		menu_item *window_pick = (menu_item *)0;
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		any.a_char = 's';
		add_menu(tmpwin, NO_GLYPH, &any, 's', 0,
			ATR_NONE, "single", MENU_UNSELECTED);
		any.a_char = 'c';
		add_menu(tmpwin, NO_GLYPH, &any, 'c', 0,
			ATR_NONE, "combination", MENU_UNSELECTED);
		any.a_char = 'f';
		add_menu(tmpwin, NO_GLYPH, &any, 'f', 0,
			ATR_NONE, "full", MENU_UNSELECTED);
		any.a_char = 'r';
		add_menu(tmpwin, NO_GLYPH, &any, 'r', 0,
			ATR_NONE, "reversed", MENU_UNSELECTED);
		end_menu(tmpwin, "Select message history display type:");
		if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {
			iflags.prevmsg_window = window_pick->item.a_char;
			free((genericptr_t)window_pick);
		}
		destroy_nhwindow(tmpwin);
			retval = TRUE;
    }
#endif
	else if (!strcmp("align_message", optname) ||
		!strcmp("align_status", optname)
	) {
		menu_item *window_pick = (menu_item *)0;
		char abuf[BUFSZ];
		boolean msg = (*(optname+6) == 'm');

		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		any.a_int = ALIGN_TOP;
		add_menu(tmpwin, NO_GLYPH, &any, 't', 0,
			ATR_NONE, "top", MENU_UNSELECTED);
		any.a_int = ALIGN_BOTTOM;
		add_menu(tmpwin, NO_GLYPH, &any, 'b', 0,
			ATR_NONE, "bottom", MENU_UNSELECTED);
		any.a_int = ALIGN_LEFT;
		add_menu(tmpwin, NO_GLYPH, &any, 'l', 0,
			ATR_NONE, "left", MENU_UNSELECTED);
		any.a_int = ALIGN_RIGHT;
		add_menu(tmpwin, NO_GLYPH, &any, 'r', 0,
			ATR_NONE, "right", MENU_UNSELECTED);
		Sprintf(abuf, "Select %s window placement relative to the map:",
			msg ? "message" : "status");
		end_menu(tmpwin, abuf);
		if (select_menu(tmpwin, PICK_ONE, &window_pick) > 0) {		
			if (msg) iflags.wc_align_message = window_pick->item.a_int;
			else iflags.wc_align_status = window_pick->item.a_int;
			free((genericptr_t)window_pick);
		}
		destroy_nhwindow(tmpwin);
			retval = TRUE;
	} else if (!strcmp("number_pad", optname)) {
		static const char *npchoices[3] =
			{"0 (off)", "1 (on)", "2 (on, DOS compatible)"};
		const char *npletters = "abc";
		menu_item *mode_pick = (menu_item *)0;

		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(npchoices); i++) {
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, npletters[i], 0,
				 ATR_NONE, npchoices[i], MENU_UNSELECTED);
			}
		end_menu(tmpwin, "Select number_pad mode:");
		if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
			int mode = mode_pick->item.a_int - 1;
			switch(mode) {
				case 2:
					iflags.num_pad = 1;
					iflags.num_pad_mode = 1;
					break;
				case 1:
					iflags.num_pad = 1;
					iflags.num_pad_mode = 0;
					break;
				case 0:
				default:
					iflags.num_pad = 0;
					iflags.num_pad_mode = 0;
			}
			free((genericptr_t)mode_pick);
			}
		destroy_nhwindow(tmpwin);
			retval = TRUE;
#ifdef SORTLOOT
    } else if (!strcmp("sortloot", optname)) {
		const char *sortl_name;
		menu_item *sortl_pick = (menu_item *)0;
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(sortltype); i++) {
			sortl_name = sortltype[i];
			any.a_char = *sortl_name;
			add_menu(tmpwin, NO_GLYPH, &any, *sortl_name, 0,
				 ATR_NONE, sortl_name, MENU_UNSELECTED);
		}
		end_menu(tmpwin, "Select loot sorting type:");
		if (select_menu(tmpwin, PICK_ONE, &sortl_pick) > 0) {
			iflags.sortloot = sortl_pick->item.a_char;
			free((genericptr_t)sortl_pick);
		}
		destroy_nhwindow(tmpwin);
		retval = TRUE;
#endif /* SORTLOOT */
    } else if (!strcmp("menu_headings", optname)) {
		static const char *mhchoices[3] = {"bold", "inverse", "underline"};
		const char *npletters = "biu";
		menu_item *mode_pick = (menu_item *)0;

		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (i = 0; i < SIZE(mhchoices); i++) {
			any.a_int = i + 1;
			add_menu(tmpwin, NO_GLYPH, &any, npletters[i], 0,
				 ATR_NONE, mhchoices[i], MENU_UNSELECTED);
			}
		end_menu(tmpwin, "How to highlight menu headings:");
		if (select_menu(tmpwin, PICK_ONE, &mode_pick) > 0) {
			int mode = mode_pick->item.a_int - 1;
			switch(mode) {
				case 2:
					iflags.menu_headings = ATR_ULINE;
					break;
				case 0:
					iflags.menu_headings = ATR_BOLD;
					break;
				case 1:
				default:
					iflags.menu_headings = ATR_INVERSE;
			}
			free((genericptr_t)mode_pick);
			}
		destroy_nhwindow(tmpwin);
			retval = TRUE;
#ifdef AUTOPICKUP_EXCEPTIONS
    } else if (!strcmp("autopickup_exception", optname)) {
    	boolean retval;
	int pick_cnt, pick_idx, opt_idx, pass;
	int totalapes = 0, numapes[2] = {0,0};
	menu_item *pick_list = (menu_item *)0;
	anything any;
	char apebuf[BUFSZ];
	struct autopickup_exception *ape;
	static const char *action_titles[] = {
		"a", "add new autopickup exception",
		"l", "list autopickup exceptions",
		"r", "remove existing autopickup exception",
		"e", "exit this menu",
	};
ape_again:
	opt_idx = 0;
	totalapes = count_ape_maps(&numapes[AP_LEAVE], &numapes[AP_GRAB]);
	tmpwin = create_nhwindow(NHW_MENU);
	start_menu(tmpwin);
	any.a_int = 0;
	for (i = 0; i < SIZE(action_titles) ; i += 2) {
		any.a_int++;
		if (!totalapes && (i >= 2 && i < 6)) continue;
		add_menu(tmpwin, NO_GLYPH, &any, *action_titles[i],
		      0, ATR_NONE, action_titles[i+1], MENU_UNSELECTED);
        }
	end_menu(tmpwin, "Do what?");
	if ((pick_cnt = select_menu(tmpwin, PICK_ONE, &pick_list)) > 0) {
		for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx) {
			opt_idx = pick_list[pick_idx].item.a_int - 1;
		}
		free((genericptr_t)pick_list);
		pick_list = (menu_item *)0;
	}
	destroy_nhwindow(tmpwin);
	if (pick_cnt < 1) return FALSE;

	if (opt_idx == 0) {	/* add new */
		getlin("What new autopickup exception pattern?", &apebuf[1]);
		if (apebuf[1] == '\033') return FALSE;
		apebuf[0] = '"';
		Strcat(apebuf,"\"");
		add_autopickup_exception(apebuf);
		goto ape_again;
	} else if (opt_idx == 3) {
		retval = TRUE;
	} else {	/* remove */
		tmpwin = create_nhwindow(NHW_MENU);
		start_menu(tmpwin);
		for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
		    if (numapes[pass] == 0) continue;
		    ape = iflags.autopickup_exceptions[pass];
		    any.a_void = 0;
		    add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
				(pass == 0) ? "Never pickup" : "Always pickup",
				MENU_UNSELECTED);
		    for (i = 0; i < numapes[pass] && ape; i++) {
			any.a_void = (opt_idx == 1) ? 0 : ape;
			Sprintf(apebuf, "\"%s\"", ape->pattern);
			add_menu(tmpwin, NO_GLYPH, &any,
				0, 0, ATR_NONE, apebuf, MENU_UNSELECTED);
			ape = ape->next;
		    }
		}
		Sprintf(apebuf, "%s autopickup exceptions",
			(opt_idx == 1) ? "List of" : "Remove which");
		end_menu(tmpwin, apebuf);
		pick_cnt = select_menu(tmpwin,
					(opt_idx == 1) ?  PICK_NONE : PICK_ANY,
					&pick_list);
		if (pick_cnt > 0) {
	    	    for (pick_idx = 0; pick_idx < pick_cnt; ++pick_idx)
			remove_autopickup_exception(
			 (struct autopickup_exception *)pick_list[pick_idx].item.a_void);
	        }
	        free((genericptr_t)pick_list);
	        pick_list = (menu_item *)0;
		destroy_nhwindow(tmpwin);
		goto ape_again;
	}
	retval = TRUE;
#endif /* AUTOPICKUP_EXCEPTIONS */
    }
    return retval;
}

#define rolestring(val,array,field) ((val >= 0) ? array[val].field : \
				     (val == ROLE_RANDOM) ? randomrole : none)

/* This is ugly. We have all the option names in the compopt[] array,
   but we need to look at each option individually to get the value. */
STATIC_OVL const char *
get_compopt_value(optname, buf)
const char *optname;
char *buf;
{
	char ocl[MAXOCLASSES+1];
	static const char none[] = "(none)", randomrole[] = "random",
		     to_be_done[] = "(to be done)",
		     defopt[] = "default",
		     defbrief[] = "def";
	int i;

	buf[0] = '\0';
	if (!strcmp(optname,"align_message"))
		Sprintf(buf, "%s", iflags.wc_align_message == ALIGN_TOP     ? "top" :
				   iflags.wc_align_message == ALIGN_LEFT    ? "left" :
				   iflags.wc_align_message == ALIGN_BOTTOM  ? "bottom" :
				   iflags.wc_align_message == ALIGN_RIGHT   ? "right" :
				   defopt);
	else if (!strcmp(optname,"align_status"))
		Sprintf(buf, "%s", iflags.wc_align_status == ALIGN_TOP     ? "top" :
				   iflags.wc_align_status == ALIGN_LEFT    ? "left" :
				   iflags.wc_align_status == ALIGN_BOTTOM  ? "bottom" :
				   iflags.wc_align_status == ALIGN_RIGHT   ? "right" :
				   defopt);
	else if (!strcmp(optname,"align"))
		Sprintf(buf, "%s", rolestring(flags.initalign, aligns, adj));
#ifdef WIN32CON
	else if (!strcmp(optname,"altkeyhandler"))
		Sprintf(buf, "%s", iflags.altkeyhandler[0] ?
			iflags.altkeyhandler : "default");
#endif
	else if (!strcmp(optname, "attack_mode"))
	Sprintf(buf, "%s",
		iflags.attack_mode == ATTACK_MODE_PACIFIST
			? "pacifist"
			: iflags.attack_mode == ATTACK_MODE_CHAT
				? "chat"
				: iflags.attack_mode == ATTACK_MODE_ASK
					? "ask"
					: iflags.attack_mode == ATTACK_MODE_FIGHT_ALL
						? "fight"
						: none);
	else if (!strcmp(optname, "boulder"))
		Sprintf(buf, "%c", iflags.bouldersym ?
			iflags.bouldersym : oc_syms[(int)objects[BOULDER].oc_class]);
	else if (!strcmp(optname, "catname")) 
		Sprintf(buf, "%s", catname[0] ? catname : none );
	else if (!strcmp(optname, "disclose")) {
		for (i = 0; i < NUM_DISCLOSURE_OPTIONS; i++) {
			char topt[2];
			if (i) Strcat(buf," ");
			topt[1] = '\0';
			topt[0] = flags.end_disclose[i];
			Strcat(buf, topt);
			topt[0] = disclosure_options[i];
			Strcat(buf, topt);
		}
	}
	else if (!strcmp(optname, "dogname")) 
		Sprintf(buf, "%s", dogname[0] ? dogname : none );
#ifdef DUMP_LOG
	else if (!strcmp(optname, "dumpfile"))
		Sprintf(buf, "%s", dump_fn[0] ? dump_fn: none );
#endif
	else if (!strcmp(optname, "hp_notify_fmt"))
		Sprintf(buf, "%s", iflags.hp_notify_fmt ? iflags.hp_notify_fmt : "[HP%c%a=%h]" );
	else if (!strcmp(optname, "inherited"))
		Sprintf(buf, "%s", inherited[0] ? inherited : none );
	else if (!strcmp(optname, "dungeon"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "effects"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "font_map"))
		Sprintf(buf, "%s", iflags.wc_font_map ? iflags.wc_font_map : defopt);
	else if (!strcmp(optname, "font_message"))
		Sprintf(buf, "%s", iflags.wc_font_message ? iflags.wc_font_message : defopt);
	else if (!strcmp(optname, "font_status"))
		Sprintf(buf, "%s", iflags.wc_font_status ? iflags.wc_font_status : defopt);
	else if (!strcmp(optname, "font_menu"))
		Sprintf(buf, "%s", iflags.wc_font_menu ? iflags.wc_font_menu : defopt);
	else if (!strcmp(optname, "font_text"))
		Sprintf(buf, "%s", iflags.wc_font_text ? iflags.wc_font_text : defopt);
	else if (!strcmp(optname, "font_size_map")) {
		if (iflags.wc_fontsiz_map) Sprintf(buf, "%d", iflags.wc_fontsiz_map);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_message")) {
		if (iflags.wc_fontsiz_message) Sprintf(buf, "%d",
							iflags.wc_fontsiz_message);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_status")) {
		if (iflags.wc_fontsiz_status) Sprintf(buf, "%d", iflags.wc_fontsiz_status);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_menu")) {
		if (iflags.wc_fontsiz_menu) Sprintf(buf, "%d", iflags.wc_fontsiz_menu);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "font_size_text")) {
		if (iflags.wc_fontsiz_text) Sprintf(buf, "%d",iflags.wc_fontsiz_text);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "fruit")) 
		Sprintf(buf, "%s", pl_fruit);
	else if (!strcmp(optname, "gender"))
		Sprintf(buf, "%s", rolestring(flags.initgend, genders, adj));
	else if (!strcmp(optname, "horsename")) 
		Sprintf(buf, "%s", horsename[0] ? horsename : none);
	else if (!strcmp(optname, "map_mode"))
		Sprintf(buf, "%s",
			iflags.wc_map_mode == MAP_MODE_TILES      ? "tiles" :
			iflags.wc_map_mode == MAP_MODE_ASCII4x6   ? "ascii4x6" :
			iflags.wc_map_mode == MAP_MODE_ASCII6x8   ? "ascii6x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII8x8   ? "ascii8x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII16x8  ? "ascii16x8" :
			iflags.wc_map_mode == MAP_MODE_ASCII7x12  ? "ascii7x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII8x12  ? "ascii8x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII16x12 ? "ascii16x12" :
			iflags.wc_map_mode == MAP_MODE_ASCII12x16 ? "ascii12x16" :
			iflags.wc_map_mode == MAP_MODE_ASCII10x18 ? "ascii10x18" :
			iflags.wc_map_mode == MAP_MODE_ASCII_FIT_TO_SCREEN ?
			"fit_to_screen" : defopt);
	else if (!strcmp(optname, "menustyle")) 
		Sprintf(buf, "%s", menutype[(int)flags.menu_style] );
	else if (!strcmp(optname, "menu_deselect_all"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_deselect_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_first_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_invert_all"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_headings")) {
		Sprintf(buf, "%s", (iflags.menu_headings == ATR_BOLD) ?
			"bold" :   (iflags.menu_headings == ATR_INVERSE) ?
			"inverse" :   (iflags.menu_headings == ATR_ULINE) ?
			"underline" : "unknown");
	}
	else if (!strcmp(optname, "menu_invert_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_last_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_next_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_previous_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_search"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_select_all"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "menu_select_page"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "monsters"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "msghistory"))
		Sprintf(buf, "%u", iflags.msg_history);
#ifdef TTY_GRAPHICS
	else if (!strcmp(optname, "msg_window"))
		Sprintf(buf, "%s", (iflags.prevmsg_window=='s') ? "single" :
					(iflags.prevmsg_window=='c') ? "combination" :
					(iflags.prevmsg_window=='f') ? "full" : "reversed");
#endif
	else if (!strcmp(optname, "name"))
		Sprintf(buf, "%s", plname);
	else if (!strcmp(optname, "number_pad"))
		Sprintf(buf, "%s",
			(!iflags.num_pad) ? "0=off" :
			(iflags.num_pad_mode) ? "2=on, DOS compatible" : "1=on");
	else if (!strcmp(optname, "objects"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "packorder")) {
		oc_to_str(flags.inv_order, ocl);
		Sprintf(buf, "%s", ocl);
	     }
#ifdef CHANGE_COLOR
	else if (!strcmp(optname, "palette")) 
		Sprintf(buf, "%s", get_color_string());
#endif
	else if (!strcmp(optname, "pettype")) 
		Sprintf(buf, "%s", (preferred_pet == 'c') ? "cat" :
				(preferred_pet == 'd') ? "dog" :
				(preferred_pet == 'n') ? "none" : "random");
	else if (!strcmp(optname, "pokedex"))
		Sprintf(buf, "%d", iflags.pokedex);
	else if (!strcmp(optname, "pickup_burden"))
		Sprintf(buf, "%s", burdentype[flags.pickup_burden] );
	else if (!strcmp(optname, "pickup_types")) {
		oc_to_str(flags.pickup_types, ocl);
		Sprintf(buf, "%s", ocl[0] ? ocl : "all" );
	     }
	else if (!strcmp(optname, "race"))
		Sprintf(buf, "%s", rolestring(flags.initrace, races, noun));
#ifdef CONVICT
	else if (!strcmp(optname, "ratname")) 
		Sprintf(buf, "%s", ratname[0] ? catname : none );
#endif /* CONVICT */
	else if (!strcmp(optname, "role"))
		Sprintf(buf, "%s", rolestring(flags.initrole, roles, name.m));
	else if (!strcmp(optname, "runmode"))
		Sprintf(buf, "%s", runmodes[iflags.runmode]);
	else if (!strcmp(optname, "delay_length"))
		Sprintf(buf, "%s", delay_lengths[iflags.delay_length]);
	else if (!strcmp(optname, "scores")) {
		Sprintf(buf, "%d top/%d around%s", flags.end_top,
				flags.end_around, flags.end_own ? "/own" : "");
	}
	else if (!strcmp(optname, "scroll_amount")) {
		if (iflags.wc_scroll_amount) Sprintf(buf, "%d",iflags.wc_scroll_amount);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "scroll_margin")) {
		if (iflags.wc_scroll_margin) Sprintf(buf, "%d",iflags.wc_scroll_margin);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname,"species"))
		Sprintf(buf, "%s", rolestring(flags.initspecies, species, name));
#ifdef SORTLOOT
	else if (!strcmp(optname, "sortloot")) {
		char *sortname = (char *)NULL;
		for (i=0; i < SIZE(sortltype) && sortname==(char *)NULL; i++) {
		   if (iflags.sortloot == sortltype[i][0])
		     sortname = (char *)sortltype[i];
		}
		if (sortname != (char *)NULL)
		   Sprintf(buf, "%s", sortname);
	}
#endif /* SORTLOOT */
	else if (!strcmp(optname, "player_selection"))
		Sprintf(buf, "%s", iflags.wc_player_selection ? "prompts" : "dialog");
#ifdef MSDOS
	else if (!strcmp(optname, "soundcard")) {
		Sprintf(buf, "%s", to_be_done);
	}
#endif
	else if (!strcmp(optname, "statuseffects"))
		Sprintf(buf, "0x%llX", iflags.statuseffects);
	else if (!strcmp(optname, "statuslines"))
		Sprintf(buf, "%d", iflags.statuslines);
	else if (!strcmp(optname, "suppress_alert")) {
	    if (flags.suppress_alert == 0L)
		Strcpy(buf, none);
	    else
		Sprintf(buf, "%lu.%lu.%lu",
			FEATURE_NOTICE_VER_MAJ,
			FEATURE_NOTICE_VER_MIN,
			FEATURE_NOTICE_VER_PATCH);
	}
	else if (!strcmp(optname, "term_cols")) {
		if (iflags.wc2_term_cols) Sprintf(buf, "%d",iflags.wc2_term_cols);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "term_rows")) {
		if (iflags.wc2_term_rows) Sprintf(buf, "%d",iflags.wc2_term_rows);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "tile_file"))
		Sprintf(buf, "%s", iflags.wc_tile_file ? iflags.wc_tile_file : defopt);
	else if (!strcmp(optname, "tile_height")) {
		if (iflags.wc_tile_height) Sprintf(buf, "%d",iflags.wc_tile_height);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "tile_width")) {
		if (iflags.wc_tile_width) Sprintf(buf, "%d",iflags.wc_tile_width);
		else Strcpy(buf, defopt);
	}
	else if (!strcmp(optname, "traps"))
		Sprintf(buf, "%s", to_be_done);
	else if (!strcmp(optname, "travelplus"))
		Sprintf(buf, "%d", iflags.travelplus);
	else if (!strcmp(optname, "vary_msgcount")) {
		if (iflags.wc_vary_msgcount) Sprintf(buf, "%d",iflags.wc_vary_msgcount);
		else Strcpy(buf, defopt);
	}
#ifdef MSDOS
	else if (!strcmp(optname, "video"))
		Sprintf(buf, "%s", to_be_done);
#endif
#ifdef VIDEOSHADES
	else if (!strcmp(optname, "videoshades"))
		Sprintf(buf, "%s-%s-%s", shade[0],shade[1],shade[2]);
	else if (!strcmp(optname, "videocolors"))
		Sprintf(buf, "%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d-%d",
			ttycolors[CLR_RED], ttycolors[CLR_GREEN],
			ttycolors[CLR_BROWN], ttycolors[CLR_BLUE],
			ttycolors[CLR_MAGENTA], ttycolors[CLR_CYAN],
			ttycolors[CLR_ORANGE], ttycolors[CLR_BRIGHT_GREEN],
			ttycolors[CLR_YELLOW], ttycolors[CLR_BRIGHT_BLUE],
			ttycolors[CLR_BRIGHT_MAGENTA],
			ttycolors[CLR_BRIGHT_CYAN]);
#endif /* VIDEOSHADES */
	else if (!strcmp(optname,"windowborders"))
		Sprintf(buf, "%s", iflags.wc2_windowborders == 1     ? "1=on" :
				   iflags.wc2_windowborders == 2             ? "2=off" :
				   iflags.wc2_windowborders == 3             ? "3=auto" :
				   defopt);
	else if (!strcmp(optname, "windowtype"))
		Sprintf(buf, "%s", windowprocs.name);
	else if (!strcmp(optname, "windowcolors"))
		Sprintf(buf, "%s/%s %s/%s %s/%s %s/%s",
			iflags.wc_foregrnd_menu    ? iflags.wc_foregrnd_menu : defbrief,
			iflags.wc_backgrnd_menu    ? iflags.wc_backgrnd_menu : defbrief,
			iflags.wc_foregrnd_message ? iflags.wc_foregrnd_message : defbrief,
			iflags.wc_backgrnd_message ? iflags.wc_backgrnd_message : defbrief,
			iflags.wc_foregrnd_status  ? iflags.wc_foregrnd_status : defbrief,
			iflags.wc_backgrnd_status  ? iflags.wc_backgrnd_status : defbrief,
			iflags.wc_foregrnd_text    ? iflags.wc_foregrnd_text : defbrief,
			iflags.wc_backgrnd_text    ? iflags.wc_backgrnd_text : defbrief);
	else if (!strcmp(optname, "wizlevelport"))
		Sprintf(buf, "%d", iflags.wizlevelport);
	else if (!strcmp(optname, "wizcombatdebug"))
		Sprintf(buf, "%d", iflags.wizcombatdebug);
#ifdef PREFIXES_IN_USE
	else {
	    for (i = 0; i < PREFIX_COUNT; ++i)
		if (!strcmp(optname, fqn_prefix_names[i]) && fqn_prefix[i])
			Sprintf(buf, "%s", fqn_prefix[i]);
	}
#endif

	if (buf[0]) return buf;
	else return "unknown";
}

int
dotogglepickup()
{
	char buf[BUFSZ], ocl[MAXOCLASSES+1];

	flags.pickup = !flags.pickup;
	if (flags.pickup) {
	    oc_to_str(flags.pickup_types, ocl);
	    Sprintf(buf, "ON, for %s objects%s", ocl[0] ? ocl : "all",
#ifdef AUTOPICKUP_EXCEPTIONS
			(iflags.autopickup_exceptions[AP_LEAVE] ||
			 iflags.autopickup_exceptions[AP_GRAB]) ?
			 ((count_ape_maps((int *)0, (int *)0) == 1) ?
			    ", with one exception" : ", with some exceptions") :
#endif
			"");
	} else {
	    Strcpy(buf, "OFF");
	}
	pline("Autopickup: %s.", buf);
	return MOVE_CANCELLED;
}

#ifdef AUTOPICKUP_EXCEPTIONS
int
add_autopickup_exception(mapping)
const char *mapping;
{
	struct autopickup_exception *ape, **apehead;
	char text[256], *text2;
	int textsize = 0;
	boolean grab = FALSE;

	if (sscanf(mapping, "\"%255[^\"]\"", text) == 1) {
		text2 = &text[0];
		if (*text2 == '<') {		/* force autopickup */
			grab = TRUE;
			++text2;
		} else if (*text2 == '>') {	/* default - Do not pickup */
			grab = FALSE;
			++text2;
		}
		textsize = strlen(text2);
		apehead = (grab) ? &iflags.autopickup_exceptions[AP_GRAB] :
				   &iflags.autopickup_exceptions[AP_LEAVE];
		ape = (struct autopickup_exception *)
				alloc(sizeof(struct autopickup_exception));
		ape->pattern = (char *) alloc(textsize+1);
		Strcpy(ape->pattern, text2);
		ape->is_regexp = iflags.ape_regex;
		if (iflags.ape_regex) {
		    int errnum;
		    char errbuf[80];
		    const char *err = (char *)0;
		    errnum = regcomp(&ape->match, text2, REG_EXTENDED | REG_NOSUB);
		    if (errnum != 0) {
			regerror(errnum, &ape->match, errbuf, sizeof(errbuf));
			err = errbuf;
		    }
		    if (err) {
			raw_printf("\nAUTOPICKUP_EXCEPTION regex error: %s\n", err);
			wait_synch();
			free(ape);
			return 0;
		    }
		}
		ape->grab = grab;
		if (!*apehead) ape->next = (struct autopickup_exception *)0;
		else ape->next = *apehead;
		*apehead = ape;
	} else {
	    raw_print("syntax error in AUTOPICKUP_EXCEPTION");
	    return 0;
	}
	return 1;
}

STATIC_OVL void
remove_autopickup_exception(whichape)
struct autopickup_exception *whichape;
{
    struct autopickup_exception *ape, *prev = 0;
    int chain = whichape->grab ? AP_GRAB : AP_LEAVE;

    for (ape = iflags.autopickup_exceptions[chain]; ape;) {
	if (ape == whichape) {
	    struct autopickup_exception *freeape = ape;
	    ape = ape->next;
	    if (prev) prev->next = ape;
	    else iflags.autopickup_exceptions[chain] = ape;
	    free(freeape->pattern);
	    if (freeape->is_regexp) {
		(void) regfree(&freeape->match);
	    }
	    free(freeape);
	} else {
	    prev = ape;
	    ape = ape->next;
	}
    }
}

STATIC_OVL int
count_ape_maps(leave, grab)
int *leave, *grab;
{
	struct autopickup_exception *ape;
	int pass, totalapes, numapes[2] = {0,0};

	for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
		ape = iflags.autopickup_exceptions[pass];
		while(ape) {
			ape = ape->next;
			numapes[pass]++;
		}
	}
	totalapes = numapes[AP_LEAVE] + numapes[AP_GRAB];
	if (leave) *leave = numapes[AP_LEAVE];
	if (grab) *grab = numapes[AP_GRAB];
	return totalapes;
}

void
free_autopickup_exceptions()
{
	struct autopickup_exception *ape;
	int pass;

	for (pass = AP_LEAVE; pass <= AP_GRAB; ++pass) {
		while((ape = iflags.autopickup_exceptions[pass]) != 0) {
		    free(ape->pattern);
		    if (ape->is_regexp) {
			(void) regfree(&ape->match);
		    }
			iflags.autopickup_exceptions[pass] = ape->next;
			free(ape);
		}
	}
}
#endif /* AUTOPICKUP_EXCEPTIONS */

/* data for option_help() */
static const char *opt_intro[] = {
	"",
	"                 NetHack Options Help:",
	"",
#define CONFIG_SLOT 3	/* fill in next value at run-time */
	(char *)0,
#if !defined(MICRO) && !defined(MAC)
	"or use `NETHACKOPTIONS=\"<options>\"' in your environment",
#endif
	"(<options> is a list of options separated by commas)",
#ifdef VMS
	"-- for example, $ DEFINE NETHACKOPTIONS \"noautopickup,fruit:kumquat\"",
#endif
	"or press \"O\" while playing and use the menu.",
	"",
 "Boolean options (which can be negated by prefixing them with '!' or \"no\"):",
	(char *)0
};

static const char *opt_epilog[] = {
	"",
 "Some of the options can be set only before the game is started; those",
	"items will not be selectable in the 'O' command's menu.",
	(char *)0
};

void
option_help()
{
    char buf[BUFSZ], buf2[BUFSZ];
    register int i;
    winid datawin;

    datawin = create_nhwindow(NHW_TEXT);
    Sprintf(buf, "Set options as OPTIONS=<options> in %s", configfile);
    opt_intro[CONFIG_SLOT] = (const char *) buf;
    for (i = 0; opt_intro[i]; i++)
	putstr(datawin, 0, opt_intro[i]);

    /* Boolean options */
    for (i = 0; boolopt[i].name; i++) {
	if (boolopt[i].addr) {
#ifdef WIZARD
	    if (boolopt[i].addr == &iflags.sanity_check && !wizard) continue;
	    if (boolopt[i].addr == &iflags.menu_tab_sep && !wizard) continue;
#endif
	    next_opt(datawin, boolopt[i].name);
	}
    }
    next_opt(datawin, "");

    /* Compound options */
    putstr(datawin, 0, "Compound options:");
    for (i = 0; compopt[i].name; i++) {
	Sprintf(buf2, "`%s'", compopt[i].name);
	Sprintf(buf, "%-20s - %s%c", buf2, compopt[i].descr,
		compopt[i+1].name ? ',' : '.');
	putstr(datawin, 0, buf);
    }

    for (i = 0; opt_epilog[i]; i++)
	putstr(datawin, 0, opt_epilog[i]);

    display_nhwindow(datawin, FALSE);
    destroy_nhwindow(datawin);
    return;
}

/*
 * prints the next boolean option, on the same line if possible, on a new
 * line if not. End with next_opt("").
 */
void
next_opt(datawin, str)
winid datawin;
const char *str;
{
	static char *buf = 0;
	int i;
	char *s;

	if (!buf) *(buf = (char *)alloc(BUFSZ)) = '\0';

	if (!*str) {
		s = eos(buf);
		if (s > &buf[1] && s[-2] == ',')
		    Strcpy(s - 2, ".");	/* replace last ", " */
		i = COLNO;	/* (greater than COLNO - 2) */
	} else {
		i = strlen(buf) + strlen(str) + 2;
	}

	if (i > COLNO - 2) { /* rule of thumb */
		putstr(datawin, 0, buf);
		buf[0] = 0;
	}
	if (*str) {
		Strcat(buf, str);
		Strcat(buf, ", ");
	} else {
		putstr(datawin, 0, str);
		free(buf),  buf = 0;
	}
	return;
}

/* Returns the fid of the fruit type; if that type already exists, it
 * returns the fid of that one; if it does not exist, it adds a new fruit
 * type to the chain and returns the new one.
 */
int
fruitadd(str)
char *str;
{
	register int i;
	register struct fruit *f;
	struct fruit *lastf = 0;
	int highest_fruit_id = 0;
	char buf[PL_FSIZ];
	boolean user_specified = (str == pl_fruit);
	/* if not user-specified, then it's a fruit name for a fruit on
	 * a bones level...
	 */

	/* Note: every fruit has an id (spe for fruit objects) of at least
	 * 1; 0 is an error.
	 */
	if (user_specified) {
		/* disallow naming after other foods (since it'd be impossible
		 * to tell the difference)
		 */

		boolean found = FALSE, numeric = FALSE;

		for (i = bases[FOOD_CLASS]; objects[i].oc_class == FOOD_CLASS;
						i++) {
			if (!strcmp(OBJ_NAME(objects[i]), pl_fruit)) {
				found = TRUE;
				break;
			}
		}
		{
		    char *c;

		    c = pl_fruit;

		    for(c = pl_fruit; *c >= '0' && *c <= '9'; c++)
			;
		    if (isspace(*c) || *c == 0) numeric = TRUE;
		}
		{
		    int len = strlen(str);
		    if (found || numeric ||
			    !strncmp(str, "cursed ", 7) ||
			    !strncmp(str, "uncursed ", 9) ||
			    !strncmp(str, "blessed ", 8) ||
			    !strncmp(str, "partly eaten ", 13) ||
			    (!strncmp(str, "tin of ", 7) &&
			     (!strcmp(str+7, "spinach") ||
			      name_to_mon(str+7) >= LOW_PM)) ||
			    !strcmp(str, "empty tin") ||
			    (
			     ((len >= 8 && !strncmp(eos(str)-7," corpse",7)) ||
			      (len >= 5 && !strncmp(eos(str)-4, " egg",4)))  &&
			     name_to_mon(str) >= LOW_PM
			    )
		       )
		    {
			Strcpy(buf, pl_fruit);
			Strcpy(pl_fruit, "candied ");
			nmcpy(pl_fruit+8, buf, PL_FSIZ-8);
		    }
		}
	}
	for(f=ffruit; f; f = f->nextf) {
		lastf = f;
		if(f->fid > highest_fruit_id) highest_fruit_id = f->fid;
		if(!strncmp(str, f->fname, PL_FSIZ))
			goto nonew;
	}
	/* if adding another fruit would overflow spe, use a random
	   fruit instead... we've got a lot to choose from. */
	if (highest_fruit_id >= 127) return rnd(127);
	highest_fruit_id++;
	f = newfruit();
	if (ffruit) lastf->nextf = f;
	else ffruit = f;
	Strcpy(f->fname, str);
	f->fid = highest_fruit_id;
	f->nextf = 0;
nonew:
	if (user_specified) current_fruit = highest_fruit_id;
	return f->fid;
}

/*
 * This is a somewhat generic menu for taking a list of NetHack style
 * class choices and presenting them via a description
 * rather than the traditional NetHack characters.
 * (Benefits users whose first exposure to NetHack is via tiles).
 *
 * prompt
 *	     The title at the top of the menu.
 *
 * category: 0 = monster class
 *           1 = object  class
 *
 * way
 *	     FALSE = PICK_ONE, TRUE = PICK_ANY
 *
 * class_list
 *	     a null terminated string containing the list of choices.
 *
 * class_selection
 *	     a null terminated string containing the selected characters.
 *
 * Returns number selected.
 */
int
choose_classes_menu(prompt, category, way, class_list, class_select)
const char *prompt;
int category;
boolean way;
char *class_list;
char *class_select;
{
    menu_item *pick_list = (menu_item *)0;
    winid win;
    anything any;
    char buf[BUFSZ];
    int i, n;
    int ret;
    int next_accelerator, accelerator;

    if (class_list == (char *)0 || class_select == (char *)0) return 0;
    accelerator = 0;
    next_accelerator = 'a';
    any.a_void = 0;
    win = create_nhwindow(NHW_MENU);
    start_menu(win);
    while (*class_list) {
	const char *text;
	boolean selected;

	text = (char *)0;
	selected = FALSE;
	switch (category) {
		case 0:
			text = monexplain[def_char_to_monclass(*class_list)];
			accelerator = *class_list;
			Sprintf(buf, "%s", text);
			break;
		case 1:
			text = objexplain[def_char_to_objclass(*class_list)];
			accelerator = next_accelerator;
			Sprintf(buf, "%c  %s", *class_list, text);
			break;
		default:
			impossible("choose_classes_menu: invalid category %d",
					category);
	}
	if (way && *class_select) {	/* Selections there already */
		if (index(class_select, *class_list)) {
			selected = TRUE;
		}
	}
	any.a_int = *class_list;
	add_menu(win, NO_GLYPH, &any, accelerator,
		  category ? *class_list : 0,
		  ATR_NONE, buf, selected);
	++class_list;
	if (category > 0) {
		++next_accelerator;
		if (next_accelerator == ('z' + 1)) next_accelerator = 'A';
		if (next_accelerator == ('Z' + 1)) break;
	}
    }
    end_menu(win, prompt);
    n = select_menu(win, way ? PICK_ANY : PICK_ONE, &pick_list);
    destroy_nhwindow(win);
    if (n > 0) {
	for (i = 0; i < n; ++i)
	    *class_select++ = (char)pick_list[i].item.a_int;
	free((genericptr_t)pick_list);
	ret = n;
    } else if (n == -1) {
	class_select = eos(class_select);
	ret = -1;
    } else
	ret = 0;
    *class_select = '\0';
    return ret;
}

struct wc_Opt wc_options[] = {
	{"ascii_map", WC_ASCII_MAP},
	{"color", WC_COLOR},
	{"eight_bit_tty", WC_EIGHT_BIT_IN},
	{"hilite_pet", WC_HILITE_PET},
	{"popup_dialog", WC_POPUP_DIALOG},
	{"player_selection", WC_PLAYER_SELECTION},
	{"preload_tiles", WC_PRELOAD_TILES},
	{"tiled_map", WC_TILED_MAP},
	{"tile_file", WC_TILE_FILE},
	{"tile_width", WC_TILE_WIDTH},
	{"tile_height", WC_TILE_HEIGHT},
	{"use_inverse", WC_INVERSE},
	{"align_message", WC_ALIGN_MESSAGE},
	{"align_status", WC_ALIGN_STATUS},
	{"font_map", WC_FONT_MAP},
	{"font_menu", WC_FONT_MENU},
	{"font_message",WC_FONT_MESSAGE},
#if 0
	{"perm_invent",WC_PERM_INVENT},
#endif
	{"font_size_map", WC_FONTSIZ_MAP},
	{"font_size_menu", WC_FONTSIZ_MENU},
	{"font_size_message", WC_FONTSIZ_MESSAGE},
	{"font_size_status", WC_FONTSIZ_STATUS},
	{"font_size_text", WC_FONTSIZ_TEXT},
	{"font_status", WC_FONT_STATUS},
	{"font_text", WC_FONT_TEXT},
	{"map_mode", WC_MAP_MODE},
	{"scroll_amount", WC_SCROLL_AMOUNT},
	{"scroll_margin", WC_SCROLL_MARGIN},
	{"splash_screen", WC_SPLASH_SCREEN},
	{"vary_msgcount",WC_VARY_MSGCOUNT},
	{"windowcolors", WC_WINDOWCOLORS},
	{"mouse_support", WC_MOUSE_SUPPORT},
	{(char *)0, 0L}
};

struct wc_Opt wc2_options[] = {
	{"fullscreen", WC2_FULLSCREEN},
	{"softkeyboard", WC2_SOFTKEYBOARD},
	{"wraptext", WC2_WRAPTEXT},
	{"term_cols", WC2_TERM_COLS},
	{"term_rows", WC2_TERM_ROWS},
	{"windowborders", WC2_WINDOWBORDERS},
	{"petattr", WC2_PETATTR},
	{"guicolor", WC2_GUICOLOR},
	{"use_darkgray", WC2_DARKGRAY},
	{(char *)0, 0L}
};


/*
 * If a port wants to change or ensure that the
 * SET_IN_FILE, DISP_IN_GAME, or SET_IN_GAME status of an option is
 * correct (for controlling its display in the option menu) call
 * set_option_mod_status()
 * with the second argument of 0,2, or 3 respectively.
 */
void
set_option_mod_status(optnam, status)
const char *optnam;
int status;
{
	int k;
	if (status < SET_IN_FILE || status > SET_IN_GAME) {
		impossible("set_option_mod_status: status out of range %d.",
			   status);
		return;
	}
	for (k = 0; boolopt[k].name; k++) {
		if (!strncmpi(boolopt[k].name, optnam, strlen(optnam))) {
			boolopt[k].optflags = status;
			return;
		}
	}
	for (k = 0; compopt[k].name; k++) {
		if (!strncmpi(compopt[k].name, optnam, strlen(optnam))) {
			compopt[k].optflags = status;
			return;
		}
	}
}

/*
 * You can set several wc_options in one call to
 * set_wc_option_mod_status() by setting
 * the appropriate bits for each option that you
 * are setting in the optmask argument
 * prior to calling.
 *    example: set_wc_option_mod_status(WC_COLOR|WC_SCROLL_MARGIN, SET_IN_GAME);
 */
void
set_wc_option_mod_status(optmask, status)
unsigned long optmask;
int status;
{
	int k = 0;
	if (status < SET_IN_FILE || status > SET_IN_GAME) {
		impossible("set_wc_option_mod_status: status out of range %d.",
			   status);
		return;
	}
	while (wc_options[k].wc_name) {
		if (optmask & wc_options[k].wc_bit) {
			set_option_mod_status(wc_options[k].wc_name, status);
		}
		k++;
	}
}

STATIC_OVL boolean
is_wc_option(optnam)
const char *optnam;
{
	int k = 0;
	while (wc_options[k].wc_name) {
		if (strcmp(wc_options[k].wc_name, optnam) == 0)
			return TRUE;
		k++;
	}
	return FALSE;
}

STATIC_OVL boolean
wc_supported(optnam)
const char *optnam;
{
	int k = 0;
	while (wc_options[k].wc_name) {
		if (!strcmp(wc_options[k].wc_name, optnam) &&
		    (windowprocs.wincap & wc_options[k].wc_bit))
			return TRUE;
		k++;
	}
	return FALSE;
}


/*
 * You can set several wc2_options in one call to
 * set_wc2_option_mod_status() by setting
 * the appropriate bits for each option that you
 * are setting in the optmask argument
 * prior to calling.
 *    example: set_wc2_option_mod_status(WC2_FULLSCREEN|WC2_SOFTKEYBOARD|WC2_WRAPTEXT, SET_IN_FILE);
 */

void
set_wc2_option_mod_status(optmask, status)
unsigned long optmask;
int status;
{
	int k = 0;
	if (status < SET_IN_FILE || status > SET_IN_GAME) {
		impossible("set_wc2_option_mod_status: status out of range %d.",
			   status);
		return;
	}
	while (wc2_options[k].wc_name) {
		if (optmask & wc2_options[k].wc_bit) {
			set_option_mod_status(wc2_options[k].wc_name, status);
		}
		k++;
	}
}

STATIC_OVL boolean
is_wc2_option(optnam)
const char *optnam;
{
	int k = 0;
	while (wc2_options[k].wc_name) {
		if (strcmp(wc2_options[k].wc_name, optnam) == 0)
			return TRUE;
		k++;
	}
	return FALSE;
}

STATIC_OVL boolean
wc2_supported(optnam)
const char *optnam;
{
	int k = 0;
	while (wc2_options[k].wc_name) {
		if (!strcmp(wc2_options[k].wc_name, optnam) &&
		    (windowprocs.wincap2 & wc2_options[k].wc_bit))
			return TRUE;
		k++;
	}
	return FALSE;
}


STATIC_OVL void
wc_set_font_name(wtype, fontname)
int wtype;
char *fontname;
{
	char **fn = (char **)0;
	if (!fontname) return;
	switch(wtype) {
	    case NHW_MAP:
	    		fn = &iflags.wc_font_map;
			break;
	    case NHW_MESSAGE:
	    		fn = &iflags.wc_font_message;
			break;
	    case NHW_TEXT:
	    		fn = &iflags.wc_font_text;
			break;
	    case NHW_MENU:
	    		fn = &iflags.wc_font_menu;
			break;
	    case NHW_STATUS:
	    		fn = &iflags.wc_font_status;
			break;
	    default:
	    		return;
	}
	if (fn) {
		if (*fn) free(*fn);
		*fn = (char *)alloc(strlen(fontname) + 1);
		Strcpy(*fn, fontname);
	}
	return;
}

STATIC_OVL int
wc_set_window_colors(op)
char *op;
{
	/* syntax:
	 *  menu white/black message green/yellow status white/blue text white/black
	 */

	int j;
	char buf[BUFSZ];
	char *wn, *tfg, *tbg, *newop;
	static const char *wnames[] = { "menu", "message", "status", "text" };
	static const char *shortnames[] = { "mnu", "msg", "sts", "txt" };
	static char **fgp[] = {
		&iflags.wc_foregrnd_menu,
		&iflags.wc_foregrnd_message,
		&iflags.wc_foregrnd_status,
		&iflags.wc_foregrnd_text
	};
	static char **bgp[] = {
		&iflags.wc_backgrnd_menu,
		&iflags.wc_backgrnd_message,
		&iflags.wc_backgrnd_status,
		&iflags.wc_backgrnd_text
	};

	Strcpy(buf, op);
	newop = mungspaces(buf);
	while (newop && *newop) {

		wn = tfg = tbg = (char *)0;

		/* until first non-space in case there's leading spaces - before colorname*/
		while(*newop && isspace(*newop)) newop++;
		if (*newop) wn = newop;
		else return 0;

		/* until first space - colorname*/
		while(*newop && !isspace(*newop)) newop++;
		if (*newop) *newop = '\0';
		else return 0;
		newop++;

		/* until first non-space - before foreground*/
		while(*newop && isspace(*newop)) newop++;
		if (*newop) tfg = newop;
		else return 0;

		/* until slash - foreground */
		while(*newop && *newop != '/') newop++;
		if (*newop) *newop = '\0';
		else return 0;
		newop++;

		/* until first non-space (in case there's leading space after slash) - before background */
		while(*newop && isspace(*newop)) newop++;
		if (*newop) tbg = newop;
		else return 0;

		/* until first space - background */
		while(*newop && !isspace(*newop)) newop++;
		if (*newop) *newop++ = '\0';

		for (j = 0; j < 4; ++j) {
			if (!strcmpi(wn, wnames[j]) ||
			    !strcmpi(wn, shortnames[j])) {
				if (tfg && !strstri(tfg, " ")) {
					if (*fgp[j]) free(*fgp[j]);
					*fgp[j] = (char *)alloc(strlen(tfg) + 1);
					Strcpy(*fgp[j], tfg);
				}
				if (tbg && !strstri(tbg, " ")) {
					if (*bgp[j]) free(*bgp[j]);
					*bgp[j] = (char *)alloc(strlen(tbg) + 1);
					Strcpy(*bgp[j], tbg);
				}
 				break;
			}
		}
	}
	return 1;
}

#endif	/* OPTION_LISTS_ONLY */

/*options.c*/
