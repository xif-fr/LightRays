#include "ObjetDiffusant.h"
#include <cmath>

decltype(ObjetCourbe_Diffusant::BRDF) ObjetCourbe_Diffusant::BRDF_Lambert = [] (float theta_i, float theta_r) -> float {
	return 1;
};

// Diffusion du rayon incident
//
std::vector<Rayon> ObjetCourbe_Diffusant::re_emit (const Rayon& ray, std::shared_ptr<void> interception) {
	intercept_courbe_t& intercept = *(intercept_courbe_t*)interception.get();
	std::vector<Rayon> rayons;
	
	// nombre de rayons ré-émis selon l'intensité du rayon incident
	size_t n_re_emit = std::max<size_t>(1, lroundf(n_re_emit_par_intens * ray.spectre.intensite_tot()));
	
	for (size_t k = 0; k < n_re_emit; k++) {

		float ang_refl = 0;
		switch (diff_met) {
			case diffus_methode_t::AleaUnif:
				ang_refl = M_PI/2 * (1-2*rand01()); break;		// angles aléatoires équirépartis
			case diffus_methode_t::Equirep:
				ang_refl = M_PI/2 * (1-2*(k+1)/(float)(n_re_emit+1)); break;	// angles déterministes équirépartis
			default:
				throw std::runtime_error("TODO");
		}

		Rayon ray_refl;
		ray_refl.orig = intercept.p_incid;
		ray_refl.dir_angle = intercept.ang_normale + ang_refl;
		
		// pondération de l'intensité par la BRDF en fonction de l'angle incident et réfléchi
		ray_refl.spectre = ray.spectre;
		if (not BRDF_lambda) {
			float ampl = albedo / n_re_emit * BRDF( intercept.ang_incid, ang_refl );
			ray_refl.spectre.for_each([&] (float, pola_t, float& I) {
				I *= ampl;
			});
		} else {
			ray_refl.spectre.for_each([&] (float lambda, pola_t, float& I) {
				I *= albedo / n_re_emit * BRDF_lambda( intercept.ang_incid, ang_refl, lambda );
			});
		}
		
		rayons.push_back(std::move(ray_refl));
	}
		
	return rayons;
}
