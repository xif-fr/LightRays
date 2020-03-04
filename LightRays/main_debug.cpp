#include "SceneTest.h"
#include <fmt/core.h>

int main (int, char const**) {
	
	Scene_TestCommon scene (L"Debug", /*ecran_image*/true, /*lentille image*/true);
	scene.creer_ecrans_autour();
	
	auto source_omni_ponct = std::make_shared<Source_PonctOmni>(
		point_t{0.34306,0.48611}, Specte::polychromatique({{0.8,0.8,1.7,2.2}})
	);
	auto color = source_omni_ponct->spectre.rgb256_noir_intensite(false);
	fmt::print("({}, {}, {})", std::get<0>(color),std::get<1>(color),std::get<2>(color));
	source_omni_ponct->dens_ang = 500;
	source_omni_ponct->dir_alea = false;
	scene.sources.push_back(source_omni_ponct);
	scene.ajouter_bouge_action(source_omni_ponct->position, [&] (point_t mouse, float angle, bool alt) -> point_t {
		source_omni_ponct->position = mouse;
		return mouse;
	});
	
	auto source_sect = std::make_shared<Source_SecteurDisqueLambertien>(
		0.03, angle_interv_t(-M_PI/10, +M_PI/10)+M_PI, point_t{0.1,0.5}, Specte::polychromatique({{0.8,0.8,1.7,2.2}})
	);
	source_sect->dens_ang = 30000;
	source_sect->dir_alea = true;
	scene.sources.push_back(source_sect);
	scene.ajouter_bouge_action(source_sect->position, [&] (point_t mouse, float angle, bool alt) -> point_t {
		if (alt) {
			float l = source_sect->ang.longueur();
			source_sect->ang = angle_interv_t(-l/2, +l/2) + angle;
		}
		else source_sect->position = mouse;
		return source_sect->position;
	});
	
//	scene.propag_rayons_dessin_window = nullptr;
//	scene.propag_intercept_dessin_window = scene.win_scene;
	scene.static_text.insert(scene.static_text.begin(), {
		L"[X] debug rayon",
	});
	
	struct propag_debug_rayons_t {
		point_t milieu_rayon;
		Rayon ray;
		ObjetArc::intercept_courbe_t intercept;
	};
	std::vector<propag_debug_rayons_t> propag_debug_rayons;
	bool propag_debug = false;
	
	auto propag_intercept_debug_info_mouse = [&] (Objet& o, const Rayon& ray, std::shared_ptr<void> intercept_struct) {
		if (dynamic_cast<ObjetCourbe*>(&o) != nullptr) { // si c'est un ObjetCourbe (sinon le point d'interception n'est pas défini)
			ObjetArc::intercept_courbe_t& intercept = *std::static_pointer_cast<ObjetArc::intercept_courbe_t>(intercept_struct);
			propag_debug_rayons.push_back(propag_debug_rayons_t{
				.milieu_rayon = milieu_2points(ray.orig, intercept.p_incid),
				.ray = ray,
				.intercept = intercept
			});
		}
/*		auto p = o.point_interception(intercept_struct);
		if (p.has_value()) {
			propag_debug_rayons.push_back(propag_debug_rayons_t{
				.milieu_rayon = milieu_2points(ray.orig, *p),
				.ray = ray,
			});
		}*/
	};
	
	scene.boucle(
	/*f_event*/ [&] (sf::Event event) {
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::X) {
			if (propag_debug) {
				propag_debug = false;
				scene.propag_intercept_cb = nullptr;
			} else {
				propag_debug = true;
				scene.propag_intercept_cb = propag_intercept_debug_info_mouse;
			}
		}
	},
	/*f_pre_propag*/ [&] () {
	},
	/*f_post_propag*/ [&] () {
		if (propag_debug and not propag_debug_rayons.empty()) {
			auto closest_rayon = propag_debug_rayons.begin();
			float closest_dist2 = Inf;
			for (auto it = propag_debug_rayons.begin(); it != propag_debug_rayons.end(); it++) {
				float dist2 = (scene.mouse - it->milieu_rayon).norm2();
				if (dist2 < closest_dist2) {
					closest_dist2 = dist2;
					closest_rayon = it;
				}
			}
			const Rayon& ray = closest_rayon->ray;
			auto point_milieu = sf::c01::buildCircleShapeCR(closest_rayon->milieu_rayon, 0.003);
			point_milieu.setFillColor(sf::Color::White);
			scene.win_scene->draw(point_milieu);
			auto text = sf::c01::buildText(scene.font, closest_rayon->milieu_rayon, {
				fmt::format(L"orig : ( {:.3f}, {:.3f} )", ray.orig.x, ray.orig.y),
				fmt::format(L"angle : {:.2f}π", ray.dir_angle/M_PI),
				fmt::format(L"incid : ( {:.3f}, {:.3f} )", closest_rayon->intercept.p_incid.x, closest_rayon->intercept.p_incid.y),
				fmt::format(L"angle incid : {:.2f}π/2 {}", closest_rayon->intercept.ang_incid/(M_PI/2), closest_rayon->intercept.sens_reg ? L"reg" : L"inv"),
				fmt::format(L"I n°1 TE : {:.3e}", ray.spectre.comps[2]),
				fmt::format(L"I n°1 TM : {:.3e}", ray.spectre.comps[3]),
			}, sf::Color::White);
			scene.win_scene->draw(text);
		}
	});
	
	return 0;
}
