/*******************************************************************************
 * Objets optiques génériques de type courbes : ces classes virtuelles définissent
 * la géométrie des objets (lignes, arcs de cercle, composites…) pour mettre en
 * commun les routines géométriques d'interception et de ré-émission de rayons lumineux.
 *******************************************************************************/

#ifndef _LIGHTRAYS_OBJETS_COURBES_H_
#define _LIGHTRAYS_OBJETS_COURBES_H_

#include "Objet.h"

//------------------------------------------------------------------------------
// Objet optique courbe. Déclare la structure d'interception commune donnant
//  le point d'intersection, l'angle d'incidence (angle du rayon à la normale),
//  l'angle absolu `ang_normale` du vecteur normal à la courbe (par rapport à
//  l'horizontale), et le sens d'incidence du rayon `sens_reg` (l'objet étant
//  orienté; `true` si même sens que sur les figures du document).

class ObjetCourbe : virtual public Objet {
public:
	struct intercept_courbe_t {
		point_t p_incid;
		float ang_incid;
		float ang_normale;
		bool sens_reg;
	};
	
	// Pour faciliter l'utilisation des `ObjetCourbe`, la méthode `essai_intercept_courbe` revoie
	// directement un pointeur de `intercept_courbe_t`, au lieu de la structure opaque renvoyée par `essai_intercept`.
	virtual std::optional<std::shared_ptr<intercept_courbe_t>> essai_intercept_courbe (const Rayon& ray) const = 0;
	// `essai_intercept` est alors une simple redirection vers `essai_intercept_courbe` et calcul de distance
	virtual Objet::intercept_t essai_intercept (const Rayon& ray) const override final;
	// Le point d'interception est toujours défini pour les `ObjetCourbe`
	virtual std::optional<point_t> point_interception (std::shared_ptr<void> intercept_struct) const override;
	
	// Extrémités de la courbe. Utilisé pour vérifier qu'un `ObjetComposite` est fermé.
	virtual std::pair<point_t,point_t> objet_extremit () const = 0;
	
	// Dessin de l'interception : affiche le rayon et la normale
	virtual void dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> intercept) const override;
};

//------------------------------------------------------------------------------
// Objet optique de forme linéaire : segment du point `a` au point `b`. Routine
//  d'intersection rayon-segment implémentée ici. `essai_intercept_courbe` renvoie
//  une stucture `intercept_t` donnant, en plus de `intercept_curve_t`, l'abscisse
//  du point d'interception `s_incid` ∈ [0,1] sur le segment a-b

class ObjetLigne : virtual public ObjetCourbe {
public:
	point_t a, b;
	
	ObjetLigne (point_t pos_a, point_t pos_b) : a(pos_a), b(pos_b) {}  // point a et b
	ObjetLigne (point_t pos_a, float l_b, float ang_b);                // point a, longueur, et angle (horizontale,a,b)
	
	ObjetLigne& operator= (const ObjetLigne&) = default;
	ObjetLigne (const ObjetLigne&) = default;
	virtual ~ObjetLigne () {}
	
	// Routine d'intersection segment [seg_a,seg_b] avec demi-droite définie par son origine `o_droite` et un angle `ang_droite`.
	// Retrourne (si intersection) le vecteur segment et le vecteur unitaire rayon (pour opti) et l'abscisse `s_seg` sur le segment et `t_dd` de la demi-droite du point d'intersection
	struct intersection_segdd_t { vec_t v_seg; vec_t u_dd; float s_seg; float t_dd; };
	static std::optional<intersection_segdd_t> intersection_segment_demidroite (point_t seg_a, point_t seg_b, point_t o_droite, float ang_droite);
	// Test d'interception du rayon sur la ligne.
	// Si interception, renvoie un pointeur de intercept_ligne_t
	struct intercept_ligne_t : public ObjetCourbe::intercept_courbe_t {
		float s_incid;
	};
	std::optional<std::shared_ptr<intercept_courbe_t>> essai_intercept_courbe (const Rayon& ray) const override final;
	
	// Extension et extrémités du segment
	virtual extension_t objet_extension () const override { return { .pos = a + (b-a)/2, .rayon = !(b-a) }; }
	virtual std::pair<point_t,point_t> objet_extremit () const override { return {a, b}; }

	// Dessin
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
	vec_t vecteur_u_perp () const; // vecteur unitaire perpendiculaire au segment
};

//------------------------------------------------------------------------------
// Objet optique en forme d'arc de cercle, de centre `c`, de rayon `R`, entre les
//  deux angles par rapport à l'horizontale définis par l'intervalle angulaire
//  `ang` dans le sens trigonométrique. Routine d'intersection rayon-arc de cercle
//  implémentée ici `courbe_intercept` renvoie une stucture `intercept_arc_t`
//  donnant, en plus de `intercept_curve_t`, l'angle absolu (par rapport à
//  l'horizontale) `theta_incid` du point d'incidence (dans l'intervalle `ang`).
// Si `inv_int`=true, l'intérieur du cercle est marqué comme étant le milieu extérieur.

class ObjetArc : virtual public ObjetCourbe {
public:
	point_t c;
	float R;
	angle_interv_t ang;
	bool inv_int;
	
	// construction par le centre, le rayon et l'intervalle angulaire
	ObjetArc (point_t centre, float radius, angle_interv_t ang_interval, bool inv_interieur) : c(centre), R(radius), ang(ang_interval), inv_int(inv_interieur) {}
	// construction du petit arc de cercle de rayon R passant par les points `a` et `b`
	ObjetArc (point_t a, point_t b, float radius, bool inv_interieur);
	
	ObjetArc& operator= (const ObjetArc&) = default;
	ObjetArc (const ObjetArc&) = default;
	virtual ~ObjetArc () {}
	
	// Routine d'interception du rayon sur l'arc de cercle.
	// Si interception, renvoie un pointeur de intercept_courbe_t
	struct intercept_arc_t : public ObjetCourbe::intercept_courbe_t {
		float theta_incid;
	};
	std::optional<std::shared_ptr<intercept_courbe_t>> essai_intercept_courbe (const Rayon& ray) const override final;
	
	// Extension et extrémités de l'arc
	virtual extension_t objet_extension () const override { return { .pos = c, .rayon = R }; }
	virtual std::pair<point_t,point_t> objet_extremit () const override;
	
	// Dessin
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
};

//------------------------------------------------------------------------------
// Objet optique fermé composé, délimité par des objets de type `ObjetCourbe`
// Le test de fermeture a seulement le statut d'une assertion à la construction,
//  et n'est pas imposé comme un invariant (`comp` est public).
// Les méthodes d'interception, de ré-émission et de dessin sont, en gros,
//  juste relayés aux sous-objets. Joue le rôle d'un conteneur.

class ObjetComposite : virtual public Objet {
protected:
	// Liste des courbes composant cet objet
	const std::vector< std::unique_ptr<ObjetCourbe> > comp;
	
	void test_fermeture () const;
	
public:
	
	// Construction à partir d'une liste de courbes
	ObjetComposite (std::vector<std::unique_ptr<ObjetCourbe>>&& comp) : Objet(), comp(std::move(comp)) { test_fermeture(); }
	
	ObjetComposite& operator= (const ObjetArc&) = delete; // pas de copie polymorphique des sous-objets
	ObjetComposite (const ObjetArc&) = delete;
	virtual ~ObjetComposite () {}
	
	// Interception : si il y a, le rayon est simplement intercepté par l'objet le plus proche sur son chemin
	struct intercept_composite_t {
		decltype(comp)::const_iterator courbe_intercept; // itérateur de la courbe interceptée; `comp.end()` si pas d'interception
		std::shared_ptr<ObjetCourbe::intercept_courbe_t> intercept_struct; // structure d'interception de cette courbe
	};
	virtual Objet::intercept_t essai_intercept (const Rayon& ray) const override;
	virtual std::optional<point_t> point_interception (std::shared_ptr<void> intercept_struct) const override;
	
	virtual extension_t objet_extension () const override;
	
	// Simple ré-émission par le sous-objet qui a intercepté
	virtual std::vector<Rayon> re_emit (const Rayon& ray, std::shared_ptr<void> intercept) override;
	
	// Dessin
	virtual void dessiner (sf::RenderWindow& window, bool emphasize) const override;
	virtual void dessiner_interception (sf::RenderWindow& window, const Rayon& ray, std::shared_ptr<void> intercept) const override;
};

#endif
