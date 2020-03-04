/*******************************************************************************
 * Définission des rayons et des spectres, fonctions utilitaires sur les rayons.
 *******************************************************************************/

#ifndef _LIGHTRAYS_RAYON_H_
#define _LIGHTRAYS_RAYON_H_

#include <array>
#include <functional>
#include <tuple>
#include "Util.h"

#define N_COULEURS 4

// Longueurs d'ondes des `N_COULEURS` composantes des rayons
constexpr float lambda_color [N_COULEURS] = {
	7e-4, 6e-4, 5e-4, 4e-4
};
// La position dans ce tableau définit un `color_id_t`
typedef uint8_t color_id_t;

// Conversion longueur d'onde -> RGB pour affichage
std::tuple<float,float,float> wavelenght_to_rgb (float lamda_mm, float gamma = 0.8);

// Polarisation d'un rayonnement.
// Puisque l'on ne s'intéresse qu'à l'optique des réfractions et réflexions, seul
//  compte le caractère TE ou TM. Et puisque l'on est en 2D, ce caractère est invariant
//  dans une réflexion ou une réfration, où le rayon reste dans le plan 2D.
enum pola_t { PolTE = 0, PolTM = 1 };

// Spectre en intensité/puissance (suivant le contexte) d'un rayonnement, discrétisé en `N_COULEURS`
//  (×2 pour la polarisation TE/TM) composantes de longueur d'onde définies `lambda_color`.
// La manipulation de `AmplComp::comps` doit se faire systématiquement avec for_each ou for_each_manual { ampl[i_comp_array] }.
//
struct Specte {
	std::array<float,2*N_COULEURS> comps;
	
	inline static void for_each_manual (std::function<void(uint8_t i_comp_array, float lambda, pola_t pol)> f) {
		for (uint8_t i = 0; i < 2*N_COULEURS; i++)
			f(i, lambda_color[i/2], i%2==0 ? pola_t::PolTE : pola_t::PolTM);
	}
	inline void for_each (std::function<void(float lambda, pola_t pol, float& I)> f) {			// version mutable avec lambda
		for_each_manual([&] (uint8_t i, float lambda, pola_t pol) {  f(lambda, pol, this->comps[i]);  });
	}
	inline void for_each_cid (std::function<void(color_id_t cid, pola_t pol, float& I)> f) {	// version mutable avec id de couleur
		for_each_manual([&] (uint8_t i, float, pola_t pol) {  f(i/2, pol, this->comps[i]);  });
	}
	inline void for_each (std::function<void(float lambda, pola_t pol, float I)> f) const {		// version constante avec lambda
		for_each_manual([&] (uint8_t i, float lambda, pola_t pol) {  f(lambda, pol, this->comps[i]);  });
	}
	
	static Specte monochromatique (float I, color_id_t id, std::optional<pola_t> pol = std::nullopt);
	static Specte polychromatique (std::array<float,N_COULEURS> I, std::optional<pola_t> pol = std::nullopt);
	
	// Sommation des intensités de chaque composante (intégration sur tout le spectre),
	// et division par le nombre de composantes (2×`N_COULEURS`).
	// Optionellement, sélectionne seulement les composantes d'une polarisation donnée.
	float intensite_tot () const;
	float intensite_tot (pola_t pola_sel) const;
	
	// Conversion du spectre en couleur RGB pour affichage sur fond noir, par rapport à une intensité
	//  de saturation de 1.0 _par composante_. Si `chroma_only` est vrai, ignore l'intensité lumineuse.
	// Le booléen indique si il y a saturation.
	// Optionellement, sélectionne seulement les composantes d'une polarisation donnée.
	// [TODO] Pour un affichage sur fond quelconque, il faudrait convertir la luminance en transparence.
	std::tuple<uint8_t,uint8_t,uint8_t,bool> rgb256_noir_intensite (bool chroma_only, std::optional<pola_t> pola_sel = std::nullopt) const;
};

// Spectre tel que l'affichage RGB est à peu près blanc
extern const Specte spectre_blanc;

// Définition d'un rayon : son origine, son angle à l'horizontale
//  et son spectre en intensité associé.
//
struct Rayon {
	point_t orig;
	float dir_angle;
	Specte spectre;
};

#endif
