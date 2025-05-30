/*	SCCS Id: @(#)bones.c	3.4	2003/09/06	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985,1993. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "artifact.h"

extern char bones[];	/* from files.c */
#ifdef MFLOPPY
extern long bytes_counted;
#endif

STATIC_DCL boolean FDECL(no_bones_level_core, (d_level *, boolean));
STATIC_DCL boolean FDECL(no_bones_level, (d_level *));
STATIC_DCL void FDECL(goodfruit, (int));
STATIC_DCL void FDECL(resetobjs,(struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(drop_upon_death, (struct monst *, struct obj *, int, int));
STATIC_DCL void FDECL(sanitize_name, (char *namebuf));

STATIC_OVL boolean
no_bones_level_core(lev, recursed)
d_level *lev;
boolean recursed;
{
	extern d_level save_dlevel;		/* in do.c */
	s_level *sptr;
	branch * bptr;

	if (ledger_no(&save_dlevel)) assign_level(lev, &save_dlevel);

	return (boolean)(((sptr = Is_special(lev)) != 0 && !sptr->boneid)
		|| !dungeons[lev->dnum].boneid
			/* no bones in the true madman home level */
		|| (Role_if(PM_MADMAN) && qstart_level.dnum == lev->dnum && qlocate_level.dlevel == (lev->dlevel+1))
			/* no bones in the branch level TO the Quest (because this portal can be voided) */
		|| (!recursed && (bptr = Is_branchlev(lev)) && In_quest(branchlev_other_end(bptr, lev)))
			/* no bones in a branch level if the other end is nobones */
		|| (!recursed && (bptr = Is_branchlev(lev)) && no_bones_level_core(branchlev_other_end(bptr, lev), TRUE))
		   /* no bones in the invocation level               */
		|| (In_hell(lev) && lev->dlevel == dunlevs_in_dungeon(lev) - 1)
		);
}

/* wrapper for no_bones_level_core, so that it and only it can recursively call itself with recursed = TRUE */
STATIC_OVL boolean
no_bones_level(lev)
d_level * lev;
{
	return no_bones_level_core(lev, FALSE);
}

/* Call this function for each fruit object saved in the bones level: it marks
 * that particular type of fruit as existing (the marker is that that type's
 * ID is positive instead of negative).  This way, when we later save the
 * chain of fruit types, we know to only save the types that exist.
 */
STATIC_OVL void
goodfruit(id)
int id;
{
	register struct fruit *f;

	for(f=ffruit; f; f=f->nextf) {
		if(f->fid == -id) {
			f->fid = id;
			return;
		}
	}
}

STATIC_OVL void
resetobjs(ochain,restore)
struct obj *ochain;
boolean restore;
{
	struct obj *otmp;

	for (otmp = ochain; otmp; otmp = otmp->nobj) {
		if (!restore) {
			while (otmp && get_ox(otmp, OX_ESUM)) otmp = otmp->nobj;
			if (!otmp) break;
		}

		if (otmp->cobj)
		    resetobjs(otmp->cobj,restore);

		if (restore && get_ox(otmp, OX_ENAM))
			sanitize_name(ONAME(otmp));

		if (!restore && otmp->oartifact > NROFARTIFACTS) {
			/* randarts get cleared in bonesfiles */
			otmp->oartifact = 0;
		}
		
		if (((otmp->otyp != CORPSE || otmp->corpsenm < SPECIAL_PM)
			&& otmp->otyp != STATUE)
			&& (!otmp->oartifact ||
			   (restore && (art_already_exists(otmp->oartifact)
					|| (is_quest_artifact(otmp) && !In_quest(&u.uz)))))
		) {
			otmp->oartifact = 0;
			rem_ox(otmp, OX_ENAM);
		} else if (otmp->oartifact && restore) {
			/* Set appearance-based artifacts to their correct types in this game */
			if(otmp->oartifact == ART_RING_OF_THROR
			 || otmp->oartifact == ART_NARYA
			 || otmp->oartifact == ART_NENYA
			 || otmp->oartifact == ART_VILYA
			 || otmp->oartifact == ART_LOMYA
			 || otmp->oartifact == ART_MANTLE_OF_HEAVEN
			 || otmp->oartifact == ART_VESTMENT_OF_HELL
			 || otmp->oartifact == ART_CROWN_OF_THE_SAINT_KING
			 || otmp->oartifact == ART_HELM_OF_THE_DARK_LORD
			 || otmp->oartifact == ART_SHARD_FROM_MORGOTH_S_CROWN
			){
				otmp->otyp = artilist[otmp->oartifact].otyp;
			}
			artifact_exists(otmp,ONAME(otmp),TRUE);
			/* otmp was gifted to the deceased adventurer, not you who just found it */
			otmp->gifted = 0;
		}
		if (restore) {
			/* rings and wands' material and color should always match their description */
			if (otmp->oclass == RING_CLASS || otmp->oclass == WAND_CLASS){
				set_material_gm(otmp, objects[otmp->otyp].oc_material);
				set_object_color(otmp);
			}
			
			/* books' color should always match their description */
			if(otmp->oclass == SPBOOK_CLASS){
				set_object_color(otmp);
			}
			/*Make sure the life-saving effect is unused for this player*/
			if (otmp->oartifact == ART_EARTH_CRYSTAL
				|| otmp->oartifact == ART_FIRE_CRYSTAL
				|| otmp->oartifact == ART_WATER_CRYSTAL
				|| otmp->oartifact == ART_AIR_CRYSTAL
				|| otmp->oartifact == ART_BLACK_CRYSTAL
			) {
				otmp->oeroded3 = 0;
			}
		}
		if (!restore) {
			/* do not zero out o_ids for ghost levels anymore */

			if(objects[otmp->otyp].oc_uses_known) otmp->known = 0;
			otmp->dknown = otmp->bknown = 0;
			otmp->rknown = 0;
			otmp->sknown = 0;
			// otmp->ostolen = 0;
			otmp->invlet = 0;
			otmp->no_charge = 0;
			otmp->was_thrown = 0;

			if (otmp->otyp == SLIME_MOLD) goodfruit(otmp->spe);
#ifdef MAIL
			else if (otmp->otyp == SCR_MAIL) otmp->spe = 1;
#endif
			else if (otmp->otyp == EGG) otmp->spe = 0;
			else if (otmp->otyp == TIN) {
			    /* make tins of unique monster's meat be empty */
			    if (otmp->corpsenm >= LOW_PM &&
				    (mons[otmp->corpsenm].geno & G_UNIQ))
				otmp->corpsenm = NON_PM;
			} else if (otmp->otyp == CLAWED_HAND) {
				otmp->otyp = otmp->oartifact == ART_AMALGAMATED_SKIES ? TWO_HANDED_SWORD : CLUB;
			} else if (otmp->otyp == AMULET_OF_YENDOR) {
			    /* no longer the real Amulet */
			    otmp->otyp = FAKE_AMULET_OF_YENDOR;
			    curse(otmp);
			} else if (otmp->otyp == CANDELABRUM_OF_INVOCATION) {
			    if (otmp->lamplit)
				end_burn(otmp, TRUE);
			    otmp->otyp = WAX_CANDLE;
			    otmp->age = 50L;  /* assume used */
			    if (otmp->spe > 0)
				otmp->quan = (long)otmp->spe;
			    otmp->spe = 0;
			    otmp->owt = weight(otmp);
			    curse(otmp);
			} else if (otmp->otyp == BELL_OF_OPENING) {
			    otmp->otyp = BELL;
				set_material_gm(otmp, COPPER);
			    curse(otmp);
				fix_object(otmp);
			} else if (otmp->otyp == SPE_BOOK_OF_THE_DEAD) {
			    otmp->otyp = SPE_BLANK_PAPER;
				otmp->obj_color = objects[SPE_BLANK_PAPER].oc_color;
				remove_oprop(otmp, OPROP_TACTB);
			    curse(otmp);
			}
			if(is_lightsaber(otmp)){
				if(otmp->lamplit) lightsaber_deactivate(otmp,TRUE);
			}
			if (otmp->oartifact == ART_ANNULUS) { /*Convert the Annulus to an ordinary whatever*/
				otmp->oartifact = 0;
				rem_ox(otmp, OX_ENAM);
				otmp->owt = weight(otmp);
				//Hilt and focus gem
				if(!otmp->cobj)
				{
					struct obj *gem = mksobj(rn2(6) ? SAPPHIRE : AQUAMARINE, NO_MKOBJ_FLAGS);
					gem->quan = 1;
					gem->owt = weight(gem);
					add_to_container(otmp, gem);
					container_weight(otmp);
				}
				otmp->ovar1_lightsaberHandle = random_saber_hilt();
			}
			if (otmp->oartifact == ART_ILLITHID_STAFF || otmp->oartifact == ART_ELDER_CEREBRAL_FLUID) { /*Convert ACU special artis to oridnary items */
				otmp->oartifact = 0;
				rem_ox(otmp, OX_ENAM);
				otmp->owt = weight(otmp);
			}
			if (otmp->oartifact == ART_HAND_OF_VECNA ||
				otmp->oartifact == ART_EYE_OF_VECNA
			) { /*Convert the Vecna artifacts to an ordinary whatever*/
				otmp->oartifact = 0;
				rem_ox(otmp, OX_ENAM);
				otmp->owt = weight(otmp);
			}
			//Vibroblades and force pikes: Ok for bones inclusion
			//Basic Firearms: Ok for bones inclusion
			//Blasters: Max out the recharge counter and halve remaining shots
			//Raygun: Max out the recharge counter and halve remaining shots
			//Plasteel Armor: Ok for bones inclusion
			//Sensors and Hyposprays: Ok for bones inclusion
			if(otmp->otyp == HAND_BLASTER || otmp->otyp == ARM_BLASTER || otmp->otyp == RAYGUN){
			    otmp->ovar1_charges /= 2;
			    otmp->recharged = 4;
				curse(otmp);
			//Actually, make it so these can barely be recharged/don't work, but let them be saved
			// } else if(otmp->otyp == RAYGUN || otmp->otyp == BULLET_FABBER){
			    // otmp->otyp = SUBETHAIC_MECHANISM;
			    // otmp->spe = 0;
				// otmp->owt = weight(otmp);
			}
		}
	}
}

 /* while loading bones, strip out text possibly supplied by old player
    that might accidentally or maliciously disrupt new player's display */
void
sanitize_name(namebuf)
char *namebuf;
{
	int c;
	boolean strip_8th_bit = (!strcmp(windowprocs.name, "tty")
						  && !iflags.wc_eight_bit_input);

	/* it's tempting to skip this for single-user platforms, since
	only the current player could have left these bones--except
	things like "hearse" and other bones exchange schemes make
	that assumption false */
	while (*namebuf) {
		c = *namebuf & 0177;
		if (c < ' ' || c == '\177') {
			/* non-printable or undesirable */
			*namebuf = '.';
		} else if (c != *namebuf) {
			/* expected to be printable if user wants such things */
			if (strip_8th_bit)
				*namebuf = '_';
		}
		++namebuf;
	}
}

STATIC_OVL void
drop_upon_death(mtmp, cont, x, y)
struct monst *mtmp;
struct obj *cont;
int x;
int y;
{
	struct obj *otmp;
	int thoughttries = 0;

	uswapwep = 0; /* ensure curse() won't cause swapwep to drop twice */
	while ((otmp = invent) != 0) {
		obj_extract_self(otmp);
		obj_no_longer_held(otmp);

		otmp->owornmask = 0;
		if(u.ugrave_arise == (NON_PM - 3)) set_material(otmp, GOLD);
		if(u.ugrave_arise == (NON_PM - 4)) set_material(otmp, SALT);
		/* lamps don't go out when dropped */
		if ((cont || artifact_light(otmp)) && obj_is_burning(otmp))
		    end_burn(otmp, TRUE);	/* smother in statue */

		if(otmp->otyp == SLIME_MOLD) goodfruit(otmp->spe);

		if(rn2(5)) curse(otmp);
		if (mtmp)
			(void) add_to_minv(mtmp, otmp);
		else if (cont)
			(void) add_to_container(cont, otmp);
		else if (isok(x, y))
			place_object(otmp, x, y);
	}
	while(u.thoughts && thoughttries++ < 5){
		otmp = 0;
		/* it's not necessary to properly remove the thoughts; the player is dead */
		if(u.thoughts & ANTI_CLOCKWISE_METAMORPHOSIS){
			u.thoughts &= ~ANTI_CLOCKWISE_METAMORPHOSIS;
			otmp = mksobj(ANTI_CLOCKWISE_METAMORPHOSIS_G, MKOBJ_NOINIT);
		} else if(u.thoughts & CLOCKWISE_METAMORPHOSIS){
			u.thoughts &= ~CLOCKWISE_METAMORPHOSIS;
			otmp = mksobj(CLOCKWISE_METAMORPHOSIS_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & ARCANE_BULWARK){
			u.thoughts &= ~ARCANE_BULWARK;
			otmp = mksobj(SPARKLING_LAKE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & DISSIPATING_BULWARK){
			u.thoughts &= ~DISSIPATING_BULWARK;
			otmp = mksobj(FADING_LAKE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & SMOLDERING_BULWARK){
			u.thoughts &= ~SMOLDERING_BULWARK;
			otmp = mksobj(SMOKING_LAKE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & FROSTED_BULWARK){
			u.thoughts &= ~FROSTED_BULWARK;
			otmp = mksobj(FROSTED_LAKE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & BLOOD_RAPTURE){
			u.thoughts &= ~BLOOD_RAPTURE;
			otmp = mksobj(RAPTUROUS_EYE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & CLAWMARK){
			u.thoughts &= ~CLAWMARK;
			otmp = mksobj(CLAWMARK_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & CLEAR_DEEPS){
			u.thoughts &= ~CLEAR_DEEPS;
			otmp = mksobj(CLEAR_SEA_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & DEEP_SEA){
			u.thoughts &= ~DEEP_SEA;
			otmp = mksobj(DEEP_SEA_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & TRANSPARENT_SEA){
			u.thoughts &= ~TRANSPARENT_SEA;
			otmp = mksobj(HIDDEN_SEA_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & COMMUNION){
			u.thoughts &= ~COMMUNION;
			otmp = mksobj(COMMUNION_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & CORRUPTION){
			u.thoughts &= ~CORRUPTION;
			otmp = mksobj(CORRUPTION_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & EYE_THOUGHT){
			u.thoughts &= ~EYE_THOUGHT;
			otmp = mksobj(EYE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & FORMLESS_VOICE){
			u.thoughts &= ~FORMLESS_VOICE;
			otmp = mksobj(FORMLESS_VOICE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & GUIDANCE){
			u.thoughts &= ~GUIDANCE;
			otmp = mksobj(GUIDANCE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & IMPURITY){
			u.thoughts &= ~IMPURITY;
			otmp = mksobj(IMPURITY_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & MOON){
			u.thoughts &= ~MOON;
			otmp = mksobj(MOON_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & WRITHE){
			u.thoughts &= ~WRITHE;
			otmp = mksobj(WRITHE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & RADIANCE){
			u.thoughts &= ~RADIANCE;
			otmp = mksobj(RADIANCE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & BEASTS_EMBRACE){
			u.thoughts &= ~BEASTS_EMBRACE;
			otmp = mksobj(BEAST_S_EMBRACE_GLYPH, MKOBJ_NOINIT);
		} else if(u.thoughts & SIGHT){
			u.thoughts &= ~SIGHT;
			otmp = mksobj(ORRERY_GLYPH, MKOBJ_NOINIT);
		//Philosophy runes do not death-drop
		} else if(u.thoughts & DEFILEMENT){
			u.thoughts &= ~DEFILEMENT;
		} else if(u.thoughts & LUMEN){
			u.thoughts &= ~LUMEN;
		} else if(u.thoughts & ROTTEN_EYES){
			u.thoughts &= ~ROTTEN_EYES;
		} else {
			pline("Can't find glyph!");
		}
		if(otmp){
			if(Race_if(PM_ANDROID)){
				set_material_gm(otmp, PLASTIC);
				fix_object(otmp);
			}
			if(Race_if(PM_CLOCKWORK_AUTOMATON)){
				set_material_gm(otmp, COPPER);
				fix_object(otmp);
			}
			if(Race_if(PM_WORM_THAT_WALKS)){
				set_material_gm(otmp, SHELL_MAT);
				fix_object(otmp);
			}
			if(rn2(5)) curse(otmp);
			if (mtmp)
				(void) add_to_minv(mtmp, otmp);
			else if (cont)
				(void) add_to_container(cont, otmp);
			else
				place_object(otmp, x, y);
		}
	}
	scatter_seals();
#ifndef GOLDOBJ
	if(u.ugold) {
		long ugold = u.ugold;
		if (mtmp) mtmp->mgold = ugold;
		else if (cont) (void) add_to_container(cont, mkgoldobj(ugold));
		else (void)mkgold_core(ugold, x, y, FALSE);
		u.ugold = ugold;	/* undo mkgoldobj()'s removal */
	}
#endif
	if (cont) cont->owt = weight(cont);
}

/* check whether bones are feasible */
boolean
can_make_bones()
{
	register struct trap *ttmp;

	if (ledger_no(&u.uz) <= 0 || ledger_no(&u.uz) > maxledgerno())
	    return FALSE;
	if (no_bones_level(&u.uz))
	    return FALSE;		/* no bones for specific levels */
	if (u.uswallow) {
	    return FALSE;		/* no bones when swallowed */
	}

	if(depth(&u.uz) <= 0 ||		/* bulletproofing for endgame */
	   (!rn2(1 + (depth(&u.uz)>>2))	/* fewer ghosts on low levels */
#ifdef WIZARD
		&& !wizard
#endif
		)) return FALSE;
	/* don't let multiple restarts generate multiple copies of objects
	 * in bones files */
	if (discover) return FALSE;
	return TRUE;
}

/* save bones and possessions of a deceased adventurer */
void
savebones(corpse)
struct obj *corpse;
{
	int fd, x, y, angelnum=0;
	struct trap *ttmp;
	struct monst *mtmp;
	struct permonst *mptr;
	struct fruit *f;
	char c, *bonesid;
	char whynot[BUFSZ];

	/* caller has already checked `can_make_bones()' */

	clear_bypasses();
	fd = open_bonesfile(&u.uz, &bonesid);
	if (fd >= 0) {
		(void) close(fd);
		compress_bonesfile();
#ifdef WIZARD
		if (wizard) {
		    if (yn("Bones file already exists.  Replace it?") == 'y') {
			if (delete_bonesfile(&u.uz)) goto make_bones;
			else pline("Cannot unlink old bones.");
		    }
		}
#endif
		return;
	}

#ifdef WIZARD
 make_bones:
#endif
	unleash_all();
	/* in case these characters are not in their home bases */
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
	    if (DEADMONSTER(mtmp)) continue;
	    mptr = mtmp->data;
	    if (mtmp->iswiz || mptr->mtyp == PM_MEDUSA ||
		    mptr->msound == MS_NEMESIS || mptr->msound == MS_LEADER ||
			quest_status.leader_m_id == mtmp->m_id || 
		    mptr->mtyp == PM_VLAD_THE_IMPALER || 
		    is_keter(mptr) || 
		    mptr->mtyp == PM_HOUND_OF_TINDALOS || 
		    mptr->mtyp == PM_CLAIRVOYANT_CHANGED || 
			(mptr->mtyp == PM_DREAD_SERAPH && !(mtmp->mstrategy & STRAT_WAITFORU)) || 
			(mptr->mtyp == PM_WEEPING_ANGEL && angelnum > 0) || 
			(is_drow(mptr) && mtmp->mfaction == LOST_HOUSE) ||
			(is_dprince(mptr) && !Inhell) || 
			(is_dlord(mptr) && !Inhell) ||
			(get_mx(mtmp, MX_ESUM))
		) mongone(mtmp);
		if(mptr->mtyp == PM_WEEPING_ANGEL) angelnum++;
	}
#ifdef STEED
	if (u.usteed) dismount_steed(DISMOUNT_BONES);
#endif
	dmonsfree();		/* discard dead or gone monsters */

	/* mark all fruits as nonexistent; when we come to them we'll mark
	 * them as existing (using goodfruit())
	 */
	for(f=ffruit; f; f=f->nextf) f->fid = -f->fid;

	/* check iron balls separately--maybe they're not carrying it */
	if (uball) uball->owornmask = uchain->owornmask = 0;

	/* Hero is no longer on the map. Needs to be done before makemons. */
	x = u.ux;
	y = u.uy;
	u.ux = 0;
	u.uy = 0;
	
	/* dispose of your possessions, usually cursed */
	if (u.ugrave_arise == (NON_PM - 1)) {
		struct obj *otmp;

		/* embed your possessions in your statue */
		otmp = mk_named_object(STATUE, &mons[u.umonnum],
				       x, y, plname);

		drop_upon_death((struct monst *)0, otmp, x, y);
		if (!otmp) return;	/* couldn't make statue */
		mtmp = (struct monst *)0;
	} else if (u.ugrave_arise == (NON_PM - 3)) {
		struct obj *otmp;

		/* embed your possessions in your statue */
		otmp = mk_named_object(STATUE, &mons[u.umonnum],
				       x, y, plname);
		set_material_gm(otmp, GOLD);
		drop_upon_death((struct monst *)0, otmp, x, y);
		if (!otmp) return;	/* couldn't make statue */
		mtmp = (struct monst *)0;
	} else if (u.ugrave_arise == (NON_PM - 4)) {
		struct obj *otmp;

		/* embed your possessions in your statue */
		otmp = mk_named_object(STATUE, &mons[u.umonnum],
				       x, y, plname);
		set_material_gm(otmp, SALT);
		drop_upon_death((struct monst *)0, otmp, x, y);
		if (!otmp) return;	/* couldn't make statue */
		mtmp = (struct monst *)0;
	} else if (u.ugrave_arise == (NON_PM - 5)) {
		struct obj *otmp;

		/* embed your possessions in your statue */
		otmp = mk_named_object(STATUE, &mons[u.umonnum],
				       x, y, plname);
		set_material_gm(otmp, GLASS);
		drop_upon_death((struct monst *)0, otmp, x, y);
		if (!otmp) return;	/* couldn't make statue */
		mtmp = (struct monst *)0;
	} else if (Race_if(PM_VAMPIRE)) {
		/* don't let vampires rise as some other monsters */
		drop_upon_death((struct monst *)0, (struct obj *)0, x, y);
		mtmp = (struct monst *)0;
		u.ugrave_arise = NON_PM;
	} else if (u.ugrave_arise < LOW_PM) {
		/* drop everything */
		drop_upon_death((struct monst *)0, (struct obj *)0, x, y);
		/* trick makemon() into allowing monster creation
		 * on your location
		 */
		in_mklev = TRUE;
		mtmp = makemon(&mons[PM_GHOST], x, y, MM_NONAME);
		in_mklev = FALSE;
		if (!mtmp) return;
		mtmp = christen_monst(mtmp, plname);
		if (corpse)
			(void) obj_attach_mid(corpse, mtmp->m_id); 
	} else {
		/* give your possessions to the monster you become */
		if(u.ugrave_arise == PM_ZOMBIE){
			u.ugrave_arise = (u.mfemale && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum;
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
			if(mtmp)
				set_template(mtmp, ZOMBIFIED);
		} else if(u.ugrave_arise == PM_SKELETON){
			u.ugrave_arise = (u.mfemale && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum;
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
			if(mtmp)
				set_template(mtmp, SKELIFIED);
		} else if(u.ugrave_arise == PM_BAALPHEGOR){
			u.ugrave_arise = (u.mfemale && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum;
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
			if(mtmp)
				set_template(mtmp, CRYSTALFIED);
		} else if(u.ugrave_arise == PM_ANCIENT_OF_CORRUPTION){
			u.ugrave_arise = (u.mfemale && urace.femalenum != NON_PM) ? urace.femalenum : urace.malenum;
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
			if(mtmp)
				set_template(mtmp, SLIME_REMNANT);
		} else if(u.ugrave_arise == PM_VAMPIRE){
			u.ugrave_arise = (u.mfemale && urole.femalenum != NON_PM) ? urole.femalenum : urole.malenum;
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
			if(mtmp)
				set_template(mtmp, VAMPIRIC);
		} else {
			in_mklev = TRUE;
			mtmp = makemon(&mons[u.ugrave_arise], x, y, NO_MM_FLAGS);
			in_mklev = FALSE;
		}
		if (!mtmp) {
			drop_upon_death((struct monst *)0, (struct obj *)0, x, y);
			return;
		}
		mtmp = christen_monst(mtmp, plname);
		newsym(x, y);
		Your("body rises from the dead as %s%s...",
			an(mons[u.ugrave_arise].mname),
			has_template(mtmp, ZOMBIFIED) ? " zombie" :
			has_template(mtmp, SKELIFIED) ? " skeleton" :
			has_template(mtmp, CRYSTALFIED) ? " vitrean" :
			has_template(mtmp, SLIME_REMNANT) ? " slimy remnant" :
			""
			);
		display_nhwindow(WIN_MESSAGE, FALSE);
		drop_upon_death(mtmp, (struct obj *)0, x, y);
		m_dowear(mtmp, TRUE);
		init_mon_wield_item(mtmp);
		m_level_up_intrinsic(mtmp);
	}
	if (mtmp) {
		mtmp->m_lev = (u.ulevel ? u.ulevel : 1);
		mtmp->mhp = mtmp->mhpmax = u.uhpmax;
		mtmp->female = flags.female;
		mtmp->msleeping = 1;
	}
	for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		resetobjs(mtmp->minvent,FALSE);
		/* do not zero out m_ids for bones levels any more */
		mtmp->mlstmv = 0L;
		if(mtmp->mtame) untame(mtmp, 0);
	}
	for(ttmp = ftrap; ttmp; ttmp = ttmp->ntrap) {
		ttmp->madeby_u = 0;
		ttmp->tseen = (ttmp->ttyp == HOLE);
	}
	if (Role_if(PM_RANGER) && Race_if(PM_GNOME) && on_level(&u.uz, &minetown_level)) {
		levl[sstairs.sx][sstairs.sy].typ = ROOM;
		sstairs.sx = sstairs.sy = 0;
	}
	resetobjs(fobj,FALSE);
	resetobjs(level.buriedobjlist, FALSE);

	/* Clear all memory from the level. */
	for(x=0; x<COLNO; x++) for(y=0; y<ROWNO; y++) {
	    levl[x][y].seenv = 0;
	    levl[x][y].waslit = 0;
	    levl[x][y].glyph = cmap_to_glyph(S_stone);
	}

	fd = create_bonesfile(&u.uz, &bonesid, whynot);
	if(fd < 0) {
#ifdef WIZARD
		if(wizard)
			pline("%s", whynot);
#endif
		/* bones file creation problems are silent to the player.
		 * Keep it that way, but place a clue into the paniclog.
		 */
		paniclog("savebones", whynot);
		return;
	}
	c = (char) (strlen(bonesid) + 1);

#ifdef MFLOPPY  /* check whether there is room */
	if (iflags.checkspace) {
	    savelev(fd, ledger_no(&u.uz), COUNT_SAVE);
	    /* savelev() initializes bytes_counted to 0, so it must come
	     * first here even though it does not in the real save.  the
	     * resulting extra bflush() at the end of savelev() may increase
	     * bytes_counted by a couple over what the real usage will be.
	     *
	     * note it is safe to call store_version() here only because
	     * bufon() is null for ZEROCOMP, which MFLOPPY uses -- otherwise
	     * this code would have to know the size of the version
	     * information itself.
	     */
	    store_version(fd);
	    bwrite(fd, (genericptr_t) &c, sizeof c);
	    bwrite(fd, (genericptr_t) bonesid, (unsigned) c);	/* DD.nnn */
	    savefruitchn(fd, COUNT_SAVE);
	    bflush(fd);
	    if (bytes_counted > freediskspace(bones)) { /* not enough room */
# ifdef WIZARD
		if (wizard)
			pline("Insufficient space to create bones file.");
# endif
		(void) close(fd);
		cancel_bonesfile();
		return;
	    }
	    co_false();	/* make sure stuff before savelev() gets written */
	}
#endif /* MFLOPPY */

	store_version(fd);
	bwrite(fd, (genericptr_t) &c, sizeof c);
	bwrite(fd, (genericptr_t) bonesid, (unsigned) c);	/* DD.nnn */
	savefruitchn(fd, WRITE_SAVE | FREE_SAVE);
	update_mlstmv();	/* update monsters for eventual restoration */
	savelev(fd, ledger_no(&u.uz), WRITE_SAVE | FREE_SAVE);
	bclose(fd);
	commit_bonesfile(&u.uz);
	compress_bonesfile();
}

int
getbones()
{
	register int fd;
	register int ok;
	char c, *bonesid, oldbonesid[10];

	if(discover)		/* save bones files for real games */
		return(0);

	/* wizard check added by GAN 02/05/87 */
	if(rn2(3)	/* only once in three times do we find bones */
#ifdef WIZARD
		&& !wizard
#endif
		) return(0);

	if (!iflags.bones) return(0);
	if(no_bones_level(&u.uz)) return(0);
	fd = open_bonesfile(&u.uz, &bonesid);
	if (fd < 0) return(0);

	if ((ok = uptodate(fd, bones)) == 0) {
#ifdef WIZARD
	    if (!wizard)
#endif
		pline("Discarding unuseable bones; no need to panic...");
	} else {
#ifdef WIZARD
		if(wizard)  {
			if(yn("Get bones?") == 'n') {
				(void) close(fd);
				compress_bonesfile();
				return(0);
			}
		}
#endif
		mread(fd, (genericptr_t) &c, sizeof c);	/* length incl. '\0' */
		mread(fd, (genericptr_t) oldbonesid, (unsigned) c); /* DD.nnn */
		if (strcmp(bonesid, oldbonesid) != 0) {
			char errbuf[BUFSZ];

			Sprintf(errbuf, "This is bones level '%s', not '%s'!",
				oldbonesid, bonesid);
#ifdef WIZARD
			if (wizard) {
				pline("%s", errbuf);
				ok = FALSE;	/* won't die of trickery */
			}
#endif
			trickery(errbuf);
		} else {
			register struct monst *mtmp;

			getlev(fd, 0, 0, TRUE);

			/* Note that getlev() now keeps tabs on unique
			 * monsters such as demon lords, and tracks the
			 * birth counts of all species just as makemon()
			 * does.  If a bones monster is extinct or has been
			 * subject to genocide, their mhpmax will be
			 * set to the magic DEFUNCT_MONSTER cookie value.
			 */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
				if (M_HAS_NAME(mtmp))
					sanitize_name(MNAME(mtmp));
			    if (mtmp->mhpmax == DEFUNCT_MONSTER) {
#if defined(DEBUG) && defined(WIZARD)
				if (wizard)
				    pline("Removing defunct monster %s from bones.",
					mtmp->data->mname);
#endif
				mongone(mtmp);
			    } else
				/* to correctly reset named artifacts on the level */
				resetobjs(mtmp->minvent,TRUE);
			}
			resetobjs(fobj,TRUE);
			resetobjs(level.buriedobjlist,TRUE);
			has_loaded_bones = 1;
		}
	}
	(void) close(fd);

#ifdef WIZARD
	if(wizard) {
		if(yn("Unlink bones?") == 'n') {
			compress_bonesfile();
			return(ok);
		}
	}
#endif
	if (!delete_bonesfile(&u.uz)) {
		/* When N games try to simultaneously restore the same
		 * bones file, N-1 of them will fail to delete it
		 * (the first N-1 under AmigaDOS, the last N-1 under UNIX).
		 * So no point in a mysterious message for a normal event
		 * -- just generate a new level for those N-1 games.
		 */
		/* pline("Cannot unlink bones."); */
		return(0);
	}
	return(ok);
}

/*bones.c*/
