/*	SCCS Id: @(#)dungeon.c	3.4	1999/10/30	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>

#include "hack.h"
#include "dgn_file.h"
#include "dlb.h"
#include "display.h"
#include "mapseen.h"

#ifdef OVL1

//define DUNGEON_FILE	"dungeon"

const char *DUNGEON_FILE = "dungeon";
//[] = {
//	"dngnch1", //Temple of Chaos (FF1) TEMPLE_OF_CHAOS
//	"dngnch2", //Aquallor->Mithardir (D&D) MITHARDIR
//	"dngnch3"  //sorta-Mordor (LotR) MORDOR
//};

#define X_START		"x-strt"
#define X_LOCATE	"x-loca"
#define X_GOAL		"x-goal"

struct proto_dungeon {
	struct	tmpdungeon tmpdungeon[MAXDUNGEON];
	struct	tmplevel   tmplevel[LEV_LIMIT];
	s_level *final_lev[LEV_LIMIT];	/* corresponding level pointers */
	struct	tmpbranch  tmpbranch[BRANCH_LIMIT];

	int	start;	/* starting index of current dungeon sp levels */
	int	n_levs;	/* number of tmplevel entries */
	int	n_brs;	/* number of tmpbranch entries */
};

int n_dgns;				/* number of dungeons (used here,  */
					/*   and mklev.c)		   */
static branch *branches = (branch *) 0;	/* dungeon branch list		   */

struct lchoice {
	int idx;
	schar lev[MAXLINFO];
	schar playerlev[MAXLINFO];
	int dgn[MAXLINFO];
	char menuletter;
};

static void FDECL(Fread, (genericptr_t, int, int, dlb *));
STATIC_DCL int FDECL(dname_to_dnum, (const char *));
STATIC_DCL int FDECL(find_branch, (const char *, struct proto_dungeon *));
STATIC_DCL int FDECL(parent_dnum, (const char *, struct proto_dungeon *));
STATIC_DCL int FDECL(level_range, (int,int,int,int,struct proto_dungeon *,int *));
STATIC_DCL int FDECL(parent_dlevel, (const char *, struct proto_dungeon *));
STATIC_DCL int FDECL(correct_branch_type, (struct tmpbranch *));
STATIC_DCL branch *FDECL(add_branch, (int, int, struct proto_dungeon *));
STATIC_DCL void FDECL(add_level, (s_level *));
STATIC_DCL void FDECL(init_level, (int,int,struct proto_dungeon *));
STATIC_DCL int FDECL(possible_places, (int, boolean *, struct proto_dungeon *));
STATIC_DCL int FDECL(pick_level, (boolean *, int));
STATIC_DCL boolean FDECL(place_level, (int, struct proto_dungeon *));
STATIC_DCL boolean FDECL(select_alternate, (int, struct proto_dungeon *));
#ifdef WIZARD
STATIC_DCL const char *FDECL(br_string, (int));
STATIC_DCL void FDECL(print_branch, (winid, int, int, int, BOOLEAN_P, struct lchoice *));
#endif

mapseen *mapseenchn = (struct mapseen *)0;
STATIC_DCL void FDECL(free_mapseen, (mapseen *));
STATIC_DCL mapseen *FDECL(load_mapseen, (int));
STATIC_DCL void FDECL(save_mapseen, (int, mapseen *));
STATIC_DCL mapseen *FDECL(find_mapseen, (d_level *));
STATIC_DCL void FDECL(print_mapseen, (winid,mapseen *,BOOLEAN_P));
STATIC_DCL boolean FDECL(interest_mapseen, (mapseen *));
STATIC_DCL char *FDECL(seen_string, (XCHAR_P, const char *));
STATIC_DCL const char *FDECL(br_string2, (branch *));

#ifdef DEBUG
#define DD	dungeons[i]
STATIC_DCL void NDECL(dumpit);

STATIC_OVL void
dumpit()
{
	int	i;
	s_level	*x;
	branch *br;

	for(i = 0; i < n_dgns; i++)  {
	    fprintf(stderr, "\n#%d \"%s\" (%s):\n", i,
				DD.dname, DD.proto);
	    fprintf(stderr, "    num_dunlevs %d, dunlev_ureached %d\n",
				DD.num_dunlevs, DD.dunlev_ureached);
	    fprintf(stderr, "    depth_start %d, ledger_start %d\n",
				DD.depth_start, DD.ledger_start);
	    fprintf(stderr, "    flags:%s%s%s\n",
		    DD.flags.rogue_like ? " rogue_like" : "",
		    DD.flags.maze_like  ? " maze_like"  : "",
		    DD.flags.hellish    ? " hellish"    : "");
	    getchar();
	}
	fprintf(stderr,"\nSpecial levels:\n");
	for(x = sp_levchn; x; x = x->next) {
	    fprintf(stderr, "%s (%d): ", x->proto, x->rndlevs);
	    fprintf(stderr, "on %d, %d; ", x->dlevel.dnum, x->dlevel.dlevel);
	    fprintf(stderr, "flags:%s%s%s%s\n",
		    x->flags.rogue_like	? " rogue_like" : "",
		    x->flags.maze_like  ? " maze_like"  : "",
		    x->flags.hellish    ? " hellish"    : "",
		    x->flags.town       ? " town"       : "");
	    getchar();
	}
	fprintf(stderr,"\nBranches:\n");
	for (br = branches; br; br = br->next) {
	    fprintf(stderr, "%d: %s, end1 %d %d, end2 %d %d, %s\n",
		br->id,
		br->type == BR_STAIR ? "stair" :
		    br->type == BR_NO_END1 ? "no end1" :
		    br->type == BR_NO_END2 ? "no end2" :
		    br->type == BR_PORTAL  ? "portal"  :
					     "unknown",
		br->end1.dnum, br->end1.dlevel,
		br->end2.dnum, br->end2.dlevel,
		br->end1_up ? "end1 up" : "end1 down");
	}
	getchar();
	fprintf(stderr,"\nDone\n");
	getchar();
}
#endif

/* Save the dungeon structures. */
void
save_dungeon(fd, perform_write, free_data)
    int fd;
    boolean perform_write, free_data;
{
    branch *curr, *next;
	mapseen *curr_ms, *next_ms;
    int    count;

    if (perform_write) {
	bwrite(fd, (genericptr_t) &n_dgns, sizeof n_dgns);
	bwrite(fd, (genericptr_t) dungeons, sizeof(dungeon) * (unsigned)n_dgns);
	bwrite(fd, (genericptr_t) &dungeon_topology, sizeof dungeon_topology);
	bwrite(fd, (genericptr_t) tune, sizeof tune);

	for (count = 0, curr = branches; curr; curr = curr->next)
	    count++;
	bwrite(fd, (genericptr_t) &count, sizeof(count));

	for (curr = branches; curr; curr = curr->next)
	    bwrite(fd, (genericptr_t) curr, sizeof (branch));

	count = maxledgerno();
	bwrite(fd, (genericptr_t) &count, sizeof count);
	bwrite(fd, (genericptr_t) level_info,
			(unsigned)count * sizeof (struct linfo));
	bwrite(fd, (genericptr_t) &inv_pos, sizeof inv_pos);

	for (count = 0, curr_ms = mapseenchn; curr_ms; curr_ms = curr_ms->next)
		count++;
	bwrite(fd, (genericptr_t) &count, sizeof(count));

	for (curr_ms = mapseenchn; curr_ms; curr_ms = curr_ms->next)
		save_mapseen(fd, curr_ms);
    }

    if (free_data) {
		for (curr = branches; curr; curr = next) {
			next = curr->next;
			free((genericptr_t) curr);
		}
		branches = 0;
		for (curr_ms = mapseenchn; curr_ms; curr_ms = next_ms) {
			next_ms = curr_ms->next;
			if (curr_ms->custom)
				free((genericptr_t)curr_ms->custom);
			free((genericptr_t) curr_ms);
		}
		mapseenchn = 0;
    }
}

/* Restore the dungeon structures. */
void
restore_dungeon(fd)
    int fd;
{
    branch *curr, *last;
	mapseen *curr_ms, *last_ms;
    int    count, i;

    mread(fd, (genericptr_t) &n_dgns, sizeof(n_dgns));
    mread(fd, (genericptr_t) dungeons, sizeof(dungeon) * (unsigned)n_dgns);
    mread(fd, (genericptr_t) &dungeon_topology, sizeof dungeon_topology);
    mread(fd, (genericptr_t) tune, sizeof tune);

    last = branches = (branch *) 0;

    mread(fd, (genericptr_t) &count, sizeof(count));
    for (i = 0; i < count; i++) {
	curr = (branch *) alloc(sizeof(branch));
	mread(fd, (genericptr_t) curr, sizeof(branch));
	curr->next = (branch *) 0;
	if (last)
	    last->next = curr;
	else
	    branches = curr;
	last = curr;
    }

    mread(fd, (genericptr_t) &count, sizeof(count));
    if (count >= MAXLINFO)
	panic("level information count larger (%d) than allocated size", count);
    mread(fd, (genericptr_t) level_info, (unsigned)count*sizeof(struct linfo));
    mread(fd, (genericptr_t) &inv_pos, sizeof inv_pos);

	mread(fd, (genericptr_t) &count, sizeof(count));
	last_ms = (mapseen *) 0;
	for (i = 0; i < count; i++) {
		curr_ms = load_mapseen(fd);
		curr_ms->next = (mapseen *) 0;
		if (last_ms)
			last_ms->next = curr_ms;
		else
			mapseenchn = curr_ms;
		last_ms = curr_ms;
	}
}

static void
Fread(ptr, size, nitems, stream)
	genericptr_t	ptr;
	int	size, nitems;
	dlb	*stream;
{
	int cnt;

	if((cnt = dlb_fread(ptr, size, nitems, stream)) != nitems) {
	    panic(
 "Premature EOF on dungeon description file!\r\nExpected %d bytes - got %d.",
		  (size * nitems), (size * cnt));
	    terminate(EXIT_FAILURE);
	}
}

STATIC_OVL int
dname_to_dnum(s)
const char	*s;
{
	int	i;

	for (i = 0; i < n_dgns; i++)
	    if (!strcmp(dungeons[i].dname, s)) return i;

	panic("Couldn't resolve dungeon number for name \"%s\".", s);
	/*NOT REACHED*/
	return (int)0;
}

s_level *
find_level(s)
	const char *s;
{
	s_level *curr;
	for(curr = sp_levchn; curr; curr = curr->next)
	    if (!strcmpi(s, curr->proto)) break;
	return curr;
}

/* Find the branch that links the named dungeon. */
STATIC_OVL int
find_branch(s, pd)
	const char *s;		/* dungeon name */
	struct proto_dungeon *pd;
{
	int i;

	if (pd) {
	    for (i = 0; i < pd->n_brs; i++)
		if (!strcmp(pd->tmpbranch[i].name, s)) break;
	    if (i == pd->n_brs) panic("find_branch: can't find %s", s);
	} else {
	    /* support for level tport by name */
	    branch *br;
	    const char *dnam;

	    for (br = branches; br; br = br->next) {
		dnam = dungeons[br->end2.dnum].dname;
		if (!strcmpi(dnam, s) ||
			(!strncmpi(dnam, "The ", 4) && !strcmpi(dnam + 4, s)))
		    break;
	    }
	    i = br ? ((ledger_no(&br->end1) << 8) | ledger_no(&br->end2)) : -1;
	}
	return i;
}


/*
 * Find the "parent" by searching the prototype branch list for the branch
 * listing, then figuring out to which dungeon it belongs.
 */
STATIC_OVL int
parent_dnum(s, pd)
const char	   *s;	/* dungeon name */
struct proto_dungeon *pd;
{
	int	i;
	int	pdnum;

	i = find_branch(s, pd);
	/*
	 * Got branch, now find parent dungeon.  Stop if we have reached
	 * "this" dungeon (if we haven't found it by now it is an error).
	 */
	for (pdnum = 0; strcmp(pd->tmpdungeon[pdnum].name, s); pdnum++)
	    if ((i -= pd->tmpdungeon[pdnum].branches) < 0)
		return(pdnum);

	panic("parent_dnum: couldn't resolve branch.");
	/*NOT REACHED*/
	return (int)0;
}

/*
 * Return a starting point and number of successive positions a level
 * or dungeon entrance can occupy.
 *
 * Note: This follows the acouple (instead of the rcouple) rules for a
 *	 negative random component (rand < 0).  These rules are found
 *	 in dgn_comp.y.  The acouple [absolute couple] section says that
 *	 a negative random component means from the (adjusted) base to the
 *	 end of the dungeon.
 */
STATIC_OVL int
level_range(dgn, base, rand, chain, pd, adjusted_base)
	int	dgn;
	int	base, rand, chain;
	struct proto_dungeon *pd;
	int *adjusted_base;
{
	int lmax = dungeons[dgn].num_dunlevs;

	if (chain >= 0) {		 /* relative to a special level */
	    s_level *levtmp = pd->final_lev[chain];
	    if (!levtmp) panic("level_range: empty chain level!");

	    base += levtmp->dlevel.dlevel;
	} else {			/* absolute in the dungeon */
	    /* from end of dungeon */
	    if (base < 0) base = (lmax + base + 1);
	}

	if (base < 1)
	    panic("level_range: base value out of range (<0)");
	if (base > lmax)
	    panic("level_range: base value out of range (base:%d > lmax:%d)", base, lmax);

	*adjusted_base = base;

	if (rand == -1) {	/* from base to end of dungeon */
	    return (lmax - base + 1);
	} else if (rand) {
	    /* make sure we don't run off the end of the dungeon */
	    return (((base + rand - 1) > lmax) ? lmax-base+1 : rand);
	} /* else only one choice */
	return 1;
}

STATIC_OVL int
parent_dlevel(s, pd)
	const char	*s;
	struct proto_dungeon *pd;
{
	int i, j, num, base, dnum = parent_dnum(s, pd);
	branch *curr;


	i = find_branch(s, pd);
	num = level_range(dnum, pd->tmpbranch[i].lev.base,
					      pd->tmpbranch[i].lev.rand,
					      pd->tmpbranch[i].chain,
					      pd, &base);

	/* KMH -- Try our best to find a level without an existing branch */
	i = j = rn2(num);
	do {
		if (++i >= num) i = 0;
		for (curr = branches; curr; curr = curr->next)
			if ((curr->end1.dnum == dnum && curr->end1.dlevel == base+i) ||
				(curr->end2.dnum == dnum && curr->end2.dlevel == base+i))
				break;
	} while (curr && i != j);
	return (base + i);
}

/* Convert from the temporary branch type to the dungeon branch type. */
STATIC_OVL int
correct_branch_type(tbr)
    struct tmpbranch *tbr;
{
    switch (tbr->type) {
	case TBR_STAIR:		return BR_STAIR;
	case TBR_NO_UP:		return tbr->up ? BR_NO_END1 : BR_NO_END2;
	case TBR_NO_DOWN:	return tbr->up ? BR_NO_END2 : BR_NO_END1;
	case TBR_PORTAL:	return BR_PORTAL;
    }
    impossible("correct_branch_type: unknown branch type");
    return BR_STAIR;
}

/*
 * Add the given branch to the branch list.  The branch list is ordered
 * by end1 dungeon and level followed by end2 dungeon and level.  If
 * extract_first is true, then the branch is already part of the list
 * but needs to be repositioned.
 */
void
insert_branch(new_branch, extract_first)
   branch *new_branch;
   boolean extract_first;
{
    branch *curr, *prev;
    long new_val, curr_val, prev_val;

    if (extract_first) {
	for (prev = 0, curr = branches; curr; prev = curr, curr = curr->next)
	    if (curr == new_branch) break;

	if (!curr) panic("insert_branch: not found");
	if (prev)
	    prev->next = curr->next;
	else
	    branches = curr->next;
    }
    new_branch->next = (branch *) 0;

/* Convert the branch into a unique number so we can sort them. */
#define branch_val(bp) \
	((((long)(bp)->end1.dnum * (MAXLEVEL+1) + \
	  (long)(bp)->end1.dlevel) * (MAXDUNGEON+1) * (MAXLEVEL+1)) + \
	 ((long)(bp)->end2.dnum * (MAXLEVEL+1) + (long)(bp)->end2.dlevel))

    /*
     * Insert the new branch into the correct place in the branch list.
     */
    prev = (branch *) 0;
    prev_val = -1;
    new_val = branch_val(new_branch);
    for (curr = branches; curr;
		    prev_val = curr_val, prev = curr, curr = curr->next) {
	curr_val = branch_val(curr);
	if (prev_val < new_val && new_val <= curr_val) break;
    }
    if (prev) {
	new_branch->next = curr;
	prev->next = new_branch;
    } else {
	new_branch->next = branches;
	branches = new_branch;
    }
}

/* Add a dungeon branch to the branch list. */
STATIC_OVL branch *
add_branch(dgn, child_entry_level, pd)
    int dgn;
    int child_entry_level;
    struct proto_dungeon *pd;
{
    static int branch_id = 0;
    int branch_num;
    branch *new_branch;

    branch_num = find_branch(dungeons[dgn].dname,pd);
    new_branch = (branch *) alloc(sizeof(branch));
    new_branch->next = (branch *) 0;
    new_branch->id = branch_id++;
    new_branch->type = correct_branch_type(&pd->tmpbranch[branch_num]);
    new_branch->end1.dnum = parent_dnum(dungeons[dgn].dname, pd);
    new_branch->end1.dlevel = parent_dlevel(dungeons[dgn].dname, pd);
    new_branch->end2.dnum = dgn;
    new_branch->end2.dlevel = child_entry_level;
    new_branch->end1_up = pd->tmpbranch[branch_num].up ? TRUE : FALSE;

    insert_branch(new_branch, FALSE);
    return new_branch;
}

/*
 * Add new level to special level chain.  Insert it in level order with the
 * other levels in this dungeon.  This assumes that we are never given a
 * level that has a dungeon number less than the dungeon number of the
 * last entry.
 */
STATIC_OVL void
add_level(new_lev)
    s_level *new_lev;
{
	s_level *prev, *curr;

	prev = (s_level *) 0;
	for (curr = sp_levchn; curr; curr = curr->next) {
	    if (curr->dlevel.dnum == new_lev->dlevel.dnum &&
		    curr->dlevel.dlevel > new_lev->dlevel.dlevel)
		break;
	    prev = curr;
	}
	if (!prev) {
	    new_lev->next = sp_levchn;
	    sp_levchn = new_lev;
	} else {
	    new_lev->next = curr;
	    prev->next = new_lev;
	}
}

STATIC_OVL void
init_level(dgn, proto_index, pd)
	int dgn, proto_index;
	struct proto_dungeon *pd;
{
	s_level	*new_level;
	struct tmplevel *tlevel = &pd->tmplevel[proto_index];

	pd->final_lev[proto_index] = (s_level *) 0; /* no "real" level */
#ifdef WIZARD
	if (!wizard)
#endif
	    if (tlevel->chance <= rn2(100)) return;

	pd->final_lev[proto_index] = new_level =
					(s_level *) alloc(sizeof(s_level));
	/* load new level with data */
	Strcpy(new_level->proto, tlevel->name);
	new_level->boneid = tlevel->boneschar;
	new_level->dlevel.dnum = dgn;
	new_level->dlevel.dlevel = 0;	/* for now */

	new_level->flags.town = !!(tlevel->flags & TOWN);
	new_level->flags.hellish = !!(tlevel->flags & HELLISH);
	new_level->flags.maze_like = !!(tlevel->flags & MAZELIKE);
	new_level->flags.rogue_like = !!(tlevel->flags & ROGUELIKE);
	new_level->flags.align = ((tlevel->flags & D_ALIGN_MASK) >> 4);
	if (!new_level->flags.align) 
	    new_level->flags.align =
		((pd->tmpdungeon[dgn].flags & D_ALIGN_MASK) >> 4);

	new_level->rndlevs = tlevel->rndlevs;
	new_level->next    = (s_level *) 0;
}

STATIC_OVL int
possible_places(idx, map, pd)
    int idx;		/* prototype index */
    boolean *map;	/* array MAXLEVEL+1 in length */
    struct proto_dungeon *pd;
{
    int i, start, count;
    s_level *lev = pd->final_lev[idx];

    /* init level possibilities */
    for (i = 0; i <= MAXLEVEL; i++) map[i] = FALSE;

    /* get base and range and set those entried to true */
    count = level_range(lev->dlevel.dnum, pd->tmplevel[idx].lev.base,
					pd->tmplevel[idx].lev.rand,
					pd->tmplevel[idx].chain,
					pd, &start);
    for (i = start; i < start+count; i++)
	map[i] = TRUE;

    /* mark off already placed levels */
    for (i = pd->start; i < idx; i++) {
	if (pd->final_lev[i] && map[pd->final_lev[i]->dlevel.dlevel]) {
	    map[pd->final_lev[i]->dlevel.dlevel] = FALSE;
	    --count;
	}
    }

    return count;
}

/* Pick the nth TRUE entry in the given boolean array. */
STATIC_OVL int
pick_level(map, nth)
    boolean *map;	/* an array MAXLEVEL+1 in size */
    int nth;
{
    int i;
    for (i = 1; i <= MAXLEVEL; i++)
	if (map[i] && !nth--) return (int) i;
    panic("pick_level:  ran out of valid levels");
    return 0;
}

#ifdef DDEBUG
static void FDECL(indent,(int));

static void
indent(d)
int d;
{
    while (d-- > 0) fputs("    ", stderr);
}
#endif

/*
 * Place a level.  First, find the possible places on a dungeon map
 * template.  Next pick one.  Then try to place the next level.  If
 * sucessful, we're done.  Otherwise, try another (and another) until
 * all possible places have been tried.  If all possible places have
 * been exausted, return false.
 */
STATIC_OVL boolean
place_level(proto_index, pd)
    int proto_index;
    struct proto_dungeon *pd;
{
    boolean map[MAXLEVEL+1];	/* valid levels are 1..MAXLEVEL inclusive */
    s_level *lev;
    int npossible;
#ifdef DDEBUG
    int i;
#endif

    if (proto_index == pd->n_levs) return TRUE;	/* at end of proto levels */

    lev = pd->final_lev[proto_index];

    /* No level created for this prototype, goto next. */
    if (!lev) return place_level(proto_index+1, pd);

    npossible = possible_places(proto_index, map, pd);

    for (; npossible; --npossible) {
	lev->dlevel.dlevel = pick_level(map, rn2(npossible));
#ifdef DDEBUG
	indent(proto_index-pd->start);
	fprintf(stderr,"%s: trying %d [ ", lev->proto, lev->dlevel.dlevel);
	for (i = 1; i <= MAXLEVEL; i++)
	    if (map[i]) fprintf(stderr,"%d ", i);
	fprintf(stderr,"]\n");
#endif
	if (place_level(proto_index+1, pd)) return TRUE;
	map[lev->dlevel.dlevel] = FALSE;	/* this choice didn't work */
    }
#ifdef DDEBUG
    indent(proto_index-pd->start);
    fprintf(stderr,"%s: failed\n", lev->proto);
#endif
    return FALSE;
}

/* */
struct {
	char name[20];
	int prev;
	boolean selected;
	boolean set;
} alternate_history[MAXDUNGEON];

/* return TRUE if this dungeon, which possesses alternates, should be chosen */
boolean
select_alternate(i, pd)
int i;
struct proto_dungeon * pd;
{
	int j;
	int altnum = 0;
	boolean select;
	char * dgn_name = pd->tmpdungeon[i].name;

	/* check list of previous alternates checked */
	for (j=0; j<i && alternate_history[j].set; j++) {
		if (!strncmp(dgn_name, alternate_history[j].name, 20)) {
			if (alternate_history[j].selected)
				return FALSE;	/* we've already selected a dungeon with this name */
			else {
				altnum = alternate_history[j].prev;	/* how many alternates have we passed by so far? */
				break;
			}
		}
	}

	/* general case, may be overridden by special cases */
	/* 1/X chance of being selected, where X is the number of versions remaining to choose from */
	select = !rn2(pd->tmpdungeon[i].alternates - altnum + 1);

	/* are we a special case? */
	if (!strcmp(dgn_name, "Chaos Quest") && flags.chaosvar && (wizard || is_june())) {
		// chaosvar is incremented by 1 so that all variants are nonzero in the option
		// select if alt_i == our chosen version
		select = (altnum == (flags.chaosvar-1));
	}
	/* record Chaos Quest version, if selected */
	if (select && !strcmp(dgn_name, "Chaos Quest"))
		chaos_dvariant = altnum;

	/* update alternate_history */
	strncpy(alternate_history[j].name, dgn_name, 20);
	alternate_history[j].set = TRUE;
	alternate_history[j].prev++;
	alternate_history[j].selected = select;

	return select;
}

struct level_map {
	const char *lev_name;
	d_level *lev_spec;
} level_map[] = {
	/*Dungeons of Doom*/
	{ "castle",	&stronghold_level },
	{ "oracle",	&oracle_level },
	{ "villag",	&village_level },
	{ "bigroom",	&bigroom_level },
#ifdef REINCARNATION
	{ "rogue",	&rogue_level },
#endif
	{ "chall",	&challenge_level },
	/*Gehennom*/
	{ "valley",	&valley_level },
	
	{ "wizard1",	&wiz1_level },
	{ "wizard2",	&wiz2_level },
	{ "wizard3",	&wiz3_level },
	
//	{ "asmodeus",	&asmodeus_level },
//	{ "baalz",	&baalzebub_level },
	{ "hell",	&hell1_level },
	{ "hell2",	&hell2_level },
	{ "hell3",	&hell3_level },
	{ "fakewiz1",	&portal_level },
//	{ "juiblex",	&juiblex_level },
//	{ "orcus",	&orcus_level },
	{ "abyss",	&abyss1_level },
	{ "abys2",	&abyss2_level },
	{ "brine",	&abyss3_level },
	
	{ "sanctum",	&sanctum_level },
	{ "sigil",	&sigil_level },
	{ "nowhere",	&nowhere_level },

	{ "icetwn",	&icetown_level },
	{ "iceboss",	&iceboss_level },
	
	{ "bftemple",	&bftemple_level },
	{ "bfboss",	&bfboss_level },
	
	{ "dsbog",	&dsbog_level },
	{ "dsboss",	&dsboss_level },

	{ "leveetwn",	&leveetwn_level },
	{ "arcboss",	&arcboss_level },


	{ "minetn",     &minetown_level },
#ifdef RECORD_ACHIEVE
	{ "minend",     &mineend_level },
	{ "soko1",      &sokoend_level },
#endif
	/*Void*/
	{ "ilsensin",	&ilsensin_level },
	{ "farvoid",	&farvoid_level },
	{ "aligvoid",	&aligvoid_level },
	{ "nrvoid2",	&nrvoid2_level },
	{ "nearvoid",	&nearvoid_level },
	
	/*Sacristy*/
	{ "sacris",	&sacris_level },

	/*Planes*/
	{ "air",	&air_level },
	{ "astral",	&astral_level },
	{ "earth",	&earth_level },
	{ "fire",	&fire_level },
	{ "water",	&water_level },
	/*Quest Levels*/
	{ X_START,	&qstart_level },
	{ X_LOCATE,	&qlocate_level },
	{ X_GOAL,	&nemesis_level },
	/*Law Levels*/
	{ "path1",	&path1_level },
	{ "path2",	&path2_level },
	{ "path3",	&path3_level },
	{ "illregrd",	&illregrd_level },
	{ "arcadia1",	&arcadia1_level },
	{ "arcadia2",	&arcadia2_level },
	{ "arcadia3",	&arcadia3_level },
	{ "arcward",	&arcward_level },
	{ "arcfort",	&arcfort_level },
	{ "atower1",	&tower1_level },
	{ "atower2",	&tower2_level },
	{ "atower3",	&tower3_level },
	{ "towrtop",	&towertop_level },
	/*Neutral Levels*/
	{ "gatetwn",&gatetown_level },
	{ "spire",	&spire_level },
	{ "sumall",	&sum_of_all_level },
	{ "leth-a",	&lethe_headwaters },
	{ "lethe-e",	&bridge_temple },
	{ "lethe-f",	&lethe_temples },
	{ "nkai-a",	&nkai_a_level },
	{ "nkai-b",	&nkai_b_level },
	{ "nkai-c",	&nkai_c_level },
	{ "nkai-z",	&nkai_z_level },
	{ "rlyeh",	&rlyeh_level },
	/*Chaos levels*/
	{ "chaosf",	&chaosf_level },
	{ "chaoss",	&chaoss_level },
	{ "chaost",	&chaost_level },
	{ "chaosm",	&chaosm_level },
	{ "chaosfrh",	&chaosfrh_level },
	{ "chaosffh",	&chaosffh_level },
	{ "chaossth",	&chaossth_level },
	{ "chaosvth",	&chaosvth_level },
	{ "chaose",		&chaose_level },
	/*Chaos 2 levels*/
	{ "ossa1",&elshava_level },
	{ "mith3",&lastspire_level },
	/*Chaos 3 levels*/
	{ "frst1",&forest_1_level },
	{ "frst2",&forest_2_level },
	{ "frst3",&forest_3_level },
	{ "ford",&ford_level },
	{ "frst4",&forest_4_level },
	{ "mord1",&mordor_1_level },
	{ "mord2",&mordor_2_level },
	{ "spi1",&spider_level },
	{ "mdpth1",&mordor_depths_1_level },
	{ "mdpth2",&mordor_depths_2_level },
	{ "mdpth3",&mordor_depths_3_level },
	{ "bore1",&borehole_1_level },
	{ "bore2",&borehole_2_level },
	{ "bore3",&borehole_3_level },
	{ "bore4",&borehole_4_level },
	/*Fort Knox*/
	{ "knox",	&knox_level },
	{ "",		(d_level *)0 }
};

void
init_dungeons()		/* initialize the "dungeon" structs */
{
	dlb	*dgn_file;
	register int i, cl = 0, cb = 0;
	register s_level *x;
	struct proto_dungeon pd;
	struct level_map *lev_map;
	struct version_info vers_info;
	boolean quest_i;
	
	pd.n_levs = pd.n_brs = 0;

	dgn_file = dlb_fopen(DUNGEON_FILE, RDBMODE);
	if (!dgn_file) {
	    char tbuf[BUFSZ];
	    Sprintf(tbuf, "Cannot open dungeon description - \"%s",
		DUNGEON_FILE);
#ifdef DLBRSRC /* using a resource from the executable */
	    Strcat(tbuf, "\" resource!");
#else /* using a file or DLB file */
# if defined(DLB)
	    Strcat(tbuf, "\" from ");
#  ifdef PREFIXES_IN_USE
	    Strcat(tbuf, "\n\"");
	    if (fqn_prefix[DATAPREFIX]) Strcat(tbuf, fqn_prefix[DATAPREFIX]);
#  else
	    Strcat(tbuf, "\"");
#  endif
	    Strcat(tbuf, DLBFILE);
# endif
	    Strcat(tbuf, "\" file!");
#endif
#ifdef WIN32
	    interject_assistance(1, INTERJECT_PANIC, (genericptr_t)tbuf,
				 (genericptr_t)fqn_prefix[DATAPREFIX]);
#endif
	    panic1(tbuf);
	}

	/* validate the data's version against the program's version */
	Fread((genericptr_t) &vers_info, sizeof vers_info, 1, dgn_file);
	/* we'd better clear the screen now, since when error messages come from
	 * check_version() they will be printed using pline(), which doesn't
	 * mix with the raw messages that might be already on the screen
	 */
	if (iflags.window_inited) clear_nhwindow(WIN_MAP);
		if (!check_version(&vers_info, DUNGEON_FILE, TRUE))
	    panic("Dungeon description not valid.");


	/*
	 * Read in each dungeon and transfer the results to the internal
	 * dungeon arrays.
	 */
	sp_levchn = (s_level *) 0;
	Fread((genericptr_t)&n_dgns, sizeof(int), 1, dgn_file);
	if (n_dgns >= MAXDUNGEON)
	    panic("init_dungeons: too many dungeons");

	for (i = 0; i < n_dgns; i++) {
	    Fread((genericptr_t)&pd.tmpdungeon[i],
				    sizeof(struct tmpdungeon), 1, dgn_file);

	    if ((pd.tmpdungeon[i].alternates > 0 && !select_alternate(i, &pd))
			|| pd.tmpdungeon[i].chance <= rn2(100)) {
			int j;

			/* skip over any levels or branches */
			for(j = 0; j < pd.tmpdungeon[i].levels; j++)
				Fread((genericptr_t)&pd.tmplevel[cl], sizeof(struct tmplevel),
								1, dgn_file);

			for(j = 0; j < pd.tmpdungeon[i].branches; j++)
				Fread((genericptr_t)&pd.tmpbranch[cb],
						sizeof(struct tmpbranch), 1, dgn_file);
			n_dgns--; i--;
			continue;
	    }

	    Strcpy(dungeons[i].dname, pd.tmpdungeon[i].name);
	    Strcpy(dungeons[i].proto, pd.tmpdungeon[i].protoname);
	    dungeons[i].boneid = pd.tmpdungeon[i].boneschar;

	    if(pd.tmpdungeon[i].lev.rand)
			dungeons[i].num_dunlevs = (int)rn1(pd.tmpdungeon[i].lev.rand,
								 pd.tmpdungeon[i].lev.base);
	    else dungeons[i].num_dunlevs = (int)pd.tmpdungeon[i].lev.base;
		
		if(!strcmp(dungeons[i].dname, "The Quest")){
			quest_i = TRUE;
			if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
				if(dungeons[i].num_dunlevs < 6){
					dungeons[i].num_dunlevs = 6;
				}
			}
		}
		else quest_i = FALSE;
		
	    if(!i) {
		dungeons[i].ledger_start = 0;
		dungeons[i].depth_start = 1;
		dungeons[i].dunlev_ureached = 1;
	    } else {
		dungeons[i].ledger_start = dungeons[i-1].ledger_start +
					      dungeons[i-1].num_dunlevs;
		dungeons[i].dunlev_ureached = 0;
	    }

	    dungeons[i].flags.hellish = !!(pd.tmpdungeon[i].flags & HELLISH);
	    dungeons[i].flags.maze_like = !!(pd.tmpdungeon[i].flags & MAZELIKE);
	    dungeons[i].flags.rogue_like = !!(pd.tmpdungeon[i].flags & ROGUELIKE);
	    dungeons[i].flags.align = ((pd.tmpdungeon[i].flags & D_ALIGN_MASK) >> 4);
	    /*
	     * Set the entry level for this dungeon.  The pd.tmpdungeon entry
	     * value means:
	     *		< 0	from bottom (-1 == bottom level)
	     *		  0	default (top)
	     *		> 0	actual level (1 = top)
	     *
	     * Note that the entry_lev field in the dungeon structure is
	     * redundant.  It is used only here and in print_dungeon().
	     */
	    if (pd.tmpdungeon[i].entry_lev < 0) {
		dungeons[i].entry_lev = dungeons[i].num_dunlevs +
						pd.tmpdungeon[i].entry_lev + 1;
		if (dungeons[i].entry_lev <= 0) dungeons[i].entry_lev = 1;
	    } else if (pd.tmpdungeon[i].entry_lev > 0) {
		dungeons[i].entry_lev = pd.tmpdungeon[i].entry_lev;
		if (dungeons[i].entry_lev > dungeons[i].num_dunlevs)
		    dungeons[i].entry_lev = dungeons[i].num_dunlevs;
	    } else { /* default */
		dungeons[i].entry_lev = 1;	/* defaults to top level */
	    }

	    if (i) {	/* set depth */
		branch *br;
		schar from_depth;
		boolean from_up;

		br = add_branch(i, dungeons[i].entry_lev, &pd);

		/* Get the depth of the connecting end. */
		if (br->end1.dnum == i) {
		    from_depth = depth(&br->end2);
		    from_up = !br->end1_up;
		} else {
		    from_depth = depth(&br->end1);
		    from_up = br->end1_up;
		}

		/*
		 * Calculate the depth of the top of the dungeon via
		 * its branch.  First, the depth of the entry point:
		 *
		 *	depth of branch from "parent" dungeon
		 *	+ -1 or 1 depending on a up or down stair or
		 *	  0 if portal
		 *
		 * Followed by the depth of the top of the dungeon:
		 *
		 *	- (entry depth - 1)
		 *
		 * We'll say that portals stay on the same depth.
		 */
		dungeons[i].depth_start = from_depth
					+ (br->type == BR_PORTAL ? 0 :
							(from_up ? -1 : 1))
					- (dungeons[i].entry_lev - 1);
	    }

	    /* this is redundant - it should have been flagged by dgn_comp */
	    if(dungeons[i].num_dunlevs > MAXLEVEL)
		dungeons[i].num_dunlevs = MAXLEVEL;

	    pd.start = pd.n_levs;	/* save starting point */
	    pd.n_levs += pd.tmpdungeon[i].levels;
	    if (pd.n_levs > LEV_LIMIT)
		panic("init_dungeon: too many special levels");
	    /*
	     * Read in the prototype special levels.  Don't add generated
	     * special levels until they are all placed.
	     */
	    for(; cl < pd.n_levs; cl++) {
			Fread((genericptr_t)&pd.tmplevel[cl],
						sizeof(struct tmplevel), 1, dgn_file);
			init_level(i, cl, &pd);
	    }
		//If the quest is long enough, move the locate level to create one to two of each filler level
		//	New levels are added to the end of tmplevel, this asumes that the locate level is 2nd to last :(
		if(quest_i){
			if(urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH){
				pd.tmplevel[pd.n_levs-2].lev.base++;
			}
			else if(dungeons[i].num_dunlevs == 7){
				pd.tmplevel[pd.n_levs-2].lev.base++;
			}
			else if(dungeons[i].num_dunlevs == 6){
				pd.tmplevel[pd.n_levs-2].lev.base += rn2(2);
			}
		}
	    /*
	     * Recursively place the generated levels for this dungeon.  This
	     * routine will attempt all possible combinations before giving
	     * up.
	     */
	    if (!place_level(pd.start, &pd))
		panic("init_dungeon:  couldn't place levels");
#ifdef DDEBUG
	    fprintf(stderr, "--- end of dungeon %d ---\n", i);
	    fflush(stderr);
	    getchar();
#endif
	    for (; pd.start < pd.n_levs; pd.start++)
		if (pd.final_lev[pd.start]) add_level(pd.final_lev[pd.start]);


	    pd.n_brs += pd.tmpdungeon[i].branches;
	    if (pd.n_brs > BRANCH_LIMIT)
		panic("init_dungeon: too many branches");
	    for(; cb < pd.n_brs; cb++)
		Fread((genericptr_t)&pd.tmpbranch[cb],
					sizeof(struct tmpbranch), 1, dgn_file);
	}
	(void) dlb_fclose(dgn_file);

	for (i = 0; i < 5; i++) tune[i] = 'A' + rn2(7);
	tune[5] = 0;

	/*
	 * Find most of the special levels and dungeons so we can access their
	 * locations quickly.
	 */
	for (lev_map = level_map; lev_map->lev_name[0]; lev_map++) {
		x = find_level(lev_map->lev_name);
		if (x) {
			assign_level(lev_map->lev_spec, &x->dlevel);
			if (!strncmp(lev_map->lev_name, "x-", 2)) {
				/* This is where the name substitution on the
				 * levels of the quest dungeon occur.
				 */
				Sprintf(x->proto, "%s%s", urole.filecode, &lev_map->lev_name[1]);
			} else if (lev_map->lev_spec == &knox_level) {
				branch *br;
				/*
				 * Kludge to allow floating Knox entrance.  We
				 * specify a floating entrance by the fact that
				 * its entrance (end1) has a bogus dnum, namely
				 * n_dgns.
				 */
				for (br = branches; br; br = br->next)
				    if (on_level(&br->end2, &knox_level)) break;

				if (br) br->end1.dnum = n_dgns;
				/* adjust the branch's position on the list */
				insert_branch(br, TRUE);
			}
		}
	}
/*
 *	I hate hardwiring these names. :-(
 */
	quest_dnum = dname_to_dnum("The Quest");
	neutral_dnum = dname_to_dnum("Neutral Quest");
	rlyeh_dnum = dname_to_dnum("The Lost Cities");
	spire_dnum = dname_to_dnum("The Spire");
	chaos_dnum = dname_to_dnum("Chaos Quest");
	law_dnum = dname_to_dnum("Law Quest");
	sokoban_dnum = dname_to_dnum("Lokoban");
	mines_dnum = dname_to_dnum("The Gnomish Mines");
	ice_dnum = dname_to_dnum("The Ice Caves");
	blackforest_dnum = dname_to_dnum("The Black Forest");
	dismalswamp_dnum = dname_to_dnum("The Dismal Swamp");
	archipelago_dnum = dname_to_dnum("The Archipelago");
	tower_dnum = dname_to_dnum("Vlad's Tower");
	tomb_dnum = dname_to_dnum("The Lost Tomb");
	sea_dnum = dname_to_dnum("The Sunless Sea");
	temple_dnum = dname_to_dnum("The Temple of Moloch");

	/* one special fixup for dummy surface level */
	if ((x = find_level("dummy")) != 0) {
	    i = x->dlevel.dnum;
	    /* the code above puts earth one level above dungeon level #1,
	       making the dummy level overlay level 1; but the whole reason
	       for having the dummy level is to make earth have depth -1
	       instead of 0, so adjust the start point to shift endgame up */
	    if (dunlevs_in_dungeon(&x->dlevel) > 1 - dungeons[i].depth_start)
		dungeons[i].depth_start -= 1;
	    /* TO DO: strip "dummy" out all the way here,
	       so that it's hidden from <ctrl/O> feedback. */
	}

#ifdef DEBUG
	dumpit();
#endif
}

int
dunlev(lev)	/* return the level number for lev in *this* dungeon */
d_level	*lev;
{
	return(lev->dlevel);
}

int
dunlevs_in_dungeon(lev)	/* return the lowest level number for *this* dungeon*/
d_level	*lev;
{
	return(dungeons[lev->dnum].num_dunlevs);
}

int
deepest_lev_reached(noquest) /* return the lowest level explored in the game*/
boolean noquest;
{
	/* this function is used for three purposes: to provide a factor
	 * of difficulty in monster generation; to provide a factor of
	 * difficulty in experience calculations (botl.c and end.c); and
	 * to insert the deepest level reached in the game in the topten
	 * display.  the 'noquest' arg switch is required for the latter.
	 *
	 * from the player's point of view, going into the Quest is _not_
	 * going deeper into the dungeon -- it is going back "home", where
	 * the dungeon starts at level 1.  given the setup in dungeon.def,
	 * the depth of the Quest (thought of as starting at level 1) is
	 * never lower than the level of entry into the Quest, so we exclude
	 * the Quest from the topten "deepest level reached" display
	 * calculation.  _However_ the Quest is a difficult dungeon, so we
	 * include it in the factor of difficulty calculations.
	 */
	register int i;
	d_level tmp;
	register schar ret = 0;

	for(i = 0; i < n_dgns; i++) {
	    if((tmp.dlevel = dungeons[i].dunlev_ureached) == 0) continue;
	    if(!strcmp(dungeons[i].dname, "The Quest") && noquest) continue;

	    tmp.dnum = i;
	    if(depth(&tmp) > ret) ret = depth(&tmp);
	}
	return((int) ret);
}

/* return a bookkeeping level number for purpose of comparisons and
 * save/restore */
int
ledger_no(lev)
d_level	*lev;
{
	return((int)(lev->dlevel + dungeons[lev->dnum].ledger_start));
}

/*
 * The last level in the bookkeeping list of level is the bottom of the last
 * dungeon in the dungeons[] array.
 *
 * Maxledgerno() -- which is the max number of levels in the bookkeeping
 * list, should not be confused with dunlevs_in_dungeon(lev) -- which
 * returns the max number of levels in lev's dungeon, and both should
 * not be confused with deepest_lev_reached() -- which returns the lowest
 * depth visited by the player.
 */
int
maxledgerno()
{
    return (int) (dungeons[n_dgns-1].ledger_start +
				dungeons[n_dgns-1].num_dunlevs);
}

/* return the dungeon that this ledgerno exists in */
int
ledger_to_dnum(ledgerno)
int	ledgerno;
{
	register int i;

	/* find i such that (i->base + 1) <= ledgerno <= (i->base + i->count) */
	for (i = 0; i < n_dgns; i++)
	    if (dungeons[i].ledger_start < ledgerno &&
		ledgerno <= dungeons[i].ledger_start + dungeons[i].num_dunlevs)
		return (int)i;

	panic("level number out of range [ledger_to_dnum(%d)]", (int)ledgerno);
	/*NOT REACHED*/
	return (int)0;
}

/* return the level of the dungeon this ledgerno exists in */
int
ledger_to_dlev(ledgerno)
int	ledgerno;
{
	return((int)(ledgerno - dungeons[ledger_to_dnum(ledgerno)].ledger_start));
}

#endif /* OVL1 */
#ifdef OVL0

/* returns the depth of a level, in floors below the surface	*/
/* (note levels in different dungeons can have the same depth).	*/
schar
depth(lev)
d_level	*lev;
{
	return((schar)( dungeons[lev->dnum].depth_start + lev->dlevel - 1));
}

boolean
on_level(lev1, lev2)	/* are "lev1" and "lev2" actually the same? */
d_level	*lev1, *lev2;
{
	return((boolean)((lev1->dnum == lev2->dnum) && (lev1->dlevel == lev2->dlevel)));
}

#endif /* OVL0 */
#ifdef OVL1

/* is this level referenced in the special level chain? */
s_level *
Is_special(lev)
d_level	*lev;
{
	s_level *levtmp;

	for (levtmp = sp_levchn; levtmp; levtmp = levtmp->next)
	    if (on_level(lev, &levtmp->dlevel)) return(levtmp);

	return((s_level *)0);
}

/*
 * Is this a multi-dungeon branch level?  If so, return a pointer to the
 * branch.  Otherwise, return null.
 */
branch *
Is_branchlev(lev)
	d_level	*lev;
{
	branch *curr;

	for (curr = branches; curr; curr = curr->next) {
	    if (on_level(lev, &curr->end1) || on_level(lev, &curr->end2))
		return curr;
	}
	return (branch *) 0;
}

d_level *
branchlev_other_end(bptr, lev)
branch * bptr;
d_level * lev;
{
	d_level * other_end;
	if (bptr->end1.dnum == lev->dnum && bptr->end1.dlevel == lev->dlevel)
		other_end = &(bptr->end2);
	else if (bptr->end2.dnum == lev->dnum && bptr->end2.dlevel == lev->dlevel)
		other_end = &(bptr->end1);
	else
		other_end = (d_level *)0;
	return other_end;
}

/* goto the next level (or appropriate dungeon) */
void
next_level(at_stairs)
boolean	at_stairs;
{
	if (at_stairs && u.ux == sstairs.sx && u.uy == sstairs.sy) {
		/* Taking a down dungeon branch. */
		goto_level(&sstairs.tolev, at_stairs, FALSE, FALSE);
	} else {
		/* Going down a stairs or jump in a trap door. */
		d_level	newlevel;

		newlevel.dnum = u.uz.dnum;
		newlevel.dlevel = u.uz.dlevel + 1;
		goto_level(&newlevel, at_stairs, !at_stairs, FALSE);
	}
}

/* goto the previous level (or appropriate dungeon) */
void
prev_level(at_stairs)
boolean	at_stairs;
{
	if (at_stairs && u.ux == sstairs.sx && u.uy == sstairs.sy) {
		/* Taking an up dungeon branch. */
		/* KMH -- Upwards branches are okay if not level 1 */
		/* (Just make sure it doesn't go above depth 1) */
		if(!u.uz.dnum && u.uz.dlevel == 1 && !u.uhave.amulet) done(ESCAPED);
		else goto_level(&sstairs.tolev, at_stairs, FALSE, FALSE);
	} else {
		/* Going up a stairs or rising through the ceiling. */
		d_level	newlevel;
		newlevel.dnum = u.uz.dnum;
		newlevel.dlevel = u.uz.dlevel - 1;
		goto_level(&newlevel, at_stairs, FALSE, FALSE);
	}
}

void
u_on_newpos(x, y)
int x, y;
{
	u.ux = x;
	u.uy = y;
#ifdef CLIPPING
	cliparound(u.ux, u.uy);
#endif
#ifdef STEED
	/* ridden steed always shares hero's location */
	if (u.usteed) u.usteed->mx = u.ux, u.usteed->my = u.uy;
#endif
}

void
u_on_sstairs() {	/* place you on the special staircase */

	if (sstairs.sx) {
	    u_on_newpos(sstairs.sx, sstairs.sy);
	} else {
	    /* code stolen from goto_level */
	    int trycnt = 0;
	    xchar x, y;
#ifdef DEBUG
	    pline("u_on_sstairs: picking random spot");
#endif
#define badspot(x,y) ((levl[x][y].typ != ROOM && levl[x][y].typ != CORR && levl[x][y].typ != GRASS && levl[x][y].typ != SAND && levl[x][y].typ != SOIL) || MON_AT(x, y))
	    do {
		x = rnd(COLNO-1);
		y = rn2(ROWNO);
		if (!badspot(x, y)) {
		    u_on_newpos(x, y);
		    return;
		}
	    } while (++trycnt <= 500);

		/* try extra hard to place player */
		for (x=1; x <= COLNO-1; x++)
		for (y=0; y <= ROWNO-1; y++)
		if (!badspot(x, y)) {
			u_on_newpos(x, y);
			return;
		}
	    panic("u_on_sstairs: could not relocate player!");
#undef badspot
	}
}

void
u_on_upstairs()	/* place you on upstairs (or special equivalent) */
{
	if (xupstair) {
		u_on_newpos(xupstair, yupstair);
	} else
		u_on_sstairs();
}

void
u_on_dnstairs()	/* place you on dnstairs (or special equivalent) */
{
	if (xdnstair) {
		u_on_newpos(xdnstair, ydnstair);
	} else
		u_on_sstairs();
}

boolean
On_stairs(x, y)
xchar x, y;
{
	return((boolean)((x == xupstair && y == yupstair) ||
	       (x == xdnstair && y == ydnstair) ||
	       (x == xdnladder && y == ydnladder) ||
	       (x == xupladder && y == yupladder) ||
	       (x == sstairs.sx && y == sstairs.sy)));
}

boolean
Is_botlevel(lev)
d_level *lev;
{
	return((boolean)(lev->dlevel == dungeons[lev->dnum].num_dunlevs));
}

boolean
Can_dig_down(lev)
d_level *lev;
{
	return((boolean)(!level.flags.hardfloor
	    && !Is_botlevel(lev) && !Invocation_lev(lev)));
}

/*
 * Like Can_dig_down (above), but also allows falling through on the
 * stronghold level.  Normally, the bottom level of a dungeon resists
 * both digging and falling.
 */
boolean
Can_fall_thru(lev)
d_level *lev;
{
	return((boolean)(Can_dig_down(lev) || Is_stronghold(lev)));
}

/*
 * True if one can rise up a level (e.g. cursed gain level).
 * This happens on intermediate dungeon levels or on any top dungeon
 * level that has a stairwell style branch to the next higher dungeon.
 * Checks for amulets and such must be done elsewhere.
 */
boolean
Can_rise_up(x, y, lev)
int	x, y;
d_level *lev;
{
    /* can't rise up from inside the top of the Wizard's tower */
    /* KMH -- or in sokoban */
    if (In_endgame(lev) || In_sokoban(lev) || In_void(lev) ||
			(Is_wiz1_level(lev) && In_W_tower(x, y, lev)))
	return FALSE;
    return (boolean)(lev->dlevel > 1 ||
		(dungeons[lev->dnum].entry_lev == 1 && ledger_no(lev) != 1 &&
		 sstairs.sx && sstairs.up));
}

/*
 * It is expected that the second argument of get_level is a depth value,
 * either supplied by the user (teleport control) or randomly generated.
 * But more than one level can be at the same depth.  If the target level
 * is "above" the present depth location, get_level must trace "up" from
 * the player's location (through the ancestors dungeons) the dungeon
 * within which the target level is located.  With only one exception
 * which does not pass through this routine (see level_tele), teleporting
 * "down" is confined to the current dungeon.  At present, level teleport
 * in dungeons that build up is confined within them.
 */
void
get_level(newlevel, levnum)
d_level *newlevel;
int levnum;
{
	branch *br;
	int dgn = u.uz.dnum;

	if (levnum <= 0 && In_endgame(&u.uz)) {
	    levnum = u.uz.dlevel;
	} else if (levnum > dungeons[dgn].depth_start
			    + dungeons[dgn].num_dunlevs - 1) {
	    /* beyond end of dungeon, jump to last level */
	    levnum = dungeons[dgn].num_dunlevs;
	} else {
	    /* The desired level is in this dungeon or a "higher" one. */

	    /*
	     * Branch up the tree until we reach a dungeon that contains the
	     * levnum.
	     */
	    if (levnum < dungeons[dgn].depth_start) {
			if(In_mines(&u.uz) && u.uz.dlevel < dungeons[u.uz.dnum].entry_lev){
				dgn = tower_dnum;
			}
			else{
				do {
					/*
					 * Find the parent dungeon of this dungeon.
					 *
					 * This assumes that end2 is always the "child" and it is
					 * unique.
					 */
					for (br = branches; br; br = br->next)
						if (br->end2.dnum == dgn) break;
					if (!br)
						panic("get_level: can't find parent dungeon");

					dgn = br->end1.dnum;
				} while (levnum < dungeons[dgn].depth_start);
			}
	    }
	    /* We're within the same dungeon; calculate the level. */
	    levnum = levnum - dungeons[dgn].depth_start + 1;
	}

	newlevel->dnum = dgn;
	newlevel->dlevel = levnum;
}

#endif /* OVL1 */
#ifdef OVL0

boolean
In_quest(lev)	/* are you in the quest dungeon? */
d_level *lev;
{
	return((boolean)(lev->dnum == quest_dnum));
}

boolean
In_mines_quest(lev)	/* are you in the mines, or a quest dungeon 'located' in the mines? */
d_level *lev;
{
	return((boolean)(lev->dnum == mines_dnum || (Role_if(PM_RANGER) && Race_if(PM_GNOME) && lev->dnum == quest_dnum) || Is_mineend_level(&u.uz)));
}

#endif /* OVL0 */
#ifdef OVL1

boolean
In_outdoors(lev)
d_level *lev;
{
	if(In_quest(lev)){
		if(Role_if(PM_ARCHEOLOGIST) || Role_if(PM_BARBARIAN) || Role_if(PM_EXILE)
		|| Role_if(PM_PIRATE) || Role_if(PM_PRIEST) || Role_if(PM_SAMURAI)
		|| Role_if(PM_VALKYRIE) || Role_if(PM_WIZARD)){
			return lev->dlevel <= qlocate_level.dlevel;
		} else if(Role_if(PM_HEALER) || Role_if(PM_NOBLEMAN) || Role_if(PM_UNDEAD_HUNTER)){
			return TRUE;
		} else if(Role_if(PM_RANGER)){
			return lev->dlevel < qlocate_level.dlevel;
		} else if(Role_if(PM_KNIGHT)){
			return lev->dlevel < nemesis_level.dlevel;
		} else if(Role_if(PM_TOURIST)) {
			return on_level(lev, &qlocate_level) || on_level(lev, &qstart_level);
		}
	} else if(In_tower(lev)){
		return lev->dlevel==4;
	} 
	else if(Is_paradise(lev) || Is_sunkcity(lev)) return TRUE;
	else if(In_outlands(lev)){
		return lev->dlevel < sum_of_all_level.dlevel;
	} else if(Is_arcadia_woods(lev)) return TRUE;
	else if(In_cha(lev)) {
		if (In_FF_quest(lev)) {
			return on_level(lev, &chaosf_level) || on_level(lev, &chaoss_level);
		} else if (In_mithardir_quest(lev)) {
			return (In_mithardir_desert(lev) || on_level(lev, &elshava_level));
		} else if (In_mordor_quest(lev)) {
			return (In_mordor_forest(lev) || In_mordor_fields(lev));
		}
	} else if(In_blackforest(lev)){
		return TRUE;
	} else if(In_dismalswamp(lev)){
		return TRUE;
	}
	return FALSE;
}

boolean
In_cave(lev)
d_level *lev;
{
	if(In_mines_quest(&u.uz) || (Inhell && !on_level(&valley_level, &u.uz))) return TRUE;
	if(In_quest(lev)){
		if(Role_if(PM_BARBARIAN)
		|| Role_if(PM_PIRATE) || Role_if(PM_PRIEST) || Role_if(PM_SAMURAI)
		|| Role_if(PM_VALKYRIE)){
			return lev->dlevel > qlocate_level.dlevel;
		} else if(Role_if(PM_RANGER)){
			return lev->dlevel >= qlocate_level.dlevel;
		} else if(Role_if(PM_CAVEMAN)){
			return TRUE;
		} else if(Role_if(PM_KNIGHT) || Role_if(PM_PRIEST)){
			return lev->dlevel == nemesis_level.dlevel;
		} else if(Role_if(PM_MONK)) {
			return on_level(lev, &qlocate_level);
		}
	}
	else if(Is_sunsea(lev)) return TRUE;
	else if(In_outlands(lev)){
		return lev->dlevel >= sum_of_all_level.dlevel;
	}
	else if(Is_village_level(lev) && dungeon_topology.village_variant == CAVE_VILLAGE) return TRUE;
	else if(In_icecaves(lev)) return TRUE;
	else if(In_lost_cities(lev)) return TRUE;
	return FALSE;
}

boolean
In_mines(lev)	/* are you in the mines dungeon? */
d_level	*lev;
{
	return((boolean)(lev->dnum == mines_dnum || Is_mineend_level(&u.uz)));
}

boolean
In_neu(lev)	/* are you on the neutral quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == neutral_dnum || lev->dnum == rlyeh_dnum));
}

boolean
In_outlands(lev)	/* are you on the neutral quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == neutral_dnum));
}

boolean
In_lost_cities(lev)	/* are you on the neutral quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == rlyeh_dnum));
}

boolean
In_cha(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum));
}

boolean
In_mithardir_desert(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MITHARDIR && lev->dlevel <= lastspire_level.dlevel && !on_level(lev, &elshava_level)));
}

boolean
In_mithardir_catacombs(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MITHARDIR && lev->dlevel > lastspire_level.dlevel));
}

boolean
In_mithardir_terminus(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MITHARDIR && lev->dlevel == dungeons[lev->dnum].num_dunlevs));
}

boolean
In_mithardir_quest(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MITHARDIR));
}

boolean
In_FF_quest(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == TEMPLE_OF_CHAOS));
}

boolean
In_mordor_quest(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR));
}

boolean
In_mordor_forest(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR && (
		on_level(&u.uz, &forest_1_level)
		|| on_level(&u.uz, &forest_2_level)
		|| on_level(&u.uz, &forest_3_level)
	)));
}

boolean
In_mordor_fields(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR && (
		on_level(&u.uz, &forest_4_level)
	)));
}

boolean
In_mordor_buildings(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR && (
		on_level(&u.uz, &mordor_1_level)
		|| on_level(&u.uz, &mordor_2_level)
	)));
}

boolean
In_mordor_depths(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR && (
		on_level(&u.uz, &mordor_depths_1_level)
		|| on_level(&u.uz, &mordor_depths_2_level)
		|| on_level(&u.uz, &mordor_depths_3_level)
	)));
}

boolean
In_mordor_borehole(lev)	/* are you on the chaotic quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == chaos_dnum && chaos_dvariant == MORDOR && (
		on_level(&u.uz, &borehole_1_level)
		|| on_level(&u.uz, &borehole_2_level)
		|| on_level(&u.uz, &borehole_3_level)
		|| on_level(&u.uz, &borehole_4_level)
	)));
}

boolean
In_law(lev)	/* are you on the lawful quest? */
d_level	*lev;
{
	return((boolean)(lev->dnum == law_dnum));
}

boolean
In_paths_of_law(lev)	/* In the paths of law region? */
d_level	*lev;
{
	return((boolean)(lev->dnum == law_dnum));
}


/*
 * Return the branch for the given dungeon.
 *
 * This function assumes:
 *	+ This is not called with "Dungeons of Doom".
 *	+ There is only _one_ branch to a given dungeon.
 *	+ Field end2 is the "child" dungeon.
 */
branch *
dungeon_branch(s)
    const char *s;
{
    branch *br;
    int  dnum;

    dnum = dname_to_dnum(s);

    /* Find the branch that connects to dungeon i's branch. */
    for (br = branches; br; br = br->next)
	if (br->end2.dnum == dnum) break;

    if (!br) panic("dgn_entrance: can't find entrance to %s", s);

    return br;
}

/*
 * This returns true if the hero is on the same level as the entrance to
 * the named dungeon.
 *
 * Called from do.c and mklev.c.
 *
 * Assumes that end1 is always the "parent".
 */
boolean
at_dgn_entrance(s)
    const char *s;
{
    branch *br;

    br = dungeon_branch(s);
    return((boolean)(on_level(&u.uz, &br->end1) ? TRUE : FALSE));
}

boolean
In_V_tower(lev)	/* is `lev' part of Vlad's tower? */
d_level	*lev;
{
	return((boolean)(lev->dnum == tower_dnum));
}

boolean
On_W_tower_level(lev)	/* is `lev' a level containing the Wizard's tower? */
d_level	*lev;
{
	return (boolean)(Is_wiz1_level(lev) ||
			 Is_wiz2_level(lev) ||
			 Is_wiz3_level(lev));
}

boolean
In_W_tower(x, y, lev)	/* is <x,y> of `lev' inside the Wizard's tower? */
int	x, y;
d_level	*lev;
{
	if (!On_W_tower_level(lev)) return FALSE;
	/*
	 * Both of the exclusion regions for arriving via level teleport
	 * (from above or below) define the tower's boundary.
	 *	assert( updest.nIJ == dndest.nIJ for I={l|h},J={x|y} );
	 */
	if (dndest.nlx > 0)
	    return (boolean)within_bounded_area(x, y, dndest.nlx, dndest.nly,
						dndest.nhx, dndest.nhy);
	else
	    impossible("No boundary for Wizard's Tower?");
	return FALSE;
}

#endif /* OVL1 */
#ifdef OVL0

boolean
In_hell(lev)	/* are you in one of the Hell levels? */
d_level	*lev;
{
	return((boolean)(dungeons[lev->dnum].flags.hellish));
}

#endif /* OVL0 */
#ifdef OVL1

void
find_hell(lev)	/* sets *lev to be the gateway to Gehennom... */
d_level *lev;
{
	lev->dnum = valley_level.dnum;
	lev->dlevel = 1;
}

void
goto_hell(at_stairs, falling)	/* go directly to hell... */
boolean	at_stairs, falling;
{
	d_level lev;

	find_hell(&lev);
	goto_level(&lev, at_stairs, falling, FALSE);
}

void
assign_level(dest, src)		/* equivalent to dest = source */
d_level	*dest, *src;
{
	dest->dnum = src->dnum;
	dest->dlevel = src->dlevel;
}

void
assign_rnd_level(dest, src, range)	/* dest = src + rn1(range) */
d_level	*dest, *src;
int range;
{
	dest->dnum = src->dnum;
	dest->dlevel = src->dlevel + ((range > 0) ? rnd(range) : -rnd(-range)) ;

	if(dest->dlevel > dunlevs_in_dungeon(dest))
		dest->dlevel = dunlevs_in_dungeon(dest);
	else if(dest->dlevel < 1)
		dest->dlevel = 1;
}

#endif /* OVL1 */
#ifdef OVL0

/* 
 * pct/100 of the time, returns the alignment of the current level / branch
 * otherwise, returns randomly between A_LAWFUL/A_NEUTRAL/A_CHAOTIC
 */
int
induced_align(pct)
int	pct;
{
	s_level	*lev = Is_special(&u.uz);
	aligntyp al;

	if (lev && lev->flags.align)
		if(rn2(100) < pct) return(Amask2align(lev->flags.align));

	if(dungeons[u.uz.dnum].flags.align)
		if(rn2(100) < pct) return(Amask2align(dungeons[u.uz.dnum].flags.align));

	al = rn2(3) - 1;
	return(al);
}

#endif /* OVL0 */
#ifdef OVL1

boolean
Invocation_lev(lev)
d_level *lev;
{
	return((boolean)(lev->dnum == sanctum_level.dnum && lev->dlevel == (sanctum_level.dlevel - 1)));
	//return((boolean)(In_hell(lev) &&
	//	lev->dlevel == (dungeons[lev->dnum].num_dunlevs - 1)));
}

/* use instead of depth() wherever a degree of difficulty is made
 * dependent on the location in the dungeon (eg. monster creation).
 */
int
level_difficulty()
{
	int dpth;
	if (In_endgame(&u.uz))
		dpth = ((int)(depth(&sanctum_level) + u.ulevel/2));
	else if(Infuture)
		dpth = Is_nemesis(&u.uz) ? ((int)(depth(&sanctum_level) + u.ulevel/2)) : (((int) depth(&u.uz)) + u.ulevel);
	else
		if (u.uhave.amulet)
			dpth = (deepest_lev_reached(FALSE));
		else
			dpth = ((int) depth(&u.uz));
	
	if(flags.descendant && !(
		(Role_if(PM_CONVICT && !Race_if(PM_SALAMANDER))
		|| (Role_if(PM_HEALER) && Race_if(PM_DROW))
		|| Role_if(PM_MADMAN))
		&& u.ulevel < 14)
	)
		dpth += 10;

	return max(1, dpth);
}

void
name_by_lev(char *buf, d_level *lev){
	Sprintf(buf, "level %d of %s", lev->dlevel, dungeons[lev->dnum].dname);
}


/* Take one word and try to match it to a level.
 * Recognized levels are as shown by print_dungeon().
 */
schar
lev_by_name(nam)
const char *nam;
{
    schar lev = 0;
    s_level *slev;
    d_level dlev;
    const char *p;
    int idx, idxtoo;
    char buf[BUFSZ];

    /* allow strings like "the oracle level" to find "oracle" */
    if (!strncmpi(nam, "the ", 4)) nam += 4;
    if ((p = strstri(nam, " level")) != 0 && p == eos((char*)nam) - 6) {
	nam = strcpy(buf, nam);
	*(eos(buf) - 6) = '\0';
    }
    /* hell is the old name, and wouldn't match; gehennom would match its
       branch, yielding the castle level instead of the valley of the dead */
    if (!strcmpi(nam, "gehennom") || !strcmpi(nam, "hell")) {
	if (In_V_tower(&u.uz)) nam = " to Vlad's tower";  /* branch to... */
	else nam = "valley";
    }

    if ((slev = find_level(nam)) != 0) {
	dlev = slev->dlevel;
	idx = ledger_no(&dlev);
	if ((dlev.dnum == u.uz.dnum ||
		/* within same branch, or else main dungeon <-> gehennom */
		(u.uz.dnum == valley_level.dnum &&
			dlev.dnum == challenge_level.dnum) ||
		(u.uz.dnum == challenge_level.dnum &&
			dlev.dnum == valley_level.dnum)) &&
	    (	/* either wizard mode or else seen and not forgotten */
#ifdef WIZARD
	     wizard ||
#endif
		(level_info[idx].flags & (FORGOTTEN|VISITED)) == VISITED)) {
	    lev = depth(&slev->dlevel);
	}
    } else {	/* not a specific level; try branch names */
	idx = find_branch(nam, (struct proto_dungeon *)0);
	/* "<branch> to Xyzzy" */
	if (idx < 0 && (p = strstri(nam, " to ")) != 0)
	    idx = find_branch(p + 4, (struct proto_dungeon *)0);

	if (idx >= 0) {
	    idxtoo = (idx >> 8) & 0x00FF;
	    idx &= 0x00FF;
	    if (  /* either wizard mode, or else _both_ sides of branch seen */
#ifdef WIZARD
		wizard ||
#endif
		((level_info[idx].flags & (FORGOTTEN|VISITED)) == VISITED &&
		 (level_info[idxtoo].flags & (FORGOTTEN|VISITED)) == VISITED)) {
		if (ledger_to_dnum(idxtoo) == u.uz.dnum) idx = idxtoo;
		dlev.dnum = ledger_to_dnum(idx);
		dlev.dlevel = ledger_to_dlev(idx);
		lev = depth(&dlev);
	    }
	}
    }
    return lev;
}

#ifdef WIZARD

/* Convert a branch type to a string usable by print_dungeon(). */
STATIC_OVL const char *
br_string(type)
    int type;
{
    switch (type) {
	case BR_PORTAL:	 return "Portal";
	case BR_NO_END1: return "Connection";
	case BR_NO_END2: return "One way stair";
	case BR_STAIR:	 return "Stair";
    }
    return " (unknown)";
}

/* Print all child branches between the lower and upper bounds. */
STATIC_OVL void
print_branch(win, dnum, lower_bound, upper_bound, bymenu, lchoices)
    winid win;
    int   dnum;
    int   lower_bound;
    int   upper_bound;
    boolean bymenu;
    struct lchoice *lchoices;
{
    branch *br;
    char buf[BUFSZ];
    anything any;

    /* This assumes that end1 is the "parent". */
    for (br = branches; br; br = br->next) {
	if (br->end1.dnum == dnum && lower_bound < br->end1.dlevel &&
					br->end1.dlevel <= upper_bound) {
	    Sprintf(buf,"   %s to %s: %d",
		    br_string(br->type),
		    dungeons[br->end2.dnum].dname,
		    depth(&br->end1));
	    if (bymenu) {
		lchoices->lev[lchoices->idx] = br->end1.dlevel;
		lchoices->dgn[lchoices->idx] = br->end1.dnum;
		lchoices->playerlev[lchoices->idx] = depth(&br->end1);
		any.a_void = 0;
		any.a_int = lchoices->idx + 1;
		add_menu(win, NO_GLYPH, &any, lchoices->menuletter,
				0, ATR_NONE, buf, MENU_UNSELECTED);
		if (lchoices->menuletter == 'z') lchoices->menuletter = 'A';
		else if (lchoices->menuletter == 'Z') lchoices->menuletter = 'a';
		else lchoices->menuletter++;
		lchoices->idx++;
	    } else
		putstr(win, 0, buf);
	}
    }
}

/* Print available dungeon information. */
boolean
print_dungeon(bymenu, gotdgn, rlev, rdgn)
boolean bymenu;	/* If TRUE, ask for input to select a level */
boolean gotdgn;	/* Requires bymenu. If FALSE, select dungeon. If TRUE, select a level within rdgn. */
schar *rlev;	/* returns selected level depth */
int *rdgn;		/* returns selected level dungeon number */
{
    int     i, last_level, nlev;
    char    buf[BUFSZ];
    boolean first;
    s_level *slev;
    dungeon *dptr;
    branch  *br;
    anything any;
    struct lchoice lchoices;

	boolean twopartmenu = iflags.wizlevelport;
	boolean onlyselecteddungeon = (iflags.wizlevelport & WIZLVLPORT_SELECTED_DUNGEON);
	boolean dungeonsfirst = (iflags.wizlevelport & WIZLVLPORT_BRANCHES_FIRST) && bymenu && !gotdgn;

    winid   win = create_nhwindow(NHW_MENU);
    if (bymenu) {
		start_menu(win);
		lchoices.idx = 0;
		lchoices.menuletter = 'a';
    }

    for (i = 0, dptr = dungeons; i < n_dgns; i++, dptr++) {

		/* OPTION: show only levels in selected dungeon on 2nd menu */
		if (onlyselecteddungeon && gotdgn && (i != *rdgn))
			continue;
		/* floating branches cannot be teleported to - they must be anchored first */
		if ((i == knox_level.dnum) && !u.uevent.knoxmade)
			continue;

		nlev = dptr->num_dunlevs;
		if (nlev > 1)
			Sprintf(buf, "%s: levels %d to %d", dptr->dname, dptr->depth_start,
							dptr->depth_start + nlev - 1);
		else
			Sprintf(buf, "%s: level %d", dptr->dname, dptr->depth_start);

		/* Most entrances are uninteresting. */
		if (dptr->entry_lev != 1) {
			if (dptr->entry_lev == nlev)
			Strcat(buf, ", entrance from below");
			else
			Sprintf(eos(buf), ", entrance on %d",
				dptr->depth_start + dptr->entry_lev - 1);
		}

		if (bymenu) {
			if (!gotdgn && twopartmenu) {
				any.a_int = lchoices.idx + 1;
				lchoices.dgn[lchoices.idx] = i;
				add_menu(win, NO_GLYPH, &any, lchoices.menuletter, 0, dungeonsfirst ? ATR_NONE : iflags.menu_headings, buf, MENU_UNSELECTED);
				if (lchoices.menuletter == 'z') lchoices.menuletter = 'A';
				else if (lchoices.menuletter == 'Z') lchoices.menuletter = 'a';
				else lchoices.menuletter++;
				lchoices.idx++;
			}
			else {
				any.a_void = 0;
				add_menu(win, NO_GLYPH, &any, 0, 0, dungeonsfirst ? ATR_NONE : iflags.menu_headings, buf, MENU_UNSELECTED);
			}
		} else
			putstr(win, 0, buf);

		/*
		* Circle through the special levels to find levels that are in
		* this dungeon.
		*/
		if (!dungeonsfirst) {
			for (slev = sp_levchn, last_level = 0; slev; slev = slev->next) {
				if (slev->dlevel.dnum != i) continue;
				
				/* print any branches before this level */
				print_branch(win, i, last_level, slev->dlevel.dlevel, (bymenu && ((gotdgn && i == *rdgn) || !twopartmenu)), &lchoices);
				
				Sprintf(buf, "   %s: %d", slev->proto, depth(&slev->dlevel));
				if (Is_stronghold(&slev->dlevel))
					Sprintf(eos(buf), " (tune %s)", tune);
				if (bymenu) {
					if ((gotdgn && i == *rdgn) || !twopartmenu) {
						lchoices.lev[lchoices.idx] = slev->dlevel.dlevel;
						lchoices.dgn[lchoices.idx] = i;
						
						lchoices.playerlev[lchoices.idx] = depth(&slev->dlevel);
						any.a_void = 0;
						any.a_int = lchoices.idx + 1;
						add_menu(win, NO_GLYPH, &any, lchoices.menuletter,
							0, ATR_NONE, buf, MENU_UNSELECTED);
						if (lchoices.menuletter == 'z') lchoices.menuletter = 'A';
						else lchoices.menuletter++;
						lchoices.idx++;
					}
					else {
						any.a_void = 0;
						add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
					}
				} else {
					putstr(win, 0, buf);
				}
				last_level = slev->dlevel.dlevel;
			}
			/* print branches after the last special level */
			print_branch(win, i, last_level, MAXLEVEL, (bymenu && ((gotdgn && i == *rdgn) || !twopartmenu)), &lchoices);
		}
    }

    /* Print out floating branches (if any). */
    for (first = TRUE, br = branches; br; br = br->next) {
		if (br->end1.dnum == n_dgns) {
			if (first) {
				if (!bymenu) {
				putstr(win, 0, "");
				putstr(win, 0, "Floating branches");
			}
			first = FALSE;
			}
			Sprintf(buf, "   %s to %s",
				br_string(br->type), dungeons[br->end2.dnum].dname);
			if (!bymenu)
			putstr(win, 0, buf);
		}
    }
    if (bymenu) {
    	int n;
		menu_item *selected;
		int idx;

		if (lchoices.idx > 1) {
			end_menu(win, "Level teleport to where:");
			n = select_menu(win, PICK_ONE, &selected);
			destroy_nhwindow(win);
			if (n > 0)
				idx = selected[0].item.a_int - 1;
			free((genericptr_t) selected);
		}
		else {
			/* only one choice; don't bother showing it */
			end_menu(win, "");
			destroy_nhwindow(win);
			idx = 0;
			n = 1;
		}
		if (n > 0) {
			if (!gotdgn && twopartmenu) {
				*rdgn = lchoices.dgn[idx];
				return print_dungeon(bymenu, TRUE, rlev, rdgn);
			}
			if (rlev && rdgn) {
				*rlev = lchoices.lev[idx];
				*rdgn = lchoices.dgn[idx];
				return TRUE;
			}
		}
		return FALSE;
	}

    /* I hate searching for the invocation pos while debugging. -dean */
    if (Invocation_lev(&u.uz)) {
	putstr(win, 0, "");
	Sprintf(buf, "Invocation position @ (%d,%d), hero @ (%d,%d)",
		inv_pos.x, inv_pos.y, u.ux, u.uy);
	putstr(win, 0, buf);
    }
    /*
     * The following is based on the assumption that the inter-level portals
     * created by the level compiler (not the dungeon compiler) only exist
     * one per level (currently true, of course).
     */
    else if (Is_earthlevel(&u.uz) || Is_waterlevel(&u.uz)
				|| Is_firelevel(&u.uz) || Is_airlevel(&u.uz)) {
	struct trap *trap;
	for (trap = ftrap; trap; trap = trap->ntrap)
	    if (trap->ttyp == MAGIC_PORTAL) break;

	putstr(win, 0, "");
	if (trap)
	    Sprintf(buf, "Portal @ (%d,%d), hero @ (%d,%d)",
		trap->tx, trap->ty, u.ux, u.uy);
	else
	    Sprintf(buf, "No portal found.");
	putstr(win, 0, buf);
    }

    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return FALSE;
}
#endif /* WIZARD */

#endif /* OVL1 */
/* Record that the player knows about a branch from a level. This function
 * will determine whether or not it was a "real" branch that was taken.
 * This function should not be called for a transition done via level
 * teleport or via the Eye.
 */
void 
recbranch_mapseen(source, dest)
	d_level *source;
	d_level *dest;
{
	mapseen *mptr;
	branch* br;

	/* not a branch */
	if (source->dnum == dest->dnum) return;

	/* we only care about forward branches */
	for (br = branches; br; br = br->next) {
		if (on_level(source, &br->end1) && on_level(dest, &br->end2)) break;
		if (on_level(source, &br->end2) && on_level(dest, &br->end1)) return;
	}

	/* branch not found, so not a real branch. */
	if (!br) return;
  
	if ((mptr = find_mapseen(source))) {
		if (mptr->br && br != mptr->br)
			impossible("Two branches on the same level?");
		mptr->br = br;
	} else {
		impossible("Can't note branch for unseen level (%d, %d)", 
			source->dnum, source->dlevel);
	}
}

/* add a custom name to the current level */
int
donamelevel()
{
	mapseen *mptr;
	char qbuf[QBUFSZ];	/* Buffer for query text */
	char nbuf[BUFSZ];	/* Buffer for response */
	int len;

	if (!(mptr = find_mapseen(&u.uz))) return MOVE_CANCELLED;

	Sprintf(qbuf,"What do you want to call this dungeon level? ");
	getlin(qbuf, nbuf);

	if (index(nbuf, '\033')) return MOVE_CANCELLED;

	len = strlen(nbuf) + 1;
	if (mptr->custom) {
		free((genericptr_t)mptr->custom);
		mptr->custom = (char *)0;
		mptr->custom_lth = 0;
	}
	
	if (*nbuf) {
		mptr->custom = (char *) alloc(sizeof(char) * len);
		mptr->custom_lth = len;
		strcpy(mptr->custom, nbuf);
	}
   
	return MOVE_CANCELLED;
}

boolean
check_msgod(mapseen *mptr, int godnum)
{
	if(godnum >= MAX_GOD || godnum < 1){
		impossible("Attempting to check god number %d in mapseen?", godnum);
		return FALSE;
	}
	return !!(mptr->feat.msgods[(godnum-1)/16] & (0x1L << ((godnum-1)%16)));
}

void
add_msgod(mapseen *mptr, int godnum)
{
	if(godnum >= MAX_GOD || godnum < 1){
		impossible("Attempting to set god number %d in mapseen?", godnum);
	}
	mptr->feat.msgods[(godnum-1)/16] |= (0x1L << ((godnum-1)%16));
}

int
count_msgods(mapseen *mptr)
{
	int count = 0;
	for (int i = 0; i < MSGODS_ARRAY_SIZE; i++)
		count += stdc_count_ones(mptr->feat.msgods[i]);
	return count;
}

/* find the particular mapseen object in the chain */
/* may return 0 */
STATIC_OVL mapseen *
find_mapseen(lev)
d_level *lev;
{
	mapseen *mptr;

	for (mptr = mapseenchn; mptr; mptr = mptr->next)
		if (on_level(&(mptr->lev), lev)) break;

	return mptr;
}

void
forget_mapseen(ledger_no)
int ledger_no;
{
	mapseen *mptr;

	for (mptr = mapseenchn; mptr; mptr = mptr->next)
		if (dungeons[mptr->lev.dnum].ledger_start + 
			mptr->lev.dlevel == ledger_no) break;

	/* if not found, then nothing to forget */
	if (mptr) {
		mptr->feat.forgot = 1;
		mptr->br = (branch *)0;

		/* custom names are erased, not forgotten until revisted */
		if (mptr->custom) {
			mptr->custom_lth = 0;
			free((genericptr_t)mptr->custom);
			mptr->custom = (char *)0;
		}

		memset((genericptr_t) mptr->rooms, 0, sizeof(mptr->rooms));
	}
}

void
rm_mapseen(ledger_num)
int ledger_num;
{
    mapseen *mptr, *mprev = (mapseen *)0;

    for (mptr = mapseenchn; mptr; mprev = mptr, mptr = mptr->next)
        if (dungeons[mptr->lev.dnum].ledger_start + mptr->lev.dlevel
            == ledger_num)
            break;

    if (!mptr)
        return;

    if (mptr->custom)
        free((genericptr_t) mptr->custom);

    if (mprev) {
        mprev->next = mptr->next;
        free(mptr);
    } else {
        mapseenchn = mptr->next;
        free(mptr);
    }
}


STATIC_OVL void
save_mapseen(fd, mptr)
int fd;
mapseen *mptr;
{
	branch *curr;
	int count;

	count = 0;
	for (curr = branches; curr; curr = curr->next) {
		if (curr == mptr->br) break;
		count++;
	}

	bwrite(fd, (genericptr_t) &count, sizeof(int));
	bwrite(fd, (genericptr_t) &mptr->lev, sizeof(d_level));
	bwrite(fd, (genericptr_t) &mptr->feat, sizeof(mapseen_feat));
	bwrite(fd, (genericptr_t) &mptr->custom_lth, sizeof(unsigned));
	if (mptr->custom_lth)
		bwrite(fd, (genericptr_t) mptr->custom, 
		sizeof(char) * mptr->custom_lth);
	bwrite(fd, (genericptr_t) &mptr->rooms, sizeof(mptr->rooms));
}

STATIC_OVL mapseen *
load_mapseen(fd)
int fd;
{
	int branchnum, count;
	mapseen *load;
	branch *curr;

	load = (mapseen *) alloc(sizeof(mapseen));
	mread(fd, (genericptr_t) &branchnum, sizeof(int));

	count = 0;
	for (curr = branches; curr; curr = curr->next) {
		if (count == branchnum) break;
		count++;
	}
	load->br = curr;

	mread(fd, (genericptr_t) &load->lev, sizeof(d_level));
	mread(fd, (genericptr_t) &load->feat, sizeof(mapseen_feat));
	mread(fd, (genericptr_t) &load->custom_lth, sizeof(unsigned));
	if (load->custom_lth > 0) {
		load->custom = (char *) alloc(sizeof(char) * load->custom_lth);
		mread(fd, (genericptr_t) load->custom, 
			sizeof(char) * load->custom_lth);
	} else load->custom = (char *) 0;
	mread(fd, (genericptr_t) &load->rooms, sizeof(load->rooms));

	return load;
}

/* Remove all mapseen objects for a particular dnum.
 * Useful during quest expulsion to remove quest levels.
 */
void
remdun_mapseen(dnum)
int dnum;
{
	mapseen *mptr, *prev;
	
	prev = mapseenchn;
	if (!prev) return;
	mptr = prev->next;

	for (; mptr; prev = mptr, mptr = mptr->next) {
		if (mptr->lev.dnum == dnum) {
			prev->next = mptr->next;
			free((genericptr_t) mptr);
			mptr = prev;
		}
	}
}

void
init_mapseen(lev)
d_level *lev;
{
	/* Create a level and insert in "sorted" order.  This is an insertion
	 * sort first by dungeon (in order of discovery) and then by level number.
	 */
	mapseen *mptr;
	mapseen *init;
	mapseen *old;
	
	init = (mapseen *) alloc(sizeof(mapseen));
	(void) memset((genericptr_t)init, 0, sizeof(mapseen));
	init->lev.dnum = lev->dnum;
	init->lev.dlevel = lev->dlevel;

	if (!mapseenchn) {
		mapseenchn = init;
		return;
	}

	/* walk until we get to the place where we should
	 * insert init between mptr and mptr->next
	 */
	for (mptr = mapseenchn; mptr->next; mptr = mptr->next) {
		if (mptr->next->lev.dnum == init->lev.dnum) break;
	}
	for (; mptr->next; mptr = mptr->next) {
		if ((mptr->next->lev.dnum != init->lev.dnum) ||
			(mptr->next->lev.dlevel > init->lev.dlevel)) break;
	}

	old = mptr->next;
	mptr->next = init;
	init->next = old;
}

#define INTEREST(feat) \
	((feat).nfount) || \
	((feat).nforge) || \
	((feat).nsink) || \
	((feat).ngrave) || \
	((feat).nthrone) || \
	((feat).naltar) || \
	((feat).nshop) || \
	((feat).nmorgue) || \
	((feat).ntemple) || \
	((feat).ntree)
	/*
	|| ((feat).water) || \
	((feat).ice) || \
	((feat).lava)
	*/

/* returns true if this level has something interesting to print out */
STATIC_OVL boolean
interest_mapseen(mptr)
mapseen *mptr;
{
	return ((on_level(&u.uz, &mptr->lev) || (!mptr->feat.forgot)) && (
		INTEREST(mptr->feat) ||
		(Is_hell1(&mptr->lev)) || 
		(Is_hell2(&mptr->lev)) || 
		(Is_hell3(&mptr->lev)) || 
		(Is_abyss1(&mptr->lev)) || 
		(Is_abyss2(&mptr->lev)) || 
		(Is_abyss3(&mptr->lev)) || 
		(mptr->custom) || 
		(mptr->br)
	));
}

/* recalculate mapseen for the current level */
void
recalc_mapseen()
{
	mapseen *mptr;
	struct monst *shkp;
	int x, y, ridx;

	/* Should not happen in general, but possible if in the process
	 * of being booted from the quest.  The mapseen object gets
	 * removed during the expulsion but prior to leaving the level
	 */
	if (!(mptr = find_mapseen(&u.uz))) return;

	/* reset all features */
	memset((genericptr_t) &mptr->feat, 0, sizeof(mapseen_feat));

	/* track rooms the hero is in */
	for (x = 0; x < sizeof(u.urooms); x++) {
		if (!u.urooms[x]) continue;

		ridx = u.urooms[x] - ROOMOFFSET;
		if (rooms[ridx].rtype < SHOPBASE ||
			((shkp = shop_keeper(u.urooms[x])) && inhishop(shkp)))
			mptr->rooms[ridx] |= MSR_SEEN;
		else
			/* shops without shopkeepers are no shops at all */
			mptr->rooms[ridx] &= ~MSR_SEEN;
	}

	/* recalculate room knowledge: for now, just shops and temples
	 * this could be extended to an array of 0..SHOPBASE
	 */
	for (x = 0; x < sizeof(mptr->rooms); x++) {
		if (mptr->rooms[x] & MSR_SEEN) {
			if (rooms[x].rtype >= SHOPBASE) {
				if (!mptr->feat.nshop)
					mptr->feat.shoptype = rooms[x].rtype;
				else if (mptr->feat.shoptype != rooms[x].rtype)
					mptr->feat.shoptype = 0;
				mptr->feat.nshop = min(mptr->feat.nshop + 1, 3);
			} else if (rooms[x].rtype == TEMPLE)
				/* altar and temple alignment handled below */
				mptr->feat.ntemple = min(mptr->feat.ntemple + 1, 3);
			else if (rooms[x].rtype == MORGUE)
				mptr->feat.nmorgue = min(mptr->feat.nmorgue + 1, 3);
		}
	}

	/* Update styp with typ if and only if it is in sight or the hero can
	 * feel it on their current location (i.e. not levitating).  This *should*
	 * give the "last known typ" for each dungeon location.  (At the very least,
	 * it's a better assumption than determining what the player knows from
	 * the glyph and the typ (which is isn't quite enough information in some
	 * cases).
	 *
	 * It was reluctantly added to struct rm to track.  Alternatively
	 * we could track "features" and then update them all here, and keep
	 * track of when new features are created or destroyed, but this
	 * seemed the most elegant, despite adding more data to struct rm.
	 *
	 * Although no current windowing systems (can) do this, this would add the
	 * ability to have non-dungeon glyphs float above the last known dungeon
	 * glyph (i.e. items on fountains).
	 *
	 * (vision-related styp update done in loop below)
	 */
	if (!Levitation)
		levl[u.ux][u.uy].styp = levl[u.ux][u.uy].typ;

	for (x = 0; x < COLNO; x++) {
		for (y = 0; y < ROWNO; y++) {
			/* update styp from viz_array */
			if (viz_array[y][x] & IN_SIGHT)
				levl[x][y].styp = levl[x][y].typ;

			switch (levl[x][y].styp) {
			/*
			case ICE:
				mptr->feat.ice = 1;
				break;
			case POOL:
			case MOAT:
			case WATER:
				mptr->feat.water = 1;
				break;
			case LAVAPOOL:
				mptr->feat.lava = 1;
				break;
			*/
			case TREE:
				mptr->feat.ntree = min(mptr->feat.ntree + 1, 3);
				break;
			case FOUNTAIN:
				mptr->feat.nfount = min(mptr->feat.nfount + 1, 3);
				break;
			case FORGE:
				mptr->feat.nforge = min(mptr->feat.nforge + 1, 3);
				break;
			case GRAVE:
				mptr->feat.ngrave = min(mptr->feat.ngrave + 1, 3);
				break;
			case THRONE:
				mptr->feat.nthrone = min(mptr->feat.nthrone + 1, 3);
				break;
			case SINK:
				mptr->feat.nsink = min(mptr->feat.nsink + 1, 3);
				break;
			case ALTAR:
				add_msgod(mptr, god_at_altar(x,y));
				mptr->feat.naltar = min(mptr->feat.naltar + 1, 3);
				break;
			}
		}
	}
}

int
dooverview()
{
	winid win;
	mapseen *mptr;
	boolean first;
	boolean printdun;
	boolean intrest;
	int lastdun;

	first = TRUE;
	intrest = FALSE;

	/* lazy intialization */
	(void) recalc_mapseen();

	if(mapseenchn){
		for (mptr = mapseenchn; mptr; mptr = mptr->next)
			if (interest_mapseen(mptr)) {
				intrest = TRUE;
				break;
			}
	}
	if(intrest){
		win = create_nhwindow(NHW_MENU);
		for (mptr = mapseenchn; mptr; mptr = mptr->next) {
			/* only print out info for a level or a dungeon if interest */
			if (interest_mapseen(mptr)) {
				printdun = (first || lastdun != mptr->lev.dnum);
				/* if (!first) putstr(win, 0, ""); */
				print_mapseen(win, mptr, printdun);

				if (printdun) {
					first = FALSE;
					lastdun = mptr->lev.dnum;
				}
			}
		}

		display_nhwindow(win, TRUE);
		destroy_nhwindow(win);
	} else {
		You("have found nothing of note.");
	}
	return MOVE_CANCELLED;
}

STATIC_OVL char *
seen_string(x, obj)
xchar x;
const char *obj;
{
	/* players are computer scientists: 0, 1, 2, n */
	switch(x) {
	case 0: return "no";
	/* an() returns too much.  index is ok in this case */
	case 1: return index(vowels, *obj) ? "an" : "a";
	case 2: return "some";
	case 3: return "many";
	}

	return "(unknown)";
}

/* better br_string */
STATIC_OVL const char *
br_string2(br)
branch *br;
{
	/* Special case: quest portal says closed if kicked from quest */
	boolean closed_portal = 
		(br->end2.dnum == quest_dnum && u.uevent.qexpelled);
	switch(br->type)
	{
	case BR_PORTAL:	 return closed_portal ? "Sealed portal" : "Portal";
	case BR_NO_END1: return "Connection";
	case BR_NO_END2: return (br->end1_up) ? "One way stairs up" : 
		"One way stairs down";
	case BR_STAIR:	 return (br->end1_up) ? "Stairs up" : "Stairs down";
	}

	return "(unknown)";
}

STATIC_OVL const char*
shop_string(rtype)
int rtype;
{
	/* Yuck, redundancy...but shclass.name doesn't cut it as a noun */
	switch(rtype) {
		case SHOPBASE:
			return "general store";
		case ARMORSHOP:
			return "armor shop";
		case SCROLLSHOP:
			return "scroll shop";
		case POTIONSHOP:
			return "potion shop";
		case WEAPONSHOP:
			return "weapon shop";
		case FOODSHOP:
			return "delicatessen";
		case RINGSHOP:
			return "jewelers";
		case WANDSHOP:
			return "wand shop";
		case BOOKSHOP:
			return "bookstore";
		case CANDLESHOP:
			return "lighting shop";
		default:
			/* In case another patch adds a shop type that doesn't exist,
			 * do something reasonable like "a shop".
			 */
			return "shop";
	}
}

/* some utility macros for print_mapseen */
#define TAB "   "
#define SBULLET ""
#define PREFIX TAB TAB SBULLET
#define COMMA (i++ > 0 ? ", " : PREFIX)
#define ADDNTOBUF(nam, var) { if (var) \
	Sprintf(eos(buf), "%s%s " nam "%s", COMMA, seen_string((var), (nam)), \
	((var) != 1 ? "s" : "")); }
#define ADDTOBUF(nam, var) { if (var) Sprintf(eos(buf), "%s " nam, COMMA); }

STATIC_OVL void
print_mapseen(win, mptr, printdun)
winid win;
mapseen *mptr;
boolean printdun;
{
	char buf[BUFSZ];
	int i, depthstart;

	/* Damnable special cases */
	/* The quest and knox should appear to be level 1 to match
	 * other text.
	 */
	if (mptr->lev.dnum == quest_dnum || mptr->lev.dnum == knox_level.dnum)
		depthstart = 1;
	else
		depthstart = dungeons[mptr->lev.dnum].depth_start;  

	if (printdun) {
		/* Sokoban lies about dunlev_ureached and we should
		 * suppress the negative numbers in the endgame.
		 */
		if (dungeons[mptr->lev.dnum].dunlev_ureached == 1 ||
			mptr->lev.dnum == sokoban_dnum || In_endgame(&mptr->lev))
			Sprintf(buf, "%s:", dungeons[mptr->lev.dnum].dname);
		else
			Sprintf(buf, "%s: levels %d to %d", 
				dungeons[mptr->lev.dnum].dname,
				depthstart, depthstart + 
				dungeons[mptr->lev.dnum].dunlev_ureached - 1);
		putstr(win, ATR_INVERSE, buf);
	}

	/* calculate level number */
	i = depthstart + mptr->lev.dlevel - 1;
	if (Is_astralevel(&mptr->lev))
		Sprintf(buf, TAB "Astral Plane:");
	else if (In_endgame(&mptr->lev))
		/* Negative numbers are mildly confusing, since they are never
		 * shown to the player, except in wizard mode.  We could show
		 * "Level -1" for the earth plane, for example.  Instead,
		 * show "Plane 1" for the earth plane to differentiate from
		 * level 1.  There's not much to show, but maybe the player
		 * wants to #annotate them for some bizarre reason.
		 */
		Sprintf(buf, TAB "Plane %i:", -i);
	else
		Sprintf(buf, TAB "Level %d:", i);
	
#ifdef WIZARD
	/* wizmode prints out proto dungeon names for clarity */
	if (wizard) {
		s_level *slev = Is_special(&mptr->lev);
		if (slev)
			Sprintf(eos(buf), " [%s]", slev->proto);
	}
	else
#endif
	{ //Block associated with the else in the ifdef
		if(Is_hell1(&mptr->lev)){
			if(Is_bael_level(&mptr->lev)){
				Sprintf(eos(buf), " [Avernus]");
			} else if(Is_dis_level(&mptr->lev)){
				Sprintf(eos(buf), " [Dis]");
			} else if(Is_mammon_level(&mptr->lev)){
				Sprintf(eos(buf), " [Minauros]");
			} else if(Is_belial_level(&mptr->lev)){
				Sprintf(eos(buf), " [Phlegethos]");
			} else if(Is_chromatic_level(&mptr->lev)){
				Sprintf(eos(buf), " [Dragon Caves]");
			} else {
				Sprintf(eos(buf), " [Upper Hell]");
			}
		}
		else if(Is_hell2(&mptr->lev)){
			if(Is_leviathan_level(&mptr->lev)){
				Sprintf(eos(buf), " [Stygia]");
			} else if(Is_lilith_level(&mptr->lev)){
				Sprintf(eos(buf), " [Malbolge]");
			} else if(Is_baalzebub_level(&mptr->lev)){
				Sprintf(eos(buf), " [Maladomini]");
			} else if(Is_mephisto_level(&mptr->lev)){
				Sprintf(eos(buf), " [Cania]");
			} else {
				Sprintf(eos(buf), " [Lower Hell]");
			}
		}
		else if(Is_hell3(&mptr->lev)){
			Sprintf(eos(buf), " [Nessus]");
		}
		else if(Is_abyss1(&mptr->lev)){
			if(Is_juiblex_level(&mptr->lev)){
				Sprintf(eos(buf), " [Slime Pits]");
			} else if(Is_zuggtmoy_level(&mptr->lev)){
				Sprintf(eos(buf), " [Shedaklah]");
			} else if(Is_yeenoghu_level(&mptr->lev)){
				Sprintf(eos(buf), " [Dun Savannah]");
			} else if(Is_baphomet_level(&mptr->lev)){
				Sprintf(eos(buf), " [Lyktion]");
			} else if(Is_night_level(&mptr->lev)){
				Sprintf(eos(buf), " [Bone Plateau]");
			} else if(Is_kostchtchie_level(&mptr->lev)){
				Sprintf(eos(buf), " [Iron Wastes]");
			} else {
				Sprintf(eos(buf), " [First Abyss]");
			}
		}
		else if(Is_abyss2(&mptr->lev)){
			if(Is_malcanthet_level(&mptr->lev)){
				Sprintf(eos(buf), " [Shendilavri]");
			} else if(Is_grazzt_level(&mptr->lev)){
				Sprintf(eos(buf), " [Azzagrat]");
			} else if(Is_orcus_level(&mptr->lev)){
				Sprintf(eos(buf), " [Thanatos]");
			} else if(Is_lolth_level(&mptr->lev)){
				Sprintf(eos(buf), " [Demonweb]");
			} else {
				Sprintf(eos(buf), " [Second Abyss]");
			}
		}
		else if(Is_abyss3(&mptr->lev)){
			if(Is_demogorgon_level(&mptr->lev)){
				Sprintf(eos(buf), " [Brine Flats]");
			} else if(Is_dagon_level(&mptr->lev)){
				Sprintf(eos(buf), " [Shadowsea]");
			} else if(Is_lamashtu_level(&mptr->lev)){
				Sprintf(eos(buf), " [Salt Flats]");
			} else {
				Sprintf(eos(buf), " [Third Abyss]");
			}
		}
		else if(In_outlands(&mptr->lev)){
			if(dunlev(&mptr->lev) == 1) Sprintf(eos(buf)," [Gatetown]");
			else if(dunlev(&mptr->lev) == 6) Sprintf(eos(buf)," [Spire]");
			else if(dunlev(&mptr->lev) == 7) Sprintf(eos(buf)," [Sum of All]");
		}
	}

	if (mptr->custom)
		Sprintf(eos(buf), " (%s)", mptr->custom);

	/* print out glyph or something more interesting? */
	Sprintf(eos(buf), "%s", on_level(&u.uz, &mptr->lev) ? 
		" <- You are here" : "");
	putstr(win, ATR_BOLD, buf);

	if (mptr->feat.forgot) return;

	if (INTEREST(mptr->feat)) {
		buf[0] = 0;
		
		i = 0; /* interest counter */

		/* List interests in an order vaguely corresponding to
		 * how important they are.
		 */
		if (mptr->feat.nshop > 1)
			ADDNTOBUF("shop", mptr->feat.nshop)
		else if (mptr->feat.nshop == 1)
			Sprintf(eos(buf), "%s%s", COMMA, 
				an(shop_string(mptr->feat.shoptype)));

		/* Temples + non-temple altars get munged into just "altars" */
		if (!mptr->feat.ntemple || mptr->feat.ntemple != mptr->feat.naltar)
			ADDNTOBUF("altar", mptr->feat.naltar)
		else
			ADDNTOBUF("temple", mptr->feat.ntemple)
		/* and print out altar's god if they are all to one god */
		if ((mptr->feat.ntemple || mptr->feat.naltar) && count_msgods(mptr) == 1) {
			int godnum;
			for (godnum = 1; godnum < MAX_GOD; godnum++)
				if (check_msgod(mptr, godnum))
					break;
			Sprintf(eos(buf), " to %s", godname(godnum));
		}

		ADDNTOBUF("fountain", mptr->feat.nfount)
		ADDNTOBUF("forge", mptr->feat.nforge)
		ADDNTOBUF("sink", mptr->feat.nsink)
		if(mptr->feat.nmorgue){
			ADDNTOBUF("graveyard", mptr->feat.nmorgue)
		} else {
			ADDNTOBUF("grave", mptr->feat.ngrave)
		}
		ADDNTOBUF("throne", mptr->feat.nthrone)
		ADDNTOBUF("tree", mptr->feat.ntree);
		/*
		ADDTOBUF("water", mptr->feat.water)
		ADDTOBUF("lava", mptr->feat.lava)
		ADDTOBUF("ice", mptr->feat.ice)
		*/

		/* capitalize afterwards */
		i = strlen(PREFIX);
		buf[i] = toupper(buf[i]);

		putstr(win, 0, buf);
	}

	/* print out branches */
	if (mptr->br) {
		Sprintf(buf, PREFIX "%s to %s", br_string2(mptr->br), 
			dungeons[mptr->br->end2.dnum].dname);

		/* since mapseen objects are printed out in increasing order
		 * of dlevel, clarify which level this branch is going to
		 * if the branch goes upwards.  Unless it's the end game
		 */
		if (mptr->br->end1_up && !In_endgame(&(mptr->br->end2)))
			Sprintf(eos(buf), ", level %d", depth(&(mptr->br->end2)));
		putstr(win, 0, buf);
	}
}

void
dust_storm()
{
	int x, y;
	struct trap *ttmp;
	for (x = 0; x < COLNO; x++) {
		for (y = 0; y < ROWNO; y++) {
			if(IS_SAND(levl[x][y].typ)){
				wipe_engr_at(x, y, rnd(3));
				ttmp = t_at(x, y);
				if(ttmp && 
					(ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT)
					&& !rn2(200)
				){
					del_engr_at(x, y);
					if(u.ux == x && u.uy == y && u.utrap && u.utraptype == TT_PIT){
						pline("The pit fills in!");
						nomul(-3, "stuck in the sand");
					}
					deltrap(ttmp);
					bury_objs(x, y);
					newsym(x,y);
				} else if(!rn2(6000)){ //1/3rd the bury rate (objects tend to get buried)
					unearth_objs(x, y);
					newsym(x,y);
				} else if(!rn2(2000)){ //Somewhat less than 1 square per turn in a full map of sand.
					bury_objs(x, y);
					newsym(x,y);
				}
				if(In_mithardir_desert(&u.uz)){
					if(!rn2(6000)){ //Dust clouds last many turns
						create_dust_cloud(x, y, 1, 1);
					}
				}
			}
		}
	}
}


/*dungeon.c*/
