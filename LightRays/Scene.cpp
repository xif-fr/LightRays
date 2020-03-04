#include "Scene.h"
#include <cmath>
#include "sfml_c01.hpp"

// Test d'interception du rayon contre toutes les objets de la scène puis renvoi des
//  rayons ré-émis; la première interception sur le trajet du rayon est choisie.
// Voir ObjetComposite::essai_intercept_composite pour un code similaire.
//
std::vector<Rayon> Scene::interception_re_emission (const Rayon& ray) {
	
	decltype(objets)::const_iterator objet_intercept = objets.end();
	std::shared_ptr<void> intercept_struct;
	float dist2_min = Inf;
	
	for (auto it = objets.begin(); it != objets.end(); it++) {
		// Objet::extension_t ex = (*it)->objet_extension();
		// Todo. Pour que ça puisse apporter quelque chose, il faut que ça soit calculé à l'avance et faire une grille et un test très rapide
		#warning To do
		Objet::intercept_t intercept = (*it)->essai_intercept(ray);
		if (intercept.dist2 < dist2_min) {
			dist2_min = intercept.dist2;
			objet_intercept = it;
			intercept_struct = intercept.intercept_struct;
		}
	}
	
	if (objet_intercept == objets.end())
		return {};
	else {
		if (propag_intercept_dessin_window != nullptr)
			(*objet_intercept)->dessiner_interception(*propag_intercept_dessin_window, ray, intercept_struct);
		if (propag_rayons_dessin_window != nullptr) {
			auto p = (*objet_intercept)->point_interception(intercept_struct);
			if (p.has_value()) {
				float c = propag_rayons_dessin_gain * ray.spectre.intensite_tot();
				auto line = sf::c01::buildLine(ray.orig, *p, sf::Color(255, 255, 255, (uint8_t)std::min(255.f,c)));
				propag_rayons_dessin_window->draw(line);
			}
		}
		if (propag_intercept_cb)
			propag_intercept_cb(**objet_intercept, ray, intercept_struct);
		return (*objet_intercept)->re_emit(ray, intercept_struct);
	}
}

// Fonction récurrente : interception par les objets de la scène puis ré-émission
//  avec la méthode `interception_re_emission`.
//
void Scene::propagation_recur (const Rayon& ray, uint16_t profondeur_recur) {
	stats_sum_prof_recur += profondeur_recur;
	if (profondeur_recur >= propag_profondeur_recur_max) {
		stats_n_rayons_profmax++;
		return;
	}
	stats_n_rayons++;
	std::vector<Rayon> rays = this->interception_re_emission(ray);
	profondeur_recur++;
	for (const Rayon& ray : rays) {
		if (ray.spectre.intensite_tot() < intens_cutoff) {
			stats_n_rayons_discarded++;
			continue;
		}
		if (propag_emit_cb)
			propag_emit_cb(ray, profondeur_recur);
		this->propagation_recur(ray, profondeur_recur);
	}
}

// Émet les rayons de toutes les sources de la scène et appelle `propagation_recur`.
//
void Scene::emission_propagation () {
	for (auto& source : sources) {
		std::vector<Rayon> rays = source->genere_rayons();
		stats_n_rayons_emis += rays.size();
		for (const Rayon& ray : rays) {
			if (propag_emit_cb)
				propag_emit_cb(ray, 0);
			this->propagation_recur(ray, 0);
		}
	}
}

///------- Méthodes utilitaires et Scene_ObjetsBougeables -------///

void Scene::ecrans_do (std::function<void (Ecran_Base &)> f) {
	for (auto obj : objets) {
		Ecran_Base* ecran = dynamic_cast<Ecran_Base*>(obj.get());
		if (ecran != nullptr)
			f(*ecran);
	}
}

bool Scene_ObjetsBougeables::objetsBougeables_event_SFML (const sf::Event& event) {
	if (objet_bougeant == nullptr) {
		if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::LShift)
			bouge_action_alt = true;
		if (event.type == sf::Event::KeyReleased and event.key.code == sf::Keyboard::LShift)
			bouge_action_alt = false;
		if (event.type == sf::Event::MouseButtonReleased)
			objet_bougeant = bougeable_nearest;
	} else {
		if (event.type == sf::Event::MouseButtonReleased) {
			bouge_action_alt = false;
			objet_bougeant = nullptr;
		}
	}
	if (event.type == sf::Event::MouseMoved) {
		mouse = sf::c01::fromWin(sf::Vector2f(event.mouseMove.x,event.mouseMove.y));
		if (objet_bougeant == nullptr) {
			if (bouge_actions.empty())
				bougeable_nearest = nullptr;
			else {
				bougeable_nearest = &bouge_actions.front();
				float dist2_min = Inf;
				for (bouge_action_t& obj : bouge_actions) {
					float dist2 = !(obj.pos - mouse);
					if (dist2 < dist2_min) {
						dist2_min = dist2;
						bougeable_nearest = &obj;
					}
				}
			}
		} else {
			vec_t v = mouse - objet_bougeant->pos;
			float dir_angle = atan2f(v.y, v.x);
			point_t new_pos = objet_bougeant->action_bouge(mouse, dir_angle, bouge_action_alt);
			objet_bougeant->pos = new_pos;
			return true;
		}
	}
	return false;
}

void Scene_ObjetsBougeables::dessiner_pointeur_nearest_bougeable (sf::RenderWindow& win) {
	if (objet_bougeant == nullptr and bougeable_nearest != nullptr) {
		auto pointeur = sf::c01::buildCircleShapeCR(bougeable_nearest->pos, 0.005);
		pointeur.setFillColor(sf::Color::Blue);
		win.draw(pointeur);
	}
}
