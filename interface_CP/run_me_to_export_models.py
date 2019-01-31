
__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__note_______ = 'This python scripts can be used to run the algorithm.\
                 It can be used to solve R data set from SND problems'


import logging as logger
import subprocess

logger.basicConfig(level=logger.INFO,
                   format='[%(asctime)s]: [%(levelname)s]: %(message)s')


def main():
    # logger.info((dir(subprocess)))
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
        arg_base = 'gcc -DGLIBCXX_DEBUG -g -static-libstdc++ -std=c++11 -DNDEBUG -DIL_STD -c    main.cpp'
        subprocess.call(arg_base, shell=True)
        arg_base = 'gcc -static-libstdc++ -DGLIBCXX_DEBUG -g -L/home/rahrag/builds/lib -o main main.o ../docopt.o -fopenmp -lboost_system -lboost_thread-mt -lilocplex -lconcert -lcplex -lm -lpthread'
        subprocess.call(arg_base, shell=True)
    except Exception as e:
        logger.error('Failed to compile the exporter...')
        logger.error(e)

    logger.info('Running the export command:  ./main')
    logger.info('Starting the export...')
    try:
        subprocess.call('./main', shell=True)
    except Exception as e:
        logger.error('Failed to export models')
        logger.error(e)

    logger.info('Done With Exporting.')


if __name__ == '__main__':
    main()
else:
    logger.error('This is a standalone module!')
