#!/usr/bin/python3

import simpy
import random
import numpy as np
import scipy
import scipy.stats
import paho.mqtt.client as mqtt
from sqlite3 import Error
from scipy import spatial
from time import sleep
from time import localtime, strftime
import json

import configparser
import mysql.connector


#def on_log(client,userdata, level,buf):
 #   print("log: "+buf)

def on_connect(client, userdata, flags, rc):
    if rc==0:
        print("connected OK")
    else:
        print("Bad connection Returned code=",rc)
def on_disconnect(client, userdata, flags, rc=0):
    print("Disconnect result code "+str(rc))

def on_message(client, userdata,msg):
    topic=msg.topic
    m_decode=str(msg.payload.decode("utf-8"))
    print("message received", m_decode)

def distribution(use):

    if use=="very_low":
        size_waste=abs(np.random.normal(loc=10, scale=8))
        weight_waste=abs(np.random.normal(loc=1, scale=1))
    if use=="low":
        size_waste=abs(np.random.normal(loc=30, scale=10))
        weight_waste=abs(np.random.normal(loc=2.5, scale=1.5))
    if use=="mid":
        size_waste=abs(np.random.normal(loc=50, scale=15))
        weight_waste=abs(np.random.normal(loc=5, scale=2))
    if use=="high":
        size_waste=abs(np.random.normal(loc=70, scale=10))
        weight_waste=abs(np.random.normal(loc=7.5, scale=1.5))
    if use=="very_high":
        size_waste=abs(np.random.normal(loc=90, scale=8))
        weight_waste=abs(np.random.normal(loc=10, scale=1))
    return size_waste, weight_waste

def position(sizex1, sizex2, sizey1, sizey2):
    posx=random.uniform(sizex1, sizex2)
    posy=random.uniform(sizey1, sizey2)
    return posx, posy

def usability(number_of_bins,usage):

    usability=["very_low", "low", "mid", "high", "very_high"]

    for i in range(number_of_bins):
        if len(usage)!=number_of_bins:
            u=random.choice(usability)
            if u=="very_low" and usage.count("very_low")<round(0.1*number_of_bins):
                usage.append(u)
            else:
                u="low"
            if u=="low" and usage.count("low")<round(0.2*number_of_bins):
                usage.append(u)
            else:
                u="mid"
            if u=="mid" and usage.count("mid")<round(0.4*number_of_bins):
                usage.append(u)
            else:
                u="high"
            if u=="high" and usage.count("high")<round(0.2*number_of_bins):
                usage.append(u)
            else:
                u="very_high"
            if u=="very_high" and usage.count("very_high")<round(0.1*number_of_bins):
                usage.append(u)
            else:
                u="very_low"

def dist_day(number_of_bins, bin_behavior, day, size, weight, previous_result_size=[0], previous_result_weight=[0]):
    for i in range(number_of_bins):
        [a,b]=distribution(usage[i])
        size[i]=a
        weight[i]=b
        
    if day=="Sat" or day=="Sun":
        bin_behavior=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    mult_matrix_size=np.outer(size, bin_behavior)
    mult_matrix_weight=np.outer(weight, bin_behavior)
    if len(previous_result_size)==number_of_bins:
        mult_matrix_size[:N, 0] = previous_result_size
    if len(previous_result_weight)==number_of_bins:
        mult_matrix_weight[:N, 0] = previous_result_weight
    dist_matrix_size=np.cumsum(mult_matrix_size,axis=1)
    dist_matrix_weight=np.cumsum(mult_matrix_weight,axis=1)
    dist_matrix_size=np.clip(dist_matrix_size, 0, 100)
    dist_matrix_weight=np.clip(dist_matrix_weight, 0, 100)
    return dist_matrix_size, dist_matrix_weight
    
def recolection(day, hour, recolection_day, recolection_hour, size, weight, total_add_waste_size, total_add_waste_weight, count):

    if day in recolection_day and hour == recolection_hour:
        for i in range(len(size)):
            size[i]=0.0
            weight[i]=0.0
            dist_matrix_size[0:N, count:24] = 0
            dist_matrix_weight[0:N, count:24] = 0
            total_add_waste_size[total_add_waste_size>0]=0.0
            add_waste_size[add_waste_size>0]=0.0
            total_add_waste_weight[total_add_waste_weight>0]=0.0
            add_waste_weight[add_waste_weight>0]=0.0
        print ("collected")

    print("count: ")
    print(count)

def waste_reposition(number_of_bins, next_bin_size, current_bin_position, waste_size, waste_weight, index, count, counter):
    if next_bin_size+waste_size<=100:
        add_waste_size[index,count]=waste_size
        add_waste_weight[index,count]=waste_weight
    else:
        dist,ind = spatial.KDTree(pos).query(current_bin_position, number_of_bins)
        ind=ind[counter]
        if size[ind]+waste_size<=100:
            add_waste_size[ind,count]=waste_size
            add_waste_weight[ind,count]=waste_weight
        else:
            counter=counter+1
            if counter<number_of_bins:
                waste_reposition(number_of_bins, next_bin_size, current_bin_position, waste_size, waste_weight, index, count, counter)
            else:
                add_waste_size[ind,count]=0
                add_waste_weight[ind,count]=0


                
            
N=10 #number of bins
TIME = 3600
TOTAL_HEIGHT = 120 #cm of the bin  
weight=[]  #Weights
size=[]  #heights
usage=[] #usage
pos=[] #position of the bins
bin_full=[]
#count=1  #hours
day=1    #day
week=0   #week
recolection_day="Tue", "Thu"
recolection_hour= 18
bin_behavior=[0, 0, 0, 0, 0, 0, 0, 0.02, 0.02, 0.1, 0.1, 0.02, 0.2, 0.2, 0.02, 0.02, 0.1, 0.1, 0.02, 0.02, 0.02, 0.02, 0, 0]
j=0
close_hour=21
open_hour=7
generated_dist=True
sizex1=9.226661
sizey1=45.476939
sizex2=9.23419
sizey2= 45.479332
s=(N,24)
add_waste_size=np.zeros(s)
total_add_waste_size=np.zeros(s)
add_waste_weight=np.zeros(s)
total_add_waste_weight=np.zeros(s)


config = configparser.ConfigParser()
config.read('secret.ini')

broker = config['DEFAULT']['HOST']

user = config['DATABASE']['USER']
password = config['DATABASE']['PASSWORD']
database = config['DATABASE']['DB_NAME']

client = mqtt.Client("python1") #create new instances


client.on_connect=on_connect #bind call back function
client.on_disconnect=on_disconnect
client.on_message=on_message
print("Connecting to broker ",broker)

client.connect(broker) #connect to broker
client.loop_start() #start loop




for i in range(N):
    if len(weight)!=N or len(size)!=N:
        weight.append(i)
        size.append(i)
        usability(N, usage)
        [a,b] = position(sizex1, sizex2, sizey1, sizey2)
        pos.append([a,b])
        bin_full.append(False)
        
    if len(weight)==N or len(size)==N and generated_dist==True:
        [dist_matrix_size, dist_matrix_weight]=dist_day(N, bin_behavior, day, size, weight)
        generated_dist=False
        print("ok")


while True:

    now = strftime("%a; %d; %b; %Y; %H:%M:%S", localtime())
    now = now.split(";")
    print(now)
    day_of_week= now[0]
    current_time = now[4].split(":")
    current_hour = int(current_time[0])


    if current_hour%24==0:
        #day=day+1
        #count=0
        dms=dist_matrix_size+total_add_waste_size
        dmw=dist_matrix_weight+total_add_waste_weight
        previous_result_size=dms[0:N, 23]
        previous_result_weight=dmw[0:N, 23]
        [dist_matrix_size, dist_matrix_weight]=dist_day(N, bin_behavior, day, size, weight, previous_result_size, previous_result_weight)
        j=0
        total_add_waste_size[total_add_waste_size>0]=0.0
        total_add_waste_weight[total_add_waste_weight>0]=0.0
        add_waste_size[add_waste_size>0]=0.0
        add_waste_weight[add_waste_weight>0]=0.0
        
        
    if day_of_week == "Sun":
       week=week+1

   

    for i in range (N):
        
        size[i]=dist_matrix_size.item((i, current_hour))+total_add_waste_size.item((i,current_hour))
        weight[i]=dist_matrix_weight.item((i, current_hour))+total_add_waste_weight.item((i,current_hour))
        

        if size[i]>=100:
            size[i]=100
            bin_full[i]=True
            dist_matrix_weight[i, current_hour:24]=np.repeat(dist_matrix_weight[i, current_hour], 24-current_hour).reshape((1, 24-current_hour))
            current_bin_pos=pos[i]
            dist,ind = spatial.KDTree(pos).query(current_bin_pos,2)
            ind=ind[1]
            mult_matrix_size=np.outer(size, bin_behavior)
            mult_matrix_weight=np.outer(weight, bin_behavior)
            counter=1

            waste_reposition(N, size[ind], pos[i], mult_matrix_size[i,current_hour], mult_matrix_weight[i,current_hour], ind, current_hour, counter)
        
            total_add_waste_size=np.cumsum(add_waste_size,axis=1)
            total_add_waste_weight=np.cumsum(add_waste_weight,axis=1)
         

        bin_name= "12%s" % i
        bin_level= size[i] + random.choice([0,0, 0, 0, 0.5, 0, 1, 2 ,3,1,1,0,0])
        bin_weight= weight[i] * 17  + random.choice([0,0, 0, 0, 0.5, 0, 1, 2 ,3,0,0,1,1,6])
        bin_position_lon=pos[i][0]
        bin_position_lat=pos[i][1]
            
        json_bin = {
                    "height": bin_level,
                    "total_height": TOTAL_HEIGHT,
                    "weight": bin_weight,
                    "username_": str(bin_name)
        }

        print(json_bin)
        client.publish("smartbin/info", json.dumps(json_bin))
        client.publish("house/%s/bin_name" % bin_name, bin_name)
        client.publish("house/%s/fillpercentage" % bin_name, bin_level)
        client.publish("house/%s/weight" % bin_name, bin_weight)
        client.publish("house/%s/position_lon" % bin_name, bin_position_lon)
        client.publish("house/%s/position_lat" % bin_name, bin_position_lat)

        record=[week, day_of_week, current_hour, bin_name, bin_level, bin_weight]
         

        conn = mysql.connector.connect(user=user,
                                       password=password,
                                       host=broker,
                                       database=database)

        c=conn.cursor() #cursor to create, modify and query tables in the db
        weight_record = [bin_name, bin_weight]
        height_record = [bin_name, bin_level, TOTAL_HEIGHT]
        #c.execute("INSERT INTO bin_weight (bin_id, weight) VALUES (%s,%s)", weight_record) 
        #c.execute("INSERT INTO bin_height (bin_id, height, total_height) VALUES (%s,%s,%s)", height_record) 

        conn.commit()


        
        if size[i]>=90:
            client.publish("house/%s/alert" % bin_name, "Warning, the bin %s is full" %i)
            print("Warning, the bin %s is full" % i)
        else:
            client.publish("house/%s/alert" % bin_name, " ")



    conn.close()                 
    j=j+1
    
    
    #client.publish("house/day" , day_of_week)
    #client.publish("house/hour" , count)
    #client.publish("house/week" , week)

    recolection(day_of_week, current_hour, recolection_day, recolection_hour,size, weight, total_add_waste_size, total_add_waste_weight, current_hour)
   
    print ("---------")
    #count=count+1
    
    
    sleep(TIME)