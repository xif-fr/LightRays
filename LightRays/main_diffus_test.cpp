/********************************************************************************
 * Scène de test d'une surface diffusante avec une composante de rélfexion
 * lambertienne et une composante spéculaire en cos(θ)^n. Bilan d'énergie
 ********************************************************************************/

#include "SceneTest.h"
#include "ObjetDiffusant.h"
#include <sstream>
#include <iomanip>

int main (int, char const**) {
	
	Scene_TestCommon scene (L"Test objets diffusants", /*ecran image*/true, /*lentille image*/true);
	scene.creer_ecrans_autour(/*lumino*/ 0.0005, /*épaisseur*/ 0.025);
		
	float source_ext_ang = 0.1;
	auto source_omni = scene.creer_source_omni_secteur(point_t{0.3,0.66}, source_ext_ang, /*ang_base*/-0.22*M_PI, /*dens_ray*/60000);
	
	// création du plan diffusant
	auto panel_diffus = std::make_shared<ObjetLigne_Diffusant>(ObjetCourbe_Diffusant::BRDF_Lambert, point_t{0.45,0.51}, point_t{0.55,0.49});
	point_t panel_m = milieu_2points(panel_diffus->a,panel_diffus->b);
	scene.objets.push_back(panel_diffus);
	// mise au point au milieu du plan
	scene.lentille_mise_au_point(panel_m.x);
	// déplacement/rotation du plan à la souris
	scene.ajouter_bouge_action(panel_m, [&] (point_t mouse, float angle, bool alt) -> point_t {
		vec_t v = panel_diffus->b - panel_diffus->a;
		if (alt) {
			v = !v * vec2_t{ cosf(angle), sinf(angle) };
		} else {
			panel_m = mouse;
		}
		panel_diffus->a = panel_m +  v/2;
		panel_diffus->b = panel_m + -v/2;
		return panel_m;
	});
	// composante de réflexion en cos^n, où on se rapporche d'une réflexion très directive lorsque n augmente (spéculaire)
	int diffus_n = 3;
	// coeff de normalisation tel que l'intégrale de `cos^n / c` fasse 1
	// on vérifie qu'on a bien conservation de l'énergie avec `objet_bilan` ci-dessous
	auto diffus_c_calc = [] (int n) -> float {
		float c = 2;
		while (n >= 3) {
			c *= (n-1.f) / n;
			n -= 2;
		}
		return c;
	};
	float diffus_c = diffus_c_calc(diffus_n);
	float refl_spec = 1;
	// distribution angulaire de réflectance et ses deux composantes
	panel_diffus->BRDF_lambda = [&] (float theta_i, float theta_r, float lambda) -> float {
		// spectre de réflexion diffuse centrée autour de 450nm
		float f_col = (lambda-4.5e-4)/1e-4; f_col = expf(-f_col*f_col);
		return f_col * (1-refl_spec)  +  refl_spec * M_PI * powf( std::max(0.f, cosf(theta_i+theta_r)), diffus_n) / diffus_c;
		//     réflexion lambertienne    réflexion spéculaire (réflectivité de lobe cos^n)
	};
	// À améliorer : Il n'y a pas conservation de l'énergie lorsque le rayon incident n'est pas normal,
	// car il y a troncature de l'angle du rayon réfléchi sur [-pi/2,+pi/2] !
	
	// Bilan d'énergie pour vérifier si `diffus_c_calc` est correct.
	auto objet_bilan = scene.creer_objet<Objet_BilanEnergie>(point_t{0.5,0.5}, 0.2);
	
	// Diaphragme
	scene.creer_objet<Objet_Bloqueur>(point_t{1.3,0.53}, point_t{1.3,0.6});
	scene.creer_objet<Objet_Bloqueur>(point_t{1.3,0.47}, point_t{1.3,0.4});
	
	scene.static_text.insert(scene.static_text.begin(), {
		L"Surface diffusante avec une composante de rélfexion lambertienne (uniforme) et une composante spéculaire en cos(θ)^n",
		L"[W/X] augmente/diminue la directivité de la partie spéculaire de la réflexion",
		L"[C/V] diminue/augmente la partie spéculaire de la réflexion, vs lambertienne",
		L"[E/Z] élargit/rétrécit la source",
		L"[K] mode déterministe",
		L""
	});
	
	scene.boucle(
	/*f_event*/ [&] (sf::Event event) {
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::W) { // réflexion spéculaire plus piquée
			diffus_n = 2*diffus_n+1;
			diffus_c = diffus_c_calc(diffus_n);
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::X) { // réflexion spéculaire moins piquée
			if (diffus_n > 1)
				diffus_n = (diffus_n-1)/2;
			diffus_c = diffus_c_calc(diffus_n);
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::V) { // plus de réflexion spéculaire
			refl_spec = std::min<float>(1, refl_spec+0.02);
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::C) { // moins de réflexion spéculaire
			refl_spec = std::max<float>(0, refl_spec-0.02);
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::E) { // étend la source
			source_ext_ang *= 1.1;
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Z) { // rétrécit la source
			source_ext_ang *= 0.9;
			scene.reset_ecrans = true;
		}
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::K) { // mode déterministe (équirépartition des rayons émis)
			panel_diffus->diff_met = ObjetLigne_Diffusant::Equirep;
			panel_diffus->n_re_emit_par_intens = 300;
			source_omni->dens_ang = 600;
			source_omni->dir_alea = false;
			scene.ecrans_do([] (Ecran_Base& e) { e.luminosite *= 80; });
			scene.propag_rayons_dessin_gain *= 80;
			scene.reset_ecrans = true;
		}
	},
	/*f_pre_propag*/ [&] () {
		// réinitialisation de `objet_bilan`
		if (scene.reset_ecrans)
			objet_bilan->reset();
		// on fait toujours pointer la souce vers le centre du plan diffusant
		vec_t v = panel_m - source_omni->position;
		source_omni->secteur = angle_interv_t(-source_ext_ang,+source_ext_ang) + atan2f(v.y,v.x);
	},
	/*f_post_propag*/ [&] () {
		// affichage du bilan d'énergie
		objet_bilan->commit();
		auto stat = objet_bilan->bilan();
		float diff = stat.flux_in - stat.flux_out;
		std::wstringstream s;
		s << L"bilan énergie :" << std::endl;
		s << std::setprecision(1) << std::fixed;
		s << L"flux in " << stat.flux_in << ", flux out " << stat.flux_out << std::endl;
		s << std::setprecision(1);
		s << L"loss " << diff << ", loss rel " << 100*diff/stat.flux_in << std::endl;
		auto text = sf::c01::buildText(scene.font, (objet_bilan->c + objet_bilan->R*vec_t{+0.8,+0.9}), {s.str()}, sf::Color::White);
		scene.win_scene->draw(text);
	});

    return 0;
}
