
__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__note_______ = 'This python scripts can be used to run the algorithm.\
                 It can be used to solve R data set from SND problems'


import logging as logger
import os
import subprocess

logger.basicConfig(level=logger.INFO,
                   format='[%(asctime)s]: [%(levelname)s]: %(message)s')


def main():
    model_files_path = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD/models"
    pwd = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD"
    # logger.info((dir(subprocess)))
    logger.info('Running a {0} machine.'.format(subprocess.os.uname()[0]))
    logger.info('Working directory is {0}'.format(
        subprocess.os.getcwd()))
    try:
        logger.info('Cleaning up the old stuff...')
        subprocess.call('rm' + " " + 'main', shell=True)
        subprocess.call('rm' + " " + 'main.o', shell=True)
        subprocess.call('rm' + " " + '../models/*', shell=True)
        subprocess.call('rm' + " " + '../opt_model_dir/*', shell=True)
    except Exception as e:
        logger.error('Failed to clean up')
        logger.error(e)

    try:
        logger.info('Compiling the export program with c++11...')
        # gcc -o model_refiner  model_refiner.c -lilocplex -lconcert -lcplex -lm -lpthread
        # !!!!! Complie following line if you dont have docopt.o in the directory
        # arg_base = 'g++ -std=c++11 -lstdc++ -DNDEBUG -DIL_STD -c externals/docopt/docopt.cpp   -g'
        # subprocess.call(arg_base, shell=True)
        arg_base = 'g++ -std=c++11  -DIL_STD -c  main.cpp -g'
        subprocess.call(arg_base, shell=True)
        arg_base = 'g++ -g -o main main.o ../docopt.o -fopenmp -lboost_system -lboost_thread -lilocplex -lconcert -lcplex -lm -lpthread'
        subprocess.call(arg_base, shell=True)
    except Exception as e:
        logger.error('Failed to compile the exporter...')
        logger.error(e)

    logger.info('Setting the instances to be solved...')
    # , '05', '06', '07', '08', '09', '10', '11']  # [04,05,06,07,08,09,10,11]
    INSTANCE_LIST = ['04']
    COST_CAP_RATIOS = [5]  # [3,5,7,9]
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
    logger.info('  Done!')

    for instance in INSTANCE_LIST:

        num_nodes = network_info[instance][0]
        num_arcs = network_info[instance][1]
        num_od = network_info[instance][2]

        for capacity_ratio in COST_CAP_RATIOS:
            logger.info('Loading the data files...')
            instance_file_name = 'r' + instance + \
                '.' + str(capacity_ratio) + '.dow'
            logger.info('  Network file {0} is picked!'.format(
                instance_file_name))
            instance_file_path = './data_files/' + instance_file_name

            for correlation in CORRELATION:
                for num_scenario in SCENARIOS:

                    scenario_file_name = 'r' + \
                        instance + '-' + str(correlation) + \
                        '-' + str(num_scenario)
                    logger.info('  Scenario file {0} is picked!'.format(
                                scenario_file_name))
                    scenario_file_path = './data_files/scenarios/' + scenario_file_name

                    logger.info('Running the export command:  ./main --num_nodes={0}  --num_od={1} --num_arcs={2} --num_scenario={3} --file_path={4} --scenario_path={5}'.format(
                        num_nodes, num_od, num_arcs, num_scenario, instance_file_path, scenario_file_path))
                    logger.info('Starting the export...')
                    try:
                        subprocess.call('./main --num_nodes=' +
                                        str(num_nodes) + ' --num_od=' + str(num_od) +
                                        ' --num_arcs=' +
                                        str(num_arcs) + ' --num_scenario='
                                        + str(num_scenario) + ' --file_path=' +
                                        instance_file_path + ' --scenario_path=' +
                                        scenario_file_path, shell=True)
                        subprocess.call('cp' + " " + model_files_path +
                                        '/*.sav' + " " + pwd + '/opt_model_dir/', shell=True)
                        # os.chdir(pwd)
                        # logger.info('Starting the optimization...')
                        # subprocess.call('./main  --model_dir=' + model_files_path +
                        #                 ' --current_dir=' + pwd, shell=True)
                        # os.chdir(pwd + '/interface_ND')
                    except Exception as e:
                        logger.error('Failed to export models')
                        logger.error(e)

    logger.info('Done With Exporting.')


if __name__ == '__main__':
    main()
else:
    logger.error('This is a standalone module!')
