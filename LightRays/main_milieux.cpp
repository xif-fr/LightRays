/********************************************************************************
 * Scène de test des objets réfléchissants & réfractants (ObjetMilieux) :
 * réflexion interne totale, séparation des couleurs (indice variable), lentille
 * réelle "épaisse", miroirs, etc…
 ********************************************************************************/

#include "SceneTest.h"
#include "ObjetMilieux.h"

int main (int, char const**) {
	
	Scene_TestCommon scene (L"Réfraction et milieux", /*ecran image*/true, /*lentille image*/false);
	scene.creer_ecrans_autour(/*lumino*/0.001, /*épaisseur*/0.02);
	
	/// --------- Sources ---------
	
	auto source_unique = scene.creer_source_unique_rayon(point_t{0.1,0.3}, /*dir*/0, /*ampl*/400.);
	source_unique->spectre = Specte::polychromatique({{8,8,17,22}});
	
	auto source_omni = scene.creer_source_omni_secteur(point_t{0.34722,0.50972}, /*ang_ext*/0.1, /*ang_base*/0, 10000);
	
	/// --------- Lame ---------
	
	std::vector<point_t> pts = { {0,0}, {0.2,0}, {0.2,0.03}, {0,0.03} };
	pts = rotate_points({0.2,0.1}, 0.4*M_PI, pts);
	pts = translate_points({0.2,0.35}, pts);
	auto lame = std::make_shared<ObjetComposite_LignesMilieu>(pts, 2.4);
	scene.objets.push_back(lame);
	scene.ajouter_bouge_action(pts[0], [&] (point_t mouse, float angle, bool alt) -> point_t {
		lame->re_positionne(mouse);
		return mouse;
	});
	
	/// --------- Dioptre à indice variable ---------
	
	std::function<float(float)> indice_refr_lambda = [] (float lambda) {
		return 1 + 0.1 * lambda / lambda_color[N_COULEURS/2];
	};
	auto dioptre = std::make_shared<ObjetLigne_Milieux>(indice_refr_lambda, point_t{0.8,0.25}, point_t{0.85,0.35});
	scene.objets.push_back(dioptre);
	scene.ajouter_bouge_action(dioptre->a, [&] (point_t mouse, float angle, bool alt) -> point_t {
		if (!alt) { dioptre->b = dioptre->b + (mouse - dioptre->a); dioptre->a = mouse; }
		else dioptre->b = mouse;
		return dioptre->a;
	});
	
	/// --------- Lentille réelle convergente ---------
	
	// Les deux dioptres en arc de cercle formant la lentille convergente
	scene.creer_objet<ObjetArc_Milieux>( 1.7, point_t{1.2,0.4}, point_t{1.2,0.6}, 0.3, false );
	scene.creer_objet<ObjetArc_Milieux>( 1.7, point_t{1.2,0.6}, point_t{1.2,0.4}, 0.3, false );
	// Diaphragme
	scene.creer_objet<Objet_Bloqueur>(point_t{1.3,0.52}, point_t{1.3,0.6});
	scene.creer_objet<Objet_Bloqueur>(point_t{1.3,0.48}, point_t{1.3,0.4});
	
	/// --------- Miroir ---------
	
	auto miroir = std::make_shared<ObjetLigne_Miroir>(point_t{0.2,0.45}, point_t{0.2,0.55});
	scene.objets.push_back(miroir);
	scene.ajouter_bouge_action(miroir->a, [&] (point_t mouse, float angle, bool alt) -> point_t {
		if (!alt) { miroir->b = miroir->b + (mouse - miroir->a); miroir->a = mouse; }
		else miroir->b = mouse;
		return miroir->a;
	});
	
	/// --------- Filtre ---------
	
	auto filtre_vert = scene.creer_objet<Objet_Filtre>((color_id_t)2, point_t{0.1,0.8}, point_t{0.2,0.8});
	scene.ajouter_bouge_action(filtre_vert->a, [&] (point_t mouse, float angle, bool alt) -> point_t {
		if (!alt) { filtre_vert->b = filtre_vert->b + (mouse - filtre_vert->a); filtre_vert->a = mouse; }
		else filtre_vert->b = mouse;
		return filtre_vert->a;
	});
	
	/// --------- Prisme ---------
	
	const std::vector<point_t> pts_triangle = { {1,0}, {cosf(2*M_PI/3),sinf(2*M_PI/3)}, {cosf(-2*M_PI/3),sinf(-2*M_PI/3)} };
	point_t prisme_position = {0.5, 0.8};
	float prisme_angle = 0;
	float prisme_taille = 0.1;
	auto prisme_scene_it = scene.objets.insert(scene.objets.end(), nullptr);
	auto prisme_move_action = [&] (point_t mouse, float angle, bool alt) -> point_t {
		if (alt) prisme_angle = angle;
		else prisme_position = mouse;
		auto pts_tri = homothetie_points({0,0}, prisme_taille, pts_triangle);
		pts_tri = rotate_points({0,0}, prisme_angle, pts_tri);
		pts_tri = translate_points((vec_t)prisme_position, pts_tri);
		*prisme_scene_it = std::make_shared<ObjetComposite_LignesMilieu>(pts_tri, indice_refr_lambda);
		return prisme_position;
	};
	prisme_move_action(prisme_position, 0, false);
	scene.ajouter_bouge_action(prisme_position, prisme_move_action);
	
	
	scene.boucle(nullptr, nullptr, nullptr);
	
	return 0;
}
