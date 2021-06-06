import sys
from data import ReadData
from model import TrainModel


def main(train_data):
    train, val, test = ReadData(train_data)
    TrainModel(train, val, test)


if __name__ == '__main__':
    main(sys.argv[1])
