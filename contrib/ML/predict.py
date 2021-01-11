import pickle
import sys
import pandas as pd

def SetType(l, dtype=float):
    return list(map(dtype, l))

def Predict(y_lp, rc, y_lp_sum, dual, model="gboost"):
#     ["Dual","y_lp_sum", "RC", "ylp", "yint"]
    df=pd.DataFrame({ "RC":rc,"ylp":y_lp  })
#     try:
    loaded_model = pickle.load(open("contrib/ML/{}.sav".format(model), 'rb'))
    for col in df.columns.tolist():
        if col=="y_lp" or col=="y_int":
            continue
        scaler = pickle.load(open("contrib/ML/scaler_{}.sav".format(col), 'rb'))
        df[col]=scaler.transform(df[[col]])
    df["pred"] = loaded_model.predict(df)
    if model =="mlp":
        df["score"] = pd.DataFrame([i[1] for i in loaded_model.predict_proba(df.drop(["pred"],axis="columns"))])
        df["q"]=0.51 #0.51 means do not fix
        df.loc[df.score>df.score.quantile(0.75),["q"]]=df.pred.max()
        df.loc[df.score<df.score.quantile(0.25),["q"]]=df.pred.min()
    else:
        df["score"] = loaded_model.decision_function(df.drop("pred",axis="columns"))
        df["q"]=0.51 #0.51 means do not fix
        df.loc[df.score>df[df.score>0].score.quantile(0.5),["q"]]=df.pred.max()
        df.loc[df.score<df[df.score<0].score.quantile(0.5),["q"]]=df.pred.min()
#     except:
#         print("FATAL ERROR: Something went wrong with the prediction model. Please make sure a trained model exists or turn off the ML component.")
    df["Dual"]=0
    df["y_lp_sum"]=0
    df.to_csv("contrib/ML/predicted_values.csv", sep=',', encoding='utf-8', index=False)

y_lp = SetType(sys.argv[1].split(",")[:-1])
rc = SetType(sys.argv[2].split(",")[:-1])
y_lp_sum = SetType(sys.argv[3].split(",")[:-1])
dual = SetType(sys.argv[4].split(",")[:-1])
assert(len(rc)==len(y_lp))

Predict(y_lp, rc, y_lp_sum, dual)