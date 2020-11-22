import pymysql
import datetime
import json


def read_db_config(dest):
    config = {}
    f = open(dest)
    config = json.load(f)
    



def db_con(config):
    if 'host' not in config \
            or 'user' not in config \
            or 'password' not in config\
            or 'db' not in config:
        raise ValueError

    host = config['host']
    user = config['user']
    password = config['password']
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


def insert_data():
    connection = db_con()

    try:
        with connection.cursor() as cursor:
            sql = "INSERT INTO `weather` (`city`, `temperature`, `humidity`, `date`) " \
                  "VALUES (%s, %s, %s, %s)"
            cursor.execute(sql, ('SALT LAKE CITY', '100', '100.1', datetime.datetime.now()))
        connection.commit()
    finally:
        connection.close()

