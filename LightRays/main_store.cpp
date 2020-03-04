/********************************************************************************
 * Scène de test d'un store constitué d'arcs de cercles diffusants
 ********************************************************************************/

#include "SceneTest.h"
#include "ObjetDiffusant.h"

int main (int, char const**) {
	
	Scene_TestCommon scene (L"Soleil et store", /*ecran_image*/false, /*lentille image*/false);
	
	scene.creer_ecrans_autour(/*lumino*/0.001, /*épaisseur*/0.02);
	
	auto ciel = scene.creer_ciel_bleu();
	// Soleil modélisé par des rayons parallèles : source de type `Source_LinParallels`
	auto soleil = std::make_shared<Source_LinParallels>(point_t{0.1,0.65}, vec_t{0,0.3}, /*dir_angle*/-0.15*M_PI, spectre_blanc);
	soleil->dens_lin = 6000;
	scene.sources.push_back(soleil);
	
	// Store : ensemble d'arcs de cercles diffusants régulièrement espacés
	std::vector<std::shared_ptr<ObjetArc_Diffusant>> store;
	for (float y = 0.4; y < 0.95; y += 0.03) {
		auto arc = scene.creer_objet<ObjetArc_Diffusant>(
			ObjetCourbe_Diffusant::BRDF_Lambert,
			/*centre*/point_t{0.3,y}, /*rayon*/0.04, angle_interv_t(M_PI/4,M_PI/2), /*inv_int*/false
		);
		arc->n_re_emit_par_intens = 1;
		store.push_back(arc);
	}
	// Rotation des éléments du store avec la souris
	scene.ajouter_bouge_action(point_t{0.32,0.5}, [&] (point_t mouse, float angle, bool alt) -> point_t {
		for (auto arc : store)
			arc->ang = angle_interv_t(M_PI/4,M_PI/2) + (angle - M_PI/2);
		return point_t{0.32,0.5};
	});
	
	// Un sol diffusant lambertien
	auto sol = scene.creer_objet<ObjetLigne_Diffusant>(ObjetCourbe_Diffusant::BRDF_Lambert, point_t{0.3,0.3}, point_t{2,0.3});
	sol->n_re_emit_par_intens = 1;
	
	scene.static_text.insert(scene.static_text.begin(), {
		L"Store constitué d'arcs de cercles diffusants, dont l'angle est ajoutable en cliquant sur son ancre.",
		L"Le but est de minimiser la lumière éblouissante du soleil tout en maximisant la lumière diffusée (e.g. sur le plafond)",
		L""
	});
	
	scene.boucle(nullptr, nullptr, nullptr);
	
	return 0;
}
