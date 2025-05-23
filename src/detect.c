/*	SCCS Id: @(#)detect.c	3.4	2003/08/13	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Detection routines, including crystal ball, magic mapping, and search
 * command.
 */

#include "hack.h"
#include "artifact.h"

extern boolean known;	/* from read.c */

STATIC_DCL void FDECL(do_dknown_of, (struct obj *));
STATIC_DCL boolean FDECL(check_map_spot, (int,int,CHAR_P,unsigned));
STATIC_DCL boolean FDECL(clear_stale_map, (CHAR_P,unsigned));
STATIC_DCL void FDECL(sense_trap, (struct trap *,XCHAR_P,XCHAR_P,int));
STATIC_DCL void FDECL(show_map_spot, (int,int));
STATIC_PTR void FDECL(findone,(int,int,genericptr_t));
STATIC_PTR void FDECL(openone,(int,int,genericptr_t));
STATIC_PTR boolean unconstrain_map(void);
STATIC_PTR void reconstrain_map(void);
STATIC_PTR void map_redisplay(void);
STATIC_PTR void browse_map(const char *);
STATIC_PTR int reveal_terrain_getglyph(int, int, unsigned, int, unsigned);
STATIC_PTR void reveal_terrain(int);

/* Recursively search obj for an object in class oclass and return 1st found */
struct obj *
o_in(obj, oclass)
struct obj* obj;
char oclass;
{
	register struct obj* otmp;
	struct obj *temp;

	if (obj->oclass == oclass) return obj;

	if (Has_contents(obj)) {
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
		if (otmp->oclass == oclass) return otmp;
		else if (Has_contents(otmp) && (temp = o_in(otmp, oclass)))
		return temp;
	}
	return (struct obj *) 0;
}

/* Recursively search obj for an object made of specified material and return 1st found */
struct obj *
o_material(obj, material)
struct obj* obj;
unsigned material;
{
	register struct obj* otmp;
	struct obj *temp;

	if (obj_is_material(obj, material)) return obj;

	if (Has_contents(obj)) {
	for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
		if (obj_is_material(otmp, material)) return otmp;
		else if (Has_contents(otmp) && (temp = o_material(otmp, material)))
		return temp;
	}
	return (struct obj *) 0;
}

/* Recursively search obj for an artifact and return 1st found */
struct obj *
o_artifact(obj)
struct obj* obj;
{
	register struct obj* otmp;
	struct obj *temp;

	if (obj->oartifact) return obj;

	if (Has_contents(obj)) {
		for (otmp = obj->cobj; otmp; otmp = otmp->nobj)
		if (otmp->oartifact) return otmp;
		else if (Has_contents(otmp) && (temp = o_artifact(otmp)))
			return temp;
	}
	return (struct obj *) 0;
}

STATIC_OVL void
do_dknown_of(obj)
struct obj *obj;
{
	struct obj *otmp;

	obj->dknown = 1;
	if (Has_contents(obj)) {
	for(otmp = obj->cobj; otmp; otmp = otmp->nobj)
		do_dknown_of(otmp);
	}
}

/* Check whether the location has an outdated object displayed on it. */
STATIC_OVL boolean
check_map_spot(x, y, oclass, material)
int x, y;
register char oclass;
unsigned material;
{
	register int glyph;
	register struct obj *otmp;
	register struct monst *mtmp;

	glyph = glyph_at(x,y);
	if (glyph_is_object(glyph)) {
		/* there's some object shown here */
		if (oclass == ALL_CLASSES) {
		return((boolean)( !(level.objects[x][y] ||	 /* stale if nothing here */
				((mtmp = m_at(x,y)) != 0 &&
				(
#ifndef GOLDOBJ
				 mtmp->mgold ||
#endif
						 mtmp->minvent)))));
		} else {
		if (material && objects[glyph_to_obj(glyph)].oc_material == material) {
			/* the object shown here is of interest because material matches */
			for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
				if (o_material(otmp, GOLD)) return FALSE;
			/* didn't find it; perhaps a monster is carrying it */
			if ((mtmp = m_at(x,y)) != 0) {
				for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_material(otmp, GOLD)) return FALSE;
				}
			/* detection indicates removal of this object from the map */
			return TRUE;
		}
			if (oclass && objects[glyph_to_obj(glyph)].oc_class == oclass) {
			/* the object shown here is of interest because its class matches */
			for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
				if (o_in(otmp, oclass)) return FALSE;
			/* didn't find it; perhaps a monster is carrying it */
#ifndef GOLDOBJ
			if ((mtmp = m_at(x,y)) != 0) {
				if (oclass == COIN_CLASS && mtmp->mgold)
					return FALSE;
				else for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_in(otmp, oclass)) return FALSE;
				}
#else
			if ((mtmp = m_at(x,y)) != 0) {
				for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
					if (o_in(otmp, oclass)) return FALSE;
				}
#endif
			/* detection indicates removal of this object from the map */
			return TRUE;
			}
		}
	}
	return FALSE;
}

/*
   When doing detection, remove stale data from the map display (corpses
   rotted away, objects carried away by monsters, etc) so that it won't
   reappear after the detection has completed.  Return true if noticeable
   change occurs.
 */
STATIC_OVL boolean
clear_stale_map(oclass, material)
register char oclass;
unsigned material;
{
	register int zx, zy;
	register boolean change_made = FALSE;

	for (zx = 1; zx < COLNO; zx++)
		for (zy = 0; zy < ROWNO; zy++)
		if (check_map_spot(zx, zy, oclass,material)) {
			unmap_object(zx, zy);
			change_made = TRUE;
		}

	return change_made;
}

/* look for gold, on the floor or in monsters' possession */
int
gold_detect(sobj)
register struct obj *sobj;
{
	register struct obj *obj;
	register struct monst *mtmp;
	int uw = u.uinwater;
	int usw = u.usubwater;
	struct obj *temp;
	boolean stale;

	known = stale = clear_stale_map(COIN_CLASS,
				(unsigned)(sobj->blessed ? GOLD : 0));

	/* look for gold carried by monsters (might be in a container) */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;	/* probably not needed in this case but... */
#ifndef GOLDOBJ
	if (mtmp->mgold || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#else
	if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#endif
		known = TRUE;
		goto outgoldmap;	/* skip further searching */
	} else for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if (sobj->blessed && o_material(obj, GOLD)) {
			known = TRUE;
			goto outgoldmap;
		} else if (o_in(obj, COIN_CLASS)) {
		known = TRUE;
		goto outgoldmap;	/* skip further searching */
		}
	}
	
	/* look for gold objects */
	for (obj = fobj; obj; obj = obj->nobj) {
	if (sobj->blessed && o_material(obj, GOLD)) {
		known = TRUE;
		if (obj->ox != u.ux || obj->oy != u.uy) goto outgoldmap;
	} else if (o_in(obj, COIN_CLASS)) {
		known = TRUE;
		if (obj->ox != u.ux || obj->oy != u.uy) goto outgoldmap;
	}
	}

	if (!known) {
	/* no gold found on floor or monster's inventory.
	   adjust message if you have gold in your inventory */
	if (sobj) {
		char buf[BUFSZ];
		if (youracedata->mtyp == PM_GOLD_GOLEM || youracedata->mtyp == PM_TREASURY_GOLEM) {
			Sprintf(buf, "You feel like a million %s!",
				currency(2L));
		} else if (hidden_gold() ||
#ifndef GOLDOBJ
				u.ugold)
#else
					money_cnt(invent))
#endif
			Strcpy(buf,
				"You feel worried about your future financial situation.");
		else
			Strcpy(buf, "You feel materially poor.");
		strange_feeling(sobj, buf);
		}
	return(1);
	}
	/* only under me - no separate display required */
	if (stale) docrt();
	You("notice some gold between your %s.", makeplural(body_part(FOOT)));
	return(0);

outgoldmap:
	cls();

	u.uinwater = 0;
	u.usubwater = 0;
	/* Discover gold locations. */
	for (obj = fobj; obj; obj = obj->nobj) {
		if (sobj->blessed && (temp = o_material(obj, GOLD))) {
		if (temp != obj) {
		temp->ox = obj->ox;
		temp->oy = obj->oy;
		}
		map_object(temp,1);
	} else if ((temp = o_in(obj, COIN_CLASS))) {
		if (temp != obj) {
		temp->ox = obj->ox;
		temp->oy = obj->oy;
		}
		map_object(temp,1);
	}
	}
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;	/* probably overkill here */
#ifndef GOLDOBJ
	if (mtmp->mgold || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#else
	if (findgold(mtmp->minvent) || monsndx(mtmp->data) == PM_GOLD_GOLEM) {
#endif
		struct obj gold;

		gold.otyp = GOLD_PIECE;
		gold.ox = mtmp->mx;
		gold.oy = mtmp->my;
		map_object(&gold,1);
	} else for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if (sobj->blessed && (temp = o_material(obj, GOLD))) {
		temp->ox = mtmp->mx;
		temp->oy = mtmp->my;
		map_object(temp,1);
		break;
		} else if ((temp = o_in(obj, COIN_CLASS))) {
		temp->ox = mtmp->mx;
		temp->oy = mtmp->my;
		map_object(temp,1);
		break;
		}
	}
	
	newsym(u.ux,u.uy);
	You_feel("very greedy, and sense gold!");
	exercise(A_WIS, TRUE);
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	return(0);
}

/* returns 1 if nothing was detected		*/
/* returns 0 if something was detected		*/
int
food_detect(sobj)
register struct obj	*sobj;
{
	register struct obj *obj;
	register struct monst *mtmp;
	register int ct = 0, ctu = 0;
	boolean confused = (Confusion || (sobj && sobj->cursed)), stale;
	char oclass = confused ? POTION_CLASS : FOOD_CLASS;
	const char *what = confused ? something : "food";
	int uw = u.uinwater, usw = u.usubwater;

	stale = clear_stale_map(oclass, 0);

	for (obj = fobj; obj; obj = obj->nobj)
	if (o_in(obj, oclass)) {
		if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
		else ct++;
	}
	for (mtmp = fmon; mtmp && !ct; mtmp = mtmp->nmon) {
	/* no DEADMONSTER(mtmp) check needed since dmons never have inventory */
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if (o_in(obj, oclass)) {
		ct++;
		break;
		}
	}
	
	if (!ct && !ctu) {
	known = stale && !confused;
	if (stale) {
		docrt();
		You("sense a lack of %s nearby.", what);
		if (sobj && sobj->blessed) {
		if (!u.uedibility) Your("%s starts to tingle.", body_part(NOSE));
		u.uedibility = 1;
		}
	} else if (sobj) {
		char buf[BUFSZ];
		Sprintf(buf, "Your %s twitches%s.", body_part(NOSE),
			(sobj->blessed && !u.uedibility) ? " then starts to tingle" : "");
		if (sobj->blessed && !u.uedibility) {
		boolean savebeginner = flags.beginner;	/* prevent non-delivery of */
		flags.beginner = FALSE;			/* 	message			*/
		strange_feeling(sobj, buf);
		flags.beginner = savebeginner;
		u.uedibility = 1;
		} else
		strange_feeling(sobj, buf);
	}
	return !stale;
	} else if (!ct) {
	known = TRUE;
	You("%s %s nearby.", sobj ? "smell" : "sense", what);
	if (sobj && sobj->blessed) {
		if (!u.uedibility) pline("Your %s starts to tingle.", body_part(NOSE));
		u.uedibility = 1;
	}
	} else {
	struct obj *temp;
	known = TRUE;
	cls();
	u.uinwater = 0;
	u.usubwater = 0;
	for (obj = fobj; obj; obj = obj->nobj)
		if ((temp = o_in(obj, oclass)) != 0) {
		if (temp != obj) {
			temp->ox = obj->ox;
			temp->oy = obj->oy;
		}
		map_object(temp,1);
		}
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		/* no DEADMONSTER(mtmp) check needed since dmons never have inventory */
		for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if ((temp = o_in(obj, oclass)) != 0) {
			temp->ox = mtmp->mx;
			temp->oy = mtmp->my;
			map_object(temp,1);
			break;	/* skip rest of this monster's inventory */
		}
	newsym(u.ux,u.uy);
	if (sobj) {
		if (sobj->blessed) {
			Your("%s %s to tingle and you smell %s.", body_part(NOSE),
				u.uedibility ? "continues" : "starts", what);
		u.uedibility = 1;
		} else
		Your("%s tingles and you smell %s.", body_part(NOSE), what);
	}
	else You("sense %s.", what);
	display_nhwindow(WIN_MAP, TRUE);
	exercise(A_WIS, TRUE);
	docrt();
	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	}
	return(0);
}

/*
 * Used for scrolls, potions, spells, and crystal balls.  Returns:
 *
 *	1 - nothing was detected
 *	0 - something was detected
 */
int
object_detect(detector, class)
struct obj	*detector;	/* object doing the detecting */
int		class;		/* an object class, 0 for all */
{
	register int x, y;
	char stuff[BUFSZ];
	int is_cursed = (detector && detector->cursed);
	int do_dknown = (detector && (detector->oclass == POTION_CLASS ||
					detector->oclass == SPBOOK_CLASS ||
					detector->oartifact) &&
			detector->blessed);
	int ct = 0, ctu = 0;
	register struct obj *obj, *otmp = (struct obj *)0;
	register struct monst *mtmp;
	int uw = u.uinwater, usw = u.usubwater;
	int sym, boulder = 0;

	if (class < 0 || class >= MAXOCLASSES) {
	impossible("object_detect:  illegal class %d", class);
	class = 0;
	}

	/* Special boulder symbol check - does the class symbol happen
	 * to match iflags.bouldersym which is a user-defined?
	 * If so, that means we aren't sure what they really wanted to
	 * detect. Rather than trump anything, show both possibilities.
	 * We can exclude checking the buried obj chain for boulders below.
	 */
	sym = class ? def_oc_syms[class] : 0;
	if (sym && iflags.bouldersym && sym == iflags.bouldersym)
		boulder = ROCK_CLASS;

	if (Hallucination || (Confusion && class == SCROLL_CLASS))
	Strcpy(stuff, something);
	else
		Strcpy(stuff, class ? oclass_names[class] : "objects");
	if (boulder && class != ROCK_CLASS) Strcat(stuff, " and/or large stones");

	if (do_dknown) for(obj = invent; obj; obj = obj->nobj) do_dknown_of(obj);

	for (obj = fobj; obj; obj = obj->nobj) {
		if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) {
			if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
			else ct++;
		}
		if (do_dknown) do_dknown_of(obj);
	}

	if (!Is_paradise(&u.uz)){
		for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
		if (!class || o_in(obj, class)) {
		if (obj->ox == u.ux && obj->oy == u.uy) ctu++;
		else ct++;
		}
		if (do_dknown) do_dknown_of(obj);
		}
	}

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj) {
		if ((!class && !boulder) || o_in(obj, class) || o_in(obj, boulder)) ct++;
		if (do_dknown) do_dknown_of(obj);
	}
	if ((is_cursed && mtmp->m_ap_type == M_AP_OBJECT &&
		(!class || class == objects[mtmp->mappearance].oc_class)) ||
#ifndef GOLDOBJ
		(mtmp->mgold && (!class || class == COIN_CLASS))) {
#else
		(findgold(mtmp->minvent) && (!class || class == COIN_CLASS))) {
#endif
		ct++;
		break;
	}
	}

	if (!clear_stale_map(!class ? ALL_CLASSES : class, 0) && !ct) {
	if (!ctu) {
		if (detector){
			if(detector->otyp == SENSOR_PACK)
				pline("No objects detected.");
			else
				strange_feeling(detector, "You feel a lack of something.");
		}
		return 1;
	}

	You("sense %s nearby.", stuff);
	return 0;
	}

	cls();

	u.uinwater = 0;
	u.usubwater = 0;
/*
 *	Map all buried objects first.
 */
	if (!Is_paradise(&u.uz)){
		for (obj = level.buriedobjlist; obj; obj = obj->nobj)
		if (!class || (otmp = o_in(obj, class))) {
			if (class) {
			if (otmp != obj) {
				otmp->ox = obj->ox;
				otmp->oy = obj->oy;
			}
			map_object(otmp, 1);
			} else
			map_object(obj, 1);
		}
	}
	/*
	 * If we are mapping all objects, map only the top object of a pile or
	 * the first object in a monster's inventory.  Otherwise, go looking
	 * for a matching object class and display the first one encountered
	 * at each location.
	 *
	 * Objects on the floor override buried objects.
	 */
	for (x = 1; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
		for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if ((!class && !boulder) ||
			(otmp = o_in(obj, class)) || (otmp = o_in(obj, boulder))) {
			if (class || boulder) {
			if (otmp != obj) {
				otmp->ox = obj->ox;
				otmp->oy = obj->oy;
			}
			map_object(otmp, 1);
			} else
			map_object(obj, 1);
			break;
		}

	/* Objects in the monster's inventory override floor objects. */
	for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if ((!class && !boulder) ||
		 (otmp = o_in(obj, class)) || (otmp = o_in(obj, boulder))) {
		if (!class && !boulder) otmp = obj;
		otmp->ox = mtmp->mx;		/* at monster location */
		otmp->oy = mtmp->my;
		map_object(otmp, 1);
		break;
		}
	/* Allow a mimic to override the detected objects it is carrying. */
	if (is_cursed && mtmp->m_ap_type == M_AP_OBJECT &&
		(!class || class == objects[mtmp->mappearance].oc_class)) {
		struct obj temp = {0};

		temp.otyp = mtmp->mappearance;	/* needed for obj_to_glyph() */
		temp.ox = mtmp->mx;
		temp.oy = mtmp->my;
		temp.obj_color = objects[mtmp->mappearance].oc_color;
		temp.corpsenm = PM_TENGU;		/* if mimicing a corpse */
		map_object(&temp, 1);
#ifndef GOLDOBJ
	} else if (mtmp->mgold && (!class || class == COIN_CLASS)) {
#else
	} else if (findgold(mtmp->minvent) && (!class || class == COIN_CLASS)) {
#endif
		struct obj gold = {0};

		gold.otyp = GOLD_PIECE;
		gold.ox = mtmp->mx;
		gold.oy = mtmp->my;
		gold.obj_color = objects[GOLD_PIECE].oc_color;
		map_object(&gold, 1);
	}
	}

	newsym(u.ux,u.uy);
	You("detect the %s of %s.", ct ? "presence" : "absence", stuff);
	display_nhwindow(WIN_MAP, TRUE);
	/*
	 * What are we going to do when the hero does an object detect while blind
	 * and the detected object covers a known pool?
	 */
	docrt();	/* this will correctly reset vision */

	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	return 0;
}

/*
 * Used for artifact effects.  Returns:
 *
 *	1 - nothing was detected
 *	0 - something was detected
 */
int
artifact_detect(detector)
struct obj	*detector;	/* object doing the detecting */
{
	register int x, y;
	char stuff[BUFSZ];
	int is_cursed = (detector && detector->cursed);
	int do_dknown = (detector && (detector->oclass == POTION_CLASS ||
					detector->oclass == SPBOOK_CLASS ||
					detector->oartifact) &&
			detector->blessed);
	int ct = 0;
	register struct obj *obj, *otmp = (struct obj *)0;
	register struct monst *mtmp;
	int uw = u.uinwater;
	int usw = u.usubwater;

	if (is_cursed){ /* Possible false negative */
		Role_if(PM_PIRATE) ? strange_feeling(detector, "Ye feel a lack o' something.") : strange_feeling(detector, "You feel a lack of something.");
		return 1;
	}
	
	if (Hallucination)
		Strcpy(stuff, something);
	else
		Strcpy(stuff, "artifacts");

	if (do_dknown) for(obj = invent; obj; obj = obj->nobj) if(obj->oartifact) do_dknown_of(obj);

	for (obj = fobj; obj; obj = obj->nobj) {
		if (obj && o_artifact(obj)) {
			if (obj->ox != u.ux || obj->oy != u.uy) ct++;
			if (do_dknown) do_dknown_of(obj);
		}
	}

	for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
		if (obj && o_artifact(obj)) {
			if (obj->ox != u.ux || obj->oy != u.uy) ct++;
			if (do_dknown) do_dknown_of(obj);
		}
	}

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
		for (obj = mtmp->minvent; obj; obj = obj->nobj) {
			if (obj && o_artifact(obj)) {
				ct++;
				if (do_dknown) do_dknown_of(obj);
			}
		}
	}

	if (!clear_stale_map(ALL_CLASSES, 0) && !ct) {
		Role_if(PM_PIRATE) ? strange_feeling(detector, "Ye feel a lack o' something.") : strange_feeling(detector, "You feel a lack of something.");
		return 1;
	}

	cls();

	u.uinwater = 0;
	u.usubwater = 0;
/*
 *	Map all buried objects first.
 */

	for (obj = level.buriedobjlist; obj; obj = obj->nobj)
		if (obj && (otmp = o_artifact(obj))) {
			if (otmp != obj) {
				otmp->ox = obj->ox;
				otmp->oy = obj->oy;
			}
			map_object(otmp, 1);
		}
	/*
	 * If we are mapping all objects, map only the top object of a pile or
	 * the first object in a monster's inventory.  Otherwise, go looking
	 * for a matching object class and display the first one encountered
	 * at each location.
	 *
	 * Objects on the floor override buried objects.
	 */
	for (x = 1; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
		for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if (obj && (otmp = o_artifact(obj))) {
			if (otmp != obj) {
				otmp->ox = obj->ox;
				otmp->oy = obj->oy;
			}
			map_object(otmp, 1);
			break;
		}
	/* Objects in the monster's inventory override floor objects. */
	for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if (obj && (otmp = o_artifact(obj))) {
			otmp->ox = mtmp->mx;		/* at monster location */
			otmp->oy = mtmp->my;
			map_object(otmp, 1);
			break;
		}
	}

	newsym(u.ux,u.uy);
	You("detect the %s o%s %s.", ct ? "presence" : "absence", Role_if(PM_PIRATE) ? "'":"f",stuff);
	display_nhwindow(WIN_MAP, TRUE);
	/*
	 * What are we going to do when the hero does an object detect while blind
	 * and the detected object covers a known pool?
	 */
	docrt();	/* this will correctly reset vision */

	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	return 0;
}

/*
 * Used for spirit effects.  Returns:
 *
 *	1 - nothing was detected
 *	0 - something was detected
 */
int
book_detect(blessed)
boolean blessed;	/* do blessed detecting */
{
	register int x, y;
	char stuff[BUFSZ];
	int do_dknown = blessed;
	int ct = 0;
	register struct obj *obj;
	register struct monst *mtmp;
	int uw = u.uinwater;
	int usw = u.usubwater;
	
	if (Hallucination)
		Strcpy(stuff, something);
	else
		Strcpy(stuff, "books");

	if (do_dknown) for(obj = invent; obj; obj = obj->nobj) if(obj->oclass == SPBOOK_CLASS) do_dknown_of(obj);

	for (obj = fobj; obj; obj = obj->nobj) {
		if (obj && obj->oclass == SPBOOK_CLASS) {
			if (obj->ox != u.ux || obj->oy != u.uy) ct++;
			if (do_dknown) do_dknown_of(obj);
		}
	}

	for (obj = level.buriedobjlist; obj; obj = obj->nobj) {
		if (obj && obj->oclass == SPBOOK_CLASS) {
			if (obj->ox != u.ux || obj->oy != u.uy) ct++;
			if (do_dknown) do_dknown_of(obj);
		}
	}

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
		for (obj = mtmp->minvent; obj; obj = obj->nobj) {
			if (obj && obj->oclass == SPBOOK_CLASS){
				ct++;
				if (do_dknown) do_dknown_of(obj);
			}
		}
	}

	if (!clear_stale_map(ALL_CLASSES, 0) && !ct) {
		strange_feeling((struct obj *) 0, "You feel a lack of something.");
		return 1;
	}

	cls();

	u.uinwater = 0;
	u.usubwater = 0;
/*
 *	Map all buried objects first.
 */
	for (obj = level.buriedobjlist; obj; obj = obj->nobj)
		if (obj && obj->oclass == SPBOOK_CLASS) {
			map_object(obj, 1);
		}
	/*
	 * If we are mapping all objects, map only the top object of a pile or
	 * the first object in a monster's inventory.  Otherwise, go looking
	 * for a matching object class and display the first one encountered
	 * at each location.
	 *
	 * Objects on the floor override buried objects.
	 */
	for (x = 1; x < COLNO; x++)
	for (y = 0; y < ROWNO; y++)
		for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
		if (obj && obj->oclass == SPBOOK_CLASS) {
			map_object(obj, 1);
	break;
		}
	/* Objects in the monster's inventory override floor objects. */
	for (mtmp = fmon ; mtmp ; mtmp = mtmp->nmon) {
	if (DEADMONSTER(mtmp)) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj)
		if (obj && obj->oclass == SPBOOK_CLASS) {
			map_object(obj, 1);
	break;
		}
	}

	newsym(u.ux,u.uy);
	You("detect the %s of %s.", ct ? "presence" : "absence",stuff);
	display_nhwindow(WIN_MAP, TRUE);
	/*
	 * What are we going to do when the hero does an object detect while blind
	 * and the detected object covers a known pool?
	 */
	docrt();	/* this will correctly reset vision */

	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	return 0;
}

/*
 * Used by: crystal balls, potions, fountains
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
monster_detect(otmp, mclass)
register struct obj *otmp;	/* detecting object (if any) */
int mclass;			/* monster class, 0 for all */
{
	register struct monst *mtmp;
	int mcnt = 0;


	/* Note: This used to just check fmon for a non-zero value
	 * but in versions since 3.3.0 fmon can test TRUE due to the
	 * presence of dmons, so we have to find at least one
	 * with positive hit-points to know for sure.
	 */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if (!DEADMONSTER(mtmp)) {
		mcnt++;
		break;
	}

	if (!mcnt) {
	if (otmp){
		if(otmp->otyp == SENSOR_PACK)
			pline("No life-signs detected.");
		else
			strange_feeling(otmp, Hallucination ?
				"You get the heebie jeebies." :
				"You feel threatened.");
	}
	return 1;
	} else {
	boolean woken = FALSE;

	cls();
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if (!mclass || mtmp->data->mlet == mclass ||
			(mtmp->mtyp == PM_LONG_WORM && mclass == S_WORM_TAIL)
		)
			if (mtmp->mx > 0) {
				if (mclass && def_monsyms[mclass] == ' ')
					show_glyph(mtmp->mx,mtmp->my,
						detected_mon_to_glyph(mtmp));
				else
					show_glyph(mtmp->mx,mtmp->my,mon_to_glyph(mtmp));
				/* don't be stingy - display entire worm */
				if (mtmp->mtyp == PM_LONG_WORM) detect_wsegs(mtmp,0);
			}
		if (otmp && otmp->cursed &&
			(mtmp->msleeping || !mtmp->mcanmove)
		){
			mtmp->msleeping = mtmp->mfrozen = 0;
			if(mtmp->mtyp != PM_GIANT_TURTLE || !(mtmp->mflee))
				mtmp->mcanmove = 1;
			woken = TRUE;
		}
	}
	display_self();
	You("sense the presence of monsters.");
	if (woken)
		pline("Monsters sense the presence of you.");
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	}
	return 0;
}

int
pet_detect_and_heal(otmp)
register struct obj *otmp;	/* detecting object (if any) */
{
	register struct monst *mtmp;
	int mcnt = 0;


	/* Note: This used to just check fmon for a non-zero value
	 * but in versions since 3.3.0 fmon can test TRUE due to the
	 * presence of dmons, so we have to find at least one
	 * with positive hit-points to know for sure.
	 */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if (!DEADMONSTER(mtmp) && mtmp->mtame) {
		mcnt++;
		break;
	}

	if (!mcnt) {
	boolean savebeginner = flags.beginner;	/* prevent non-delivery of */
	flags.beginner = FALSE;			/* 	message			*/
	if (otmp)
		strange_feeling((struct obj *)0, Hallucination ?
				"You suddenly recall the hamster you had as a child." :
				"You feel lonely.");
	flags.beginner = savebeginner;
	return 1;
	} else {
	cls();
	display_self();
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if (mtmp->mtame && mtmp->mx > 0) {
		show_glyph(mtmp->mx,mtmp->my,mon_to_glyph(mtmp));
		/* don't be stingy - display entire worm */
		if (mtmp->mtyp == PM_LONG_WORM) detect_wsegs(mtmp,0);
		/* heal */
		if(canseemon(mtmp) && mtmp->mtame < 20) mtmp->mhp += d(4, 8);
		if (mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		}
	}
	You(Hallucination ?
		"are at one with your comrades." :
		"sense the presence of your retinue.");
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (!DEADMONSTER(mtmp) && (mtmp->mtame && mtmp->mx > 0) && canseemon(mtmp))
		pline("%s is in awe of %s!", upstart(y_monnam(mtmp)), yname(otmp));
	}
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	}
	return 0;
}

/*
 * Used by: LEADERSHIP artifacts (Clarent (from Greyknight's patch))
 *
 * Returns 1 if nothing was detected.
 * Returns 0 if something was detected.
 */
int
pet_detect_and_tame(otmp)
register struct obj *otmp;	/* detecting object (if any) */
{
	register struct monst *mtmp;
	int mcnt = 0;


	/* Note: This used to just check fmon for a non-zero value
	 * but in versions since 3.3.0 fmon can test TRUE due to the
	 * presence of dmons, so we have to find at least one
	 * with positive hit-points to know for sure.
	 */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if (!DEADMONSTER(mtmp) && mtmp->mtame) {
		mcnt++;
		break;
	}

	if (!mcnt) {
	boolean savebeginner = flags.beginner;	/* prevent non-delivery of */
	flags.beginner = FALSE;			/* 	message			*/
	if (otmp)
		strange_feeling((struct obj *)0, Hallucination ?
				"You suddenly recall the hamster you had as a child." :
				"You feel lonely.");
	flags.beginner = savebeginner;
	return 1;
	} else {
	cls();
	display_self();
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if (mtmp->mtame && mtmp->mx > 0) {
		show_glyph(mtmp->mx,mtmp->my,mon_to_glyph(mtmp));
		/* don't be stingy - display entire worm */
		if (mtmp->mtyp == PM_LONG_WORM) detect_wsegs(mtmp,0);
		/* increase tameness */
		if(canseemon(mtmp) && mtmp->mtame < 20) mtmp->mtame++;
		}
	}
	You(Hallucination ?
		"are at one with your comrades." :
		"sense the presence of your retinue.");
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (!DEADMONSTER(mtmp) && (mtmp->mtame && mtmp->mx > 0) && canseemon(mtmp))
		pline("%s is in awe of %s!", upstart(y_monnam(mtmp)), yname(otmp));
	}
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	}
	return 0;
}

STATIC_OVL void
sense_trap(trap, x, y, src_cursed)
struct trap *trap;
xchar x, y;
int src_cursed;
{
	if (Hallucination || src_cursed) {
	struct obj obj;			/* fake object */
	if (trap) {
		obj.ox = trap->tx;
		obj.oy = trap->ty;
	} else {
		obj.ox = x;
		obj.oy = y;
	}
	obj.otyp = (src_cursed) ? GOLD_PIECE : random_object();
	obj.corpsenm = random_monster();	/* if otyp == CORPSE */
	map_object(&obj,1);
	} else if (trap) {
	map_trap(trap,1);
	trap->tseen = 1;
	} else {
	struct trap temp_trap;		/* fake trap */
	temp_trap.tx = x;
	temp_trap.ty = y;
	temp_trap.ttyp = BEAR_TRAP;	/* some kind of trap */
	map_trap(&temp_trap,1);
	}

}

/* the detections are pulled out so they can	*/
/* also be used in the crystal ball routine	*/
/* returns 1 if nothing was detected		*/
/* returns 0 if something was detected		*/
int
trap_detect(sobj)
register struct obj *sobj;
/* sobj is null if crystal ball, *scroll if gold detection scroll */
{
	register struct trap *ttmp;
	register struct obj *obj;
	register int door;
	int uw = u.uinwater;
	int usw = u.usubwater;
	boolean found = FALSE;
	coord cc;

	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
	if (ttmp->tx != u.ux || ttmp->ty != u.uy)
		goto outtrapmap;
	else found = TRUE;
	}
	for (obj = fobj; obj; obj = obj->nobj) {
	if ((obj->otyp==BOX || obj->otyp==SARCOPHAGUS || obj->otyp==CHEST) && obj->otrapped) {
		if (obj->ox != u.ux || obj->oy != u.uy)
		goto outtrapmap;
		else found = TRUE;
	}
	}
	for (door = 0; door < doorindex; door++) {
	cc.x = doors[door].x;
	cc.y = doors[door].y;
	if (levl[cc.x][cc.y].doormask & D_TRAPPED) {
		if (cc.x != u.ux || cc.y != u.uy)
		goto outtrapmap;
		else found = TRUE;
	}
	}
	if (!found) {
	char buf[42];
	Sprintf(buf, "Your %s stop itching.", makeplural(body_part(TOE)));
	strange_feeling(sobj,buf);
	return(1);
	}
	/* traps exist, but only under me - no separate display required */
	Your("%s itch.", makeplural(body_part(TOE)));
	return(0);
outtrapmap:
	cls();

	u.uinwater = 0;
	u.usubwater = 0;
	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
	sense_trap(ttmp, 0, 0, sobj && sobj->cursed);

	for (obj = fobj; obj; obj = obj->nobj)
	if ((obj->otyp==BOX || obj->otyp==SARCOPHAGUS || obj->otyp==CHEST) && obj->otrapped)
	sense_trap((struct trap *)0, obj->ox, obj->oy, sobj && sobj->cursed);

	for (door = 0; door < doorindex; door++) {
		cc.x = doors[door].x;
		cc.y = doors[door].y;
		if (levl[cc.x][cc.y].doormask & D_TRAPPED)
		sense_trap((struct trap *)0, cc.x, cc.y, sobj && sobj->cursed);
	}

	newsym(u.ux,u.uy);
	You_feel("%s.", sobj && sobj->cursed ? "very greedy" : "entrapped");
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	u.uinwater = uw;
	u.usubwater = usw;
	if (Underwater) under_water(2);
	if (u.uburied) under_ground(2);
	return(0);
}

const char *
level_distance(where)
d_level *where;
{
	register schar ll = depth(&u.uz) - depth(where);
	register boolean indun = (u.uz.dnum == where->dnum);

	if (ll < 0) {
	if (ll < (-8 - rn2(3)))
		if (!indun)	return "far away";
		else	return "far below";
	else if (ll < -1)
		if (!indun)	return "away below you";
		else	return "below you";
	else
		if (!indun)	return "in the distance";
		else	return "just below";
	} else if (ll > 0) {
	if (ll > (8 + rn2(3)))
		if (!indun)	return "far away";
		else	return "far above";
	else if (ll > 1)
		if (!indun)	return "away above you";
		else	return "above you";
	else
		if (!indun)	return "in the distance";
		else	return "just above";
	} else
		if (!indun)	return "in the distance";
		else	return "near you";
}

static const struct {
	const char *what;
	d_level *where;
} level_detects[] = {
  { "Delphi", &oracle_level },
  { "a worthy opponent", &challenge_level },
  { "a castle", &stronghold_level },
  { "the Wizard of Yendor's tower", &wiz1_level },
};

int
use_crystal_ball(obj)
struct obj *obj;
{
	char ch;
	int oops;

	if (Blind) {
	pline("Too bad you can't see %s.", the(xname(obj)));
	return 0;
	}
	oops = (rnd(obj->blessed ? 16 : 20) > ACURR(A_INT) || obj->cursed);
	if (oops && (obj->spe > 0)) {
	switch (rnd(obj->oartifact ? 4 : 5)) {
		case 1 : pline("%s too much to comprehend!", Tobjnam(obj, "are"));
		break;
	case 2 : pline("%s you!", Tobjnam(obj, "confuse"));
		make_confused(HConfusion + rnd(100),FALSE);
		break;
	case 3 : if (!resists_blnd(&youmonst)) {
		pline("%s your vision!", Tobjnam(obj, "damage"));
		make_blinded(Blinded + rnd(100),FALSE);
		if (!Blind) Your1(vision_clears);
		} else {
		pline("%s your vision.", Tobjnam(obj, "assault"));
		You("are unaffected!");
		}
		break;
	case 4 : pline("%s your mind!", Tobjnam(obj, "zap"));
		(void) make_hallucinated(HHallucination + rnd(100),FALSE,0L);
		break;
	case 5 : pline("%s!", Tobjnam(obj, "explode"));
		useup(obj);
		obj = 0;	/* it's gone */
		losehp(rnd(30), "exploding crystal ball", KILLED_BY_AN);
		break;
	}
	if (obj){
		consume_obj_charge(obj, TRUE);
		return 1;
	} else return 0;
	}

	if (Hallucination) {
	if (!obj->spe) {
		pline("All you see is funky %s haze.", hcolor((char *)0));
	} else {
		switch(rnd(6)) {
		case 1 : You("grok some groovy globs of incandescent lava.");
		break;
		case 2 : pline("Whoa!  Psychedelic colors, %s!",
			   poly_gender() == 1 ? "babe" : "dude");
		break;
		case 3 : pline_The("crystal pulses with sinister %s light!",
				hcolor((char *)0));
		break;
		case 4 : You("see goldfish swimming above fluorescent rocks.");
		break;
		case 5 : You("see tiny snowflakes spinning around a miniature farmhouse.");
		break;
		default: pline("Oh wow... like a kaleidoscope!");
		break;
		}
		consume_obj_charge(obj, TRUE);
	}
	return 1;
	}

	/* read a single character */
	if (flags.verbose) You("may look for an object or monster symbol.");
	ch = yn_function("What do you look for?", (char *)0, '\0');
	/* Don't filter out ' ' here; it has a use */
	if ((ch != def_monsyms[S_GHOST]) && (ch != def_monsyms[S_SHADE]) && index(quitchars,ch)) { 
	if (flags.verbose) pline1(Never_mind);
	return 1;
	}
	You("peer into %s...", the(xname(obj)));
	nomul(-rnd(obj->blessed ? 6 : 10), "peering into a crystal ball");
	nomovemsg = "";
	if (obj->spe <= 0)
	pline_The("vision is unclear.");
	else {
	int class;
	int ret = 0;

	makeknown(CRYSTAL_BALL);
	consume_obj_charge(obj, TRUE);

	/* special case: accept ']' as synonym for mimic
	 * we have to do this before the def_char_to_objclass check
	 */
	if (ch == DEF_MIMIC_DEF) ch = DEF_MIMIC;

	if ((class = def_char_to_objclass(ch)) != MAXOCLASSES)
		ret = object_detect((struct obj *)0, class);
	else if ((class = def_char_to_monclass(ch)) != MAXMCLASSES)
		ret = monster_detect((struct obj *)0, class);
	else if (iflags.bouldersym && (ch == iflags.bouldersym))
		ret = object_detect((struct obj *)0, ROCK_CLASS);
	else switch(ch) {
		case '^':
			ret = trap_detect((struct obj *)0);
			break;
		default:
			{
			int i = rn2(SIZE(level_detects));
			You("see %s, %s.",
			level_detects[i].what,
			level_distance(level_detects[i].where));
			}
			ret = 0;
			break;
	}

	if (ret) {
		if (!rn2(100))  /* make them nervous */
		You("see the Wizard of Yendor gazing out at you.");
		else pline_The("vision is unclear.");
	}
	}
	return 1;
}

STATIC_OVL void
show_map_spot(x, y)
register int x, y;
{
	register struct rm *lev;

	if (Confusion && rn2(7)) return;
	lev = &levl[x][y];

	lev->seenv = SVALL;

	/* Secret corridors are found, but not secret doors. */
	if (lev->typ == SCORR) {
	lev->typ = CORR;
	unblock_point(x,y);
	}

	/* if we don't remember an object or trap there, map it */
	if (lev->typ == ROOM ?
		(glyph_is_cmap(lev->glyph) && !glyph_is_trap(lev->glyph) &&
		glyph_to_cmap(lev->glyph) != ROOM) :
		(!glyph_is_object(lev->glyph) && !glyph_is_trap(lev->glyph))) {
	if (level.flags.hero_memory) {
		magic_map_background(x,y,0);
		newsym(x,y);			/* show it, if not blocked */
	} else {
		magic_map_background(x,y,1);	/* display it */
	}
	}
}

void
do_mapping()
{
	register int zx, zy;
	int uw = u.uinwater;
	int usw = u.usubwater;

	u.uinwater = 0;
	u.usubwater = 0;
	for (zx = 1; zx < COLNO; zx++)
	for (zy = 0; zy < ROWNO; zy++)
		show_map_spot(zx, zy);
	exercise(A_WIS, TRUE);
	u.uinwater = uw;
	u.usubwater = usw;
	if (!level.flags.hero_memory || Underwater) {
	flush_screen(1);			/* flush temp screen */
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
	}
}

void
do_vicinity_map(x,y)
int x, y;
{
	register int zx, zy;
	int lo_y = (y-5 < 0 ? 0 : y-5),
	hi_y = (y+6 > ROWNO ? ROWNO : y+6),
	lo_x = (x-9 < 1 ? 1 : x-9),	/* avoid column 0 */
	hi_x = (x+10 > COLNO ? COLNO : x+10);

	for (zx = lo_x; zx < hi_x; zx++)
	for (zy = lo_y; zy < hi_y; zy++)
		show_map_spot(zx, zy);

	if (!level.flags.hero_memory || Underwater) {
	flush_screen(1);			/* flush temp screen */
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
	}
}

/* convert a secret door into a normal door */
void
cvt_sdoor_to_door(lev)
struct rm *lev;
{
	int newmask = lev->doormask & ~WM_MASK;

#ifdef REINCARNATION
	if (Is_rogue_level(&u.uz))
		/* rogue didn't have doors, only doorways */
		newmask = D_NODOOR;
	else
#endif
		/* newly exposed door is closed */
		if (!(newmask & D_LOCKED)) newmask |= D_CLOSED;

	lev->typ = DOOR;
	lev->doormask = newmask;
}


STATIC_PTR void
findone(zx,zy,num)
int zx,zy;
genericptr_t num;
{
	register struct trap *ttmp;
	register struct monst *mtmp;

	if(levl[zx][zy].typ == SDOOR) {
		cvt_sdoor_to_door(&levl[zx][zy]);	/* .typ = DOOR */
		magic_map_background(zx, zy, 0);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if(levl[zx][zy].typ == SCORR) {
		levl[zx][zy].typ = CORR;
		unblock_point(zx,zy);
		magic_map_background(zx, zy, 0);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if ((ttmp = t_at(zx, zy)) != 0) {
		if(!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
			ttmp->tseen = 1;
			newsym(zx,zy);
			(*(int*)num)++;
		}
	} else if ((mtmp = m_at(zx, zy)) != 0) {
		if(mtmp->m_ap_type) {
			seemimic(mtmp);
			(*(int*)num)++;
		}
		if (mtmp->mundetected &&
			(is_hider(mtmp->data) || is_underswimmer(mtmp->data))) {
			mtmp->mundetected = 0;
			newsym(zx, zy);
			(*(int*)num)++;
		}
		if (!canspotmon(mtmp) &&
					!glyph_is_invisible(levl[zx][zy].glyph))
			map_invisible(zx, zy);
	} else if (glyph_is_invisible(levl[zx][zy].glyph)) {
		unmap_object(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	}
}

STATIC_PTR void
openone(zx,zy,num)
int zx,zy;
genericptr_t num;
{
	register struct trap *ttmp;
	register struct obj *otmp;

	if(OBJ_AT(zx, zy)) {
		for(otmp = level.objects[zx][zy];
				otmp; otmp = otmp->nexthere) {
			if(Is_box(otmp) && otmp->olocked) {
			otmp->olocked = 0;
			(*(int*)num)++;
			}
		}
		/* let it fall to the next cases. could be on trap. */
	}
	if(levl[zx][zy].typ == SDOOR || (levl[zx][zy].typ == DOOR &&
			  (levl[zx][zy].doormask & (D_CLOSED|D_LOCKED)))) {
		if(levl[zx][zy].typ == SDOOR)
			cvt_sdoor_to_door(&levl[zx][zy]);	/* .typ = DOOR */
		if(levl[zx][zy].doormask & D_TRAPPED) {
			if(distu(zx, zy) < 3) b_trapped("door", 0);
			else Norep("You %s an explosion!",
				cansee(zx, zy) ? "see" :
				   (flags.soundok ? "hear" :
						"feel the shock of"));
			wake_nearto_noisy(zx, zy, 11*11);
			levl[zx][zy].doormask = D_NODOOR;
		} else
			levl[zx][zy].doormask = D_ISOPEN;
		unblock_point(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if(levl[zx][zy].typ == SCORR) {
		levl[zx][zy].typ = CORR;
		unblock_point(zx, zy);
		newsym(zx, zy);
		(*(int*)num)++;
	} else if ((ttmp = t_at(zx, zy)) != 0) {
		if (!ttmp->tseen && ttmp->ttyp != STATUE_TRAP) {
			ttmp->tseen = 1;
			newsym(zx,zy);
			(*(int*)num)++;
		}
	} else if (find_drawbridge(&zx, &zy)) {
		/* make sure it isn't an open drawbridge */
		open_drawbridge(zx, zy);
		(*(int*)num)++;
	}
}

int
findit()	/* returns number of things found */
{
	int num = 0;

	if(u.uswallow) return(0);
	do_clear_area(u.ux, u.uy, BOLT_LIM, findone, (genericptr_t) &num);
	return(num);
}

int
openit()	/* returns number of things found and opened */
{
	int num = 0;

	if(u.uswallow) {
		if (is_animal(u.ustuck->data)) {
			if (Blind) pline("Its mouth opens!");
			else pline("%s opens its mouth!", Monnam(u.ustuck));
		}
		expels(u.ustuck, u.ustuck->data, TRUE);
		return(-1);
	}

	do_clear_area(u.ux, u.uy, BOLT_LIM, openone, (genericptr_t) &num);
	return(num);
}

void
find_trap(trap)
struct trap *trap;
{
	int tt = what_trap(trap->ttyp);
	boolean cleared = FALSE;

	trap->tseen = 1;
	exercise(A_WIS, TRUE);
	if (Blind)
	feel_location(trap->tx, trap->ty);
	else
	newsym(trap->tx, trap->ty);

	if (levl[trap->tx][trap->ty].glyph != trap_to_glyph(trap)) {
		/* There's too much clutter to see your find otherwise */
	cls();
	map_trap(trap, 1);
	display_self();
	cleared = TRUE;
	}

	You("find %s.", an(defsyms[trap_to_defsym(tt)].explanation));

	if (cleared) {
	display_nhwindow(WIN_MAP, TRUE);	/* wait */
	docrt();
	}
}

int
dosearch0(aflag)
register int aflag;
{
#ifdef GCC_BUG
/* some versions of gcc seriously muck up nested loops. if you get strange
   crashes while searching in a version compiled with gcc, try putting
   #define GCC_BUG in *conf.h (or adding -DGCC_BUG to CFLAGS in the
   makefile).
 */
	volatile xchar x, y;
#else
	register xchar x, y;
#endif
	register struct trap *trap;
	register struct monst *mtmp;

	if(u.uswallow) {
		if (!aflag)
			pline("What are you looking for?  The exit?");
	} else {//note, was SPFX_SEARCH.  SEEK isn't anywhere, I think this was a bug -Chris
		int fund = (uwep && uwep->oartifact &&
			arti_worn_prop(uwep, ARTP_SEEK)) ?
			uwep->spe : 0;
		if(u.sealsActive&SEAL_OTIAX) fund += spiritDsize();
		if(Role_if(PM_ROGUE)){
			fund += 2;
			if(u.ulevel > 13)
				fund += 3;
			if(u.ulevel > 29)
				fund += 5;
		}
		if (ublindf && ublindf->otyp == LENSES && !Blind)
			fund += 2; /* JDS: lenses help searching */
		if (ublindf && ublindf->otyp == SUNGLASSES && !Blind)
			fund -= 2; /* wearing sunglasses indoors hinders searching */
		for(x = u.ux-1; x < u.ux+2; x++)
		  for(y = u.uy-1; y < u.uy+2; y++) {
		if(!isok(x,y)) continue;
		if(x != u.ux || y != u.uy) {
			// if (Blind && !aflag) feel_location(x,y);
			if (!cansee(x,y) && !aflag) feel_location(x,y);
			if(levl[x][y].typ == SDOOR) {
			if(rnl(100) >= 14+fund) continue;
			cvt_sdoor_to_door(&levl[x][y]);	/* .typ = DOOR */
			exercise(A_WIS, TRUE);
			nomul(0, NULL);
			if (!cansee(x,y) && !aflag)
				feel_location(x,y);	/* make sure it shows up */
			else
				newsym(x,y);
			} else if(levl[x][y].typ == SCORR) {
			if(rnl(100) >= 14+fund) continue;
			levl[x][y].typ = CORR;
			unblock_point(x,y);	/* vision */
			exercise(A_WIS, TRUE);
			nomul(0, NULL);
			newsym(x,y);
			} else {
		/* Be careful not to find anything in an SCORR or SDOOR */
			if((mtmp = m_at(x, y)) && !aflag) {
				if(mtmp->m_ap_type) {
					seemimic(mtmp);
		find:		exercise(A_WIS, TRUE);
				if (!canspotmon(mtmp)) {
					if (glyph_is_invisible(levl[x][y].glyph)) {
					/* found invisible monster in a square
					 * which already has an 'I' in it.
					 * Logically, this should still take
					 * time and lead to a return(1), but if
					 * we did that the player would keep
					 * finding the same monster every turn.
					 */
					continue;
					} else {
					You_feel("an unseen monster!");
					map_invisible(x, y);
					/* It used to take time to find a single
					 * monster this made pets very irritating
					 * if moving around blind, since the pet
					 * would be dancing all around you and
					 * obstructing all your search attempts.
					 * Since Drow especially spend a LOT of
					 * time blind, this has to go.
					 */
					continue;
					}
				} else if (!sensemon(mtmp))
					You("find %s.", a_monnam(mtmp));
				return MOVE_STANDARD;
				}
				if(!canspotmon(mtmp)) {
				if (mtmp->mundetected &&
				   (is_hider(mtmp->data) || is_underswimmer(mtmp->data)))
					mtmp->mundetected = 0;
				newsym(x,y);
				goto find;
				}
			}

			/* see if an invisible monster has moved--if Blind,
			 * feel_location() already did it
			 */
			if (!aflag && !mtmp && !Blind &&
					glyph_is_invisible(levl[x][y].glyph)) {
				unmap_object(x,y);
				newsym(x,y);
			}

			if ((trap = t_at(x,y)) && !trap->tseen && rnl(100) < 22) {
				nomul(0, NULL);

				if (trap->ttyp == STATUE_TRAP) {
				if (activate_statue_trap(trap, x, y, FALSE))
					exercise(A_WIS, TRUE);
				return MOVE_STANDARD;
				} else {
				find_trap(trap);
				}
			}
			}
		}
		}
	}
	return MOVE_STANDARD;
}

int
dosearch()
{
	return(dosearch0(0));
}

/* Pre-map the sokoban levels */
void
sokoban_detect()
{
	register int x, y;
	register struct trap *ttmp;
	register struct obj *obj;

	/* Map the background and boulders */
	for (x = 1; x < COLNO; x++)
		for (y = 0; y < ROWNO; y++) {
			levl[x][y].seenv = SVALL;
			if(darksight(youracedata)){
				set_lit(x,y,0);
				levl[x][y].waslit = FALSE;
			}
			else levl[x][y].waslit = TRUE;
			map_background(x, y, 1);
			for (obj = level.objects[x][y]; obj; obj = obj->nexthere)
				if (obj->otyp == MASSIVE_STONE_CRATE)
					map_object(obj, 1);
		}

	/* Map the traps */
	for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		ttmp->tseen = 1;
		map_trap(ttmp, 1);
	}
}

/* bring hero out from underwater or underground or being engulfed;
   return True iff any change occurred */
static boolean
unconstrain_map(void)
{
    boolean res = u.uinwater || u.uburied || u.uswallow;

    /* bring Underwater, buried, or swallowed hero to normal map;
       bypass set_uinwater() */
    iflags.save_uinwater = u.uinwater, u.uinwater = 0;
    iflags.save_uburied  = u.uburied,  u.uburied  = 0;
    iflags.save_uswallow = u.uswallow, u.uswallow = 0;

    return res;
}

/* put hero back underwater or underground or engulfed */
static void
reconstrain_map(void)
{
    /* if was in water and taken out, put back; bypass set_uinwater() */
    u.uinwater = iflags.save_uinwater, iflags.save_uinwater = 0;
    u.uburied  = iflags.save_uburied,  iflags.save_uburied  = 0;
    u.uswallow = iflags.save_uswallow, iflags.save_uswallow = 0;
}

static void
map_redisplay(void)
{
    reconstrain_map();
    docrt(); /* redraw the screen to remove unseen traps from the map */
    if (Underwater)
        under_water(2);
    if (u.uburied)
        under_ground(2);
}

#define TER_MAP    0x01
#define TER_TRP    0x02
#define TER_OBJ    0x04
#define TER_MON    0x08
#define TER_FULL   0x10   /* explore|wizard mode view full map */
#define TER_DETECT 0x20   /* detect_foo magic rather than #terrain */

static int
reveal_terrain_getglyph(
    int x, int y,
    unsigned swallowed,
    int default_glyph,
    unsigned which_subset)
{
    int glyph, levl_glyph;
    uchar seenv;
    boolean keep_traps = (which_subset & TER_TRP) != 0,
            keep_objs = (which_subset & TER_OBJ) != 0,
            keep_mons = (which_subset & TER_MON) != 0,
            full = (which_subset & TER_FULL) != 0;
    struct monst *mtmp;
    struct trap *t;

    /* for 'full', show the actual terrain for the entire level,
       otherwise what the hero remembers for seen locations with
       monsters, objects, and/or traps removed as caller dictates */
    seenv = (full || level.flags.hero_memory)
              ? levl[x][y].seenv : cansee(x, y) ? SVALL : 0;
    if (full) {
        levl[x][y].seenv = SVALL;
        glyph = back_to_glyph(x, y);
        levl[x][y].seenv = seenv;
    } else {
        levl_glyph = level.flags.hero_memory
              ? levl[x][y].glyph
              : seenv ? back_to_glyph(x, y): default_glyph;
        /* glyph_at() returns the displayed glyph, which might
           be a monster.  levl[][].glyph contains the remembered
           glyph, which will never be a monster (unless it is
           the invisible monster glyph, which is handled like
           an object, replacing any object or trap at its spot) */
        glyph = !swallowed ? glyph_at(x, y) : levl_glyph;
        if (keep_mons && (u.ux == x && u.uy == y) && swallowed)
            glyph = mon_to_glyph(u.ustuck);
        else if (((glyph_is_monster(glyph)
                   || glyph_is_warning(glyph)) && !keep_mons)
                 || glyph_is_swallow(glyph))
            glyph = levl_glyph;
        if (((glyph_is_object(glyph) && !keep_objs)
             || glyph_is_invisible(glyph))
            && keep_traps && !covers_traps(x, y)) {
            if ((t = t_at(x, y)) != 0 && t->tseen)
                glyph = trap_to_glyph(t);
        }
        if ((glyph_is_object(glyph) && !keep_objs)
            || (glyph_is_trap(glyph) && !keep_traps)
            || glyph_is_invisible(glyph)) {
            if (!seenv) {
                glyph = default_glyph;
            } else if (levl[x][y].styp == levl[x][y].typ) {
                glyph = back_to_glyph(x, y);
            } else {
                /* look for a mimic here posing as furniture;
                   if we don't find one, we'll have to fake it */
                if ((mtmp = m_at(x, y)) != 0
                    && mtmp->m_ap_type == M_AP_FURNITURE) {
                    glyph = cmap_to_glyph(mtmp->mappearance);
                } else {
                    struct rm save_spot;

                    /*
                     * We have a topology type but we want a screen symbol
                     * in order to derive a glyph.  Some screen symbols need
                     * the flags field of levl[][] in addition to the type
                     * (to disambiguate STAIRS to S_upstair or S_dnstair,
                     * for example).  Current flags might not be intended
                     * for remembered type, but we've got no other choice.
                     * An exception is wall_info which can be recalculated and
                     * needs to be.  Otherwise back_to_glyph() -> wall_angle()
                     * might issue an impossible() for it if it is currently
                     * doormask==D_OPEN for an open door remembered as a wall.
                     */
                    save_spot = levl[x][y];
                    levl[x][y].typ = levl[x][y].styp;
                    glyph = back_to_glyph(x, y);
                    levl[x][y] = save_spot;
                }
            }
        }
    }
    /* FIXME: dirty hack */
    if (glyph == cmap_to_glyph(S_drkroom))
        glyph = cmap_to_glyph(S_litroom);
    else if (glyph == cmap_to_glyph(S_litcorr))
        glyph = cmap_to_glyph(S_corr);
    return glyph;
}

/* use getpos()'s 'autodescribe' to view whatever is currently shown on map */
static void
browse_map(const char *ter_explain)
{
    coord dummy_pos; /* don't care whether player actually picks a spot */
	boolean adescribe = iflags.autodescribe; /* preserve previous autodescribe state */
	
    dummy_pos.x = u.ux, dummy_pos.y = u.uy; /* starting spot for getpos() */
	iflags.autodescribe = TRUE;
    (void) getpos(&dummy_pos, FALSE, ter_explain);
	iflags.autodescribe = adescribe;
}

void
reveal_terrain(which_subset)
	int which_subset; /* TER_TRP | TER_OBJ | TER_MON | TER_FULL */
{
    /* 'full' overrides impairment and implies no-traps, no-objs, no-mons */
    boolean full = (which_subset & TER_FULL) != 0; /* show whole map */

    if ((Hallucination || Stunned || Confusion) && !full) {
        You("are too disoriented for this.");
    } else {
        int  x, y;
        int glyph, default_glyph;
        char buf[BUFSZ];
        /* there is a TER_MAP bit too; we always show map regardless of it */
        boolean keep_traps = (which_subset & TER_TRP) != 0,
                keep_objs = (which_subset & TER_OBJ) != 0,
                keep_mons = (which_subset & TER_MON) != 0; /* not used */
        unsigned swallowed = u.uswallow; /* before unconstrain_map() */

        if (unconstrain_map())
            docrt();
        default_glyph = cmap_to_glyph(level.flags.arboreal ? S_tree : S_stone);

        for (x = 1; x < COLNO; x++)
            for (y = 0; y < ROWNO; y++) {
                glyph = reveal_terrain_getglyph(x,y, swallowed,
                                                default_glyph, which_subset);
                show_glyph(x, y, glyph);
            }

        /* hero's location is not highlighted, but getpos() starts with
           cursor there, and after moving it anywhere '@' moves it back */
        flush_screen(1);
        if (full) {
            Strcpy(buf, "underlying terrain");
        } else {
            Strcpy(buf, "known terrain");
            if (keep_traps)
                Sprintf(eos(buf), "%s traps",
                        (keep_objs || keep_mons) ? "," : " and");
            if (keep_objs)
                Sprintf(eos(buf), "%s%s objects",
                        (keep_traps || keep_mons) ? "," : "",
                        keep_mons ? "" : " and");
            if (keep_mons)
                Sprintf(eos(buf), "%s and monsters",
                        (keep_traps || keep_objs) ? "," : "");
        }
        pline("Showing %s only...", buf);

        /* allow player to move cursor around and get autodescribe feedback
           based on what is visible now rather than what is on 'real' map */
        which_subset |= TER_MAP; /* guarantee non-zero */
        browse_map("anything of interest");

        map_redisplay();
    }
    return;
}

/* #terrain command -- show known map, inspired by crawl's '|' command */
int
doterrain(void)
{
    winid men;
    menu_item *sel;
    anything any;
    int n;
    int which;
    int clr = 0;

    /*
     * normal play: choose between known map without mons, obj, and traps
     *  (to see underlying terrain only), or
     *  known map without mons and objs (to see traps under mons and objs), or
     *  known map without mons (to see objects under monsters);
     * explore mode: normal choices plus full map (w/o mons, objs, traps);
     * wizard mode: normal and explore choices plus
     *  a dump of the internal levl[][].typ codes w/ level flags, or
     *  a legend for the levl[][].typ codes dump
     */
    men = create_nhwindow(NHW_MENU);
    start_menu(men);
    any.a_void = 0;


    any.a_int = 1;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "known map without monsters, objects, and traps",
             MENU_SELECTED);
			 
    any.a_int = 2;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "known map without monsters and objects",
             MENU_UNSELECTED);
    any.a_int = 3;
    add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
             "known map without monsters",
             MENU_UNSELECTED);
    end_menu(men, "View which?");
	
	if (discover || wizard){
		any.a_int = 4;
        add_menu(men, NO_GLYPH, &any, 0, 0, ATR_NONE,
                 "full map without monsters, objects, and traps",
                 MENU_UNSELECTED);
	}
    n = select_menu(men, PICK_ONE, &sel);
    destroy_nhwindow(men);
    /*
     * n <  0: player used ESC to cancel;
     * n == 0: preselected entry was explicitly chosen and got toggled off;
     * n == 1: preselected entry was implicitly chosen via <space>|<enter>;
     * n == 2: another entry was explicitly chosen, so skip preselected one.
     */
    which = (n < 0) ? -1 : (n == 0) ? 1 : sel[0].item.a_int;
    if (n > 1 && which == 1)
        which = sel[1].item.a_int;
    if (n > 0)
        free((genericptr_t) sel);

    switch (which) {
    case 1: /* known map */
        reveal_terrain(TER_MAP);
        break;
    case 2: /* known map with known traps */
        reveal_terrain(TER_MAP | TER_TRP);
        break;
    case 3: /* known map with known traps and objects */
        reveal_terrain(TER_MAP | TER_TRP | TER_OBJ);
        break;
    case 4: /* full map */
        reveal_terrain(TER_MAP | TER_FULL);
        break;
    default:
        break;
    }
    return 0; /* no time elapses */
}

#undef TER_MAP
#undef TER_TRP
#undef TER_OBJ
#undef TER_MON
#undef TER_FULL
#undef TER_DETECT

/*detect.c*/
