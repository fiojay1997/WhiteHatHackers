/**
 * TEAM: WHITEHATHACKERS 
 * MEMBERS: Jake, Leon, Nanda, Kaijie, Sephora, Austin
 * DATE: 10/08/2020
 * 
 **/

#include "encode_video.cpp"
#include <vector>

using namespace std;

/**
 * Main starting point of program
 * Final Video product svg_image.mp4
 **/
int main()
{

    //set variables for final movie output
    const int width = 1080;
    const int height = 900;
    const int nframes = 100;

    //initialize video
    video movie("output", width, height);

    for each frame of movie
    for (int iframe = 0; iframe < nframes; iframe++)
    {
      //add frame
      movie.addFrame("output.png");
    }

    
}
