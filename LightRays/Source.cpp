#include "Source.h"
#include <cmath>

const decltype(Source_Omni::directivite) Source_Omni::directivite_unif = [] (float) -> float { return 1.f; };

///------------------------ Émission des sources ------------------------///

std::vector<Rayon> Source_UniqueRayon::genere_rayons () {
	return { Rayon{
		.orig = position,
		.dir_angle = dir_angle,
		.spectre = spectre
	} };
};

std::vector<Rayon> Source_LinParallels::genere_rayons () {
	std::vector<Rayon> rayons;
	size_t n_rayons = lroundf(dens_lin * !vec);
	float seg_dir_angle = atan2f(vec.y, vec.x) - M_PI/2;
	for (size_t k = 0; k < n_rayons; k++) {
		Rayon ray = {
			.orig = a + vec * (float)k / n_rayons,
			.dir_angle = seg_dir_angle + dir_angle_rel,
			.spectre = spectre
		};
		rayons.push_back(std::move(ray));
	}
	return rayons;
}

std::vector<Rayon> Source_PonctOmni::genere_rayons () {
	std::vector<Rayon> rayons;
	size_t n_rayons = lroundf(dens_ang);
	for (size_t k = 0; k < n_rayons; k++) {
		Rayon rayon = {
			.orig = position,
			.dir_angle = dir_alea ?
				(float)(2*M_PI) * rand01() :
				(float)(2*M_PI * k) / n_rayons,
			.spectre = spectre
		};
		if (secteur.has_value() and not secteur->inclus(rayon.dir_angle))
			continue;
		rayon.spectre.for_each([&] (float, pola_t, float& I) {
			I *= this->directivite( rayon.dir_angle );
		});
		rayons.push_back(std::move(rayon));
	}
	return rayons;
}

std::vector<Rayon> Source_SecteurDisqueLambertien::genere_rayons () {
	std::vector<Rayon> rayons;
	size_t n_rayons = lroundf(dens_ang * ang.longueur()/(2*M_PI));
	for (size_t k = 0; k < n_rayons; k++) {
		Rayon ray = { .spectre = spectre };
		ray.dir_angle = ang.beg() + ang.longueur() * ( dir_alea ? rand01() : ((float)k / n_rayons) );
		ray.orig = position + R * vec_t{ .x = cosf(ray.dir_angle), .y = sinf(ray.dir_angle) };
		float incr_angle = M_PI/2 * (1-2*rand01());
		ray.spectre.for_each([&] (float, pola_t, float& I) {
			I *= this->directivite( ray.dir_angle ) * cosf(incr_angle);
		});
		ray.dir_angle += incr_angle;
		rayons.push_back(std::move(ray));
	}
	return rayons;
}

std::vector<Rayon> Source_LinLambertien::genere_rayons () {
	std::vector<Rayon> rayons;
	size_t n_rayons = lroundf(dens_lin * !vec);
	float dir_angle = atan2f(vec.y, vec.x) - M_PI/2;
	for (size_t k = 0; k < n_rayons; k++) {
		float incr_angle = M_PI/2 * (1-2*rand01());
		Rayon ray = {
			.orig = a + vec * (float)k / n_rayons,
			.dir_angle = dir_angle + incr_angle,
			.spectre = spectre
		};
		ray.spectre.for_each([&] (float, pola_t, float& I) {
			I *= cosf(incr_angle);
		});
		rayons.push_back(std::move(ray));
	}
	return rayons;
};

///------------------------ Affichage ------------------------///

#include "sfml_c01.hpp"

void Source_UniqueRayon::dessiner (sf::RenderWindow& win) const {
	win.draw( sf::c01::buildLine(position,
								 position + 0.01 * vec_t{ .x = cosf(dir_angle), .y = sinf(dir_angle) },
								 sf::Color::Yellow) );
}

void Source_PonctOmni::dessiner (sf::RenderWindow& win) const {
	auto c = sf::c01::buildCircleShapeCR(position, 0.005);
	c.setFillColor(sf::Color::Yellow);
	win.draw(c);
}

void Source_SecteurDisqueLambertien::dessiner (sf::RenderWindow& win) const {
	size_t N = R * 100 * ang.longueur();
	if (N < 3) N = 3;
	sf::ConvexShape secteur (N+2);
	for (size_t k = 0; k <= N; k++) {
		float theta = ang.beg() + ang.longueur() * (float) k / N;
		secteur.setPoint(k, sf::c01::toWin(
			position + R * vec_t{ .x = cosf(theta),
			                      .y = sinf(theta) }
		));
	}
	secteur.setPoint(N+1, sf::c01::toWin(position));
	secteur.setFillColor(sf::Color::Yellow);
	win.draw(secteur);
}

void Source_LinLambertien::dessiner (sf::RenderWindow& win) const {
	win.draw(
		sf::c01::buildLine(a, a+vec, sf::Color::Yellow)
	);
}

void Source_LinParallels::dessiner (sf::RenderWindow& win) const {
	win.draw(
		sf::c01::buildLine(a, a+vec, sf::Color::Yellow)
	);
}
