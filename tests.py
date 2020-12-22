################################################################################
#                                IME-USP (2017)                                #
#                    MAC0422 - Sistemas Operacionais - EP2                     #
#                                                                              #
#                                    Testes                                    #
#                                                                              #
#                              Raphael R. Gusmao                               #
################################################################################

import math
import subprocess
from xlwt import Workbook

d_values = [250]
n_values = [20, 40, 60]; n_len = len(n_values)
v_values = [20, 40, 60]; v_len = len(v_values)
measurements = 30

################################################################################
def output_to_excel (mean_time_results, mean_memory_results, interval_time_results, interval_memory_results):
    wb = Workbook()
    time_sheet = wb.add_sheet('Time')
    memory_sheet = wb.add_sheet('Memory')
    for i in range(n_len):
        time_sheet.write(i+1, 0, n_values[i])
        memory_sheet.write(i+1, 0, n_values[i])
    for j in range(v_len):
        time_sheet.write(0, j+1, v_values[j])
        memory_sheet.write(0, j+1, v_values[j])
    for i in range(n_len):
        for j in range(v_len):
            time_sheet.write(i+1, j+1, mean_time_results[i][j])
            memory_sheet.write(i+1, j+1, mean_memory_results[i][j])
            time_sheet.write(i+1, j+v_len+2, interval_time_results[i][j])
            memory_sheet.write(i+1, j+v_len+2, interval_memory_results[i][j])
    wb.save("tests.xls")

################################################################################
def get_statistics (times, memories):
    mean_time = 0; interval_time = 0
    mean_memory = 0; interval_memory = 0
    for i in range(measurements):
        mean_time += times[i]
        mean_memory += memories[i]
    mean_time /= measurements
    mean_memory /= measurements
    for i in range(measurements):
        interval_time += (times[i]-mean_time)*(times[i]-mean_time)
        interval_memory += (memories[i]-mean_memory)*(memories[i]-mean_memory)
    interval_time = 1.96*math.sqrt(interval_time/(measurements-1))/math.sqrt(measurements)
    interval_memory = 1.96*math.sqrt(interval_memory/(measurements-1))/math.sqrt(measurements)
    return round(mean_time, 3), round(mean_memory), interval_time, interval_memory

################################################################################
def execute (d, n, v):
    command = '/usr/bin/time -v ./ep2 ' + str(d) + ' ' + str(n) + ' ' + str(v)
    log = subprocess.Popen(command.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()[1].decode('ascii').split("\n")
    time = log[4].split()[-1].split(":")
    kbytes = int(log[9].split()[-1])
    seconds = 0
    if (len(time) == 2):# m:ss
        seconds = float(time[0])*60 + float(time[1])
    elif (len(time) == 3):# h:mm:ss
        seconds = float(time[0])*60*60 + float(time[1])*60 + float(time[2])
    return seconds, kbytes

################################################################################
def main ():
    mean_time_results = [[0 for j in range(v_len)] for i in range(n_len)]
    mean_memory_results = [[0 for j in range(v_len)] for i in range(n_len)]
    interval_time_results = [[0 for j in range(v_len)] for i in range(n_len)]
    interval_memory_results = [[0 for j in range(v_len)] for i in range(n_len)]
    for d in d_values:
        for i in range(n_len):
            for j in range(v_len):
                times = []
                memories = []
                print("$ ./ep2 " + str(d) + " " + str(n_values[i]) + " " + str(v_values[j]))
                for k in range(measurements):
                    seconds, kbytes = execute(d, n_values[i], v_values[j])
                    times.append(seconds)
                    memories.append(kbytes)
                    print("\tMedicao " + str(k+1) + ": (Time = " + str(seconds) + " s | Memory: " + str(kbytes) + " KB)")
                mean_time, mean_memory, interval_time, interval_memory = get_statistics(times, memories)
                mean_time_results[i][j] = mean_time
                mean_memory_results[i][j] = mean_memory
                interval_time_results[i][j] = interval_time
                interval_memory_results[i][j] = interval_memory
                print("\t\tMean Time: " + str(mean_time) + " | Confidence Interval Time: " + str(interval_time))
                print("\t\tMean Memory: " + str(mean_memory) + " | Confidence Interval Memory: " + str(interval_memory))
    output_to_excel(mean_time_results, mean_memory_results, interval_time_results, interval_memory_results)

main()
################################################################################
