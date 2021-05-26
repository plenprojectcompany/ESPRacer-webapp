# -*- coding: utf-8 -*-
import cv2
import numpy as np
from  PIL import Image
import matplotlib.pyplot as plt

from wsgiref.simple_server import make_server
import json

import os
import pprint
import time
import requests
import urllib.error
import urllib.request
import io
import tempfile

webcamera_url = 'http://192.168.100.17/capture'
red_x = 0
red_y = 0
yellow_x = 0
yellow_y = 0
green_x = 0
green_y = 0
blue_x = 0
blue_y = 0

def imread_webcamera(url):
    # 画像をリクエストする
    res = requests.get(url)
    img = None
    # Tempfileを作成して即読み込む
    with tempfile.NamedTemporaryFile(dir='./') as fp:
        fp.write(res.content)
        fp.file.seek(0)
        img = cv2.imread(fp.name)
    return img


def app(environ, start_response):
    status = '200 OK'
    headers = [
        ('Content-type', 'application/json; charset=utf-8'),
        ('Access-Control-Allow-Origin', '*'),
    ]
    start_response(status, headers)

    # 画像の取得
    im = imread_webcamera(webcamera_url)
    hsv_image = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)  # BGR画像 -> HSV画像

    # 赤色抽出
    hsv_min1_red = np.array([0,140,140]) # hueが0付近のため領域が2つ存在する。2つマスクを作成し合成する
    hsv_max1_red = np.array([5,255,255])
    hsv_min2_red = np.array([175,140,140])
    hsv_max2_red = np.array([179,255,255])
    mask_red = cv2.inRange(hsv_image, hsv_min1_red, hsv_max1_red) + cv2.inRange(hsv_image, hsv_min2_red, hsv_max2_red)

    label = cv2.connectedComponentsWithStats(mask_red)
    if(label[0] == 1):
        red_x = 0
        red_y = 0
    else:
        n = label[0] - 1
        data = np.delete(label[2], 0, 0)
        center = np.delete(label[3], 0, 0)
        max_index = np.argmax(data[:,4])
        red_x = round(center[max_index][0])
        red_y = round(center[max_index][1])

    # 黄色抽出
    hsv_min_yellow = np.array([20,50,140]) # 色相・彩度・明度
    hsv_max_yellow = np.array([40,170,255])
    mask_yellow = cv2.inRange(hsv_image, hsv_min_yellow, hsv_max_yellow)

    label = cv2.connectedComponentsWithStats(mask_yellow)
    if(label[0] == 1):
        yellow_x = 0
        yellow_y = 0
    else:
        n = label[0] - 1
        data = np.delete(label[2], 0, 0)
        center = np.delete(label[3], 0, 0)
        max_index = np.argmax(data[:,4])
        yellow_x = round(center[max_index][0])
        yellow_y = round(center[max_index][1])

    # 緑色抽出
    hsv_min_green = np.array([40,120,140]) # 色相・彩度・明度
    hsv_max_green = np.array([90,240,255])
    mask_green = cv2.inRange(hsv_image, hsv_min_green, hsv_max_green)

    label = cv2.connectedComponentsWithStats(mask_green)
    if(label[0] == 1):
        green_x = 0
        green_y = 0
    else:
        n = label[0] - 1
        data = np.delete(label[2], 0, 0)
        center = np.delete(label[3], 0, 0)
        max_index = np.argmax(data[:,4])
        green_x = round(center[max_index][0])
        green_y = round(center[max_index][1])

    # 青色抽出
    hsv_min_blue = np.array([100,140,140]) # 色相・彩度・明度
    hsv_max_blue = np.array([120,255,255])
    mask_blue = cv2.inRange(hsv_image, hsv_min_blue, hsv_max_blue)

    label = cv2.connectedComponentsWithStats(mask_blue)
    if(label[0] == 1):
        blue_x = 0
        blue_y = 0
    else:
        n = label[0] - 1
        data = np.delete(label[2], 0, 0)
        center = np.delete(label[3], 0, 0)
        max_index = np.argmax(data[:,4])
        blue_x = round(center[max_index][0])
        blue_y = round(center[max_index][1])

    # return [json.dumps({'x' : return_x_value_of_red()}).encode("utf-8")]
    return  [json.dumps({
        'color_list' : ['red', 'yellow', 'green', 'blue'],
        'x_axis' : [red_x, yellow_x, green_x , blue_x],
        'y_axis' : [red_y, yellow_y, green_y , blue_y]
    }).encode("utf-8")]


with make_server('', 8080, app) as httpd:
  print("Serving on port 8080...")
  httpd.serve_forever()
