import os
import sys
from PIL import Image


def get_img():
    if not os.path.exists('./res'):
        os.makedirs('./res')
    os.system("aws s3 sync s3://air-quality-cs3505-bucket ./res")
    os.system("aws s3 sync s3://fire-cs3505-bucket ./res")
    os.system("aws s3 sync s3://soil-cs3505-bucket ./res")
    os.system("aws s3 sync s3://sunrise-cs3505-bucket ./res")
    os.system("aws s3 sync s3://water-cs3505-bucket ./res")
    os.system("aws s3 sync s3://weather-cs3505-bucket ./res")
    return get_files('./res')
    

def get_files(path):
    onlyfiles = [f for f in os.listdir(path) if os.path.isfile(os.path.join(path, f))]
    if '.DS_Store' in onlyfiles:
        onlyfiles.remove('.DS_Store')
    return onlyfiles  


def stitch_img():
    # create the directory for storing the stitched images
    if not os.path.exists('./res'):
        os.makedirs('./res')
    sunrise_imgs = get_files('./re')
    weather_imgs = get_files('./res')
    min_length = min(len(sunrise_imgs), len(weather_imgs))
    for i in range(0, min_length):
        imgs_str = '"../res/sunrise/' +str(sunrise_imgs[i]) + '"' + ' "../res/weather/' + str(weather_imgs[i]) + '" '
        if not os.path.exists(str(i)):
            os.makedirs(str(i))
        os.system('cp ' + imgs_str + ' ' + str(i))


def concat_img():
    files = get_files("./res")
    os.chdir('res')
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
    files = get_files("./res")
    basewidth = 900
    baseheight = 900
    if not os.path.exists('./resized'):
        os.makedirs('./resized')
    i = 0
    for f in files:
        img = Image.open('./res/' +f)
        img = img.resize((basewidth,baseheight), Image.ANTIALIAS)
        f = str(i)
        i += 1
        img.save('./resized/' + f + '.png') 


if __name__ == "__main__":
    get_img()
    resize_img()