/*	SCCS Id: @(#)monflag.h	3.4	1996/05/04	*/
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONFLAG_H
#define MONFLAG_H

//msound tokens
#define MS_SILENT	0	/* makes no sound */
#define MS_BARK		1	/* if full moon, may howl */
#define MS_MEW		2	/* mews or hisses */
#define MS_ROAR		3	/* roars */
#define MS_GROWL	4	/* growls */
#define MS_SQEEK	5	/* squeaks, as a rodent */
#define MS_SQAWK	6	/* squawks, as a bird */
#define MS_HISS		7	/* hisses */
#define MS_BUZZ		8	/* buzzes (killer bee) */
#define MS_GRUNT	9	/* grunts (or speaks own language) */
#define MS_NEIGH	10	/* neighs, as an equine */
#define MS_WAIL		11	/* wails, as a tortured soul */
#define MS_SHEEP	12	/* wails, as a tortured soul */
#define MS_GURGLE	13	/* gurgles, as liquid or through saliva */
#define MS_BURBLE	14	/* burbles (jabberwock) */
#define MS_JUBJUB	15	/* jubjub bird's fear inducing call */
#define MS_ANIMAL	15	/* up to here are animal noises */ //note: checks are <=, so this should be == the last animal sound
#define MS_HOWL		16	/* wakes up others */
#define MS_SHRIEK	17	/* wakes up others, calls monsters */
#define MS_SHOG		18	/* wakes up others, shoggoth style */
#define MS_BONES	19	/* rattles bones (skeleton) */
#define MS_LAUGH	20	/* grins, smiles, giggles, and laughs */
#define MS_MUMBLE	21	/* says something or other */
#define MS_IMITATE	22	/* imitates others (leocrotta) */
#define MS_ORC		MS_GRUNT	/* intelligent brutes */
#define MS_HUMANOID	23	/* generic traveling companion */
#define MS_ARREST	24	/* "Stop in the name of the law!" (was Keystones) */
#define MS_SOLDIER	25	/* army and watchmen expressions */
#define MS_GUARD	26	/* "Please drop that gold and follow me." */
#define MS_DJINNI	27	/* "Thank you for freeing me!" */
#define MS_NURSE	28	/* "Take off your shirt, please." */
#define MS_SEDUCE	29	/* "Hello, sailor." (Nymphs) */
#define MS_VAMPIRE	30	/* vampiric seduction, Vlad's exclamations */
#define MS_BRIBE	31	/* asks for money, or berates you */
#define MS_CUSS		32	/* berates (demons) or intimidates (Wiz) */
#define MS_RIDER	33	/* astral level special monsters */
#define MS_LEADER	34	/* your class leader */
#define MS_NEMESIS	35	/* your nemesis */
#define MS_GUARDIAN	36	/* your leader's guards */
#define MS_SELL		37	/* demand payment, complain about shoplifters */
#define MS_ORACLE	38	/* do a consultation */
#define MS_PRIEST	39	/* ask for contribution; do cleansing */
#define MS_SPELL	40	/* spellcaster not matching any of the above */
#define MS_WERE		41	/* lycanthrope in human form */
#define MS_BOAST	42	/* giants */
#define MS_DREAD	43	/* song of the dread seraphim */
#define MS_OONA		44	/* song of Oona */
#define MS_SONG		45	/* other singers */
#define MS_INTONE	46	/* regular songs plus black flower's songs */
#define MS_FLOWER	47	/* black flower's songs */
#define MS_TRUMPET	48	/* Archon's trumpet */
#define MS_UNCURSE	49	/* Curse merchant */
#define MS_STATS	50	/* Sells stat points */
#define MS_GLYPHS	51	/* Exchanges glyphs for artifacts */
#define MS_SECRETS	52	/* Tells secrets */
#define MS_SCREAM	53	/* Screams in madness */
#define MS_HARROW	54	/* Summon wraiths */
#define MS_APOC		55	/* Dire Revelations */
#define MS_COUGH	56	/* Sick coughing */
#define MS_RIBBIT	57	/* Frog sounds */
#define MS_TATTOO	58	/* Tattoos service */
#define MS_SMITH	59	/* Armorsmith service */
#define MS_PORTAL	60	/* Offer portal options */
#define MS_DYE 		61	/* Sell object dyeing */


#define MR_FIRE			0x0001	/* 1 resists fire */
#define MR_COLD			0x0002	/* 2 resists cold */
#define MR_SLEEP		0x0004	/* 4 resists sleep */
#define MR_DISINT		0x0008	/* 8 resists disintegration */
#define MR_ELEC			0x0010	/* 16 resists electricity */
#define MR_POISON		0x0020	/* 32 resists poison */
#define MR_ACID			0x0040	/* 64 resists acid */
#define MR_STONE		0x0080	/* 128 resists petrification */
#define MR_DRAIN		0x0100	/* 256 resists level drain */
#define MR_SICK			0x0200	/* resists sickness */
#define MR_MAGIC		0x0400	/* has player-style magic resistance */
#define MR_REFLECT		0x0800	/* reflects */
#define MR_MASK			0x0FFF	/* mask that covers all MR resistances */
#define MR_ALL			(MR_STONE|MR_ACID|MR_POISON|MR_ELEC|MR_DISINT|MR_SLEEP|MR_COLD|MR_FIRE|MR_DRAIN|MR_SICK)
/* other conveyances: teleport, teleport control, telepathy */

//MM_ Monster Motility
#define MM_FLY			0x00000001L	/* can fly or float */
#define MM_SWIM			0x00000002L	/* can traverse water */
#define MM_AMORPHOUS	0x00000004L	/* can flow under doors */
#define MM_WALLWALK		0x00000008L	/* can phase thru rock */
#define MM_CLING		0x00000010L	/* can cling to ceiling */
#define MM_TUNNEL		0x00000020L	/* can tunnel thru rock */
#define MM_NEEDPICK		0x00000040L	/* needs pick to tunnel */
#define MM_AMPHIBIOUS	0x00000080L	/* can survive underwater */
#define MM_BREATHLESS	0x00000100L	/* doesn't need to breathe */
#define MM_TPORT		0x00000200L	/* can teleport */
#define MM_TPORT_CNTRL	0x00000400L	/* controls where it teleports to */
#define MM_TENGTPORT	0x00000800L	/* Monster teleports as Tengu */
#define MM_STATIONARY	0x00001000L	/* does not move. */
#define MM_FLOAT		0x00002000L	/* Monster floats instead of truly flying (modifier) */
#define MM_NOTONL		0x00004000L	/* Tries to avoid being on same line as player */
#define MM_FLEETFLEE	0x00008000L	/* Flees if adjacent if it thinks it can outrun you */
#define MM_WEBRIP		0x00010000L	/* Tears through webs instead of getting stuck */
#define MM_DOORBUST		0x00020000L	/* Breaks down locked doors (UNIMPLEMENTED, part of ma_giant currently!) */
#define MM_AQUATIC		0x00040000L	/* Can ONLY survive underwater */

//Monster Thoughts and behavior
#define MT_WANTSAMUL	0x00000001L	/* would like to steal the amulet */
#define MT_WANTSBELL	0x00000002L	/* wants the bell */
#define MT_WANTSBOOK	0x00000004L	/* wants the book */
#define MT_WANTSCAND	0x00000008L	/* wants the candelabrum */
#define MT_WANTSARTI	0x00000010L	/* wants the quest artifact */
#define MT_WAITFORU		0x00000020L	/* waits to see you or get attacked */
#define MT_CLOSE		0x00000040L	/* lets you close unless attacked */
//^MUST FIT WITHIN STRAT_GOAL: 0x000000ffL, see monst.h, wizard.c

#define MT_PEACEFUL		0x00000080L	/* always starts peaceful */
#define MT_DOMESTIC		0x00000100L	/* can be tamed by feeding */
#define MT_WANDER		0x00000200L	/* wanders randomly */
#define MT_STALK		0x00000400L	/* follows you to other levels */
#define MT_ROCKTHROW	0x00000800L	/* throws boulders */
#define MT_GREEDY		0x00001000L	/* likes gold */
#define MT_JEWELS		0x00002000L	/* likes gems */
#define MT_COLLECT		0x00004000L	/* picks up weapons and food */
#define MT_MAGIC		0x00008000L	/* picks up magic items */
#define MT_CONCEAL		0x00010000L	/* hides under objects */
#define MT_HIDE			0x00020000L	/* mimics, blends in with ceiling */
#define MT_MINDLESS		0x00040000L	/* has no mind--golem, zombie, mold */
#define MT_ANIMAL		0x00080000L	/* has animal mind */
#define MT_CARNIVORE	0x00100000L	/* eats corpses */
#define MT_HERBIVORE	0x00200000L	/* eats fruits */
#define MT_HOSTILE		0x00400000L	/* always starts hostile */
#define MT_TRAITOR		0x00800000L	/* slash'em tag. */
#define MT_NOTAKE		0x01000000L	/* doesn't pick up items. */
#define MT_METALLIVORE	0x02000000L	/* eats metal. */
#define MT_MAGIVORE		0x04000000L	/* eats magic */
#define MT_BOLD			0x08000000L	/* recovers from fear quickly */
#define MT_DETACHED		0x10000000L	/* detached from purpose/subsumed but intelligent, mindless but with muse roughly */

#define MT_OMNIVORE		(MT_CARNIVORE|MT_HERBIVORE)	/* eats both */
#define MT_MAID		(MT_MAGIC|MT_COLLECT|MT_JEWELS|MT_GREEDY)	/* tiddies up the dungeon */
#define MT_WANTSALL		(MT_WANTSAMUL|MT_WANTSBELL|MT_WANTSBOOK|MT_WANTSCAND|MT_WANTSARTI)	/* wants any major artifact */
#define MT_COVETOUS		MT_WANTSALL		/* wants something */
#define MT_WAITMASK		(MT_WAITFORU|MT_CLOSE)	/* waiting... */

//Monster Skills
#define MF_MARTIAL_B        0x00000001L   /* basic   martial skill */
#define MF_MARTIAL_S        0x00000002L   /* skilled martial skill */
#define MF_MARTIAL_E        0x00000004L   /* expert  martial skill */
#define MF_BAB_FULL        	0x00000008L   /* full base attack bonus */
#define MF_BAB_HALF        	0x00000010L   /* half base attack bonus */
#define MF_LEVEL_30        	0x00000020L   /* can reach level 30 */
#define MF_LEVEL_45        	0x00000040L   /* can reach level 45 */
#define MF_PHYS_SCALING   	0x00000080L   /* recieves HD-based bonus on physical damage */

//Monster Body plan
#define MB_NOEYES		0x00000001L	/* no eyes to gaze into or blind */
#define MB_NOHANDS		0x00000002L	/* no hands to handle things */
#define MB_NOLIMBS		0x00000004L	/* no arms/legs to kick/wear on */
#define MB_NOHEAD		0x00000008L	/* no head to behead */
#define MB_HUMANOID		0x00000010L	/* has humanoid head/arms/torso */
#define MB_ANIMAL		0x00000020L	/* has animal body */
#define MB_SLITHY		0x00000040L	/* has serpent body */
#define MB_UNSOLID		0x00000080L	/* has no solid or liquid body */
#define MB_THICK_HIDE	0x00000100L	/* has thick hide or scales */
#define MB_OVIPAROUS	0x00000200L	/* can lay eggs */
#define MB_ACID			0x00000400L	/* acidic to eat */
#define MB_POIS			0x00000800L	/* poisonous to eat */
#define MB_CHILL		0x00001000L	/* cold to eat */
#define MB_TOSTY		0x00002000L	/* hot to eat */
#define MB_HALUC		0x00004000L	/* hallucinogenic */
#define MB_MALE			0x00008000L	/* always male */
#define MB_FEMALE		0x00010000L	/* always female */
#define MB_NEUTER		0x00020000L	/* neither male nor female */
#define MB_STRONG		0x00040000L	/* strong (or big) monster */
#define MB_WINGS		0x00080000L	/* has wings */
#define MB_LONGHEAD		0x00100000L	/* has 'long' (animal) head */
#define MB_LONGNECK		0x00200000L	/* has 'long' (snakelike) head and neck */
#define MB_NOFEET		0x00400000L	/* no feet to wear boots */
#define MB_HAS_FEET		0x00800000L	/* does have humanoid feet, even though it has a non-humanoid body plan */
#define MB_CAN_AMULET	0x01000000L	/* can wear an amulet even if it has no head */
#define MB_INDIGESTIBLE	0x02000000L	/* immune to purple worms */
#define MB_INSUBSTANTIAL	0x04000000L	/* Weapons pass through the monster */
#define MB_NOGLOVES		0x08000000L	/* can handle things but has no glove slot */
#define MB_NOHAT		0x10000000L	/* has a head to be beheaded but has no helm slot */
#define MB_SKELETAL		0x20000000L	/* only bones */

#define MB_SNAKELEG	(MB_HUMANOID|MB_SLITHY)
#define MB_CENTAUR	(MB_HUMANOID|MB_ANIMAL)

#define MB_BODYTYPEMASK	(MB_HUMANOID|MB_ANIMAL|MB_SLITHY)
#define MB_HEADMODIMASK	(MB_LONGHEAD|MB_LONGNECK)



//Monster Vision types and other sensorium details
#define MV_NORMAL		0x00000001L		/* can't see more than 1 square in the dark */
#define MV_INFRAVISION	0x00000002L		/* has infravision (does not show squares) */
#define MV_DARKSIGHT	0x00000004L		/* sees in the dark, blinded by light */
#define MV_LOWLIGHT2	0x00000008L		/* sees farther in the dark (2 squares) */
#define MV_LOWLIGHT3	0x00000010L		/* sees farther in the dark (3 squares) */
#define MV_CATSIGHT		0x00000020L		/* sees in the dark, sees in the light, not both at once */
#define MV_ECHOLOCATE	0x00800040L		/* sees via sound, in both light and dark, blinded and stunned by loud noises */
#define MV_BLOODSENSE	0x00000080L		/* detect monsters by seeing their blood (does not show squares, does not require eyes) */
#define MV_LIFESENSE	0x00000100L		/* detect monsters by sensing their life-force (does not show squares, does not require eyes) */
#define MV_EXTRAMISSION	0x00000200L		/* see perfectly in light and dark */
//define MV_BLINDSIGHT	0x00000000L		/* see without eyes OBSOLETE*/
#define MV_TELEPATHIC	0x00000400L		/* Monster is telepathic, detects any non-mindless creatures on the level (does not show squares) */
#define MV_RLYEHIAN		0x00000800L		/* Cannot see into or through water squares. Blind in water */
#define MV_SEE_INVIS	0x00001000L		/* can see invisible creatures (modifier) */
#define MV_DETECTION	0x00002000L		/* detect monsters in line-of-sight (does not show squares) */
#define MV_OMNI			0x00004000L		/* non-square-sight does not require line-of-sight */
#define MV_SCENT		0x00008000L		/* has a sensitive nose or other chemoreception (6 square detection range) */
#define MV_EARTHSENSE	0x00010000L		/* detect monsters by hearing their steps through the ground (blocked by stealth, flying, defness) */

//Monster Game mechanics and bookkeeping
#define MG_REGEN		0x00000001L	/* regenerates hit points */
#define MG_NOPOLY		0x00000002L	/* players mayn't poly into one */
#define MG_MERC			0x00000004L	/* is a guard or soldier */
#define MG_PNAME		0x00000008L	/* monster name is a proper name */
#define MG_LORD			0x00000010L	/* is a lord to its kind */
#define MG_PRINCE		0x00000020L	/* is an overlord to its kind */
#define MG_NASTY		0x00000040L	/* extra-nasty monster (more xp) */
#define MG_INFRAVISIBLE	0x00000080L	/* visible by infravision */
#define MG_OPAQUE		0x00000100L	/* Monster blocks line of sight */
#define MG_DISPLACEMENT	0x00000200L	/* Monster has displacement */
#define MG_HATESSILVER	0x00000400L	/* Monster hates silver */
#define MG_HATESIRON	0x00000800L	/* Monster hates iron/steel */
#define MG_HATESUNHOLY	0x00001000L	/* Monster hates cursed objects */
#define MG_HATESHOLY	0x00002000L	/* Monster hates blessed objects */
#define MG_RIDER		0x00004000L	/* Monster has perfect resurection */
#define MG_DEADLY		0x00008000L	/* Monster's corpse is deadly to taste */
#define MG_TRACKER		0x00010000L	/* Monster can track players by sight or scent (guideline: predators with mv_scent or 'woodsy' inteligent beings) */
#define MG_NOSPELLCOOLDOWN	0x00020000L	/* Monster can cast spells without depending on or increasing mspec_used */
#define MG_RBLUNT	0x00040000L	/* Monster takes reduced damage from blunt weapons */
#define MG_RSLASH	0x00080000L	/* Monster takes reduced damage from slashing weapons */
#define MG_RPIERCE	0x00100000L	/* Monster takes reduced damage from piercing weapons */
#define MG_VBLUNT	(MG_RSLASH|MG_RPIERCE)
#define MG_VSLASH	(MG_RBLUNT|MG_RPIERCE)
#define MG_VPIERCE	(MG_RSLASH|MG_RBLUNT)
#define MG_RALL		(MG_RPIERCE|MG_RSLASH|MG_RBLUNT)	/* Monster takes reduced damage from weapons */
#define MG_WRESIST	0x00200000L	/* Monster takes 1 damage from weapons */
#define MG_NOTAME	0x00400000L	/* Monster can't be tamed (replaces S_HUMAN as the check) */
#define MG_NOWISH	0x00800000L	/* Monster can't be wished for in figurine/statue form */
#define MG_BACKSTAB	0x01000000L	/* Monster does extra damage vs. incapacitated foes (as rogue) */
#define MG_COMMANDER	0x02000000L /* Monster commands its underlings to fight better */
#define MG_SANLOSS		0x04000000L /* Seeing the monster damages sanity */
#define MG_INSIGHT		0x08000000L /* Seeing the monster grants insight */
#define MG_RIDER_HP		0x10000000L /* Has 10d8 HP */
#define MG_FUTURE_WISH	0x20000000L /* Wishable by tourists only */
#define MG_HATESUNBLESSED	0x40000000L /* Monster hates neutral holiness objects */

//Monster rAce
#define MA_UNDEAD		0x00000001L	/* is walking dead */
#define MA_WERE			0x00000002L	/* is a lycanthrope */
#define MA_HUMAN		0x00000004L	/* is a human */
#define MA_ELF			0x00000008L	/* is an elf */
#define MA_DROW			0x00000010L	/* is a drow */
#define MA_DWARF		0x00000020L	/* is a dwarf */
#define MA_GNOME		0x00000040L	/* is a gnome */
#define MA_ORC			0x00000080L	/* is an orc */
#define MA_VAMPIRE		0x00000100L	/* is a vampire */
#define MA_CLOCK		0x00000200L	/* is a clockwork automaton */
#define MA_UNLIVING		0x00000400L	/* isn't alive */
#define MA_PLANT		0x00000800L	/* is a plant (or a fungus.  Which is SORTA a plant) */
#define MA_GIANT		0x00001000L	/* is a giant */
#define MA_INSECTOID	0x00002000L	/* is an insectoid creature */
#define MA_ARACHNID		0x00004000L	/* is an arachind creature */
#define MA_AVIAN		0x00008000L	/* is a bird-like-creature */
#define MA_REPTILIAN	0x00010000L	/* is a reptilian creature */
#define MA_ANIMAL		0x00020000L	/* is a more-or-less mundane animal (or a hybrid of an animal and something else) */
#define MA_AQUATIC		0x00040000L	/* is a water-dwelling creature */
#define MA_DEMIHUMAN	0x00080000L	/* is a humanoid with animal traits */
#define MA_FEY			0x00100000L	/* is a member of the fair folk */
#define MA_ELEMENTAL	0x00200000L	/* is an elemental being */
#define MA_DRAGON		0x00400000L	/* is a dragon */
#define MA_DEMON		0x00800000L	/* is a demon */
#define MA_MINION		0x01000000L	/* is a minion of a deity */
#define MA_PRIMORDIAL	0x02000000L	/* is an ancient race */
#define MA_ET			0x04000000L	/* is an alien race */
#define MA_G_O_O		0x08000000L	/* is a Great Old One */
#define MA_ETHEREAL		0x10000000L	/* is an ethereal being*/

#define MA_XORN			0x20000000L	/* is a Xorn */

#define MZ_TINY		0		/* < 2' */
#define MZ_SMALL	1		/* 2-4' */
#define MZ_MEDIUM	2		/* 4-7' */
#define MZ_HUMAN	MZ_MEDIUM	/* human-sized */
#define MZ_LARGE	3		/* 7-12' */
#define MZ_HUGE		4		/* 12-25' */
#define MZ_GIGANTIC	7		/* off the scale */

//Monster Warded
#define MW_ELDER_SIGN		0x00000001L	/* by the elder sign */
#define MW_EYE_OF_YGG		0x00000002L	/* by the fully-reinforced elder sign */
#define MW_ELDER_EYE_ELEM	0x00000004L	/* by the basic elder elemental eye */
#define MW_ELDER_EYE_ENERGY	0x00000008L	/* by the 4-level elder eye */
#define MW_ELDER_EYE_PLANES	0x00000010L	/* by the 7-level elder eye */


/* for mons[].geno (constant during game) */
#define G_NON_GEN_CORPSE	(G_SPCORPSE|G_NOCORPSE)		/* nongenerated corpse */
#define G_DEPTHS	0x00200000		/* generated only in the lost cities/depths */
#define G_SPCORPSE	0x00100000		/* Corpse is special, and should be non-wishable */
#define G_INSTMSK	0x000FC000		/* mask for insight (five bits, 0 to 63), 11111100000000000000 */
#define	G_S_INST(x)	(x<<14)
#define	G_C_INST(x)	((x&G_INSTMSK)>>14)
#define G_UNIQ		0x00002000		/* generated only once */
#define G_PLANES	0x00001000		/* generated only in planes */
#define G_NOHELL	0x00000800		/* not generated in "hell" */
#define G_HELL		0x00000400		/* generated only in "hell" */
#define G_NOGEN		0x00000200		/* generated only specially */
#define G_SGROUP	0x00000080		/* appear in small groups normally */
#define G_LGROUP	0x00000040		/* appear in large groups normally */
#define G_GENO		0x00000020		/* can be genocided */
#define G_NOCORPSE	0x00000010		/* no corpse left ever */
#define G_FREQ		0x00000007		/* creation frequency mask */

/* for mvitals[].mvflags (variant during game), along with G_NOCORPSE */
#define G_KNOWN		0x0004		/* have been encountered */
#define G_GONE		(G_GENOD|G_EXTINCT)
#define G_GENOD		0x0002		/* have been genocided */
#define G_EXTINCT	0x0001		/* have been extinguished as
					   population control */
#define MV_KNOWS_EGG	0x0008		/* player recognizes egg of this
					   monster type */

/* tactics() may call for a specific spell */
/* 0 = no spell */
       /* attack spells */
#define PSI_BOLT			   1
#define OPEN_WOUNDS			   PSI_BOLT+1
#define MAGIC_MISSILE          OPEN_WOUNDS+1 /* magic missile */
#define DRAIN_LIFE             MAGIC_MISSILE+1  /* drain life */
#define ARROW_RAIN             DRAIN_LIFE+1
//5
#define CONE_OF_COLD           ARROW_RAIN+1  /* cone of cold */
#define LIGHTNING              CONE_OF_COLD+1
#define LIGHTNING_BOLT         LIGHTNING+1
#define FIRE_PILLAR            LIGHTNING_BOLT+1
#define GEYSER                 FIRE_PILLAR+1
//10
#define ACID_RAIN              GEYSER+1
#define ICE_STORM              ACID_RAIN+1
#define HAIL_FLURY             ICE_STORM+1
#define SUMMON_MONS            HAIL_FLURY+1
#define SUMMON_DEVIL           SUMMON_MONS+1
//15
#define DEATH_TOUCH			   SUMMON_DEVIL+1
       /* healing spells */
#define CURE_SELF              DEATH_TOUCH+1  /* healing */
#define MASS_CURE_CLOSE        CURE_SELF+1  /* heal allies */
#define MASS_CURE_FAR          MASS_CURE_CLOSE+1  /* heal allies */
#define RECOVER                MASS_CURE_FAR+1  /* remove afflictions */
       /* divination spells */
//20
#define MAKE_VISIBLE           RECOVER+1
       /* (dis)enchantment spells */
#define HASTE_SELF             MAKE_VISIBLE+1 /* haste self */
#define STUN_YOU               HASTE_SELF+1
#define CONFUSE_YOU            STUN_YOU+1
#define PARALYZE               CONFUSE_YOU+1
//25
#define BLIND_YOU              PARALYZE+1
#define SLEEP                  BLIND_YOU+1 /* sleep */
#define DRAIN_ENERGY           SLEEP+1
#define WEAKEN_STATS           DRAIN_ENERGY+1
#define WEAKEN_YOU			   WEAKEN_STATS+1
//30
#define DESTRY_ARMR            WEAKEN_YOU+1
#define DESTRY_WEPN            DESTRY_ARMR+1
#define EVIL_EYE			   DESTRY_WEPN+1
       /* clerical spells */
#define CURSE_ITEMS            EVIL_EYE+1
#define INSECTS                CURSE_ITEMS+1
//35
#define RAISE_DEAD             INSECTS+1
#define SUMMON_ANGEL           RAISE_DEAD+1
#define SUMMON_ALIEN           SUMMON_ANGEL+1
#define SUMMON_YOUNG           SUMMON_ALIEN+1
#define PLAGUE                 SUMMON_YOUNG+1
//40
#define PUNISH                 PLAGUE+1
#define AGGRAVATION			   PUNISH+1
       /* escape spells */
#define DISAPPEAR              AGGRAVATION+1 /* invisibility */
       /* matter spells */
#define DARKNESS               DISAPPEAR+1
#define SUMMON_SPHERE          DARKNESS+1 /* flame sphere */
//45
#define MAKE_WEB               SUMMON_SPHERE+1
#define DROP_BOULDER           MAKE_WEB+1
#define EARTHQUAKE             DROP_BOULDER+1
#define SLIMIFY                EARTHQUAKE+1
#define TURN_TO_STONE          SLIMIFY+1
       /* unique monster spells */
//50
#define NIGHTMARE              TURN_TO_STONE+1
#define FILTH                  NIGHTMARE+1
#define CLONE_WIZ              FILTH+1
#define STRANGLE               CLONE_WIZ+1
#define MON_FIRA               STRANGLE+1
//55
#define MON_FIRAGA             MON_FIRA+1
#define MON_BLIZZARA           MON_FIRAGA+1
#define MON_BLIZZAGA           MON_BLIZZARA+1
#define MON_THUNDARA           MON_BLIZZAGA+1
#define MON_THUNDAGA           MON_THUNDARA+1
//60
#define MON_FLARE              MON_THUNDAGA+1
#define MON_WARP               MON_FLARE+1
#define MON_POISON_GAS         MON_WARP+1
#define MON_PROTECTION         MON_POISON_GAS+1
#define SOLID_FOG      	       MON_PROTECTION+1
//65
#define ACID_BLAST       	   SOLID_FOG+1
#define PRISMATIC_SPRAY        ACID_BLAST+1
#define SILVER_RAYS            PRISMATIC_SPRAY+1
#define GOLDEN_WAVE            SILVER_RAYS+1
#define VULNERABILITY          GOLDEN_WAVE+1
//70
#define MASS_HASTE             VULNERABILITY+1
#define MON_TIME_STOP          MASS_HASTE+1
#define TIME_DUPLICATE         MON_TIME_STOP+1
#define NAIL_TO_THE_SKY        TIME_DUPLICATE+1
#define STERILITY_CURSE        NAIL_TO_THE_SKY+1
//75
#define DISINT_RAY             STERILITY_CURSE+1
#define MON_WARP_THROW         DISINT_RAY+1
#define MAGM_BLAST             MON_WARP_THROW+1
#define SUMMON_TANNIN          MAGM_BLAST+1
#define DISINTEGRATION         SUMMON_TANNIN+1
//80
#define TREMOR                 DISINTEGRATION+1
#define INCARCERATE            TREMOR+1
#define MUMMY_CURSE            INCARCERATE+1
#define YELLOW_DEAD            MUMMY_CURSE+1
#define MON_CANCEL             YELLOW_DEAD+1
#define STARFALL               MON_CANCEL+1
//85
#define EARTH_CRACK            STARFALL+1
#define MON_AURA_BOLT          EARTH_CRACK+1
#define RAIN                   MON_AURA_BOLT+1
#define BLOOD_RAIN             RAIN+1
#define STEAM_GEYSER           BLOOD_RAIN+1
//90
#define MOTHER_S_GAZE          STEAM_GEYSER+1
#define PAIN_BOLT              MOTHER_S_GAZE+1
#define SAN_BOLT               PAIN_BOLT+1
#define DOUBT_BOLT             SAN_BOLT+1
#define BARF_BOLT              DOUBT_BOLT+1
//95
#define BABBLE_BOLT            BARF_BOLT+1
#define MON_SPE_BEARTRAP       BABBLE_BOLT+1
#define PYRO_STORM             MON_SPE_BEARTRAP+1
#define GOD_RAY                PYRO_STORM+1
#define MON_RED_WORD           GOD_RAY+1
//100
#define HYPNOTIC_COLORS        MON_RED_WORD+1
#define CRUSH_BOLT             HYPNOTIC_COLORS+1
#define MADF_BURST             CRUSH_BOLT+1
#define HOLY_BOLT              MADF_BURST+1
#define MIST_WOLVES            HOLY_BOLT+1
//105
#define FORCE_SPHERES          MIST_WOLVES+1
#define PEST_THREADS           FORCE_SPHERES+1

#define MON_LASTSPELL          PEST_THREADS
//Not yet implemented
// #define MON_FIRE               STRANGLE+1
// #define MON_BLIZZARD           MON_FIRAGA+1
// #define MON_THUNDER            MON_BLIZZAGA+1

#endif /* MONFLAG_H */
