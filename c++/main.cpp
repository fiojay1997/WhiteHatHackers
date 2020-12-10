/**
 * TEAM: WHITEHATHACKERS 
 * MEMBERS: Jake, Leon, Nanda, Kaijie, Sephora, Austin
 * DATE: 10/08/2020
 * 
 **/

#include "encode_video.cpp"
#include <vector>
#include <dirent.h>

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
    const int nframes = 1;

    //initialize video
    video movie("output", width, height);

    for (int iframe = 0; iframe < nframes; iframe++)
    {
        DIR * dir = NULL;
        struct dirent * ent = NULL;
        dir = opendir("./stitched_img");
        if (dir != NULL)
        {
            ent = readdir(dir);
            while ((ent = readdir(dir)) != NULL)
            {
                string str(ent->d_name);
                if (str != "." && str != "..")
                {
                    string final_str = "./stitched_img/" + str;
                    cout << final_str << endl;
                    movie.addFrame(final_str);
                }
            }
            closedir(dir);
        }
        else 
        {
            cout << "failed to read files" << endl;
        }
    }

    return 0;
}
