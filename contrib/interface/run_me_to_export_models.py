
__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__note_______ = 'This python scripts can be used to run the algorithm.\
                 It can be used to solve R data set from SND problems'


import subprocess

num_processors = 2

subprocess.call('rm' + " " + 'main', shell=True)
subprocess.call('rm' + " " + 'main.o', shell=True)
subprocess.call('rm' + " " + 'models/*.lp', shell=True)
subprocess.call('pwd', shell=True)

print 'Compiling the export program with c++11...'
# gcc -o model_refiner  model_refiner.c -lilocplex -lconcert -lcplex -lm -lpthread
# !!!!! Complie following line if you dont have docopt.o in the directory
# arg_base = 'g++ -std=c++11 -lstdc++ -DNDEBUG -DIL_STD -c externals/docopt/docopt.cpp   -g'
# subprocess.call(arg_base, shell=True)
arg_base = 'g++ -std=c++11 -lstdc++  -DIL_STD -c  main.cpp -g'
subprocess.call(arg_base, shell=True)
arg_base = 'g++ -g -o main main.o ../externals/docopt/docopt.o -fopenmp -lboost_system -lboost_thread-mt -lilocplex -lconcert -lcplex -lm -lpthread'
subprocess.call(arg_base, shell=True)
print '  Done!'

print 'Setting the instances to be solved...'
INSTANCE_LIST = ['04']  # [04,05,06,07,08,09,10,11]
COST_CAP_RATIOS = [9]  # [3,5,7,9]
CORRELATION = [0.2]  # [0,0.2,0.8]
SCENARIOS = [64]  # [16,32,64]

network_info = {}
network_info['04'] = [10, 60, 10]
network_info['05'] = [10, 60, 25]
network_info['06'] = [10, 60, 50]
network_info['07'] = [10, 82, 10]
network_info['08'] = [10, 83, 25]
network_info['09'] = [10, 83, 50]
network_info['10'] = [20, 120, 40]
network_info['11'] = [20, 120, 100]
print '  Done!'

for instance in INSTANCE_LIST:

    num_nodes = network_info[instance][0]
    num_arcs = network_info[instance][1]
    num_od = network_info[instance][2]

    for capacity_ratio in COST_CAP_RATIOS:

        print 'Loading the data files...'
        instance_file_name = 'r' + instance + \
            '.' + str(capacity_ratio) + '.dow'
        print '  Network file ' + instance_file_name + ' is picked!'
        instance_file_path = './data/' + instance_file_name

        for correlation in CORRELATION:
            for num_scenario in SCENARIOS:

                scenario_file_name = 'r' + \
                    instance + '-' + str(correlation) + '-' + str(num_scenario)
                print '  Scenario file ' + scenario_file_name + ' is picked!'
                scenario_file_path = './data/scenarios/' + scenario_file_name

                print 'Running the optimization command...'
                print '  ./main --num_nodes=' + str(num_nodes) + ' --num_od=' + str(num_od) + ' --num_arcs=' + str(
                    num_arcs) + ' --num_scenario=' \
                    + str(num_scenario) + ' --file_path=' + instance_file_path + \
                    ' --scenario_path=' + scenario_file_path
                print '\nStarting the optimization...'

                # subprocess.call('./main ' + '--help',
                #                 shell=True)  # to show help
                # subprocess.call('./main --num_nodes=' +
                #                 str(num_nodes), shell=True)
                subprocess.call('./main --num_nodes=' +
                                str(num_nodes) + ' --num_od=' + str(num_od) +
                                ' --num_arcs=' +
                                str(num_arcs) + ' --num_scenario='
                                + str(num_scenario) + ' --file_path=' +
                                instance_file_path + ' --scenario_path=' +
                                scenario_file_path, shell=True)


print '\nDone With Exporting...'
