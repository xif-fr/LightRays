/*******************************************************************************
 * Objet optique générique, capable de se dessiner, et surtout de tester
 * l'interception d'un rayon, et, le cas échéant, de ré-émettre le rayon.
 *******************************************************************************/

#ifndef _LIGHTRAYS_OBJET_H_
#define _LIGHTRAYS_OBJET_H_

#include <memory>
#include "Rayon.h"
#include "Util.h"
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>
// Note : tous les std::optional<std::shared_ptr<T>> pourraient être remplacés par des T* (null ou pas),
//        mais il faudrait gérer la désallocation à la main

//------------------------------------------------------------------------------
// Classe de base des objets optiques. Déclare l'interface commune utilisée lors
//  de la propagation (interception puis ré-émission) des rayons, et l'affichage.
// Un objet est capable de tester l'interception d'un rayon avec `essai_intercept`,
//  et, le cas échéant, de ré-émettre le rayon `re_emit` et (optionnellement) de
//  donner le point d'interception avec `point_interception`. Il est aussi capable
//  de se dessiner et de donner son extension spatiale approximative.

class Objet {
public:
	
	Objet () {}
	Objet& operator= (const Objet&) = default;
	Objet (const Objet&) = default;
	virtual ~Objet () {}
	
	// L'objet intercepte-t-il le rayon ? Si oui, donne la distance géométrique au carré
	//  `dist2` parcourue par le rayon depuis son point d'émission, et renvoie une structure
	//  interne à utiliser éventuellement pour la ré-émission du même rayon.
	// Si non, `intercept_struct` est nul et `dist2 = Inf`
	// Attention, la création de std::shared_ptr<void> est délicate (https://stackoverflow.com/questions/25858939/is-there-a-way-to-cast-shared-ptrvoid-to-shared-ptrt/25858963 et voir ObjetCourbe::essai_intercept)
	struct intercept_t { float dist2; std::shared_ptr<void> intercept_struct; };
	virtual intercept_t essai_intercept (const Rayon& ray) const = 0;
	// Point d'interception. Aucune garantie, non défini par défaut.
	virtual std::optional<point_t> point_interception (std::shared_ptr<void> intercept_struct) const { return std::nullopt; }
	
	// Extension spatiale approximative pessimiste de l'objet à fin d'optimisation
	//  (ne pas avoir à appeller un coûteux `essai_intercept(ray)` lorsque qu'on est
	//  certain que le rayon ne sera pas intercepté par l'objet)
	struct extension_t { point_t pos; float rayon; };
	virtual extension_t objet_extension () const = 0;
	
	// Ré-émission du rayon, devant utiliser la structure `.intercept_struct` renvoyée par `essai_intercept(ray)`
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) = 0;
	
	// Rendu graphique de l'objet
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const = 0;
	// Rendu graphique de l'interception d'un rayon (voir re_emit)
	virtual void dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> intercept) const = 0;
};

#endif
