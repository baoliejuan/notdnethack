/*	SCCS Id: @(#)do.c	3.4	2003/12/02	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 'd', 'D' (drop), '>', '<' (up, down) */

#include "hack.h"
#include "lev.h"
#include "artifact.h"

#ifdef SINKS
# ifdef OVLB
STATIC_DCL void FDECL(trycall, (struct obj *));
# endif /* OVLB */
STATIC_DCL void FDECL(dosinkring, (struct obj *));
#endif /* SINKS */

STATIC_PTR int NDECL(wipeoff);

#ifdef OVL0
STATIC_DCL int FDECL(menu_drop, (int));
#endif
#ifdef OVL2
STATIC_DCL int NDECL(currentlevel_rewrite);
STATIC_DCL void NDECL(final_level);
STATIC_DCL boolean NDECL(no_spirits);

/* static boolean FDECL(badspot, (XCHAR_P,XCHAR_P)); */
#endif

#ifdef OVLB

static NEARDATA const char drop_types[] =
	{ ALLOW_COUNT, COIN_CLASS, ALL_CLASSES, 0 };

/* 'd' command: drop one inventory item */
int
dodrop()
{
#ifndef GOLDOBJ
	int result, i = (invent || u.ugold) ? 0 : (SIZE(drop_types) - 1);
#else
	int result, i = (invent) ? 0 : (SIZE(drop_types) - 1);
#endif

	if (*u.ushops) sellobj_state(SELL_DELIBERATE);
	result = drop(getobj(&drop_types[i], "drop"));
	if (*u.ushops) sellobj_state(SELL_NORMAL);
	reset_occupations();

	if(result && roll_madness(MAD_TALONS)){
		You("panic after giving up a belonging!");
		HPanicking += 1+rnd(6);
	}
	
	return result ? MOVE_STANDARD : MOVE_CANCELLED;
}

#endif /* OVLB */
#ifdef OVL0

/* Called when a boulder is dropped, thrown, or pushed.  If it ends up
 * in a pool, it either fills the pool up or sinks away.  In either case,
 * it's gone for good...  If the destination is not a pool, returns FALSE.
 */
boolean
boulder_hits_pool(otmp, rx, ry, pushing)
struct obj *otmp;
register int rx, ry;
boolean pushing;
{
	if (!otmp || !is_boulder(otmp))
	    impossible("Not a boulder?");
	else if (!Is_waterlevel(&u.uz) && (is_pool(rx,ry, FALSE) || is_lava(rx,ry))) {
	    boolean lava = is_lava(rx,ry), cubewater = is_3dwater(rx, ry), fills_up;
	    const char *what = waterbody_name(rx,ry);
	    schar ltyp = levl[rx][ry].typ;
	    int chance = rn2(10);		/* water: 90%; lava: 10% */
	    fills_up = cubewater ? FALSE : lava ? chance == 0 : chance != 0;

	    if (fills_up) {
		struct trap *ttmp = t_at(rx, ry);

		if (ltyp == DRAWBRIDGE_UP) {
		    levl[rx][ry].drawbridgemask &= ~DB_UNDER; /* clear lava */
		    levl[rx][ry].drawbridgemask |= DB_FLOOR;
		} else
		    levl[rx][ry].typ = ROOM;

		if (ttmp) (void) delfloortrap(ttmp);
		bury_objs(rx, ry);
		
		newsym(rx,ry);
		if (pushing) {
		    You("push %s into the %s.", the(xname(otmp)), what);
		    if (flags.verbose && !Blind)
			pline("Now you can cross it!");
		    /* no splashing in this case */
		}
	    }
	    if (!fills_up || !pushing) {	/* splashing occurs */
		if (!u.uinwater) {
		    if (pushing ? !Blind : cansee(rx,ry)) {
			There("is a large splash as %s %s the %s.",
			      the(xname(otmp)), fills_up? "fills":"falls into",
			      what);
		    } else if (flags.soundok)
			You_hear("a%s splash.", lava ? " sizzling" : "");
		    wake_nearto_noisy(rx, ry, 40);
		}

		if (fills_up && u.uinwater && distu(rx,ry) == 0) {
		    u.uinwater = 0;
		    u.usubwater = 0;
		    docrt();
		    vision_full_recalc = 1;
		    You("find yourself on dry land again!");
		} else if (lava && distu(rx,ry) <= 2) {
		    You("are hit by molten lava%c",
			Fire_resistance ? '.' : '!');
			burn_away_slime();
			melt_frozen_air();
		    losehp(d((Fire_resistance ? 1 : 3), 6),
			   "molten lava", KILLED_BY);
		} else if (!fills_up && flags.verbose &&
			   (pushing ? !Blind : cansee(rx,ry)))
		    pline("It sinks without a trace!");
	    }

	    /* boulder is now gone */
	    if (pushing) delobj(otmp);
	    else obfree(otmp, (struct obj *)0);
	    return TRUE;
	}
	return FALSE;
}

/* Used for objects which sometimes do special things when dropped; must be
 * called with the object not in any chain.  Returns TRUE if the object goes
 * away.
 */
boolean
flooreffects(obj,x,y,verb)
struct obj *obj;
int x,y;
const char *verb;
{
	struct trap *t;
	struct monst *mtmp;

	if (obj->where != OBJ_FREE)
	    panic("flooreffects: obj not free");

	/* make sure things like water_damage() have no pointers to follow */
	obj->nobj = obj->nexthere = (struct obj *)0;

	if(In_quest(&u.uz) && urole.neminum == PM_BLIBDOOLPOOLP__GRAVEN_INTO_FLESH && levl[x][y].typ == AIR && obj != uball && obj != uchain){
		add_to_migration(obj);
		obj->ox = u.uz.dnum;
		obj->oy = qlocate_level.dlevel+1;
		obj->owornmask = (long)MIGR_RANDOM;
		newsym(x,y);
		return TRUE;
	}

	if (is_boulder(obj) && boulder_hits_pool(obj, x, y, FALSE))
		return TRUE;
	else if (is_boulder(obj) && (t = t_at(x,y)) != 0 &&
		 (t->ttyp==PIT || t->ttyp==SPIKED_PIT
			|| t->ttyp==TRAPDOOR || t->ttyp==HOLE)) {
		if (((mtmp = m_at(x, y)) && mtmp->mtrapped) ||
			(u.utrap && u.ux == x && u.uy == y)) {
		    if (*verb)
			pline_The("%s %s into the pit%s.",
				xname(obj),
				vtense((const char *)0, verb),
				(mtmp) ? "" : " with you");
		    if (mtmp) {
			if (!mon_resistance(mtmp,PASSES_WALLS) &&
				!throws_rocks(mtmp->data)) {
				if (hmon_general(&youmonst, mtmp, (struct attack *)0, (struct attack *)0, &obj, (struct obj *)0, HMON_PROJECTILE|HMON_FIRED, 0, 0, TRUE, rnd(20), FALSE, -1) != MM_DEF_DIED
					&& !is_whirly(mtmp->data))
				return FALSE;	/* still alive */
			}
			mtmp->mtrapped = 0;
		    } else {
			if (!Passes_walls && !throws_rocks(youracedata) && !(u.sealsActive&SEAL_YMIR)) {
			    losehp(rnd(15), "squished under a heavy object",
				   NO_KILLER_PREFIX);
			    return FALSE;	/* player remains trapped */
			} else u.utrap = 0;
		    }
		}
		if (*verb) {
			if (Blind) {
				if ((x == u.ux) && (y == u.uy))
					You_hear("a CRASH! beneath you.");
				else
					You_hear("the %s %s.", xname(obj), verb);
			} else if (cansee(x, y)) {
				pline_The("%s %s%s.",
					xname(obj),
				    t->tseen ? "" : "triggers and ",
				    t->ttyp == TRAPDOOR ? "plugs a trap door" :
				    t->ttyp == HOLE ? "plugs a hole" :
				    "fills a pit");
			}
		}
		deltrap(t);
		if(obj->otyp == MASS_OF_STUFF){
			place_object(obj, x, y);
			separate_mass_of_stuff(obj, FALSE);
			obj = (struct obj *) 0;
		}
		bury_objs(x, y); //Crate handling: Bury everything here (inc mass of stuff products) then free the boulder after
		if(obj && obj->otyp == MASSIVE_STONE_CRATE){
			struct obj *item;
			if(Blind) pline("Click!");
			else pline("The crate pops open as it lands.");
			/* drop any objects contained inside the crate */
			while ((item = obj->cobj) != 0) {
				obj_extract_self(item);
				place_object(item, x, y);
			}
		}
		if(obj) obfree(obj, (struct obj *)0);
		newsym(x,y);
		return TRUE;
	} else if (is_lava(x, y)) {
		return lava_damage(obj, x, y);
	} else if (is_pool(x, y, TRUE)) {
		/* Reasonably bulky objects (arbitrary) splash when dropped.
		 * If you're floating above the water even small things make noise.
		 * Stuff dropped near fountains always misses 
		 */
		if ((Blind || (Levitation || Flying)) && flags.soundok &&
		    ((x == u.ux) && (y == u.uy))) {
		    if (!Underwater) {
			if (weight(obj) > 9) {
				pline("Splash!");
		        } else if (Levitation || Flying) {
				pline("Plop!");
		        }
		    }
		    //map_background(x, y, 0);
		    newsym(x, y);
		}
		return water_damage(obj, FALSE, FALSE, FALSE, (struct monst *) 0);
	} else if (u.ux == x && u.uy == y &&
		(!u.utrap || u.utraptype != TT_PIT) &&
		(t = t_at(x,y)) != 0 && t->tseen &&
			(t->ttyp==PIT || t->ttyp==SPIKED_PIT)) {
		/* you escaped a pit and are standing on the precipice */
		if (Blind && flags.soundok)
			You_hear("%s %s downwards.",
				The(xname(obj)), otense(obj, "tumble"));
		else
			pline("%s %s into %s pit.",
				The(xname(obj)), otense(obj, "tumble"),
				the_your[t->madeby_u]);
	}
	if (is_lightsaber(obj) && litsaber(obj) && obj->oartifact != ART_INFINITY_S_MIRRORED_ARC && obj->otyp != KAMEREL_VAJRA) {
		if (cansee(x, y)) You("see %s deactivate.", an(xname(obj)));
		lightsaber_deactivate(obj, TRUE);
	}
	if (obj->oartifact == ART_HOLY_MOONLIGHT_SWORD && obj->lamplit) {
		if (cansee(x, y)) You("see %s go out.", an(xname(obj)));
		end_burn(obj, TRUE);
	}
	return FALSE;
}

#endif /* OVL0 */
#ifdef OVLB

void
doaltarobj(struct obj *obj, int god_index)  /* obj is an object dropped on an altar */
{
	if (Blind || Misotheism)
		return;

	/* Hunter "gods" are semi-atheistic philosophies */
	if (no_altar_index(god_index)){
		pline("%s %s on the altar.", Doname2(obj),
			otense(obj, "land"));
		return;
	}

	/* KMH, conduct */
	u.uconduct.gnostic++;

	if ((obj->blessed || obj->cursed) && obj->oclass != COIN_CLASS) {
		There("is %s flash as %s %s the altar.",
			an(hcolor(obj->blessed ? NH_AMBER : NH_BLACK)),
			doname(obj), otense(obj, "hit"));
		if (!Hallucination) obj->bknown = 1;
	} else {
		pline("%s %s on the altar.", Doname2(obj),
			otense(obj, "land"));
		obj->bknown = 1;
	}

	/* From NetHack4: colored flashes one level deep inside containers. */
	if (Has_contents(obj) && !obj->olocked) { /* && obj->cknown */
		int blessed = 0;
		int cursed = 0;
		struct obj * otmp;
		struct obj * nxto;
		for (otmp = obj->cobj; otmp; otmp = nxto) {
			nxto = otmp->nobj;
			if (otmp->oclass == COIN_CLASS) continue;
			if (otmp->blessed)
				blessed++;
			if (otmp->cursed)
				cursed++;
			if (!Hallucination && !otmp->bknown) {
				otmp->bknown = 1;
				obj_extract_self(otmp);
				add_to_container(obj, otmp);
			}
		}
		/* even when hallucinating, if you get no flashes at all, you know
		* everything's uncursed, so save the player the trouble of manually
		* naming them all */
		if (Hallucination && blessed + cursed == 0) {
			for (otmp = obj->cobj; otmp; otmp = nxto) {
				nxto = otmp->nobj;
				if (otmp->oclass == COIN_CLASS) continue;
				if (!otmp->bknown) {
					otmp->bknown = 1;
					obj_extract_self(otmp);
					add_to_container(obj, otmp);
				}
			}
		}
		if (blessed + cursed > 0) {
			const char* color;
			if (Hallucination) {
				color = "pretty multichromatic";
			}
			else if (blessed == 0) {
				color = hcolor(NH_BLACK);
			}
			else if (cursed == 0) {
				color = hcolor(NH_AMBER);
			}
			else {
				color = "colored";
			}

			pline("From inside %s, you see %s flash%s.",
				the(xname(obj)),
				(blessed + cursed == 1 ? an(color) : color),
				(blessed + cursed == 1 ? "" : "es"));
		}
	}
}

#ifdef SINKS
STATIC_OVL
void
trycall(obj)
register struct obj *obj;
{
	if(!objects[obj->otyp].oc_name_known &&
	   !objects[obj->otyp].oc_uname)
	   docall(obj);
}

STATIC_OVL
void
dosinkring(obj)  /* obj is a ring being dropped over a kitchen sink */
register struct obj *obj;
{
	register struct obj *otmp,*otmp2;
	register boolean ideed = TRUE;

	You("drop %s down the drain.", doname(obj));
	if(obj->oartifact){
		pline("But it seems to expand as if falls, and doesn't fit!");
		goto giveback;
	}
	obj->in_use = TRUE;	/* block free identification via interrupt */
	switch(obj->otyp) {	/* effects that can be noticed without eyes */
		case RIN_WISHES:
		if (obj->spe > 0)
		{
			struct monst * mtmp;
			if ((mtmp = makemon(&mons[PM_WERERAT], u.ux, u.uy, NO_MM_FLAGS)))
			{
				if (!Blind)
					You_hear("something from the pipes wish it was a real boy, and %s scuttles out of the sink!", a_monnam(mtmp));
				else
					You_hear("something from the pipes wish it was a real boy, and then scuttling noises!");
				obj->spe--;
				mtmp->mpeaceful = TRUE;
				newsym(mtmp->mx, mtmp->my);
				if (obj->spe > 0)
				{
					pline("It leaves the ring in the sink.");
					goto giveback;
				}
			}
			You("wish you hadn't done that.");
		}
		else You("kinda wish you hadn't done that.");
		break;
	    case RIN_SEARCHING:
		You("thought your %s got lost in the sink, but there it is!",
			xname(obj));
		goto giveback;
	    case RIN_SLOW_DIGESTION:
		pline_The("ring is regurgitated!");
giveback:
		obj->in_use = FALSE;
		dropx(obj);
		trycall(obj);
		return;
	    case RIN_LEVITATION:
		pline_The("sink quivers upward for a moment.");
		break;
	    case RIN_POISON_RESISTANCE:
		You("smell rotten %s.", makeplural(fruitname(FALSE)));
		break;
	    case RIN_AGGRAVATE_MONSTER:
		pline("Several flies buzz angrily around the sink.");
		break;
	    case RIN_SHOCK_RESISTANCE:
		pline("Static electricity surrounds the sink.");
		break;
	    case RIN_CONFLICT:
		You_hear("loud noises coming from the drain.");
		break;
	    case RIN_ALACRITY:
		pline_The("water flow seems faster now.");
		break;
	    case RIN_SUSTAIN_ABILITY:	/* KMH */
		pline_The("water flow seems fixed.");
		break;
	    case RIN_GAIN_STRENGTH:
		pline_The("water flow seems %ser now.",
			(obj->spe<0) ? "weak" : "strong");
		break;
	    case RIN_GAIN_CONSTITUTION:
		pline_The("water flow seems %ser now.",
			(obj->spe<0) ? "less" : "great");
		break;
	    case RIN_INCREASE_ACCURACY:	/* KMH */
		pline_The("water flow %s the drain.",
			(obj->spe<0) ? "misses" : "hits");
		break;
	    case RIN_INCREASE_DAMAGE:
		pline_The("water's force seems %ser now.",
			(obj->spe<0) ? "small" : "great");
		break;
	    case RIN_HUNGER:
		ideed = FALSE;
		for(otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp2) {
		    otmp2 = otmp->nexthere;
		    if (otmp != uball && otmp != uchain &&
			    !obj_resists(otmp, 0, 99)) {
			if (!Blind) {
			    pline("Suddenly, %s %s from the sink!",
				  doname(otmp), otense(otmp, "vanish"));
			    ideed = TRUE;
			}
			delobj(otmp);
		    }
		}
		break;
	    case MEAT_RING:
		/* Not the same as aggravate monster; besides, it's obvious. */
		pline("Several flies buzz around the sink.");
		break;
	    default:
		ideed = FALSE;
		break;
	}
	if(!Blind && !ideed && obj->otyp != RIN_HUNGER) {
	    ideed = TRUE;
	    switch(obj->otyp) {		/* effects that need eyes */
		case RIN_ADORNMENT:
		    pline_The("faucets flash brightly for a moment.");
		    break;
		case RIN_REGENERATION:
		    pline_The("sink looks as good as new.");
		    break;
		case RIN_INVISIBILITY:
		    You("don't see anything happen to the sink.");
		    break;
		case RIN_FREE_ACTION:
		    You("see the ring slide right down the drain!");
		    break;
		case RIN_SEE_INVISIBLE:
		    You("see some air in the sink.");
		    break;
		case RIN_STEALTH:
		pline_The("sink seems to blend into the floor for a moment.");
		    break;
		case RIN_FIRE_RESISTANCE:
		pline_The("hot water faucet flashes brightly for a moment.");
		    break;
		case RIN_COLD_RESISTANCE:
		pline_The("cold water faucet flashes brightly for a moment.");
		    break;
		case RIN_PROTECTION_FROM_SHAPE_CHAN:
		    pline_The("sink looks nothing like a fountain.");
		    break;
		case RIN_PROTECTION:
		    pline_The("sink glows %s for a moment.",
			    hcolor((obj->spe<0) ? NH_BLACK : NH_SILVER));
		    break;
		case RIN_WARNING:
		    pline_The("sink glows %s for a moment.", hcolor(NH_WHITE));
		    break;
		case RIN_TELEPORTATION:
		    pline_The("sink momentarily vanishes.");
		    break;
		case RIN_TELEPORT_CONTROL:
	    pline_The("sink looks like it is being beamed aboard somewhere.");
		    break;
		case RIN_POLYMORPH:
		    pline_The("sink momentarily looks like a fountain.");
		    break;
		case RIN_POLYMORPH_CONTROL:
	pline_The("sink momentarily looks like a regularly erupting geyser.");
		    break;
	    }
	}
	if(ideed)
	    trycall(obj);
	else
	    You_hear("the ring bouncing down the drainpipe.");
	if (!rn2(20)) {
		pline_The("sink backs up, leaving %s.", doname(obj));
		obj->in_use = FALSE;
		dropx(obj);
	} else
		useup(obj);
}
#endif

#endif /* OVLB */
#ifdef OVL0

/* some common tests when trying to drop or throw items */
boolean
canletgo(obj,word)
register struct obj *obj;
register const char *word;
{
	if(obj->owornmask & (W_ARMOR | W_RING | W_AMUL | W_TOOL | W_BELT)){
		if (*word)
			Norep("You cannot %s %s you are wearing.",word,
				something);
		return(FALSE);
	}
	if (obj->otyp == LOADSTONE && obj->cursed) {
		/* getobj() kludge sets corpsenm to user's specified count
		   when refusing to split a stack of cursed loadstones */
		if (*word) {
			/* getobj() ignores a count for throwing since that is
			   implicitly forced to be 1; replicate its kludge... */
			if (!strcmp(word, "throw") && obj->quan > 1L)
			    obj->corpsenm = 1;
			pline("For some reason, you cannot %s%s the stone%s!",
			      word, obj->corpsenm ? " any of" : "",
			      plur(obj->quan));
		}
		obj->corpsenm = 0;		/* reset */
		obj->bknown = 1;
		return(FALSE);
	}
	if (obj->otyp == LEASH && obj->leashmon != 0) {
		if (*word)
			pline_The("leash is tied around your %s.",
					body_part(HAND));
		return(FALSE);
	}
#ifdef STEED
	if (obj->owornmask & W_SADDLE) {
		if (*word)
			You("cannot %s %s you are sitting on.", word,
				something);
		return (FALSE);
	}
#endif
	return(TRUE);
}


int
drop(obj)
struct obj *obj;
{
	if(!obj) return(0);
	if(!canletgo(obj,"drop"))
		return(0);
	if(obj == uwep) {
		if(welded(uwep)) {
			weldmsg(obj);
			return(0);
		}
		setuwep((struct obj *)0);
	}
	if(obj == uquiver) {
		setuqwep((struct obj *)0);
	}
	if (obj == uswapwep) {
		setuswapwep((struct obj *)0);
	}

	if (u.uswallow) {
		/* barrier between you and the floor */
		if(flags.verbose)
		{
			char buf[BUFSZ];

			/* doname can call s_suffix, reusing its buffer */
			Strcpy(buf, s_suffix(mon_nam(u.ustuck)));
			You("drop %s into %s %s.", doname(obj), buf,
				mbodypart(u.ustuck, STOMACH));
		}
	} else {
#ifdef SINKS
	    if((obj->oclass == RING_CLASS || obj->otyp == MEAT_RING) &&
			IS_SINK(levl[u.ux][u.uy].typ)) {
		dosinkring(obj);
		return(1);
	    }
#endif
	    if (!can_reach_floor()) {
		if(flags.verbose) You("drop %s.", doname(obj));
#ifndef GOLDOBJ
		if (obj->oclass != COIN_CLASS || obj == invent) freeinv(obj);
#else
		/* Ensure update when we drop gold objects */
		if (obj->oclass == COIN_CLASS) flags.botl = 1;
		freeinv(obj);
#endif
		bhitpos.x = u.ux; bhitpos.y = u.uy;
		obj->ox = u.ux; obj->oy = u.uy;
		hitfloor2(&youmonst, &obj, (struct obj *)0, FALSE, FALSE);
		return(1);
	    }
	    if (!IS_ALTAR(levl[u.ux][u.uy].typ) && flags.verbose)
		You("drop %s.", doname(obj));
	}
	dropx(obj);
	return(1);
}

/* Called in several places - may produce output */
/* eg ship_object() and dropy() -> sellobj() both produce output */
void
dropx(obj)
register struct obj *obj;
{
#ifndef GOLDOBJ
	if (obj->oclass != COIN_CLASS || obj == invent) freeinv(obj);
#else
        /* Ensure update when we drop gold objects */
        if (obj->oclass == COIN_CLASS) flags.botl = 1;
        freeinv(obj);
#endif
	if (!u.uswallow) {
	    if (ship_object(obj, u.ux, u.uy, FALSE)) return;
	    if (IS_ALTAR(levl[u.ux][u.uy].typ))
			doaltarobj(obj, god_at_altar(u.ux,u.uy)); /* set bknown */
	}
	dropy(obj);
}

void
dropy(obj)
register struct obj *obj;
{
	if (obj == uwep) setuwep((struct obj *)0);
	if (obj == uquiver) setuqwep((struct obj *)0);
	if (obj == uswapwep) setuswapwep((struct obj *)0);

	if (!u.uswallow && flooreffects(obj,u.ux,u.uy,"drop")) return;
	/* uswallow check done by GAN 01/29/87 */
	if(u.uswallow) {
	    boolean could_petrify = FALSE;
	    boolean could_poly = FALSE;
	    boolean could_slime = FALSE;
	    boolean could_grow = FALSE;
	    boolean could_heal = FALSE;

	    if (obj != uball) {		/* mon doesn't pick up ball */
		if (obj->otyp == CORPSE) {
		    could_petrify = touch_petrifies(&mons[obj->corpsenm]);
		    could_poly = polyfodder(obj) && !resists_poly(u.ustuck->data);
		    could_slime = (obj->corpsenm == PM_GREEN_SLIME || obj->corpsenm == PM_FLUX_SLIME) &&  !Slime_res(u.ustuck);
		    could_grow = (obj->corpsenm == PM_WRAITH);
		    could_heal = (obj->corpsenm == PM_NURSE);
		}
		(void) mpickobj(u.ustuck,obj);
		if (is_animal(u.ustuck->data)) {
		    if (could_poly || could_slime) {
			(void) newcham(u.ustuck,
				       could_poly ? NON_PM : PM_GREEN_SLIME,
				       FALSE, could_slime);
			delobj(obj);	/* corpse is digested */
		    } else if (could_petrify) {
			minstapetrify(u.ustuck, TRUE);
			/* Don't leave a cockatrice corpse in a statue */
			if (!u.uswallow) delobj(obj);
		    } else if (could_grow) {
			(void) grow_up(u.ustuck, (struct monst *)0);
			delobj(obj);	/* corpse is digested */
		    } else if (could_heal) {
			u.ustuck->mhp = u.ustuck->mhpmax;
			delobj(obj);	/* corpse is digested */
		    }
		}
	    }
	} else  {
	    place_object(obj, u.ux, u.uy);
	    if (obj == uball)
		drop_ball(u.ux,u.uy);
	    else
		sellobj(obj, u.ux, u.uy);
	    stackobj(obj);
	    if(Blind && Levitation)
		map_object(obj, 0);
	    newsym(u.ux,u.uy);	/* remap location under self */
	}
}

/* things that must change when not held; recurse into containers.
   Called for both player and monsters */
void
obj_no_longer_held(obj)
struct obj *obj;
{
	if (!obj) {
	    return;
	} else if ((Is_container(obj) || obj->otyp == STATUE) && obj->cobj) {
	    struct obj *contents;
	    for(contents=obj->cobj; contents; contents=contents->nobj)
		obj_no_longer_held(contents);
	}
	switch(obj->otyp) {
	case CRYSKNIFE:
	    /* KMH -- Fixed crysknives have only 10% chance of reverting */
	    /* only changes when not held by player or monster */
	    if (!obj->oerodeproof || !rn2(10)) {
		obj->otyp = WORM_TOOTH;
		obj->oerodeproof = 0;
	    }
	    break;
	case CRYSTAL_SKULL:{
		struct monst *nmon;
		for(struct monst *mtmp = fmon; mtmp; mtmp = nmon){
			nmon = mtmp->nmon;
			if(!DEADMONSTER(mtmp) && get_mx(mtmp, MX_ESUM)){
				if(mtmp->mextra_p->esum_p->sm_o_id == obj->o_id){
					update_skull_mon(mtmp, obj);
					if(!get_mx(mtmp, MX_ESUM))
						impossible("Non-summoned skull monster in obj_no_longer_held");
					else {
						int dur = timer_duration_remaining(get_timer(mtmp->timed, DESUMMON_MON));
						mtmp->mextra_p->esum_p->permanent = 0;
						abjure_summon(mtmp, dur);
					}
				}
			}
		}
		}break;
	}
}

boolean
obj_summon_out(obj)
struct obj *obj;
{
	for(struct monst *mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if(get_mx(mtmp, MX_ESUM))
			if(mtmp->mextra_p->esum_p->sm_o_id == obj->o_id)
				return TRUE;
	//Else
	return FALSE;
}

/* 'D' command: drop several things */
int
doddrop()
{
	int result = 0;

	add_valid_menu_class(0); /* clear any classes already there */
	if (*u.ushops) sellobj_state(SELL_DELIBERATE);
	if (flags.menu_style != MENU_TRADITIONAL ||
		(result = ggetobj("drop", drop, 0, FALSE, (unsigned *)0)) < -1)
	    result = menu_drop(result);
	if (*u.ushops) sellobj_state(SELL_NORMAL);
	reset_occupations();

	if(result && roll_madness(MAD_TALONS)){
		You("panic after giving up your property!");
		HPanicking += 1+rnd(6);
	}
	return result ? MOVE_STANDARD : MOVE_CANCELLED;
}

/* Drop things from the hero's inventory, using a menu. */
STATIC_OVL int
menu_drop(retry)
int retry;
{
    int n, i, n_dropped = 0;
    long cnt;
    struct obj *otmp, *otmp2;
#ifndef GOLDOBJ
    struct obj *u_gold = 0;
#endif
    menu_item *pick_list;
    boolean all_categories = TRUE;
    boolean drop_everything = FALSE;

#ifndef GOLDOBJ
    if (u.ugold) {
	/* Hack: gold is not in the inventory, so make a gold object
	   and put it at the head of the inventory list. */
	u_gold = mkgoldobj(u.ugold);	/* removes from u.ugold */
	u_gold->in_use = TRUE;
	u.ugold = u_gold->quan;		/* put the gold back */
	assigninvlet(u_gold);		/* might end up as NOINVSYM */
	u_gold->nobj = invent;
	invent = u_gold;
    }
#endif
    if (retry) {
	all_categories = (retry == -2);
    } else if (flags.menu_style == MENU_FULL) {
	all_categories = FALSE;
	n = query_category("Drop what type of items?",
			invent,
			UNPAID_TYPES | ALL_TYPES | CHOOSE_ALL |
			BUC_BLESSED | BUC_CURSED | BUC_UNCURSED | BUC_UNKNOWN,
			&pick_list, PICK_ANY);
	if (!n) goto drop_done;
	for (i = 0; i < n; i++) {
	    if (pick_list[i].item.a_int == ALL_TYPES_SELECTED)
		all_categories = TRUE;
	    else if (pick_list[i].item.a_int == 'A')
		drop_everything = TRUE;
	    else
		add_valid_menu_class(pick_list[i].item.a_int);
	}
	free((genericptr_t) pick_list);
    } else if (flags.menu_style == MENU_COMBINATION) {
	unsigned ggoresults = 0;
	all_categories = FALSE;
	/* Gather valid classes via traditional NetHack method */
	i = ggetobj("drop", drop, 0, TRUE, &ggoresults);
	if (i == -2) all_categories = TRUE;
	if (ggoresults & ALL_FINISHED) {
		n_dropped = i;
		goto drop_done;
	}
    }

    if (drop_everything) {
	for(otmp = invent; otmp; otmp = otmp2) {
	    otmp2 = otmp->nobj;
	    n_dropped += drop(otmp);
	}
    } else {
	/* should coordinate with perm invent, maybe not show worn items */
	n = query_objlist("What would you like to drop?", invent,
			USE_INVLET|INVORDER_SORT, &pick_list,
			PICK_ANY, all_categories ? allow_all : allow_category);
	if (n > 0) {
	    for (i = 0; i < n; i++) {
		otmp = pick_list[i].item.a_obj;
		cnt = pick_list[i].count;
		if (cnt < otmp->quan) {
		    if (welded(otmp)) {
			;	/* don't split */
		    } else if (otmp->otyp == LOADSTONE && otmp->cursed) {
			/* same kludge as getobj(), for canletgo()'s use */
			otmp->corpsenm = (int) cnt;	/* don't split */
		    } else {
#ifndef GOLDOBJ
			if (otmp->oclass == COIN_CLASS)
			    (void) splitobj(otmp, otmp->quan - cnt);
			else
#endif
			    otmp = splitobj(otmp, cnt);
		    }
		}
		n_dropped += drop(otmp);
	    }
	    free((genericptr_t) pick_list);
	}
    }

 drop_done:
#ifndef GOLDOBJ
    if (u_gold && invent && invent->oclass == COIN_CLASS) {
	/* didn't drop [all of] it */
	u_gold = invent;
	invent = u_gold->nobj;
	u_gold->in_use = FALSE;
	dealloc_obj(u_gold);
	update_inventory();
    }
#endif
    return n_dropped;
}

#endif /* OVL0 */
#ifdef OVL2

enum AcuItemsCheck {
	ACU_MISSING_STAFF = (1 << 0),
	ACU_MISSING_FLUID = (1 << 1),
};

/* on a ladder, used in goto_level */
static NEARDATA boolean at_ladder = FALSE;

int
dodown()
{
	struct trap *trap = 0;
	boolean stairs_down = ((u.ux == xdnstair && u.uy == ydnstair) ||
		    (u.ux == sstairs.sx && u.uy == sstairs.sy && !sstairs.up)),
		ladder_down = (u.ux == xdnladder && u.uy == ydnladder);

#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove) {
		pline("%s won't move!", Monnam(u.usteed));
		return MOVE_CANCELLED;
	} else
#endif
	if (Levitation) {
	    if ((HLevitation & I_SPECIAL) || (ELevitation & W_ARTI)) {
		/* end controlled levitation */
		if (ELevitation & W_ARTI) {
		    struct obj *obj;

		    for(obj = invent; obj; obj = obj->nobj) {
			if (obj->oartifact &&
					artifact_has_invprop(obj,LEVITATION)) {
			    if (obj->age < monstermoves)
				obj->age = monstermoves + rnz(100);
			    else
				obj->age += rnz(100);
			}
		    }
		}
		if (float_down(I_SPECIAL|TIMEOUT, W_ARTI))
		    return MOVE_STANDARD;   /* came down, so moved */
	    }
	    floating_above(stairs_down ? "stairs" : ladder_down ?
			   "ladder" : surface(u.ux, u.uy));
	    return MOVE_CANCELLED;   /* didn't move */
	}
	if (!stairs_down && !ladder_down) {
		if (!(trap = t_at(u.ux,u.uy)) ||
			(trap->ttyp != TRAPDOOR && trap->ttyp != HOLE)
			|| !Can_fall_thru(&u.uz) || !trap->tseen) {

			if (flags.autodig && !flags.nopick &&
				uwep && (is_pick(uwep) || (is_lightsaber(uwep) && litsaber(uwep)) || (uwep->otyp == SEISMIC_HAMMER))) {
				return use_pick_axe2(uwep);
			} else if(uarmg && is_pick(uarmg)){
				return use_pick_axe2(uarmg);
			} else {
				if(levl[u.ux][u.uy].typ == STAIRS){
					if (Is_hell3(&u.uz) && !(u.ux == xupstair && u.uy == yupstair)){
						pline("These stairs are fake!");
						levl[u.ux][u.uy].typ = ROOM;
						newsym(u.ux, u.uy);
					} else {
						if(levl[u.ux][u.uy].ladder != LA_DOWN){
							pline("These stairs don't go down!");
						}
						else {
							pline("These stairs have been blocked by rubble!");
							levl[u.ux][u.uy].typ = ROOM;
							newsym(u.ux, u.uy);
						}
					}
				}
				else You_cant("go down here.");
				return MOVE_CANCELLED;
			}
		}
	}
	if(u.ustuck && (u.uswallow || !sticks(&youmonst))) {
		You("are %s, and cannot go down.",
			!u.uswallow ? "being held" : is_animal(u.ustuck->data) ?
			"swallowed" : "engulfed");
		return MOVE_STANDARD;
	}
	if(u.veil && Is_sumall(&u.uz)){
		You("are standing at the head of a strangely-angled staircase.");
		You("feel reality threatening to slip away!");
		if (yn("Are you sure you want to descend?") != 'y')
			return MOVE_CANCELLED;
		else pline("So be it.");
		u.veil = FALSE;
		change_uinsight(1);
	}
	if (on_level(&valley_level, &u.uz) && !u.uevent.gehennom_entered) {
		You("are standing at the gate to Gehennom.");
		pline("Unspeakable cruelty and harm lurk down there.");
		if (yn("Are you sure you want to enter?") != 'y')
			return(0);
		else pline("So be it.");
		u.uevent.gehennom_entered = 1;	/* don't ask again */
#ifdef RECORD_ACHIEVE
		achieve.enter_gehennom = 1;
#endif
	}
	if(on_level(&spire_level,&u.uz)){
		u.uevent.sum_entered = 1; //entered sum of all
	}
	if(!next_to_u()) {
		You("are held back by your pet!");
		return MOVE_CANCELLED;
	}

	if (trap)
	    You("%s %s.", locomotion(&youmonst, "jump"),
		trap->ttyp == HOLE ? "down the hole" : "through the trap door");

	if (trap && Is_stronghold(&u.uz)) {
		goto_hell(FALSE, TRUE);
	} else {
		at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
		next_level(!trap);
		at_ladder = FALSE;
	}
	return MOVE_MOVED;
}

int
doup()
{
	if( (u.ux != xupstair || u.uy != yupstair)
	     && (!xupladder || u.ux != xupladder || u.uy != yupladder)
	     && (!sstairs.sx || u.ux != sstairs.sx || u.uy != sstairs.sy
			|| !sstairs.up)
		 && !(Role_if(PM_RANGER) && Race_if(PM_GNOME) && Is_qstart(&u.uz) && levl[u.ux][u.uy].ladder == LA_UP)
	) {
		if(uwep && uwep->oartifact == ART_ROD_OF_SEVEN_PARTS && artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights > 0){
			struct obj *pseudo;
			pseudo = mksobj(SPE_LEVITATION, MKOBJ_NOINIT);
			pseudo->blessed = pseudo->cursed = 0;
			pseudo->blessed = TRUE;
			pseudo->quan = 23L;			/* do not let useup get it */
			(void) peffects(pseudo, TRUE);
			(void) peffects(pseudo, TRUE);
			(void) peffects(pseudo, TRUE);
			obfree(pseudo, (struct obj *)0);	/* now, get rid of it */
			artinstance[ART_ROD_OF_SEVEN_PARTS].RoSPflights--;
		}
		else{
			if(levl[u.ux][u.uy].typ == STAIRS){
				if(levl[u.ux][u.uy].ladder != LA_UP){
					pline("These stairs don't go up!");
				}
				else {
					pline("These stairs have been blocked by rubble!");
					levl[u.ux][u.uy].typ = ROOM;
					newsym(u.ux, u.uy);
				}
			}
			else You_cant("go up here.");
		}
		return MOVE_CANCELLED;
	}
#ifdef STEED
	if (u.usteed && !u.usteed->mcanmove) {
		pline("%s won't move!", Monnam(u.usteed));
		return MOVE_CANCELLED;
	} else
#endif
	if(u.ustuck && (u.uswallow || !sticks(&youmonst))) {
		You("are %s, and cannot go up.",
			!u.uswallow ? "being held" : is_animal(u.ustuck->data) ?
			"swallowed" : "engulfed");
		return MOVE_STANDARD;
	}
	if(near_capacity() > SLT_ENCUMBER) {
		/* No levitation check; inv_weight() already allows for it */
		Your("load is too heavy to climb the %s.",
			levl[u.ux][u.uy].typ == STAIRS ? "stairs" : "ladder");
		return MOVE_STANDARD;
	}
	if(ledger_no(&u.uz) == 1) {
		if (iflags.debug_fuzzer)
			return MOVE_CANCELLED;
		if(!Role_if(PM_ANACHRONOUNBINDER)){
			if (yn("Beware, there will be no return! Still climb?") != 'y')
				return MOVE_CANCELLED;
		} else {
			int missing_items = acu_asc_items_check();
			if (yn("Beware, there will be no return! Still climb?") != 'y')
				return MOVE_CANCELLED;
			else if(u.uhave.amulet && missing_items){
				acu_asc_items_warning(missing_items);
				return MOVE_CANCELLED;
			}
		}
		if(Role_if(PM_UNDEAD_HUNTER) && u.uevent.udemigod){
			if(u.veil){
				if(philosophy_index(u.ualign.god)
				 && yesno("You feel that there is a deeper truth still to be uncovered here. Still climb?", iflags.paranoid_quit) != 'y'
				){
					return MOVE_CANCELLED;
				}
			}
			else {
				if (!quest_status.moon_close && yesno("You have the nagging feeling you have incomplete buisness here. Still climb?", iflags.paranoid_quit) != 'y')
					return MOVE_CANCELLED;
				if (philosophy_index(u.ualign.god)
				 && research_incomplete()
				 && yesno("You worry that you have not completed your research! Still climb?", iflags.paranoid_quit) != 'y'
				)
					return MOVE_CANCELLED;
			}
		}
	}
	if(!next_to_u()) {
		You("are held back by your pet!");
		return MOVE_CANCELLED;
	}
	at_ladder = (boolean) (levl[u.ux][u.uy].typ == LADDER);
	prev_level(TRUE);
	at_ladder = FALSE;
	return MOVE_MOVED;
}

/*
* Disclaimer, I do not endorse this code or know if it works and I refuse to read it so we are sticking with it.
*/
int
acu_asc_items_check()
{
	struct obj *otmp;
	int missing_items = ACU_MISSING_STAFF | ACU_MISSING_FLUID;
	for(otmp = invent; otmp; otmp=otmp->nobj){
		if(otmp->oartifact == ART_ILLITHID_STAFF){
			if(otmp->cobj->oartifact == ART_ELDER_CEREBRAL_FLUID) {
				missing_items &= ~ACU_MISSING_FLUID;
			}
			missing_items &= ~ACU_MISSING_STAFF;
		} else if(otmp->oartifact == ART_ELDER_CEREBRAL_FLUID){
			missing_items &= ~ACU_MISSING_FLUID;
		}
	}
	return missing_items;
}

void
acu_asc_items_warning(int missing_items)
{
	if (!missing_items) {
		impossible("warning about not-missing acu items");
		return;
	}
	You("require the %s.", missing_items == ACU_MISSING_STAFF
						? "Illithid Staff"
						: missing_items == ACU_MISSING_FLUID
							? "Elder Cerebral Fluid"
							: "Illithid Staff and the Elder Cerebral Fluid");

}

d_level save_dlevel = {0, 0};

/* check that we can write out the current level */
STATIC_OVL int
currentlevel_rewrite()
{
	register int fd;
	char whynot[BUFSZ];

	/* since level change might be a bit slow, flush any buffered screen
	 *  output (like "you fall through a trap door") */
	mark_synch();

	fd = create_levelfile(ledger_no(&u.uz), whynot);
	if (fd < 0) {
		/*
		 * This is not quite impossible: e.g., we may have
		 * exceeded our quota. If that is the case then we
		 * cannot leave this level, and cannot save either.
		 * Another possibility is that the directory was not
		 * writable.
		 */
		pline("%s", whynot);
		return -1;
	}

#ifdef MFLOPPY
	if (!savelev(fd, ledger_no(&u.uz), COUNT_SAVE)) {
		(void) close(fd);
		delete_levelfile(ledger_no(&u.uz));
		pline("NetHack is out of disk space for making levels!");
		You("can save, quit, or continue playing.");
		return -1;
	}
#endif
	return fd;
}

#ifdef INSURANCE
void
save_currentstate()
{
	int fd;

	if (flags.ins_chkpt) {
		/* write out just-attained level, with pets and everything */
		fd = currentlevel_rewrite();
		if(fd < 0) return;
		bufon(fd);
		savelev(fd,ledger_no(&u.uz), WRITE_SAVE);
		bclose(fd);
	}

	/* write out non-level state */
	savestateinlock();
}
#endif

/*
static boolean
badspot(x, y)
register xchar x, y;
{
	return((levl[x][y].typ != ROOM && levl[x][y].typ != AIR &&
			 levl[x][y].typ != CORR) || MON_AT(x, y));
}
*/

void
goto_level(newlevel, at_stairs, falling, portal)
d_level *newlevel;
boolean at_stairs, falling;
int portal;
{
	int fd, l_idx;
	int new_ledger;
	boolean cant_go_back,
		up = (depth(newlevel) < depth(&u.uz)),
		newdungeon = (u.uz.dnum != newlevel->dnum),
		was_in_W_tower = In_W_tower(u.ux, u.uy, &u.uz),
		familiar = FALSE;
	boolean new = FALSE;	/* made a new level? */
	struct monst *mtmp;
	struct obj *obj;
	char whynot[BUFSZ];
	if(Is_nowhere(&u.uz) && !flags.phasing) return;
	if(In_adventure_branch(&u.uz) && In_tower(newlevel)) up = TRUE;
	if(In_adventure_branch(newlevel) && In_tower(&u.uz)) up = FALSE;

	if (dunlev(newlevel) > dunlevs_in_dungeon(newlevel))
		newlevel->dlevel = dunlevs_in_dungeon(newlevel);
	if (newdungeon && In_endgame(newlevel)) { /* 1st Endgame Level !!! */
	    if (u.uhave.amulet) {
			livelog_write_string("entered the Planes");
			assign_level(newlevel, &earth_level);
	    } else return;
	}
	new_ledger = ledger_no(newlevel);
	if (new_ledger <= 0)
		done(ESCAPED);	/* in fact < 0 is impossible */

	/* If you have the amulet and are trying to get out of Gehennom, going
	 * up a set of stairs sometimes does some very strange things!
	 * Biased against law and towards chaos, but not nearly as strongly
	 * as it used to be (prior to 3.2.0).
	 * Odds:	    old				    nethack				         dnethack
	 *	"up"    L      N      C		"up"    L      N      C		"up"    L      N      C 
	 *	 +1   75.0   75.0   75.0	 +1   75.0   75.0   75.0	 +1   66.66  66.66  66.6
	 *	  0    0.0   12.5   25.0	  0    6.25   8.33  12.5	  0    8.33  11.11  16.6
	 *	 -1    8.33   4.17   0.0	 -1    6.25   8.33  12.5	 -1    8.33  11.11  16.6
	 *	 -2    8.33   4.17   0.0	 -2    6.25   8.33   0.0	 -2    8.33  11.11   0.0
	 *	 -3    8.33   4.17   0.0	 -3    6.25   0.0    0.0	 -3    8.33   0.0    0.0
	 */
	if (Inhell && up && u.uhave.amulet && !newdungeon && !portal &&
				(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz)-3) &&
				(u.uz.dlevel < wiz1_level.dlevel) &&
				(u.uz.dlevel > valley_level.dlevel) ) {
		if (!rn2(3)) {
		    int odds = 3 + (int)u.ualign.type,		/* 2..4 */
			diff = odds <= 1 ? 0 : rn2(odds);	/* paranoia */

		    if (diff != 0) {
			assign_rnd_level(newlevel, &u.uz, diff);
			/* if inside the tower, stay inside */
			if (was_in_W_tower &&
			    !On_W_tower_level(newlevel)) diff = 0;
		    }
		    if (diff == 0)
			assign_level(newlevel, &u.uz);

		    new_ledger = ledger_no(newlevel);

		    pline("A mysterious force momentarily surrounds you...");
		    if (on_level(newlevel, &u.uz)) {
			(void) safe_teleds(FALSE);
			(void) next_to_u();
			return;
		    } else
			at_stairs = at_ladder = FALSE;
		}
	}
	/* Prevent the player from going past the first quest level unless
	 * (s)he has been given the go-ahead by the leader.
	 */
	if ((!up && Is_qhome(&u.uz) && !newdungeon && !ok_to_quest() && !flags.stag)
	&& !(Race_if(PM_HALF_DRAGON) && Role_if(PM_NOBLEMAN) && flags.initgend)
	) {
		pline("A mysterious force prevents you from descending.");
		return;
	}

	if (In_void(&u.uz) && (!no_spirits() || !u.uhave.amulet)) {
		pline("A mysterious force prevents you from %s.",up?"ascending":"descending");
		if(!no_spirits()){
			pline("There is too much life here.");
		}
		return;
	}
	/* Mysterious force to shake up the uh quest*/
	if(!up && !newdungeon && !portal && In_quest(&u.uz) 
		&& Role_if(PM_UNDEAD_HUNTER) && !mvitals[PM_MOON_S_CHOSEN].died
		&& dunlev(&u.uz) < qlocate_level.dlevel
		&& rnd(20) < Insight && rn2(2)
	){
		int diff = rn2(2);	/* 0 - 1 */
		if (diff != 0) {
			assign_rnd_level(newlevel, &u.uz, diff);
		}
		else {
			assign_level(newlevel, &u.uz);
		    new_ledger = ledger_no(newlevel);

		    pline("A mysterious force momentarily surrounds you...");
		    if (on_level(newlevel, &u.uz)) {
				(void) safe_teleds(FALSE);
				(void) next_to_u();
				return;
		    } else
				at_stairs = at_ladder = FALSE;
		}
	}
	// if (on_level(&u.uz, &nemesis_level) && !(quest_status.got_quest) && flags.stag) {
		// pline("A mysterious force prevents you from leaving.");
		// return;
	// }
	if (In_quest(&u.uz) && Race_if(PM_DWARF) &&  !up &&
		urole.neminum == PM_BOLG && Is_qlocate(&u.uz) && 
		!((mvitals[PM_SMAUG].mvflags & G_GENOD && !In_quest(&u.uz)) || mvitals[PM_SMAUG].died > 0)
	) {
		pline("A mysterious force prevents you from descending.");
		return;
	}

	if (on_level(newlevel, &u.uz)) return;		/* this can happen */

	fd = currentlevel_rewrite();
	if (fd < 0) return;

	if (falling) /* assuming this is only trap door or hole */
	    impact_drop((struct obj *)0, u.ux, u.uy, newlevel->dlevel, TRUE);

	check_special_room(TRUE);		/* probably was a trap door */
	if (Punished) unplacebc();
	u.utrap = 0;				/* needed in level_tele */
	fill_pit(u.ux, u.uy);
	u.ustuck = 0;				/* idem */
	u.uinwater = 0;
	u.usubwater = 0;
	u.uundetected = 0;	/* not hidden, even if means are available */
	u.uz.flags.mirror = 0; /*Level has a mirror on it (needed for Nudzirath) */
	for(obj = fobj; obj; obj = obj->nobj){
		if(obj->otyp == MIRROR)
			u.uz.flags.mirror = 1;
	}
	if(!Is_nowhere(newlevel)) keepdogs(FALSE, newlevel, portal);
	u.ux = u.uy = 0;			/* comes after keepdogs() */
	
	if (u.uswallow)				/* idem */
		u.uswldtim = u.uswallow = 0;
	recalc_mapseen(); /* recalculate map overview before we leave the level */
	/*
	 *  We no longer see anything on the level.  Make sure that this
	 *  follows u.uswallow set to null since uswallow overrides all
	 *  normal vision.
	 */
	vision_recalc(2);

	/*
	 * Save the level we're leaving.  If we're entering the endgame,
	 * we can get rid of all existing levels because they cannot be
	 * reached any more.  We still need to use savelev()'s cleanup
	 * for the level being left, to recover dynamic memory in use and
	 * to avoid dangling timers and light sources.
	 */
	cant_go_back = (newdungeon && In_endgame(newlevel));
	if (!cant_go_back) {
	    update_mlstmv();	/* current monsters are becoming inactive */
	    bufon(fd);		/* use buffered output */
	}
	savelev(fd, ledger_no(&u.uz),
		cant_go_back ? FREE_SAVE : (WRITE_SAVE | FREE_SAVE));
	bclose(fd);
	if (cant_go_back) {
	    /* discard unreachable levels; keep #0 */
	    for (l_idx = maxledgerno(); l_idx > 0; --l_idx)
		delete_levelfile(l_idx);
	}

#ifdef REINCARNATION
	if (Is_rogue_level(newlevel) || Is_rogue_level(&u.uz))
		assign_rogue_graphics(Is_rogue_level(newlevel));
#endif
#ifdef USE_TILES
	substitute_tiles(newlevel);
#endif
	/* record this level transition as a potential seen branch unless using
	 * some non-standard means of transportation (level teleport).
	 */
	if ((at_stairs || falling || portal) && (u.uz.dnum != newlevel->dnum))
		recbranch_mapseen(&u.uz, newlevel);
	assign_level(&u.uz0, &u.uz);
	assign_level(&u.uz, newlevel);
	assign_level(&u.utolev, newlevel);
	u.utotype = 0;
	if (dunlev_reached(&u.uz) < dunlev(&u.uz))
		dunlev_reached(&u.uz) = dunlev(&u.uz);
	reset_rndmonst(NON_PM);   /* u.uz change affects monster generation */

	/* set default level change destination areas */
	/* the special level code may override these */
	(void) memset((genericptr_t) &updest, 0, sizeof updest);
	(void) memset((genericptr_t) &dndest, 0, sizeof dndest);

	if (!(level_info[new_ledger].flags & LFILE_EXISTS)) {
remake:
		/* entering this level for first time; make it now */
		if (level_info[new_ledger].flags & (FORGOTTEN|VISITED)) {
		    impossible("goto_level: returning to discarded level?");
		    level_info[new_ledger].flags &= ~(FORGOTTEN|VISITED);
		}
		mklev();
		new = TRUE;	/* made the level */
		if(Role_if(PM_TOURIST)){
			int dungeon_depth = 1;
			if (In_quest(&u.uz)) dungeon_depth = dunlev(&u.uz);
			else if (In_endgame(&u.uz) || Is_rlyeh(&u.uz) || Is_valley(&u.uz)) dungeon_depth = 100;
			else if (In_tower(&u.uz)) dungeon_depth = (5 - dunlev(&u.uz))*5;
			else if (In_law(&u.uz)) dungeon_depth = (path1_level.dlevel - u.uz.dlevel) + depth(&path1_level);
			else if (In_sokoban(&u.uz)) dungeon_depth = (5 - dunlev(&u.uz))*5;
			else dungeon_depth = depth(&u.uz) > 0 ? depth(&u.uz) : depth(&u.uz)-1;
			
			// more_experienced(u.ulevel*u.ulevel,0);
			more_experienced(u.ulevel*dungeon_depth,0);
			newexplevel();
		}
	} else {
		/* returning to previously visited level; reload it */
		fd = open_levelfile(new_ledger, whynot);
		if (fd < 0) {
			pline("%s", whynot);
			pline("Probably someone removed it.");
			goto remake; //Try remaking missing levels
			killer = whynot;
			done(TRICKED);
			/* we'll reach here if running in wizard mode */
			error("Cannot continue this game.");
		}
		minit();	/* ZEROCOMP */
		getlev(fd, hackpid, new_ledger, FALSE);
		(void) close(fd);
		oinit(); /* reassign level dependent obj probabilities (Pat Rankin)*/
	}
	/* do this prior to level-change pline messages */
	vision_reset();		/* clear old level's line-of-sight */
	vision_full_recalc = 0;	/* don't let that reenable vision yet */
	flush_screen(-1);	/* ensure all map flushes are postponed */
	

	if (portal == PAINTING_OUT){
		struct obj *painting;
		for(painting = fobj; painting; painting = painting->nobj)
			if(painting->oartifact == ART_PAINTING_FRAGMENT)
				break;
	    if (!painting){
			impossible("goto_level: no painting found!");
			goto misc_levelport;
		}
		if(!isok(painting->ox, painting->oy)){
			impossible("goto_level: painting doesn't know where it is!");
			goto misc_levelport;
		}
	    u_on_newpos(painting->ox, painting->oy);
	}
	else if (portal && !In_endgame(&u.uz)
		&& !(Role_if(PM_NOBLEMAN) && Race_if(PM_HALF_DRAGON) && flags.initgend && Is_qstart(&u.uz))
	) {
	    /* find the portal on the new level */
	    register struct trap *ttrap;
		int found=0;

		for (ttrap = ftrap; ttrap; ttrap = ttrap->ntrap){
			if (ttrap->ttyp == MAGIC_PORTAL && ttrap->dst.dlevel == u.uz0.dlevel){ //try to find a portal back to starting lev
				found = 1;
				break;
			}
		}
		if(!found){
			for (ttrap = ftrap; ttrap; ttrap = ttrap->ntrap) //otherwise just go with any portal
				if (ttrap->ttyp == MAGIC_PORTAL) break;
		}

	    if (!ttrap){
			impossible("goto_level: no corresponding portal!");
			goto misc_levelport;
		}
	    seetrap(ttrap);
	    u_on_newpos(ttrap->tx, ttrap->ty);
	} else if (at_stairs && !In_endgame(&u.uz)) {
	    if (up) {
			if (at_ladder) {
				dnladder.u_traversed = TRUE;
				u_on_newpos(xdnladder, ydnladder);
			} else {
				if (newdungeon) {
					sstairs.u_traversed = TRUE;
					if (Is_stronghold(&u.uz)) {
					register xchar x, y;

					do {
						x = (COLNO - 2 - rnd(5));
						y = rn1(ROWNO - 4, 3);
					} while(occupied(x, y) ||
						IS_WALL(levl[x][y].typ));
						u_on_newpos(x, y);
					} else u_on_sstairs();
				} else {
					dnstair.u_traversed = TRUE;
					u_on_dnstairs();
				}
			}
			/* Remove bug which crashes with levitation/punishment  KAA */
			if (Punished && !Levitation) {
				pline("With great effort you climb the %s.",
				at_ladder ? "ladder" : "stairs");
			} else if (at_ladder)
				You("climb up the ladder.");
	    } else {	/* down */
			if (at_ladder) {
				upladder.u_traversed = TRUE;
				u_on_newpos(xupladder, yupladder);
			} else {
				if (newdungeon) {
					sstairs.u_traversed = TRUE;
					u_on_sstairs();
				} else {
					upstair.u_traversed = TRUE;
					u_on_upstairs();
				}
			}
			if (u.dz && Flying)
				You("fly down along the %s.",
				at_ladder ? "ladder" : "stairs");
			else if (u.dz &&
	#ifdef CONVICT
				(near_capacity() > UNENCUMBERED || (Punished &&
				((uwep != uball) || ((P_SKILL(P_FLAIL) < P_BASIC))
				|| !Role_if(PM_CONVICT)))
				 || Fumbling)
	#else
				(near_capacity() > UNENCUMBERED || Punished || Fumbling)
	#endif /* CONVICT */
			) {
				You("fall down the %s.", at_ladder ? "ladder" : "stairs");
				if (Punished) {
				drag_down();
				ballrelease(FALSE);
				}
				if(((uwep && is_lightsaber(uwep) && litsaber(uwep))) ||
					(uswapwep && is_lightsaber(uswapwep) && litsaber(uswapwep) && u.twoweap)
				){
					boolean mainsaber = (uwep && is_lightsaber(uwep) && litsaber(uwep));
					boolean mainsaber_locked = (uwep && (uwep->oartifact == ART_INFINITY_S_MIRRORED_ARC || uwep->otyp == KAMEREL_VAJRA));
					boolean secsaber = (uswapwep && is_lightsaber(uswapwep) && litsaber(uswapwep) && u.twoweap);
					boolean secsaber_locked = (uswapwep && (uswapwep->oartifact == ART_INFINITY_S_MIRRORED_ARC || uswapwep->otyp == KAMEREL_VAJRA));
					if((mainsaber &&  mainsaber_locked)
						|| (secsaber && secsaber_locked)
					){
						int lrole = rnl(20);
						if(lrole+5 < ACURR(A_DEX)){
							You("roll and dodge your tumbling energy sword%s.", (mainsaber && secsaber) ? "s" : "");
						} else {
							You("come into contact with your energy sword%s.", (mainsaber && secsaber && (lrole >= ACURR(A_DEX) || (mainsaber_locked && secsaber_locked))) ? "s" : "");
							if(mainsaber && (mainsaber_locked || lrole >= ACURR(A_DEX)))
								losehp(dmgval(uwep,&youmonst,0,&youmonst), "falling downstairs with a lit lightsaber", KILLED_BY);
							if(secsaber && (secsaber_locked || lrole >= ACURR(A_DEX)))
								losehp(dmgval(uswapwep,&youmonst,0,&youmonst), "falling downstairs with a lit lightsaber", KILLED_BY);
						}
						if(mainsaber && !mainsaber_locked)
							lightsaber_deactivate(uwep, TRUE);
						if(secsaber && !secsaber_locked)
							lightsaber_deactivate(uswapwep, TRUE);
					} else {
						if(rnl(20) < ACURR(A_DEX)){
							You("hurriedly deactivate your energy sword%s.", (mainsaber && secsaber) ? "s" : "");
						} else {
							You("come into contact with your energy sword%s.", (mainsaber && secsaber) ? "s" : "");
							if(mainsaber) losehp(dmgval(uwep,&youmonst,0,&youmonst), "falling downstairs with a lit lightsaber", KILLED_BY);
							if(secsaber) losehp(dmgval(uswapwep,&youmonst,0,&youmonst), "falling downstairs with a lit lightsaber", KILLED_BY);
						}
						if(mainsaber) lightsaber_deactivate(uwep, TRUE);
						if(secsaber) lightsaber_deactivate(uswapwep, TRUE);
					}
				}
				/* falling off steed has its own losehp() call */
				if (u.usteed)
					dismount_steed(DISMOUNT_FELL);
				else
					losehp(rnd(3), "falling downstairs", KILLED_BY);

				selftouch("Falling, you");
			} else if (u.dz && at_ladder)
				You("climb down the ladder.");
	    }
	} else {	/* trap door or level_tele or In_endgame */
misc_levelport:
	    if (was_in_W_tower && On_W_tower_level(&u.uz))
		/* Stay inside the Wizard's tower when feasible.	*/
		/* Note: up vs down doesn't really matter in this case. */
		place_lregion(dndest.nlx, dndest.nly,
				dndest.nhx, dndest.nhy,
				0,0, 0,0, LR_DOWNTELE, (d_level *) 0);
	    else if (up)
		place_lregion(updest.lx, updest.ly,
				updest.hx, updest.hy,
				updest.nlx, updest.nly,
				updest.nhx, updest.nhy,
				LR_UPTELE, (d_level *) 0);
	    else
		place_lregion(dndest.lx, dndest.ly,
				dndest.hx, dndest.hy,
				dndest.nlx, dndest.nly,
				dndest.nhx, dndest.nhy,
				LR_DOWNTELE, (d_level *) 0);
	    if (falling) {
		if (Punished) ballfall();
		selftouch("Falling, you");
	    }
	}

	if (Punished) placebc();
	obj_delivery();		/* before killing geno'd monsters' eggs */
	losedogs();
	kill_genocided_monsters();  /* for those wiped out while in limbo */
	/*
	 * Expire all timers that have gone off while away.  Must be
	 * after migrating monsters and objects are delivered
	 * (losedogs and obj_delivery).
	 */
	run_timers();

	initrack();

	if ((mtmp = m_at(u.ux, u.uy)) != 0
#ifdef STEED
		&& mtmp != u.usteed
#endif
		) {
	    /* There's a monster at your target destination; it might be one
	       which accompanied you--see mon_arrive(dogmove.c)--or perhaps
	       it was already here.  Randomly move you to an adjacent spot
	       or else the monster to any nearby location.  Prior to 3.3.0
	       the latter was done unconditionally. */
	    coord cc;

	    if (!rn2(2) &&
		    enexto(&cc, u.ux, u.uy, youracedata) &&
		    distu(cc.x, cc.y) <= 2)
		u_on_newpos(cc.x, cc.y);	/*[maybe give message here?]*/
	    else
		mnexto(mtmp);

	    if ((mtmp = m_at(u.ux, u.uy)) != 0) {
#ifdef WIZARD
		/* there was an unconditional impossible("mnearto failed")
		   here, but it's not impossible and we're prepared to cope
		   with the situation, so only say something when debugging */
		if (wizard) pline("(monster in hero's way)");
#endif
		if (!rloc(mtmp, TRUE)){
			if(wizard) pline("arriving later.");
		    /* no room to move it; send it away, to return later */
		    migrate_to_level(mtmp, ledger_no(&u.uz),
				     MIGR_RANDOM, (coord *)0);
			mtmp->marriving = TRUE;
		}
	    }
	}

	/* initial movement of bubbles just before vision_recalc */
	if (Is_waterlevel(&u.uz))
		movebubbles();

	if (level_info[new_ledger].flags & FORGOTTEN) {
	    forget_map(100);	/* forget the map */
	    // forget_traps();		/* forget all traps too */
	    familiar = TRUE;
	    level_info[new_ledger].flags &= ~FORGOTTEN;
	}

	/* Reset the screen. */
	vision_reset();		/* reset the blockages */
	docrt();		/* does a full vision recalc */
	flush_screen(-1);

	/*
	 *  Move all plines beyond the screen reset.
	 */

	/* give room entrance message, if any */
	check_special_room(FALSE);

	/* Check whether we just entered Gehennom. */
	if (!In_hell(&u.uz0) && Inhell) {
	    if (Is_valley(&u.uz)) {
		You("arrive at the Valley of the Dead...");
		pline_The("odor of burnt flesh and decay pervades the air.");
#ifdef MICRO
		display_nhwindow(WIN_MESSAGE, FALSE);
#endif
		You_hear("groans and moans everywhere.");
	    } else {
			pline("It is hot here.  You smell smoke...");
#ifdef RECORD_ACHIEVE
			achieve.enter_gehennom = 1;
#endif
		}
	}

	if (familiar) {
	    static const char * const fam_msgs[4] = {
		"You have a sense of deja vu.",
		"You feel like you've been here before.",
		"This place %s familiar...",
		0	/* no message */
	    };
	    static const char * const halu_fam_msgs[4] = {
		"Whoa!  Everything %s different.",
		"You are surrounded by twisty little passages, all alike.",
		"Gee, this %s like uncle Conan's place...",
		0	/* no message */
	    };
	    const char *mesg;
	    char buf[BUFSZ];
	    int which = rn2(4);

	    if (Hallucination)
		mesg = halu_fam_msgs[which];
	    else
		mesg = fam_msgs[which];
	    if (mesg && index(mesg, '%')) {
		Sprintf(buf, mesg, !Blind ? "looks" : "seems");
		mesg = buf;
	    }
	    if (mesg) pline1(mesg);
	}

#ifdef REINCARNATION
	if (new && Is_rogue_level(&u.uz))
	    You("enter what seems to be an older, more primitive world.");
#endif
	/* Final confrontation */
	if (In_endgame(&u.uz) && newdungeon && u.uhave.amulet){
		if(Role_if(PM_MADMAN) && Race_if(PM_ELF)){
			makemon(&mons[flags.initgend ? PM_PUPPET_EMPEROR_XELETH : PM_PUPPET_EMPRESS_XEDALLI], u.ux, u.uy, MM_ADJACENTOK);
			verbalize("Why won't you JUST. STAY. DEAD!?");
			makemon(&mons[PM_FLAXEN_STAR_PHANTOM], u.ux, u.uy, MM_ADJACENTOK);
			makemon(&mons[PM_FLAXEN_STARSHADOW], u.ux, u.uy, MM_ADJACENTOK);
			makemon(&mons[PM_FLAXEN_STARSHADOW], u.ux, u.uy, MM_ADJACENTOK);
		}
		else
			resurrect();
	}
	if (newdungeon && In_V_tower(&u.uz) && In_hell(&u.uz0))
		pline_The("heat and smoke are gone.");

	boolean restart_quest = (!u.uevent.qrecalled && Role_if(PM_UNDEAD_HUNTER) && !mvitals[PM_MOON_S_CHOSEN].died && u.uevent.qcompleted && quest_status.time_doing_quest >= UH_QUEST_TIME_1);
	boolean recalled = u.uevent.qrecalled && (Role_if(PM_UNDEAD_HUNTER) && !mvitals[PM_MOON_S_CHOSEN].died);
	/* the message from your quest leader */
	if (((!In_quest(&u.uz0) && at_dgn_entrance("The Quest"))
		|| (restart_quest && !In_quest(&u.uz0) && In_quest(&u.uz))
	  ) &&
		!(u.uevent.qexpelled 
		  || (u.uevent.qcompleted && !(recalled || restart_quest))
		  || quest_status.leader_is_dead
		)
	) {
		if(Role_if(PM_EXILE)){
			You("sense something reaching out to you....");
		} else if(Role_if(PM_MADMAN)){
			if(u.uevent.qcalled){
				You("again sense Lady Constance pleading for help.");
			}
			else {
				You("receive a faint telepathic message from Lady Constance:");
				pline("Your help is urgently needed at Archer Asylum!  Look for a ...ic transporter.");
				pline("You couldn't quite make out that last message.");
			}
		} else if(Role_if(PM_HEALER) && Race_if(PM_DROW)){
			if(u.uevent.qcalled){
				You("again sense Sister T'eirastra pleading for help.");
			}
			else {
				You("receive a faint telepathic message from T'eirastra:");
				pline("Your help is urgently needed at Menzoberranzan!  Look for a ...ic transporter.");
				pline("You couldn't quite make out that last message.");
			}
		} else if(Role_if(PM_UNDEAD_HUNTER) && restart_quest){
			u.uevent.qrecalled = TRUE;
			quest_status.got_thanks = FALSE;
			if(quest_status.time_doing_quest >= UH_QUEST_TIME_4){
				You("again sense Vicar Amalia pleading for help.");
				pline("But it's garbled and confused.");
			}
			else if(quest_status.time_doing_quest >= UH_QUEST_TIME_2){
				You("again sense Vicar Amalia pleading for help:");
				pline("Something is wrong. The infection is spreading in the upper city!");
			}
			else if(quest_status.time_doing_quest >= UH_QUEST_TIME_1){
				You("again sense Vicar Amalia pleading for help:");
				pline("Something is wrong. The infection is now spreading in the city!");
			}
			//Futureproof, but I don't think this can be reached.
			else {
				You("again sense Vicar Amalia pleading for help:");
				pline("Something is wrong. The infection is still spreading.");
			}
		} else {
			if (u.uevent.qcalled) {
				com_pager(Role_if(PM_ROGUE) ? 4 : 3);
			} else {
				com_pager(2);
				u.uevent.qcalled = TRUE;
			}
		}
	}

	/* once Croesus is dead, his alarm doesn't work any more */
	if (Is_knox(&u.uz) && (new || !mvitals[PM_CROESUS].died)) {
		You("penetrated a high security area!");
		pline("An alarm sounds!");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		    if (!DEADMONSTER(mtmp) && mtmp->msleeping) mtmp->msleeping = 0;
	}

	if (Is_astralevel(&u.uz))
	    final_level();
	else
	    onquest();
	assign_level(&u.uz0, &u.uz); /* reset u.uz0 */

	//Restock -- Dummied out for now, I'm not confident this won't play horribly without further tuning
	long timeline = monstermoves;
	/*int spawn_freq = random_frequency();
	if(Role_if(PM_ANACHRONONAUT) && Infuture)
		timeline = quest_status.time_doing_quest;
	if(spawn_freq && spawn_freq <= 70 && timeline > level.lastmove){
		int delta = timeline - level.lastmove;
		if(delta >= spawn_freq){
			extern const int monstr[];
			int count = 0;
			int target = level_difficulty()*3;
			for(struct monst *mtmp = fmon; mtmp && count < target; mtmp = mtmp->nmon){
				if(!mtmp->mtame)
					count += monstr[mtmp->mtyp]/(mtmp->mpeaceful ? 2 : 1);
			}
			for (delta = delta/spawn_freq; delta > 0 && count < target; delta--){
				if(rn2(3+(count*30)/target)){
					count += spawn_random_monster();
				}
			}
		}
	}*/
	level.lastmove = timeline;
#ifdef INSURANCE
	save_currentstate();
#endif

	/* assume this will always return TRUE when changing level */
	(void) in_out_region(u.ux, u.uy);
	(void) pickup(1);
	if(Is_waterlevel(&u.uz)) spoteffects(FALSE); /*Decided to be specific about this.  Dump character in water when coming out of portal*/
#ifdef WHEREIS_FILE
        touch_whereis();
#endif
}

STATIC_OVL boolean
no_spirits()
{
	struct monst *mtmp;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon){
		if(!DEADMONSTER(mtmp) && !(mtmp->mtame)&& !(mtmp->mpeaceful)){
			return FALSE;
		}
	}
	return TRUE;
}

STATIC_OVL void
final_level()
{
	struct monst *mtmp;
	struct obj *otmp;
	coord mm;
	int i;

	/* reset monster hostility relative to player */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp)) reset_hostility(mtmp);

	/* create some player-monsters */
	create_mplayers(rn1(4, 3), TRUE);

	/* Center of All arrives to stop the player from throwing off the Balance. */
	coa_arrive();
	
	if(u.uevent.ukilled_apollyon){
		livelog_write_string("confronted the Fallen");
		int host;
	    pline(
	     "A voice booms: \"The Angel of the Pit hast fallen!  We have returned!\"");
		(void) makemon(&mons[PM_LUCIFER], u.ux, u.uy, MM_ADJACENTOK);
		(void) makemon(&mons[PM_ANCIENT_OF_DEATH], u.ux, u.uy, MM_ADJACENTOK);
		(void) makemon(&mons[PM_ANCIENT_OF_ICE], u.ux, u.uy, MM_ADJACENTOK);
/*		for(host = 0; host < 10; host++ )*/ (void) makemon(&mons[PM_FALLEN_ANGEL], u.ux, u.uy, MM_ADJACENTOK);
	}
	if(Role_if(PM_MADMAN) && Race_if(PM_ELF)){
		makemon(&mons[PM_FLAXEN_STAR_PHANTOM], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STAR_PHANTOM], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STAR_PHANTOM], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STAR_PHANTOM], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STAR_PHANTOM], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_FLAXEN_STARSHADOW], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_CARCOSAN_COURTIER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_CARCOSAN_COURTIER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_CARCOSAN_COURTIER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_CARCOSAN_COURTIER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_CARCOSAN_COURTIER], 0, 0, MM_ADJACENTOK);
	}
	else if(Role_if(PM_UNDEAD_HUNTER) && quest_status.moon_close){
		makemon(&mons[PM_MOON_ENTITY_TONGUE], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_EYE_CLUSTER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MOON_ENTITY_TONGUE], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_EYE_CLUSTER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MOON_ENTITY_TONGUE], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_EYE_CLUSTER], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_ENTITY_MANIPALP], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MIST_WOLF], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);

		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
		makemon(&mons[PM_MOON_FLEA], 0, 0, MM_ADJACENTOK);
	}
	/* create a guardian angel next to player, if worthy */
	if(!u.veil){
		You("notice the air thrums with hidden holy energy.");
	}
	/* create a guardian angel next to player, if worthy */
	if (Conflict || u.ualign.type == A_VOID || u.ualign.type == A_NONE) {
		if(Conflict)
			pline(
			 "A voice booms: \"Thy desire for conflict shall be fulfilled!\"");
		else
			pline("A voice booms: \"Die, heretic!\"");
	    for (i = rnd(4); i > 0; --i) {
		mm.x = u.ux;
		mm.y = u.uy;
		if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL]))
		    (void) mk_roamer(&mons[PM_ANGEL], u.ualign.type,
				     mm.x, mm.y, FALSE);
	    }

	} else if (u.ualign.record > 8 && !philosophy_index(u.ualign.god)) {	/* fervent */
	    pline("A voice whispers: \"Thou hast been worthy of me!\"");
	    mm.x = u.ux;
	    mm.y = u.uy;
	    if (enexto(&mm, mm.x, mm.y, &mons[PM_ANGEL])) {
		if ((mtmp = mk_roamer(&mons[PM_ANGEL], u.ualign.type,
				      mm.x, mm.y, TRUE)) != 0) {
		    if (!Blind)
			pline("An angel appears near you.");
		    else
			You_feel("the presence of a friendly angel near you.");
			initedog(mtmp);
		    mtmp->mtame = 10;
		    /* make him strong enough vs. endgame foes */
		    mtmp->m_lev = rn1(8,15);
		    mtmp->mhp = mtmp->mhpmax = 8*mtmp->m_lev - rnd(7);
		    if ((otmp = select_hwep(mtmp)) == 0) {
			otmp = mksobj(SABER, MKOBJ_NOINIT);
			if (mpickobj(mtmp, otmp))
			    panic("merged weapon?");
		    }
		    bless(otmp);
		    if (otmp->spe < 4) otmp->spe += rnd(4);
		    if ((otmp = which_armor(mtmp, W_ARMS)) == 0 ||
			    otmp->otyp != SHIELD_OF_REFLECTION
			) {
				(void) mongets(mtmp, AMULET_OF_REFLECTION, NO_MKOBJ_FLAGS);
				m_dowear(mtmp, TRUE);
				init_mon_wield_item(mtmp);
				m_level_up_intrinsic(mtmp);
		    }
		}
	    }
	}
}

static char *dfr_pre_msg = 0,	/* pline() before level change */
	    *dfr_post_msg = 0;	/* pline() after level change */

static int dfr_post_dmg = 0;
static int dfr_post_san = 0;

/* change levels at the end of this turn, after monsters finish moving */
void
schedule_goto(tolev, at_stairs, falling, portal_flag, pre_msg, post_msg, post_dmg, post_san)
d_level *tolev;
boolean at_stairs, falling;
int portal_flag;
const char *pre_msg, *post_msg;
int post_dmg;
int post_san;
{
	int typmask = 0100;		/* non-zero triggers `deferred_goto' */
	if(Is_nowhere(&u.uz) && !flags.phasing) return;

	/* destination flags (`goto_level' args) */
	if (at_stairs)	 typmask |= 0x1;
	if (falling)	 typmask |= 0x2;
	if (portal_flag) typmask |= 0x4;
	if (portal_flag == PAINTING_OUT) typmask |= 0x8;
	if (portal_flag < 0) typmask |= 0x80;	/* flag for portal removal */
	u.utotype = typmask;
	/* destination level */
	assign_level(&u.utolev, tolev);

	if (pre_msg)
	    dfr_pre_msg = strcpy((char *)alloc(strlen(pre_msg) + 1), pre_msg);
	if (post_msg)
	    dfr_post_msg = strcpy((char *)alloc(strlen(post_msg)+1), post_msg);
	if(post_dmg)
		dfr_post_dmg = post_dmg;
	if(post_san)
		dfr_post_san = post_san;
}

/* handle something like portal ejection */
void
deferred_goto()
{
	if (!on_level(&u.uz, &u.utolev)) {
	    d_level dest;
	    int typmask = u.utotype; /* save it; goto_level zeroes u.utotype */

	    assign_level(&dest, &u.utolev);
	    if (dfr_pre_msg) pline1(dfr_pre_msg);
	    goto_level(&dest, !!(typmask&0x1), !!(typmask&0x2), (typmask&0x8) ? PAINTING_OUT : !!(typmask&0x4));
	    if (typmask & 0x80) {	/* remove portal */
		struct trap *t = t_at(u.ux, u.uy);

		if (t) {
		    deltrap(t);
		    newsym(u.ux, u.uy);
		}
	    }
	    if (dfr_post_msg) pline1(dfr_post_msg);
		if (dfr_post_dmg){
			losehp(dfr_post_dmg, "abrupt arrival", KILLED_BY_AN);
			dfr_post_dmg = 0;
		}
		if (dfr_post_san){
			change_usanity(dfr_post_san, dfr_post_san < 0);
			dfr_post_san = 0;
		}
	}
	u.utotype = 0;		/* our caller keys off of this */
	if (dfr_pre_msg)
	    free((genericptr_t)dfr_pre_msg),  dfr_pre_msg = 0;
	if (dfr_post_msg)
	    free((genericptr_t)dfr_post_msg),  dfr_post_msg = 0;
}

#endif /* OVL2 */
#ifdef OVL3

/*
 * Return TRUE if we created a monster for the corpse.  If successful, the
 * corpse is gone.
 */

boolean
revive_corpse(corpse, different)
struct obj *corpse;
int different;
{
    struct monst *mtmp, *mcarry;
    boolean is_uwep, chewed;
    xchar where;
    char *cname, cname_buf[BUFSZ];
    struct obj *container = (struct obj *)0;
    int container_where = 0;
    where = corpse->where;
    is_uwep = corpse == uwep;
    cname = eos(strcpy(cname_buf, "bite-covered "));
    Strcpy(cname, corpse_xname(corpse, TRUE));
    mcarry = (where == OBJ_MINVENT) ? corpse->ocarry : 0;
	int ox, oy;
	if(where == OBJ_FLOOR){
		ox = corpse->ox;
		oy = corpse->oy;
	}
	
    if (where == OBJ_CONTAINED) {
    	struct monst *mtmp2 = (struct monst *)0;
		container = corpse->ocontainer;
    	mtmp2 = get_container_location(container, &container_where, (int *)0);
		/* container_where is the outermost container's location even if nested */
		if (container_where == OBJ_MINVENT && mtmp2) mcarry = mtmp2;
    }
    mtmp = revive(corpse, FALSE);      /* corpse is gone if successful && quan == 1 */

    if (mtmp) {
	/*
	 * [ALI] Override revive's HP calculation. The HP that a mold starts
	 * with do not depend on the HP of the monster whose corpse it grew on.
	 */
	if (different)
	    mtmp->mhp = mtmp->mhpmax;
	else if(has_sunflask(mtmp->mtyp))
		mtmp->mvar_flask_charges = MAX_FLASK_CHARGES(mtmp);
	chewed = !different && (mtmp->mhp < mtmp->mhpmax);
	if (chewed) cname = cname_buf;	/* include "bite-covered" prefix */
	if(different==REVIVE_ZOMBIE){
		if(mtmp->mspores){
			set_template(mtmp, SPORE_ZOMBIE);
			if(couldsee(mtmp->mx, mtmp->my) && distmin(u.ux, u.uy, mtmp->mx, mtmp->my)){
				IMPURITY_UP(u.uimp_rot)
			}
			mtmp->mspores = 0;
		}
		else {
			set_template(mtmp, ZOMBIFIED);
		}
		mtmp->zombify = 0;
		if(mtmp->mpeaceful && !mtmp->mtame){
			mtmp->mpeaceful = 0;
		}
		if(has_template(mtmp, SPORE_ZOMBIE) && Nightmare && u.umadness&MAD_SPORES && rn2(100) < Insanity){
			mtmp->mpeaceful = TRUE;
		}
	}
	if(different==REVIVE_YELLOW){
		set_template(mtmp, YELLOW_TEMPLATE);
		mtmp->zombify = 0;
		set_faction(mtmp, YELLOW_FACTION);
		mtmp->mcrazed = 0;
		if(mtmp->mpeaceful && !mtmp->mtame){
			mtmp->mpeaceful = 0;
		}
	}
	switch (where) {
	    case OBJ_INVENT:
		if (is_uwep) {
		    if (different==GROW_MOLD) {
				Your("weapon goes moldy.");
				pline("%s writhes out of your grasp!", Monnam(mtmp));
		    }
		    else if (different==GROW_SLIME) {
				Your("weapon goes slimy.");
				pline("%s slips out of your grasp!", Monnam(mtmp));
		    }
		    else if (different==GROW_BBLOOM) {
				Your("weapon sprouts flowers.");
				pline("%s pushes out of your grasp!", Monnam(mtmp));
		    }
		    else if (different==REVIVE_ZOMBIE || different==REVIVE_YELLOW) {
				pline_The("%s rises from the dead!", cname);
				pline("%s writhes out of your grasp!", Monnam(mtmp));
		    }
		    else{
				pline_The("%s writhes out of your grasp!", cname);
			}
		}
		else
		    You_feel("squirming in your backpack!");
		break;

	    case OBJ_FLOOR:
		if (cansee(mtmp->mx, mtmp->my)) {
		    if (different==GROW_MOLD)
				pline("%s grows on a moldy corpse!",
				  Amonnam(mtmp));
		    else if (different==REVIVE_MOLD)
				pline("%s regrows from its corpse!",
				  Amonnam(mtmp));
		    else if (different==GROW_SLIME)
				pline("%s leaks from a putrefying corpse!",
				  Amonnam(mtmp));
		    else if (different==GROW_BBLOOM)
				pline("%s sprouts from a corpse!",
				  Amonnam(mtmp));
		    else if (different==REVIVE_ZOMBIE || different==REVIVE_YELLOW)
				pline("%s rises from the dead!",
				  Amonnam(mtmp));
		    else if (different==REVIVE_SHADE)
				pline("%s forms from a corpse!",
				  Amonnam(mtmp));
		    else if (mtmp->mtyp==PM_DEATH)
				pline("Death cannot die.");
			else
				pline("%s rises from the dead!", chewed ?
					Adjmonnam(mtmp, "bite-covered") : Monnam(mtmp));
		}
		start_timer(0, TIMER_MONSTER, REVIVE_PICKUP, (genericptr_t)mtmp);
		break;

	    case OBJ_MINVENT:		/* probably a nymph's */
		if (cansee(mtmp->mx, mtmp->my)) {
		    if (canseemon(mcarry))
			pline("Startled, %s drops %s as it %s!",
			      mon_nam(mcarry), different ? "a corpse" : an(cname),
			      different==GROW_MOLD ? "goes moldy" : 
			      different==GROW_SLIME ? "putrefies" : 
			      different==GROW_BBLOOM ? "sprouts" : 
			      different==REVIVE_ZOMBIE ? "rises from the dead" : 
			      different==REVIVE_YELLOW ? "rises from the dead" : 
			      different==REVIVE_SHADE ? "dissolves into shadow" : 
				  "revives");
		    else
			pline("%s suddenly appears!", chewed ?
			      Adjmonnam(mtmp, "bite-covered") : Monnam(mtmp));
		}
		break;
	   case OBJ_CONTAINED:
	   	if (container_where == OBJ_MINVENT && cansee(mtmp->mx, mtmp->my) &&
		    mcarry && canseemon(mcarry) && container) {
		        char sackname[BUFSZ];
		        Sprintf(sackname, "%s %s", s_suffix(mon_nam(mcarry)),
				xname(container)); 
	   		pline("%s writhes out of %s!", Amonnam(mtmp), sackname);
	   	} else if (container_where == OBJ_INVENT && container) {
		        char sackname[BUFSZ];
		        Strcpy(sackname, an(xname(container)));
	   		pline("%s %s out of %s in your pack!",
	   			Blind ? Something : Amonnam(mtmp),
				locomotion(mtmp,"writhes"),
	   			sackname);
	   	} else if (container_where == OBJ_FLOOR && container &&
		            cansee(mtmp->mx, mtmp->my)) {
		        char sackname[BUFSZ];
		        Strcpy(sackname, an(xname(container)));
			pline("%s escapes from %s!", Amonnam(mtmp), sackname);
		}
		break;
	    default:
		/* we should be able to handle the other cases... */
		impossible("revive_corpse: lost corpse @ %d", where);
		break;
	}
	newsym(mtmp->mx,mtmp->my);
	if(is_great_old_one(mtmp->data)){
		TRANSCENDENCE_IMPURITY_UP(FALSE)
	}
	return TRUE;
    }
    return FALSE;
}

/* Revive the corpse via a timeout. */
/*ARGSUSED*/
void
revive_mon(arg, timeout)
genericptr_t arg;
long timeout;
{
    struct obj *body = (struct obj *) arg;

    /* if we succeed, the corpse is gone, otherwise, rot it away */
    if (!revive_corpse(body, 
					(is_fungus(&mons[body->corpsenm]) &&
					!is_migo(&mons[body->corpsenm])) ? 
						REVIVE_MOLD : 
						REVIVE_MONSTER)
	) {
		if (is_rider(&mons[body->corpsenm]))
			You_feel("less hassled.");
		(void) start_timer(250L - (monstermoves-body->age),
						TIMER_OBJECT, ROT_CORPSE, arg);
    }
}

/* 
 * Monster picks up and equips all items it likes/can wear on its square.
 * For use with reviving, so this only happens when timers run, so *after*
 * all bhito effects finish in the case of a wand affecting a rider corpse
 */
void
revive_mon_pickup(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct monst *mtmp = (struct monst *) arg;

	if (timeout != monstermoves)
		return;

	if(level.objects[mtmp->mx][mtmp->my] && !mtmp->menvy){
		struct obj *cur;
		struct obj *nobj;
		for(cur = level.objects[mtmp->mx][mtmp->my]; cur; cur = nobj){
			nobj = cur->nexthere;
			/* Monsters don't pick up your ball and chain */
			if(cur == uball || cur == uchain)
				continue;

			/* Monsters don't pick up bolted magic chests */
			if(cur->otyp == MAGIC_CHEST && cur->obolted)
				continue;

			if(likes_obj(mtmp, cur) || can_equip(mtmp, cur)){
				obj_extract_self(cur);
				mpickobj(mtmp, cur);
			}
		}
		m_dowear(mtmp, TRUE);
		init_mon_wield_item(mtmp);
		m_level_up_intrinsic(mtmp);
	}
}

static const int molds[] = 
{
	PM_BROWN_MOLD,
	PM_YELLOW_MOLD,
	PM_GREEN_MOLD,
	PM_RED_MOLD
};
static const int slimes[] = 
{
	PM_BROWN_PUDDING,
	PM_BLACK_PUDDING,
	PM_ACID_BLOB,
	PM_GELATINOUS_CUBE
};
static const int shades[] = 
{
	PM_SHADE,
	PM_SHADE,
	PM_SHADE,
	PM_SHADE,
	PM_SHADE,
	
	PM_SHADE,
	PM_SHADE,
	PM_SHADE,
	PM_SHADE,
	PM_PHANTASM
};
/* Revive the corpse as a mold via a timeout. */
/*ARGSUSED*/
void
moldy_corpse(arg, timeout)
genericptr_t arg;
long timeout;
{
	int pmtype, oldtyp, oldquan;
	struct obj *body = (struct obj *) arg;

	/* Turn the corpse into a mold corpse if molds are available */
	oldtyp = body->corpsenm;

	struct monst *attchmon = 0;
	if(get_ox(body, OX_EMON)) attchmon = EMON(body);
	if(attchmon && attchmon->brainblooms){
		pmtype = PM_BRAINBLOSSOM_PATCH;
		rem_ox(body, OX_EMON);
		attchmon = 0;
	}
	else {
		/* Weight towards non-motile fungi.
		 */
		//	fruitadd("slime mold");
		pmtype = molds[rn2(SIZE(molds))];
	}

	/* [ALI] Molds don't grow in adverse conditions.  If it ever
	 * becomes possible for molds to grow in containers we should
	 * check for iceboxes here as well.
	 */
	if ((
			(body->where == OBJ_FLOOR || body->where==OBJ_BURIED) &&
			(is_pool(body->ox, body->oy, FALSE) || is_lava(body->ox, body->oy) ||
				is_ice(body->ox, body->oy))
		) || (
			(body->where == OBJ_CONTAINED && body->ocontainer->otyp == ICE_BOX)
		)
	) pmtype = -1;

	if (pmtype != -1) {
		if(couldsee(body->ox, body->oy) && distmin(body->ox, body->oy, u.ux, u.uy) <= BOLT_LIM){
			IMPURITY_UP(u.uimp_rot)
		}
		/* We don't want special case revivals */
		if (cant_create(&pmtype, TRUE) || get_ox(body, OX_EMON))
			pmtype = -1; /* cantcreate might have changed it so change it back */
		else {
				body->corpsenm = pmtype;

			/* oeaten isn't used for hp calc here, and zeroing it 
			 * prevents eaten_stat() from worrying when you've eaten more
			 * from the corpse than the newly grown mold's nutrition
			 * value.
			 */
			body->oeaten = 0;

			/* [ALI] If we allow revive_corpse() to get rid of revived
			 * corpses from hero's inventory then we run into problems
			 * with unpaid corpses.
			 */
			if (body->where == OBJ_INVENT)
				body->quan++;
			oldquan = body->quan;
			if (revive_corpse(body, (pmtype == PM_BRAINBLOSSOM_PATCH) ? GROW_BBLOOM : GROW_MOLD)) {
				if (oldquan != 1) {		/* Corpse still valid */
					body->corpsenm = oldtyp;
					if (body->where == OBJ_INVENT) {
						useup(body);
						oldquan--;
					}
				}
				if (oldquan == 1)
				body = (struct obj *)0;	/* Corpse gone */
			}
		}
	}

	/* If revive_corpse succeeds, it handles the reviving corpse.
	 * If there was more than one corpse, or the revive failed,
	 * set the remaining corpse(s) to rot away normally.
	 * Revive_corpse handles genocides
	 */
	if (body) {
		body->corpsenm = oldtyp; /* Fixup corpse after (attempted) revival */
		body->owt = weight(body);
		(void) start_timer(250L - (monstermoves-peek_at_iced_corpse_age(body)),
			TIMER_OBJECT, ROT_CORPSE, arg);
	}
}

/* Revive the corpse as a slime via a timeout. */
/*ARGSUSED*/
void
slimy_corpse(arg, timeout)
genericptr_t arg;
long timeout;
{
	int pmtype, oldtyp, oldquan;
	struct obj *body = (struct obj *) arg;

	/* Turn the corpse into a slimy corpse if slimes are available */
	oldtyp = body->corpsenm;

	pmtype = slimes[rn2(SIZE(slimes))];

	/* [ALI] Molds don't grow in adverse conditions.  If it ever
	 * becomes possible for molds to grow in containers we should
	 * check for iceboxes here as well.
	 */
	if ((body->where == OBJ_FLOOR || body->where==OBJ_BURIED) &&
	  (is_pool(body->ox, body->oy, FALSE) || is_lava(body->ox, body->oy) ||
	  is_ice(body->ox, body->oy)))
	pmtype = -1;

	if (pmtype != -1) {
	/* We don't want special case revivals */
		if (cant_create(&pmtype, TRUE) || get_ox(body, OX_EMON))
			pmtype = -1; /* cantcreate might have changed it so change it back */
		else {
			body->corpsenm = pmtype;

		/* oeaten isn't used for hp calc here, and zeroing it 
		 * prevents eaten_stat() from worrying when you've eaten more
		 * from the corpse than the newly grown mold's nutrition
		 * value.
		 */
		body->oeaten = 0;

		/* [ALI] If we allow revive_corpse() to get rid of revived
		 * corpses from hero's inventory then we run into problems
		 * with unpaid corpses.
		 */
		if (body->where == OBJ_INVENT)
			body->quan++;
		oldquan = body->quan;
			if (revive_corpse(body, GROW_SLIME)) {
			if (oldquan != 1) {		/* Corpse still valid */
			body->corpsenm = oldtyp;
			if (body->where == OBJ_INVENT) {
				useup(body);
				oldquan--;
			}
			}
			if (oldquan == 1)
			body = (struct obj *)0;	/* Corpse gone */
		}
		}
	}
}

/* Revive the corpse as a shade via a timeout. */
/*ARGSUSED*/
void
shady_corpse(arg, timeout)
genericptr_t arg;
long timeout;
{
	int pmtype, oldtyp, oldquan;
	struct obj *body = (struct obj *) arg;

	/* Turn the corpse into a slimy corpse if shades are available */
	oldtyp = body->corpsenm;

	pmtype = shades[rn2(SIZE(shades))];
	if(!rn2(100)) pmtype = PM_WRAITH;

	/* [ALI] Molds don't grow in adverse conditions.  If it ever
	 * becomes possible for molds to grow in containers we should
	 * check for iceboxes here as well.
	 */
	if ((body->where == OBJ_FLOOR || body->where==OBJ_BURIED) &&
	  (is_pool(body->ox, body->oy, FALSE) || is_lava(body->ox, body->oy) ||
	  is_ice(body->ox, body->oy)))
	pmtype = -1;

	if (pmtype != -1) {
	/* We don't want special case revivals */
		if (cant_create(&pmtype, TRUE) || get_ox(body, OX_EMON))
			pmtype = -1; /* cantcreate might have changed it so change it back */
		else {
			body->corpsenm = pmtype;

		/* oeaten isn't used for hp calc here, and zeroing it 
		 * prevents eaten_stat() from worrying when you've eaten more
		 * from the corpse than the newly grown mold's nutrition
		 * value.
		 */
		body->oeaten = 0;

		/* [ALI] If we allow revive_corpse() to get rid of revived
		 * corpses from hero's inventory then we run into problems
		 * with unpaid corpses.
		 */
		if (body->where == OBJ_INVENT)
			body->quan++;
		oldquan = body->quan;
			if (revive_corpse(body, REVIVE_SHADE)) {
			if (oldquan != 1) {		/* Corpse still valid */
			body->corpsenm = oldtyp;
			if (body->where == OBJ_INVENT) {
				useup(body);
				oldquan--;
			}
			}
			if (oldquan == 1)
			body = (struct obj *)0;	/* Corpse gone */
		}
		}
	}

	/* If revive_corpse succeeds, it handles the reviving corpse.
	 * If there was more than one corpse, or the revive failed,
	 * set the remaining corpse(s) to rot away normally.
	 * Revive_corpse handles genocides
	 */
	if (body) {
		body->corpsenm = oldtyp; /* Fixup corpse after (attempted) revival */
		body->owt = weight(body);
		(void) start_timer(250L - (monstermoves-peek_at_iced_corpse_age(body)),
			TIMER_OBJECT, ROT_CORPSE, arg);
	}
}

/* Revive the corpse as a zombie via a timeout. */
/*ARGSUSED*/
void
zombie_corpse(arg, timeout)
genericptr_t arg;
long timeout;
{
	int pmtype, oldtyp, oldquan;
	struct obj *body = (struct obj *) arg;
	
	pmtype = body->corpsenm;
	
	/* [ALI] Molds don't grow in adverse conditions.  If it ever
	 * becomes possible for molds to grow in containers we should
	 * check for iceboxes here as well.
	 */
	if ((body->where == OBJ_FLOOR || body->where==OBJ_BURIED) &&
	  (is_pool(body->ox, body->oy, FALSE) || is_lava(body->ox, body->oy) ||
	  is_ice(body->ox, body->oy)))
	pmtype = -1;
	
	if (pmtype != -1) {
		/* We don't want special case revivals */
		if (cant_create(&pmtype, TRUE) || (get_ox(body, OX_EMON) && !(EMON(body)->zombify || EMON(body)->mspores)))
			pmtype = -1; /* cantcreate might have changed it so change it back */
		else {
			body->corpsenm = pmtype;

			/* oeaten isn't used for hp calc here, and zeroing it 
			 * prevents eaten_stat() from worrying when you've eaten more
			 * from the corpse than the newly grown mold's nutrition
			 * value.
			 */
			body->oeaten = 0;

			/* [ALI] If we allow revive_corpse() to get rid of revived
			 * corpses from hero's inventory then we run into problems
			 * with unpaid corpses.
			 */
			if (body->where == OBJ_INVENT)
				body->quan++;
			oldquan = body->quan;
			if (revive_corpse(body, REVIVE_ZOMBIE)) {
				if (oldquan != 1) {		/* Corpse still valid */
				if (body->where == OBJ_INVENT) {
					useup(body);
					oldquan--;
				}
				}
				if (oldquan == 1)
				body = (struct obj *)0;	/* Corpse gone */
			}
		}
	}

	/* If revive_corpse succeeds, it handles the reviving corpse.
	 * If there was more than one corpse, or the revive failed,
	 * set the remaining corpse(s) to rot away normally.
	 * Revive_corpse handles genocides
	 */
	if (body) {
		body->owt = weight(body);
		(void) start_timer(250L - (monstermoves-peek_at_iced_corpse_age(body)),
			TIMER_OBJECT, ROT_CORPSE, arg);
	}
}

void
yellow_corpse(arg, timeout)
genericptr_t arg;
long timeout;
{
	int pmtype, oldtyp, oldquan;
	struct obj *body = (struct obj *) arg;
	
	pmtype = body->corpsenm;
	
	/* [ALI] Molds don't grow in adverse conditions.  If it ever
	 * becomes possible for molds to grow in containers we should
	 * check for iceboxes here as well.
	 */
	if ((body->where == OBJ_FLOOR || body->where==OBJ_BURIED) &&
	  (is_pool(body->ox, body->oy, FALSE) || is_lava(body->ox, body->oy) ||
	  is_ice(body->ox, body->oy)))
	pmtype = -1;
	
	if (pmtype != -1) {
		/* We don't want special case revivals */
		if (cant_create(&pmtype, TRUE))
			pmtype = -1; /* cantcreate might have changed it so change it back */
		else {
			body->corpsenm = pmtype;

			/* oeaten isn't used for hp calc here, and zeroing it 
			 * prevents eaten_stat() from worrying when you've eaten more
			 * from the corpse than the newly grown mold's nutrition
			 * value.
			 */
			body->oeaten = 0;

			/* [ALI] If we allow revive_corpse() to get rid of revived
			 * corpses from hero's inventory then we run into problems
			 * with unpaid corpses.
			 */
			if (body->where == OBJ_INVENT)
				body->quan++;
			oldquan = body->quan;
			if (revive_corpse(body, REVIVE_YELLOW)) {
				if (oldquan != 1) {		/* Corpse still valid */
				if (body->where == OBJ_INVENT) {
					useup(body);
					oldquan--;
				}
				}
				if (oldquan == 1)
				body = (struct obj *)0;	/* Corpse gone */
			}
		}
	}

	/* If revive_corpse succeeds, it handles the reviving corpse.
	 * If there was more than one corpse, or the revive failed,
	 * set the remaining corpse(s) to rot away normally.
	 * Revive_corpse handles genocides
	 */
	if (body) {
		body->owt = weight(body);
		(void) start_timer(250L - (monstermoves-peek_at_iced_corpse_age(body)),
			TIMER_OBJECT, ROT_CORPSE, arg);
	}
}

int
donull()
{
	static long lastreped = -13; // counter to tell if you recently tried to repair yourself/meditate
	u.unull = TRUE;
	int regen = 0;

	int *hp = (Upolyd) ? (&u.mh) : (&u.uhp);
	int *hpmax = (Upolyd) ? (&u.mhmax) : (&u.uhpmax);
	
	if ((*hp) < (*hpmax)){
		if (uclockwork) {
			if(lastreped < monstermoves-13) You("attempt to make repairs.");
			if(!rn2(15 - u.ulevel/2)){
				(*hp) += rnd(10);
				flags.botl = 1;
			}
			lastreped = monstermoves;
			regen = 1;
		} else if (uandroid && u.uen > 0) {
			(*hp) += u.ulevel/6+1;
			if(rn2(6) < u.ulevel%6) (*hp) += 1;
			u.uen--;
			flags.botl = 1;
			regen = 1;
		} 
		if (uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_HEALING){
			(*hp) += 1;
			regen = 2;
		}

		if ((*hp) >= (*hpmax) && regen > 0){
			if(uclockwork && lastreped == monstermoves){
				You("complete your repairs.");
				lastreped = -13;
			} else if (uandroid && regen == 1){
				You("finish regenerating.");
			} else if (regen == 2){
				Your("sword hums contentedly.");
			}
			stop_occupation();
			(*hp) = (*hpmax);
		}
	} else if (!Role_if(PM_MONK) && u.sealsActive&SEAL_EURYNOME && ++u.eurycounts>5) {
		// monks meditate & fast, increasing pw regen and lowering hunger rate while they haven't moved
		unbind(SEAL_EURYNOME,TRUE);
	}
	return MOVE_STANDARD;	/* Do nothing, but let other things happen */
}

#endif /* OVL3 */
#ifdef OVLB

STATIC_PTR int
wipeoff()
{
	if(u.ucreamed < 4)	u.ucreamed = 0;
	else			u.ucreamed -= 4;
	if (Blinded < 4)	Blinded = 0;
	else			Blinded -= 4;
	if (!Blinded) {
		pline("You've got the glop off.");
		u.ucreamed = 0;
		Blinded = 1;
		make_blinded(0L,TRUE);
		return MOVE_FINISHED_OCCUPATION;
	} else if (!u.ucreamed) {
		Your("%s feels clean now.", body_part(FACE));
		return MOVE_FINISHED_OCCUPATION;
	}
	return MOVE_STANDARD;		/* still busy */
}

int
dowipe()
{
	if(u.ucreamed)  {
		static NEARDATA char buf[39];

		Sprintf(buf, "wiping off your %s", body_part(FACE));
		set_occupation(wipeoff, buf, 0);
		/* Not totally correct; what if they change back after now
		 * but before they're finished wiping?
		 */
		return MOVE_STANDARD;
	}
	Your("%s is already clean.", body_part(FACE));
	return MOVE_STANDARD;
}

void
set_wounded_legs(side, timex)
register long side;
register int timex;
{
	/* KMH -- STEED
	 * If you are riding, your steed gets the wounded legs instead.
	 * You still call this function, but don't lose hp.
	 * Caller is also responsible for adjusting messages.
	 */

	if(!Wounded_legs) {
		ATEMP(A_DEX)--;
		flags.botl = 1;
	}

	if(!Wounded_legs || (HWounded_legs & TIMEOUT))
		HWounded_legs = timex;
	EWounded_legs = side;
	(void)encumber_msg();
}

void
heal_legs()
{
	if(Wounded_legs) {
		if (ATEMP(A_DEX) < 0) {
			ATEMP(A_DEX)++;
			flags.botl = 1;
		}

#ifdef STEED
		if (!u.usteed)
#endif
		{
			/* KMH, intrinsics patch */
			if((EWounded_legs & BOTH_SIDES) == BOTH_SIDES) {
			Your("%s feel somewhat better.",
				makeplural(body_part(LEG)));
		} else {
			Your("%s feels somewhat better.",
				body_part(LEG));
		}
		}
		HWounded_legs = EWounded_legs = 0;
	}
	(void)encumber_msg();
}

int
dowait()
{
	struct monst *mtmp;
	if (!getdir("Indicate pet that should wait, or '.' for all.")) return MOVE_CANCELLED;
	if(!(u.dx || u.dy)){
		You("order all your pets to wait for your return.");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mtame) mtmp->mwait = monstermoves;
		}
	}
	else if(isok(u.ux+u.dx, u.uy+u.dy)) {
		mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if(!mtmp){
			pline("There is no target there.");
			return MOVE_INSTANT;
		}
		if(mtmp->mtame){
			mtmp->mwait = monstermoves;
			You("order %s to wait for your return.", mon_nam(mtmp));
		}
	} else pline("There is no target there.");
	return MOVE_INSTANT;
}

int
docome()
{
	struct monst *mtmp;
	if (!getdir("Indicate pet that should come with you, or '.' for all.")) return MOVE_CANCELLED;
	if(!(u.dx || u.dy)){
		You("order all your pets to follow you.");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mtame) mtmp->mwait = 0;
		}
	}
	else if(isok(u.ux+u.dx, u.uy+u.dy)) {
		mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if(!mtmp){
			pline("There is no target there.");
			return MOVE_INSTANT;
		}
		if(mtmp->mtame){
			mtmp->mwait = 0;
			You("order %s to follow you.", mon_nam(mtmp));
		}
	} else pline("There is no target there.");
	return MOVE_INSTANT;
}


int
doattack()
{
	struct monst *mtmp;
	if (!getdir("Indicate pet that should engage in battle, or '.' for all.")) return MOVE_CANCELLED;
	if(!(u.dx || u.dy)){
		You("order all your pets to engage in battle.");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mtame){
				// if(mtmp->mretreat)
					// mtmp->mretreat = 0;
				// else
					mtmp->mpassive = 0;
			}
		}
	}
	else if(isok(u.ux+u.dx, u.uy+u.dy)) {
		mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if(!mtmp){
			pline("There is no target there.");
			return MOVE_INSTANT;
		}
		if(mtmp->mtame){
			// mtmp->mretreat = 0;
			mtmp->mpassive = 0;
			You("order %s to engage in battle.", mon_nam(mtmp));
		}
	} else pline("There is no target there.");
	return MOVE_INSTANT;
}


int
dopassive()
{
	struct monst *mtmp;
	if (!getdir("Indicate pet that should not engage foes, or '.' for all.")) return MOVE_CANCELLED;
	if(!(u.dx || u.dy)){
		You("order all your pets not to engage foes.");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mtame) mtmp->mpassive = 1;
		}
	}
	else if(isok(u.ux+u.dx, u.uy+u.dy)) {
		mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if(!mtmp){
			pline("There is no target there.");
			return MOVE_INSTANT;
		}
		if(mtmp->mtame){
			mtmp->mpassive = 1;
			You("order %s not to engage foes.", mon_nam(mtmp));
		}
	} else pline("There is no target there.");
	return MOVE_INSTANT;
}


int
dodropall()
{
	struct monst *mtmp;
	if (!getdir("Indicate pet that drop all non-worn gear, or '.' for all.")) return MOVE_CANCELLED;
	if(!(u.dx || u.dy)){
		You("order all your pets to drop their junk.");
		for(mtmp = fmon; mtmp; mtmp = mtmp->nmon){
			if(mtmp->mtame){
				boolean keep = TRUE;
				while(keep){
					keep = FALSE;
					for(struct obj *otmp = mtmp->minvent; otmp; otmp = otmp->nobj){
						if(!(otmp->owornmask)){
							obj_extract_and_unequip_self(otmp);
							mdrop_obj(mtmp, otmp, TRUE);
							keep = TRUE;
							break;
						}
					}
				}
			}
		}
	}
	else if(isok(u.ux+u.dx, u.uy+u.dy)) {
		mtmp = m_at(u.ux+u.dx, u.uy+u.dy);
		if(!mtmp){
			pline("There is no target there.");
			return MOVE_INSTANT;
		}
		if(mtmp->mtame){
			You("order %s to drop %s gear.", mon_nam(mtmp), mhis(mtmp));
			boolean keep = TRUE;
			while(keep){
				keep = FALSE;
				for(struct obj *otmp = mtmp->minvent; otmp; otmp = otmp->nobj){
					if(!(otmp->owornmask)){
						obj_extract_and_unequip_self(otmp);
						mdrop_obj(mtmp, otmp, TRUE);
						keep = TRUE;
						break;
					}
				}
			}
		}
	} else pline("There is no target there.");
	return MOVE_INSTANT;
}


int
dodownboy()
{
	u.peaceful_pets = TRUE;
	return MOVE_INSTANT;
}

int
dosickem()
{
	u.peaceful_pets = FALSE;
	return MOVE_INSTANT;
}

#endif /* OVLB */

/*do.c*/
