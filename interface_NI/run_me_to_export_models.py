
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
        # subprocess.call('rm' + " " + '../models/*.sav', shell=True)
        subprocess.call('rm' + " " + '../models/*', shell=True)
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
    snipno = [3]  # , 4]  # 4
    budget = [50]  # 30, 40, 50, 60, 70, 80, 90]  # 30 40 50 60  70 80 90
    instanceNo = [3]  # 0, 1, 2, 3, 4]  # 0 1 2 3 4
    for snipnokey in range(len(snipno)):
        for inskey in range(len(budget)):
            for fcrkey in range(len(instanceNo)):
                arcgainname = 'data_files/arcgain' + \
                    str(instanceNo[fcrkey]) + '.txt'
                intd_arcname = 'data_files/intd_arc' + \
                    str(instanceNo[fcrkey]) + '.txt'
                scenname = 'data_files/Scenarios.txt'
                psiname = 'data_files/psi.txt'
                # --------------------------------------------------------------------------------------------
                # print './main ' + arcgainname + ' ' + intd_arcname + ' ' + psiname + ' ' + scenname  \
                #     + ' ' + str(budget[inskey]) + ' ' + \
                #     str(instanceNo[fcrkey]) + ' ' + str(snipno[snipnokey])

                subprocess.call('./main ' + arcgainname + ' ' +
                                intd_arcname + ' ' + psiname + ' ' + scenname
                                + ' ' + str(budget[inskey]) + ' ' + str(instanceNo[fcrkey]) + ' ' + str(snipno[snipnokey]), shell=True)
                subprocess.call('cp' + " " + model_files_path +
                                '/*.sav' + " " + pwd + '/contrib/opt_model_dir/', shell=True)
                # os.chdir(pwd)
                # logger.info('Starting the optimization...')
                # subprocess.call('./main  --model_dir=' + model_files_path +
                #                 ' --current_dir=' + pwd, shell=True)
                # os.chdir(pwd + '/interface_NI')

    logger.info('Done With Exporting.')


if __name__ == '__main__':
    main()
else:
    logger.error('This is a standalone module!')
