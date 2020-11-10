import requests as rq
import json
import time
import os
import re


# make header for the request
def make_header(browser_info, arguments, inputs):
    header = {}
    if browser_info is not None:
        header['User-Agent'] = browser_info.strip().lower()
    else:
        # use firefox as default
        header['User-Agent'] = 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:23.0) Gecko/20100101 Firefox/23.0'
    # arguments should have the same number as inputs
    if len(arguments) == len(inputs):
        for a, i in zip(arguments, inputs):
            header[a] = i
        return header
    else:
        return {"Error": "404"}


# make requests, if there are any input, make request with header
def request(url, header):
    if header is None:
        response = rq.get(url)
    else:
        response = rq.get(url, header)
    if response.status_code == 200:
        return response.json()
    else:
        error_json = json.dumps({"Error": "500"})
        return error_json


# filter all the information that are not needed
def filter_info(data, filter_words):
    new_dic = dict(data)
    for key, value in data.iteritems():
        for f in filter_words:
            if key == f:
                del new_dic[key]
    return new_dic


# write data to local json file
def save_json(data, dest_file_name):
    time_str = time.strftime("%Y%m%d-%H%M%S")
    dest = dest_file_name + "_" + time_str + ".json"
    if not os.path.exists(dest):
        try:
            with open(dest, "w", encoding="utf-8") as d:
                json.dump(data, d, ensure_ascii=False, indent=4)
            return json.dumps({"Success": "200"})
        except FileNotFoundError:
            return json.dumps({"Error": "404"})


# extract url from given json
def extract_url(data):
    urls = []
    r = dict(data)
    regex = re.compile(r'^(?:http|ftp)s?://'
                       r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|'
                       r'localhost|'
                       r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})'
                       r'(?::\d+)?'
                       r'(?:/?|[/?]\S+)$',
                       re.IGNORECASE)
    for key, value in data.iteritems():
        if key == "url" or regex.match(regex, value):
            urls.append(value)
            del r[key]
    return r


# download resources from the given address to the destination
def download_resources(address, dest_file_name):
    suffix = ""
    # check the type of the resource
    address_split = address.split('.')
    if len(address_split) >= 2:
        suffix = address_split[-1]

    # set up the file name
    time_str = time.strftime("%Y%m%d-%H%M%S")
    dest = dest_file_name + "_" + time_str + "." + suffix

    # download
    with open(dest, "wb") as f:
        response = rq.get(address, stream=True)
        if not response.ok:
            return json.dumps({"Error": "404"})
        else:
            for block in response.iter_content(1024):
                if not block:
                    break
                f.write(block)
    return json.dumps({"Success": "200"})


# assembly all the functions
def proceed(api, browser_info, keywords, file_dest, resources_dest):
    pass


if __name__ == '__main__':
    # TODO: take in command line arguments
    # TODO: handle all the errors
    proceed()






