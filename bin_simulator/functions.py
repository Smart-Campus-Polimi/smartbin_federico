import constants as c
import random
import numpy as np



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

def usability(number_of_bins):
    usage = []
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

    return usage

def dist_day(number_of_bins, bin_behavior, day, size, weight, usage, i, previous_result_size=[0], previous_result_weight=[0]):
    for i in range(number_of_bins):
        [a,b]=distribution(usage[i])
        size[i]=round(a)
        weight[i]=round(b)
        
    if day=="Sat" or day=="Sun":
        bin_behavior=[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    mult_matrix_size=np.outer(size, bin_behavior)
    mult_matrix_weight=np.outer(weight, bin_behavior)
    if len(previous_result_size)==number_of_bins:
        mult_matrix_size[:c.BINS_NUMBER, 0] = previous_result_size
    if len(previous_result_weight)==number_of_bins:
        mult_matrix_weight[:c.BINS_NUMBER, 0] = previous_result_weight
    dist_matrix_size=np.cumsum(mult_matrix_size,axis=1)
    dist_matrix_weight=np.cumsum(mult_matrix_weight,axis=1)
    dist_matrix_size=np.clip(dist_matrix_size, 0, 100)
    dist_matrix_weight=np.clip(dist_matrix_weight, 0, 100)
    return dist_matrix_size, dist_matrix_weight
    
def recolection(day, hour, RECOLECTION_DAYS, RECOLECTION_HOUR, size, weight, total_add_waste_size, total_add_waste_weight, count):

    if day in RECOLECTION_DAYS and hour == RECOLECTION_HOUR:
        for i in range(len(size)):
            size[i]=0.0
            weight[i]=0.0
            dist_matrix_size[0:c.BINS_NUMBER, count:24] = 0
            dist_matrix_weight[0:c.BINS_NUMBER, count:24] = 0
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
