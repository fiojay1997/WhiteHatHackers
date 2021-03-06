import requests as rq
import json
import time
import os
import re
from pathlib import Path
import datetime
import pymysql
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import boto
import urllib
from PIL import Image
import sys
import boto3
import io


# make header for the request
# user should provide browser information and accepting content type
# arguments and corresponding inputs should be provided
#
# browser_info      : string (firefox/chrome/etc)
# content_type      : string (application/json/text/html/etc)
# arguments         : string (character_set/http header infos)
# inputs            : string (corresponding info for arguments)
# TODO: add more info here
def make_header(browser_info, content_type, arguments, inputs):
    header = {}
    if browser_info is not None:
        header['User-Agent'] = browser_info.strip().lower()
    else:
        header['User-Agent'] = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:23.0) Gecko/20100101 Firefox/23.0'
    header['Accept'] = 'application/json' if content_type is None else content_type
    if len(arguments) == len(inputs) and arguments is not None:
        for a, i in zip(arguments, inputs):
            header[a] = i
        return header
    else:
        return {"Error": "Wrong format"}


# make parameters for the request
# parameters should be compatible with the given api
# for GET POST requests, parameters are not yet supported
#
# data_mapping      : dictionary (dictionary contains key value stores for parameters)
def make_params(data_mapping):
    params = {}
    for i, j in data_mapping.items():
        params[i] = j
    return params


# make requests, if there are any input, make request with header
# if there are any parameter provied, the parameters should be included
# TODO: backlogging for this method
#
# url               : string (given api)
# params            : dictionary (given parameters, could be none)
def request(url, params=None):
    if params is None:
        try:
            rep = rq.get(url)
        except:
            return {"Error": "Request failed without parameters"}
    else:
        try:
            rep = rq.get(url, headers=params)
        except:
            return {"Error": "Request failed with parameters"}
    if rep.status_code == 200:
        return rep.json()
    else:
        error = {"Error": "Request failed"}
        return error


# filter all the information that are not needed
# TODO: this should not be used yet, incompleted
# TODO: backlogging for this method
#
# data              : dictionary (store all the information pairs)
# filter_words      : array of string (all the keywords that should be filtered out)
def filter_info(data, filter_words):
    new_dic = dict(data)
    for key, value in data.iteritems():
        for f in filter_words:
            if key == f:
                del new_dic[key]
    return new_dic


# select data user need
def select_data(data, selected):
    new_r = {}
    for key, value in data.items():
        if key not in selected:
            new_r[key] = value
    return new_r


# write data to local json file
# if no destination file name given, just assume it's data.json
# TODO: this function should be be generic, not only support json
# TODO: set up buffer for saving
#
# data              : dictionary (contains all the data that's wating for saving)
# dest_file_name    : string (location for saving json, default as "data")
def save_json(data, dest_file_name="data"):
    path = "data"
    time_str = time.strftime("%Y%m%d-%H%M%S")
    dest = path + "/" + dest_file_name + "_" + time_str + ".json"
    Path(path).mkdir(parents=True, exist_ok=True)

    if not os.path.exists(dest):
        try:
            with open(dest, "w", encoding="utf-8") as d:
                json.dump(data, d, ensure_ascii=False, indent=4)
            return {"Success": "Save succeed"}
        except FileNotFoundError:
            return {"Error": "File not found"}


# extract url from given json
# TODO: check if there is any unsolvable urls
# TODO: check for resource types, save according to source types
# TODO: set up backlogging for this function
#
# data              : dictionary (this should be a json string passed in)
def extract_url(data):
    urls = []
    url_regex = r"(?i)\b((?:https?://|www\d{0,3}[.]|[a-z0-9.\-]+[.][a-z]{2,4}/)(?:[^\s()<>]+|\(([^\s()<>]+|(\([^\s(" \
                r")<>]+\)))*\))+(?:\(([^\s()<>]+|(\([^\s()<>]+\)))*\)|[^\s`!()\[\]{};:'\".,<>?«»“”‘’])) "
    regex = re.compile(url_regex, re.IGNORECASE)
    for key, value in data.items():
        if isinstance(value, str):
            if key == "url" or regex.match(value):
                urls.append(value)
    return urls


# download resources from the given address to the destination
# TODO: set up backlogging for this function
# TODO: check for resource types, check if it could downloaded
#
# address           : string (url for the resource)
# dest_file_name    : string (destination for saving all the resources)
def download_resources(address, dest_file_name=None):
    if dest_file_name is None:
        dest_file_name = "resource"
    suffix = ""
    # check the type of the resource
    # address_split = address.split('.')
    # if len(address_split) >= 2:
    #     suffix = address_split[-1]

    # set up the file name
    path = "resources"
    time_str = time.strftime("%Y%m%d-%H%M%S")
    dest = path + "/" + dest_file_name + "_" + time_str + "." + "jpeg"

    Path(path).mkdir(parents=True, exist_ok=True)

    # download
    with open(dest, "wb") as f:
        response = rq.get(address, stream=True)
        if not response.ok:
            return {"Error": "Request resources failed"}
        else:
            for block in response.iter_content(1024):
                if not block:
                    break
                f.write(block)
    return {"Success": "Download succeeded"}


# read json config from the directory:
# if there is any requirements, they have to be in the config file
# TODO: set up back tracking for the file
# TODO: decompose the structure dict(dict(...))
#
# dest              : string (the destination file for reading config file)
# required          : array of string (array contains all the filter words)
def read_config(dest, required=None):
    if required is None:
        required = []
    f = open(dest)
    data = json.load(f)
    file_config = {}
    for r in required:
        if r not in data:
            return {"Error": "Wrong format of the configuration file"}
        file_config[r] = data[r]
    for d in data:
        if d not in required:
            file_config[d] = data[d]
    return file_config


# read database config
# db_config.json should always be placed in the same directory
def read_db_config(dest="db_config.json"):
    f = open(dest)
    return json.load(f)


# connect to mysql database
def db_con(config):
    if 'host' not in config \
            or 'user' not in config \
            or 'password' not in config\
            or 'db' not in config:
        raise ValueError

    host = config['host']
    user = config['user']
    password = config['password']
    db = config['db']
    port = None
    if 'port' in config:
        port = config['port']

    if port is not None:
        try:
            connection = pymysql.connect(host=host, user=user, password=password, db=db, port=port)
            return connection
        except AssertionError:
            return
    else:
        try:
            connection = pymysql.connect(host=host, user=user, password=password, db=db, port=3306)
            return connection
        except AssertionError:
            return


# insert the data to database
def insert_data(data_map):
    print(data_map)
    config = read_db_config()
    table = config["table"]
    connection = db_con(config)

    try:
        with connection.cursor() as cursor:
            if 'date' not in data_map:
                data_map['date'] = datetime.datetime.now()
            keys = list(data_map.keys())
            values = list(data_map.values())
            key_str = ''
            value_str = ''
            for k in range(len(keys) - 1):
                key_str += str(keys[k]) + ", "
            key_str += str(keys[-1])
            for v in range(len(values) - 1):
                value_str += "'" + str(values[v]) + "'" + ", "
            value_str += "'" + str(values[-1]) + "'"

            sql = "INSERT INTO " + table + " (" + key_str + ")" + " VALUES " + "(" + value_str + ")"
            print(sql)
            cursor.execute(sql)
        connection.commit()
    finally:
        connection.close()
   

# save the data as text file
# the text file should always be in the same format 
# this text file should have a header so it can be converted to csv
def save_text_file(data_map):
    header_str = ""
    info = ""
    for key, value in data_map.items():
        header_str += key
        header_str += ","
        info += str(value) + ","
    info = info[:-1]
    header_str = header_str[:-1]
    with open("/tmp/data.txt", "w") as f:
        f.write(header_str + "\n")
        f.write(info)
    

# convert a txt file to csv file
# this is for pandas to read
def convert_text_to_csv():
    data = pd.read_csv("/tmp/data.txt")
    data.to_csv("/tmp/data.csv", index = None)


# generate image from the given api and its data fields
def generate_img(data1, data2, data3):
    df = pd.read_csv("/tmp/data.csv")
    names = ["Salt Lake City"]
    x = np.arange(len(names))

    w = 0.3
    if data1 is not None and data2 is not None and data3 is not None:
        plt.bar(x - w, df[data1].values, width = w, label = data1)
        plt.bar(x, df[data2].values, width = w, label = data2)
        plt.bar(x + w, df[data3].values, width = w, label = data3)
    else:
        plt.bar(x, df[data1].values, width = w, label = data1)

    plt.xticks(x, names)
    plt.ylim([0, 3])
    plt.tight_layout()
    date = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    plt.xlabel(date)

    plt.legend(loc = 'upper center', bbox_to_anchor = (0.5, -0.2), fancybox = True, ncol = 5)
    plt.savefig("/tmp/data.png", bbox_inches = "tight")


# resize image and store to S3
# the bucket name should be decided by the container name
# the access key and secrete key only works on my computer, so change it
# container_name    : string (which api are we calling)
def store_img_to_s3(container_name):
    client = boto3.client('s3', region_name='us-east-1')
    dest = container_name + str(datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")) + '.png'
    client.upload_file('/tmp/data.png', container_name + '-bucket', dest)
    return {'result': 'success'}


# assembly all the functions
# build the header -> build the parameters -> make the request -> check for
# resources -> if there are any, download them, save them locally -> save all
# the data
# TODO: set up backlogging for this method
# TODO: this should be more generic
# TODO: this should really be maintained by a class
# TODO: a proxy should be added in order to prevent site ban
#
# keywords          : array of string (keywords the user wants from the api)
# file_dest         : string (location to save to files)
# resources_dest    : string (location to store the resources)
# browser_info      : string (the browser used for simulating the action)
# config_dest       : string (location stores the config file)
def proceed(keywords=None, file_dest="data",
            resources_dest="resource", browser_info=None, config_dest="soil_config.json"):
    if keywords is None:
        keywords = []
    config = read_config(config_dest)
    params = make_params(config)
    assert params["api"] is not None
    api = params["api"]
    del params["api"]

    urls = extract_url(params)
    download_result = None
    if len(urls) != 0:
        for url in urls:
            download_result = download_resources(url)
            
    # TODO: for backlog
    # TODO: use response for logging
    # TODO: check if response throws error
    # TODO: check if the result is legal
    # TODO: this part should be changed for logging
    if download_result is not None:
        if "Success" in download_result:
            print("Download succeeded")
        else:
            print("Download failed")
    # TODO: set up backlogging for data downloading
    # TODO: downloading should not only support json
    temp = {}
    for k, v in params.items():
        if k != "selected":
            temp[k] = v
    response = request(api, temp)

    # TODO: this needs to be fixed
    if "selected" in config:
        selected = [config["selected"]]
        return_info = {}
        # get what we want here
        for key in response.keys():
            if key in selected[0]:
                if len(selected[0]) > 1:
                    for k in selected[0]:
                        return_info[k] = response[k]
                else:
                    return_info['scantime'] = response['data'][0]['scantime']
                    return_info['soil_temperature'] = response['data'][0]['soil_temperature']
                    return_info['soil_moisture'] = response['data'][0]['soil_moisture']

        # add db operations
        nested_info = {}
        for key in return_info.keys():
            if type(return_info[key]) == dict:
                for k, v in return_info[key].items():
                    nested_info[k] = v
            else:
                nested_info[key] = return_info[key]
        # insert to database
        # insert_data(nested_info)
        # save the data as txt file
        save_text_file(nested_info) 
        # convert it to csv
        convert_text_to_csv()
        
        if (config_dest == "sunrise_config.json"):
            generate_img("sunrise", "sunset", "day_length")
            store_img_to_s3("sunrise-cs3505")

        if (config_dest == "weather_config.json"):
            generate_img("temp", "humidity", None)
            store_img_to_s3("weather-cs3505")

        if (config_dest == "air_config.json"):
            generate_img("AQI", "PM10", "PM25")
            store_img_to_s3("air-quality-cs3505")

        if (config_dest == "fire_config.json"):
            generate_img("grass_pollen", "tree_pollen", "weed_pollen")
            store_img_to_s3("fire-cs3505")

        if (config_dest == "soil_config.json"):
            generate_img("scantime", "soil_temperature", "soil_moisture")
            store_img_to_s3("soil-cs3505")

        if (config_dest == "water_config.json"):
            generate_img("water_vapor", None, None)
            store_img_to_s3("water-cs3505")
 
    else:
        save_json(response)


# lambda service handler
def sunrise_api_collector(event, context):
    proceed()
    return {"result": "success"}
