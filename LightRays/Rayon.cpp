#include "Rayon.h"
#include "cmath"

// Spectre dont les composantes sont ajustées pour avoir à peu près du blanc en affichage RBG
const Specte spectre_blanc = Specte::polychromatique({{0.8,0.8,1.7,2.2}});

Specte Specte::monochromatique (float A, color_id_t color_id, std::optional<pola_t> pola_sel) {
	if (color_id >= N_COULEURS)
		throw std::out_of_range("identifiant couleur invalide");
	Specte specte;
	Specte::for_each_manual([&] (size_t i, float lambda, pola_t pol) {
		if (pola_sel.has_value() and *pola_sel != pol) {
			specte.comps[i] = 0;
		} else {
			if (color_id == i/2)
				specte.comps[i] = A;
			else
				specte.comps[i] = 0;
		}
	});
	return specte;
}

Specte Specte::polychromatique (std::array<float,N_COULEURS> I, std::optional<pola_t> pola_sel) {
	Specte specte;
	Specte::for_each_manual([&] (size_t i, float lambda, pola_t pol) {
		if (pola_sel.has_value() and *pola_sel != pol) {
			specte.comps[i] = 0;
		} else {
			specte.comps[i] = I[i/2];
		}
	});
	return specte;
}

// Convert a given wavelength of light to an approximate RGB color value.
// The wavelength must be given in mm in the range from 380nm through 750nm.
// Based on code by Dan Bruton : http://www.physics.sfasu.edu/astro/color/spectra.html
std::tuple<float,float,float> wavelenght_to_rgb (float lamda, float gamma) {
	float wavelength = lamda * 1e6;
	float R = 0., G = 0., B = 0., attenuation;
	if (wavelength >= 380 and wavelength <= 440) {
		attenuation = 0.3 + 0.7 * (wavelength - 380) / (440 - 380);
		R = powf( (-(wavelength - 440) / (440 - 380)) * attenuation, gamma);
		B = powf( 1.0 * attenuation, gamma);
	} else if (wavelength >= 440 and wavelength <= 490) {
		G = powf( (wavelength - 440) / (490 - 440), gamma);;
		B = 1.0;
	} else if (wavelength >= 490 and wavelength <= 510) {
		G = 1.0;
		B = powf( -(wavelength - 510) / (510 - 490), gamma);
	} else if (wavelength >= 510 and wavelength <= 580) {
		R = powf( (wavelength - 510) / (580 - 510), gamma);
		G = 1.0;
	} else if (wavelength >= 580 and wavelength <= 645) {
		R = 1.0;
		G = powf( -(wavelength - 645) / (645 - 580), gamma);
	} else if (wavelength >= 645 and wavelength <= 750) {
		attenuation = 0.3 + 0.7 * (750 - wavelength) / (750 - 645);
		R = powf( 1.0 * attenuation, gamma);
	}
	return { R, G, B };
}

struct rgb_cache_t {
	std::array< std::tuple<float,float,float>, 2*N_COULEURS> comp_couleurs_rgb;
	rgb_cache_t () {
		Specte::for_each_manual([&] (uint8_t i_comp_array, float lambda, pola_t pol) {
			comp_couleurs_rgb[i_comp_array] = wavelenght_to_rgb( lambda );
		});
	}
} rgb_cache;

std::tuple<uint8_t,uint8_t,uint8_t,bool> Specte::rgb256_noir_intensite (bool chroma_only, std::optional<pola_t> pola_sel) const {
	float I_r = 0, I_g = 0, I_b = 0;
	Specte::for_each_manual([&] (uint8_t i_comp_array, float lambda, pola_t pol) {
		if (pola_sel.has_value() and *pola_sel != pol)
			return;
		float rc, gc, bc; std::tie(rc,gc,bc) = /* wavelenght_to_rgb(lambda) */ rgb_cache.comp_couleurs_rgb[i_comp_array];
		I_r += rc * comps[i_comp_array];
		I_g += gc * comps[i_comp_array];
		I_b += bc * comps[i_comp_array];
	});
	I_r /= 2*N_COULEURS;
	I_g /= 2*N_COULEURS;
	I_b /= 2*N_COULEURS;
	float max_I = std::max({I_r, I_g, I_b});
	bool sat = (max_I > 1.);
	if (sat or chroma_only) {
		I_r /= max_I; I_g /= max_I; I_b /= max_I;
	}
	return { 255*I_r, 255*I_g, 255*I_b, sat };
}

float Specte::intensite_tot (pola_t pola_sel) const {
	float I_tot = 0;
	Specte::for_each_manual([&] (uint8_t i_comp_array, float, pola_t pol) {
		if (pola_sel != pol)
			return;
		I_tot += comps[i_comp_array];
	});
	I_tot /= 2*N_COULEURS;
	return I_tot;
}

float Specte::intensite_tot () const {
	float I_tot = 0;
	Specte::for_each_manual([&] (uint8_t i_comp_array, float, pola_t) {
		I_tot += comps[i_comp_array];
	});
	I_tot /= 2*N_COULEURS;
	return I_tot;
}