import requests as rq
import json
from datetime import date

# make up the request header
# including keyword, type id, category id
# keyword : string 
# type_id : int 
# cate_id : int
# return  : map 
def make_header(keyword, type_id, cate_id):
    keyword = keyword
    search_type = type_id
    category = cate_id
    return {"keyword" : keyword, "typeIds" : search_type, "categoryIds" : category}

# make api call
# if bad request, error messgage should not be stored
# api       : string
# head_info : map
# return    : map
def make_api_call(api, head_info):
    response = rq.get(api, params = head_info)
    print(response)
    return response.json()

# filter information 
# exclude useless information
# include information corresponding to the keywords provided
# repack json 
# json    : map
# return  : map
def filter(json, required_keywords):
    for key, value in json.item():
        for j in required_keywords:
            if value == j:
                json.remove(key)   
    return json

# save json to local files
# save files by date
# collect data each day
# data    : map(json)
# dest    : string
def save_file(data, dest):
    dest_file = "data" + "_" + date.today()
    with open(dest_file) as f:
        json.dump(data, f)

# extract url info from given json
# json    : map
# return  : array
def extract_url(json):
    urls = []
    regex = re.compile(r'^(?:http|ftp)s?://'
                       r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|'
                       r'localhost|' 
                       r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})'
                       r'(?::\d+)?' 
                       r'(?:/?|[/?]\S+)$',
                        re.IGNORECASE)
    for key, value in json.items():
        if key == "url" or re.match(regex, value):
            urls.append(value)
            json.remove(key)
    return urls


# download pictures from the give url to the destination 
# address  : string
# dest     : string
def download_pic(address, dest):
    with open(dest, "wb") as f:
        response = rq.get(address, stream=True)
        if not response.ok:
            print response
        else:
            for block in response.iter_content(1024):
                if not block:
                    break;
                f.write(block)
        
# assembly all the actions 
# api     : string
def proceed(api, header, dest):
    header = make_header(header)
    ret_info = make_api_call()
    filtered_info = filter()
    
    urls = extract_url(filtered_info)
    if len(urls) is not 0:
        for url in urls:
            download_pic(url, dest)
    
    if filtered_info is not "":
        save_file(filtered_info)
        return "Success"
    return "Error"

if __name__ == "__main__":
    # TODO: set up parameters for input
    # TODO: set up the filter
    proceed()
