/*	SCCS Id: @(#)mkmaze.c	3.4	2002/04/04	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "sp_lev.h"
#include "lev.h"	/* save & restore info */

/* from sp_lev.c, for fixup_special() */
extern char *lev_message;
extern lev_region *lregions;
extern int num_lregions;

STATIC_DCL boolean FDECL(iswall,(int,int));
STATIC_DCL boolean FDECL(iswall_or_stone,(int,int));
STATIC_DCL boolean FDECL(is_solid,(int,int));
STATIC_DCL int FDECL(extend_spine, (int [3][3], int, int, int));
STATIC_DCL boolean FDECL(okay,(int,int,int,int));
STATIC_DCL void FDECL(maze0xy,(coord *));
STATIC_DCL boolean FDECL(put_lregion_here,(XCHAR_P,XCHAR_P,XCHAR_P,
	XCHAR_P,XCHAR_P,XCHAR_P,XCHAR_P,BOOLEAN_P,d_level *));
STATIC_DCL void NDECL(fixup_special);
STATIC_DCL boolean FDECL(maze_inbounds, (XCHAR_P, XCHAR_P));
STATIC_DCL void FDECL(move, (int *,int *,int));
STATIC_DCL void NDECL(setup_waterlevel);
STATIC_DCL void NDECL(unsetup_waterlevel);
STATIC_DCL void NDECL(fill_dungeon_of_ill_regard);


STATIC_OVL boolean
iswall(x,y)
int x,y;
{
    register int type;

    if (!isok(x,y)) return FALSE;
    type = levl[x][y].typ;
    return (IS_WALL(type) || IS_DOOR(type) ||
	    type == SDOOR || type == IRONBARS);
}

STATIC_OVL boolean
iswall_or_stone(x,y)
    int x,y;
{
    register int type;

    /* out of bounds = stone */
    if (!isok(x,y)) return TRUE;

    type = levl[x][y].typ;
    return (type == STONE || IS_WALL(type) || IS_DOOR(type) ||
	    type == SDOOR || type == IRONBARS);
}

/* return TRUE if out of bounds, wall or rock */
STATIC_OVL boolean
is_solid(x,y)
    int x, y;
{
    return (!isok(x,y) || IS_STWALL(levl[x][y].typ));
}


/*
 * Return 1 (not TRUE - we're doing bit vectors here) if we want to extend
 * a wall spine in the (dx,dy) direction.  Return 0 otherwise.
 *
 * To extend a wall spine in that direction, first there must be a wall there.
 * Then, extend a spine unless the current position is surrounded by walls
 * in the direction given by (dx,dy).  E.g. if 'x' is our location, 'W'
 * a wall, '.' a room, 'a' anything (we don't care), and our direction is
 * (0,1) - South or down - then:
 *
 *		a a a
 *		W x W		This would not extend a spine from x down
 *		W W W		(a corridor of walls is formed).
 *
 *		a a a
 *		W x W		This would extend a spine from x down.
 *		. W W
 */
STATIC_OVL int
extend_spine(locale, wall_there, dx, dy)
    int locale[3][3];
    int wall_there, dx, dy;
{
    int spine, nx, ny;

    nx = 1 + dx;
    ny = 1 + dy;

    if (wall_there) {	/* wall in that direction */
	if (dx) {
	    if (locale[ 1][0] && locale[ 1][2] && /* EW are wall/stone */
		locale[nx][0] && locale[nx][2]) { /* diag are wall/stone */
		spine = 0;
	    } else {
		spine = 1;
	    }
	} else {	/* dy */
	    if (locale[0][ 1] && locale[2][ 1] && /* NS are wall/stone */
		locale[0][ny] && locale[2][ny]) { /* diag are wall/stone */
		spine = 0;
	    } else {
		spine = 1;
	    }
	}
    } else {
	spine = 0;
    }

    return spine;
}


/*
 * Wall cleanup.  This function has two purposes: (1) remove walls that
 * are totally surrounded by stone - they are redundant.  (2) correct
 * the types so that they extend and connect to each other.
 */
void
wallification(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	uchar type;
	register int x,y;
	struct rm *lev;
	int bits;
	int locale[3][3];	/* rock or wall status surrounding positions */
	/*
	 * Value 0 represents a free-standing wall.  It could be anything,
	 * so even though this table says VWALL, we actually leave whatever
	 * typ was there alone.
	 */
	static xchar spine_array[16] = {
	    VWALL,	HWALL,		HWALL,		HWALL,
	    VWALL,	TRCORNER,	TLCORNER,	TDWALL,
	    VWALL,	BRCORNER,	BLCORNER,	TUWALL,
	    VWALL,	TLWALL,		TRWALL,		CROSSWALL
	};

	/* sanity check on incoming variables */
	if (x1<0 || x2>=COLNO || x1>x2 || y1<0 || y2>=ROWNO || y1>y2)
	    panic("wallification: bad bounds (%d,%d) to (%d,%d)",x1,y1,x2,y2);

	/* Step 1: change walls surrounded by rock to rock. */
	for(x = x1; x <= x2; x++)
	    for(y = y1; y <= y2; y++) {
		lev = &levl[x][y];
		type = lev->typ;
		if (IS_WALL(type) && type != DBWALL) {
		    if (is_solid(x-1,y-1) &&
			is_solid(x-1,y  ) &&
			is_solid(x-1,y+1) &&
			is_solid(x,  y-1) &&
			is_solid(x,  y+1) &&
			is_solid(x+1,y-1) &&
			is_solid(x+1,y  ) &&
			is_solid(x+1,y+1))
		    lev->typ = STONE;
		}
	    }

	/*
	 * Step 2: set the correct wall type.  We can't combine steps
	 * 1 and 2 into a single sweep because we depend on knowing if
	 * the surrounding positions are stone.
	 */
	for(x = x1; x <= x2; x++)
	    for(y = y1; y <= y2; y++) {
		lev = &levl[x][y];
		type = lev->typ;
		if ( !(IS_WALL(type) && type != DBWALL)) continue;

		/* set the locations TRUE if rock or wall or out of bounds */
		locale[0][0] = iswall_or_stone(x-1,y-1);
		locale[1][0] = iswall_or_stone(  x,y-1);
		locale[2][0] = iswall_or_stone(x+1,y-1);

		locale[0][1] = iswall_or_stone(x-1,  y);
		locale[2][1] = iswall_or_stone(x+1,  y);

		locale[0][2] = iswall_or_stone(x-1,y+1);
		locale[1][2] = iswall_or_stone(  x,y+1);
		locale[2][2] = iswall_or_stone(x+1,y+1);

		/* determine if wall should extend to each direction NSEW */
		bits =    (extend_spine(locale, iswall(x,y-1),  0, -1) << 3)
			| (extend_spine(locale, iswall(x,y+1),  0,  1) << 2)
			| (extend_spine(locale, iswall(x+1,y),  1,  0) << 1)
			|  extend_spine(locale, iswall(x-1,y), -1,  0);

		/* don't change typ if wall is free-standing */
		if (bits) lev->typ = spine_array[bits];
	    }
}

STATIC_OVL boolean
okay(x,y,dir,depth)
int x,y;
register int dir;
int depth;
{
	move(&x,&y,dir);
	move(&x,&y,dir);
	if(!maze_inbounds(x, y))
		return(FALSE);
	if((depth < 3) && (levl[x][y].roomno - ROOMOFFSET >= level.flags.sp_lev_nroom))	/* if we're early, we can bust through randomly-placed rooms */
		return(TRUE);
	if(levl[x][y].typ != 0)
		return(FALSE);
	return(TRUE);
}

STATIC_OVL void
maze0xy(cc)	/* find random starting point for maze generation */
	coord	*cc;
{
	int tryct = 200;
	do{
		cc->x = 3 + 2 * rn2((x_maze_max >> 1) - 1);
		cc->y = 3 + 2 * rn2((y_maze_max >> 1) - 1);
	} while (levl[cc->x][cc->y].roomno != NO_ROOM && tryct-->0);
	if (tryct == 0)
		impossible("Could not place starting point for maze generation");
	return;
}

/*
 * Bad if:
 *	pos is occupied OR
 *	pos is inside restricted region (lx,ly,hx,hy) OR
 *	NOT (pos is corridor and a maze level OR pos is a room OR pos is air)
 */
boolean
bad_location(x, y, lx, ly, hx, hy)
    xchar x, y;
    xchar lx, ly, hx, hy;
{
    return((boolean)(occupied(x, y) ||
	   within_bounded_area(x,y, lx,ly, hx,hy) ||
	   !((levl[x][y].typ == CORR && (level.flags.is_maze_lev || level.flags.is_cavernous_lev)) ||
		   (Is_waterlevel(&u.uz) && levl[x][y].typ == MOAT) ||
	       levl[x][y].typ == ROOM || 
	       levl[x][y].typ == GRASS || 
	       levl[x][y].typ == SOIL || 
	       levl[x][y].typ == SAND || 
	       levl[x][y].typ == PUDDLE || 
	       levl[x][y].typ == AIR)));
}

/* pick a location in area (lx, ly, hx, hy) but not in (nlx, nly, nhx, nhy) */
/* and place something (based on rtype) in that region */
void
place_lregion(lx, ly, hx, hy, nlx, nly, nhx, nhy, rtype, lev)
    xchar	lx, ly, hx, hy;
    xchar	nlx, nly, nhx, nhy;
    xchar	rtype;
    d_level	*lev;
{
    int trycnt;
    boolean oneshot;
    xchar x, y;

    if(!lx) { /* default to whole level */
	/*
	 * if there are rooms and this a branch, let place_branch choose
	 * the branch location (to avoid putting branches in corridors).
	 */
	if(rtype == LR_BRANCH && nroom) {
	    place_branch(Is_branchlev(&u.uz), 0, 0);
	    return;
	}

	lx = 1; hx = COLNO-1;
	ly = 1; hy = ROWNO-1;
    }

    /* first a probabilistic approach */

    oneshot = (lx == hx && ly == hy);
    for (trycnt = 0; trycnt < 200; trycnt++) {
	x = rn1((hx - lx) + 1, lx);
	y = rn1((hy - ly) + 1, ly);
	if (put_lregion_here(x,y,nlx,nly,nhx,nhy,rtype,oneshot,lev))
	    return;
    }

    /* then a deterministic one */

    oneshot = TRUE;
    for (x = lx; x <= hx; x++)
	for (y = ly; y <= hy; y++)
	    if (put_lregion_here(x,y,nlx,nly,nhx,nhy,rtype,oneshot,lev))
		return;

    impossible("Couldn't place lregion type %d!", rtype);
}

STATIC_OVL boolean
put_lregion_here(x,y,nlx,nly,nhx,nhy,rtype,oneshot,lev)
xchar x, y;
xchar nlx, nly, nhx, nhy;
xchar rtype;
boolean oneshot;
d_level *lev;
{
    if (bad_location(x, y, nlx, nly, nhx, nhy)) {
	if (!oneshot) {
	    return FALSE;		/* caller should try again */
	} else {
	    /* Must make do with the only location possible;
	       avoid failure due to a misplaced trap.
	       It might still fail if there's a dungeon feature here. */
	    struct trap *t = t_at(x,y);

	    if (t && t->ttyp != MAGIC_PORTAL) deltrap(t);
	    if (bad_location(x, y, nlx, nly, nhx, nhy)) return FALSE;
	}
    }
    switch (rtype) {
    case LR_TELE:
    case LR_UPTELE:
    case LR_DOWNTELE:
	/* "something" means the player in this case */
	if(flags.phasing){
		x = u.old_lev.ux;
		y = u.old_lev.uy;
	}
	if(MON_AT(x, y)) {
	    /* move the monster if no choice, or just try again */
	    if(oneshot) (void) rloc(m_at(x,y), FALSE);
	    else return(FALSE);
	}
	u_on_newpos(x, y);
	break;
    case LR_PORTAL:
	mkportal(x, y, lev->dnum, lev->dlevel);
	break;
    case LR_DOWNSTAIR:
    case LR_UPSTAIR:
	mkstairs(x, y, (char)rtype, room_at(x,y));
	break;
    case LR_BRANCH:
	place_branch(Is_branchlev(&u.uz), x, y);
	break;
    }
    return(TRUE);
}

static boolean was_waterlevel; /* ugh... this shouldn't be needed */

static const int angelnums[] = {PM_JUSTICE_ARCHON, PM_SWORD_ARCHON, PM_SHIELD_ARCHON, PM_TRUMPET_ARCHON, PM_WARDEN_ARCHON, PM_THRONE_ARCHON, PM_LIGHT_ARCHON, 
						  PM_MOVANIC_DEVA, PM_MONADIC_DEVA, PM_ASTRAL_DEVA, PM_GRAHA_DEVA, PM_SURYA_DEVA, PM_MAHADEVA, 
						  PM_LILLEND,
						  PM_COURE_ELADRIN, PM_NOVIERE_ELADRIN, PM_BRALANI_ELADRIN, PM_FIRRE_ELADRIN, PM_SHIERE_ELADRIN, PM_GHAELE_ELADRIN, 
							PM_TULANI_ELADRIN, PM_GAE_ELADRIN, PM_BRIGHID_ELADRIN, PM_CAILLEA_ELADRIN, 
						  PM_DAUGHTER_OF_BEDLAM, PM_MARILITH,
						  PM_ERINYS, PM_FALLEN_ANGEL, PM_ANCIENT_OF_BLESSINGS, PM_ANCIENT_OF_ICE, PM_ANCIENT_OF_DEATH
						 };

int 
init_village(rndlevs)
int rndlevs;
{
	if(Race_if(PM_ELF) || Race_if(PM_ENT))
		return FOREST_VILLAGE;
	int levvar;
	boolean levelBad = TRUE;
	while(levelBad){
		levvar = rnd(rndlevs);
		levelBad = FALSE;
		if(levvar == GRASS_VILLAGE && (Race_if(PM_DROW) || Race_if(PM_VAMPIRE) || Race_if(PM_ORC)))
			levelBad = TRUE;
		if(levvar == LAKE_VILLAGE && (Race_if(PM_DROW) || Race_if(PM_VAMPIRE) || Race_if(PM_ORC) || Race_if(PM_SALAMANDER)))
			levelBad = TRUE;
		if(levvar == FOREST_VILLAGE && (Race_if(PM_DROW) || Race_if(PM_SALAMANDER) || Race_if(PM_CLOCKWORK_AUTOMATON)))
			levelBad = TRUE;
		if(levvar == CAVE_VILLAGE && !(Race_if(PM_DROW) || Race_if(PM_GNOME) || Race_if(PM_CHIROPTERAN) || Race_if(PM_ORC) || Race_if(PM_VAMPIRE)))
			levelBad = TRUE;
	}
	return levvar;

}

/* this is special stuff that the level compiler cannot (yet) handle */
STATIC_OVL void
fixup_special()
{
    register lev_region *r = lregions;
    struct d_level lev;
    register int x, y;
    struct mkroom *croom;
    boolean added_branch = FALSE;

    if (was_waterlevel) {
	was_waterlevel = FALSE;
	u.uinwater = 0;
	u.usubwater = 0;
	unsetup_waterlevel();
    } else if (Is_waterlevel(&u.uz)) {
	level.flags.hero_memory = 0;
	was_waterlevel = TRUE;
	/* water level is an odd beast - it has to be set up
	   before calling place_lregions etc. */
	setup_waterlevel();
    }
    for(x = 0; x < num_lregions; x++, r++) {
	switch(r->rtype) {
	case LR_BRANCH:
	    added_branch = TRUE;
	    goto place_it;

	case LR_PORTAL:
	    if(*r->rname.str >= '0' && *r->rname.str <= '9') {
		/* "chutes and ladders" */
		lev = u.uz;
		lev.dlevel = atoi(r->rname.str);
	    } else {
		s_level *sp = find_level(r->rname.str);
		lev = sp->dlevel;
	    }
	    /* fall into... */

	case LR_UPSTAIR:
	case LR_DOWNSTAIR:
	place_it:
	    place_lregion(r->inarea.x1, r->inarea.y1,
			  r->inarea.x2, r->inarea.y2,
			  r->delarea.x1, r->delarea.y1,
			  r->delarea.x2, r->delarea.y2,
			  r->rtype, &lev);
	    break;

	case LR_TELE:
	case LR_UPTELE:
	case LR_DOWNTELE:
	    /* save the region outlines for goto_level() */
	    if(r->rtype == LR_TELE || r->rtype == LR_UPTELE) {
		    updest.lx = r->inarea.x1; updest.ly = r->inarea.y1;
		    updest.hx = r->inarea.x2; updest.hy = r->inarea.y2;
		    updest.nlx = r->delarea.x1; updest.nly = r->delarea.y1;
		    updest.nhx = r->delarea.x2; updest.nhy = r->delarea.y2;
	    }
	    if(r->rtype == LR_TELE || r->rtype == LR_DOWNTELE) {
		    dndest.lx = r->inarea.x1; dndest.ly = r->inarea.y1;
		    dndest.hx = r->inarea.x2; dndest.hy = r->inarea.y2;
		    dndest.nlx = r->delarea.x1; dndest.nly = r->delarea.y1;
		    dndest.nhx = r->delarea.x2; dndest.nhy = r->delarea.y2;
	    }
	    /* place_lregion gets called from goto_level() */
	    break;
	}

	if (r->rname.str) free((genericptr_t) r->rname.str),  r->rname.str = 0;
    }

    /* place dungeon branch if not placed above */
    if (!added_branch && Is_branchlev(&u.uz)) {
	place_lregion(0,0,0,0,0,0,0,0,LR_BRANCH,(d_level *)0);
    }

	/* MAIN AREAS */
	/* KMH -- Sokoban levels */
	if(In_sokoban(&u.uz))
		sokoban_detect();

	/* FORT KNOX: fill vault */
	if (Is_knox(&u.uz)) {
		/* using an unfilled morgue for rm id */
		croom = search_special(MORGUE);
		/* avoid inappropriate morgue-related messages */
		level.flags.graveyard = level.flags.has_morgue = 0;
		croom->rtype = OROOM;	/* perhaps it should be set to VAULT? */
		/* stock the main vault */
		for (x = croom->lx; x <= croom->hx; x++)
		for (y = croom->ly; y <= croom->hy; y++) {
			(void)mkgold((long)rn1(300, 600), x, y);
			if (!rn2(3) && !is_pool(x, y, TRUE))
				(void)maketrap(x, y, rn2(3) ? LANDMINE : SPIKED_PIT);
		}
	}
    /* MEDUSA'S FLOOR: add statues */
    if (Is_medusa_level(&u.uz)) {
	struct obj *otmp;
	int tryct;
	croom = &rooms[0]; /* only one room on the medusa level */
	for (tryct = rnd(4); tryct; tryct--) {
	    x = somex(croom); y = somey(croom);
	    if (goodpos(x, y, (struct monst *)0, 0)) {
		otmp = mk_tt_object(STATUE, x, y);
		while (otmp && (poly_when_stoned(&mons[otmp->corpsenm]) ||
				pm_resistance(&mons[otmp->corpsenm],MR_STONE))) {
		    otmp->corpsenm = rndmonnum();
		    otmp->owt = weight(otmp);
		}
	    }
	}
	if (rn2(2))
	    otmp = mk_tt_object(STATUE, somex(croom), somey(croom));
	else /* Medusa statues don't contain books */
	    otmp = mkcorpstat(STATUE, (struct monst *)0, (struct permonst *)0,
			      somex(croom), somey(croom), FALSE);
	if (otmp) {
	    while (pm_resistance(&mons[otmp->corpsenm],MR_STONE)
		   || poly_when_stoned(&mons[otmp->corpsenm])) {
		otmp->corpsenm = rndmonnum();
		otmp->owt = weight(otmp);
	    }
	}
    }
	/* CASTLE: make graveyard */
	if (Is_stronghold(&u.uz)) {
		level.flags.graveyard = 1;
	}
	/* WIZARD'S TOWER: add secret door */
	if(Is_wiz1_level(&u.uz)) {
	croom = search_special(MORGUE);
	create_secret_door(croom, W_SOUTH|W_EAST|W_WEST);
    }
	/* SANCTUM: add sdoor to temple*/
	if (Is_sanctum(&u.uz)) {
		croom = search_special(TEMPLE);

		create_secret_door(croom, W_ANY);
	}
	/* ALIGNMENT QUESTS */
	/* LAW QUEST: features */
	if (In_law(&u.uz)){
		/*Convert the room squares left by mazewalk to grass prior to placing shops*/
		if (Is_arcadia_woods(&u.uz)){
			for (x = 0; x<COLNO; x++){
				for (y = 0; y<ROWNO; y++){
					if (levl[x][y].typ == ROOM && (x<69 || !Is_arcadia3(&u.uz))) levl[x][y].typ = GRASS;
				}
			}
		}
		if (Is_illregrd(&u.uz)){
			fill_dungeon_of_ill_regard();
		}
		place_law_features();
	}
	/* CHAOS QUEST 2: various features */
	if (In_mithardir_quest(&u.uz)){
		if (In_mithardir_desert(&u.uz) || on_level(&u.uz, &elshava_level)){
			int i, j, walls;
			for (x = 0; x<COLNO; x++){
				for (y = 0; y<ROWNO; y++){
					levl[x][y].lit = TRUE;
					if (m_at(x, y) && !ACCESSIBLE(levl[x][y].typ))
						rloc(m_at(x, y), TRUE);
					if(OBJ_AT(x, y) && !ACCESSIBLE(levl[x][y].typ))
						rlocos_at(x, y);
					if(In_mithardir_desert(&u.uz) && IS_WALL(levl[x][y].typ)){
						walls = 0;
						i = -1; j = 0;
						if(isok(x+i,y+j) && IS_WALL(levl[x+i][y+j].typ))
							walls++;
						i = 0; j = -1;
						if(isok(x+i,y+j) && IS_WALL(levl[x+i][y+j].typ))
							walls++;
						i = 1; j = 0;
						if(isok(x+i,y+j) && IS_WALL(levl[x+i][y+j].typ))
							walls++;
						i = 0; j = 1;
						if(isok(x+i,y+j) && IS_WALL(levl[x+i][y+j].typ))
							walls++;
						if(walls < 2){
							levl[x][y].typ = STONE;
							if(walls == 1){
								//Go back to previous line
								x = max(0, x-1);
								y = max(0, y-1);
							}
						}
					}
				}
			}
			wallification(1, 0, COLNO - 1, ROWNO - 1);
		}
	}
	/* CHAOS QUEST 3: various features */
	if (In_mordor_quest(&u.uz)){
		if(In_mordor_forest(&u.uz) || Is_ford_level(&u.uz))
			place_chaos_forest_features();
		if(Is_ford_level(&u.uz) || In_mordor_fields(&u.uz)){
			for (x = 1; x<COLNO; x++){
				for (y = 0; y<ROWNO; y++){
					levl[x][y].lit = TRUE;
				}
			}
		}
		if(In_mordor_fields(&u.uz)){
			for (x = 2*COLNO/3; x<COLNO; x++){
				for (y = 0; y<ROWNO; y++){
					if (levl[x][y].typ == TREE &&
						(x > 2*COLNO/3 || rn2(2))
					) levl[x][y].typ = GRASS;
				}
			}
			//Note: Post forest-conversion
			place_chaos_forest_features();
		}
		if(on_level(&u.uz, &mordor_depths_2_level)){
			place_chaos_forest_features();
			for (x = 1; x < COLNO - 1; x++)
			for (y = 0; y < ROWNO - 1; y++){
				if (levl[x][y].typ == STONE) levl[x][y].typ = HWALL;
			}
			wallification(1, 0, COLNO - 1, ROWNO - 1);
		}
		else if(In_mordor_depths(&u.uz) || In_mordor_borehole(&u.uz)){
			place_chaos_forest_features();
		}
	}
	/*Elf huts on elf shared home*/
	if(In_quest(&u.uz) && urole.neminum == PM_NECROMANCER){
		place_elfquest_forest_features();
	}
	/* NEUTRAL QUEST: various features */
	if (In_outlands(&u.uz)){
		if (!(u.uz.dlevel == spire_level.dlevel || Is_gatetown(&u.uz) || Is_sumall(&u.uz)))
			place_neutral_features();
		if (u.uz.dlevel < gatetown_level.dlevel + 4){
			for (x = 0; x<COLNO; x++){
				for (y = 0; y<ROWNO; y++){
					if (levl[x][y].typ == TREE) levl[x][y].lit = TRUE;
				}
			}
		}
		if (u.uz.dlevel == spire_level.dlevel){
			for (x = 2; x <= x_maze_max; x++)
			for (y = 2; y <= y_maze_max; y++){
				if (m_at(x, y) && !ACCESSIBLE(levl[x][y].typ))
					rloc(m_at(x, y), TRUE);
			}
		}
		if (Is_sumall(&u.uz)){
			place_sum_all_features();
			for (x = 1; x <= COLNO - 1; x++)
			for (y = 1; y <= ROWNO - 1; y++){
				if (levl[x][y].typ == STONE) levl[x][y].typ = HWALL;
				if (!ZAP_POS(levl[x][y].typ) && m_at(x, y)) rloc(m_at(x, y), TRUE);
			}
			wallification(1, 1, COLNO - 1, ROWNO - 1);
			for (x = 1; x <= COLNO - 1; x++)
			for (y = 1; y <= ROWNO - 1; y++){
				if (levl[x][y].typ != STONE) levl[x][y].lit = TRUE;
			}
		}
	}
	/* DEMON LAIRS */
	/* ORCUS'S FLOOR: remove shopkeepers*/
	if (on_level(&u.uz, &orcus_level)) {
		register struct monst *mtmp, *mtmp2;

		/* it's a ghost town, get rid of shopkeepers */
		for (mtmp = fmon; mtmp; mtmp = mtmp2) {
			mtmp2 = mtmp->nmon;
			if (mtmp->isshk) mongone(mtmp);
		}
	}
	/* LOLTH'S FLOOR: vaults and webs */
	if (Is_lolth_level(&u.uz)){
		int x, y;
		place_lolth_vaults();
		for (x = 0; x<COLNO; x++){
			for (y = 0; y<ROWNO; y++){
				if (levl[x][y].typ == ROOM && levl[x][y].roomno == NO_ROOM) maketrap(x, y, WEB);
			}
		}
	}
	/* DISPATER'S FLOOR: place crazed angel statues in the iron bar walls*/
	if (Is_dis_level(&u.uz)){
		for (x = 0; x<COLNO; x++){
			for (y = 0; y<ROWNO; y++){
				if (levl[x][y].typ == IRONBARS){
					struct monst *angel;
					angel = makemon(&mons[angelnums[rn2(SIZE(angelnums))]], x, y, MM_EDOG | MM_ADJACENTOK | NO_MINVENT | MM_NOCOUNTBIRTH);
					if (angel){
						initedog(angel);
						angel->m_lev = is_eladrin(angel->data) ? 30 : min(30, 3 * (int)(angel->data->mlevel / 2)+1);
						angel->mhpmax = (angel->m_lev * 8) - 4;
						angel->mhp = angel->mhpmax;
						angel->female = TRUE;
						angel->mtame = 10;
						angel->mpeaceful = 1;
						angel->mcrazed = 1;
					}
					mkcorpstat(STATUE, angel, (struct permonst *)0, x, y, FALSE);
					mongone(angel);
				}
			}
		}
	}
	/* PLAYER QUESTS */
	/*Salamander*/
	if(Pantheon_if(PM_SALAMANDER) && In_quest(&u.uz) && Is_nemesis(&u.uz)) {
		/* using an unfilled morgue for rm id */
		croom = search_special(MORGUE);
		/* avoid inappropriate morgue-related messages */
		level.flags.graveyard = level.flags.has_morgue = 0;
		croom->rtype = OROOM;	/* perhaps it should be set to VAULT? */
		/* stock the main vault */
		for(x = croom->lx; x <= croom->hx; x++)
		    for(y = croom->ly; y <= croom->hy; y++) {
			if(levl[x][y].typ == ROOM) (void) mkgold((long) rn1(100, 300), x, y);
		    }
	}
	if(Pantheon_if(PM_SALAMANDER) && 
			In_quest(&u.uz) && 
			(!Is_qlocate(&u.uz) && !Is_qstart(&u.uz) && !Is_nemesis(&u.uz))
		){
			int x, y;
			struct obj* otmp;
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(levl[x][y].typ == STONE){
						levl[x][y].typ = LAVAPOOL;
						if(!rn2(100)){
							otmp = mksobj_at(GARNET, x, y, NO_MKOBJ_FLAGS);
							otmp->quan = 1L;
							otmp->oknapped = KNAPPED_SPEAR;
						}
					}
				}
			}
		}
		if(In_icecaves(&u.uz)){
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(levl[x][y].typ == ICE){
						if(!rn2(20)) levl[x][y].typ = ROOM;
						if(!rn2(500)){
							mksobj_at(PICK_AXE, x, y, NO_MKOBJ_FLAGS);
							mksobj_at(HELMET, x, y, NO_MKOBJ_FLAGS);
							if(!rn2(2)) mksobj_at(LEATHER_ARMOR, x, y, NO_MKOBJ_FLAGS);
							else mksobj_at(STUDDED_LEATHER_ARMOR, x, y, NO_MKOBJ_FLAGS);
							if(!rn2(4)) mksobj_at(HIGH_BOOTS, x, y, NO_MKOBJ_FLAGS);
							else mksobj_at(LOW_BOOTS, x, y, NO_MKOBJ_FLAGS);
							(void) mkcorpstat(CORPSE, (struct monst *) 0, 
								&mons[PM_MINER], x, y, TRUE);
						}
					}
				}
			}
		}
		if(In_blackforest(&u.uz)){
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(levl[x][y].typ == TREE){
						if(!rn2(2)) levl[x][y].typ = DEADTREE;
					}
					if(levl[x][y].typ == SOIL){
						if(!rn2(10)) levl[x][y].typ = CLOUD;
						if(!rn2(500)){
							switch(rn2(6)){
								case 0:
									mksobj_at(SICKLE, x, y, NO_MKOBJ_FLAGS);
								break;
								case 1:
									mksobj_at(SCYTHE, x, y, NO_MKOBJ_FLAGS);
								break;
								case 2:
									mksobj_at(KNIFE, x, y, NO_MKOBJ_FLAGS);
								break;
								case 3:
									mksobj_at(CLUB, x, y, NO_MKOBJ_FLAGS);
								break;
								case 4:
									mksobj_at(AXE, x, y, NO_MKOBJ_FLAGS);
								break;
								case 5:
									mksobj_at(VOULGE, x, y, NO_MKOBJ_FLAGS);
								break;
							}
							mksobj_at(CLOAK, x, y, NO_MKOBJ_FLAGS);
							mksobj_at(GLOVES, x, y, NO_MKOBJ_FLAGS);
							(void) mkcorpstat(CORPSE, (struct monst *) 0, 
								&mons[PM_PEASANT], x, y, TRUE);
						}
					}
				}
			}
		}
		if(In_dismalswamp(&u.uz)){
			place_swamp_features();
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(levl[x][y].typ == TREE){
						if(rn2(5)) levl[x][y].typ = POOL;
					}
					if(levl[x][y].typ == SOIL){
						if(!rn2(2)) levl[x][y].typ = GRASS;
					}
				}
			}
		}
		if(In_archipelago(&u.uz)){
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(t_at(x,y)) (void) mksobj_at(GILLYWEED, x, y, NO_MKOBJ_FLAGS);
					if(levl[x][y].typ == MOAT){
						if(!rn2(200)){
							if(Is_leveetwn_level(&u.uz)){
								levl[x][y].typ = ROOM;
							} else {
								struct obj *box;
								struct obj *otmp;
								box = mksobj_at(CHEST, x, y, NO_MKOBJ_FLAGS);
								if(!rn2(5)){
									set_material_gm(box, GOLD);
								}
								switch(rnd(5)){
									case 1:
										otmp = mksobj(HELMET,NO_MKOBJ_FLAGS);
									break;
									case 2:
										otmp = mksobj(GAUNTLETS,NO_MKOBJ_FLAGS);
									break;
									case 3:
										otmp = mksobj(rn2(4)?CHAIN_MAIL:PLATE_MAIL,NO_MKOBJ_FLAGS);
									break;
									case 4:
										otmp = mksobj(rn2(3)?BUCKLER:KITE_SHIELD,NO_MKOBJ_FLAGS);
									break;
									case 5:
										otmp = mksobj(rn2(3)?LOW_BOOTS:HIGH_BOOTS,NO_MKOBJ_FLAGS);
									break;
									default:
										otmp = mksobj(rn2(3)?LOW_BOOTS:HIGH_BOOTS,NO_MKOBJ_FLAGS);
									break;
								}
								add_to_container(box,otmp);
								
							}
						}
					}
					if(levl[x][y].typ == SAND && !t_at(x,y)  && !OBJ_AT(x,y)  && !rn2(10) && !Is_leveetwn_level(&u.uz) && !Is_arcboss_level(&u.uz)){
						levl[x][y].typ = TREE;
					}
				}
			}
		}
		if(In_adventure_branch(&u.uz) && dunlev(&u.uz) == 1){
			branch *br = dungeon_branch("Vlad's Tower");
			br->end1 = u.uz;
			place_branch(br,0,0);
			
		}
		if(In_tower(&u.uz) && dunlev(&u.uz) == 4 && u.ubranch){
			if(u.ubranch == ICE_CAVES){
				for(x = 21; x<COLNO; x++){
					for(y = 0; y<ROWNO; y++){
						if(levl[x][y].typ == GRASS){
							if(rn2(80)) levl[x][y].typ = ICE;
							else levl[x][y].typ = ROOM;
						}
						if(levl[x][y].typ == TREE){
							levl[x][y].typ = VWALL;
						}
					}
				}

			}
			if(u.ubranch == BLACK_FOREST){
				for(x = 21; x<COLNO; x++){
					for(y = 0; y<ROWNO; y++){
						if(levl[x][y].typ == POOL){
							levl[x][y].typ = TREE;
						}
						if(levl[x][y].typ == TREE){
							if(!rn2(2)) levl[x][y].typ = DEADTREE;
						}
						if(levl[x][y].typ == GRASS){
							if(!rn2(10)) levl[x][y].typ = CLOUD;
							else levl[x][y].typ = SOIL;
						}
					}
				}

			}
			if(u.ubranch == DISMAL_SWAMP){
				for(x = 21; x<COLNO; x++){
					for(y = 0; y<ROWNO; y++){
						if(levl[x][y].typ == STONE){
							levl[x][y].typ = TREE;
						}
						if(levl[x][y].typ == TREE){
							if(rn2(5)) levl[x][y].typ = POOL;
						}
						if(levl[x][y].typ == GRASS){
							if(!rn2(2)) levl[x][y].typ = GRASS;
							else levl[x][y].typ = SOIL;
						}
					}
				}

			}
			if(u.ubranch == ARCHIPELAGO){
				for(x = 21; x<COLNO; x++){
					for(y = 0; y<ROWNO; y++){
						if(levl[x][y].typ == GRASS && rn2(5) && !m_at(x,y)){
							levl[x][y].typ = MOAT;
						} else if(levl[x][y].typ == GRASS){
							levl[x][y].typ = SAND;
						}
						if(levl[x][y].typ == CORR){
							levl[x][y].typ = MOAT;
						}
						if(levl[x][y].typ == TREE){
							levl[x][y].typ = SAND;
						}
					}
				}

			}
		}
	/* DWARF KNIGHT QUEST: add stuff to the locate level */
	if (urole.neminum == PM_BOLG && In_quest(&u.uz) && Is_qlocate(&u.uz)) {
		int rmn, piled, disty, distx;
		/* using an unfilled morgue for rm id */
		croom = search_special(MORGUE);
		disty = croom->hy - croom->ly;
		distx = croom->hx - croom->lx;
		rmn = (croom - rooms) + ROOMOFFSET;
		/* avoid inappropriate morgue-related messages */
		level.flags.graveyard = level.flags.has_morgue = 0;
		croom->rtype = OROOM;	/* perhaps it should be set to VAULT? */
		/* stock the main vault */
		for (x = croom->lx; x <= croom->hx; x++)
		for (y = croom->ly; y <= croom->hy; y++) {
			if (!levl[x][y].edge &&
				(int)levl[x][y].roomno == rmn){
				piled = 1;
				if (y < croom->ly + disty * 1 / 3 && x > croom->lx + distx * 1 / 5 && x < croom->lx + distx * 4 / 5) piled++;
				if (y < croom->ly + disty * 2 / 3 && x > croom->lx + distx * 2 / 5 && x < croom->lx + distx * 3 / 5) piled++;
				for (; piled > 0; piled--){
					if (rn2(2)) mkobj_at(WEAPON_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (rn2(2)) mkobj_at(ARMOR_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (rn2(6)) mkobj_at(RING_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(3))mkobj_at(TOOL_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (rn2(6)) mkobj_at(SCROLL_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(4))mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(4)) mkobj_at(BELT_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(3))mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(2))mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (!rn2(4))mksobj_at(SILVER_SLINGSTONE, x, y, NO_MKOBJ_FLAGS);
					if (rn2(3)) mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
					if (rn2(4)) mkobj_at(GEM_CLASS, x, y, NO_MKOBJ_FLAGS);
				}
				(void)mkgold((long)rn1(1000, 100), x, y);
			}
		}
	}
	/* ELF QUEST: transfer equip */
	if (urole.neminum == PM_NECROMANCER && In_quest(&u.uz) && Is_nemesis(&u.uz)) {
		struct obj *chest;
		struct obj *obj, *nobj;
		struct monst *mon;
		for(chest = fobj; chest; chest = chest->nobj){
			if(chest->otyp == CHEST && IS_THRONE(levl[chest->ox][chest->oy].typ))
				break;
		}
		if(chest) for(mon = fmon; mon; mon = mon->nmon){
			if(mon->entangled_otyp != SHACKLES)
				continue;
			for(obj = mon->minvent; obj; obj = nobj){
				nobj = obj->nobj;
				if(obj->otyp == SHACKLES)
					continue;
				mon->misc_worn_check &= ~obj->owornmask;
				update_mon_intrinsics(mon, obj, FALSE, FALSE);
				if (obj->owornmask & W_WEP){
					setmnotwielded(mon,obj);
					MON_NOWEP(mon);
				}
				if (obj->owornmask & W_SWAPWEP){
					setmnotwielded(mon,obj);
					MON_NOSWEP(mon);
				}
				obj->owornmask = 0L;
				obj_extract_self(obj);
				add_to_container(chest, obj);
			}
		}
	}
	/* Undead Hunter quest: Haunted forest features */
	if (Role_if(PM_UNDEAD_HUNTER) && In_quest(&u.uz) && qlocate_level.dlevel < u.uz.dlevel && !Is_nemesis(&u.uz)) {
		place_haunted_forest_features();
	}
	/* DROW QUEST: transfer equip */
	if (urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && In_quest(&u.uz)) {
		if(qlocate_level.dlevel < u.uz.dlevel)
			place_drow_healer_features();
		struct obj *chest;
		struct obj *obj, *nobj;
		struct monst *mon;
		int ctyp = CHEST;
		if(Is_nemesis(&u.uz))
			ctyp = SACK;
		for(chest = fobj; chest; chest = chest->nobj){
			if(chest->otyp == ctyp)
				break;
		}
		if(chest) for(mon = fmon; mon; mon = mon->nmon){
			if(mon->entangled_otyp != SHACKLES)
				continue;
			for(obj = mon->minvent; obj; obj = nobj){
				nobj = obj->nobj;
				if(obj->otyp == SHACKLES)
					continue;
				mon->misc_worn_check &= ~obj->owornmask;
				update_mon_intrinsics(mon, obj, FALSE, FALSE);
				if (obj->owornmask & W_WEP){
					setmnotwielded(mon,obj);
					MON_NOWEP(mon);
				}
				if (obj->owornmask & W_SWAPWEP){
					setmnotwielded(mon,obj);
					MON_NOSWEP(mon);
				}
				obj->owornmask = 0L;
				obj_extract_self(obj);
				add_to_container(chest, obj);
			}
		}
	}
	/* PRIEST QUEST: make graveyard */
	if (Role_if(PM_PRIEST) && In_quest(&u.uz)) {
		/* less chance for undead corpses (lured from lower morgues) */
		level.flags.graveyard = 1;
	}
	/* HEALER QUEST: put some plague victims around the map */
	if (Role_if(PM_HEALER) && In_quest(&u.uz)) {
		if(!Race_if(PM_DROW)) {
			int plague_types[] = {PM_HOBBIT, PM_DWARF, PM_BUGBEAR, PM_DWARF_LORD, PM_DWARF_CLERIC,
				PM_DWARF_QUEEN, PM_DWARF_KING, PM_DEEP_ONE, PM_IMP, PM_QUASIT, PM_WINGED_KOBOLD,
				PM_DRYAD, PM_NAIAD, PM_OREAD, PM_DEMINYMPH, PM_THRIAE, PM_HILL_ORC, PM_ORC_SHAMAN, 
				PM_ORC_CAPTAIN, PM_JUSTICE_ARCHON,
				PM_MOVANIC_DEVA, PM_LILLEND, PM_COURE_ELADRIN,
				PM_CHIROPTERAN, PM_PLAINS_CENTAUR, PM_FOREST_CENTAUR, PM_MOUNTAIN_CENTAUR,
				PM_DRIDER, PM_FORMIAN_CRUSHER, PM_FORMIAN_TASKMASTER, PM_MYRMIDON_HOPLITE,
				PM_MYRMIDON_LOCHIAS, PM_MYRMIDON_YPOLOCHAGOS, PM_MYRMIDON_LOCHAGOS,
				PM_GNOME, PM_GNOME_LORD, PM_GNOME_LADY, PM_TINKER_GNOME, PM_GNOME_KING, PM_GNOME_QUEEN,
				PM_VAMPIRE, PM_VAMPIRE_LORD, PM_VAMPIRE_LADY,
				PM_PEASANT, PM_PEASANT, PM_PEASANT, PM_PEASANT, PM_NURSE, PM_WATCHMAN, PM_WATCH_CAPTAIN, 
				PM_WOODLAND_ELF, PM_GREEN_ELF, PM_GREY_ELF, PM_ELF_LORD, PM_ELF_LADY, PM_ELVENKING, PM_ELVENQUEEN,
				PM_DROW_CAPTAIN, PM_HEDROW_WIZARD,
				PM_HORNED_DEVIL, PM_SUCCUBUS, PM_INCUBUS,
				PM_BARBARIAN, PM_HALF_DRAGON, PM_BARD, PM_HEALER, PM_RANGER, PM_VALKYRIE,
				PM_SMALL_GOAT_SPAWN, PM_GOAT_SPAWN
			};
			int x, y, tries;
			for(int i = d(2,4); i >0; i--){
				tries = 10;
				do {
					x = rn2(COLNO)+1;
					y = rn2(ROWNO);
				}
				while(!(isok(x,y) && levl[x][y].typ == SOIL) && tries-->0);
				
				if(isok(x,y) && levl[x][y].typ == SOIL)
					makemon_full(&mons[ROLL_FROM(plague_types)], x, y, NO_MM_FLAGS, PLAGUE_TEMPLATE, -1);
			}
		}
	}
	/* KNIGHT QUEST: convert half the swamp to a forest on the knight locate level*/
	if (Role_if(PM_KNIGHT) &&
		In_quest(&u.uz) &&
		Is_qlocate(&u.uz)
		){
		int x, y;
		for (x = 0; x<COLNO / 2; x++){
			for (y = 0; y<ROWNO; y++){
				if (levl[x][y].typ == POOL) levl[x][y].typ = TREE;
			}
		}
	}
	/* GNOME RANGER QUEST: add ladder to quest*/
	if (Role_if(PM_RANGER) && Race_if(PM_GNOME) && on_level(&u.uz, &minetown_level)){
		int x, y, good = FALSE;
		while (!good){
			x = rn2(COLNO-4)+3;
			y = rn2(ROWNO-2)+1;
			if (isok(x, y) && levl[x][y].typ == ROOM && !costly_spot(x, y))
				good = TRUE;
			else continue;

			levl[x][y].typ = STAIRS;
			levl[x][y].ladder = LA_DOWN;
			sstairs.sx = x;
			sstairs.sy = y;
			sstairs.up = 0;
			assign_level(&sstairs.tolev, &qstart_level);
		}
	}
	/* WISHING REVAMP: Place candle of wishing on valley altar, if needed to make up for a 1-wish-ring */
	if(on_level(&valley_level, &u.uz)){
		if(u.ring_wishes == -1)
			u.ring_wishes = rnd(3);
		if(u.ring_wishes == 1){
			int x, y;
			for(x = 0; x<COLNO; x++){
				for(y = 0; y<ROWNO; y++){
					if(isok(x,y) && levl[x][y].typ == ALTAR){
						mksobj_at(CANDLE_OF_INVOCATION, x, y, MKOBJ_NOINIT);
					}
				}
			}
		}
	}

    if(lev_message) {
	char *str, *nl;
	for(str = lev_message; (nl = index(str, '\n')) != 0; str = nl+1) {
	    *nl = '\0';
	    pline("%s", str);
	}
	if(*str)
	    pline("%s", str);
	free((genericptr_t)lev_message);
	lev_message = 0;
    }

    if (lregions)
	free((genericptr_t) lregions),  lregions = 0;
    num_lregions = 0;
}

/* ROOMS IN MAZE FUNCTIONS
 * Many thanks to aosdict's xnethack, which was consulted (read: copied) in making this
 */

/* 
 * Returns true if the (x,y) location given falls withing the allowable bounds for mazes
 */
boolean
maze_inbounds(x, y)
xchar x, y;
{
	return (x >= 2 && y >= 2 && x < x_maze_max && y < y_maze_max && isok(x, y));
}

/* 
 * Returns true if the entire rectangle defined by its top-left and bottom-right corners
 * is overtop of a mazewalk-textured area.
 */
boolean
rectangle_in_mazewalk_area(lx, ly, hx, hy)
xchar lx, ly, hx, hy;
{
	int x, y;

	/* first check that both corners of the rectangle are inside maze boundaries */
	if (!maze_inbounds(lx, ly) || !maze_inbounds(hx, hy))
		return FALSE;

	for (x = lx; x <= hx; x++)
	for (y = ly; y <= hy; y++)
	{
		if (levl[x][y].typ != (((x % 2) && (y % 2)) ? STONE : HWALL))
			return FALSE;
	}
	return TRUE;
}

/* 
 * The sophisticated and classy cousin of rectangle_in_mazewalk_area().
 * Determines if a cutting out a rectangle would immediately separate sections of mazewalk from each other
 */
boolean
maze_rectangle_border_is_okay(lx, ly, hx, hy)
xchar lx, ly, hx, hy;
{
	int x = lx-1;	// we want to be on the [stone] part of the mazewalk fill
	int y = ly-1;
	int dx = 2;
	int dy = 0;
	boolean prev_okay = (maze_inbounds(x, y) && !levl[x][y].typ);
	int changes = 0;

	do{
		/* record if whether or not */
		if (prev_okay != (maze_inbounds(x, y) && !levl[x][y].typ)){
			changes++;
			prev_okay = (maze_inbounds(x, y) && !levl[x][y].typ);
		}
		/* move to next spot */
		x += dx;
		y += dy;

		if (dx && (x - 1 == hx || x + 1 == lx))
		{
			dy = dx;
			dx = 0;
		}
		else if (dy && (y - 1 == hy || y + 1 == ly))
		{
			dx = -dy;
			dy = 0;
		}
	} while ((x != lx - 1) || (y != ly - 1));

	if (changes > 2)
	{
		return FALSE;
	}
	else
		return TRUE;
}

/*
 * Sets the solidwall parameters for rand maze room walls for the given room
 */
void
maze_room_set_destructible_flags(r)
struct mkroom * r;
{
	xchar rlx, rly, rhx, rhy;	// rectangles to check for mazewalk-ness
	
	int i;
	int side = 0;

	for (i = 0; i < 4; i++)
	{
		side = 1 << i;
		rlx = r->lx;
		rly = r->ly;
		rhx = r->hx;
		rhy = r->hy;
		switch (side)
		{
		case W_NORTH: rly -= 2; rhy = rly;	break;
		case W_SOUTH: rhy += 2; rly = rhy;	break;
		case W_EAST:  rhx += 2; rlx = rhx;	break;
		case W_WEST:  rlx -= 2; rhx = rlx;	break;
		}
		if (!rectangle_in_mazewalk_area(rlx, rly, rhx, rhy))
		{
			r->solidwall |= side;
		}
	}
	return;
}

/*
 * Utility function to destroy a wall at the given spot.
 */
void
destroy_wall(x, y)
xchar x, y;
{
	/* don't destroy walls of the maze border */
	if (   !maze_inbounds(x + 1, y + 0)
		|| !maze_inbounds(x - 1, y - 0)
		|| !maze_inbounds(x + 0, y + 1)
		|| !maze_inbounds(x - 0, y - 1)
		)
		return;
	if (IS_WALL(levl[x][y].typ) || IS_DOOR(levl[x][y].typ) || IS_SDOOR(levl[x][y].typ)) {
		levl[x][y].typ = ROOM;
		levl[x][y].flags = 0; /* clear door mask */
	}
	return;
}

/*
 * Attempts to add rooms to the yet-unwalked mazewalk sections of a maze
 */
void
maze_add_rooms(attempts, maxsize)
int attempts;
int maxsize;
{
	xchar x, y;

	/* Ineligible maze levels for rooms */
	if (Invocation_lev(&u.uz))
		return;

	for (; attempts > 0; attempts--) {
		int roomidx;
		coord roompos;
		xchar lx, ly, hx, hy, width, height;
		int door_attempts = 3;
		int no_doors_made = 0;

		/* room must fit nicely in the maze; that is, its corner spaces should
		* all be valid maze0xy() locations. Thus, it should have odd
		* dimensions. */
		if (maxsize == -1)
			maxsize = rn2(4);
		width = 2 * rn2(maxsize+1) + 3;
		height = 2 * rn2(maxsize+1) + 3;

		/* Pick a corner location. Which directions the room points out from
		* this corner are randomized so that we don't have a bias towards any
		* edge of the map. */
		maze0xy(&roompos);
		if (rn2(2)) {
			/* original roompos is a bottom corner */
			roompos.y -= (height - 1);
		}
		if (rn2(2)) {
			/* original roompos is a right corner */
			roompos.x -= (width - 1);
		}
		/* these variables are the boundaries including walls */
		lx = roompos.x - 1;
		ly = roompos.y - 1;
		hx = roompos.x + width;
		hy = roompos.y + height;

		/* validate location */
		if (!rectangle_in_mazewalk_area(lx, ly, hx, hy) ||	/* room itself has to fit */
			(
			/* a slightly larger room must also fit, to guarantee we can connect it into the larger maze */
			!rectangle_in_mazewalk_area(lx - 2, ly, hx, hy) &&
			!rectangle_in_mazewalk_area(lx, ly - 2, hx, hy) &&
			!rectangle_in_mazewalk_area(lx, ly, hx + 2, hy) &&
			!rectangle_in_mazewalk_area(lx, ly, hx, hy + 2)
			))	{
			/* can't place, out of bounds */
			continue;
		}
		if (!maze_rectangle_border_is_okay(lx, ly, hx, hy))	{
			/* can't place, the room would probably cut the map in two */
			continue;
		}

		/* place the room into the the maze */
		if (!create_room(roompos.x, roompos.y, width, height,
			1, 1, OROOM, -1))
			continue;
		struct mkroom *r = &rooms[nroom - 1];

		/* determine which walls border mazewalk area (so they are permeable at levelgen) */
		maze_room_set_destructible_flags(r);


		/* put in some doors */
		/* more door attempts for large rooms */
		for (; door_attempts < ((hx - lx)+(hy - ly))/4; door_attempts++);
		/* try to add the doors */
		for (; door_attempts > 0; door_attempts--) {
			/* don't use finddpos - it'll bias doors towards the top left of
			* the room */
			xchar doorx, doory;
			boolean horiz = rn2(2) ? TRUE : FALSE;
			if (horiz) {
				doorx = rn2(hx - lx) / 2 * 2 + 1 + lx;
				doory = rn2(2) ? ly : hy;
			}
			else {
				doorx = rn2(2) ? hx : lx;
				doory = rn2(hy - ly) / 2 * 2 + 1 + ly;
			}
			/* Don't generate doors where the space outside of them is blocked.
			 * Shouldn't need okdoor() - with previous location generation, doors can't be next to
			 * each other and they should always be on walls
			 */
			if (   !maze_inbounds(doorx + !horiz, doory + horiz)	// cannot be on the maze edge
				|| !maze_inbounds(doorx - !horiz, doory - horiz)	// cannot be on the maze edge
				//|| IS_WALL(levl[doorx + !horiz][doory + horiz].typ)	// cannot be blocked by walls
				//|| IS_WALL(levl[doorx - !horiz][doory - horiz].typ)	// cannot be blocked by walls
				|| (r->solidwall & (((doory == ly)*W_NORTH) +
									((doory == hy)*W_SOUTH) +
									((doorx == lx)*W_WEST)  +
									((doorx == hx)*W_EAST)))		// cannot be on a solid wall
				)
			{
				if (door_attempts == 1) {
					door_attempts++; /* try again */
					no_doors_made++;
					if (no_doors_made == 200) {
						impossible("maze_add_rooms: can't place a door on rectangle (%d,%d ; %d,%d)", lx,ly,hx,hy);
						break;
					}
				}
				continue;
			}
			dodoor(doorx, doory, r);
		}
		/* set roomno so mazewalk and other maze generation ignore it */
		topologize(r);
	}
	return;
}

/*
* Attempts to add rooms to the yet-unwalked mazewalk sections of a maze
*/
void
maze_add_openings(attempts)
int attempts;
{
	xchar x, y;

	/* Ineligible maze levels for rooms */
	if (Invocation_lev(&u.uz))
		return;

	for (; attempts > 0; attempts--) {
		coord roompos;
		xchar lx, ly, hx, hy;
		xchar x, y;

		/* Pick the location. */
		maze0xy(&roompos);

		/* area around the center location */
		lx = roompos.x - 1;
		ly = roompos.y - 1;
		hx = roompos.x + 1;
		hy = roompos.y + 1;

		/* validate location */
		if (!rectangle_in_mazewalk_area(lx - 1, ly - 1, hx + 1, hy + 1))	{
			/* can't place, close to out of bounds or actually out of bounds */
			continue;
		}
		if (!maze_rectangle_border_is_okay(lx, ly, hx, hy))	{
			/* can't place, the space would probably cut the map in two */
			continue;
		}
		/* place the opening into the the maze */
		for (x = lx; x <= hx; x++)
		for (y = ly; y <= hy; y++)
		{
			if (!((x - roompos.x) && (y - roompos.y)))
				levl[x][y].typ = ROOM;
		}
	}
	return;
}

/*
 * Attempts to make the maze more livable by remove simple dead ends from the maze
 * If called with careful = TRUE, it will only remove dead ends into places it is certain are not part of sp_lev generation
 */
void
maze_remove_deadends(floortyp, careful)
int floortyp;
int careful;
{
	char dirok[4];
	int x, y, dir, idx, idx2, dx, dy, dx2, dy2;

	dirok[0] = 0; /* lint suppression */
	for (x = 2; x < x_maze_max; x++)
	for (y = 2; y < y_maze_max; y++)
	if ((levl[x][y].typ == floortyp) && (x % 2) && (y % 2)) {
		idx = idx2 = 0;
		for (dir = 0; dir < 4; dir++) {
			dx = dx2 = x;
			dy = dy2 = y;
			move(&dx, &dy, dir);
			if (!maze_inbounds(dx, dy)) {
				idx2++;
				continue;
			}
			move(&dx2, &dy2, dir);
			move(&dx2, &dy2, dir);
			if (!maze_inbounds(dx2, dy2)) {
				idx2++;
				continue;
			}
			if (IS_WALL(levl[dx][dy].typ)
				&& (levl[dx2][dy2].typ == floortyp)) {
				if (!careful)
				{
					dirok[idx++] = dir;
				}
				else
				{// will only make paths into rooms that were randomly placed prior to mazewalking
					if (levl[dx2][dy2].roomno - ROOMOFFSET >= level.flags.sp_lev_nroom)
						dirok[idx++] = dir;
				}
				idx2++;
				continue;
			}
			if (IS_WALL(levl[dx][dy].typ)) {
				idx2++;
				continue;
			}
		}
		if (idx2 >= 3 && idx > 0) {
			dx = dx2 = x;
			dy = dy2 = y;
			dir = dirok[rn2(idx)];
			move(&dx, &dy, dir);
			move(&dx2, &dy2, dir);
			move(&dx2, &dy2, dir);
			if (levl[dx2][dy2].roomno != NO_ROOM) {
				dodoor(dx, dy, &rooms[levl[dx2][dy2].roomno - ROOMOFFSET]);
			}
			else {
				levl[dx][dy].typ = floortyp;
			}
		}
	}
	return;
}

/*
* Maybe destroy the walls of the room
* walls can be destroyed if:
* - it is not a special room that wants full walls
* - the room is not lit
* chance/100 probability
*/
void
maze_damage_rooms(chance)
int chance;
{
	xchar x, y;
	xchar lx, ly, hx, hy;
	int wallsgone;		// bitfield
	int i;
	struct mkroom * r;

	for (r = &rooms[level.flags.sp_lev_nroom]; r != &rooms[nroom]; r++)
	{
		/* hx == -1 is sometimes set as a special flag, which then means we can't find the walls of this room to damage it */
		if (r->hx < 0)
			continue;

		if (!special_room_requires_full_walls(r->rtype) && !r->rlit && (rn2(100) < chance))
		{
			wallsgone = 0x15;

			lx = r->lx - 1;	// left wall
			ly = r->ly - 1;	// top wall
			hx = r->hx + 1;	// right wall
			hy = r->hy + 1;	// bottom wall

			/* large rooms are less likely to lose walls */
			for (i = (hx - lx) / 4; i > 0; i--)
				wallsgone &= (((rn2(16) | rn2(16)) & (W_NORTH | W_SOUTH)) | W_EAST | W_WEST);
			for (i = (hy - ly) / 4; i > 0; i--)
				wallsgone &= (((rn2(16) | rn2(16)) & (W_EAST | W_WEST)) | W_NORTH | W_SOUTH);

			/* "solid" walls that don't border maze filler cannot be destroyed in this manner */
			wallsgone &= ~r->solidwall;

			/* do not remove walls bording the edge of the maze */
			if (ly == 2)			wallsgone &= ~W_NORTH;
			if (hy == y_maze_max)	wallsgone &= ~W_SOUTH;
			if (hx == x_maze_max)	wallsgone &= ~W_EAST;
			if (lx == 2)			wallsgone &= ~W_WEST;

			/* actually destroy walls */
			for (x = lx + 1; x <= hx - 1; ++x) {
				if (wallsgone & W_NORTH) destroy_wall(x, ly);	// top
				if (wallsgone & W_SOUTH) destroy_wall(x, hy);	// bottom
			}
			for (y = ly + 1; y <= hy - 1; ++y) {
				if (wallsgone & W_EAST) destroy_wall(hx, y);	// right
				if (wallsgone & W_WEST) destroy_wall(lx, y);	// left
			}
			/* corners and middles are destroyed if the wall sections they connected were destroyed */
			if ((wallsgone & W_NORTH) && (wallsgone & W_WEST)) destroy_wall(lx, ly);
			if ((wallsgone & W_NORTH) && (wallsgone & W_EAST)) destroy_wall(hx, ly);
			if ((wallsgone & W_SOUTH) && (wallsgone & W_EAST)) destroy_wall(hx, hy);
			if ((wallsgone & W_SOUTH) && (wallsgone & W_WEST)) destroy_wall(lx, hy);

			/* re-draw walls*/
			if (wallsgone)
				wallification(lx, ly, hx, hy);

			/* note that r is not quite ordinary anymore */
			if (wallsgone && r->rtype == OROOM)
				r->rtype = JOINEDROOM;
		}
	}
}

/* Postprocessing after most of the rest of the level has been created
 * (including stairs), and some rooms exist.
 * Select attempts rooms to be converted into special rooms, then possibly
 * place some other furniture inside the room.
 */
void
maze_touchup_rooms(attempts)
int attempts;
{
	struct mkroom *r;
	if (nroom == 0)
		return;

	int i;
	for (; attempts > 0; attempts--) {
		if (wizard && nh_getenv("SHOPTYPE")) {
			/* full manual override */
			mkroom(SHOPBASE);
		}
		else {
			i = random_special_room();
			if (i)
				mkroom(i);
		}
	}
	for (i = level.flags.sp_lev_nroom; i < nroom; ++i) {
		/* set r */
		r = &rooms[i];

		/* add furniture to simple rooms */
		if (r->rtype == OROOM || r->rtype == JOINEDROOM)
		{
			/* probabilities here are deflated from makelevel() */
			if (!rn2(20))
				mkfeature(FOUNTAIN, FALSE, r);
			if (!rn2(60))
				mkfeature(FORGE, FALSE, r);
			if (!rn2(80))
				mkfeature(SINK, FALSE, r);
			if (!rn2(100))
				mkfeature(GRAVE, FALSE, r);
			if (!rn2(100))
				mkfeature(ALTAR, FALSE, r);
		}
	}
}

/*
 * Removes the given room, replacing its contents with pre-wallwalk fill
 */
void
maze_remove_room(room_index)
int room_index;
{
	int x, y;
	struct mkroom *troom = &rooms[room_index];

	for (x = troom->lx - 1; x <= troom->hx + 1; x++)
	for (y = troom->ly - 1; y <= troom->hy + 1; y++)
	{
		levl[x][y].typ = ((x % 2) && (y % 2)) ? STONE : HWALL;
		levl[x][y].flags = 0;
		levl[x][y].roomno = NO_ROOM;
	}
	remove_room(room_index);

	return;
}

void
create_maze()
{
	int x, y;
	coord mm;

	/* make maze base */
	for (x = 2; x <= x_maze_max; x++)
	for (y = 2; y <= y_maze_max; y++)
		levl[x][y].typ = ((x % 2) && (y % 2)) ? STONE : HWALL;

	/* add rooms */
	maze_add_rooms( 5,-1);
	maze_add_rooms(10, 3);
	maze_add_rooms( 5, 0);

	/* add maze openings */
	maze_add_openings(5);

	/* perform mazewalk */
	maze0xy(&mm);
	walkfrom((int)mm.x, (int)mm.y, 0);

	/* remove dead ends */
	maze_remove_deadends(ROOM, FALSE);

	/* put a boulder at the maze center */
	(void)mksobj_at(BOULDER, (int)mm.x, (int)mm.y, NO_MKOBJ_FLAGS);

	wallification(2, 2, x_maze_max, y_maze_max);

	return;
}

void
makemaz(s)
register const char *s;
{
	int x,y;
	char protofile[20];
	s_level	*sp = Is_special(&u.uz);
	coord mm;
	int levvar = 0;
	if(*s) {
	    if(sp && sp->rndlevs){
			levvar = rnd((int) sp->rndlevs);
			if(Is_village_level(&u.uz)) levvar = init_village((int) sp->rndlevs);
			/* special cases -- 
			 * chalev should always use the corresponding level
			 * hell/abyss floors are set at game start for oracle sneak peeks
			 * medusa & grue are still random as of right now, as is sea
			*/
			if (!strcmp(sp->proto, "chalev"))
				levvar = chaos_dvariant + 1;
			else if (Is_hell1(&u.uz)){
				if (dungeon_topology.hell1_variant == CHROMA_LEVEL) levvar = BAEL_LEVEL;
				else levvar = dungeon_topology.hell1_variant;
			}
			else if (Is_hell2(&u.uz))
				levvar = dungeon_topology.hell2_variant;
			else if (Is_abyss1(&u.uz))
				levvar = dungeon_topology.abyss_variant;
			else if (Is_abyss2(&u.uz))
				levvar = dungeon_topology.abys2_variant;
			else if (Is_abyss3(&u.uz))
				levvar = dungeon_topology.brine_variant;
						
			Sprintf(protofile, "%s-%d", s, levvar);
		}
	    else Strcpy(protofile, s);
	} else if(*(dungeons[u.uz.dnum].proto)) {
	    if(dunlevs_in_dungeon(&u.uz) > 1) {
			if(sp && sp->rndlevs){
				levvar = rnd((int) sp->rndlevs);
				Sprintf(protofile, "%s%d-%d", dungeons[u.uz.dnum].proto,
						 dunlev(&u.uz),
						 levvar);
			}else Sprintf(protofile, "%s%d", dungeons[u.uz.dnum].proto,
							dunlev(&u.uz));
	    } else if(sp && sp->rndlevs) {
			levvar = rnd((int) sp->rndlevs);
		    Sprintf(protofile, "%s-%d", dungeons[u.uz.dnum].proto,
						levvar);
	    } else Strcpy(protofile, dungeons[u.uz.dnum].proto);

	} else Strcpy(protofile, "");

//	pline("%s", protofile);
//	pline("%d", levvar);
	if (Is_challenge_level(&u.uz)){
		dungeon_topology.challenge_variant = levvar;
	} else if(In_sea(&u.uz)){
		dungeon_topology.sea_variant = levvar;
	} else if(Is_village_level(&u.uz)){
		dungeon_topology.village_variant = levvar;
	}
	
	if(dungeon_topology.alt_tower){
		if(Is_arcadiatower2(&u.uz)){
			Strcpy(protofile, "btower2");
		} else if(Is_arcadiatower3(&u.uz)){
			Strcpy(protofile, "btower3");
		} else if(Is_arcadiadonjon(&u.uz)){
			Strcpy(protofile, "towrtob");
		}
	}
	
	if(Is_hell1(&u.uz) && dungeon_topology.hell1_variant == CHROMA_LEVEL){
			Strcpy(protofile, "hell-a");
	}
	/* quick hack for Binders entering Astral -- change the gods out before loading the level, so that
	 * the altars are all generated to the correct gods */
	if (Role_if(PM_EXILE) && Is_astralevel(&u.uz)) {
		/* the Deities on Astral are those that stand at the Gate, not the creational ones governing the Dungeon */
		urole.lgod = GOD_PISTIS_SOPHIA;
		urole.ngod = GOD_THE_VOID;
		urole.cgod = GOD_YALDABAOTH;
	}
	/* similarly, swap out the regular pantheon god for your aligned god at this point */
	if (u.ugodbase[UGOD_CURRENT] != urole.lgod &&
		u.ugodbase[UGOD_CURRENT] != urole.ngod &&
		u.ugodbase[UGOD_CURRENT] != urole.cgod ) {
		switch(galign(u.ugodbase[UGOD_CURRENT])) {
			case A_LAWFUL:  urole.lgod = u.ugodbase[UGOD_CURRENT]; break;
			case A_NEUTRAL: urole.ngod = u.ugodbase[UGOD_CURRENT]; break;
			case A_CHAOTIC: urole.cgod = u.ugodbase[UGOD_CURRENT]; break;
		}
	}

#ifdef WIZARD
	/* SPLEVTYPE format is "level-choice,level-choice"... */
	if (wizard && *protofile && sp && sp->rndlevs) {
	    char *ep = getenv("SPLEVTYPE");	/* not nh_getenv */
	    if (ep) {
		/* rindex always succeeds due to code in prior block */
		int len = (rindex(protofile, '-') - protofile) + 1;

		while (ep && *ep) {
		    if (!strncmp(ep, protofile, len)) {
			int pick = atoi(ep + len);
			/* use choice only if valid */
			if (pick > 0 && pick <= (int) sp->rndlevs)
			    Sprintf(protofile + len, "%d", pick);
			break;
		    } else {
			ep = index(ep, ',');
			if (ep) ++ep;
		    }
		}
	    }
	}
#endif

	if(*protofile) {
	    Strcat(protofile, LEV_EXT);
	    if(load_special(protofile)) {
		fixup_special();
		/* some levels can end up with monsters
		   on dead mon list, including light source monsters */
		dmonsfree();
		return;	/* no mazification right now */
	    }
	    impossible("Couldn't load \"%s\" - making a maze.", protofile);
	}

	level.flags.is_maze_lev = TRUE;

	create_maze();

	mazexy(&mm);
	mkstairs(mm.x, mm.y, 1, room_at(mm.x,mm.y));		/* up */

	if (!Invocation_lev(&u.uz)) {
	    mazexy(&mm);
	    mkstairs(mm.x, mm.y, 0, room_at(mm.x,mm.y));	/* down */
	} else {	/* choose "vibrating square" location */
#define x_maze_min 2
#define y_maze_min 2
	    /*
	     * Pick a position where the stairs down to Moloch's Sanctum
	     * level will ultimately be created.  At that time, an area
	     * will be altered:  walls removed, moat and traps generated,
	     * boulders destroyed.  The position picked here must ensure
	     * that that invocation area won't extend off the map.
	     *
	     * We actually allow up to 2 squares around the usual edge of
	     * the area to get truncated; see mkinvokearea(mklev.c).
	     */
#define INVPOS_X_MARGIN (6 - 2)
#define INVPOS_Y_MARGIN (5 - 2)
#define INVPOS_DISTANCE 11
	    int x_range = x_maze_max - x_maze_min - 2*INVPOS_X_MARGIN - 1,
		y_range = y_maze_max - y_maze_min - 2*INVPOS_Y_MARGIN - 1;

#ifdef DEBUG
	    if (x_range <= INVPOS_X_MARGIN || y_range <= INVPOS_Y_MARGIN ||
		   (x_range * y_range) <= (INVPOS_DISTANCE * INVPOS_DISTANCE))
		panic("inv_pos: maze is too small! (%d x %d)",
		      x_maze_max, y_maze_max);
#endif
	    inv_pos.x = inv_pos.y = 0; /*{occupied() => invocation_pos()}*/
	    do {
		x = rn1(x_range, x_maze_min + INVPOS_X_MARGIN + 1);
		y = rn1(y_range, y_maze_min + INVPOS_Y_MARGIN + 1);
		/* we don't want it to be too near the stairs, nor
		   to be on a spot that's already in use (wall|trap) */
	    } while (x == xupstair || y == yupstair ||	/*(direct line)*/
		     abs(x - xupstair) == abs(y - yupstair) ||
		     distmin(x, y, xupstair, yupstair) <= INVPOS_DISTANCE ||
		     !SPACE_POS(levl[x][y].typ) || occupied(x, y));
	    inv_pos.x = x;
	    inv_pos.y = y;
#undef INVPOS_X_MARGIN
#undef INVPOS_Y_MARGIN
#undef INVPOS_DISTANCE
#undef x_maze_min
#undef y_maze_min
	}

	/* place branch stair or portal */
	place_branch(Is_branchlev(&u.uz), 0, 0);

	/* add special rooms, dungeon features */
	if (!In_quest(&u.uz))
		maze_touchup_rooms(rnd(3));
	maze_damage_rooms(85);

	for(x = rn1(8,11); x; x--) {
		mazexy(&mm);
		(void) mkobj_at(rn2(2) ? GEM_CLASS : 0, mm.x, mm.y, MKOBJ_ARTIF);
	}
	for(x = rn1(10,2); x; x--) {
		mazexy(&mm);
		(void) mksobj_at(BOULDER, mm.x, mm.y, NO_MKOBJ_FLAGS);
	}
	for (x = rn2(3); x; x--) {
		mazexy(&mm);
		(void) makemon(&mons[PM_MINOTAUR], mm.x, mm.y, NO_MM_FLAGS);
	}
	for(x = rn1(5,7); x; x--) {
		mazexy(&mm);
		(void) makemon((struct permonst *) 0, mm.x, mm.y, NO_MM_FLAGS);
	}
	for(x = rn1(6,7); x; x--) {
		mazexy(&mm);
		(void) mkgold(0L,mm.x,mm.y);
	}
	for(x = rn1(6,7); x; x--)
		mktrap(0,1,(struct mkroom *) 0, (coord*) 0);
}

#ifdef MICRO
/* Make the mazewalk iterative by faking a stack.  This is needed to
 * ensure the mazewalk is successful in the limited stack space of
 * the program.  This iterative version uses the minimum amount of stack
 * that is totally safe.
 */
void
walkfrom(x,y,depth)
int x,y,depth;
{
#define CELLS (ROWNO * COLNO) / 4		/* a maze cell is 4 squares */
	char mazex[CELLS + 1], mazey[CELLS + 1];	/* char's are OK */
	int q, a, dir, pos;
	int dirs[4];

	pos = 1;
	mazex[pos] = (char) x;
	mazey[pos] = (char) y;
	while (pos) {
		x = (int) mazex[pos];
		y = (int) mazey[pos];
		if(!IS_DOOR(levl[x][y].typ)) {
		    /* might still be on edge of MAP, so don't overwrite */
#ifndef WALLIFIED_MAZE
		    levl[x][y].typ = CORR;
#else
		    levl[x][y].typ = ROOM;
#endif
		    levl[x][y].flags = 0;
		}
		q = 0;
		for (a = 0; a < 4; a++)
			if(okay(x, y, a, depth)) dirs[q++]= a;
		if (!q)
			pos--;
		else {
			dir = dirs[rn2(q)];
			move(&x, &y, dir);
#ifndef WALLIFIED_MAZE
			levl[x][y].typ = CORR;
#else
			levl[x][y].typ = ROOM;
#endif
			move(&x, &y, dir);
			if (levl[x][y].roomno - ROOMOFFSET >= level.flags.sp_lev_nroom)
				maze_remove_room(levl[x][y].roomno - ROOMOFFSET);
			pos++;
			if (pos > CELLS)
				panic("Overflow in walkfrom");
			mazex[pos] = (char) x;
			mazey[pos] = (char) y;
		}
	}
}
#else

void
walkfrom(x,y,depth)
int x,y,depth;
{
	register int q,a,dir;
	int dirs[4];

	if(!IS_DOOR(levl[x][y].typ)) {
	    /* might still be on edge of MAP, so don't overwrite */
#ifndef WALLIFIED_MAZE
	    levl[x][y].typ = CORR;
#else
	    levl[x][y].typ = ROOM;
#endif
	    levl[x][y].flags = 0;
	}

	while(1) {
		q = 0;
		for(a = 0; a < 4; a++)
			if(okay(x,y,a,depth))
				dirs[q++]= a;
		if(!q) return;
		dir = dirs[rn2(q)];
		move(&x,&y,dir);
		if (levl[x][y].roomno - ROOMOFFSET >= level.flags.sp_lev_nroom)
			maze_remove_room(levl[x][y].roomno - ROOMOFFSET);
#ifndef WALLIFIED_MAZE
		levl[x][y].typ = CORR;
#else
		levl[x][y].typ = ROOM;
#endif
		move(&x,&y,dir);
		walkfrom(x,y,depth+1);
		/* move x and y back to our original spot */
		move(&x, &y, (dir + 2) % 4);
		move(&x, &y, (dir + 2) % 4);
	}
}
#endif /* MICRO */

STATIC_OVL void
move(x,y,dir)
register int *x, *y;
register int dir;
{
	switch(dir){
		case 0: --(*y); break;
		case 1: (*x)++; break;
		case 2: (*y)++; break;
		case 3: --(*x); break;
		default: panic("move: bad direction");
	}
}

void
mazexy(cc)	/* find random point in generated corridors,
		   so we don't create items in moats, bunkers, or walls */
	coord	*cc;
{
	int cpt=0;

	do {
	    cc->x = 3 + 2*rn2((x_maze_max>>1) - 1);
	    cc->y = 3 + 2*rn2((y_maze_max>>1) - 1);
	    cpt++;
	} while (cpt < 100 && levl[cc->x][cc->y].typ !=
#ifdef WALLIFIED_MAZE
		 ROOM
#else
		 CORR
#endif
		);
	if (cpt >= 100) {
		register int x, y;
		/* last try */
		for (x = 0; x < (x_maze_max>>1) - 1; x++)
		    for (y = 0; y < (y_maze_max>>1) - 1; y++) {
			cc->x = 3 + 2 * x;
			cc->y = 3 + 2 * y;
			if (levl[cc->x][cc->y].typ ==
#ifdef WALLIFIED_MAZE
			    ROOM
#else
			    CORR
#endif
			|| levl[cc->x][cc->y].typ == SAND
		 	|| levl[cc->x][cc->y].typ == SOIL
			   ) return;
		    }
		panic("mazexy: can't find a place!");
	}
	return;
}

void
bound_digging()
/* put a non-diggable boundary around the initial portion of a level map.
 * assumes that no level will initially put things beyond the isok() range.
 *
 * we can't bound unconditionally on the last line with something in it,
 * because that something might be a niche which was already reachable,
 * so the boundary would be breached
 *
 * we can't bound unconditionally on one beyond the last line, because
 * that provides a window of abuse for WALLIFIED_MAZE special levels
 */
{
	register int x,y;
	register unsigned typ;
	register struct rm *lev;
	boolean found, nonwall;
	int xmin,xmax,ymin,ymax;

	if(Is_earthlevel(&u.uz)) return; /* everything diggable here */

	found = nonwall = FALSE;
	for(xmin=0; !found; xmin++) {
		lev = &levl[xmin][0];
		for(y=0; y<=ROWNO-1; y++, lev++) {
			typ = lev->typ;
			if(typ != STONE) {
				found = TRUE;
				if(!IS_WALL(typ)) nonwall = TRUE;
			}
		}
	}
	xmin -= (nonwall || !level.flags.is_maze_lev) ? 2 : 1;
	if (xmin < 0) xmin = 0;

	found = nonwall = FALSE;
	for(xmax=COLNO-1; !found; xmax--) {
		lev = &levl[xmax][0];
		for(y=0; y<=ROWNO-1; y++, lev++) {
			typ = lev->typ;
			if(typ != STONE) {
				found = TRUE;
				if(!IS_WALL(typ)) nonwall = TRUE;
			}
		}
	}
	xmax += (nonwall || !level.flags.is_maze_lev) ? 2 : 1;
	if (xmax >= COLNO) xmax = COLNO-1;

	found = nonwall = FALSE;
	for(ymin=0; !found; ymin++) {
		lev = &levl[xmin][ymin];
		for(x=xmin; x<=xmax; x++, lev += ROWNO) {
			typ = lev->typ;
			if(typ != STONE) {
				found = TRUE;
				if(!IS_WALL(typ)) nonwall = TRUE;
			}
		}
	}
	ymin -= (nonwall || !level.flags.is_maze_lev) ? 2 : 1;

	found = nonwall = FALSE;
	for(ymax=ROWNO-1; !found; ymax--) {
		lev = &levl[xmin][ymax];
		for(x=xmin; x<=xmax; x++, lev += ROWNO) {
			typ = lev->typ;
			if(typ != STONE) {
				found = TRUE;
				if(!IS_WALL(typ)) nonwall = TRUE;
			}
		}
	}
	ymax += (nonwall || !level.flags.is_maze_lev) ? 2 : 1;

	for (x = 0; x < COLNO; x++)
	  for (y = 0; y < ROWNO; y++)
	    if (y <= ymin || y >= ymax || x <= xmin || x >= xmax) {
#ifdef DCC30_BUG
		lev = &levl[x][y];
		lev->wall_info |= W_NONDIGGABLE;
#else
		levl[x][y].wall_info |= W_NONDIGGABLE;
#endif
	    }
}

struct trap *
mkportal(x, y, todnum, todlevel)
register xchar x, y;
register int todnum, todlevel;
{
	/* a portal "trap" must be matched by a */
	/* portal in the destination dungeon/dlevel */
	struct trap *ttmp = maketrap(x, y, MAGIC_PORTAL);

	if (!ttmp) {
		impossible("portal on top of portal??");
		return (struct trap*) 0;
	}
#ifdef DEBUG
	pline("mkportal: at (%d,%d), to %s, level %d",
		x, y, dungeons[todnum].dname, todlevel);
#endif
	ttmp->dst.dnum = todnum;
	ttmp->dst.dlevel = todlevel;
	return ttmp;
}

/*
 * Special waterlevel stuff in endgame (TH).
 *
 * Some of these functions would probably logically belong to some
 * other source files, but they are all so nicely encapsulated here.
 */

/* to ease the work of debuggers at this stage */
#define register

#define CONS_OBJ   0
#define CONS_MON   1
#define CONS_HERO  2
#define CONS_TRAP  3

static struct bubble *bbubbles, *ebubbles;

static struct trap *wportal;
static int xmin, ymin, xmax, ymax;	/* level boundaries */
/* bubble movement boundaries */
#define bxmin (xmin + 1)
#define bymin (ymin + 1)
#define bxmax (xmax - 1)
#define bymax (ymax - 1)

STATIC_DCL void NDECL(set_wportal);
STATIC_DCL void FDECL(mk_bubble, (int,int,int));
STATIC_DCL void FDECL(mv_bubble, (struct bubble *,int,int,BOOLEAN_P));

void
movebubbles()
{
	static boolean up;
    struct bubble *b;
    int x, y, i, j;
	struct trap *btrap;
	static const struct rm water_pos =
		{ cmap_to_glyph(S_water), WATER, 0, 0, 0, 0, 0, 0, 0 };

	/* set up the portal the first time bubbles are moved */
    if (!wportal)
        set_wportal();

	vision_recalc(2);
	/* keep attached ball&chain separate from bubble objects */
    if (Punished)
        unplacebc();

	/*
	 * Pick up everything inside of a bubble then fill all bubble
	 * locations.
	 */
	for (b = up ? bbubbles : ebubbles; b; b = up ? b->next : b->prev) {
        if (b->cons)
	        panic("movebubbles: cons != null");
	    for (i = 0, x = b->x; i < (int) b->bm[0]; i++, x++)
		for (j = 0, y = b->y; j < (int) b->bm[1]; j++, y++)
		    if (b->bm[j + 2] & (1 << i)) {
			if (!isok(x,y)) {
			    impossible("movebubbles: bad pos (%d,%d)", x,y);
			    continue;
			}

			/* pick up objects, monsters, hero, and traps */
			if (OBJ_AT(x,y)) {
			    struct obj *olist = (struct obj *) 0, *otmp;
                struct container *cons =
                    (struct container *) alloc(
                        sizeof(struct container));

			    while ((otmp = level.objects[x][y]) != 0) {
				remove_object(otmp);
				otmp->ox = otmp->oy = 0;
				otmp->nexthere = olist;
				olist = otmp;
			    }

			    cons->x = x;
			    cons->y = y;
			    cons->what = CONS_OBJ;
			    cons->list = (genericptr_t) olist;
			    cons->next = b->cons;
			    b->cons = cons;
			}
			if (MON_AT(x,y)) {
			    struct monst *mon = m_at(x,y);
                struct container *cons =
                    (struct container *) alloc(
                        sizeof(struct container));

			    cons->x = x;
			    cons->y = y;
			    cons->what = CONS_MON;
			    cons->list = (genericptr_t) mon;

			    cons->next = b->cons;
			    b->cons = cons;

			    if(mon->wormno)
				remove_worm(mon);
			    else
				remove_monster(x, y);

			    newsym(x,y);	/* clean up old position */
			    mon->mx = mon->my = 0;
			}
			if (!u.uswallow && x == u.ux && y == u.uy) {
                struct container *cons =
                    (struct container *) alloc(
                        sizeof(struct container));

			    cons->x = x;
			    cons->y = y;
			    cons->what = CONS_HERO;
			    cons->list = (genericptr_t) 0;

			    cons->next = b->cons;
			    b->cons = cons;
			}
			if ((btrap = t_at(x,y)) != 0) {
                struct container *cons =
                    (struct container *) alloc(
                        sizeof(struct container));

			    cons->x = x;
			    cons->y = y;
			    cons->what = CONS_TRAP;
			    cons->list = (genericptr_t) btrap;

			    cons->next = b->cons;
			    b->cons = cons;
			}

			levl[x][y] = water_pos;
			// block_point(x,y);
		    }
	}

	/*
	 * Every second time traverse down.  This is because otherwise
	 * all the junk that changes owners when bubbles overlap
	 * would eventually end up in the last bubble in the chain.
	 */
	up = !up;
	for (b = up ? bbubbles : ebubbles; b; b = up ? b->next : b->prev) {
        int rx = rn2(3), ry = rn2(3);

		mv_bubble(b,b->dx + 1 - (!b->dx ? rx : (rx ? 1 : 0)),
                  b->dy + 1 - (!b->dy ? ry : (ry ? 1 : 0)), FALSE);
	}

	/* put attached ball&chain back */
	if (Punished) placebc();
	vision_full_recalc = 1;
}

/* when moving in water, possibly (1 in 3) alter the intended destination */
void
water_friction()
{
    int x, y, dx, dy;
    boolean eff = FALSE;

	if (Swimming && rn2(4))
		return;		/* natural swimmers have advantage */

    if (uarmf && uarmf->otyp == SHOES) return; /* iron boots let you walk on the seafloor (Zelda) */

	if (u.dx && !rn2(!u.dy ? 3 : 6)) {	/* 1/3 chance or half that */
		/* cancel delta x and choose an arbitrary delta y value */
		x = u.ux;
		do {
		    dy = rn2(3) - 1;		/* -1, 0, 1 */
		    y = u.uy + dy;
		} while (dy && (!isok(x,y) || !is_pool(x,y, TRUE)));//can be tossed to shore
		u.dx = 0;
		u.dy = dy;
		eff = TRUE;
	} else if (u.dy && !rn2(!u.dx ? 3 : 5)) {	/* 1/3 or 1/5*(5/6) */
		/* cancel delta y and choose an arbitrary delta x value */
		y = u.uy;
		do {
		    dx = rn2(3) - 1;		/* -1 .. 1 */
		    x = u.ux + dx;
		} while (dx && (!isok(x,y) || !is_pool(x,y, TRUE)));
		u.dy = 0;
		u.dx = dx;
		eff = TRUE;
	}
	if (eff) pline("Water turbulence affects your movements.");
}

void
save_waterlevel(fd, mode)
int fd, mode;
{
    struct bubble *b;

	if (!Is_waterlevel(&u.uz)) return;

	if (perform_bwrite(mode)) {
	    int n = 0;
	    for (b = bbubbles; b; b = b->next) ++n;
	    bwrite(fd, (genericptr_t)&n, sizeof (int));
	    bwrite(fd, (genericptr_t)&xmin, sizeof (int));
	    bwrite(fd, (genericptr_t)&ymin, sizeof (int));
	    bwrite(fd, (genericptr_t)&xmax, sizeof (int));
	    bwrite(fd, (genericptr_t)&ymax, sizeof (int));
	    for (b = bbubbles; b; b = b->next)
		bwrite(fd, (genericptr_t)b, sizeof (struct bubble));
	}
	if (release_data(mode))
	    unsetup_waterlevel();
}

void
restore_waterlevel(fd)
int fd;
{
    struct bubble *b = (struct bubble *) 0, *btmp;
    int i, n;

	if (!Is_waterlevel(&u.uz)) return;

	set_wportal();
	mread(fd,(genericptr_t)&n,sizeof(int));
	mread(fd,(genericptr_t)&xmin,sizeof(int));
	mread(fd,(genericptr_t)&ymin,sizeof(int));
	mread(fd,(genericptr_t)&xmax,sizeof(int));
	mread(fd,(genericptr_t)&ymax,sizeof(int));
	// pline("bubble size: %d",n);
	if(n > 0){
		for (i = 0; i < n; i++) {
			btmp = b;
			b = (struct bubble *)alloc(sizeof(struct bubble));
			mread(fd,(genericptr_t)b,sizeof(struct bubble));
			if (bbubbles) {
				btmp->next = b;
				b->prev = btmp;
			} else {
				bbubbles = b;
				b->prev = (struct bubble *)0;
			}
			mv_bubble(b,0,0,TRUE);
		}
		ebubbles = b;
		b->next = (struct bubble *)0;
		was_waterlevel = TRUE;
	} else {
		setup_waterlevel();
		set_wportal();
		was_waterlevel = TRUE;
	}
	return;
}

const char *waterbody_name(x, y)
xchar x,y;
{
	register struct rm *lev;
	schar ltyp;

	if (!isok(x,y))
		return "drink";		/* should never happen */
	lev = &levl[x][y];
	ltyp = lev->typ;

	if (is_lava(x,y))
		return "lava";
	else if (ltyp == ICE ||
		 (ltyp == DRAWBRIDGE_UP &&
		  (levl[x][y].drawbridgemask & DB_UNDER) == DB_ICE))
		return "ice";
	else if (((ltyp != POOL) && (ltyp != WATER) && (ltyp != PUDDLE) &&
	  !Is_medusa_level(&u.uz) && !Is_waterlevel(&u.uz) && !Is_juiblex_level(&u.uz)) ||
	   (ltyp == DRAWBRIDGE_UP && (levl[x][y].drawbridgemask & DB_UNDER) == DB_MOAT))
		return "moat";
	else if ((ltyp != POOL) && (ltyp != WATER) && (ltyp != PUDDLE) && Is_juiblex_level(&u.uz))
		return "swamp";
	else if (ltyp == POOL)
		return "pool of water";
	else return "water";
}

STATIC_OVL void
set_wportal()
{
	/* there better be only one magic portal on water level... */
	for (wportal = ftrap; wportal; wportal = wportal->ntrap)
		if (wportal->ttyp == MAGIC_PORTAL) return;
	impossible("set_wportal(): no portal!");
}

STATIC_OVL void
setup_waterlevel()
{
	register int x, y;
	register int xskip, yskip;
	register int water_glyph = cmap_to_glyph(S_water);

	/* ouch, hardcoded... */

	xmin = 3;
	ymin = 1;
	xmax = 78;
	ymax = 20;

	/* set hero's memory to water */

	for (x = xmin; x <= xmax; x++)
		for (y = ymin; y <= ymax; y++)
			levl[x][y].glyph = water_glyph;

	/* make bubbles */

	xskip = 10 + rn2(10);
	yskip = 4 + rn2(4);
	for (x = bxmin; x <= bxmax; x += xskip)
		for (y = bymin; y <= bymax; y += yskip)
			mk_bubble(x,y,rn2(7));
}

STATIC_OVL void
unsetup_waterlevel()
{
    struct bubble *b, *bb;

	/* free bubbles */

	for (b = bbubbles; b; b = bb) {
		bb = b->next;
		free((genericptr_t)b);
	}
	bbubbles = ebubbles = (struct bubble *)0;
}

STATIC_OVL void
mk_bubble(x,y,n)
int x, y, n;
{
	/*
	 * These bit masks make visually pleasing bubbles on a normal aspect
	 * 25x80 terminal, which naturally results in them being mathematically
	 * anything but symmetric.  For this reason they cannot be computed
	 * in situ, either.  The first two elements tell the dimensions of
	 * the bubble's bounding box.
	 */
    static uchar bm2[] = { 2, 1, 0x3 },
		bm3[] = {3,2,0x7,0x7},
		bm4[] = {4,3,0x6,0xf,0x6},
		bm5[] = {5,3,0xe,0x1f,0xe},
		bm6[] = {6,4,0x1e,0x3f,0x3f,0x1e},
		bm7[] = {7,4,0x3e,0x7f,0x7f,0x3e},
		bm8[] = {8,4,0x7e,0xff,0xff,0x7e},
		*bmask[] = {bm2,bm3,bm4,bm5,bm6,bm7,bm8};
	struct bubble *b;

    if (x >= bxmax || y >= bymax)
        return;
	if (n >= SIZE(bmask)) {
		impossible("n too large (mk_bubble)");
		n = SIZE(bmask) - 1;
	}
    if (bmask[n][1] > MAX_BMASK) {
        panic("bmask size is larger than MAX_BMASK");
    }
	b = (struct bubble *)alloc(sizeof(struct bubble));
    if ((x + (int) bmask[n][0] - 1) > bxmax)
        x = bxmax - bmask[n][0] + 1;
    if ((y + (int) bmask[n][1] - 1) > bymax)
        y = bymax - bmask[n][1] + 1;
	b->x = x;
	b->y = y;
	b->dx = 1 - rn2(3);
	b->dy = 1 - rn2(3);
    /* y dimension is the length of bitmap data - see bmask above */
    (void) memcpy((genericptr_t) b->bm, (genericptr_t) bmask[n],
                  (bmask[n][1] + 2) * sizeof(b->bm[0]));
	b->cons = 0;
    if (!bbubbles)
        bbubbles = b;
	if (ebubbles) {
		ebubbles->next = b;
		b->prev = ebubbles;
    } else
		b->prev = (struct bubble *)0;
	b->next =  (struct bubble *)0;
	ebubbles = b;
	mv_bubble(b,0,0,TRUE);
}

/*
 * The player, the portal and all other objects and monsters
 * float along with their associated bubbles.  Bubbles may overlap
 * freely, and the contents may get associated with other bubbles in
 * the process.  Bubbles are "sticky", meaning that if the player is
 * in the immediate neighborhood of one, he/she may get sucked inside.
 * This property also makes leaving a bubble slightly difficult.
 */
STATIC_OVL void
mv_bubble(b,dx,dy,ini)
struct bubble *b;
int dx, dy;
boolean ini;
{
    int x, y, i, j, colli = 0;
	struct container *cons, *ctemp;

	/* move bubble */
	if (dx < -1 || dx > 1 || dy < -1 || dy > 1) {
	    /* pline("mv_bubble: dx = %d, dy = %d", dx, dy); */
	    dx = sgn(dx);
	    dy = sgn(dy);
	}

	/*
	 * collision with level borders?
	 *	1 = horizontal border, 2 = vertical, 3 = corner
	 */
    if (b->x <= bxmin)
        colli |= 2;
    if (b->y <= bymin)
        colli |= 1;
    if ((int) (b->x + b->bm[0] - 1) >= bxmax)
        colli |= 2;
    if ((int) (b->y + b->bm[1] - 1) >= bymax)
        colli |= 1;

	if (b->x < bxmin) {
	    pline("bubble xmin: x = %d, xmin = %d", b->x, bxmin);
	    b->x = bxmin;
	}
	if (b->y < bymin) {
	    pline("bubble ymin: y = %d, ymin = %d", b->y, bymin);
	    b->y = bymin;
	}
	if ((int) (b->x + b->bm[0] - 1) > bxmax) {
        pline("bubble xmax: x = %d, xmax = %d", b->x + b->bm[0] - 1,
              bxmax);
	    b->x = bxmax - b->bm[0] + 1;
	}
	if ((int) (b->y + b->bm[1] - 1) > bymax) {
        pline("bubble ymax: y = %d, ymax = %d", b->y + b->bm[1] - 1,
              bymax);
	    b->y = bymax - b->bm[1] + 1;
	}

	/* bounce if we're trying to move off the border */
    if (b->x == bxmin && dx < 0)
        dx = -dx;
    if (b->x + b->bm[0] - 1 == bxmax && dx > 0)
        dx = -dx;
    if (b->y == bymin && dy < 0)
        dy = -dy;
    if (b->y + b->bm[1] - 1 == bymax && dy > 0)
        dy = -dy;

	b->x += dx;
	b->y += dy;

	/* void positions inside bubble */

	for (i = 0, x = b->x; i < (int) b->bm[0]; i++, x++)
	    for (j = 0, y = b->y; j < (int) b->bm[1]; j++, y++)
		if (b->bm[j + 2] & (1 << i)) {
		    levl[x][y].typ = MOAT;//was ROOM;// was AIR
		    levl[x][y].lit = 1;
		    // unblock_point(x,y);
		}

	/* replace contents of bubble */
	for (cons = b->cons; cons; cons = ctemp) {
	    ctemp = cons->next;
	    cons->x += dx;
	    cons->y += dy;

	    switch(cons->what) {
		case CONS_OBJ: {
		    struct obj *olist, *otmp;

		    for (olist=(struct obj *)cons->list; olist; olist=otmp) {
			otmp = olist->nexthere;
			place_object(olist, cons->x, cons->y);
		    }
		    break;
		}

		case CONS_MON: {
		    struct monst *mon = (struct monst *) cons->list;
		    (void) mnearto(mon, cons->x, cons->y, TRUE);
		    break;
		}

		case CONS_HERO: {
		    int ux0 = u.ux, uy0 = u.uy;

		    /* change u.ux0 and u.uy0? */
			u_on_newpos(cons->x, cons->y);
		    newsym(ux0, uy0);	/* clean up old position */

		    if (MON_AT(cons->x, cons->y)) {
				mnexto(m_at(cons->x,cons->y));
			}
		    break;
		}

		case CONS_TRAP: {
		    struct trap *btrap = (struct trap *) cons->list;
		    btrap->tx = cons->x;
		    btrap->ty = cons->y;
		    break;
		}

		default:
		    impossible("mv_bubble: unknown bubble contents");
		    break;
	    }
	    free((genericptr_t)cons);
	}
	b->cons = 0;

	/* boing? */
	switch (colli) {
	    case 1:
	        b->dy = -b->dy;
	        break;
	    case 3:
	        b->dy = -b->dy; /* fall through */
	    case 2:
	        b->dx = -b->dx;
	        break;
	    default:
		/* sometimes alter direction for fun anyway
		   (higher probability for stationary bubbles) */
		if (!ini && ((b->dx || b->dy) ? !rn2(20) : !rn2(5))) {
			b->dx = 1 - rn2(3);
			b->dy = 1 - rn2(3);
		}
	}
}

STATIC_DCL void
fill_dungeon_of_ill_regard(){
	int corrs = 0, all = 0, med = 0, strong = 0;
	int i = 0, j = 0;
	int x = 0, y = 0;
	int *skips;
	struct monst *mon;
	struct trap *trap;
	for (x = 0; x<COLNO; x++){
		for (y = 0; y<ROWNO; y++){
			if (isok(x,y) && levl[x][y].typ == CORR) corrs++;
		}
	}
	for(i = 0; i < PM_LONG_WORM_TAIL ; i++){
		if(mons[i].geno&(G_NOGEN|G_UNIQ)
			|| mvitals[i].mvflags&G_GONE
			|| mons[i].mlet == S_PLANT
			|| G_C_INST(mons[i].geno) != 0
		)
			continue;
		if(mons[i].maligntyp < -10)
			strong++;
		if(mons[i].maligntyp < -5)
			med++;
		if(mons[i].maligntyp < 0){
			all++;
			// pline("%s", mons[i].mname);
		}
	}
	// pline("Cells: %d, All: %d, Medium: %d, Strong: %d", corrs, all, med-strong, strong);
	skips = (int *)malloc(sizeof(int)*all);
	for(i = 0; i<all; i++){
		if(i < corrs)
			skips[i] = 0;
		else skips[i] = 1;
	}
	for(i = 0; i<all; i++){
		j = rn2(all); //swap pos i with pos j
		x = skips[i]; //keep old value of pos i
		skips[i] = skips[j];
		skips[j] = x;
	}
#define LOOP_BODY	\
			if(isok(x,y) && levl[x][y].typ == CORR){\
				while(i < PM_LONG_WORM_TAIL \
				&& (mons[i].maligntyp >= 0 || (mons[i].geno&(G_NOGEN|G_UNIQ)) || G_C_INST(mons[i].geno) != 0 || mvitals[i].mvflags&G_GONE || mons[i].mlet == S_PLANT || skips[j])){\
					if(mons[i].maligntyp < 0 && !(mons[i].geno&(G_NOGEN|G_UNIQ)) && G_C_INST(mons[i].geno) == 0 && !(mvitals[i].mvflags&G_GONE) && mons[i].mlet != S_PLANT) j++;\
					i++;\
				}\
				if(i < PM_LONG_WORM_TAIL){\
					mon = makemon(&mons[i], x, y, NO_MINVENT|MM_IGNOREWATER|MM_NOGROUP);\
					trap = maketrap(x, y, VIVI_TRAP);\
					trap->tseen = TRUE;\
					if(!mon) impossible("bad monster placement at %d, %d.", x, y);\
					else {\
						mon->mtrapped = 1;\
						mon->movement = 0;\
						mon->mhp = 1;\
					}\
					i++;\
					j++;\
				} else {\
					x = COLNO/2;\
					y = ROWNO/2;\
					break;\
				}\
			}

	i = 0;
	j = 0;
	for (y = 0; y<ROWNO/2; y++){
		for (x = 2*y+1; x<COLNO-2*y-1; x++){
			LOOP_BODY
		}
	}
	for (x = COLNO; x>COLNO/2; x--){
		for (y = (COLNO-x-1); y<(ROWNO-(COLNO-x-1)); y++){
			LOOP_BODY
		}
	}
	for (y = ROWNO; y>ROWNO/2; y--){
		for (x = COLNO-2*(ROWNO-y); x>=2*(ROWNO-y); x--){
			LOOP_BODY
		}
	}
	for (x = 0; x<COLNO/2; x++){
		for (y = ROWNO-(x-1); y>=((x-1)); y--){
			LOOP_BODY
		}
	}
	free(skips);
}
/*mkmaze.c*/
