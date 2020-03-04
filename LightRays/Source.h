/*******************************************************************************
 * Classe de base des sources. Divers types de sources.
 *******************************************************************************/

#ifndef _LIGHTRAYS_SOURCE_H_
#define _LIGHTRAYS_SOURCE_H_

#include "Rayon.h"
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>

//------------------------------------------------------------------------------
// Classe de base, abstraite, des sources. À priori, seulement définie par
//  son spectre.

class Source {
public:
	Specte spectre; // spectre de la source en intensité
	
	// source de spectre arbitraire
	Source (Specte spectre) : spectre(spectre) {}
	// source monochromatique polarisée
	Source (color_id_t couleur, float I, pola_t pol) : spectre( Specte::monochromatique(I, couleur, pol) ) {}
	// source monochromatique non-polarisée
	Source (color_id_t couleur, float I) : spectre( Specte::monochromatique(I, couleur) ) {};
	
	Source& operator= (const Source&) = default;
	Source (const Source&) = default;
	virtual ~Source () {}
	
	// génération des rayons
	virtual std::vector<Rayon> genere_rayons () = 0;
	
	// dessin de la source
	virtual void dessiner (sf::RenderWindow& window) const = 0;
};

//------------------------------------------------------------------------------
// Source émettant un unique rayon à partir du point `position` et à un angle
//  `dir_angle` à l'horizontale. "Rayon laser".

class Source_UniqueRayon : public Source {
public:
	point_t position;
	float dir_angle;
	
	// relai aux constructeurs de `Source`
	template<typename... Args> Source_UniqueRayon (point_t pos, float dir, Args&&... x) :
		Source(x...), position(pos), dir_angle(dir) {}
	
	Source_UniqueRayon& operator= (const Source_UniqueRayon&) = default;
	Source_UniqueRayon (const Source_UniqueRayon&) = default;
	virtual ~Source_UniqueRayon () {}
	
	// création de l'unique rayon
	virtual std::vector<Rayon> genere_rayons () override;
	
	void dessiner (sf::RenderWindow& window) const override;
};

//------------------------------------------------------------------------------
// Source étendue émettant un plusieurs rayon distribués sur une ligne et à un
//  angle fixe. Peut être vu comme une source ponctuelle à l'infini, ou un
//  ensemble de `Source_UniqueRayon`.

class Source_LinParallels : public Source {
public:
	point_t a; // point A du segment
	vec_t vec; // vecteur AB du segment
	float dens_lin; // nombre de rayons par unité de longueur
	float dir_angle_rel; // angle par rapport au vecteur AB
	
	template<typename... Args> Source_LinParallels (point_t pos_seg_a, point_t pos_seg_b, float dir_angle, Args&&... x) :
		Source(x...), a(pos_seg_a), vec(pos_seg_b-pos_seg_a), dens_lin(100), dir_angle_rel(dir_angle) {}
	template<typename... Args> Source_LinParallels (point_t pos_seg_a, vec_t pos_ab_vecteur, float dir_angle, Args&&... x) :
		Source(x...), a(pos_seg_a), vec(pos_ab_vecteur), dens_lin(100), dir_angle_rel(dir_angle) {}
	
	Source_LinParallels& operator= (const Source_LinParallels&) = default;
	Source_LinParallels (const Source_LinParallels&) = default;
	virtual ~Source_LinParallels () {}
	
	virtual std::vector<Rayon> genere_rayons () override;
	
	void dessiner (sf::RenderWindow& window) const override;
};

//------------------------------------------------------------------------------
// Classe de base virtuelle des sources omnidirectionnelle
//  (ou restreinte à un secteur angulaire `secteur`) à directivité arbitraire.

class Source_Omni : public Source {
public:
	point_t position; // position de la source
	float dens_ang; // nombre de rayons pour 2π
	bool dir_alea; // émission dans des directions aléatoires ou équiréparties/déterministe
	std::function< float(float theta) > directivite; // directivité (normalisée à 1)
	const static decltype(directivite) directivite_unif; // directivité unirforme
	std::optional<angle_interv_t> secteur; // secteur angulaire d'émission
	
	template<typename... Args> Source_Omni (point_t pos, Args&&... x) :
		Source(x...), position(pos), dens_ang(100), dir_alea(true), directivite(directivite_unif) {}
	
	Source_Omni& operator= (const Source_Omni&) = default;
	Source_Omni (const Source_Omni&) = default;
	virtual ~Source_Omni () {}
};

//------------------------------------------------------------------------------
// Source ponctuelle omnidirectionnelle (ou restreinte à un secteur angulaire)

class Source_PonctOmni : public Source_Omni {
public:
	virtual std::vector<Rayon> genere_rayons () override;
	
	void dessiner (sf::RenderWindow& window) const override;
	
	template<typename... Args> Source_PonctOmni (point_t pos, Args&&... x) : Source_Omni(pos, x...) {}
	
	virtual ~Source_PonctOmni () {}
};

//------------------------------------------------------------------------------
// Source étendue en secteur de disque ("projecteur") avec une émission
//  lambertienne à chaque point de sa surface (loi en cosinus). Si R très petit,
//  équivalent à Source_PonctOmni.

class Source_SecteurDisqueLambertien : public Source_Omni {
public:
	float R;
	angle_interv_t ang;
	
	template<typename... Args> Source_SecteurDisqueLambertien (float radius, angle_interv_t ang_interval, Args&&... x) :
		Source_Omni(x...), R(radius), ang(ang_interval) {}
	
	Source_SecteurDisqueLambertien& operator= (const Source_SecteurDisqueLambertien&) = default;
	Source_SecteurDisqueLambertien (const Source_SecteurDisqueLambertien&) = default;
	virtual ~Source_SecteurDisqueLambertien () {}
	
	virtual std::vector<Rayon> genere_rayons () override;
	
	void dessiner (sf::RenderWindow& window) const override;
};

//------------------------------------------------------------------------------
// Source étendue linéaire ("écran lumineux") avec une émission lambertienne
//  à chaque point de sa surface (loi en cosinus), et d'un seul côté du segment.

class Source_LinLambertien : public Source {
public:
	point_t a;
	vec_t vec;
	float dens_lin;
	
	template<typename... Args> Source_LinLambertien (point_t pos_seg_a, point_t pos_seg_b, Args&&... x) :
		Source(x...), a(pos_seg_a), vec(pos_seg_b-pos_seg_a), dens_lin(100) {}
	template<typename... Args> Source_LinLambertien (point_t pos_seg_a, vec_t pos_ab_vecteur, Args&&... x) :
		Source(x...), a(pos_seg_a), vec(pos_ab_vecteur), dens_lin(100) {}
	
	Source_LinLambertien& operator= (const Source_LinLambertien&) = default;
	Source_LinLambertien (const Source_LinLambertien&) = default;
	virtual ~Source_LinLambertien () {}
	
	virtual std::vector<Rayon> genere_rayons () override;
	
	void dessiner (sf::RenderWindow& window) const override;
};

#endif