
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
    # logger.info((dir(subprocess)))
    model_files_path = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD/models"
    pwd = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD"
    logger.info('Running a {0} machine.'.format(subprocess.os.uname()[0]))
    logger.info('Working directory is {0}'.format(
        subprocess.os.getcwd()))
    try:
        logger.info('Cleaning up the old stuff...')
        subprocess.call('rm' + " " + 'main', shell=True)
        subprocess.call('rm' + " " + 'main.o', shell=True)
        subprocess.call('rm' + " " + '../models/*.sav', shell=True)
        subprocess.call('rm' + " " + '../models/*.lp', shell=True)
    except Exception as e:
        logger.error('Failed to clean up')
        logger.error(e)

    try:
        logger.info('Compiling the export program with c++11...')
        # gcc -o model_refiner  model_refiner.c -lilocplex -lconcert -lcplex -lm -lpthread
        # !!!!! Complie following line if you dont have docopt.o in the directory
        # arg_base = 'g++ -std=c++11 -lstdc++ -DNDEBUG -DIL_STD -c externals/docopt/docopt.cpp   -g'
        # subprocess.call(arg_base, shell=True)
        arg_base = 'g++ -std=c++11 -lstdc++  -DIL_STD -c  main.cpp -g'
        subprocess.call(arg_base, shell=True)
        arg_base = 'g++ -g -o main main.o ../externals/docopt/docopt.o -fopenmp -lboost_system -lboost_thread-mt -lilocplex -lconcert -lcplex -lm -lpthread'
        subprocess.call(arg_base, shell=True)
    except Exception as e:
        logger.error('Failed to compile the exporter...')
        logger.error(e)

    logger.info('Setting the instances to be solved...')
    # Problem Info
    #
    ID = [102, 103, 104, 111, 112, 113, 114,
          121, 122, 123, 124, 131, 132, 133, 134]
    scenario = [1]  # 500 1500
    scenarioC = [0]  # 250 500 1500
    scenarioCap = [0]  # 250 500 1500
    for IDkey in range(len(ID)):
        for scenariokey in range(len(scenario)):
            for scenarioCkey in range(len(scenarioC)):
                for scenarioCapkey in range(len(scenarioCap)):
                    insname = 'data_files/cap' + str(ID[IDkey]) + '.dat'
                # --------------------------------------------------------------------------------------------
                    print './main ' + insname + ' ' + str(ID[IDkey]) + ' ' + str(scenario[scenariokey]) + ' ' + str(
                        scenarioC[scenarioCkey]) + ' ' + str(scenarioCap[scenarioCapkey])

                    subprocess.call('./main ' + insname + ' ' + str(ID[IDkey]) + ' ' + str(scenario[scenariokey]) + ' ' + str(
                        scenarioC[scenarioCkey]) + ' ' + str(scenarioCap[scenarioCapkey]), shell=True)
                    subprocess.call('cp' + " " + model_files_path +
                                    '/*.sav' + " " + pwd + '/contrib/lifted_SP_models/', shell=True)
                    os.chdir(pwd)
                    logger.info('Starting the optimization...')
                    subprocess.call('./main  --model_dir=' + model_files_path +
                                    ' --current_dir=' + pwd, shell=True)
                    os.chdir(pwd + '/interface_FL')

    logger.info('Done With Exporting.')


if __name__ == '__main__':
    main()
else:
    logger.error('This is a standalone module!')
