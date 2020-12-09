#include "video.cpp"
#include <vector>

using namespace std;

int main()
{
    const int width = 1080;
    const int height = 900;
    const int nframes = 100;

    video movie("svg_image", width, height);

    for (int iframe = 0; iframe < nframes; iframe++)
    {
        
        movie.addFrame("test.png");
        movie.addFrame("output.png");
        movie.addFrame("sun.png");
    }
    
}
