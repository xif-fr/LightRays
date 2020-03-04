/*******************************************************************************
 * Objets d'optique géométrique divers : miroirs, bloqueur, filtre,
 *  matrice ABCD, bilan d'énergie…
 *******************************************************************************/

#ifndef _LIGHTRAYS_OPTIQUES_H_
#define _LIGHTRAYS_OPTIQUES_H_

#include "ObjetsCourbes.h"

//------------------------------------------------------------------------------
// Objet "matrice ABCD" unidirectionnel : objet linéaire transmettant les rayons
// inteceptés du côté direct, après multiplication des paramètres [y,y']
// (élévation par rapport au centre, pente par rapport à la normale) par la
// matrice de transert ABCD spécifiée.

class Objet_MatriceTrsfUnidir : virtual public ObjetLigne {
public:
	// matrice de transfert
	struct mat_trsf_t { float A, B, C, D; } mat_trsf;
	// matrice de transfert d'une lentille
	static mat_trsf_t mat_trsf_lentille (float f) { return {1, 0, -1/f, 1}; }
	// matrice de transfert d'une propagation libre
	static mat_trsf_t mat_trsf_propag (float l) { return {1, l, 0, 1}; }
	
	// Constructeur : l'objet est défini par son centre optique, son diamètre, l'angle à la
	// verticale (sens trigo), et par la matrice de transfert ABCD (y relatif au plan et
	// au centre, et y' par rapport à la normale du plan). Sens de propagation direct vers +x
	// (si |angle| < π/2). Les rayons arrivant dans le sens inverse ne sont pas traités.
	Objet_MatriceTrsfUnidir (point_t centre, float diam, float ang_vertical, mat_trsf_t m);
	
	Objet_MatriceTrsfUnidir& operator= (const Objet_MatriceTrsfUnidir&) = default;
	Objet_MatriceTrsfUnidir (const Objet_MatriceTrsfUnidir&) = default;
	virtual ~Objet_MatriceTrsfUnidir () {}
	
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override;
	
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override final;
};

//------------------------------------------------------------------------------
// Bloqueur : simple objet linéaire absorbant tous les rayons interceptés.

class Objet_Bloqueur : virtual public ObjetLigne {
public:
	Objet_Bloqueur (point_t pos_a, point_t pos_b) : ObjetLigne(pos_a,pos_b) {}
	virtual ~Objet_Bloqueur () {}
	
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override final {
		return {}; // oublie simplement le rayon
	}
};

//------------------------------------------------------------------------------
// Filtre : objet linéaire transmettant les rayons interceptés, mais modifiant
// leur spectre, multipliant chaque composante du spectre par un taux de
// transmission en intensité. Peut servir de filtre de couleur, de polariseur…

class Objet_Filtre : virtual public ObjetLigne {
public:
	// Spectre de transmission du filtre, chaque composante de 0 à 1
	Specte transm;
	
	// Spectre de transmission (en intensité) complet spécifié.
	// Pour chaque composante de `sp_transmission`, 0 -> composante du rayon absorbée, et 1 -> composante du rayon transmise totalement
	template<typename... Args> Objet_Filtre (Specte sp_transmission, Args&&... x) : ObjetLigne(x...), transm(sp_transmission) {}
	
	// Seule couleur transmise (quelque soit la polarisation) spécifiée
	template<typename... Args> Objet_Filtre (color_id_t couleur_transmise, Args&&... x) : ObjetLigne(x...), transm(Specte::monochromatique(1, couleur_transmise)) {}
	
	Objet_Filtre& operator= (const Objet_Filtre&) = default;
	Objet_Filtre (const Objet_Filtre&) = default;
	virtual ~Objet_Filtre () {}
	
	// Transmission du rayon filtré
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override;
};

//------------------------------------------------------------------------------
// Miroir courbe générique : simple réflexion des rayons tel que l'angle
//  d'incidence est égal à l'angle du rayon réfléchi à la normale.

class ObjetCourbe_Miroir : virtual public ObjetCourbe {
public:
	ObjetCourbe_Miroir () : ObjetCourbe() {}
	virtual ~ObjetCourbe_Miroir () {}
	
	// Réflexion parfaite du rayon
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override final;
};

//------------------------------------------------------------------------------
// Miroir plan : ObjetCourbe_Miroir + ObjetLigne

class ObjetLigne_Miroir : virtual public ObjetCourbe_Miroir, virtual public ObjetLigne {
public:
	// relai des arguments aux contructeurs de ObjetLigne
	ObjetLigne_Miroir (point_t a, point_t b) : ObjetCourbe_Miroir(), ObjetLigne(a, b) {}
	ObjetLigne_Miroir (point_t a, float l, float ang) : ObjetCourbe_Miroir(), ObjetLigne(a, l, ang) {}
	virtual ~ObjetLigne_Miroir () {}
};

//------------------------------------------------------------------------------
// Miroir en arc de cercle : ObjetCourbe_Miroir + ObjetArc

class ObjetArc_Miroir : virtual public ObjetCourbe_Miroir, virtual public ObjetArc {
public:
	// relai des arguments aux contructeurs de ObjetArc
	ObjetArc_Miroir (point_t c, float R, angle_interv_t ang, bool inv_int) : ObjetCourbe_Miroir(), ObjetArc(c, R, ang, inv_int) {}
	ObjetArc_Miroir (point_t a, point_t b, float R, bool inv_int) : ObjetCourbe_Miroir(), ObjetArc(a, b, R, inv_int) {}
	virtual ~ObjetArc_Miroir () {}
};

//------------------------------------------------------------------------------
// Objet "bilan d'énergie" : intercepte les rayons sur un cercle et somme les
// flux entrants et sortants. Utile pour vérifier la conservation de l'intensité
// par un objet. Les rayons interceptés sont ré-émis à l'identique.

class Objet_BilanEnergie : virtual public ObjetArc {
private:
	float flux_in, flux_out;
	size_t n_ray_in, n_ray_out;
	size_t n_acc;
public:
	Objet_BilanEnergie (point_t centre, float radius) :
		ObjetArc(centre, radius, angle_interv_t::cercle_entier, false),
		flux_in(0), flux_out(0), n_ray_in(0), n_ray_out(0), n_acc(0) {}
	
	Objet_BilanEnergie& operator= (const Objet_MatriceTrsfUnidir&) = delete;
	Objet_BilanEnergie (const Objet_MatriceTrsfUnidir&) = delete;
	virtual ~Objet_BilanEnergie () {}
	
	// intercepte les rayons et accumule les flux entrants et sortants
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override final;
	
	// typiquement appelé à chaque frame, pour moyenner les valeurs sur plusieurs frames
	void commit () { n_acc++; }
	void reset () { flux_in = flux_out = 0; n_ray_in = n_ray_out = n_acc = 0; }
	// statistiques de flux : intensité entrante, intensité sortante, nombre de rayons entrants et sortants
	struct stats_par_frame_t {
		float flux_in, flux_out, n_ray_in, n_ray_out;
	};
	stats_par_frame_t bilan () { return { flux_in/n_acc, flux_out/n_acc, n_ray_in/(float)n_acc, n_ray_out/(float)n_acc }; }
};

#endif