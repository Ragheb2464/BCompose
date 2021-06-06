import sys
from data import ReadData
from model import Inference

df = ReadData(sys.argv[1])
Inference(df)
