#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <cstdlib> // pour obtenir le random
#include <ctime> // pour obtenir le random
#include <cmath> // calcul du point dans le cercle
#include "Cercle.h"

using namespace std;
using namespace cv;

int genere_coordinate( int min, int max )
{
	return min + std::rand() % (max - min + 1);
}

bool point_dans_cercle( Point p, Cercle cible )
{
	return pow( p.x - cible.x(), 2 ) + pow( p.y - cible.y(), 2 ) < pow( cible.rayon(), 2 );
}

Cercle genere_cercle( int width, int height)
{
	int coord_radius = genere_coordinate( 30, width / 10 );

	// pour que le cercle ne dépasse pas de la frame
	int coord_x = genere_coordinate( coord_radius, width - coord_radius );
	int coord_y = genere_coordinate( coord_radius, height - coord_radius );
	Cercle new_cercle( coord_x, coord_y, coord_radius );
	return new_cercle;
}


int main()
{
	VideoCapture cap;
	Mat frame, BW_Frame;
	Mat lower_red, upper_red, red_mask, hsv;
	double b, g, r;
	Vec3b point_hsv;

	cap.open( 0 );
	int width = static_cast<int>(cap.get( cv::CAP_PROP_FRAME_WIDTH ));
	int height = static_cast<int>(cap.get( cv::CAP_PROP_FRAME_HEIGHT ));

	//Pour le cercle 
	srand( time( 0 ) );
	Cercle cible = genere_cercle(width,height);
	Scalar line_Color( 0, 0, 0 );
	int thickness = 5;
	bool cercle_touche = false; // pour savoir s'il faut générer un cercle
	int score = 0; // pour laisser un léger temps entre les cercles
	int win_streak = 0, duree_cercle = 60;
	double score_multiplicateur = 1;


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
		// on retourne l'écran (miroir) pour utiliser plus instinctivement
		flip( frame, frame, +1 );

		// Conversion en hsv
		cvtColor( frame, hsv, cv::COLOR_BGR2HSV );

		// definition du filtre/masque pour rouge
		inRange( hsv, Scalar( 0, 140, 140 ), Scalar( 5, 255, 255 ), lower_red );
		inRange( hsv, Scalar( 160, 120, 120 ), Scalar( 180, 255, 255 ), upper_red );
		addWeighted( lower_red, 1.0, upper_red, 1.0, 0.0, red_mask );

		// liste des points rouges avec coordonnées
		vector<cv::Point> red_pixel_coordinates;
		findNonZero( red_mask, red_pixel_coordinates );

		// regarde si un des pixel rouge est dans le cercle
		for( int k = 0; k < red_pixel_coordinates.size(); k++ ) {
			if( point_dans_cercle( red_pixel_coordinates[k], cible ) ) {
				cercle_touche = true;
				break;
			}
		}

		if( cercle_touche ) {
			win_streak++;
			score_multiplicateur = 1.0 + static_cast<double>(win_streak) / 10.0;
			score += (width - cible.rayon()) * score_multiplicateur;
			cible = genere_cercle(width,height);
			cercle_touche = false;

			if( win_streak >= 5 ) {
				duree_cercle = max( 60 / pow( 2, win_streak / 5 ), static_cast<double>(10) );
			}
			else {
				duree_cercle = 60;
			}
		}
		if( win_streak >= 5 ) {
			string combo_text = "COMBO";
			putText( frame, combo_text, Point( width - width / 4, 30 ), FONT_HERSHEY_SIMPLEX, 1, Scalar( 0, 0, 255 ), 2 );
		}

		duree_cercle--;
		if( duree_cercle <= 0 ) {
			win_streak = 0;
			cible = genere_cercle(width, height);
			duree_cercle = 60;
		}


		Point centre_cercle( cible.x(), cible.y() );
		circle( frame, centre_cercle, cible.rayon(), line_Color, thickness );

		string score_text = "Score: " + to_string( score );
		putText( frame, score_text, Point( 10, 30 ), FONT_HERSHEY_SIMPLEX, 1, Scalar( 255, 0, 0 ), 2 );
		if( score_multiplicateur > 1 )
		{
			ostringstream stream;
			stream << fixed << setprecision( 1 ) << score_multiplicateur;
			string bonus_text = "Bonus: " + stream.str() + " encore " + to_string( duree_cercle );
			putText( frame, bonus_text, Point( 10, 70 ), FONT_HERSHEY_SIMPLEX, 1, Scalar( 0, 255, 0 ), 2 );
		}

		// affichage des frames
		imshow( "flux_cam", frame );

		if( waitKey( 30 ) >= 0 ) {
			break;
		}
	}
}