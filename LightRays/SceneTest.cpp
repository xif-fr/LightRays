#include "SceneTest.h"
#include <sstream>
#include <iomanip>
#include <unistd.h>

// Création de la scène, des fenêtres SFML, et optionnellement
//  de l'écran, de la fenêtre, et de la lentille image
//
Scene_TestCommon::Scene_TestCommon (std::wstring nom, bool creer_ecran_image, bool creer_lentille_image) :
	win_scene(nullptr), nom(nom), reset_ecrans(false), frame_i(0), gel(false), affiche_text(true), win_pixels(nullptr) {
	intens_cutoff = 1e-3;
	propag_profondeur_recur_max = 50;
		
	// Création des fenêtres SFML
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	win_scene = new sf::RenderWindow(sf::VideoMode(1100,720), std::wstring(L"Scène test - ")+nom, sf::Style::Close, settings);
	win_scene->setPosition({0,0});
	if (creer_ecran_image) {
		win_pixels = new sf::RenderWindow(sf::VideoMode(100,400), L"CCD", sf::Style::Titlebar, settings);
		win_pixels->setPosition({1100,0});
	}
	win_scene->requestFocus();
	propag_rayons_dessin_window = win_scene;
	
	// Écran et lentille image
	if (creer_ecran_image) {
		if (creer_lentille_image) {
			lentille = this->creer_objet<Objet_MatriceTrsfUnidir>( point_t{1.2,0.5}, 0.2, 0, Objet_MatriceTrsfUnidir::mat_trsf_t{1,0,0,1} );
			this->lentille_mise_au_point(0.2);
		}		
		ecran_image = this->creer_objet<EcranLigne_Multi>( point_t{1.5,0.45}, point_t{1.5,0.55}, (uint16_t)200, 0.0001 );
	}
	
	// Texte d'aide
	static_text = {
		L"[S] (dés)affiche rayons couleur intercept",
		L"[D] (dés)affiche tous rayons transparence lumino, [G]/[F] augmente/diminue lumino rayons",
		L"[Up]/[Down] augmente/diminue gain écrans, [R] reset écrans",
		L"[Space] gel scène, [H] cacher texte",
		L"[(Maj) clic] bouge ancre plus proche"
	};
	if (not font.loadFromFile(FONT_PATH))
		throw std::runtime_error("Échec de chargement de fonte");
}

// Mise au point de la lentille image
//
void Scene_TestCommon::lentille_mise_au_point (float x_obj) {
	if (lentille) {
		float f = -1 / ( 1/(1.5-1.2) + 1/(1.2-x_obj) );
		lentille->mat_trsf = Objet_MatriceTrsfUnidir::mat_trsf_t{1,0,-1/f,1};
	}
}

// Diaphragme autour de la lentille pour garder le CCD à l'ombre
//
void Scene_TestCommon::creer_bloqueurs_autour_lentille (float taille) {
	this->creer_objet<Objet_Bloqueur>(point_t{1.2f,0.6f}, point_t{1.2f,0.6f+taille});
	this->creer_objet<Objet_Bloqueur>(point_t{1.2f,0.4f-taille}, point_t{1.2f,0.4f});
}

// Destruction des fenêtres SFML
//
Scene_TestCommon::~Scene_TestCommon () {
	delete win_scene;
	if (win_pixels != nullptr)
		delete win_pixels;
}

// Boucle principale SFML :
//  - traite les évènements
//  - effectue la propagation des rayons
//  - dessine la scène, gère et affiche les statistiques, gère les écrans
//
void Scene_TestCommon::boucle (std::function<void(sf::Event)> f_event,
							   std::function<void(void)> f_pre_propag,
							   std::function<void(void)> f_post_propag) {
	
	while (win_scene->isOpen()) {
		
		// Gestion des évènements clavier et souris
		sf::Event event;
		while (win_scene->pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				win_scene->close();
			
			// Souris
			bool objet_moved = this->objetsBougeables_event_SFML(event);
			if (objet_moved)
				reset_ecrans = true;
			
			// Commandes clavier
			
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::R)
				reset_ecrans = true;
			
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::S) {
				if (propag_intercept_dessin_window == nullptr)
					propag_intercept_dessin_window = win_scene;
				else
					propag_intercept_dessin_window = nullptr;
			}
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::D) {
				if (propag_rayons_dessin_window == nullptr)
					propag_rayons_dessin_window = win_scene;
				else
					propag_rayons_dessin_window = nullptr;
			}
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::G)
				propag_rayons_dessin_gain *= 1.1;
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::F)
				propag_rayons_dessin_gain *= 0.9;
			
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Up)
				this->ecrans_do([] (Ecran_Base& e) { e.luminosite *= 1.1; });
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Down)
				this->ecrans_do([] (Ecran_Base& e) { e.luminosite *= 0.9; });
			
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::Space)
				gel = !gel;
			if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::H)
				affiche_text = !affiche_text;
			
			if (f_event)
				f_event(event);
		}
		
		if (gel) {
			usleep(1000);
			continue;
		}
		
		// Dessin des objets de la scène avant propagation
		win_scene->clear(sf::Color::Black);
		this->dessiner_scene(*win_scene);
		
		if (f_pre_propag)
			f_pre_propag();
		
		// Reset statistiques et écrans
		stats_n_rayons_emis = 0;
		stats_n_rayons = 0;
		stats_sum_prof_recur = 0;
		stats_n_rayons_profmax = 0;
		stats_n_rayons_discarded = 0;
		
		if (reset_ecrans) {
			this->ecrans_do([] (Ecran_Base& e) { e.reset(); });
			reset_ecrans = false;
			frame_i = 0;
		}
		
		// Émission et propagation des rayons
		this->emission_propagation();
		this->ecrans_do([] (Ecran_Base& e) { e.commit(); });
		
		if (f_post_propag)
			f_post_propag();
		
		// Affichage de la matrice de pixels dans la fenêtre image
		if (win_pixels != nullptr) {
			std::vector<EcranLigne_Multi::pixel_t> pix = ecran_image->matrice_pixels();
			sf::RectangleShape rect;
			rect.setSize(sf::Vector2f( 100, 400./pix.size() ));
			for (size_t k = 0; k < pix.size(); k++) {
				auto color = sf::Color(pix[k].r, pix[k].g, pix[k].b);
				rect.setPosition(sf::Vector2f( 0, k * 400./pix.size() ));
				rect.setFillColor(color);
				win_pixels->draw(rect);
			}
			win_pixels->display();
			while (win_pixels->pollEvent(event)) {}
		}
		
		// Pointeurs des objets bougeables
		this->dessiner_pointeur_nearest_bougeable(*win_scene);
		
		// Affichage de l'aide et des statistiques
		if (affiche_text) {
			std::wstringstream text;
			for (auto& s : static_text)
				text << s << std::endl;
			text << std::endl;
			text << frame_i << L" frame accumulées" << std::endl;
			text << stats_n_rayons_emis << " rayons primaires, " << stats_n_rayons << " rayons tot, " << std::fixed << std::setprecision(1) << (stats_sum_prof_recur/(float)stats_n_rayons) << " prof recur moy, " << stats_n_rayons_discarded << L" rayons jetés, " << stats_n_rayons_profmax << " max prof" << std::endl;
			text << std::setprecision(3) << "pointeur : (" << mouse.x << "," << mouse.y << ")";
			win_scene->draw( sf::c01::buildText(font, point_t{0.1f,0.015f*(8+static_text.size())}, {text.str()}, sf::Color::White) );
			
			win_scene->draw( sf::c01::buildText(font, point_t{1,0.1}, {this->nom}, sf::Color(255,255,255,160), 22) );
			
			if (lentille != nullptr)
				win_scene->draw( sf::c01::buildText(font, this->lentille->b+vec2_t{-0.03,0}, {L"Lentille"}, sf::Color::White) );
		}
		
		// Affichage SFML
		win_scene->display();
		frame_i++;
	}
}

// Création d'une source d'un unique rayon rouge, déplaçable et tournable.
//
std::shared_ptr<Source_UniqueRayon> Scene_TestCommon::creer_source_unique_rayon (point_t pos, float dir_angle, float ampl) {
	auto source = std::make_shared<Source_UniqueRayon>(pos, dir_angle, (color_id_t)0, ampl);
	this->ajouter_bouge_action(source->position, [source] (point_t mouse, float angle, bool alt) -> point_t {
		if (alt) source->dir_angle = angle;
		else source->position = mouse;
		return source->position;
	});
	this->sources.push_back(source);
	return source;
}

// Création d'une source ponctuelle blanche omnidirectionnelle sur un secteur angulaire [ang_base-ang_ext, ang_base+ang_ext]
//
std::shared_ptr<Source_PonctOmni> Scene_TestCommon::creer_source_omni_secteur (point_t pos, float ang_ext, float ang_base, float dens_ray) {
	auto source = std::make_shared<Source_PonctOmni>(pos, spectre_blanc);
	source->dens_ang = dens_ray;
	source->dir_alea = true;
	this->ajouter_bouge_action(source->position, [ang_ext,source] (point_t mouse, float angle, bool alt) -> point_t {
		if (alt) source->secteur = angle_interv_t(-ang_ext,+ang_ext) + angle;
		else source->position = mouse;
		return source->position;
	});
	source->secteur = angle_interv_t(-ang_ext,+ang_ext) + ang_base;
	this->sources.push_back(source);
	return source;
}

// Création d'une source linéaire lambertienne bleue qui prend tout le côté gauche de la scène.
//
std::shared_ptr<Source_LinLambertien> Scene_TestCommon::creer_ciel_bleu (float dens_lin) {
	auto ciel = std::make_shared<Source_LinLambertien>(
		/*a*/point_t{0.05,0.05}, /*segment*/vec_t{0,0.90},
		Specte::polychromatique({{0,0,1.1,2}})
	);
	ciel->dens_lin = dens_lin;
	this->sources.push_back(ciel);
	this->ajouter_bouge_action(ciel->a, [ciel] (point_t mouse, float, bool) -> point_t {
		ciel->a = mouse;
		return mouse;
	});
	return ciel;
}

// Création d'écrans d'épaisseur `a` tout autour de la scène.
//
void Scene_TestCommon::creer_ecrans_autour (float lumino, float a) {
	float b = (float)(win_scene->getSize().x)/SFMLC01_WINDOW_UNIT;
	float bin_density = 50;
	this->creer_objet<EcranLigne_Multi>( point_t{  a,   a}, point_t{b-a,   a}, bin_density, lumino )->epaisseur_affich = a;
	this->creer_objet<EcranLigne_Multi>( point_t{b-a,   a}, point_t{b-a, 1-a}, bin_density, lumino )->epaisseur_affich = a;
	this->creer_objet<EcranLigne_Multi>( point_t{b-a, 1-a}, point_t{  a, 1-a}, bin_density, lumino )->epaisseur_affich = a;
	this->creer_objet<EcranLigne_Multi>( point_t{  a, 1-a}, point_t{  a,   a}, bin_density, lumino )->epaisseur_affich = a;
}