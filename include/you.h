/*	SCCS Id: @(#)you.h	3.4	2000/05/21	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef YOU_H
#define YOU_H

#include "attrib.h"
#include "monst.h"
#include "cults.h"
#ifndef PROP_H
#include "prop.h"		/* (needed here for util/makedefs.c) */
#endif
#include "skills.h"
#include "engrave.h"

/*** Substructures ***/

struct RoleName {
	char	*m;	/* name when character is male */
	char	*f;	/* when female; null if same as male */
};

struct RoleAdvance {
	/* "fix" is the fixed amount, "rnd" is the random amount */
	xchar infix, inrnd;	/* at character initialization */
	xchar lofix, lornd;	/* gained per level <  urole.xlev */
	xchar hifix, hirnd;	/* gained per level >= urole.xlev */
};

struct u_have {
	Bitfield(amulet,1);	/* carrying Amulet	*/
	Bitfield(bell,1);	/* carrying Bell	*/
	Bitfield(book,1);	/* carrying Book	*/
	Bitfield(menorah,1);	/* carrying Candelabrum */
	Bitfield(questart,1);	/* carrying the Quest Artifact */
	Bitfield(unused,3);
};

struct u_event {
	Bitfield(minor_oracle,1);		/*1 received at least 1 cheap oracle */
	Bitfield(major_oracle,1);		/*2  "  expensive oracle */
	Bitfield(qcalled,1);			/*3 called by Quest leader to do task */
	Bitfield(qexpelled,1);			/*4 expelled from the Quest dungeon */
	Bitfield(qcompleted,1);			/*5 successfully completed Quest task */
	Bitfield(uheard_tune,2);		/*7 1=know about, 2=heard passtune */
	Bitfield(uopened_dbridge,1);	/*8 opened the drawbridge */
	Bitfield(uread_necronomicon,1);	/*9 have read the necronomicon */
	Bitfield(knoweddergud,1);		/*10 know the identity of the black-web god */

	Bitfield(found_square,1);		/*11 found the vibrating square */
	Bitfield(invoked,1);			/*12 invoked Gate to the Sanctum level */
	Bitfield(gehennom_entered,1);	/*13 entered Gehennom via Valley */
#define CROWNING_BITS	7
#define MAX_CROWNING	pow(2,CROWNING_BITS)-1
	Bitfield(uhand_of_elbereth,CROWNING_BITS);	/*19 became Hand of Elbereth */
	Bitfield(udemigod,1);			/*21 killed the wiz */
	Bitfield(ukilled_apollyon,1);	/*22 killed the angel of the pit.  Lucifer should spawn on Astral */
	Bitfield(ukilled_illurien,1);	/*23 Harassment */
	Bitfield(ukilled_dagon,1);		/*24 Returns */
	Bitfield(ukilled_hydra,1);		/*25 Returns */
	Bitfield(sum_entered,1);		/*26 entered Sum-of-All */
	Bitfield(uaxus_foe,1);			/*27 enemy of the modrons */
	Bitfield(utook_castle, 2);		/*29 sat on the castle throne, used artifact wish */
	Bitfield(uunknowngod, 2);		/*31 given five artifacts to the priests of the unknown god, used artifact wish */
	Bitfield(uconstellation, 2);	/*33 has earned the star emperor ring wish, used artifact wish */
#define ARTWISH_EARNED	1
#define ARTWISH_SPENT	2
	Bitfield(ascended,1);			/*34 has offered the Amulet */
	Bitfield(knoxmade,1);			/*35 Portal to Ludios has been made in the main dungeon, teleport ok */
	Bitfield(qrecalled,1);			/*36 Quest re-opened */
	
	Bitfield(padding, 9);			/*45 reseve another bitfield in event. */
};

/* KMH, conduct --
 * These are voluntary challenges.  Each field denotes the number of
 * times a challenge has been violated.
 */
struct u_conduct {		/* number of times... */
	long	unvegetarian;	/* eaten any animal */
	long	unvegan;	/* ... or any animal byproduct */
	long	food;		/* ... or any comestible */
	long	ratseaten;	/* number of rats eaten */
	long	gnostic;	/* used prayer, priest, or altar */
	long	weaphit;	/* hit a monster with a weapon */
	long	killer;		/* killed a monster yourself */
	long	wardless;	/* drew a warding symbol */
	long	elbereth;	/* wrote elbereth */
	long	literate;	/* read something (other than BotD) */
	long	polypiles;	/* polymorphed an object */
	long	polyselfs;	/* transformed yourself */
	long	wishes;		/* used a wish */
	long	wisharti;	/* wished for an artifact */
	long	shopID;		/* number of items id by shopkeepers */
	long	IDs;		/* number of items id by magic or shopkeepers */
				/* genocides already listed at end of game */
};

/*** Unified structure containing role information ***/
struct Role {
	/*** Strings that name various things ***/
	struct RoleName name;	/* the role's name (from u_init.c) */
	struct RoleName rank[9]; /* names for experience levels (from botl.c) */
	int lgod, ngod, cgod;	/* god numbers (from gnames.h) */
	int vgod;				/* god number (from gnames.h) to use for vampires */
	const char *filecode;	/* abbreviation for use in file names */
	const char *homebase;	/* quest leader's location (from questpgr.c) */
	const char *intermed;	/* quest intermediate goal (from questpgr.c) */

	/*** Indices of important monsters and objects ***/
	short malenum,		/* index (PM_) as a male (botl.c) */
	      femalenum,	/* ...or as a female (NON_PM == same) */
	      petnum,		/* PM_ of preferred pet (NON_PM == random) */
	      ldrnum,		/* PM_ of quest leader (questpgr.c) */
	      guardnum,		/* PM_ of quest guardians (questpgr.c) */
	      neminum,		/* PM_ of quest nemesis (questpgr.c) */
	      enemy1num,	/* specific quest enemies (NON_PM == random) */
	      enemy2num;
	char  enemy1sym,	/* quest enemies by class (S_) */
	      enemy2sym;
	int questarti;	/* index (ART_) of quest artifact (questpgr.c) */

	/*** Bitmasks ***/
	long marace;		/* allowable races */
	short allow;		/* bit mask of allowed variations */
#define ROLE_GENDMASK	0xf000		/* allowable genders */
#define ROLE_MALE	0x1000
#define ROLE_FEMALE	0x2000
#define ROLE_NEUTER	0x4000
#define ROLE_ALIGNMASK	AM_MASK		/* allowable alignments */
#define ROLE_LAWFUL	AM_LAWFUL
#define ROLE_NEUTRAL	AM_NEUTRAL
#define ROLE_CHAOTIC	AM_CHAOTIC

	/*** Attributes (from attrib.c and exper.c) ***/
	xchar attrbase[A_MAX];	/* lowest initial attributes */
	xchar attrdist[A_MAX];	/* distribution of initial attributes */
	struct RoleAdvance hpadv; /* hit point advancement */
	struct RoleAdvance enadv; /* energy advancement */
	xchar xlev;		/* cutoff experience level */
	xchar initrecord;	/* initial alignment record */

	/*** Spell statistics (from spell.c) ***/
	int spelbase;		/* base spellcasting penalty */
	int spelheal;		/* penalty (-bonus) for healing spells */
	int spelshld;		/* penalty for wearing any shield */
	int spelarmr;		/* penalty for wearing metal armour */
	int spelstat;		/* which stat (A_) is used */
	int spelspec;		/* spell (SPE_) the class excels at */
	int spelsbon;		/* penalty (-bonus) for that spell */

	/*** Properties in variable-length arrays ***/
	/* intrinsics (see attrib.c) */
	/* initial inventory (see u_init.c) */
	/* skills (see u_init.c) */

	/*** Don't forget to add... ***/
	/* quest leader, guardians, nemesis (monst.c) */
	/* quest artifact (artilist.h) */
	/* quest dungeon definition (dat/Xyz.dat) */
	/* quest text (dat/quest.txt) */
	/* dictionary entries (dat/data.bas) */
};

extern struct Role roles[];	/* table of available roles */
extern struct Role urole;
#define Role_if(X)	(urole.malenum == (X))
#define Pantheon_if(X)	(flags.racial_pantheon != 0 ? flags.racial_pantheon == (X) : roles[flags.pantheon].malenum == (X))
#define God_if(X)	(u.ualign.god == (X))
#define Holiness_if(X)	(gholiness(u.ualign.god) == (X))
#define Role_switch	(urole.malenum)
/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC) || \
		Race_if(PM_VAMPIRE))


/* used during initialization for race, gender, and alignment
   as well as for character class */
#define ROLE_NONE	(-1)
#define ROLE_RANDOM	(-2)

struct mask_properties {
	int msklevel;
	int mskmonnum;
	int mskrolenum;
	Bitfield(mskfemale,1);
	struct attribs	mskacurr,
					mskaexe,
					mskamax;
	align	mskalign;
	schar mskluck;
	int mskhp,mskhpmax;
	int msken,mskenmax;
	long mskexp, mskrexp;
	int	mskweapon_slots;		/* unused skill slots */
	int	mskskills_advanced;		/* # of advances made so far */
	xchar	mksskill_record[P_SKILL_LIMIT];	/* skill advancements */
	struct skills mask_skills[P_NUM_SKILLS];
};

struct old_lev{
	struct d_level uz;
	xchar ux;
	xchar uy;	
};

/*** Unified structure specifying race information ***/

struct Race {
	/*** Strings that name various things ***/
	const char *noun;	/* noun ("human", "elf") */
	const char *adj;	/* adjective ("human", "elven") */
	const char *coll;	/* collective ("humanity", "elvenkind") */
	const char *filecode;	/* code for filenames */
	struct RoleName individual; /* individual as a noun ("man", "elf") */

	/*** Indices of important monsters and objects ***/
	short malenum,		/* PM_ as a male monster */
	      femalenum,	/* ...or as a female (NON_PM == same) */
	      mummynum,		/* PM_ as a mummy */
	      zombienum;	/* PM_ as a zombie */

	/*** Bitmasks ***/
	short allow;		/* bit mask of allowed variations */
	long selfmask,		/* your own race's bit mask */
	      lovemask,		/* bit mask of always peaceful */
	      hatemask;		/* bit mask of always hostile */

	/*** Attributes ***/
	xchar attrmin[A_MAX];	/* minimum allowable attribute */
	xchar attrmax[A_MAX];	/* maximum allowable attribute */
	struct RoleAdvance hpadv; /* hit point advancement */
	struct RoleAdvance enadv; /* energy advancement */
	int   nv_range;		/* night vision range */
#define NO_NIGHTVISION	0
#define NORMALNIGHTVIS	1
#define NIGHTVISION2	2
#define NIGHTVISION3	3

	int spelspec;		/* spell (SPE_) the species excels at */
	int spelsbon;		/* penalty (-bonus) for that spell */
	/*** Properties in variable-length arrays ***/
	/* intrinsics (see attrib.c) */

	/*** Don't forget to add... ***/
	/* quest leader, guardians, nemesis (monst.c) */
	/* quest dungeon definition (dat/Xyz.dat) */
	/* quest text (dat/quest.txt) */
	/* dictionary entries (dat/data.bas) */
};

extern const struct Race races[];	/* Table of available races */
extern struct Race urace;
#define Race_if(X)	(urace.malenum == (X))
#define Race_switch	(urace.malenum)
#define youracedata	(youmonst.data)
#define Humanoid_half_dragon(role)	(Role_if(PM_MADMAN) || (Role_if(PM_NOBLEMAN) && flags.initgend))

/*** Unified structure specifying gender information ***/
struct Gender {
	const char *adj;	/* male/female/neuter */
	const char *he;		/* he/she/it */
	const char *him;	/* him/her/it */
	const char *his;	/* his/her/its */
	const char *filecode;	/* file code */
	short allow;		/* equivalent ROLE_ mask */
};
#define ROLE_GENDERS	2	/* number of permitted player genders */
				/* increment to 3 if you allow neuter roles */

extern const struct Gender genders[];	/* table of available genders */
#define uhe()	(genders[flags.female ? 1 : 0].he)
#define uhim()	(genders[flags.female ? 1 : 0].him)
#define uhis()	(genders[flags.female ? 1 : 0].his)
#define mhe(mtmp)	(genders[pronoun_gender(mtmp)].he)
#define mhim(mtmp)	(genders[pronoun_gender(mtmp)].him)
#define mhis(mtmp)	(genders[pronoun_gender(mtmp)].his)


/*** Unified structure specifying alignment information ***/
struct Align {
	const char *noun;	/* law/balance/chaos */
	const char *adj;	/* lawful/neutral/chaotic */
	const char *filecode;	/* file code */
	short allow;		/* equivalent ROLE_ mask */
	aligntyp value;		/* equivalent A_ value */
};
#define ROLE_ALIGNS	3	/* number of permitted player alignments */

extern const struct Align aligns[];	/* table of available alignments */

struct Species {
	const char *name;
	int value;
	int type;
};

extern const struct Species species[];	/* table of available species */

#define current_species_name() (uclockwork ? default_material_name(u.clk_material, FALSE) : species[flags.initspecies].name)
#define base_species_name() (Race_if(PM_CLOCKWORK_AUTOMATON) ? default_material_name(u.clk_material, FALSE) : species[flags.initspecies].name)

#define ROLE_SPECIES	37	/* number of permitted player species */
#define NONE_SPECIES 0
#define ENT_SPECIES 1
#define DRAGON_SPECIES 2
#define CLK_SPECIES 3

/*** Information about the player ***/
struct you {
	xchar ux, uy;
	schar dx, dy, dz;	/* direction of move (or zap or ... ) */
	schar di;		/* direction of FF */
	xchar tx, ty;		/* destination of travel */
	xchar itx, ity;		/* intermediary travel destination */
	xchar ux0, uy0;		/* initial position FF */
	d_level uz, uz0;	/* your level on this and the previous turn */
	d_level utolev;		/* level monster teleported you to, or uz */
	uchar utotype;		/* bitmask of goto_level() flags for utolev */
	boolean umoved;		/* changed map location (post-move) */
	boolean uattked;		/* attacked a target (post-move) */
	boolean unull;		/* passed a turn (post-move) */
	coord prev_dir;		/* previous dirction pressed (for monk moves) */
	int last_str_turn;	/* 0: none, 1: half turn, 2: full turn */
				/* +: turn right, -: turn left */
	int ulevel, ulevel_real;		/* 1 to MAXULEV */
	int ulevelmax, ulevelmax_real;
	unsigned utrap;		/* trap timeout */
	unsigned utraptype;	/* defined if utrap nonzero */
#define TT_BEARTRAP	0
#define TT_PIT		1
#define TT_WEB		2
#define TT_LAVA		3
#define TT_INFLOOR	4
#define TT_FLESH_HOOK	5
#define TT_SALIVA	6
	char	urooms[5];	/* rooms (roomno + 3) occupied now */
	char	urooms0[5];	/* ditto, for previous position */
	char	uentered[5];	/* rooms (roomno + 3) entered this turn */
	char	ushops[5];	/* shop rooms (roomno + 3) occupied now */
	char	ushops0[5];	/* ditto, for previous position */
	char	ushops_entered[5]; /* ditto, shops entered this turn */
	char	ushops_left[5]; /* ditto, shops exited this turn */

	int	 uhunger;	/* refd only in eat.c and shk.c */
	int	 uhungermax;/*  */

	struct old_lev old_lev; /*used for etheraloid phasing in/out*/
#define YouHunger	(Race_if(PM_INCANTIFIER) ? u.uen : u.uhunger)
#define	INC_BASE_NUTRITION	25
#define DEFAULT_HMAX	2000
	unsigned uhs;		/* hunger state - see eat.c */

#define FFORM_LISTSIZE	(LAST_FFORM/16 + 1)
	unsigned long int fightingForm[FFORM_LISTSIZE];/* special properties */
	int ueldritch_style;
	Bitfield(uavoid_passives,1);
	Bitfield(uavoid_msplcast,1);
	Bitfield(uavoid_grabattk,1);
	Bitfield(uavoid_englattk,1);
	Bitfield(uavoid_unsafetouch,1);
	Bitfield(uavoid_theft,1);
	Bitfield(uno_auto_attacks,1);
	int umystic;	/*Monk mystic attacks active*/
#define monk_style_active(style) (u.umystic & (1 << (style-1)))
#define toggle_monk_style(style) (u.umystic  = u.umystic ^ (1 << (style-1)))

#define DIVE_KICK 1
#define AURA_BOLT 2
#define BIRD_KICK 3
#define METODRIVE 4
#define PUMMEL    5
	// long laststruck;
	long lastmoved;
	long lastcast;
	long bladesong;
	
	boolean ukinghill; /* records if you are carying the pirate treasure (and are therefor king of the hill) */
	int protean; /* counter for the auto-polypiling power of the pirate treasure*/
	int uhouse; /* drow house info */
	int ent_species; /*species of ent tree*/
#define ENT_ASH 0
#define ENT_BEECH 1
#define ENT_BIRCH 2
#define ENT_BLUEGUM 3
#define ENT_CEDAR 4
#define ENT_CHESTNUT 5
#define ENT_CYPRESS 6
#define ENT_DOGWOOD 7
#define ENT_ELDER 8
#define ENT_ELM 9
#define ENT_FIR 10
#define ENT_GINKGO 11
#define ENT_LARCH 12
#define ENT_LOCUST 13
#define ENT_MAGNOLIA 14
#define ENT_MAPLE 15
#define ENT_MIMOSA 16
#define ENT_METHUSELAH 17
#define ENT_OAK 18
#define ENT_POPLAR 19
#define ENT_REDWOOD 20
#define ENT_SPRUCE 21
#define ENT_WILLOW 22
#define ENT_YEW 23
#define ENT_YGGDRASIL 24
#define ENT_MAX_SPECIES ENT_YGGDRASIL
	int start_house; /* starting drow house info */
	int inherited; /* what you inherited at the start, if anything */
	struct prop uprops[LAST_PROP+1];
	int rift_count;
	int vortex_count;
	int miso_count;
	
	unsigned umconf;
	char usick_cause[PL_PSIZ+20]; /* sizeof "unicorn horn named "+1 */
	Bitfield(usick_type,2);
#define SICK_VOMITABLE 0x01
#define SICK_NONVOMITABLE 0x02
#define SICK_ALL 0x03

	/* This can never be more than MAX_RANGE (vision.h). */
	int nv_range;		/* current night vision range */

	/*
	 * These variables are valid globally only when punished and blind.
	 */
#define BC_BALL  0x01	/* bit mask for ball  in 'bc_felt' below */
#define BC_CHAIN 0x02	/* bit mask for chain in 'bc_felt' below */
	int bglyph;	/* glyph under the ball */
	int cglyph;	/* glyph under the chain */
	int bc_order;	/* ball & chain order [see bc_order() in ball.c] */
	int bc_felt;	/* mask for ball/chain being felt */

	int umonster;			/* hero's role's monster num */
	int umonnum;			/* current monster number (either your role's or a polyform) */

	int mh, mhmax, mhrolled, mtimedone;	/* for polymorph-self */
#define MATTK_DSCALE         1
#define MATTK_BREATH         2
#define MATTK_HBREATH        3
#define MATTK_SPIT           4
#define MATTK_MAGIC          5
#define MATTK_REMV           6
#define MATTK_GAZE           7
#define MATTK_SUMM           8
#define MATTK_WEBS           9
#define MATTK_HIDE          10
#define MATTK_MIND          11
#define MATTK_CLOCK         12
#define MATTK_DARK          13
#define MATTK_VAMP          14
#define MATTK_REPL          15
#define MATTK_UHORN         16
#define MATTK_SHRIEK        17
#define MATTK_SCREAM        18
#define MATTK_HOLE          19
#define MATTK_REACH         20
#define MATTK_DROID         21
#define MATTK_TNKR          22
#define MATTK_U_SPELLS      23
#define MATTK_U_SPIRITS     24
#define MATTK_U_WORD        25
#define MATTK_U_TURN_UNDEAD 26 /* MATTK_U_TURN would be amusing, but even more confusing */
#define MATTK_U_STYLE       27
#define MATTK_U_MONST       28
#define MATTK_U_ELMENTAL    29
#define MATTK_WHISPER    	30
#define MATTK_TELEK	    31
#define MATTK_CRAZE	    32
#define MATTK_PULSE	    33
#define MATTK_LAVA	    34
#define MATTK_PHASE_OUT 35
#define MATTK_PHASE_IN 36
#define MATTK_YUKI 37
#define MATTK_LEPRE 38
#define MATTK_KI 39
#define MATTK_UPGRADE    	40



#define ACU_RETURN_LVL 20
#define ACU_TELEK_LVL 15
#define ACU_REFL_LVL 14
#define ACU_CRAZE_LVL 12
#define ACU_PULSE_LVL 4

	struct attribs	macurr,		/* for monster attribs */
			mamax;		/* for monster attribs */
	int ulycn;			/* lycanthrope type */
	short ucspeed;
#define	HIGH_CLOCKSPEED	1
#define	NORM_CLOCKSPEED	2
#define	SLOW_CLOCKSPEED	3
	long clockworkUpgrades;
	int uboiler;
#define	MAX_BOILER	2000
	int ustove;
	int utemp;
#define	WARM			 1
#define	HOT				 2
#define	BURNING_HOT		 5
#define	MELTING			10
#define	MELTED			20
/*Note: because clockwork_eat_menu uses ints instead of longs, all upgrades that change how you eat must fit in an int.*/
#define OIL_STOVE			0x000001L
#define WOOD_STOVE			0x000002L
#define FAST_SWITCH			0x000004L
#define EFFICIENT_SWITCH	0x000008L
#define ARMOR_PLATING		0x000010L
#define PHASE_ENGINE		0x000020L
#define MAGIC_FURNACE		0x000040L
#define HELLFIRE_FURNACE	0x000080L
#define SCRAP_MAW			0x000100L
#define HIGH_TENSION		0x000200L
#define HEAVY_BLASTER		0x000400L
#define SECOND_BLASTER		0x000800L

//define	HIGH_CLOCKSPEED	1
//define	NORM_CLOCKSPEED	2
//define	SLOW_CLOCKSPEED	3
#define	RECHARGER			4
#define ANDROID_COMBO		5
//define PHASE_ENGINE		32
//define HEAVY_BLASTER		1024
	
	int slowclock;
	
	unsigned ucreamed;
	unsigned uswldtim;		/* time you have been swallowed */

	Bitfield(uswallow,1);		/* true if swallowed */
	Bitfield(uinwater,1);		/* if you're currently in water */
	Bitfield(usubwater,1);		/* if you're currently underwater */
	Bitfield(uundetected,1);	/* if you're a hiding monster/piercer */
	Bitfield(mfemale,1);		/* saved human value of flags.female */
	Bitfield(uinvulnerable,1);	/* you're invulnerable (praying) */
	Bitfield(uburied,1);		/* you're buried */
	Bitfield(uedibility,1);		/* blessed food detection; sense unsafe food */
	/* 0 free bits? (doesn't add up?) */
	Bitfield(shambin,2);		/* Whether the shambling horror has normal innards, undifferentiated innards, or solid/nonexistent innards */
	Bitfield(stumbin,2);		/* Whether the stumbling horror has normal innards, undifferentiated innards, or solid/nonexistent innards */
	Bitfield(wandein,2);		/* Whether the wandering horror has normal innards, undifferentiated innards, or solid/nonexistent innards */
	Bitfield(umartial,1);		/* Do you know kung fu? */
	Bitfield(umaniac,1);		/* Swings wildly */
	Bitfield(phasengn,1);		/* clockwork phase engine */
	Bitfield(umummyrot,1);		/* you have mummy rot */
	Bitfield(veil,1);			/* you have not peaked behind the veil */
#define lift_veil() if(u.veil){\
				u.veil = FALSE;\
				change_uinsight(1);\
			}
	Bitfield(render_thought,1);	/* you got a thought from a veil-render */
	Bitfield(detestation_ritual,7);	/* progress in detestation ritual */
#define NO_RITUAL 0
#define RITUAL_STARTED	0x01
#define RITUAL_CHAOS	0x02
#define RITUAL_NEUTRAL	0x04
#define RITUAL_LAW		0x08
#define RITUAL_HI_CHAOS	0x10
#define RITUAL_HI_NEUTRAL	0x20
#define RITUAL_HI_LAW	0x40
#define RITUAL_DONE		(RITUAL_CHAOS|RITUAL_NEUTRAL|RITUAL_LAW)
#define HI_RITUAL_DONE	(RITUAL_HI_CHAOS|RITUAL_HI_NEUTRAL|RITUAL_HI_LAW)
	Bitfield(peaceful_pets,1);	/* pets don't attack peaceful monsters */
	Bitfield(uiearepairs,1);	/* Knows how to repair Imperial Elven Armor */
	/* 11 free bits */
	
	int oonaenergy;				/* Record the energy type used by Oona in your game. (Worm that Walks switches?) */
	int brand_otyp;				/* Record the otyp of Fire and Frost Brand in this game */
	int ring_wishes;			/* Record the how many wishes were/should be in the castle ring */
	unsigned udg_cnt;		/* timer for wizard intervention WRONG?:how long you have been demigod */
	unsigned ill_cnt;		/* timer for illurien intervention */
	unsigned yel_cnt;		/* timer for stranger intervention */
	struct u_event	uevent;		/* certain events have happened */
	struct u_have	uhave;		/* you're carrying special objects */
	struct u_conduct uconduct;	/* KMH, conduct */
	struct attribs	acurr,		/* your current attributes (eg. str)*/
			aexe,		/* for gain/loss via "exercise" */
			abon,		/* your bonus attributes (eg. str) */
			amax,		/* your max attributes (eg. str) */
			atemp,		/* used for temporary loss/gain */
			atime;		/* used for loss/gain countdown */
	long exerchkturn;	/* Stat Excercise: What turn is the next exerchk? */		
	align	ualign;			/* character alignment */
#define UGOD_CONVERT	2
#define UGOD_ORIGINAL	1
#define UGOD_CURRENT	0
	int ugodbase[UGOD_CONVERT];
	schar uluck, moreluck;		/* luck and luck bonus */
	int luckturn;
#define Luck	(u.uluck + u.moreluck)
#define LUCKADD		3	/* added value when carrying luck stone */
#define DIELUCK		4	/* subtracted from current luck on final death */
#define LUCKMAX		10	/* on moonlit nights 11 */
#define LUCKMIN		(-10)
	schar	uhitinc;		/* bonus to-hit chance */
	schar	udaminc;		/* bonus damage */
	int		ucarinc;		/* bonus carrying capacity */
	schar	uacinc;			/* bonus AC (not spell/divine) */
	schar	uac;
	schar	udr;
	uchar	uspellprot;		/* protection by SPE_PROTECTION */
	uchar	udrunken;		/* drunkeness level (based on total number of potions of booze drunk) */
	uchar	usptime;		/* #moves until uspellprot-- */
	uchar	uspmtime;		/* #moves between uspellprot-- */
	uchar	sowdisc;		/* sowing discord (spirit special attack) */
	unsigned long long int spells_maintained;
	int maintained_en_debt;
	int quivered_spell;
	int	uhp, uhpmax, uhprolled, uhpmultiplier, uhpbonus, uhpmod;
	int	uen, uenmax, uenrolled, uenmultiplier, uenbonus;			/* magical energy - M. Stephenson */
	/*"Real" numbers for a WtWalk's non-mask-based HP*/
	int uhp_real, uhpmax_real, uhprolled_real, uhpbonus_real, uhpmod_real;
	int uen_real, uenmax_real, uenrolled_real, uenbonus_real;
	int ugifts;			/* number of artifacts bestowed */
	int uartisval;		/* approximate strength of artifacts bestowed and wished for */
	int ucultsval;		/* approximate strength of wished artifacts and gifts bestowed */
	int ublessed, ublesscnt;	/* blessing/duration from #pray */
	long usaccredit;		/* credit towards next gift */
	boolean cult_atten[MAX_CULTS];	/* for having started with a cult */
#define shubbie_atten		cult_atten[GOAT_CULT]
#define silver_atten		cult_atten[FLAME_CULT]
#define yog_sothoth_atten	cult_atten[SOTH_CULT]
#define good_neighbor_atten	cult_atten[RAT_CULT]
	long ucultcredit[MAX_CULTS];	/* for doing deeds for the cult, spendable */
#define shubbie_credit			ucultcredit[GOAT_CULT]
#define silver_credit			ucultcredit[FLAME_CULT]
#define yog_sothoth_credit		ucultcredit[SOTH_CULT]
#define good_neighbor_credit	ucultcredit[RAT_CULT]
	long ucultcredit_total[MAX_CULTS];	/* total credit accumulated for the cult */ 
#define shubbie_devotion		ucultcredit_total[GOAT_CULT]
#define silver_devotion			ucultcredit_total[FLAME_CULT]
#define yog_sothoth_devotion	ucultcredit_total[SOTH_CULT]
#define good_neighbor_devotion	ucultcredit_total[RAT_CULT]
	long ucultmutagen[MAX_CULTS];	/* total mutagen taken from the cult */
#define shubbie_mutagen			ucultmutagen[GOAT_CULT]
#define yog_sothoth_mutagen		ucultmutagen[SOTH_CULT]
	long ucultmutations[MAX_CULTS];	/* total mutations taken from the cult */
#define shubbie_mutations		ucultmutations[GOAT_CULT]
#define yog_sothoth_mutations	ucultmutations[SOTH_CULT]
	d_level silver_flame_z; 
	xchar s_f_x, s_f_y; 
	long lastprayed;
	long lastslept;
	long nextsleep;
	long role_technique_turn;
#define whisperturn role_technique_turn
#define kiaiturn role_technique_turn
	int regen_blocked;
	uchar lastprayresult, reconciled;
#define	PRAY_NONE	0
#define	PRAY_GOOD	1
#define	PRAY_BAD	2
#define	PRAY_GIFT	3
#define	PRAY_ANGER	4
#define	PRAY_CONV	5
#define PRAY_INPROG	6
#define PRAY_IGNORED	7

#define	REC_NONE	0
#define	REC_REC		1
#define	REC_MOL		2

#ifndef GOLDOBJ
	long	ugold, ugold0;
#else
	long	umoney0;
#endif
	long	uexp, urexp;
	long	ucleansed;	/* to record moves when player was cleansed */
	long	usleep;		/* sleeping; monstermove you last started */
	long 	last_used_move;		/* Partial action: last used turn */
	int		last_used_movement;	/* Partial action: last used turn segment */
	int 	ustdy;		/* to record extra damage to be dealt due to having been studied */
	uchar	ubranch;	/* record which branch the village wizard opened up for you*/
#define ICE_CAVES	1
#define BLACK_FOREST	2
#define GNOMISH_MINES	3
#define DISMAL_SWAMP	4
#define ARCHIPELAGO	5
	int 	uencouraged;/* to record the buff from tame encouragement songs */
	int		uentangled_otyp; /* to record the otyp of an item entangling you */
	long	uentangled_oid; /* to record the oid of the item entangling you */
	long int spawnedGold; /* to record total amount of gold spawned in a game */
		int	utats; /*Used to store Fell's tattoo information*/
#define TAT_HOURGLASS	0x0001 
#define TAT_FALCHION	0x0002 
#define TAT_KESTREL	0x0004 
#define TAT_BULWARK	0x0008
#define TAT_FOUNTAIN	0x0010
#define TAT_CROESUS	0x0020
#define TAT_UNKNOWN	0x0040
#define TAT_WILLOW	0x0080
#define TAT_HAMMER	0x0100
#define TAT_SPEARHEAD	0x0200
#define TAT_CRYSTAL_ORB	0x0400
#define TAT_HYPHEN	0x0800
#define TAT_FLAMING_WHIP 0x1000
#define NUM_TATS	13
	long int total_damage; /* to record total amount of damage in game */
	int 	usanity;	/* to record level of sanity */
	unsigned long long int 	umadness;	/* to afflictions */
#define	MAD_DELUSIONS		0x0000000000000001LL
#define	MAD_REAL_DELUSIONS	0x0000000000000002LL
#define	MAD_SANCTITY		0x0000000000000004LL
#define	MAD_GLUTTONY		0x0000000000000008LL
#define	MAD_SPORES			0x0000000000000010LL
#define	MAD_FRIGOPHOBIA		0x0000000000000020LL
#define	MAD_CANNIBALISM		0x0000000000000040LL
#define	MAD_RAGE			0x0000000000000080LL
#define	MAD_ARGENT_SHEEN	0x0000000000000100LL
#define	MAD_SUICIDAL		0x0000000000000200LL
#define	MAD_NUDIST			0x0000000000000400LL
#define	MAD_OPHIDIOPHOBIA	0x0000000000000800LL
#define	MAD_ARACHNOPHOBIA	0x0000000000001000LL
#define	MAD_ENTOMOPHOBIA	0x0000000000002000LL
#define	MAD_THALASSOPHOBIA	0x0000000000004000LL
#define	MAD_PARANOIA		0x0000000000008000LL
#define	MAD_TALONS			0x0000000000010000LL
#define	MAD_COLD_NIGHT		0x0000000000020000LL
#define	MAD_OVERLORD		0x0000000000040000LL
#define	MAD_DREAMS			0x0000000000080000LL
#define	MAD_NON_EUCLID		0x0000000000100000LL
#define	MAD_SPIRAL			0x0000000000200000LL
#define	MAD_HELMINTHOPHOBIA	0x0000000000400000LL
#define	MAD_GOAT_RIDDEN		0x0000000000800000LL
#define	MAD_FRENZY			0x0000000001000000LL
#define	MAD_THOUSAND_MASKS	0x0000000002000000LL
#define	MAD_FORMICATION		0x0000000004000000LL
#define	MAD_HOST			0x0000000008000000LL
#define	MAD_SCIAPHILIA		0x0000000010000000LL
#define	MAD_FORGETFUL		0x0000000020000000LL
#define	MAD_TOO_BIG			0x0000000040000000LL
#define	MAD_APOSTASY		0x0000000080000000LL
#define	MAD_ROTTING			0x0000000100000000LL
#define	MAD_REACHER			0x0000000200000000LL
#define	MAD_SCORPIONS		0x0000000400000000LL
#define	MAD_VERMIN			0x0000000800000000LL
#define	LAST_MADNESS		MAD_SCORPIONS
	int 	uinsight;	/* to record level of insight */
	/*Insight rate calculation: 40: "high insight" 300: "Approximate per-turn WoYendor intervention rate" 5: "total number of harmful effects" */
#define INSIGHT_RATE (40*300*5)
#define COA_PROB	 (max(1, 10000*pow(.95,(Role_if(PM_ANACHRONOUNBINDER)?max(0,Insight-100):Insight))))
	int 	uimpurity;	/* to record level of impurity */
	Bitfield(uimp_meat, 4);
	Bitfield(uimp_blood, 4);
	Bitfield(uimp_bodies, 4);
	Bitfield(uimp_death_magic, 4);
	Bitfield(uimp_goo_transcendence, 6);
	Bitfield(uimp_theft, 4);
	Bitfield(uimp_murder, 4);
	Bitfield(uimp_bloodlust, 4);
	Bitfield(uimp_graverobbery, 4);
	Bitfield(uimp_god_anger, 4);
	Bitfield(uimp_illness, 4);
	Bitfield(uimp_dirtiness, 4);
	Bitfield(uimp_disaster, 4);
	Bitfield(uimp_seduction, 4);
	Bitfield(uimp_deep_one, 4);
	Bitfield(uimp_betrayal, 4);
	Bitfield(uimp_kuo_toa, 4);
	Bitfield(uimp_ibite, 4);
	Bitfield(uimp_mind_flayers, 4);
	Bitfield(uimp_rot, 4);
	Bitfield(uimp_poison, 4);
	Bitfield(uimp_curse, 4); //60/+30 eve/+12 bullets?
	int 	ureanimation_research;	/* to record progress on reanimation */
	//Power 1: raise crazed corpses
	//Power 2: Summon blood creatures
	//Power 3: Upgrades?
	long ureanimation_upgrades;
	int antenae_upgrades;
// #define ANTENNA_BOLT	0x0001L
// #define ANTENNA_ERRANT 	0x0002L
// #define ANTENNA_BOIL 	0x0004L
// #define ANTENNA_REJECT 	0x0008L
#define	RE_BOLT_RES		0x00000001L
#define	RE_WATER_RES	0x00000002L
#define	RE_CLAIR		0x00000004L
#define	RE_CLONE_SELF	0x00000008L
#define	ANTENNA_ERRANT	0x00000010L
#define	ANTENNA_BOLT	0x00000020L
#define	ANTENNA_REJECT	0x00000040L
#define	LAMP_PHASE		0x00000080L
#define REANIMATION_MAX LAMP_PHASE
#define REANIMATION_COUNT 8
#define check_reanimation(upgrade)	(u.ureanimation_upgrades&(upgrade))
#define add_reanimation(upgrade)	(u.ureanimation_upgrades|=(upgrade))
	int 	uparasitology_research;	/* to record progress on parasitology */
	char brainsuckers;
	char mm_up;
	char explosion_up;
	char jellyfish;
	char cuckoo;
	// int 	usaprobiology_research; /* to record progress on rot */
	int 	udefilement_research; /* to record progress on defilement */
	int mental_scores_down;
	//Path 1: Preservation
	long upreservation_upgrades;
#define PRESERVE_REDUCE_HUNGER 0x00000001L
#define PRESERVE_PREVENT_ABUSE 0x00000002L
#define PRESERVE_GAIN_DR 	   0x00000004L
#define PRESERVE_COLD_RES 	   0x00000008L
#define PRESERVE_SLEEP_RES 	   0x00000010L
#define PRESERVE_GAIN_DR_2 	   0x00000020L
//
#define PRESERVE_DEAD_TRUCE	   0x00000040L
#define PRESERVE_MAX		   PRESERVE_DEAD_TRUCE
#define PRESERVE_ROT_TRIGGER   PRESERVE_GAIN_DR
#define check_preservation(upgrade)	(u.upreservation_upgrades&(upgrade))
#define add_preservation(upgrade)	(u.upreservation_upgrades|=(upgrade))
	//Path 2: Steal will
	long uvampire_upgrades;
#define VAMPIRE_THRALLS		   0x00000001L
#define VAMPIRE_MASTERY		   0x00000002L
#define VAMPIRE_BLOOD_RIP	   0x00000004L
#define VAMPIRE_BLOOD_SPIKES   0x00000008L
#define VAMPIRE_GAZE		   0x00000010L
#define VAMPIRE_MAX			   VAMPIRE_GAZE
#define VAMPIRE_COUNT		   5
#define check_vampire(upgrade)	(u.uvampire_upgrades&(upgrade))
#define add_vampire(upgrade)	(u.uvampire_upgrades|=(upgrade))

	long urot_upgrades;
	//Path 3: New life
#define ROT_VOMIT			   0x00000001L
#define ROT_WINGS			   0x00000002L
#define ROT_CLONE			   0x00000004L
#define ROT_TRUCE			   0x00000008L
#define ROT_KIN				   0x00000010L
#define ROT_FEAST			   0x00000020L
#define ROT_CENT			   0x00000040L
#define ROT_STING			   0x00000080L
#define ROT_SPORES			   0x00000100L
#define ROT_MIN				   ROT_VOMIT
#define ROT_MAX				   ROT_SPORES
#define ROT_COUNT			   9
#define check_rot(upgrade)	(u.urot_upgrades&(upgrade))
#define add_rot(upgrade)	(u.urot_upgrades|=(upgrade))
#define remove_rot(upgrade)	(u.urot_upgrades&=~(upgrade))
	// Blood fly swarm
	// Rot enemy
	// Ants and fungi are peaceful
	// Rotting corpses spawn silvermen

	Bitfield(ublood_smithing, 1);
	Bitfield(umagic_smithing, 1);
	Bitfield(uring_lore, 1);
#define IMPURITY_UP(counter) 	if((counter) < 15){\
		if((counter) == 0 || !rn2((counter))){\
			((counter))++;\
			if((counter) == 1 || (counter) == 4 || (counter) == 15)\
				u.uimpurity++;\
		}\
	}
#define TRANSCENDENCE_IMPURITY_UP(force) 	if((u.uimp_goo_transcendence) < 63){\
		if(force || (u.uimp_goo_transcendence) < 4 || !rn2((u.uimp_goo_transcendence/2))){\
			((u.uimp_goo_transcendence))++;\
			if((u.uimp_goo_transcendence) == 1 || (u.uimp_goo_transcendence) == 4 || (u.uimp_goo_transcendence) == 8 || (u.uimp_goo_transcendence) == 16 || (u.uimp_goo_transcendence) == 32 || (u.uimp_goo_transcendence) == 63)\
				u.uimpurity++;\
		}\
	}
	Bitfield(silvergrubs, 1);

	uchar 	wimage;		/* to record if you have the image of a Weeping Angel in your mind */
	int 	umorgul;	/* to record the number of morgul wounds */
	int 	utaneggs;	/* tannin eggs */
	int		udisks;		/* to record how many aphanactonan disks you've read */
	int		uboln;		/* to record how many spirits you've gained from the boln */
	int uinvault;
	int clk_material;	/* clockwork material */
	struct monst *ustuck;
	boolean petattacked;
	boolean pethped;
#ifdef STEED
	struct monst *usteed;
	long ugallop;
	int urideturns;
#endif
	int	umortality;		/* how many times you died */
	int ugrave_arise; /* you die and become something aside from a ghost */
	time_t	ubirthday;		/* real world time when game began */
	
	int	weapon_slots;		/* unused skill slots */
	int	skills_advanced;		/* # of advances made so far */
	xchar	skill_record[P_SKILL_LIMIT];	/* skill advancements */
	struct skills weapon_skills[P_NUM_SKILLS];
	boolean twoweap;		/* KMH -- Using two-weapon combat */
	int divetimer;			/* how long you can stay under water */
	
	int role_variant;	/*Records what variant of your role you are.*/

	int umabil;	/*Monk mystic attacks active*/
#define SURGE_PUNCH		0x0001
#define FORCE_PUNCH		0x0002
#define CHI_HEALING		0x0004
#define SPIRIT_PUNCH		0x0008
#define FLICKER_PUNCH		0x0010
#define ABSORPTIVE_PUNCH	0x0020
#define ENERGY_FORM		0x0040

#define SURGE_PUNCH_LVL		0
#define FORCE_PUNCH_LVL		4
#define CHI_HEALING_LVL		6
#define SPIRIT_PUNCH_LVL	10
#define FLICKER_PUNCH_LVL	8
#define ABSORPTIVE_PUNCH_LVL	2
	long	wardsknown;	/* known wards */
#define	WARD_ELBERETH		(0x1L<<0)
#define WARD_HEPTAGRAM		(0x1L<<1)
#define WARD_GORGONEION		(0x1L<<2)
#define WARD_ACHERON		(0x1L<<3)
#define WARD_PENTAGRAM		(0x1L<<4)
#define WARD_HEXAGRAM		(0x1L<<5)
#define WARD_HAMSA			(0x1L<<6)
#define WARD_ELDER_SIGN		(0x1L<<7)
#define WARD_EYE			(0x1L<<8)
#define WARD_QUEEN			(0x1L<<9)
#define WARD_CAT_LORD		(0x1L<<10)
#define WARD_GARUDA			(0x1L<<11)
#define WARD_CTHUGHA		(0x1L<<12)
#define WARD_ITHAQUA		(0x1L<<13)
#define WARD_KARAKAL		(0x1L<<14)
#define WARD_YELLOW			(0x1L<<15)
#define WARD_TRANSIT		(0x1L<<16)
#define WARD_STABILIZE		(0x1L<<17)
#define WARD_TOUSTEFNA		(0x1L<<18)
#define WARD_DREPRUN		(0x1L<<19)
#define WARD_OTTASTAFUR		(0x1L<<20)
#define WARD_KAUPALOKI		(0x1L<<21)
#define WARD_VEIOISTAFUR	(0x1L<<22)
#define WARD_THJOFASTAFUR	(0x1L<<23)
#define NUM_WARDS	23

	
	int sealorder[31];
	long	sealsKnown;
	long	specialSealsKnown;
#define SEAL_AHAZU					0x0000001L
#define SEAL_AMON					0x0000002L
#define SEAL_ANDREALPHUS			0x0000004L
#define SEAL_ANDROMALIUS			0x0000008L
#define SEAL_ASTAROTH				0x0000010L
#define SEAL_BALAM					0x0000020L
#define SEAL_BERITH					0x0000040L
#define SEAL_BUER					0x0000080L
#define SEAL_CHUPOCLOPS				0x0000100L
#define SEAL_DANTALION				0x0000200L
#define SEAL_ECHIDNA				0x0000400L
#define SEAL_EDEN					0x0000800L
	long	edenshield;
#define SEAL_ENKI					0x0001000L
#define SEAL_EURYNOME				0x0002000L
	int		eurycounts;
#define SEAL_EVE					0x0004000L
#define SEAL_FAFNIR					0x0008000L
#define SEAL_HUGINN_MUNINN			0x0010000L
#define SEAL_IRIS					0x0020000L
	long	irisAttack;
#define SEAL_JACK					0x0040000L
#define SEAL_MALPHAS				0x0080000L
#define SEAL_MARIONETTE				0x0100000L
#define SEAL_MOTHER					0x0200000L
#define SEAL_NABERIUS				0x0400000L
#define SEAL_ORTHOS					0x0800000L
	int		orthocounts;
#define SEAL_OSE					0x1000000L
	char	osepro[128]; /*Lots of extra space*/
	char	osegen[128];
#define SEAL_OTIAX					0x2000000L
	long	otiaxAttack;
#define SEAL_PAIMON					0x4000000L
#define SEAL_SHIRO					0x8000000L
#define SEAL_SIMURGH				0x10000000L
#define SEAL_TENEBROUS				0x20000000L
#define SEAL_YMIR					0x40000000L
//Special flag for lookup tables, indicating that it is a quest spirit and should be treated as such
#define SEAL_SPECIAL				0x80000000L

//The remaining seals (Dahlver-Nar, Acererak, and the Numina) can't be learned in any way other than binder class features
#define SEAL_DAHLVER_NAR			0x00000001L
#define SEAL_ACERERAK				0x00000002L
#define SEAL_COUNCIL				0x00000004L
#define SEAL_COSMOS					0x00000008L
#define SEAL_LIVING_CRYSTAL			0x00000010L
#define SEAL_TWO_TREES				0x00000020L
#define SEAL_MISKA					0x00000040L
#define SEAL_NUDZIRATH				0x00000080L
#define SEAL_ALIGNMENT_THING		0x00000100L
#define SEAL_UNKNOWN_GOD			0x00000200L
#define SEAL_BLACK_WEB				0x00000400L
#define SEAL_YOG_SOTHOTH			0x00000800L
	long	yogAttack;
#define SEAL_NUMINA					0x40000000L
//	long	numina;	//numina does not expire, and can be immediatly re-bound once 30th level is achived if the pact is broken.
	
	int sealTimeout[NUMINA-FIRST_SEAL]; //turn on which spirit will be again eligible for binding.
	
	long spiritSummons;	
	int sealCounts;
	long sealsActive;
	long sealsUsed;
	long specialSealsActive;
	long specialSealsUsed;
	
	int wisSpirits, intSpirits;
	
#define	GATE_SPIRITS	5
#define	QUEST_SPIRIT	5
#define	GPREM_SPIRIT	6
#define	CROWN_SPIRIT	7
#define	ALIGN_SPIRIT	8
#define	OTHER_SPIRIT	9
#define	OUTER_SPIRIT	10
#define	NUM_BIND_SPRITS	11
	//Spirits in order bound:
	long spirit[NUM_BIND_SPRITS];
	long spiritTineA,spiritTineB;
	//Corresponding timeouts (turn on which binding expires):
	long spiritT[NUM_BIND_SPRITS];
	long spiritTineTA,spiritTineTB;
	
	int spiritAC;
	int spiritAttk;
	
#define	PWR_ABDUCTION				 0
#define	PWR_FIRE_BREATH				 1
#define	PWR_TRANSDIMENSIONAL_RAY	 2
#define	PWR_TELEPORT				 3
#define	PWR_JESTER_S_MIRTH			 4
#define	PWR_THIEF_S_INSTINCTS		 5
#define	PWR_ASTAROTH_S_ASSEMBLY		 6
#define	PWR_ASTAROTH_S_SHARDS		 7
#define	PWR_ICY_GLARE				 8
#define	PWR_BALAM_S_ANOINTING		 9
#define	PWR_BLOOD_MERCENARY			10
#define	PWR_SOW_DISCORD				11
#define	PWR_GIFT_OF_HEALING			12
#define	PWR_GIFT_OF_HEALTH			13
#define	PWR_THROW_WEBBING			14
#define	PWR_THOUGHT_TRAVEL			15
#define	PWR_DREAD_OF_DANTALION		16
#define	PWR_EARTH_SWALLOW			17
#define	PWR_ECHIDNA_S_VENOM			18
#define	PWR_SUCKLE_MONSTER			19
#define	PWR_PURIFYING_BLAST			20
#define	PWR_RECALL_TO_EDEN			21
#define	PWR_STARGATE				22
#define	PWR_WALKER_OF_THRESHOLDS	23
#define	PWR_GEYSER					24
#define	PWR_VENGANCE				25
#define	PWR_SHAPE_THE_WIND			26
#define	PWR_THORNS_AND_STONES		27
#define	PWR_BARRAGE					28
#define	PWR_BREATH_POISON			29
#define	PWR_RUINOUS_STRIKE			30
#define	PWR_RAVEN_S_TALONS			31
#define	PWR_HORRID_WILTING			32
#define	PWR_TURN_ANIMALS_AND_HUMANOIDS	33
#define	PWR_REFILL_LANTERN			34
#define	PWR_HELLFIRE				35
#define	PWR_CALL_MURDER				36
#define	PWR_ROOT_SHOUT				37
#define	PWR_PULL_WIRES				38
#define	PWR_DISGUSTED_GAZE			39
#define	PWR_BLOODY_TOUNGE			40
#define	PWR_SILVER_TOUNGE			41
#define	PWR_EXHALATION_OF_THE_RIFT	42
#define	PWR_QUERIENT_THOUGHTS		43
#define	PWR_GREAT_LEAP				44
#define	PWR_MASTER_OF_DOORWAYS		45
#define	PWR_READ_SPELL				46
#define	PWR_BOOK_TELEPATHY			47
#define	PWR_UNITE_THE_EARTH_AND_SKY	48
#define	PWR_HOOK_IN_THE_SKY			49
#define	PWR_ENLIGHTENMENT			50
#define	PWR_DAMNING_DARKNESS		51
#define	PWR_TOUCH_OF_THE_VOID		52
#define	PWR_ECHOS_OF_THE_LAST_WORD	53
#define	PWR_POISON_GAZE				54
#define	PWR_GAP_STEP				55
#define	PWR_MOAN					56
#define	PWR_SWALLOW_SOUL			57
#define	PWR_EMBASSY_OF_ELEMENTS		58
#define	PWR_SUMMON_MONSTER			59
#define	PWR_PSEUDONATURAL_SURGE		60
#define	PWR_SILVER_DEW				61
#define	PWR_GOLDEN_DEW				62
#define	PWR_MIRROR_SHATTER			63
#define	PWR_MIRROR_WALK				64
#define	PWR_FLOWING_FORMS			65
#define	PWR_PHASE_STEP				66
#define	PWR_BLACK_BOLT				67
#define	PWR_WEAVE_BLACK_WEB			68
#define	PWR_MAD_BURST				69
#define	PWR_UNENDURABLE_MADNESS		70
#define	PWR_CONTACT_YOG_SOTHOTH		71
#define	PWR_IDENTIFY_INVENTORY		72
#define	PWR_CLAIRVOYANCE			73
#define	PWR_FIND_PATH				74
#define	PWR_GNOSIS_PREMONITION		75
#define	NUMBER_POWERS				76

	int spiritPOrder[52]; //# of letters in alphabet, capital and lowercase
//	char spiritPLetters[NUMBER_POWERS];
	long spiritPColdowns[NUMBER_POWERS];
	
	
	/* 	variable that keeps track of summoning in your vicinity.
		Only allow 1 per turn, to help reduce summoning cascades. */
	boolean summonMonster;
	/* 	Variable that checks if the Wizard has increased the weight of the amulet */
	boolean uleadamulet;
	/*Deprecated (moved to art-instance variables?): Ugly extra artifact variables workaround.  Spaghetti code alert!*/
	char dracae_pets; /*How many pets have been incarnated by a dracae this game.*/
	unsigned char puzzle_time; /*Time needed to solve the next step of a hyperborean dial puzzle.*/
	unsigned char uhyperborean_steps; /*The most advanced step you've reached on a hyperborean dial puzzle.*/
	int goldkamcount_tame; /*number of tame golden kamerel following you around*/
	int voidChime;
	long rangBell; /*Turn last rang bell of opening on*/
	/*Keter counters*/
	int keter, chokhmah, binah, gevurah, hod, daat, netzach;
	int regifted; /*keeps track of how many artifacts the player has given to the unknown god*/
	int uaesh, uaesh_duration, ukrau, ukrau_duration, uhoon, uhoon_duration,
		uuur, uuur_duration, unaen, unaen_duration, uvaul, uvaul_duration;
	boolean ufirst_light;
	long ufirst_light_timeout;
	boolean ufirst_sky;
	long ufirst_sky_timeout;
	boolean ufirst_life;
	long ufirst_life_timeout;
	boolean ufirst_know;
	long ufirst_know_timeout;
	long thoughts;
#define MAX_GLYPHS (((Role_if(PM_MADMAN) && u.uevent.qcompleted && (Insight >= 20 || u.render_thought)) || Role_if(PM_UNDEAD_HUNTER)) ? 4 : 3)
	long mutations[MUTATION_LISTSIZE];
};	/* end of `struct you' */
#define uclockwork ((Race_if(PM_CLOCKWORK_AUTOMATON) && !Upolyd) || (Upolyd && youmonst.data->mtyp == PM_CLOCKWORK_AUTOMATON))
#define uandroid ((Race_if(PM_ANDROID) && !Upolyd) || (Upolyd && (youmonst.data->mtyp == PM_ANDROID || youmonst.data->mtyp == PM_GYNOID || youmonst.data->mtyp == PM_OPERATOR || youmonst.data->mtyp == PM_COMMANDER)))
#define umechanoid (uclockwork || uandroid)
//BAB
#define BASE_ATTACK_BONUS(wep)	((Role_if(PM_BARBARIAN) || Role_if(PM_ANACHRONOUNBINDER) || Role_if(PM_CONVICT) || Role_if(PM_KNIGHT) || Role_if(PM_ANACHRONONAUT) || \
								Role_if(PM_PIRATE) || Role_if(PM_UNDEAD_HUNTER) || Role_if(PM_SAMURAI) || Role_if(PM_VALKYRIE) || (u.sealsActive&SEAL_BERITH) || \
								(!wep && (martial_bonus() || (u.sealsActive&SEAL_EURYNOME))) || \
								(Role_if(PM_MONK) && wep && is_monk_weapon(wep)) || \
								(wep && is_lightsaber(wep) && (Unblind_telepat || (Blind && Blind_telepat)))) ? 1.00 :\
							 (Role_if(PM_ARCHEOLOGIST) || Role_if(PM_EXILE) || Role_if(PM_CAVEMAN) || Role_if(PM_MONK) || \
								Role_if(PM_NOBLEMAN) || Role_if(PM_PRIEST) || Role_if(PM_ROGUE) || Role_if(PM_RANGER) || \
								(u.sealsActive&SEAL_ENKI) || (Blind_telepat && wep && is_lightsaber(wep))) ? 0.75 :\
							 (Role_if(PM_BARD) || Role_if(PM_HEALER) || Role_if(PM_TOURIST) || Role_if(PM_WIZARD) || Role_if(PM_MADMAN)) ? 0.50:\
							  .5) /* Failsafe */


extern long sealKey[34]; /*Defined in u_init.c*/
extern boolean forcesight; /*Defined in u_init.c*/
extern boolean forceblind; /*Defined in u_init.c*/
extern char *wardDecode[26]; /*Defined in spell.c*/
extern int wardMax[18]; /*Defined in engrave.c*/
extern char *sealNames[]; /*Defined in engrave.c*/
extern char *sealTitles[]; /*Defined in engrave.c*/
extern char *andromaliusItems[18]; /*Defined in sounds.c*/
extern long int_spirits; /*Defined in sounds.c*/
extern long wis_spirits; /*Defined in sounds.c*/
extern boolean barrage; /*Defined in dothrow.c*/
extern boolean onlykicks; /*Defined in dokick.c*/
#define Upolyd (u.umonnum != u.umonster)

#endif	/* YOU_H */
