#!/usr/bin/python3
import simpy
import random
import numpy as np
import scipy
import scipy.stats
import paho.mqtt.client as mqtt
#from sqlite3 import Error  #nada
from scipy import spatial
from time import localtime, strftime, sleep
import json

import configparser
import mysql.connector

import constants as c
import functions as f


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

                
weight = []  #Weights
size = []  #heights
pos = [] #position of the bins
bin_full = []
#count=1  #hours
day=1    #day
week=0   #week
bin_behavior=[0, 0, 0, 0, 0, 0, 0, 0.02, 0.02, 0.1, 0.1, 0.02, 0.2, 0.2, 0.02, 0.02, 0.1, 0.1, 0.02, 0.02, 0.02, 0.02, 0, 0]
j=0 #??
c.CLOSE_HOUR = 21 #useless??
open_hour = 7 #useless??
generated_dist = True
sizex1 = 9.226661
sizey1 = 45.476939
sizex2 = 9.23419
sizey2 = 45.479332
s = (c.BINS_NUMBER,24)
add_waste_size = np.zeros(s)
total_add_waste_size = np.zeros(s)
add_waste_weight = np.zeros(s)
total_add_waste_weight = np.zeros(s)



#per file ini
config = configparser.ConfigParser()
config.read('secret.ini')

broker = config['DEFAULT']['HOST']

user = config['DATABASE']['USER']
password = config['DATABASE']['PASSWORD']
database = config['DATABASE']['DB_NAME']
#####


#mqtt create
client = mqtt.Client("bin_simulator") #create new instances
client.on_connect = on_connect #bind call back function
client.on_disconnect = on_disconnect
client.on_message = on_message
print("Connecting to brokero", broker)

client.connect(broker) #connect to broker
client.loop_start() #start loop
###


if __name__ == "__main__":

    i = 0
    for i in range(c.BINS_NUMBER):
        if len(weight)!=c.BINS_NUMBER or len(size)!=c.BINS_NUMBER:
            print(i)
            weight.append(i)
            size.append(i)
            usage = f.usability(c.BINS_NUMBER)
            [a,b] = f.position(sizex1, sizex2, sizey1, sizey2)
            pos.append([a,b])
            bin_full.append(False)
        
        if len(weight)==c.BINS_NUMBER or len(size)==c.BINS_NUMBER and generated_dist==True:
            [dist_matrix_size, dist_matrix_weight]=f.dist_day(c.BINS_NUMBER, bin_behavior, day, size, weight, usage, i)
            generated_dist=False


    print("weights: ", weight)  
    print("sizes: ", size)
    print("usages: ", usage)
    print("pos:", pos)
    print("bin_full: ", bin_full)
    print("dist_matrix...", [dist_matrix_size, dist_matrix_weight])

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
            previous_result_size=dms[0:c.BINS_NUMBER, 23]
            previous_result_weight=dmw[0:c.BINS_NUMBER, 23]
            [dist_matrix_size, dist_matrix_weight]=f.dist_day(c.BINS_NUMBER, bin_behavior, day, size, weight, usage, i, previous_result_size, previous_result_weight)
            #j=0
            total_add_waste_size[total_add_waste_size>0]=0.0
            total_add_waste_weight[total_add_waste_weight>0]=0.0
            add_waste_size[add_waste_size>0]=0.0
            add_waste_weight[add_waste_weight>0]=0.0
            
            
        if day_of_week == "Sun":
           week=week+1

        for i in range (c.BINS_NUMBER):
            
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

                f.waste_reposition(c.BINS_NUMBER, size[ind], pos[i], mult_matrix_size[i,current_hour], mult_matrix_weight[i,current_hour], ind, current_hour, counter)
            
                total_add_waste_size=np.cumsum(add_waste_size,axis=1)
                total_add_waste_weight=np.cumsum(add_waste_weight,axis=1)
             

            bin_name= "12%s" % i
            bin_level= round(size[i])
            bin_weight= round(weight[i] * 17)
            bin_position_lon=pos[i][0]
            bin_position_lat=pos[i][1]
                
            json_bin = {
                        "height": bin_level,
                        "total_height": c.TOTAL_HEIGHT,
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


            '''
            record=[week, day_of_week, current_hour, bin_name, bin_level, bin_weight]
             

            conn = mysql.connector.connect(user=user,
                                           password=password,
                                           host=broker,
                                           database=database)

            c=conn.cursor() #cursor to create, modify and query tables in the db
            weight_record = [bin_name, bin_weight]
            height_record = [bin_name, bin_level, c.TOTAL_HEIGHT]
            #c.execute("INSERT INTO bin_weight (bin_id, weight) VALUES (%s,%s)", weight_record) 
            #c.execute("INSERT INTO bin_height (bin_id, height, c.TOTAL_HEIGHT) VALUES (%s,%s,%s)", height_record) 

            conn.commit()


            
            if size[i]>=90:
                client.publish("house/%s/alert" % bin_name, "Warning, the bin %s is full" %i)
                print("Warning, the bin %s is full" % i)
            else:
                client.publish("house/%s/alert" % bin_name, " ")
        '''


        #conn.close()                 
        #j=j+1
        
        
        #client.publish("house/day" , day_of_week)
        #client.publish("house/hour" , count)
        #client.publish("house/week" , week)

        f.recolection(day_of_week, current_hour, c.RECOLECTION_DAYS, c.RECOLECTION_HOUR,size, weight, total_add_waste_size, total_add_waste_weight, current_hour)
       
        print ("---------")
        print(current_hour)        
        
        sleep(c.DELAY)
