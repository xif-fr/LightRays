/********************************************************************************
 * Scène de test d'un brouillard diffusant, avec source "laser" et "en secteur",
 * avec différentes méthodes de diffusion et densité du brouillard variable
 ********************************************************************************/

#include "SceneTest.h"
#include "Brouillard.h"
#include "ObjetDiffusant.h"

int main (int, char const**) {
	
	Scene_TestCommon scene (L"Scène test brouillard", /*écran image*/true, /*lentille image*/true);
	
	scene.creer_ecrans_autour(/*lumino*/0.0005);
	scene.propag_rayons_dessin_gain = 30;
	scene.ecrans_do([] (Ecran_Base& e) { e.luminosite *= 20; });
	
	// Création des sources (et d'un fond bleu parce que pourquoi pas)
	auto source_unique = scene.creer_source_unique_rayon(point_t{1,0.445}, /*dir*/0.99*M_PI, /*ampl*/400.);
	auto source_omni = scene.creer_source_omni_secteur(point_t{0.7,0.60}, /*ang_ext*/0.1, /*ang_base*/1.05*M_PI, /*dens_ray*/10000);
	auto ciel = scene.creer_ciel_bleu(/*dens lin*/300);

	// Ancre de mise au point de la lentille à la souris, sur le bord droit du brouillard par défaut
	scene.ajouter_bouge_action(point_t{0.5,0.5}, [&] (point_t mouse, float angle, bool alt) -> point_t {
		scene.lentille_mise_au_point(mouse.x);
		return point_t{mouse.x,0.5};
	});
	scene.lentille_mise_au_point(0.5);
	
	// Création du brouillard lui même avec sa fonction densité(x,y)
	float dens_brouillard = 100;
	const float taille_brouill = 0.25;
	const float resol_brouill = 0.01;
	const size_t sz_brouill = taille_brouill/resol_brouill;
	auto brouillard = scene.creer_objet<Objet_Brouillard>(
		/* bottom right */    /* résolutions */         /* nombre de voxels */
		point_t{0.3,0.4}, resol_brouill, resol_brouill, sz_brouill, sz_brouill,
		/* fonction retournant la densité du brouillard en fonction de la position */
		[&] (uint ix, uint iy) -> float {
			float x = (ix-sz_brouill/2.)/(sz_brouill/2.);
			float y = (iy-sz_brouill/2.)/(sz_brouill/2.);
			return dens_brouillard * std::max(0.f, 1 - expf(x*x + y*y - 1));
		});
	brouillard->n_re_emit_par_intens = 2;
	brouillard->intens_cutoff = 0.2;
	bool diffus_meth_tot = true;
	
	scene.static_text.insert(scene.static_text.begin(), {
		L"[T/Y] brouillard moins/plus diffusant",
		L"[M] change méthode diffusion (totale/partielle)",
		L"[P] parcours brouillard déterministe ou non",
		L""
	});
	
	scene.boucle(
	/*f_event*/ [&] (sf::Event event) {
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::T) { // diminution de la densité du brouillard
			dens_brouillard *= 0.9;
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Y) { // augmentation de la densité du brouillard
			dens_brouillard *= 1.1;
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::M) { // changement de méthode de diffusion
			if (diffus_meth_tot) {
				diffus_meth_tot = false;
				brouillard->diffus_partielle_syst_libreparcours = resol_brouill;
				brouillard->n_re_emit_par_intens = 1;
			} else {
				diffus_meth_tot = true;
				brouillard->diffus_partielle_syst_libreparcours = Inf;
				brouillard->n_re_emit_par_intens = 3;
			}
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::P) { // changement de parcours du brouillard par les rayons, déterministe ou non
			brouillard->parcours_deterministe = !brouillard->parcours_deterministe;
			scene.reset_ecrans = true;
		}
	}, /*f_pre_propag*/ nullptr, /*f_post_propag*/ nullptr);

    return 0;
}
