import os
from os import listdir
from os.path import isfile, join


def get_img():
    os.system("aws s3 sync s3://weather-cs3505-bucket ../res/weather")
    os.system("aws s3 sync s3://sunrise-bucket ../res/sunrise")
    

def get_files(path):
    onlyfiles = [f for f in listdir(path) if isfile(join(path, f))]
    return onlyfiles  


def stitch_img(pathes):
    all_files = []
    for path in pathes:
        files = []
        files.append(get_files(path))
        all_files.append(files)
    
    
    

    
if __name__ == "__main__":
    get_img()
    filenames = get_files('../res/weather')
    print(len(filenames))