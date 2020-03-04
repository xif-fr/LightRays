/*******************************************************************************
 * Brouillards.
 *******************************************************************************/

#ifndef _LIGHTRAYS_BROUILLARD_H_
#define _LIGHTRAYS_BROUILLARD_H_

#include "Objet.h"

//------------------------------------------------------------------------------
// Brouillard à "cellules/voxels" de densité arbitraire. Implémentation naïve,
//  loin d'être performante.

class Objet_Brouillard : virtual public Objet {
public:
	point_t o; // bottom-left corner (origine)
	float reso_x, reso_y; // résolutions (taille d'une cellule du brouillard) dans les directions x et y
	uint lx, ly; // taille du brouillard en nombre de cellules
	std::function< float(size_t x, size_t y) > densit_brouillard; // fonction densité(x,y) du brouillard
	std::function< float(float theta, float lambda) > directivite_diffus; // distribution angulaire des rayons diffusés, avec theta l'angle du rayon diffusé par rapport au rayon incident
	
	// Il y a deux techniques de diffusion (non équivalentes en terme de résultat) :
	// - soit on diffuse totalement (en `n_re_emit_par_intens` rayons-fils, avec la directivité `directivite_diffus`)
	//   le rayon avec une probabilité à chaque pas donnée par la densité du brouillard
	//   ("diffusion browienne")
	//   (adapté au temps réel car peu de rayons produits, mais résultat bruité)
	// - soit on diffuse partiellement (fraction donnée par la densité du brouillard, et en
	//   `n_re_emit_par_intens` rayons-fils, avec la directivité `directivite_diffus`) le rayon,
	//   mais systématiquement avec un libre parcours moyen `diffus_partielle_syst_libreparcours`
	//   ("diffusion à la bert-lambert") (attention, la directivité effective est `directivite_diffus` + un dirac theta=0)
	//   (peu bruité, mais lent car "cascade" -> nombreux rayons produits)
	// La deuxième méthode est utilisée lorsque `diffus_partielle_syst_libreparcours` est non infini
	// Lorsque la longueur `diffus_partielle_syst_libreparcours` n'est pas trop grande face à la taille du brouillard,
	//  et qu'elle est supérieure à la résolution, les deux méthodes doivent donner la même intensité
	//  transmise en ligne droite ("rayon non dévié") en moyenne
	float n_re_emit_par_intens;
	float diffus_partielle_syst_libreparcours;
	
	// Parcours déterministe ou non du rayon passant à traver le brouillard. Typiquement, soit
	//  on fait une diffusion du rayon aléatoire avec une certaine probabilité, soit on diffuse
	//  régulièrement le rayon avec cette même "probabilité". Les deux méthodes convergent à
	//  grande résolution.
	bool parcours_deterministe;
	
	// Les rayons d'intensité < `intens_cutoff` sont ignorés (amélioration de performance)
	float intens_cutoff;
	
	// Par défaut, méthode "brownienne", directivté uniforme, parcours aléatoire, pas de cutoff
	Objet_Brouillard (point_t o, float reso_x, float reso_y, uint lx, uint ly, decltype(densit_brouillard) densit_brouillard) :
		o(o), reso_x(reso_x), reso_y(reso_y), lx(lx), ly(ly), densit_brouillard(densit_brouillard),
		directivite_diffus([] (float, float) { return 1.f; }),
		n_re_emit_par_intens(5), diffus_partielle_syst_libreparcours(Inf), parcours_deterministe(false), intens_cutoff(0) {}
	
	Objet_Brouillard& operator= (const Objet_Brouillard&) = default;
	Objet_Brouillard (const Objet_Brouillard&) = default;
	virtual ~Objet_Brouillard () {}
	
	// Routines d'interception et de ré-émission
	struct intercept_brouillard_t {
		point_t p_diff;
		int x_diff, y_diff;
		float fraction_transmis;
	};
	virtual intercept_t essai_intercept (const Rayon&) const override;
	virtual extension_t objet_extension () const override;
	virtual std::vector<Rayon> re_emit (const Rayon&, std::shared_ptr<void>) override;
	virtual std::optional<point_t> point_interception (std::shared_ptr<void> intercept_struct) const override;
	
	// Dessin
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
	// Pas de dessin d'interception
	virtual void dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> intercept) const override {};
};

#endif
