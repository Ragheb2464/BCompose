import pandas as pd
import numpy as np
import warnings
import pickle

# import os
# import matplotlib.pyplot as plt

warnings.simplefilter("ignore")

from sklearn.model_selection import train_test_split

from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.preprocessing import MinMaxScaler


def ema(values):
    ewm = 0
    for i in values:
        ewm = (ewm + i) / 2
    return ewm


# df_=pd.concat([df,df[df.yInt==1].head(int(0.28*len(df)))])
def Split(df, t=0.25, v=0.0001):
    train, test = train_test_split(pd.concat([df, df[df.yInt == 1].head(
        int(max(0, len(df[df.yInt == 0]) - len(df[df.yInt == 1]))))]
                                             ), test_size=t, random_state=42)
    # print(100*train.yInt.value_counts(normalize=True))
    print("TEST", 100 * test.yInt.value_counts(normalize=True))
    train, val = train_test_split(train, test_size=v, random_state=42)
    print("TRAIN", 100 * train.yInt.value_counts(normalize=True))
    print("VAL", 100 * val.yInt.value_counts(normalize=True))
    return train, val, test


def FitTransformers(df, pwd="data/"):
    # ech problem should be scaled separately
    transformers = {}
    for col in df.columns:
        if col in ["yInt", "ylp", "y_lp_sum", "y_0", "y_1", "ylp_ema", "ylp_med"]:
            continue
        transformers[col] = StandardScaler().fit(df[[col]])
    pickle.dump(transformers, open(pwd + "transformers.pkl", 'wb'))


def TransformerData(df, pwd="data/"):
    transformers = pickle.load(open(pwd + "transformers.pkl", 'rb'))
    for col in transformers.keys():
        df[col] = transformers[col].transform(df[[col]])
    return df


def ReadData(file):
    df = pd.read_csv(file, sep=" ", header=None)
    isTraining = df.shape[1] == 5
    df.columns = ["ObjCoeff", "yLP", "Dual", "RC", "yInt"] if isTraining else ["ObjCoeff", "yLP", "Dual", "RC"]
    # print(df.head())
    # print(df.yInt.value_counts())
    # feature eng
    df["ylp_std"] = 0
    df["ylp_ema"] = 0
    df["y_0"] = 0
    df["y_1"] = 0
    #
    df["dual_std"] = 0
    df["dual_ema"] = 0
    df["dual_0"] = 0
    #
    df["rc_std"] = 0
    df["rc_0"] = 0
    # ACCUMULATION DISTRIBUTION LINE

    # ignore first t iterations
    t = 5 if isTraining else 0
    for idx, row in df.iterrows():
        yRow = np.float_(row.yLP.split("_")[t:-1])
        dRow = np.float_(row.Dual.split("_")[t:-1])
        rcRow = np.float_(row.RC.split("_")[t:-1])
        # print(yRow)
        assert (len(yRow) > 0 and "Using the ML before 5 iterations")
        # y
        df.loc[idx, "yLP"] = yRow.mean()
        df.loc[idx, "ylp_ema"] = ema(yRow)
        df.loc[idx, "ylp_std"] = yRow.std()
        df.loc[idx, "y_0"] = len([i for i in yRow if abs(i) <= 0.25]) / len(yRow)
        df.loc[idx, "y_1"] = len([i for i in yRow if abs(i) >= 0.75]) / len(yRow)
        # duals
        df.loc[idx, "Dual"] = dRow.mean()
        df.loc[idx, "dual_ema"] = ema(dRow)
        df.loc[idx, "dual_std"] = dRow.std()
        df.loc[idx, "dual_0"] = len([i for i in dRow if abs(i) < 0.1]) / len(dRow)
        # RC
        df.loc[idx, "RC"] = rcRow.mean()
        df.loc[idx, "rc_std"] = rcRow.std()
        df.loc[idx, "rc_0"] = len([i for i in rcRow if (i) < 0]) / len(rcRow)
    # set types
    col_list = ["ObjCoeff",
                "yLP", "ylp_ema", "ylp_std", "y_0", "y_1",
                "Dual", "dual_ema", "dual_std", "dual_0"
        , "RC", "rc_std", "rc_0"]
    df[col_list] = df[col_list].astype("float")
    df.drop(["Dual", "yLP","rc_std","rc_0","dual_std","dual_0","ylp_std", "y_0", "y_1"], axis="columns", inplace=True)
    # drop
    df.dropna(inplace=True)
    # print(df.shape)
    # print(df.columns)
    # info
    if isTraining:
        df["yInt"] = df["yInt"].fillna(0).astype("int")
        print(df.yInt.value_counts())
        print(df.corr().yInt.sort_values())
        # print("Fractionality dum", abs(df.yLP - df.yInt).sum())
        #
        train, val, test = Split(df)
        FitTransformers(train)
        train = TransformerData(train)
        val = TransformerData(val)
        test = TransformerData(test)
        return train, val, test
    else:
        df = TransformerData(df, "contrib/ML/data/")
        return df
