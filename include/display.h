/*	SCCS Id: @(#)display.h	3.4	1999/11/30	*/
/* Copyright (c) Dean Luick, with acknowledgements to Kevin Darcy */
/* and Dave Cohrs, 1990.					  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef VISION_H
#include "vision.h"
#endif

#ifndef MONDATA_H
#include "mondata.h"	/* for mindless() */
#endif

#ifndef INVISIBLE_OBJECTS
#define vobj_at(x,y) (level.objects[x][y])
#endif

/*
 * sensemon()
 *
 * Returns true if the hero can sense the given monster.  This includes
 * monsters that are hiding or mimicing other monsters.
 */

#define etele_dist ((uarmh && uarmh->oartifact == ART_ENFORCED_MIND) ? (BOLT_LIM * BOLT_LIM * 4) : (BOLT_LIM * BOLT_LIM))
#define tp_sensemon(mon) (	/* The hero can always sense a monster IF:  */\
    (!mindless_mon(mon)) &&	/* 1. the monster has a brain to sense AND  */\
      ((Blind && Blind_telepat) ||	/* 2a. hero is blind and telepathic OR	    */\
				/* 2b. hero is using a telepathy inducing   */\
				/*	 object and in range		    */\
      (Unblind_telepat && (distu(mon->mx, mon->my) <= etele_dist))))
/* R'lyehian psychic sight, see minds, blocked by water */
#define rlyehian_sensemon(mon)						\
	(rlyehiansight(youracedata) && !mindless_mon(mon)		\
	 && (!is_pool(u.ux, u.uy, FALSE) || Flying || Levitation || Wwalking) \
	 && (!is_pool(mon->mx, mon->my, FALSE) || !is_underswimmer(mon->data) || \
	      mon_resistance(mon,FLYING) || mon_resistance(mon,LEVITATION)))

#define sensemon(mon) (tp_sensemon(mon) || Detect_monsters || MATCH_WARN_OF_MON(mon) || sense_by_scent(mon) || rlyehian_sensemon(mon))

/*
 * mon_warning() is used to warn of any dangerous monsters in your
 * vicinity, and a glyph representing the warning level is displayed.
 */
 
/*old
 define mon_warning(mon) (Warning && !(mon)->mpeaceful && 				\
			 (distu((mon)->mx, (mon)->my) < 100) &&				\
			 (((int) ((mon)->m_lev / 4)) >= flags.warnlevel))
*/

#define mon_warning(mon) ((Warning && !(mon)->mpeaceful &&         \
         (distu((mon)->mx, (mon)->my) < 100) &&        \
         (((int) ((mon)->m_lev / 4)) >= flags.warnlevel))    \
        || ((mon)->mtyp == PM_GREAT_CTHULHU) \
		)

/*
 * mon_visible()
 *
 * Returns true if the hero can see the monster.  It is assumed that the
 * hero can physically see the location of the monster.  The function
 * vobj_at() returns a pointer to an object that the hero can see there.
 * Infravision is not taken into account.
 */
#define mon_visible(mon) (		/* The hero can see the monster     */\
					/* IF the monster		    */\
    (!mon->minvis || See_invisible(mon->mx,mon->my)) &&	/* 1. is not invisible AND	    */\
    (!mon->mundetected) &&		/* 2. not an undetected hider	    */\
    (!(mon->mburied || u.uburied))	/* 3. neither you or it is buried   */\
)

/*
 * see_with_infrared()
 *
 * This function is true if the player can see a monster using infravision.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to infravision), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_infrared(mon) (!Blind && Infravision && infravisible_mon(mon) && (couldsee(mon->mx, mon->my) || Omnisense))

/*
 * see_with_bloodsense()
 *
 * This function is true if the player can see a monster using bloodsense.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to bloodsense), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_bloodsense(mon) (Bloodsense && !(Blind && ublindf && ublindf->otyp == R_LYEHIAN_FACEPLATE) && has_blood_mon(mon) && (couldsee((mon)->mx, (mon)->my) || Omnisense))

/*
 * see_with_lifesense()
 *
 * This function is true if the player can see a monster using lifesense.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to lifesense), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_lifesense(mon) (Lifesense && !(Blind && ublindf && ublindf->otyp == R_LYEHIAN_FACEPLATE) && !nonliving((mon)->data) && (couldsee((mon)->mx, (mon)->my) || Omnisense))

/*
 * see_with_senseall()
 *
 * This function is true if the player can see a monster using senseall.
 * The caller must check for invisibility (invisible monsters are also
 * invisible to senseall), because this is usually called from within
 * canseemon() or canspotmon() which already check that.
 */
#define see_with_senseall(mon) (Senseall && !nonliving((mon)->data) && (couldsee((mon)->mx, (mon)->my) || Omnisense))

/*
 * see_with_earthsense()
 *
 * This function is true if the player can see a monster using earthsense.
 * The caller must NOT check for invisibility.
 */
#define see_with_earthsense(mon) (Earthsense && !(mon_resistance((mon),FLYING) || mon_resistance((mon),LEVITATION) || unsolid((mon)->data)))


/*
 * canseemon()
 *
 * This is the globally used canseemon().  It is not called within the display
 * routines.  Like mon_visible(), but it checks to see if the hero sees the
 * location instead of assuming it.  (And also considers worms.)
 */
#define canseemon(mon) (((mon->wormno ? worm_known(mon) : \
	    (cansee(mon->mx, mon->my) || see_with_infrared(mon) || see_with_bloodsense(mon) || see_with_lifesense(mon) || see_with_senseall(mon))) \
	&& mon_visible(mon)) || see_with_earthsense(mon))

/*
 * canseemon_eyes()
 *
 * Variant of canseemon that checks for only non-magical (can be Blinded) vision.
 * Principally for use with Medusa, who's curse cares about you knowing what she
 * "looks like" in a superficial sense.
 */
#define canseemon_eyes(mon) ((mon->wormno ? worm_known(mon) : \
	    (cansee(mon->mx, mon->my) || see_with_infrared(mon))) \
	&& mon_visible(mon))


/*
 * canspotmon(mon)
 *
 * This function checks whether you can either see a monster or sense it by
 * telepathy, and is what you usually call for monsters about which nothing is
 * known.
 */
#define canspotmon(mon) \
	((mon) && (canseemon(mon) || sensemon(mon)))

/* knowninvisible(mon)
 * This one checks to see if you know a monster is both there and invisible.
 * 1) If you can see the monster and have see invisible, it is assumed the
 * monster is transparent, but visible in some manner.	(Earlier versions of
 * Nethack were really inconsistent on this.)
 * 2) If you can't see the monster, but can see its location and you have
 * telepathy that works when you can see, you can tell that there is a
 * creature in an apparently empty spot.
 * Infravision is not relevant; we assume that invisible monsters are also
 * invisible to infravision.
 */
#define knowninvisible(mon) \
	(mtmp->minvis && \
	    ((cansee(mon->mx, mon->my) && (See_invisible(mon->mx, mon->my) || Detect_monsters)) || \
		(!Blind && (HTelepat & ~INTRINSIC) && \
		    distu(mon->mx, mon->my) <= (BOLT_LIM * BOLT_LIM) \
		) \
	    ) \
	)

/*
 * is_safepet(mon)
 *
 * A special case check used in attack() and domove().	Placing the
 * definition here is convenient.
 */
#define is_safepet(mon) \
	(mon && (mon->mtame || mon->mpeaceful) && canspotmon(mon) && flags.safe_dog \
		&& !Confusion && !Hallucination && !Stunned)


/*
 * canseeself()
 * senseself()
 *
 * This returns true if the hero can see her/himself.
 *
 * The u.uswallow check assumes that you can see yourself even if you are
 * invisible.  If not, then we don't need the check.
 */
#define canseeself()	(Blind || u.uswallow || (!Invisible && !u.uundetected))
#define senseself()	(canseeself() || Unblind_telepat || Detect_monsters)

/*
 * random_monster()
 * random_object()
 * random_trap()
 *
 * Respectively return a random monster, object, or trap number.
 */
#define random_monster() rn2(NUMMONS)
#define random_object()  rn1(NUM_OBJECTS-1,1)
#define random_trap()	 rn1(TRAPNUM-1,1)

/*
 * what_obj()
 * what_mon()
 * what_trap()
 *
 * If hallucinating, choose a random object/monster, otherwise, use the one
 * given.
 */
#define what_obj(obj)	(Hallucination ? random_object()  : obj)
#define what_mon(mon, mtmp)	((Hallucination || Delusion(mtmp)) ? random_monster() : mon)
#define what_trap(trp)	(Hallucination ? random_trap()	  : trp)

/*
 * covers_objects()
 * covers_traps()
 *
 * These routines are true if what is really at the given location will
 * "cover" any objects or traps that might be there.
 */
#define covers_objects(xx,yy)						      \
    ((is_pool(xx,yy, FALSE) && !Underwater && !Is_waterlevel(&u.uz)) || (levl[xx][yy].typ == LAVAPOOL))

#define covers_traps(xx,yy)	covers_objects(xx,yy)


/*
 * tmp_at() control calls.
 */
#define DISP_BEAM    (-1)  /* Keep all glyphs showing & clean up at end. */
#define DISP_FLASH   (-2)  /* Clean up each glyph before displaying new one. */
#define DISP_ALWAYS  (-3)  /* Like flash, but still displayed if not visible. */
#define DISP_CHANGE  (-4)  /* Change glyph. */
#define DISP_END     (-5)  /* Clean up. */
#define DISP_FREEMEM (-6)  /* Free all memory during exit only. */


/* Total number of cmap indices in the sheild_static[] array. */
#define SHIELD_COUNT 7 //was 21


/*
 * display_self()
 *
 * Display the hero.  It is assumed that all checks necessary to determine
 * _if_ the hero can be seen have already been done.
 */
#ifdef STEED
#define maybe_display_usteed	(u.usteed && mon_visible(u.usteed)) ? \
					ridden_mon_to_glyph(u.usteed) :
#else
#define maybe_display_usteed	/* empty */
#endif

#define display_self()							\
    show_glyph(u.ux, u.uy,						\
	maybe_display_usteed			/* else */		\
	youmonst.m_ap_type == M_AP_NOTHING ?				\
				hero_glyph :				\
	youmonst.m_ap_type == M_AP_FURNITURE ?				\
				cmap_to_glyph(youmonst.mappearance) :	\
	youmonst.m_ap_type == M_AP_OBJECT ?				\
				objnum_to_glyph(youmonst.mappearance) : \
	/* else M_AP_MONSTER */ monnum_to_glyph(youmonst.mappearance))

/*
 * A glyph is an abstraction that represents a _unique_ monster, object,
 * dungeon part, or effect.  The uniqueness is important.  For example,
 * It is not enough to have four (one for each "direction") zap beam glyphs,
 * we need a set of four for each beam type.  Why go to so much trouble?
 * Because it is possible that any given window dependent display driver
 * [print_glyph()] can produce something different for each type of glyph.
 * That is, a beam of cold and a beam of fire would not only be different
 * colors, but would also be represented by different symbols.
 *
 * Glyphs are grouped for easy accessibility:
 *
 * monster	Represents all the wild (not tame) monsters.  Count: NUMMONS.
 *
 * pet		Represents all of the tame monsters.  Count: NUMMONS
 *
 * invisible	Invisible monster placeholder.	Count: 1
 *
 * detect	Represents all detected monsters.  Count: NUMMONS
 *
 * corpse	One for each monster.  Count: NUMMONS
 *
 * ridden	Represents all monsters being ridden.  Count: NUMMONS 
 *
 * object	A set of 16 for each (non-rogue) color for each
 *      object.  Count: NUM_OBJECTS*16
 *
 * cmap		One for each entry in the character map.  The character map
 *		is the dungeon features and other miscellaneous things.
 *		Count: MAXPCHARS
 *
 * explosions	A set of nine for each of the following seven explosion types:
 *                   dark, noxious, muddy, wet, magical, fiery, frosty.
 *              The nine positions represent those surrounding the hero.
 *		Count: MAXEXPCHARS * EXPL_MAX (EXPL_MAX is defined in hack.h)
 *
 * zap beam	A set of four (there are four directions) for each beam type.
 *		The beam type is shifted over 2 positions and the direction
 *		is stored in the lower 2 bits.	Count: NUM_ZAP << 2
 *
 * cloud	A unqiue cloud type
 *
 * swallow	A set of eight for each monster.  The eight positions rep-
 *		resent those surrounding the hero.  The monster number is
 *		shifted over 3 positions and the swallow position is stored
 *		in the lower three bits.  Count: NUMMONS << 3
 *
 * warning	A set of six representing the different warning levels.
 *
 * The following are offsets used to convert to and from a glyph.
 */
#define NUM_ZAP CLR_MAX	/* number of zap beam types */
#define NUM_CLOUDS NUM_AD_TYPES	/* number of cloud display types */

#define GLYPH_MON_OFF		0
#define GLYPH_PET_OFF		(NUMMONS	+ GLYPH_MON_OFF)
#define GLYPH_PEACE_OFF		(NUMMONS	+ GLYPH_PET_OFF)
#define GLYPH_MTEMPLATE_OFF	(NUMMONS	+ GLYPH_PEACE_OFF)
#define GLYPH_INVIS_OFF		(NUMMONS*MAXTEMPLATE	+ GLYPH_MTEMPLATE_OFF)
#define GLYPH_DETECT_OFF	(1		+ GLYPH_INVIS_OFF)
#define GLYPH_BODY_OFF		(NUMMONS	+ GLYPH_DETECT_OFF)
#define GLYPH_RIDDEN_OFF	(NUMMONS	+ GLYPH_BODY_OFF)
#define GLYPH_OBJ_OFF		(NUMMONS	+ GLYPH_RIDDEN_OFF)
#define GLYPH_CMAP_OFF		((NUM_OBJECTS << 4)	+ GLYPH_OBJ_OFF)
#define GLYPH_EXPLODE_OFF	((MAXPCHARS - MAXEXPCHARS) + GLYPH_CMAP_OFF)
#define GLYPH_ZAP_OFF		((MAXEXPCHARS * EXPL_MAX) + GLYPH_EXPLODE_OFF)
#define GLYPH_CLOUD_OFF		((NUM_ZAP << 2) + GLYPH_ZAP_OFF)
#define GLYPH_SWALLOW_OFF	((NUM_CLOUDS) + GLYPH_CLOUD_OFF)
#define GLYPH_WARNING_OFF	((NUMMONS << 3) + GLYPH_SWALLOW_OFF)
#define MAX_GLYPH		(WARNCOUNT      + GLYPH_WARNING_OFF)

#define NO_GLYPH MAX_GLYPH

#define GLYPH_INVISIBLE GLYPH_INVIS_OFF

#define warning_to_glyph(mwarnlev) ((mwarnlev)+GLYPH_WARNING_OFF)
#define cloud_to_glyph(adtyp) ((adtyp)+GLYPH_CLOUD_OFF)
#define mon_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_MON_OFF)
#define detected_mon_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_DETECT_OFF)
#define ridden_mon_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_RIDDEN_OFF)
#define pet_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_PET_OFF)
#define peace_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_PEACE_OFF)
#define mtemplate_to_glyph(mon) ((int) what_mon((mon)->mtyp, mon)+GLYPH_MTEMPLATE_OFF)

#define cmap_to_glyph(cmap_idx) ((int) (cmap_idx)   + GLYPH_CMAP_OFF)
#define explosion_to_glyph(expltype,idx)	\
		((((expltype) * MAXEXPCHARS) + ((idx) - S_explode1)) + GLYPH_EXPLODE_OFF)

#define trap_to_glyph(trap)	\
			cmap_to_glyph(trap_to_defsym(what_trap((trap)->ttyp)))

/* Not affected by hallucination.  Gives a generic body for CORPSE */
#define objnum_to_glyph(onum)	((int) ((onum) << 4) + objects[onum].oc_color + GLYPH_OBJ_OFF)
#define monnum_to_glyph(mtyp)	((int) (mtyp) + GLYPH_MON_OFF)
#define detected_monnum_to_glyph(mtyp)	((int) (mtyp) + GLYPH_DETECT_OFF)
#define ridden_monnum_to_glyph(mtyp)	((int) (mtyp) + GLYPH_RIDDEN_OFF)
#define petnum_to_glyph(mtyp)	((int) (mtyp) + GLYPH_PET_OFF)
#define peacenum_to_glyph(mtyp)	((int) (mtyp) + GLYPH_PEACE_OFF)
#define mtemplatenum_to_glyph(mtyp, mtemplate)	((int) (mtyp) + GLYPH_MTEMPLATE_OFF + NUMMONS*(mtemplate-1))

/* The hero's glyph when seen as a monster.
 */
#define hero_glyph \
	monnum_to_glyph((Upolyd || (!iflags.showrace && flags.altrace == NON_PM)) ? u.umonnum : \
	                (flags.female && urace.femalenum != NON_PM) ? urace.femalenum : \
	                urace.malenum)


/*
 * Change the given glyph into it's given type.  Note:
 *	1) Pets, detected, and ridden monsters are animals and are converted
 *	   to the proper monster number.
 *	2) Bodies are all mapped into the generic CORPSE object
 *	3) If handed a glyph out of range for the type, these functions
 *	   will return NO_GLYPH (see exception below)
 *	4) glyph_to_swallow() does not return a showsyms[] index, but an
 *	   offset from the first swallow symbol.  If handed something
 *	   out of range, it will return zero (for lack of anything better
 *	   to return).
 */
#define glyph_to_mon(glyph)						\
	(glyph_is_normal_monster(glyph) ? ((glyph)-GLYPH_MON_OFF) :	\
	glyph_is_pet(glyph) ? ((glyph)-GLYPH_PET_OFF) :			\
	glyph_is_peace(glyph) ? ((glyph)-GLYPH_PEACE_OFF) :			\
	glyph_is_mtemplate(glyph) ? (((glyph)-GLYPH_MTEMPLATE_OFF) % NUMMONS) :			\
	glyph_is_detected_monster(glyph) ? ((glyph)-GLYPH_DETECT_OFF) :	\
	glyph_is_ridden_monster(glyph) ? ((glyph)-GLYPH_RIDDEN_OFF) :	\
	NO_GLYPH)
#define glyph_to_obj(glyph)						\
	(glyph_is_body(glyph) ? CORPSE :				\
	glyph_is_normal_object(glyph) ? ((glyph-GLYPH_OBJ_OFF)>>4) :	\
	NO_GLYPH)
#define glyph_to_trap(glyph)						\
	(glyph_is_trap(glyph) ?						\
		((int) defsym_to_trap((glyph) - GLYPH_CMAP_OFF)) :	\
	NO_GLYPH)
#define glyph_to_cmap(glyph)						\
	(glyph_is_cmap(glyph) ? ((glyph) - GLYPH_CMAP_OFF) :		\
	NO_GLYPH)
#define glyph_to_cloud_type(glyph)						\
	(glyph_is_cloud(glyph) ? (((glyph) - GLYPH_CLOUD_OFF)) : \
	0)
#define glyph_to_swallow(glyph)						\
	(glyph_is_swallow(glyph) ? (((glyph) - GLYPH_SWALLOW_OFF) & 0x7) : \
	0)
#define glyph_to_warning(glyph)						\
	(glyph_is_warning(glyph) ? ((glyph) - GLYPH_WARNING_OFF) :	\
	NO_GLYPH);

/*
 * Return true if the given glyph is what we want.  Note that bodies are
 * considered objects.
 */
#define glyph_is_monster(glyph)						\
		(glyph_is_normal_monster(glyph)				\
		|| glyph_is_pet(glyph)					\
		|| glyph_is_peace(glyph)					\
		|| glyph_is_mtemplate(glyph)					\
		|| glyph_is_ridden_monster(glyph)			\
		|| glyph_is_detected_monster(glyph))
#define glyph_is_normal_monster(glyph)					\
    ((glyph) >= GLYPH_MON_OFF && (glyph) < (GLYPH_MON_OFF+NUMMONS))
#define glyph_is_pet(glyph)						\
    ((glyph) >= GLYPH_PET_OFF && (glyph) < (GLYPH_PET_OFF+NUMMONS))
#define glyph_is_peace(glyph)						\
    ((glyph) >= GLYPH_PEACE_OFF && (glyph) < (GLYPH_PEACE_OFF+NUMMONS))
#define glyph_is_mtemplate(glyph)						\
    ((glyph) >= GLYPH_MTEMPLATE_OFF && (glyph) < (GLYPH_MTEMPLATE_OFF+NUMMONS*MAXTEMPLATE))
#define glyph_is_body(glyph)						\
    ((glyph) >= GLYPH_BODY_OFF && (glyph) < (GLYPH_BODY_OFF+NUMMONS))
#define glyph_is_ridden_monster(glyph)					\
    ((glyph) >= GLYPH_RIDDEN_OFF && (glyph) < (GLYPH_RIDDEN_OFF+NUMMONS))
#define glyph_is_detected_monster(glyph)				\
    ((glyph) >= GLYPH_DETECT_OFF && (glyph) < (GLYPH_DETECT_OFF+NUMMONS))
#define glyph_is_invisible(glyph) ((glyph) == GLYPH_INVISIBLE)
#define glyph_is_normal_object(glyph)					\
    ((glyph) >= GLYPH_OBJ_OFF && (glyph) < (GLYPH_OBJ_OFF+(NUM_OBJECTS<<4)))
#define glyph_is_object(glyph)						\
		(glyph_is_normal_object(glyph)				\
		|| glyph_is_body(glyph))
#define glyph_is_trap(glyph)						\
    ((glyph) >= (GLYPH_CMAP_OFF+trap_to_defsym(1)) &&			\
     (glyph) <	(GLYPH_CMAP_OFF+trap_to_defsym(1)+TRAPNUM))
#define glyph_is_cmap(glyph)						\
    ((glyph) >= GLYPH_CMAP_OFF && (glyph) < (GLYPH_CMAP_OFF+MAXPCHARS))
#define glyph_is_cloud(glyph)	\
    ((glyph) >= GLYPH_CLOUD_OFF && (glyph) < (GLYPH_CLOUD_OFF + NUM_CLOUDS))
#define glyph_is_swallow(glyph) \
    ((glyph) >= GLYPH_SWALLOW_OFF && (glyph) < (GLYPH_SWALLOW_OFF+(NUMMONS << 3)))
#define glyph_is_warning(glyph)	\
    ((glyph) >= GLYPH_WARNING_OFF && (glyph) < (GLYPH_WARNING_OFF + WARNCOUNT))
#endif /* DISPLAY_H */
