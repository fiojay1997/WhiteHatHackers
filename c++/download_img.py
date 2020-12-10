import os
import sys
from PIL import Image


def get_img():
    if not os.path.exists('../res'):
        os.makedirs('../res')
    os.system("aws s3 sync s3://weather-cs3505-bucket ../res")
    os.system("aws s3 sync s3://sunrise-bucket ../res")
    return get_files('../res')
    

def get_files(path):
    onlyfiles = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    if '.DS_Store' in onlyfiles:
        onlyfiles.remove('.DS_Store')
    return onlyfiles  


def stitch_img():
    # create the directory for storing the stitched images
    if not os.path.exists('stitched_img'):
        os.makedirs('stitched_img')
    sunrise_imgs = get_files('./stitched_img')
    weather_imgs = get_files('./stitched_img')
    min_length = min(len(sunrise_imgs), len(weather_imgs))
    for i in range(0, min_length):
        imgs_str = '"../res/sunrise/' +str(sunrise_imgs[i]) + '"' + ' "../res/weather/' + str(weather_imgs[i]) + '" '
        if not os.path.exists(str(i)):
            os.makedirs(str(i))
        os.system('cp ' + imgs_str + ' ' + str(i))


def concat_img():
    files = get_files("./stitched_img")
    os.chdir('stitched_img')
    images = [Image.open(x) for x in files]
    widths, heights = zip(*(i.size for i in images))

    total_width = sum(widths)
    max_height = max(heights)

    new_im = Image.new('RGB', (total_width, max_height))

    x_offset = 0
    for im in images:
        new_im.paste(im, (x_offset,0))
        x_offset += im.size[0]

    new_im.save(files + 'concated.png')


def resize_img():
    files = get_files("./stitched_img")
    basewidth = 900
    baseheight = 900

    i = 0
    for f in files:
        img = Image.open('./stitched_img' +f)
        img = img.resize((basewidth,baseheight), Image.ANTIALIAS)
        f = str(i)
        i += 1
        img.save('./stitched_img' + f + '.png') 


if __name__ == "__main__":
    resize_img()