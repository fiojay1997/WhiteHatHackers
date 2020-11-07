import requests as rq
import json
from datetime import date

"""
make up the request header
including keyword, type id, category id

keyword : string 
type_id : int 
cate_id : int

return  : map 
"""
def make_header(keyword, type_id, cate_id):
    keyword = keyword
    search_type = type_id
    category = cate_id
    return {"keyword" : keyword, "typeIds" : search_type, "categoryIds" : category}

"""
make api call
if bad request, error messgage should not be stored

api       : string
head_info : map

return    : map
"""
def make_api_call(api, head_info):
    reponse = rq.get(api, params = head_info)
    print(response)
    return response.json()

"""
filter information 
exclude useless information
include information corresponding to the keywords provided
repack json 

json    : map

return  : map
"""
def filter(json, required_keywords):
    pass     

"""
save json to local files
save files by date
collect data each day

data    : map(json)
dest    : string
"""
def save_file(data, dest):
    dest_file = "data" + "_" + date.today()
    with open(dest_file) as f:
        json.dump(data, f)

"""
assemly all the actions 

api     : string
"""
def proceed(api, header, dest):
    header = make_header(header)
    ret_info = make_api_call()
    filtered_info = filter()
    if (filtered_info is not ""):
        return filtered_info
    return "Error"

proceed()
