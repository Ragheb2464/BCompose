
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
    model_files_path = "/Users/raghebrahmaniani/BCompose/models"
    pwd = "/Users/raghebrahmaniani/BCompose/models"
    CONCERTDIR = '/Applications/CPLEX_Studio129/concert/include/'
    CPLEXDIR = '/Applications/CPLEX_Studio129/cplex/include/'
    CPLEXLIB = '/Applications/CPLEX_Studio129/cplex/lib/x86-64_osx/static_pic/'
    CONCERTLIB = '/Applications/CPLEX_Studio129/concert/lib/x86-64_osx/static_pic/'
    Sanitation_Flags = 'address,undefined,signed-integer-overflow,null,alignment,bool,builtin,bounds,float-cast-overflow,float-divide-by-zero,function,integer-divide-by-zero,return,signed-integer-overflow,implicit-conversion,unsigned-integer-overflow -fno-sanitize-recover=null -fsanitize-trap=alignment -fno-omit-frame-pointer'
    GCC = 'g++'
    OPT_FLAG = '-Ofast'
    DEBUG_FLAG = '-DGLIBCXX_DEBUG -g -O -DIL_STD -Wall -Wextra -pedantic'
    # DEBUG_FLAG = '-DGLIBCXX_DEBUG -g -O -DIL_STD -Wall -Wextra -pedantic -fsanitize={0}'.format(
    #     Sanitation_Flags)
    FLAG = DEBUG_FLAG
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
        logger.info(' -Compiling docopt...')
        arg_base = '{0} {1} -std=c++17 -DNDEBUG -DIL_STD -c ../externals/docopt/docopt.cpp  -g'.format(
            GCC, FLAG)
        subprocess.call(arg_base, shell=True)
        logger.info(' -Compiling exporter...')
        arg_base = '{0} {1} -I{2} -I{3} -std=c++17  -DIL_STD -c  main.cpp  -g'.format(
            GCC, FLAG, CPLEXDIR, CONCERTDIR)
        subprocess.call(arg_base, shell=True)
        logger.info(" -Linking...")
        arg_base = '{0} {1} -L{2} -L{3}  -std=c++17 -o main main.o docopt.o  -ldl -lboost_system -lboost_thread-mt -lilocplex -lconcert -lcplex -lm -lpthread'.format(
            GCC, FLAG, CPLEXLIB, CONCERTLIB)
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
