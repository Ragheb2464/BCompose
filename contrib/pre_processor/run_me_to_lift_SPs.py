
__author_____ = 'Ragheb Rahmaniani'
__email______ = 'Ragheb.Rahmaniani@Gmail.Com'
__copyright__ = 'All Rights Reserved.'
__revision___ = '02.01.0isr'
__update_____ = '8/11/2018'
__note_______ = 'This python scripts can be used to run the algorithm.\
                 It can be used to solve R data set from SND problems'


import logging as logger
import subprocess

# logger.basicConfig(filename='solver.log', level=logger.INFO,
#                    format='%(asctime)s:%(levelname)s:%(message)s')
logger.basicConfig(level=logger.INFO,
                   format='[%(asctime)s]: [%(levelname)s]: %(message)s')

model_files_path = "/home/rahrag/Bureau/GeorgiaTech/ND/BDD/models"

subprocess.call('rm' + " " + 'old_fashion', shell=True)
# subprocess.call('rm' + " " + 'old_fashion.o', shell=True)
subprocess.call(
    'gcc -o old_fashion old_fashion.c -lilocplex -lconcert -lcplex -lm -lpthread', shell=True)

logger.error("!!!!!!!!!!!Automatize the cp and rm on the model\n")
try:
    logger.info('Starting the pre-processor...')
    ws = 0
    with open("SP_info.txt", "w") as file:
        for id in xrange(0, 64):
            out = subprocess.Popen(
                './old_fashion /home/rahrag/Bureau/GeorgiaTech/ND/BDD/contrib/lifted_SP_models/SP_' + str(id) + '.sav', stdout=subprocess.PIPE, shell=True)
            (output, error) = out.communicate()
            p_status = out.wait()
            ws += float(output) * 1.0 / 64.0
            file.write("SP_{0} = {1}\n".format(id, output))
            if error:
                print "Lifter ERROR: ", error
            logger.info(
                "  -Pre_processor improvedd SP_{0:03}\'s representation ".format(id) + u'\u2713')
        logger.info('Expected WS is ' + str(round(ws, 3)))
        logger.info('Pre_processor is exiting successfully!')
except Exception as e:
    logger.error(e)
    logger.info("Pre-processor failed!")
