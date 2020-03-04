/*******************************************************************************
 * Surfaces diffusantes à distribution de réflectivité (BRDF) arbitraire.
 *******************************************************************************/

#ifndef _LIGHTRAYS_DIFFUS_H_
#define _LIGHTRAYS_DIFFUS_H_

#include "ObjetsCourbes.h"

class ObjetCourbe_Diffusant : virtual public ObjetCourbe {
public:
	
	// Méthode de tirage de rayon (même résultats en moyenne, mais performances et fluctuations différentes)
	enum diffus_methode_t {
		Equirep, // Équirépartition des rayons sur tous les angles i_r avec modulation d'amplitude par la BRDF
		AleaUnif, // Émission aléatoire uniforme avec modulation d'amplitude par la BRDF
		MonteCarlo, // Tirage de rayons d'intensité constante avec une distribution respectant la BRDF (Metropolis)
	} diff_met;
	// Bidirectional reflectance distribution function, possiblement dépendante de la couleur
	std::function<float(float theta_i, float theta_r)> BRDF;                        // utilisée si `BRDF_lambda` nulle
	std::function<float(float theta_i, float theta_r, float lambda)> BRDF_lambda;   // utilisée si non nulle
	// Albedo, entre 0 et 1 (simple facteur d'intensité)
	float albedo;
	// Nombre moyen de rayons ré-émis par rayon incident par unité d'intensité. Doit être grand si diffus_methode_t::Equirep utilisée.
	float n_re_emit_par_intens;
	
	static decltype(BRDF) BRDF_Lambert; // Diffusion lambertienne (isotrope <=> loi en cos(θ) <=> BRDF = 1)
	static decltype(BRDF) BRDF_Oren_Nayar (float sigma); // Diffusion de Oren Nayar
	
	ObjetCourbe_Diffusant (decltype(BRDF) BRDF) : diff_met(AleaUnif), BRDF(BRDF), BRDF_lambda(nullptr), albedo(1), n_re_emit_par_intens(1) {}
	ObjetCourbe_Diffusant& operator= (const ObjetCourbe_Diffusant&) = default;
	ObjetCourbe_Diffusant (const ObjetCourbe_Diffusant&) = default;
	virtual ~ObjetCourbe_Diffusant () {};
	
	// Diffusion du rayon incident
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override final;
};

class ObjetArc_Diffusant : virtual public ObjetCourbe_Diffusant, virtual public ObjetArc {
public:
	// relai des arguments aux constructeurs de ObjetArc
	template<typename... Args> ObjetArc_Diffusant (decltype(BRDF) BRDF, Args&&... x) : ObjetCourbe_Diffusant(BRDF), ObjetArc(x...) {}
	virtual ~ObjetArc_Diffusant () {}
};

class ObjetLigne_Diffusant : virtual public ObjetCourbe_Diffusant, virtual public ObjetLigne {
public:
	// relai des arguments aux constructeurs de ObjetLigne
	template<typename... Args> ObjetLigne_Diffusant (decltype(BRDF) BRDF, Args&&... x) : ObjetCourbe_Diffusant(BRDF), ObjetLigne(x...) {}
	virtual ~ObjetLigne_Diffusant () {};
};

#endif
