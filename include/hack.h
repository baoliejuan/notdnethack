/*	SCCS Id: @(#)hack.h	3.4	2001/04/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef HACK_H
#define HACK_H

#ifndef CONFIG_H
#include "config.h"
#endif

/*	For debugging beta code.	*/
#ifdef BETA
#define Dpline	pline
#endif

#define TELL		1
#define NOTELL		0
#define ON		1
#define OFF		0
#define BOLT_LIM	8 /* from this distance ranged attacks will be made */
#define MAX_CARR_CAP	1000	/* so that boulders can be heavier */
#define DUMMY { 0 }

/* number of times a spellbook can be read before blanking */
#define MAX_SPELL_STUDY 3

/* Length of a healing cycle */
#define HEALCYCLE 90

/* symbolic names for capacity levels */
#define UNENCUMBERED	0
#define SLT_ENCUMBER	1	/* Burdened */
#define MOD_ENCUMBER	2	/* Stressed */
#define HVY_ENCUMBER	3	/* Strained */
#define EXT_ENCUMBER	4	/* Overtaxed */
#define OVERLOADED	5	/* Overloaded */

/* hunger texts used on bottom line (each 8 chars long) */
#define SATIATED	0
#define NOT_HUNGRY	1
#define HUNGRY		2
#define WEAK		3
#define FAINTING	4
#define FAINTED		5
#define STARVED		6

/* Macros for how a rumor was delivered in outrumor() */
#define BY_ORACLE	0
#define BY_COOKIE	1
#define BY_PAPER	2
#define BY_OTHER	9

#ifdef STEED
/* Macros for why you are no longer riding */
#define DISMOUNT_GENERIC	0
#define DISMOUNT_FELL		1
#define DISMOUNT_THROWN		2
#define DISMOUNT_POLY		3
#define DISMOUNT_ENGULFED	4
#define DISMOUNT_BONES		5
#define DISMOUNT_BYCHOICE	6
#define DISMOUNT_VANISHED	7
#endif

/* Special returns from mapglyph() (note: type is unsigned int) */
#define MG_CORPSE	0x001
#define MG_INVIS	0x002
#define MG_DETECT	0x004
#define MG_PET		0x008
#define MG_RIDDEN	0x010
#define MG_STAIRS	0x020
#define MG_OBJPILE	0x040
#define MG_ZOMBIE	0x080
#define MG_PEACE	0x100
#define MG_PORTAL	0x200

/* sellobj_state() states */
#define SELL_NORMAL	(0)
#define SELL_DELIBERATE	(1)
#define SELL_DONTSELL	(2)

/*
 * This is the way the game ends.  If these are rearranged, the arrays
 * in end.c and topten.c will need to be changed.  Some parts of the
 * code assume that PANIC separates the deaths from the non-deaths.
 */
#define DIED		 0
#define BETRAYED	 1
#define CHOKING		 2
#define POISONING	 3
#define STARVING	 4
#define DROWNING	 5
#define BURNING		 6
#define DISSOLVED	 7
#define CRUSHING	 8
#define STONING		 9
#define GOLDING		10
#define SALTING		11
#define GLASSED		12
#define TURNED_SLIME	13
#define OVERWOUND 	14
#define WEEPING 	15
#define DISINTEGRATED 16
#define GENOCIDED	17 //Life saving triggers here and below, including wizard lifesaving
#define APOCALYPSE	18
#define PANICKED	19 //Below this, umortality is incremented
#define TRICKED		20
#define QUIT		21
#define ESCAPED		22
#define ASCENDED	23

#include "align.h"
#include "dungeon.h"
#include "monsym.h"
#include "mkroom.h"
#include "objclass.h"
#include "youprop.h"
#include "wintype.h"
#include "decl.h"
#include "mextra.h"
#include "oextra.h"
#include "timeout.h"
#include "zap.h"
#include "thoughtglyph.h"

NEARDATA extern coord bhitpos;	/* place where throw or zap hits or stops */

/* types of calls to bhit() */
#define ZAPPED_WAND		0	/* invisible ray, pierces monsters */
#define THROWN_WEAPON	1	/* hopefully deprecated */
#define KICKED_WEAPON	2	/* hopefully deprecated */
#define FLASHED_LIGHT	3	/* visible flashed light, stops on first mon hit */
#define INVIS_BEAM		4	/* invisible ray from a mirror, stops on first mon hit */
#define TRIGGER_BEAM	5	/* invisible ray, stops on first thing that causes associated functions to return nonzero */

/* types of calls to hmon */
#define HMON_WHACK		0x01	/* regular melee attack */
#define HMON_THRUST		0x02	/* polearm thrust */
#define HMON_PROJECTILE	0x04	/* projectile, general */
#define HMON_FIRED		0x08	/* projectile had a proper launcher (or simulation thereof), for projectiles that care */
#define HMON_TRAP		0x10	/* trap-owned attack, either projectile or melee depending on ttyp */
#define HMON_KICKED		0x20	/* object is a kicked projectile */

#define beastMateryRadius(mon)	(P_SKILL(P_BEAST_MASTERY) > P_ISRESTRICTED && distmin(u.ux, u.uy, mon->mx, mon->my) <= (P_SKILL(P_BEAST_MASTERY) - P_ISRESTRICTED))


#define MATCH_WARN_OF_MON(mon)	( MATCH_WARN_OF_MON_STRICT(mon) || \
					(u.sealsActive&SEAL_PAIMON && is_magical((mon)->data)) || \
					(u.sealsActive&SEAL_ANDROMALIUS && is_thief((mon)->data)) || \
					(u.sealsActive&SEAL_TENEBROUS && !nonliving(mon->data)) || \
					(mon->mtame && beastMateryRadius(mon)) || \
					(mon->mtyp == PM_TWIN_SIBLING) || \
					(Upolyd && youmonst.data->mtyp == PM_SHARK && has_blood((mon)->data) && \
						(mon)->mhp < (mon)->mhpmax && is_pool(u.ux, u.uy, TRUE) && is_pool((mon)->mx, (mon)->my, TRUE)) || \
					(u.specialSealsActive&SEAL_ACERERAK && is_undead(mon->data)) || \
					(uwep && uwep->oclass == WEAPON_CLASS && (uwep)->obj_material == WOOD && uwep->otyp != MOON_AXE &&\
					 (uwep->oward & WARD_THJOFASTAFUR) && ((mon)->data->mlet == S_LEPRECHAUN || (mon)->data->mlet == S_NYMPH || is_thief((mon)->data))) \
				)
#define MATCH_WARN_OF_MON_STRICT(mon)	( (Warn_of_mon && flags.warntypem && \
						(flags.warntypem & (mon)->data->mflagsm)) || \
					(Warn_of_mon && flags.warntypet && \
						(flags.warntypet & (mon)->data->mflagst)) || \
					(Warn_of_mon && flags.warntypeb && \
						(flags.warntypeb & (mon)->data->mflagsb)) || \
					(Warn_of_mon && flags.warntypeg && \
						(flags.warntypeg & (mon)->data->mflagsg)) || \
					(Warn_of_mon && flags.warntypea && \
						(flags.warntypea & (mon)->data->mflagsa)) || \
					(Warn_of_mon && flags.warntypea && \
						(flags.warntypea & MA_UNDEAD && \
						is_undead(mon->data))) || \
					(Warn_of_mon && flags.warntypev && \
						(flags.warntypev & (mon)->data->mflagsv)) || \
					(Warn_of_mon && flags.montype && \
						(flags.montype & (unsigned long long int)((unsigned long long int)1 << (int)((mon)->data->mlet)))) \
				)

#define Weightless	(Is_airlevel(&u.uz) || \
			(Is_lolth_level(&u.uz) && levl[u.ux][u.uy].typ == CLOUD) \
			|| (Role_if(PM_MADMAN) && In_quest(&u.uz) && (levl[u.ux][u.uy].typ == CLOUD)) \
			|| (!In_endgame(&u.uz) && levl[u.ux][u.uy].typ == AIR))

#include "trap.h"
#include "flag.h"
#include "rm.h"
#include "vision.h"
#include "display.h"
#include "engrave.h"
#include "rect.h"
#include "region.h"

#ifdef USE_TRAMPOLI /* This doesn't belong here, but we have little choice */
#undef NDECL
#define NDECL(f) f()
#endif

#include "extern.h"
#include "winprocs.h"

#ifdef USE_TRAMPOLI
#include "wintty.h"
#undef WINTTY_H
#include "trampoli.h"
#undef EXTERN_H
#include "extern.h"
#endif /* USE_TRAMPOLI */

#define NO_SPELL	0

/* flags to control makemon() */
#define NO_MM_FLAGS         0x00000000    /* use this rather than plain 0 */
#define NO_MINVENT          0x00000001    /* suppress minvent when creating mon */
#define MM_NOWAIT           0x00000002    /* don't set STRAT_WAITMASK flags */
#define MM_EDOG             0x00000004    /* add edog structure */
#define MM_ESUM             0x00000008    /* add summon structure, inventory (if any) is marked as summoned. */
#define MM_ANGRY            0x00000010    /* monster is created angry */
#define MM_NONAME           0x00000020    /* monster is not christened */
#define MM_NOCOUNTBIRTH     0x00000040    /* don't increment born counter (for revival) */
#define MM_IGNOREWATER      0x00000080    /* ignore water when positioning */
#define MM_ADJACENTOK       0x00000100    /* it is acceptable to use adjacent coordinates */
#define MM_ADJACENTSTRICT   0x00000200    /* ...but only ONE removed.*/
#define MM_NOGROUP          0x00000400    /* don't generate its normal accompanying groupmates */
#define MM_BIGGROUP         0x00000800    /* do generate its larger size of accompanying groupmates */
#define MM_GOODEQUIP        0x00001000    /* do generate its better equipment sets (planar equip for angels) */
#define MM_ENDGEQUIP        0x00002000    /* do generate endgame equipment */
#define MM_MALE             0x00004000    /* make monster male */
#define MM_FEMALE           0x00008000    /* make monster female */

/* flags to control mksobj() et al */
#define NO_MKOBJ_FLAGS	0x00	/* use this rather than plain 0 */
#define MKOBJ_ARTIF		0x01	/* allow to become a random artifact at standard generation rates */
#define MKOBJ_NOINIT	0x02	/* skip standard initialization of the object, like randomized enchantment and material */
#define MKOBJ_SUMMON	0x04	/* attach ox_esum struct to obj */
#define MKOBJ_GOODEQUIP	0x08	/* attach ox_esum struct to obj */

/* special mhpmax value when loading bones monster to flag as extinct or genocided */
#define DEFUNCT_MONSTER	(-100)

/* flags for special ggetobj status returns */
#define ALL_FINISHED	  0x01  /* called routine already finished the job */

/* flags to control query_objlist() */
#define BY_NEXTHERE	  0x1	/* follow objlist by nexthere field */
#define AUTOSELECT_SINGLE 0x2	/* if only 1 object, don't ask */
#define USE_INVLET	  0x4	/* use object's invlet */
#define INVORDER_SORT	  0x8	/* sort objects by packorder */
#define SIGNAL_NOMENU	  0x10	/* return -1 rather than 0 if none allowed */
#define FEEL_COCKATRICE   0x20  /* engage cockatrice checks and react */
#define SIGNAL_ESCAPE	  0x40  /* return -2 if menu was escaped */
#define NO_EQUIPMENT	  0x80  /* don't show equipped items */

/* Flags to control query_category() */
/* BY_NEXTHERE used by query_category() too, so skip 0x01 */
#define UNPAID_TYPES 0x02
#define GOLD_TYPES   0x04
#define WORN_TYPES   0x08
#define ALL_TYPES    0x10
#define BILLED_TYPES 0x20
#define CHOOSE_ALL   0x40
#define BUC_BLESSED  0x80
#define BUC_CURSED   0x100
#define BUC_UNCURSED 0x200
#define BUC_UNKNOWN  0x400
#define BUC_ALLBKNOWN (BUC_BLESSED|BUC_CURSED|BUC_UNCURSED)
#define ALL_TYPES_SELECTED -2

/* Flags to control find_mid() */
#define FM_FMON	       0x01	/* search the fmon chain */
#define FM_MIGRATE     0x02	/* search the migrating monster chain */
#define FM_MYDOGS      0x04	/* search mydogs */
#define FM_EVERYWHERE  (FM_FMON | FM_MIGRATE | FM_MYDOGS)

/* Flags to control pick_[race,role,gend,align] routines in role.c */
#define PICK_RANDOM	0
#define PICK_RIGID	1

/* Flags to control dotrap() in trap.c */
#define NOWEBMSG	0x01	/* suppress stumble into web message */
#define FORCEBUNGLE	0x02	/* adjustments appropriate for bungling */
#define RECURSIVETRAP	0x04	/* trap changed into another type this same turn */

/* Flags to control test_move in hack.c */
#define DO_MOVE		0	/* really doing the move */
#define TEST_MOVE	1	/* test a normal move (move there next) */
#define TEST_TRAV	2	/* test a future travel location */

/*** some utility macros ***/
#define yn(query) yesno(query, FALSE)
#define ynq(query) yn_function(query,ynqchars, 'q')
#define ynaq(query) yn_function(query,ynaqchars, 'y')
#define nyaq(query) yn_function(query,ynaqchars, 'n')
#define nyNaq(query) yn_function(query,ynNaqchars, 'n')
#define ynNaq(query) yn_function(query,ynNaqchars, 'y')

/* Macros for scatter */
#define VIS_EFFECTS	0x01	/* display visual effects */
#define MAY_HITMON	0x02	/* objects may hit monsters */
#define MAY_HITYOU	0x04	/* objects may hit you */
#define MAY_HIT		(MAY_HITMON|MAY_HITYOU)
#define MAY_DESTROY	0x08	/* objects may be destroyed at random */
#define MAY_FRACTURE	0x10	/* boulders & statues may fracture */

/* Macros for launching objects */
#define ROLL		0x01	/* the object is rolling */
#define FLING		0x02	/* the object is flying thru the air */
#define LAUNCH_UNSEEN	0x40	/* hero neither caused nor saw it */
#define LAUNCH_KNOWN	0x80	/* the hero caused this by explicit action */

/* Macros for explosion types */
#define EXPL_DARK	0
#define EXPL_NOXIOUS	1
#define EXPL_MUDDY	2
#define EXPL_WET	3
#define EXPL_MAGICAL	4
#define EXPL_FIERY	5
#define EXPL_FROSTY	6
#define EXPL_GRAY	7
#define EXPL_LIME	8
#define EXPL_YELLOW	9
#define EXPL_BBLUE	10
#define EXPL_MAGENTA	11
#define EXPL_RED	12
#define EXPL_CYAN	13
#define EXPL_MAX	14

#define BALL_IN_MON	(u.uswallow && uball && uball->where == OBJ_FREE)
#define CHAIN_IN_MON	(u.uswallow && uchain && uchain->where == OBJ_FREE)

/* Flags to control menus */
#define MENUTYPELEN sizeof("traditional ")
#define MENU_TRADITIONAL 0
#define MENU_COMBINATION 1
#define MENU_PARTIAL	 2
#define MENU_FULL	 3

#define MENU_SELECTED	TRUE
#define MENU_UNSELECTED FALSE

/*
 * Option flags
 * Each higher number includes the characteristics of the numbers
 * below it.
 */
#define SET_IN_FILE	0 /* config file option only */
#define SET_VIA_PROG	1 /* may be set via extern program, not seen in game */
#define DISP_IN_GAME	2 /* may be set via extern program, displayed in game */
#define SET_IN_GAME	3 /* may be set via extern program or set in the game */

#define FEATURE_NOTICE_VER(major,minor,patch) (((unsigned long)major << 24) | \
	((unsigned long)minor << 16) | \
	((unsigned long)patch << 8) | \
	((unsigned long)0))

#define FEATURE_NOTICE_VER_MAJ	  (flags.suppress_alert >> 24)
#define FEATURE_NOTICE_VER_MIN	  (((unsigned long)(0x0000000000FF0000L & flags.suppress_alert)) >> 16)
#define FEATURE_NOTICE_VER_PATCH  (((unsigned long)(0x000000000000FF00L & flags.suppress_alert)) >>  8)

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#define plur(x) (((x) == 1) ? "" : "s")

#define makeknown(x)	discover_object((x),TRUE,TRUE)
#define distu(xx,yy)	dist2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)
#define onlineu(xx,yy)	online2((int)(xx),(int)(yy),(int)u.ux,(int)u.uy)

#define rn1(x,y)	(rn2(x)+(y))

/* Only use these on an altar */
#define a_align(x,y)	(altars[levl[x][y].altar_num].align)
#define a_gnum(x,y)		(altars[levl[x][y].altar_num].god)
#define a_shrine(x,y)	(altars[levl[x][y].altar_num].shrine)

#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar()	(IS_ALTAR(levl[u.ux][u.uy].typ) || goat_mouth_at(u.ux, u.uy) || bokrug_idol_at(u.ux, u.uy) || yog_altar_at(u.ux, u.uy))
#define on_shrine()	(IS_ALTAR(levl[u.ux][u.uy].typ) && altars[levl[u.ux][u.uy].altar_num].shrine)
#define on_god_altar(god)	(IS_ALTAR(levl[u.ux][u.uy].typ) && god_at_altar(u.ux,u.uy) == (god))

/*  */

#define notel_level() (level.flags.noteleport && !(In_quest(&u.uz) && quest_status.killed_nemesis))

/* negative armor class is randomly weakened to prevent invulnerability */
#define ROLL_NEG10(AC)	(-rnd(-(AC+10)) - 10)
#define AC_VALUE(AC)	((AC) >= -10 ? (AC) : (u.sealsActive&SEAL_BALAM || activeFightingForm(FFORM_SORESU) || is_ancient_body_ent(youracedata, u.ent_species)) ? min_ints(ROLL_NEG10(AC),ROLL_NEG10(AC)) : ROLL_NEG10(AC))
#define MONSTER_AC_VALUE(AC)	((AC) >= -10 ? (AC) : ROLL_NEG10(AC))

#if defined(MICRO) && !defined(__DJGPP__)
#define getuid() 1
#define getlogin() ((char *)0)
#endif /* MICRO */

#if defined(OVERLAY)&&(defined(OVL0)||defined(OVL1)||defined(OVL2)||defined(OVL3)||defined(OVLB))
# define USE_OVLx
# define STATIC_DCL extern
# define STATIC_OVL
# ifdef OVLB
#  define STATIC_VAR
# else
#  define STATIC_VAR extern
# endif

#else	/* !OVERLAY || (!OVL0 && !OVL1 && !OVL2 && !OVL3 && !OVLB) */
# define STATIC_DCL static
# define STATIC_OVL static
# define STATIC_VAR static

/* If not compiling an overlay, compile everything. */
# define OVL0	/* highest priority */
# define OVL1
# define OVL2
# define OVL3	/* lowest specified priority */
# define OVLB	/* the base overlay segment */
#endif	/* OVERLAY && (OVL0 || OVL1 || OVL2 || OVL3 || OVLB) */

/* Macro for a few items that are only static if we're not overlaid.... */
#if defined(USE_TRAMPOLI) || defined(USE_OVLx)
# define STATIC_PTR
#else
# define STATIC_PTR static
#endif

/* The function argument to qsort() requires a particular
 * calling convention under WINCE which is not the default
 * in that environment.
 */
#if defined(WIN_CE)
# define CFDECLSPEC __cdecl
#else
# define CFDECLSPEC
#endif
 
#endif /* HACK_H */
