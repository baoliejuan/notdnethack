/*	SCCS Id: @(#)rm.h	3.4	1999/12/12	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef RM_H
#define RM_H

/*
 * The dungeon presentation graphics code and data structures were rewritten
 * and generalized for NetHack's release 2 by Eric S. Raymond (eric@snark)
 * building on Don G. Kneller's MS-DOS implementation.	See drawing.c for
 * the code that permits the user to set the contents of the symbol structure.
 *
 * The door representation was changed by Ari Huttunen(ahuttune@niksula.hut.fi)
 */

/*
 * TLCORNER	TDWALL		TRCORNER
 * +-		-+-		-+
 * |		 |		 |
 *
 * TRWALL	CROSSWALL	TLWALL		HWALL
 * |		 |		 |
 * +-		-+-		-+		---
 * |		 |		 |
 *
 * BLCORNER	TUWALL		BRCORNER	VWALL
 * |		 |		 |		|
 * +-		-+-		-+		|
 */

/* Level location types */
enum {
    STONE = 0,
    VWALL,
    HWALL,
    TLCORNER,
    TRCORNER,
    BLCORNER,
    BRCORNER,
    CROSSWALL,	/* For pretty mazes and special levels */
    TUWALL,
    TDWALL,
    TLWALL,
    TRWALL,
    DBWALL,
    TREE,	/* KMH */
    DEADTREE,	/* youkan */
    SDOOR,
    SCORR,
    POOL,
    MOAT,	/* pool that doesn't boil, adjust messages */
    WATER,
    DRAWBRIDGE_UP,
    LAVAPOOL,
    IRONBARS,	/* KMH */
    DOOR,
    CORR,
    ROOM,
    STAIRS,
    LADDER,
    FOUNTAIN,
    FORGE,
    THRONE,
    SINK,
    GRAVE,
    ALTAR,
    ICE,
    GRASS,
    SOIL,
    SAND,
    DRAWBRIDGE_DOWN,
    AIR,
    CLOUD,
    FOG,
    DUST_CLOUD,
    PUDDLE,
    HELLISH_SEAL,
    MAX_TYPE
};
#define INVALID_TYPE	127

/*
 * Avoid using the level types in inequalities:
 * these types are subject to change.
 * Instead, use one of the macros below.
 */
#define IS_WALL(typ)	((typ) && (typ) <= DBWALL)
#define IS_STWALL(typ)	((typ) <= DBWALL)	/* STONE <= (typ) <= DBWALL */
#define IS_CORNER(typ)	((typ) >= TLCORNER && (typ) <= TRWALL)
#define IS_ROCK(typ)	((typ) < POOL)		/* absolutely nonaccessible */
#define IS_DOOR(typ)	((typ) == DOOR)
#define IS_SDOOR(typ)	((typ) == SDOOR)
#define IS_TREE(typ)	((typ) == TREE || \
			(level.flags.arboreal && (typ) == STONE))
#define IS_DEADTREE(typ) ((typ) == DEADTREE)
#define IS_TREES(typ)	(IS_TREE(typ) || IS_DEADTREE(typ))
#define ACCESSIBLE(typ) ((typ) >= DOOR)		/* good position */
#define IS_ROOM(typ)	((typ) >= ROOM)		/* ROOM, STAIRS, furniture.. */
#define ZAP_POS(typ)	((typ) >= POOL)
#define SPACE_POS(typ)	((typ) > DOOR)
#define IS_POOL(typ)	((typ) >= POOL && (typ) <= DRAWBRIDGE_UP)
#define IS_3DWATER(typ)	((typ) == WATER)
#define IS_THRONE(typ)	((typ) == THRONE)
#define IS_FOUNTAIN(typ) ((typ) == FOUNTAIN)
#define IS_FORGE(typ)	((typ) == FORGE)
#define IS_SINK(typ)	((typ) == SINK)
#define IS_GRAVE(typ)	((typ) == GRAVE)
#define IS_SEAL(typ)	((typ) == HELLISH_SEAL)
#define IS_ALTAR(typ)	((typ) == ALTAR)
#define IS_DRAWBRIDGE(typ) ((typ) == DRAWBRIDGE_UP || (typ) == DRAWBRIDGE_DOWN)
#define IS_FURNITURE(typ) ((typ) >= STAIRS && (typ) <= ALTAR)
#define IS_GRASS(typ)	((typ) == GRASS)
#define IS_SOIL(typ)	((typ) == SOIL)
#define IS_SAND(typ)	((typ) == SAND)
#define IS_AIR(typ)	((typ) == AIR || (typ) == CLOUD)
#define IS_SOFT(typ)	((typ) == AIR || (typ) == CLOUD || IS_POOL(typ))
#define IS_PUDDLE(typ)	((typ) == PUDDLE)
#define IS_PUDDLE_OR_POOL(typ)	(IS_PUDDLE(typ) || IS_POOL(typ))
#define IS_FEATURE(typ)	(IS_FURNITURE(typ) || IS_DRAWBRIDGE(typ))

/*
 * The screen symbols may be the default or defined at game startup time.
 * See drawing.c for defaults.
 * Note: {ibm|dec}_graphics[] arrays (also in drawing.c) must be kept in synch.
 */

/* begin dungeon characters */

enum {
    S_stone = 0,
    S_vwall,
    S_hwall,
    S_tlcorn,
    S_trcorn,
    S_blcorn,
    S_brcorn,
    S_crwall,
    S_tuwall,
    S_tdwall,
    S_tlwall,
    S_trwall,
    S_ndoor,
    S_vodoor,
    S_hodoor,
    S_vcdoor,	/* closed door, vertical wall */
    S_hcdoor,	/* closed door, horizontal wall */
    S_bars,	/* KMH -- iron bars */
    S_tree,	/* KMH */
    S_deadtree,	/* youkan */
    S_drkroom,
    S_litroom,
    S_brightrm,
    S_corr,
    S_litcorr,
    S_upstair,
    S_dnstair,
    S_upladder,
    S_dnladder,
    S_brupstair,
    S_brdnstair,
    S_brupladder,
    S_brdnladder,
    S_altar,
    S_grave,
    S_seal,
    S_throne,
    S_sink,
    S_fountain,
    S_forge,
    S_pool,
    S_ice,
    S_litgrass,
    S_drkgrass,
    S_litsoil,
    S_drksoil,
    S_litsand,
    S_drksand,
    S_lava,
    S_vodbridge,
    S_hodbridge,
    S_vcdbridge,	/* closed drawbridge, vertical wall */
    S_hcdbridge,	/* closed drawbridge, horizontal wall */
    S_air,
    S_cloud,
    S_fog,
    S_dust,
    S_puddle,
    S_water,

/* end dungeon characters, begin traps */

    S_arrow_trap,
    S_dart_trap,
    S_falling_rock_trap,
    S_squeaky_board,
    S_bear_trap,
    S_land_mine,
    S_rolling_boulder_trap,
    S_sleeping_gas_trap,
    S_rust_trap,
    S_fire_trap,
    S_pit,
    S_spiked_pit,
    S_hole,
    S_trap_door,
    S_teleportation_trap,
    S_level_teleporter,
    S_magic_portal,
    S_web,
    S_statue_trap,
    S_magic_trap,
    S_anti_magic_trap,
    S_polymorph_trap,
    S_essence_trap,
    S_mummy_trap,
    S_switch,
    S_flesh_hook,

/* end traps, begin special effects */


    S_vbeam,	/* The 4 zap beam symbols.  Do NOT separate. */
    S_hbeam,	/* To change order or add, see function     */
    S_lslant,	/* zapdir_to_glyph() in display.c.	    */
    S_rslant,
    S_digbeam,	/* dig beam symbol */
    S_flashbeam,	/* camera flash symbol */
    S_boomleft,	/* thrown boomerang, open left, e.g ')'    */
    S_boomright,	/* thrown boomerand, open right, e.g. '('  */
    S_ss1,	/* 4 magic shield glyphs */
    S_ss2,
    S_ss3,
    S_ss4,

/* The 8 swallow symbols.  Do NOT separate.  To change order or add, see */
/* the function swallow_to_glyph() in display.c.			 */
    S_sw_tl,		/* swallow top left [1]			*/
    S_sw_tc,		/* swallow top center [2]	Order:	*/
    S_sw_tr,		/* swallow top right [3]		*/
    S_sw_ml,		/* swallow middle left [4]	1 2 3	*/
    S_sw_mr,		/* swallow middle right [6]	4 5 6	*/
    S_sw_bl,		/* swallow bottom left [7]	7 8 9	*/
    S_sw_bc,		/* swallow bottom center [8]		*/
    S_sw_br,		/* swallow bottom right [9]		*/

    S_explode1,		/* explosion top left			*/
    S_explode2,		/* explosion top center			*/
    S_explode3,		/* explosion top right		 Ex.	*/
    S_explode4,		/* explosion middle left		*/
    S_explode5,		/* explosion middle center	 /-\	*/
    S_explode6,		/* explosion middle right	 |@|	*/
    S_explode7,		/* explosion bottom left	 \-/	*/
    S_explode8,		/* explosion bottom center		*/
    S_explode9,		/* explosion bottom right		*/

/* end effects */

    MAXPCHARS		/* maximum number of mapped characters */
};
#define MAXDCHARS	(S_water+1)	/* maximum of mapped dungeon characters */
#define MAXTCHARS	(S_flesh_hook-S_water)	/* maximum of mapped trap characters */
#define MAXECHARS	(S_explode9-S_flesh_hook)	/* maximum of mapped effects characters */
#define MAXEXPCHARS	9	/* number of explosion characters */

struct symdef {
    uchar sym;
    const char	*explanation;
#ifdef TEXTCOLOR
    uchar color;
#endif
};

extern const struct symdef defsyms[MAXPCHARS];	/* defaults */
extern glyph_t showsyms[MAXPCHARS];
extern const struct symdef def_warnsyms[WARNCOUNT];
#ifdef USER_DUNGEONCOLOR
extern uchar showsymcolors[MAXPCHARS];
#endif
extern const char * const symbol_names[MAXPCHARS];

/*
 * Graphics sets for display symbols
 */
#define ASCII_GRAPHICS	0	/* regular characters: '-', '+', &c */
#define IBM_GRAPHICS	1	/* PC graphic characters */
#define DEC_GRAPHICS	2	/* VT100 line drawing characters */
#define MAC_GRAPHICS	3	/* Macintosh drawing characters */
#define CURS_GRAPHICS   4   /* Portable curses drawing characters */
#define UTF8_GRAPHICS	5	/* UTF8 characters */

/*
 * The 5 possible states of doors
 */

#define D_NODOOR	0
#define D_BROKEN	1
#define D_ISOPEN	2
#define D_CLOSED	4
#define D_LOCKED	8
#define D_TRAPPED	16

/*
 * Thrones should only be looted once.
 */
#define T_LOOTED	 1
/*
 * Flags for special throne in the noble quest.
 */
#define NOBLE_GENO	 2
#define NOBLE_KNOW	 4
#define NOBLE_PETS	 8
#define NOBLE_WISH	16

/*
 * ID number for vault type
 */
enum {
/*01*/	VN_AKKABISH = 1,
/*02*/	VN_SHALOSH,
/*03*/	VN_NACHASH,
/*04*/	VN_KHAAMNUN,
/*05*/	VN_RAGLAYIM,
/*06*/	VN_TERAPHIM,
/*07*/	VN_SARTAN,
/*08*/	VN_A_O_BLESSINGS,
/*09*/	VN_A_O_VITALITY,
/*10*/	VN_A_O_CORRUPTION,
/*11*/	VN_A_O_BURNING_WASTES,
/*12*/	VN_A_O_THOUGHT,
/*13*/	VN_A_O_DEATH,
/*14*/	VN_APOCALYPSE,
/*15*/	VN_HARROWER,
/*16*/	VN_MAD_ANGEL,
/*17*/	VN_JRT,
/*18*/	VN_N_PIT_FIEND,
/*19*/	VN_SHAYATEEN,
/*20*/	VN_MAX,
};
#define LIMIT_VN_RANGE_1_TANNINIM	(VN_A_O_BLESSINGS)
#define LIMIT_VN_RANGE_2_ANCIENT	(VN_APOCALYPSE)
#define LIMIT_VN_RANGE_3_ANGEL		(VN_N_PIT_FIEND)
#define LIMIT_VN_RANGE_4_DEVIL		(VN_SHAYATEEN)
#define LIMIT_VN_RANGE_5_DEMON		(VN_MAX)

//if VN_MAX exceeds this limit, we have a problem (Vault IDs should range from 1 to 31).
#define VAULT_LIMIT	32

/*
 * Trees have more than one kick result.
 */
#define TREE_LOOTED	1
#define TREE_SWARM	2

/*
 * Fountains have limits, and special warnings.
 */
#define F_LOOTED	1
#define F_WARNED	2
#define FOUNTAIN_IS_WARNED(x,y)		(levl[x][y].looted & F_WARNED)
#define FOUNTAIN_IS_LOOTED(x,y)		(levl[x][y].looted & F_LOOTED)
#define SET_FOUNTAIN_WARNED(x,y)	levl[x][y].looted |= F_WARNED;
#define SET_FOUNTAIN_LOOTED(x,y)	levl[x][y].looted |= F_LOOTED;
#define CLEAR_FOUNTAIN_WARNED(x,y)	levl[x][y].looted &= ~F_WARNED;
#define CLEAR_FOUNTAIN_LOOTED(x,y)	levl[x][y].looted &= ~F_LOOTED;

/*
 * Doors are even worse :-) The special warning has a side effect
 * of instantly trapping the door, and if it was defined as trapped,
 * the guards consider that you have already been warned!
 */
#define D_WARNED	16

/*
 * Sinks have 3 different types of loot that shouldn't be abused
 */
#define S_LPUDDING	1
#define S_LDWASHER	2
#define S_LRING		4

/*
 * The four directions for a DrawBridge.
 */
#define DB_NORTH	0
#define DB_SOUTH	1
#define DB_EAST		2
#define DB_WEST		3
#define DB_DIR		3	/* mask for direction */

/*
 * What's under a drawbridge.
 */
#define DB_MOAT		0
#define DB_LAVA		4
#define DB_ICE		8
#define DB_FLOOR	16
#define DB_UNDER	28	/* mask for underneath */

/*
 * Wall information.
 */
#define WM_MASK		0x07	/* wall mode (bottom three bits) */
#define W_NONDIGGABLE	0x08
#define W_NONPASSWALL	0x10

/*
 * Ladders (in Vlad's tower) may be up or down.
 */
#define LA_UP		1
#define LA_DOWN		2

/*
 * Room areas may be iced pools
 */
#define ICED_PUDDLE	4
#define ICED_POOL	8
#define ICED_MOAT	16

/*
 * The structure describing a coordinate position.
 * Before adding fields, remember that this will significantly affect
 * the size of temporary files and save files.
 */
struct rm {
	int glyph;		/* what the hero thinks is there */
	schar typ;		/* what is really there */
	Bitfield(styp, 6);	/* last seen/touched dungeon typ */
	uchar seenv;		/* seen vector */
	Bitfield(flags,5);	/* extra information for typ */
	Bitfield(horizontal,1); /* wall/door/etc is horiz. (more typ info) */
	Bitfield(lit,1);	/* speed hack for lit rooms */
	Bitfield(waslit,1);	/* remember if a location was lit */
	Bitfield(roomno,6);	/* room # for special rooms */
	Bitfield(edge,1);	/* marks boundaries for special rooms*/
};


#define SET_TYPLIT(x,y,ttyp,llit)				\
{								\
  if ((x) >= 0 && (y) >= 0 && (x) < COLNO && (y) < ROWNO) {	\
    if ((ttyp) < MAX_TYPE) levl[(x)][(y)].typ = (ttyp);		\
    if ((ttyp) == LAVAPOOL) levl[(x)][(y)].lit = 1;		\
    else if ((ttyp) == FORGE) levl[(x)][(y)].lit = 1;		\
    else if ((schar)(llit) != -2) {				\
	if ((schar)(llit) == -1) levl[(x)][(y)].lit = rn2(2);	\
	else levl[(x)][(y)].lit = (llit);			\
    }								\
  }								\
}

/*
 * Add wall angle viewing by defining "modes" for each wall type.  Each
 * mode describes which parts of a wall are finished (seen as as wall)
 * and which are unfinished (seen as rock).
 *
 * We use the bottom 3 bits of the flags field for the mode.  This comes
 * in conflict with secret doors, but we avoid problems because until
 * a secret door becomes discovered, we know what sdoor's bottom three
 * bits are.
 *
 * The following should cover all of the cases.
 *
 *	type	mode				Examples: R=rock, F=finished
 *	-----	----				----------------------------
 *	WALL:	0 none				hwall, mode 1
 *		1 left/top (1/2 rock)			RRR
 *		2 right/bottom (1/2 rock)		---
 *							FFF
 *
 *	CORNER: 0 none				trcorn, mode 2
 *		1 outer (3/4 rock)			FFF
 *		2 inner (1/4 rock)			F+-
 *							F|R
 *
 *	TWALL:	0 none				tlwall, mode 3
 *		1 long edge (1/2 rock)			F|F
 *		2 bottom left (on a tdwall)		-+F
 *		3 bottom right (on a tdwall)		R|F
 *
 *	CRWALL: 0 none				crwall, mode 5
 *		1 top left (1/4 rock)			R|F
 *		2 top right (1/4 rock)			-+-
 *		3 bottom left (1/4 rock)		F|R
 *		4 bottom right (1/4 rock)
 *		5 top left & bottom right (1/2 rock)
 *		6 bottom left & top right (1/2 rock)
 */

#define WM_W_LEFT 1			/* vertical or horizontal wall */
#define WM_W_RIGHT 2
#define WM_W_TOP WM_W_LEFT
#define WM_W_BOTTOM WM_W_RIGHT

#define WM_C_OUTER 1			/* corner wall */
#define WM_C_INNER 2

#define WM_T_LONG 1			/* T wall */
#define WM_T_BL   2
#define WM_T_BR   3

#define WM_X_TL   1			/* cross wall */
#define WM_X_TR   2
#define WM_X_BL   3
#define WM_X_BR   4
#define WM_X_TLBR 5
#define WM_X_BLTR 6

/*
 * Seen vector values.	The seen vector is an array of 8 bits, one for each
 * octant around a given center x:
 *
 *			0 1 2
 *			7 x 3
 *			6 5 4
 *
 * In the case of walls, a single wall square can be viewed from 8 possible
 * directions.	If we know the type of wall and the directions from which
 * it has been seen, then we can determine what it looks like to the hero.
 */
#define SV0 0x1
#define SV1 0x2
#define SV2 0x4
#define SV3 0x8
#define SV4 0x10
#define SV5 0x20
#define SV6 0x40
#define SV7 0x80
#define SVALL 0xFF



#define doormask	flags
#define altar_num	flags
#define wall_info	flags
#define ladder		flags
#define drawbridgemask	flags
#define looted		flags
#define icedpool	flags
#define vaulttype	flags

#define blessedftn	horizontal  /* a fountain that grants attribs */
#define disturbed	horizontal  /* a grave that has been disturbed */

struct damage {
	struct damage *next;
	long when, cost;
	coord place;
	schar typ;
};

struct levelflags {
	uchar	nfountains;		/* number of fountains on level */
	uchar	nforges;		/* number of forges on level */
	uchar	nsinks;			/* number of sinks on the level */
	int		goldkamcount_hostile;	/* number of hostile gold kamerel 'above' level */
	int		goldkamcount_peace;	/* number of peaceful gold kamerel 'above' level */
	int		sp_lev_nroom;	/* number of rooms on the level as defined by the special level generator */
	/* Several flags that give hints about what's on the level */
	Bitfield(has_shop, 1);
	Bitfield(has_vault, 1);
	Bitfield(has_zoo, 1);
	Bitfield(has_court, 1);
	Bitfield(has_morgue, 1);
	Bitfield(has_garden, 1);
	Bitfield(has_library, 1);
	Bitfield(has_beehive, 1);
	Bitfield(has_armory, 1);
	Bitfield(has_barracks, 1);
	Bitfield(has_temple, 1);
	/*11*/
	Bitfield(has_swamp, 1);
	Bitfield(has_island, 1);
	Bitfield(has_river, 1);
	Bitfield(noteleport,1);
	Bitfield(hardfloor,1);
	Bitfield(nommap,1);
	Bitfield(hero_memory,1);	/* hero has memory */
	Bitfield(shortsighted,1);	/* monsters are shortsighted */
	Bitfield(graveyard,1);		/* has_morgue, but remains set */
	Bitfield(is_maze_lev,1);
	/*21*/
	Bitfield(is_cavernous_lev,1);
	Bitfield(arboreal, 1);		/* Trees replace rock */
	Bitfield(lethe, 1);			/* All water on level causes amnesia */
	/*24*/
	/* Not currently used */
	Bitfield(slime, 1);			/* corpses on level become slimes */
	Bitfield(fungi, 1);			/* corpses on level become fungi */
	Bitfield(dun, 1);			/* level is the Dun Savannah */
	Bitfield(necro, 1);			/* corpses on level rise as undead */
	/* End not currently used */
	/*28*/
	
	Bitfield(cave, 1);			/* level is a cave */
	Bitfield(outside, 1);			/* level is outside */
	Bitfield(has_minor_spire, 1);	/* has minor spire (spawns only normal kamerel) */
	Bitfield(has_kamerel_towers, 1);		/* has kamerel tows (spawns normal kamerel) */
	/*32*/
};

typedef struct
{
    struct rm		locations[COLNO][ROWNO];
#ifndef MICROPORT_BUG
    struct obj		*objects[COLNO][ROWNO];
    struct monst	*monsters[COLNO][ROWNO];
#else
    struct obj		*objects[1][ROWNO];
    char		*yuk1[COLNO-1][ROWNO];
    struct monst	*monsters[1][ROWNO];
    char		*yuk2[COLNO-1][ROWNO];
#endif
    struct obj		*objlist;
    struct obj		*buriedobjlist;
    struct monst	*monlist;
    struct damage	*damagelist;
    struct levelflags	flags;
    long	lastmove;
}
dlevel_t;

extern dlevel_t level;	/* structure describing the current level */

/*
 * Macros for compatibility with old code. Someday these will go away.
 */
#define levl		level.locations
#define fobj		level.objlist
#define fmon		level.monlist

/*
 * Covert a trap number into the defsym graphics array.
 * Convert a defsym number into a trap number.
 * Assumes that arrow trap will always be the first trap.
 */
#define trap_to_defsym(t) (S_arrow_trap+(t)-1)
#define defsym_to_trap(d) ((d)-S_arrow_trap+1)

#define OBJ_AT(x,y)	(level.objects[x][y] != (struct obj *)0)
/*
 * Macros for encapsulation of level.monsters references.
 */
#define MON_AT(x,y)	(level.monsters[x][y] != (struct monst *)0 && \
			 !(level.monsters[x][y])->mburied)
#define MON_BURIED_AT(x,y)	(level.monsters[x][y] != (struct monst *)0 && \
				(level.monsters[x][y])->mburied)
#ifndef STEED
#define place_monster(m,x,y)	((m)->mx=(x),(m)->my=(y),\
				 level.monsters[(m)->mx][(m)->my]=(m))
#endif
#define place_worm_seg(m,x,y)	level.monsters[x][y] = m
#define m_at(x,y)		((isok(x,y) && MON_AT(x,y)) ? level.monsters[x][y] : \
						(struct monst *)0)
#define m_buried_at(x,y)	(MON_BURIED_AT(x,y) ? level.monsters[x][y] : \
						       (struct monst *)0)
#define m_u_at(x,y)		(!isok(x,y) ? (struct monst *)0 : MON_AT(x,y) ? level.monsters[x][y] : \
						(x == u.ux && y == u.uy) ? &youmonst : (struct monst *)0)

#endif /* RM_H */
