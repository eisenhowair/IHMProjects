
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include "RoiEtendu.h"
#include "Oeuvre.h"
#include <filesystem>
#include <fstream>

#define WIDTH 1280 //80 * 8 = 640
#define HEIGHT 720 // 60 * 8 = 480
#define NBR_ART 10

using namespace cv;
using namespace std;


int longueur_point( Point p, int nbr_images )
{
	vector<int> liste_points;
	for( int i = 0; i < nbr_images; i++ )
	{
		if( (p.x >= WIDTH / nbr_images * i) && (p.x <= WIDTH / nbr_images * (i + 1)) )
		{
			return i;
		}
	}
	return 0;
}


int hauteur_point( Point p )
{
	return p.y >= HEIGHT / 4 * 3 ? -1 : 1;
}

void putTextMultiline( Mat& img, const string& text, const Point& org, const Scalar& color, double fontScale, int thickness, int lineType )
{
	istringstream ss( text );
	string line;

	int y = org.y;

	while( getline( ss, line, '\n' ) )
	{
		putText( img, line, Point( org.x, y ), FONT_HERSHEY_SIMPLEX, fontScale, color, thickness, lineType );
		y += static_cast<int>(img.rows * 0.03);
	}
}


void afficheOverlay( Mat& frame, vector<RoiEtendu>& ensembleRoi, vector<Oeuvre>& ensembleImages, int numero_active_image, bool en_haut, int nbr_images ) {
	int largeur_image = WIDTH / nbr_images;

	string texte;
	for( size_t i = 0; i < ensembleRoi.size(); i++ ) {
		auto& elem_roi = ensembleRoi[i];

		if( elem_roi.rectangle().x >= 0 && elem_roi.rectangle().y >= 0 &&
			elem_roi.rectangle().x + elem_roi.rectangle().width <= frame.cols &&
			elem_roi.rectangle().y + elem_roi.rectangle().height <= frame.rows ) {

			Mat subView = frame( elem_roi.rectangle() );

			if( i < ensembleImages.size() && !ensembleImages[i].getArtwork().empty() ) {
				resize( ensembleImages[i].getArtwork(), ensembleImages[i].d_artwork, Size( largeur_image, HEIGHT / nbr_images ) );

				Mat overlay = Mat( subView.size(), ensembleImages[i].getArtwork().type() );
				ensembleImages[i].getArtwork().copyTo( overlay );

				if( !subView.empty() && !overlay.empty() ) {
					addWeighted( subView, 1.0, overlay, 1.0, 0.0, subView );
				}
				else {
					cerr << "Erreur : Le mélange addWeighted a échoué. subView ou overlay est vide." << endl;
					cout << "Dimensions de subView : " << subView.size() << endl;
					cout << "Dimensions de overlay : " << overlay.size() << endl;
					cout << "Type de subView : " << subView.type() << endl;
					cout << "Type de overlay : " << overlay.type() << endl;
				}
			}
			else {
				cerr << "Erreur : L'image source est vide." << endl;
			}
		}
		else {
			cerr << "Erreur : La région d'intérêt dépasse les dimensions de la frame." << endl;
		}
	}

	if( en_haut ) {
		// décrémentation de  numero_active_image de 1 pour obtenir l'index correct dans ensembleImages
		int index_image = numero_active_image - 1;
		numero_active_image = nbr_images - numero_active_image + 1;

		if( index_image >= 0 && index_image < ensembleImages.size() ) {
			Mat focus_mat = imread( ensembleImages.at( numero_active_image - 1 ).getChemin(), IMREAD_COLOR );
			Oeuvre focus_oeuvre( ensembleImages.at( numero_active_image - 1 ).getChemin(), focus_mat );

			// Calcul des dimensions pour l'affichage au milieu de l'écran
			int largeur_image = WIDTH / 4;
			int hauteur_image = HEIGHT / 4;
			if( largeur_image <= 0 || hauteur_image <= 0 ) {
				cerr << "Erreur : Dimensions de redimensionnement non valides." << endl;
				return;
			}

			int x_centre = (WIDTH - largeur_image) / 2;
			int y_centre = (HEIGHT - hauteur_image) / 2;

			Rect focus_image = Rect( x_centre, y_centre, largeur_image, hauteur_image );
			Mat subVue = frame( focus_image );

			Mat resized_artwork;


			if( !focus_oeuvre.getArtwork().empty() ) {
				resize( focus_oeuvre.getArtwork(), resized_artwork, Size( largeur_image, hauteur_image ) );
			}
			else {
				cerr << "Erreur : Image d'art vide." << endl;
				return;
			}

			if( !subVue.empty() && !resized_artwork.empty() ) {
				addWeighted( subVue, 0.4, resized_artwork, 0.5, 0.0, subVue );


				if( numero_active_image - 1 >= 0 && numero_active_image - 1 < ensembleImages.size() ) {
					texte = ensembleImages.at( numero_active_image - 1 ).getDescription();
				}
				else {
					cerr << "Erreur : Accès à un indice invalide dans ensembleImages." << endl;
				}

				line( frame, Point( x_centre + largeur_image, y_centre + HEIGHT / 4 ),
					Point( x_centre + largeur_image, y_centre + hauteur_image / HEIGHT / 3.5 ), Scalar( 200, 80, 80 ), 2, LINE_4 );

				Point position_texte( x_centre + largeur_image + 20, y_centre + hauteur_image / 4 );
				putTextMultiline( frame, texte, Point( x_centre + largeur_image + 20, y_centre + hauteur_image / 4 ),
					Scalar( 255, 255, 255 ), 0.7, 1, LINE_AA );
			}
			else {
				cerr << "Erreur : subView ou resized_artwork est vide." << endl;
			}
		}
	}

	imshow( "flux_cam", frame );
}


void processFrame( Mat& frame, vector<RoiEtendu>& ensembleRoi, vector<Oeuvre>& ensembleImage, int nbr_images ) {

	int espaceEntreImages = WIDTH / nbr_images;
	vector<int> liste_points( nbr_images );
	int haut_ou_bas = 0;

	Mat hsv;
	cvtColor( frame, hsv, cv::COLOR_BGR2HSV );

	Mat lower_red, upper_red, red_mask;
	inRange( hsv, Scalar( 0, 140, 140 ), Scalar( 5, 255, 255 ), lower_red );
	inRange( hsv, Scalar( 160, 120, 120 ), Scalar( 180, 255, 255 ), upper_red );
	addWeighted( lower_red, 1.0, upper_red, 1.0, 0.0, red_mask );

	vector<Point> red_pixel_coordinates;
	findNonZero( red_mask, red_pixel_coordinates );

	// détermine si le rouge est majoritairement dans la partie supérieure ou inférieure de l'écran
	for( int k = 0; k < red_pixel_coordinates.size(); k++ ) {
		liste_points[longueur_point( red_pixel_coordinates[k], nbr_images )]++;
		haut_ou_bas += hauteur_point( red_pixel_coordinates[k] );
	}

	bool en_haut = haut_ou_bas > 0;
	auto max_element_iterator = max_element( liste_points.begin(), liste_points.end() );

	int num_image_hovered = distance( liste_points.begin(), max_element_iterator ) + 1;
	Scalar line_Color( 192, 192, 192 );

	for( int i = 1; i <= nbr_images; i++ )
	{
		if( i == num_image_hovered ) {
			line_Color = Scalar( (i * 30) % 255, (i * 60) % 255, (i * 90) % 255 );
		}
	}

	flip( frame, frame, +1 );

	afficheOverlay( frame, ensembleRoi, ensembleImage, num_image_hovered, en_haut, nbr_images );
}


vector<string> putDataInVector17( vector<Oeuvre>& ensembleImages )
{
	vector<string> ressourceInfos;
	int count_image = 0;
	const filesystem::path folder{ "Ressources" };
	ressourceInfos.push_back( folder.string() );

	for( auto const& folder_elem : std::filesystem::directory_iterator{ folder } )
	{
		String chemin = folder_elem.path().string();
		cout << folder_elem.path() << '\n';
		if( folder_elem.path().extension() == ".png" || folder_elem.path().extension() == ".jpg" )
		{
			Mat new_image = imread( chemin, IMREAD_COLOR );
			if( new_image.empty() ) {
				cerr << "Erreur : Impossible de charger l'image " << chemin << endl;
			}
			else {
				Oeuvre art_piece( chemin, new_image );
				art_piece.setDescription( folder_elem.path().filename().stem().string() );
				ensembleImages.push_back( art_piece );
				count_image++;
			}
		}
	}
	ressourceInfos.push_back( to_string( count_image ) );
	return ressourceInfos;
}

vector<RoiEtendu> fillRoiWithData( vector<RoiEtendu>& ensembleRoi, int nbr_images )
{
	for( int i = 0; i < nbr_images; i++ )
	{
		RoiEtendu roiElem( WIDTH / nbr_images * i, HEIGHT - HEIGHT / nbr_images * 1.5, WIDTH / nbr_images, HEIGHT / nbr_images );
		ensembleRoi.push_back( roiElem );
	}

	return ensembleRoi;
}

int main() {

	int nbr_images;
	int nbr_pixel_droite, nbr_pixel_centre, nbr_pixel_gauche, concentration_point;
	Mat frame;
	Mat lower_red, upper_red, red_mask, hsv;

	VideoCapture cap;
	cap.open( 0 );

	cap.set( CAP_PROP_FRAME_WIDTH, WIDTH );
	cap.set( CAP_PROP_FRAME_HEIGHT, HEIGHT );

	if( !cap.isOpened() ) {
		cerr << "Erreur lors de la lecture video" << endl;
		return -1;
	}

	namedWindow( "flux_cam", WINDOW_NORMAL );
	resizeWindow( "flux_cam", WIDTH, HEIGHT );



	vector<Oeuvre> ensembleImages;

	vector<string> ressourceInfos;
	ressourceInfos = putDataInVector17( ensembleImages ); // nécessite c++17 pour filesystem
	nbr_images = stoi( ressourceInfos[1] );

	vector<RoiEtendu> ensembleRoi;
	ensembleRoi = fillRoiWithData( ensembleRoi, nbr_images );



	while( true ) {
		Mat frame, BW_Frame;
		cap >> frame;

		if( frame.empty() ) {
			break;
		}

		processFrame( frame, ensembleRoi, ensembleImages, nbr_images );

		if( waitKey( 30 ) >= 0 ) {
			break;
		}
	}

	return 0;
}