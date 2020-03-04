/*******************************************************************************
 * Scène : objets, routines de propagation-interception-réémission des rayons,
 *  et sources. Aucune loi d'optique n'est implémentée ici, il ne s'agit
 *  que d'un conteneur, d'une récursion, et de méthodes utilitaires.
 *******************************************************************************/

#ifndef _LIGHTRAYS_SCENE_H_
#define _LIGHTRAYS_SCENE_H_

#include <vector>
#include <memory>
#include "Objet.h"
#include "Source.h"
#include "Ecran.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

class Scene {
public:
	
	// Liste des objets de la scène
	// (liste de pointeurs car les objets sont traités polymorphiquement)
	std::vector< std::shared_ptr<Objet> > objets;
	
	// Raccourci pour création d'objet de type `ObjT` : simple transfert d'argument de constructeur
	template <class ObjT, typename... Args>
	std::shared_ptr<ObjT> creer_objet (Args&& ...args) {
		auto obj = std::make_shared<ObjT>(std::forward<Args>(args)...);
		this->objets.push_back(obj);
		return obj;
	}
	
	// Liste des sources de la scène
	std::vector< std::shared_ptr<Source> > sources;
	
	
		///--------- Routines de propagation des rayons ---------///
	
	// Fenêtre de dessin des interceptions et/ou rayons (voir Scene::interception_re_emission)
	sf::RenderWindow* propag_intercept_dessin_window = nullptr;
	sf::RenderWindow* propag_rayons_dessin_window = nullptr;
	float propag_rayons_dessin_gain = 10.;
	
	// Callback appelé lors de l'interception d'un rayon par un objet. Utilisation typique : déboguage.
	// `intercept_struct` est le Objet::intercept_t::intercept_struct renvoyé par Objet::essai_intercept
	std::function< void (Objet&, const Rayon&, std::shared_ptr<void> intercept_struct) > propag_intercept_cb = nullptr;
	// Callback appellé pour chaque rayon émis ou ré-émis
	std::function< void (const Rayon&, uint16_t prof_recur) > propag_emit_cb = nullptr;
	
	// Statistiques
	uint64_t stats_n_rayons, stats_n_rayons_profmax, stats_n_rayons_discarded, stats_sum_prof_recur, stats_n_rayons_emis;
	
	// Test d'interception du rayon contre toutes les objets de la scène puis ré-émission; la première
	//  interception sur le trajet du rayon depuis son origine est choisie. Renvoie tous les rayons ré-émis.
	// Si `propagation_params.window` ≠ null, dessine l'interception du rayon avec `objet.dessiner_interception`,
	//  et appelle `propag_intercept_cb` (par exemple pour un dessin du rayon de la source à l'objet) si ≠ null.
	// Si `propag_rayons_dessin_window` ≠ null, dessine les rayons en blanc transparent (∝ intensité) grâce
	//  à `objet.point_interception`. Méthode surtout interne, appelé par `propagation_recur`.
	std::vector<Rayon> interception_re_emission (const Rayon& ray);
	
		/// Propagation d'un rayon : récursion de l'interception/ré-émission
	
	// Intensité en dessous de laquelle un rayon est ignoré. Fort impact sur la performance
	float intens_cutoff = 1e-2;
	// Profondeur de récursion (= nombre de ré-émission d'un rayon initial) maximale
	// Ne devrait jouer que pour des réflexions infinies sans perte (où l'intensité ne passe jamais en dessous de `intens_cutoff`)
	uint16_t propag_profondeur_recur_max = 20;
	
	// Fonction récurrente : interception par les objets de la scène puis ré-émission;
	//  utilise `interception_re_emission`, `intens_cutoff` et `propag_profondeur_recur_max`.
	// Appelle `propag_emit_cb` si ≠ null. Méthode surtout interne, appelé par `emission_propagation`.
	void propagation_recur (const Rayon& ray, uint16_t profondeur_recur);
	
	// Fonction principale : émet les rayons de toutes les sources de la scène et appelle `propagation_recur`.
	void emission_propagation ();
	
		///--------- Affichage et interface utilisateur ---------///
	
	// Dessin de tous les objets et sources de la scène.
	void dessiner_scene (sf::RenderWindow& window) const {
		for (auto& objet : objets)
			objet->dessiner(window, false);
		for (auto& source : sources)
			source->dessiner(window);
	}
	
	// Appelle f() sur tous les objets de type Ecran_Base
	void ecrans_do (std::function<void(Ecran_Base&)> f);
	
};

///----------------------------------------------------------------------
/// Scène avec objets "bougeables" : routines utiles pour (par exemple)
///  déplacer/tourner les objets de la scène avec la souris, suivant des
///  actions spécifés par l'utilisateur (`bouge_action_t`).

class Scene_ObjetsBougeables : public Scene {
public:
	
	// Ancre à la postion `pos` déclanchant l'action `action_bouge` lorsque cliqué,
	// avec la postion de la souris et son angle à l'horizontale. Si le clic est fait
	// avec la touche Maj, `alt=true`, sinon `false`.
	struct bouge_action_t {
		point_t pos;
		std::function< point_t (point_t mouse, float angle, bool alt) > action_bouge;
	};
	std::vector<bouge_action_t> bouge_actions;
	void ajouter_bouge_action (point_t pos_initiale, decltype(bouge_action_t::action_bouge) action) {
		bouge_actions.push_back({pos_initiale, action});
	}
	// Position courante de la souris
	point_t mouse = {0,0};
	
	// Dessine un point bleu sur l'ancre la plus proche de la souris
	void dessiner_pointeur_nearest_bougeable (sf::RenderWindow& win);
	// À appeller lorsqu'un évènement souris/clavier/clic SFML est déclanché
	// Lorsqu'un objet/ancre a été déplacée, renvoie `true` (e.g. pour reset d'écrans)
	bool objetsBougeables_event_SFML (const sf::Event& event);
	
private:
	bool bouge_action_alt = false;
	bouge_action_t* bougeable_nearest = nullptr;
	bouge_action_t* objet_bougeant = nullptr;
public:
	Scene_ObjetsBougeables () = default;
	Scene_ObjetsBougeables (const Scene_ObjetsBougeables&) = delete;
};

#endif
