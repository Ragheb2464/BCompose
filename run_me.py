#!/bin/env python

__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__usage______ = 'python run_me.py'
__note_______ = 'This py script can be used to compile and run the algorithm.'

import logging as logger
import subprocess

logger.basicConfig(level=logger.INFO,
                   format='[%(asctime)s]: [%(levelname)s]: %(message)s')

# the absolute paths
model_files_path = "/Users/raghebrahmaniani/BCompose/models"
pwd = "/Users/raghebrahmaniani/BCompose"
# cplex
CONCERTDIR = '/Applications/CPLEX_Studio129/concert/include/'
CPLEXDIR = '/Applications/CPLEX_Studio129/cplex/include/'
CPLEXLIB = '/Applications/CPLEX_Studio129/cplex/lib/x86-64_osx/static_pic/'
CONCERTLIB = '/Applications/CPLEX_Studio129/concert/lib/x86-64_osx/static_pic/'
# compiler
GCC = 'g++'
# compiler flags
Sanitation_Flags = 'thread '
Sanitation_Flags = 'address,undefined,signed-integer-overflow,null,alignment,bool,builtin,bounds,float-cast-overflow,float-divide-by-zero,function,integer-divide-by-zero,return,signed-integer-overflow,implicit-conversion,unsigned-integer-overflow -fno-sanitize-recover=null -fsanitize-trap=alignment -fno-omit-frame-pointer -fsanitize-memory-track-origins=2 -Wno-unused-variable '
# DEBUG_FLAG = '-DGLIBCXX_DEBUG -g -O -DIL_STD -Wall -Wextra -pedantic'
DEBUG_FLAG = '-DGLIBCXX_DEBUG -g -DIL_STD -Wall -Wextra -pedantic -fsanitize={0}'.format(
    Sanitation_Flags)
OPT_FLAG = '-DIL_STD -Ofast'
# chose the first two (debug) or the last two (optimized)
FLAG = DEBUG_FLAG
DEBUG_SYMBOLS = '-g'
# DEBUG_SYMBOLS = ' '
# FLAG = OPT_FLAG


def Initializer():
    logger.info("Resetting the directory...")
    subprocess.call('rm' + " " + 'main', shell=True)
    subprocess.call('rm' + " " + 'main.o', shell=True)
    subprocess.call('rm' + " " + pwd + '/opt_model_dir/*', shell=True)
    subprocess.call('cp' + " " + model_files_path +
                    '/*.sav' + " " + pwd + '/opt_model_dir/', shell=True)


def Compiler():
    try:
        logger.info('Compiling the solver with c++17...')
        #
        logger.info(' -Compiling pre-processor...')
        arg_base = '{0} {1} -I{2} -I{3} -c -o  model_refiner.o  contrib/pre_processor/model_refiner.cpp {4}'.format(
            GCC, FLAG, CPLEXDIR, CONCERTDIR, DEBUG_SYMBOLS)
        subprocess.call(arg_base, shell=True)
        #
        logger.info(' -Compiling docopt...')
        arg_base = '{0} {1} -std=c++17 -DNDEBUG -DIL_STD -c externals/docopt/docopt.cpp {2}'.format(
            GCC, FLAG, DEBUG_SYMBOLS)
        subprocess.call(arg_base, shell=True)
        #
        logger.info(' -Compiling rss...')
        arg_base = '{0} {1}  -std=c++17  -DIL_STD -c  externals/rss/current_rss.cpp  {2}'.format(
            GCC, FLAG, DEBUG_SYMBOLS)
        subprocess.call(arg_base, shell=True)
        #
        logger.info(' -Compiling BCompose...')
        arg_base = '{0} {1} -I{2} -I{3} -std=c++17  -DIL_STD -c  main.cpp {4}'.format(
            GCC, FLAG, CPLEXDIR, CONCERTDIR, DEBUG_SYMBOLS)
        subprocess.call(arg_base, shell=True)
        #
        logger.info(" -Linking...")
        arg_base = '{0} {1} -L{2} -L{3}  -std=c++17 -o main main.o model_refiner.o current_rss.o docopt.o  -ldl -lboost_system -lboost_thread-mt -lilocplex -lconcert -lcplex -lm -lpthread'.format(
            GCC, FLAG, CPLEXLIB, CONCERTLIB)
        subprocess.call(arg_base, shell=True)
    except Exception as e:
        logger.error(e)
        logger.info("Failed to compile...")


def Optimizer():
    try:
        logger.info(
            'Running the optimization command: ./main  --model_dir={0} --current_dir={1}'.format(model_files_path, pwd))
        logger.info('Starting the optimization...')
        #
        subprocess.call('./main  --model_dir=' + model_files_path +
                        ' --current_dir=' + pwd, shell=True)
        #
        logger.info('Finished the optimization.')
    except Exception as e:
        logger.error(e)
        logger.info("Optimization failed!")


def main():
    Initializer()
    Compiler()
    Optimizer()


if __name__ == '__main__':
    main()
