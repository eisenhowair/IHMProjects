#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <Windows.h>
#include <cstdlib> // pour obtenir le random
#include <ctime> // pour obtenir le random
#include "Cercle.h"

/*#define width 1280  //80 * 8 = 640
#define HEIGHT 720 // 60 * 8 = 480
#define dim_coeur HEIGHT / 17
*/
using namespace std;
using namespace cv;

int genere_coordinate( int min, int max )
{
	return min + std::rand() % (max - min + 1);
}



void putTextMultiline( int numero_mesure, Mat& img, const Point& org, const Scalar& color, double fontScale, int thickness, int lineType )
{
	istringstream ss;
	if( numero_mesure == 0 )
	{
		ss = istringstream( "Appuyer sur ESPACE \npour sauvegarder les seuils minimums" );
	}
	else if( numero_mesure == 1 )
	{
		ss = istringstream( "Appuyer sur ESPACE \npour sauvegarder le seuil MAX de jaune" );
	}
	else if( (numero_mesure == 2) )
	{
		ss = istringstream( "Appuyer sur ESPACE \npour sauvegarder le seuil MAX de rouge" );
	}
	else {
		ss = istringstream( "GAME OVER \nAppuyez sur 'r' pour recommencer" );
	}
	string line;

	int y = org.y;

	while( getline( ss, line, '\n' ) )
	{
		putText( img, line, Point( org.x, y ), FONT_HERSHEY_SIMPLEX, fontScale, color, thickness, lineType );
		y += static_cast<int>(img.rows * 0.03);
	}
}


int detectionCouleur( Mat& frame, Mat masque_bas, Mat masque_haut )
{
	Mat masque_filtre;
	vector<Point> points_frame;

	addWeighted( masque_bas, 1.0, masque_haut, 1.0, 0.0, masque_filtre );

	if( !masque_filtre.empty() ) {
		findNonZero( masque_filtre, points_frame );
	}
	else {
		cerr << "Masque filtre vide" << endl;
		return -1;
	}

	return points_frame.size();
}

double updateCoordinate( double old_position_cercle, string couleur_gagnante, double ecart, int width, int height )
{
	double modulation_factor = 2 + (28 / (1 + exp( -5 * (ecart - 0.5) )));

	double new_position_cercle;
	if( couleur_gagnante == "j" )
	{
		new_position_cercle = old_position_cercle - modulation_factor;
		return max( new_position_cercle, height / 12 + width / 12 );
	}
	else if( couleur_gagnante == "r" )
	{
		new_position_cercle = old_position_cercle + modulation_factor;
		return min( new_position_cercle, width - (height / 12 + width / 12) );
	}
	else {
		return width / 2;
	}
}

bool cerclesEnContact( const Point& centre1, int rayon1, const Point& centre2, int rayon2 ) {

	double distanceCentres = norm( centre1 - centre2 );

	if( distanceCentres < rayon1 + rayon2 ) {
		return true;
	}

	return false;
}




void drawHeader( Mat& frame, int visibleHearts, int score, int niveau, int width, double dim_coeur ) {
	int gap = 5;
	int topRightCorner = frame.cols - (frame.cols / 12);
	int verticalOffset = -20;

	for( int i = 0; i < visibleHearts; ++i ) {
		Point topLeft( topRightCorner - (i + 1) * (dim_coeur + gap), dim_coeur + verticalOffset );
		Point topRight( topLeft.x + dim_coeur, dim_coeur + verticalOffset );
		Point bottomLeft( topLeft.x + dim_coeur / 4, dim_coeur + dim_coeur / 2 + verticalOffset );
		Point bottomRight( topLeft.x + 3 * dim_coeur / 4, dim_coeur + dim_coeur / 2 + verticalOffset );
		Point middleBottom( topLeft.x + dim_coeur / 2, dim_coeur * 2 + verticalOffset );

		Scalar color( 0, 0, 255, 0 );
		int thickness = 2;

		// les lignesdu bas pour représenter les cœurs
		line( frame, topLeft, middleBottom, color, thickness );
		line( frame, topRight, middleBottom, color, thickness );

		// les ellipses pour le haut des coeurs
		ellipse( frame, Point( topLeft.x + dim_coeur / 4, dim_coeur + verticalOffset ), Size( dim_coeur / 4, dim_coeur / 4 ), 0, 180, 360, color, thickness );
		ellipse( frame, Point( topLeft.x + 3 * dim_coeur / 4, dim_coeur + verticalOffset ), Size( dim_coeur / 4, dim_coeur / 4 ), 0, 180, 360, color, thickness );

		// Affichage des autres informations
		string text = "Score: " + to_string( score ) + "  Niveau: " + to_string( niveau );
		Point textPosition( width / 11, middleBottom.y );
		putText( frame, text, textPosition, FONT_HERSHEY_SIMPLEX, 0.8, Scalar( 255, 255, 255 ), 1, LINE_AA );
	}
}


int main()
{

	VideoCapture cap;
	Mat frame;

	Mat lower_yellow, upper_yellow, yellow_mask;
	Mat lower_red, upper_red, red_mask, hsv;


	cap.open( 0 );
	int width = static_cast<int>(cap.get( cv::CAP_PROP_FRAME_WIDTH ));
	int height = static_cast<int>(cap.get( cv::CAP_PROP_FRAME_HEIGHT ));
	double dim_coeur = height / 17.0;

	bool lignes_a_afficher = true;
	int prise_de_mesure = 0;
	bool mode_mesure = true;
	bool cible_presente = false;
	bool game_over = false;

	// Points jauge gauche bord
	Point p1( width / 12, height );
	Point p2( width / 12, 0 );
	Point p3( 0, 0 );
	Point p4( 0, height );


	// Points jauge droite bord
	Point p5( 11 * width / 12, height );
	Point p6( 11 * width / 12, 0 );
	Point p7( width, 0 );
	Point p8( width, height );


	//Pour facilité d'utilisation
	int hauteur_jauge_j = p2.y + 3;
	int hauteur_jauge_r = p7.y + 3;


	//Valeurs chaque frame
	int nbr_pixel_jaune = 0;
	int nbr_pixel_rouge = 0;
	double nbr_pix_jaune_norm, nbr_pix_rouge_norm;

	//Valeurs seuils
	int nbr_pixel_min_total = 0, nbr_pixel_min_rouge = 0, nbr_pixel_min_jaune = 0;
	int nbr_pixel_max_rouge = 0, nbr_pixel_max_jaune = 0;
	double position_cercle = width / 2;

	//Initialisation de la cible à toucher
	Point centre_cible;
	int score = 0, niveau = 1, nbr_vie = 3;
	srand( time( 0 ) );



	if( !cap.isOpened() ) {
		cerr << "Erreur lors de la lecture video" << endl;
		return -1;
	}
	namedWindow( "flux_cam", WINDOW_NORMAL );
	resizeWindow( "flux_cam", width, height );




	while( 1 ) {
		cap >> frame;

		if( frame.empty() ) {
			break;
		}
		flip( frame, frame, +1 );
		cout << width << " height:" << height << " dim_coeur: " << dim_coeur << endl;

		//pour ne pas comptabiliser les pixels des jauges et coeurs dans la détection de couleur
		Rect roi_centrale( p1.x, dim_coeur * 1.5, 10 * width / 12, height - dim_coeur * 1.5 );
		Rect window_size = getWindowImageRect( "flux_cam" );



		Mat frame_centrale = frame( roi_centrale );
		if( game_over )
		{
			putTextMultiline( -1, frame, Point( width / 6, height / 4 ), Scalar( 255, 255, 255 ), 0.7, 1, LINE_AA );
		}

		if( prise_de_mesure < 3 )
		{
			putTextMultiline( prise_de_mesure, frame, Point( width / 6, height / 4 ), Scalar( 255, 255, 255 ), 0.7, 1, LINE_AA );
		}

		// rectangle jauge gauche bord 
		line( frame, p1, p2, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p2, p3, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p3, p4, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p4, p1, Scalar( 0.7 ), LINE_4, 0 );

		line( frame, p5, p6, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p7, p6, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p7, p8, Scalar( 0.7 ), LINE_4, 0 );
		line( frame, p8, p5, Scalar( 0.7 ), LINE_4, 0 );

		rectangle( frame, Point( p1.x - 3, p1.y - 3 ), Point( 3, hauteur_jauge_j ), Scalar( 0, 255, 255 ), -1, LINE_8, 0 );
		rectangle( frame, Point( p5.x + 3, p5.y - 3 ), Point( p7.x - 3, hauteur_jauge_r ), Scalar( 0, 0, 255 ), -1, LINE_8, 0 );



		// Conversion en hsv, masques jaunes et rouges
		cvtColor( frame_centrale, hsv, cv::COLOR_BGR2HSV );
		inRange( hsv, Scalar( 15, 80, 80 ), Scalar( 35, 255, 255 ), lower_yellow );
		inRange( hsv, Scalar( 25, 80, 80 ), Scalar( 45, 255, 255 ), upper_yellow );
		inRange( hsv, Scalar( 0, 140, 140 ), Scalar( 5, 255, 255 ), lower_red );
		inRange( hsv, Scalar( 160, 120, 120 ), Scalar( 180, 255, 255 ), upper_red );


		if( mode_mesure && prise_de_mesure < 3 )
		{

			switch( prise_de_mesure )
			{
				// Enregistrement des seuils minimum
			case 0:
				nbr_pixel_min_jaune = detectionCouleur( frame_centrale, lower_yellow, upper_yellow );
				nbr_pixel_min_rouge = detectionCouleur( frame_centrale, lower_red, upper_red );
				break;

				// Enregistrement du seuil maximum jaune
			case 1:

				nbr_pixel_max_jaune = detectionCouleur( frame_centrale, lower_yellow, upper_yellow );
				break;

				// Enregistrement du seuil maximum rouge
			case 2:

				nbr_pixel_max_rouge = detectionCouleur( frame_centrale, lower_red, upper_red );
				break;

			default:
				break;
			}
		}
		if( !mode_mesure && !game_over )
		{
			//s'il n'y a pas de cible sur l'écran, en créer une
			if( !cible_presente )
			{
				centre_cible = Point( genere_coordinate( (height / 17 + width / 12), (width - (height / 17 + width / 12)) ), 2 * height / 3 );
				cible_presente = true;
			}
			else {
				centre_cible.y -= 5 * niveau;

				//si la cible atteint le bord
				if( centre_cible.y < height / 17 )
				{
					nbr_vie--;
					cout << "Vies restantes : " << nbr_vie << endl;
					if( nbr_vie == 0 )
					{
						game_over = true;
					}

					cible_presente = false;
				}
			}
			circle( frame, centre_cible, height / 17, Scalar( 255, 180, 180 ), 5 );



			nbr_pixel_rouge = detectionCouleur( frame_centrale, lower_red, upper_red );
			nbr_pixel_jaune = detectionCouleur( frame_centrale, lower_yellow, upper_yellow );

			//On normalise pour éviter les valeurs extrêmes entre rouge et jaune
			if( ((nbr_pixel_max_jaune - nbr_pixel_min_jaune) != 0) && ((nbr_pixel_max_rouge - nbr_pixel_min_rouge) != 0)) {
				nbr_pix_jaune_norm = static_cast<double>(nbr_pixel_jaune - nbr_pixel_min_jaune) / (nbr_pixel_max_jaune - nbr_pixel_min_jaune);
				nbr_pix_rouge_norm = static_cast<double>(nbr_pixel_rouge - nbr_pixel_min_rouge) / (nbr_pixel_max_rouge - nbr_pixel_min_rouge);
			}
			else {
				//problème au niveau des mesures
				nbr_pix_rouge_norm = 1;
				nbr_pix_jaune_norm = 1;
			}
			hauteur_jauge_r = static_cast<int>(height * (1 - nbr_pix_rouge_norm));
			hauteur_jauge_j = static_cast<int>(height * (1 - nbr_pix_jaune_norm));


			Point centre_cercle( position_cercle, height / 11 );
			circle( frame, centre_cercle, height / 12, Scalar( 255, 180, 180 ), 5 );
			if( cerclesEnContact( centre_cible, height / 17, centre_cercle, height / 12 ) )
			{
				cible_presente = false;
				score++;
				if( score >= 10 )
				{
					niveau++;
					score = 0;
					nbr_vie = 3;
				}
			}


			if( nbr_pix_jaune_norm > nbr_pix_rouge_norm )
			{
				cout << "Avantage JAUNE :" << nbr_pix_jaune_norm - nbr_pix_rouge_norm << endl;
				position_cercle = updateCoordinate( position_cercle, "j", nbr_pix_jaune_norm - nbr_pix_rouge_norm, width, height );

			}
			else if( nbr_pix_rouge_norm > nbr_pix_jaune_norm )
			{
				cout << "Avantage ROUGE :" << nbr_pix_rouge_norm - nbr_pix_jaune_norm << endl;
				position_cercle = updateCoordinate( position_cercle, "r", nbr_pix_rouge_norm - nbr_pix_jaune_norm, width, height );

			}


		}


		// affichage des frames
		drawHeader( frame, nbr_vie, score, niveau, width, dim_coeur );
		imshow( "flux_cam", frame );

		int key = waitKey( 30 );
		if( key != -1 ) {
			if( key == 32 )
			{
				//Pour différencer les différentes mesures à prendre
				//'ESPACE' enregistre les valeurs à chaque itération
				prise_de_mesure++;
				if( prise_de_mesure < 3 )
				{
					mode_mesure = true;
				}
				else {
					mode_mesure = false;
				}
			}

			// 'r' pour réinitialiser la prise de mesure à tout moment
			else if( key == 114 ) {
				prise_de_mesure = 0;
				mode_mesure = true;
				score = 0;
				game_over = false;
				niveau = 1;
				nbr_vie = 3;

			}
			else {
				mode_mesure = false;
				break;
			}
		}


	}
	return 0;
}