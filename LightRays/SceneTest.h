/*******************************************************************************
 * Scène "test" : regroupe tout le code qu'on trouverait typiquement dans
 * les main(), mais pas assez générique pour être dans l'objet `Scene`.
 *******************************************************************************/

// Aucune encapsulation ici, juste des fonctions utilitaires
// Création des fenêtres SFML, création de quelques objets courants & sources,
//  (optionellement) lentille + écran + fenêtre "CCD" pour la formation d'images
//  écrans autour de la scène.
// Hérite de Scene_ObjetsBougeables.

#ifndef _LIGHTRAYS_SCENE_TEST_H_
#define _LIGHTRAYS_SCENE_TEST_H_

#include <memory>
#include <cmath>
#include "Source.h"
#include "ObjetsOptiques.h"
#include "Ecran.h"
#include "Scene.h"
#include <SFML/Graphics.hpp>
#include "sfml_c01.hpp"
#include <string>
#include <vector>

class Scene_TestCommon : public Scene_ObjetsBougeables {
public:
	sf::RenderWindow* win_scene; // fenêtre SFML principale, affichage de la scène
	sf::Font font; // fonte SFML
	std::wstring nom; // nom de la scène
	std::vector<std::wstring> static_text; // texte supplémentaire affiché avec l'aide
	bool reset_ecrans; // drapeau : si `true`, réinitialise les écrans et repasse à false
	uint64_t frame_i; // numéro de frame depuis le dernier reset
	bool gel; // si `vrai`, gèle la scène et l'affichage
	bool affiche_text; // si `vrai`, affiche l'aide et le texte supplémentaire
	
	sf::RenderWindow* win_pixels; // fenêtre SFML secondaire optionnelle pour affichage de l'image sur l'écran `ecran_image`
	std::shared_ptr<EcranLigne_Multi> ecran_image; // écran de formation d'images (si `win_pixels!=null`)
	std::shared_ptr<Objet_MatriceTrsfUnidir> lentille; // lentille convergente pour former les images, optionnelle (et si `win_pixels!=null`)
	void lentille_mise_au_point (float x_obj); // mise au point de la lentille sur le plan x = `x_obj`
	void creer_bloqueurs_autour_lentille (float taille);
	
	// Création de la scène, des fenêtres SFML, optionnellement de l'écran et de la fenêtre image
	//  (si `creer_ecran_image` vrai) et de la lentille (si `creer_lentille_image` vrai).
	Scene_TestCommon (std::wstring nom, bool creer_ecran_image, bool creer_lentille_image);
	~Scene_TestCommon ();
	
	// Boucle principale SFML :
	//  - traite les évènements (clavier souris) et (optionnel) appelle f_event() à chaque évènement
	//  - effectue la propagation des rayons avec Scene::emission_propagation; et avant cela,
	//     appelle (optionnel) `f_pre_propag`; et après cela, appelle (optionnel) `f_post_propag`
	//  - dessine la scène
	//  - gère et affiche les statistiques, gère les écrans
	void boucle (std::function<void(sf::Event)> f_event,
				 std::function<void(void)> f_pre_propag,
				 std::function<void(void)> f_post_propag);
	
	// Création rapide de sources, voir implémentation
	std::shared_ptr<Source_UniqueRayon> creer_source_unique_rayon (point_t pos, float dir_angle, float ampl);
	std::shared_ptr<Source_PonctOmni> creer_source_omni_secteur (point_t pos, float ang_ext, float ang_base, float dens_ray);
	std::shared_ptr<Source_LinLambertien> creer_ciel_bleu (float dens_lin = 1000);
	// Création d'écrans autour de la scène
	void creer_ecrans_autour (float lumino = 0.0001, float epaiss = 0.01);
};

#endif