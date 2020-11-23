import pymysql
import datetime
import json


def read_db_config(dest="db_config.json"):
    f = open(dest)
    return json.load(f)


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


def insert_data(data_map):
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


