import pandas as pd
import numpy as np
import pickle

from sklearn import svm
from sklearn.datasets import make_classification
from sklearn.linear_model import LogisticRegression
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.neural_network import MLPClassifier

from sklearn.metrics import classification_report, confusion_matrix, auc, roc_curve
from sklearn.model_selection import GridSearchCV
from sklearn.inspection import permutation_importance


# funcs
def QuntileScore(df):
    df["pred"] = -1
    df.loc[df.score >= df.score.quantile(0.75), "pred"] = 1
    df.loc[df.score <= df.score.quantile(0.25), "pred"] = 0
    print(len(df[df.pred != -1]) / len(df),
          1 - abs(df[df.pred != -1].yInt - df[df.pred != -1].pred).sum() / len(1e-10 + df[df.pred != -1]))


def PrintStat(model, train, test):
    trainScore = 100 * model.score(train.drop("yInt", axis="columns"), train.yInt)
    testScore = 100 * model.score(test.drop("yInt", axis="columns"), test.yInt)
    print("Score", trainScore, testScore)

    fpr_gb, tpr_gb, _ = roc_curve(test.yInt, model.decision_function(test.drop("yInt", axis="columns")))
    fpr_gb_, tpr_gb_, _ = roc_curve(train.yInt, model.decision_function(train.drop("yInt", axis="columns")))
    # fpr_gb__, tpr_gb__, _ = roc_curve(df.yInt, model.decision_function(df.drop("yInt", axis="columns")))
    print("AUC", auc(fpr_gb_, tpr_gb_), auc(fpr_gb, tpr_gb))  # , "=>", auc(fpr_gb__, tpr_gb__))
    print("Quantiles")
    train["score"] = model.predict_proba(train.drop("yInt", axis="columns"))[:, 1]
    QuntileScore(train)
    test["score"] = model.predict_proba(test.drop("yInt", axis="columns"))[:, 1]
    QuntileScore(test)


def PrintPermutationImportance(clfs, df):
    #     pass
    # pickle.dump(clf, open("gboost.sav", 'wb'))
    r = permutation_importance(clfs, df.drop("yInt", axis="columns"), df.yInt
                               , n_repeats=100
                               #                                , n_jobs=-1
                               , random_state=42
                               )
    for i in r.importances_mean.argsort()[::-1]:
        print(np.round(r.importances_mean[i], 5)
              #               r.importances_std[i],
              , r.importances_mean[i] - 2 * r.importances_std[i] > 0
              , df.drop('yInt', axis='columns').columns[i],
              )


def PrintFeatureImportance(clfs, train):
    l = []
    for i in range(len(clfs.feature_importances_)):
        l.append((clfs.feature_importances_[i], train.drop("yInt", axis="columns").columns[i]))
    print(sorted(l))


def Threshold(clfs, df):
    # print(df.columns)
    df["score"] = clfs.decision_function(df.drop(["yInt", "score", "pred"], axis="columns"))
    c_0 = df[df.yInt == 0].score.count()
    print("++++++++++++++++++++")
    # print(df.score)
    # print(int(df.score.min()) - 1, int(df.score.max()) + 1)
    m_ = len(df)
    for tt in range(-1000 , 1000):
        t = tt / 1000
        m_0 = df[(df.score <= t) & (df.yInt == 1)].score.count() + df[(df.score > t) & (df.yInt == 0)].score.count()
        if m_0 < m_:
            print(t, 100 * (1 - m_0 / df.score.count()))
            m_ = m_0


def TrainModel(train, val, test):
    clfs = GradientBoostingClassifier(random_state=42
                                      # , learning_rate=0.02
                                      , n_estimators=200
                                      , max_depth=4
                                      #                                  ,min_samples_split=0.9
                                      #                                  ,subsample=0.7
                                      ).fit(train.drop("yInt", axis="columns"), train.yInt)
    PrintStat(clfs, train, test)
    pickle.dump(clfs, open("data/GradientBoostingClassifier.pkl", 'wb'))
    # Threshold(clfs, pd.concat([train, test]))


def Inference(df):
    model = pickle.load(open("contrib/ML/data/GradientBoostingClassifier.pkl", 'rb'))

    df["pred"] = model.predict(df)
    df["score"] = model.decision_function(df.drop("pred", axis="columns"))
    # df.loc[df.score < -1.496, "pred"] = 0
    # print(df["score"])
    df["q"] = 0.51  # 0.51 means do not fix
    df.loc[df.score > df[df.score > 0].score.quantile(0.5), ["q"]] = df.pred.max()
    df.loc[df.score < df[df.score < 0].score.quantile(0.5), ["q"]] = df.pred.min()
    df[["score", "pred", "q"]].to_csv("contrib/ML/data/predicted_values.csv", sep=',', encoding='utf-8', index=False)
