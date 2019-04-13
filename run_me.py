#!/bin/env python

__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__usage______ = 'python run_me.py'
__note_______ = 'This python scripts can be used to run the algorithm.\
                 It can be used to solve R data set from SND problems'


import logging as logger
import subprocess

# logger.basicConfig(filename='solver.log', level=logger.INFO,
#                    format='%(asctime)s:%(levelname)s:%(message)s')
logger.basicConfig(level=logger.INFO,
                   format='[%(asctime)s]: [%(levelname)s]: %(message)s')

model_files_path = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD/models"
pwd = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD"

subprocess.call('rm' + " " + 'main', shell=True)
subprocess.call('rm' + " " + 'main.o', shell=True)
subprocess.call('rm' + " " + pwd + '/opt_model_dir/*', shell=True)
subprocess.call('cp' + " " + model_files_path +
                '/*.sav' + " " + pwd + '/opt_model_dir/', shell=True)


try:
    logger.info('Compiling the solver with c++11...')
    arg_base = 'gcc -DGLIBCXX_DEBUG -g -c -o  model_refiner.o ' + pwd + \
        '/contrib/pre_processor/model_refiner.c  -lilocplex -lconcert -lcplex -lm -lpthread'
    subprocess.call(arg_base, shell=True)

    # Complie following line if you dont have docopt.o in the directory
    arg_base = 'gcc -DGLIBCXX_DEBUG -g -std=c++11 -DNDEBUG -DIL_STD -c externals/docopt/docopt.cpp  -g'
    subprocess.call(arg_base, shell=True)

    arg_base = 'g++ -DGLIBCXX_DEBUG -g  -std=c++11  -DIL_STD -c  externals/rss/current_rss.cpp  -g'
    subprocess.call(arg_base, shell=True)

    arg_base = 'gcc -DGLIBCXX_DEBUG -g -std=c++11  -DIL_STD -c  main.cpp  -g'
    subprocess.call(arg_base, shell=True)

    logger.info(" Linking...")
    arg_base = 'gcc -DGLIBCXX_DEBUG -g -L/home/rahrag/builds/lib -o main main.o model_refiner.o current_rss.o docopt.o   -ldl -fopenmp -lboost_system -lboost_thread -lilocplex -lconcert -lcplex -lm -lpthread'
    subprocess.call(arg_base, shell=True)
except Exception as e:
    logger.error(e)
    logger.info("Failed to compile...")

try:
    logger.info(
        'Running the optimization command: ./main  --model_dir={0} --current_dir={1}'.format(model_files_path, pwd))
    logger.info('Starting the optimization...')
    subprocess.call('./main  --model_dir=' + model_files_path +
                    ' --current_dir=' + pwd, shell=True)
    # subprocess.call('./main  --model_dir=' +
    #                 model_files_path + ' --setting_dir=' +
    #                 settings_path, shell=True)
    logger.info('Finished the optimization.')
except Exception as e:
    logger.error(e)
    logger.info("Optimization failed!")
